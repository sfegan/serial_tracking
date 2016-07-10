//-*-mode:c++; mode:font-lock;-*-

/**
 * \file TelescopeControllerLocal.cpp
 * \ingroup VTracking
 * \brief TelecopeController which does actual communication with telescope
 *
 * This derived class communiactes with the telescope, updating
 * and logging the position and accepting commands from the GUI, UI
 * and CORBA.
 *
 * Original Author: Stephen Fegan
 * $Author: aune $
 * $Date: 2011/10/06 02:14:28 $
 * $Revision: 2.16 $
 * $Tag$
 *
 **/

#include<sys/time.h>
#include<string.h>
#include<pthread.h>

#include<iostream>
#include<iomanip>
#include<fstream>
#include<sstream>
#include<string>

#include<zthread/Thread.h>
#include<zthread/Guard.h>

#include<Exception.h>
#include<Debug.h>
#include<Message.h>
#include<Messenger.h>

#include"DataStream.h"
#include"TelescopeControllerLocal.h"
#include"PositionLogger.h"

using namespace SEphem;
using namespace VTracking;
using namespace VMessaging;

// ----------------------------------------------------------------------------
// Some static functions used here
// ----------------------------------------------------------------------------

static bool closeEnough(double az1_rad, double el1_rad,
                        double az2_rad, double el2_rad)
{
#if 0
  Debug::stream()
    << Angle::toDeg(az1_rad) << ' ' << Angle::toDeg(az2_rad) << ' '
    << Angle::toDeg(el1_rad) << ' ' << Angle::toDeg(el2_rad)
    << std::endl;
#endif
#if 0
  if((Angle::makeRad(az1_rad-az2_rad).deg180()*cos((el1_rad+el2_rad)/2)<0.01)
     &&(Angle::makeRad(el1_rad-el2_rad).deg180()<0.01))return true;
#endif
  if((Angle::makeRad(az1_rad-az2_rad).deg180()<0.03)
     &&(Angle::makeRad(el1_rad-el2_rad).deg180()<0.02))return true;
  return false;
}

// ----------------------------------------------------------------------------
// TelescopeControllerLocal
// ----------------------------------------------------------------------------

const double TelescopeControllerLocal::sc_decay_rate;

TelescopeControllerLocal::
TelescopeControllerLocal(ScopeAPI* scope_api, TelescopeMotionLimits* limits,
			 const SEphem::SphericalCoords& earth_pos,
			 int sleep_time, int phase_time,
			 const CorrectionParameters& cp,
			 PositionLogger* logger, time_t logger_timeout_sec,
			 ZThread::Condition* terminate_notifier,
			 bool realtime):
  TelescopeController(limits, earth_pos, sleep_time, phase_time, 
		      cp, terminate_notifier),
  m_scope_api(scope_api), m_anticipation(double(sleep_time)/86400000.0), 
  m_close_count(0), m_ramp_down_mjd(0), m_ramp_down_az(0), m_ramp_down_el(0),
  m_logger_last_loggable_event(), m_logger_timeout_sec(logger_timeout_sec), 
  m_logger_active(true), m_logger(logger), 
  m_emergency_stop(false), m_void_vff_positions(false), m_realtime(realtime)
{
  m_tse.cf=CF_SCOPE;
  m_logger_last_loggable_event = time(0);
}

TelescopeControllerLocal::~TelescopeControllerLocal()
{
  // nothing to see here
}

std::string TelescopeControllerLocal::controllerName() const
{
  return m_scope_api->apiName();
}

void TelescopeControllerLocal::loopIsStarting()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(m_realtime)
    {
      if(seteuid(0) < 0)
	{
	  Message message(Message::DR_GLOBAL,Message::PS_EXCEPTIONAL,
			  "Cannot set user ID to ROOT");
	  message.messageStream() 
	    << "Error occurred when trying to set effective user ID to ROOT.";
	  message.detailsStream() 
	    << strerror(errno);
	  Messenger::relay()->sendMessage(message);
	  return;
	}

      pthread_t tid = pthread_self();
      int policy = SCHED_FIFO;
      struct sched_param param;
      param.sched_priority = sched_get_priority_max(policy);
      int error = pthread_setschedparam(tid, policy, &param);
      if(error != 0)
	{
	  Message message(Message::DR_GLOBAL,Message::PS_EXCEPTIONAL,
			  "Realtime scheduler error");
	  message.messageStream() 
	    << "Error occurred when trying to set realtime scheduling for "
	    << "controller loop.";
	  message.detailsStream() 
	    << strerror(error);
	  Messenger::relay()->sendMessage(message);
	  return;
	}
      else
	{
	  Message message(Message::DR_GLOBAL,Message::PS_ROUTINE,
			  "Set realtime scheduler");
	  message.messageStream() 
	    << "Telescope controller loop successfully set realtime "
	    << "scheduling.";
	  Messenger::relay()->sendMessage(message);
	}
      setuid(getuid());
    }
}

void TelescopeControllerLocal::loopIsTerminating()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  m_scope_api->cmdStandby();
}

void TelescopeControllerLocal::iterate()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  StateElements tse;
  TrackingRequest lastReq;

  // --------------------------------------------------------------------------
  // Make local copy of shared TSE object while holding mutex 
  // --------------------------------------------------------------------------

  {
    ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

    if(m_emergency_stop)
      {
	// --------------------------------------------------------------------
	// Emergency stop -- send standby command and make quick exit from loop
	// --------------------------------------------------------------------

	try
	  {
	    m_scope_api->cmdStandby();
	  }
	catch(...)
	  {
	    // nothing to see here
	  }
	return;
      }
    tse=m_tse;
  }
  
  lastReq=tse.req;
  
  bool started_with_com_failure = tse.state==TS_COM_FAILURE;
  
  tse.last_has_object = tse.has_object;
  tse.last_az_driveangle_deg = tse.status.az.driveangle_deg;
  tse.last_el_driveangle_deg = tse.status.el.driveangle_deg;
  tse.last_cmd_az_driveangle_deg = tse.cmd_az_driveangle_deg;
  tse.last_cmd_el_driveangle_deg = tse.cmd_el_driveangle_deg;
  tse.last_mjd = tse.mjd;
  
  // --------------------------------------------------------------------------
  // RECOVER FROM COM_FAILURE
  // --------------------------------------------------------------------------

  if(tse.state==TS_COM_FAILURE)
    {
      try
	{
	  m_scope_api->resetCommunication();
	  m_scope_api->cmdStandby();

	  ScopeAPI::PIDParameters az_param;
	  ScopeAPI::PIDParameters el_param;
	  m_scope_api->reqAzPIDParameters(az_param);
	  m_scope_api->reqElPIDParameters(el_param);

	  if(az_param.vlim>m_limits->maxAzSpeedDPS())
	    tse.az_slew_speed_dps = m_limits->maxAzSpeedDPS();
	  else
	    tse.az_slew_speed_dps = az_param.vlim;
	  
	  if(el_param.vlim>m_limits->maxElSpeedDPS())
	    tse.el_slew_speed_dps = m_limits->maxElSpeedDPS();
	  else
	    tse.el_slew_speed_dps = el_param.vlim;
	  
	  if(az_param.vlim>m_limits->maxAzSpeedDPS())
	    {
	      Message message(Message::DR_GLOBAL,Message::PS_UNUSUAL,
			      "Warning");
	      message.messageStream() 
		<< "Telescope AZ maximum speed " << az_param.vlim
		<< "dps is higher than maximum programmed value "
		<< m_limits->maxAzSpeedDPS() << "dps.";
	      Messenger::relay()->sendMessage(message);
	    }
	  
	  if(el_param.vlim>m_limits->maxElSpeedDPS())
	    {
	      Message message(Message::DR_GLOBAL,Message::PS_UNUSUAL,
			      "Warning");
	      message.messageStream() 
		<< "Telescope EL maximum speed " << el_param.vlim
		<< "dps is higher than maximum programmed value "
		<< m_limits->maxElSpeedDPS() << "dps.";
	      Messenger::relay()->sendMessage(message);
	    }	  

	  if((m_logger)&&(tse.req!=REQ_STOP))
	    {
	      AzElObject obj(tse.tel_azel);
	      m_logger->logCommand(PositionLogger::C_STOP,tse.mjd, tse.lmst, getEarthPos(), &obj);
	    }

	  tse.state = TS_STOP;
	  tse.req = REQ_STOP;
	}
      catch(const VTracking::Timeout& x)
	{
	  tse.state=TS_COM_FAILURE;
	}
      catch(const Exception& x)
	{
	  Message message(x,Message::DR_GLOBAL,Message::PS_EXCEPTIONAL);
	  message.detailsStream() << "Locator: 1";
	  Messenger::relay()->sendMessage(message);
	  tse.state=TS_COM_FAILURE;
	}
      catch(...)
	{
	  Message message(Message::DR_GLOBAL,Message::PS_EXCEPTIONAL,
			  "Unknown exception");
	  message.messageStream() << "Caught unknown exception";
	  message.detailsStream() << "Locator: 1";
	  tse.state=TS_COM_FAILURE;
	}
    }

  // --------------------------------------------------------------------------
  // GET TELESCOPE STATUS
  // --------------------------------------------------------------------------
  
  double tel_az_driveangle=0;
  double tel_el_driveangle=0;
  
  if(tse.state!=TS_COM_FAILURE)
    {
      try
	{
	  m_scope_api->reqStat(tse.status);
	  
	  if(!tse.status.checksumOK)
	    throw TCException("Controller Checksum Error");
      
#warning REMOTECONTROL
#if 0
	  if((tse.status.interlock)||
	     (tse.status.interlockAzPullCord)||
	     (tse.status.interlockAzStowPin)||
	     (tse.status.interlockElStowPin)||
	     (tse.status.interlockAzDoorOpen)||
	     (tse.status.interlockElDoorOpen)||
	     (tse.status.interlockSafeSwitch)||
	     (!tse.status.remoteControl))
#else
	  if((tse.status.interlock)||
	     (tse.status.interlockAzPullCord)||
	     (tse.status.interlockAzStowPin)||
	     (tse.status.interlockElStowPin)||
	     (tse.status.interlockAzDoorOpen)||
	     (tse.status.interlockElDoorOpen)||
	     (tse.status.interlockSafeSwitch)||
	     0)
#endif
	    {
	      if((m_logger)&&(tse.req!=REQ_STOP))
		{
		  AzElObject obj(tse.tel_azel);
		  m_logger->logCommand(PositionLogger::C_STOP, tse.mjd,  tse.lmst, getEarthPos(), &obj);
		}
	      tse.state = TS_STOP;
	      tse.req = REQ_STOP;
	    }
      
#if 0
	  if((tse.status.msgBadFrame)||/*(tse.status.msgCommandInvalid)||*/
	     (tse.status.msgInputOverrun)||(tse.status.msgOutputOverrun))
	    tse.state = TS_COM_FAILURE;
#endif
 
	  tel_az_driveangle = Angle::frDeg(tse.status.az.driveangle_deg);
	  tel_el_driveangle = Angle::frDeg(tse.status.el.driveangle_deg);
	  
	  double az = tel_az_driveangle;
	  double el = tel_el_driveangle;
      
	  {
	    ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
	    m_corrections.undoAzElCorrections(az,el,true);
	  }

	  tse.tel_azel = SphericalCoords::makeLatLong(el,az);
	}
      catch(const VTracking::Timeout& x)
	{
	  Message message(x,Message::DR_GLOBAL,Message::PS_UNUSUAL);
	  message.detailsStream() << "Locator: 2";
	  Messenger::relay()->sendMessage(message);

	  tse.state=TS_COM_FAILURE;
	}
      catch(const Exception& x)
	{
	  Message message(x,Message::DR_GLOBAL,Message::PS_EXCEPTIONAL);
	  message.detailsStream() << "Locator: 2";
	  Messenger::relay()->sendMessage(message);

	  tse.state=TS_COM_FAILURE;
	}
      catch(...)
	{
	  Message message(Message::DR_GLOBAL,Message::PS_EXCEPTIONAL,
			  "Unknown exception");
	  message.messageStream() << "Caught unknown exception";
	  Messenger::relay()->sendMessage(message);

	  tse.state=TS_COM_FAILURE;
	}
    }
  
  // --------------------------------------------------------------------------
  // CALCULATE THE TIMES AND TARGET LOCATION
  // --------------------------------------------------------------------------
  
  getTimes(tse.tv, tse.mjd, tse.timeangle, tse.lmst);
  
  double calc_mjd = tse.mjd;
  Angle calc_lmst = tse.lmst;

  addAnticipation(calc_mjd, calc_lmst, started_with_com_failure);
  tse.anticipation=m_anticipation;
  
  double target_az_driveangle;
  double target_el_driveangle;

  double az_speed = m_limits->maxAzSpeedDPS();
  double el_speed = m_limits->maxElSpeedDPS();
  
  bool target_in_limits = false;
  bool restricted_motion = false;

  tse.cmd_az_driveangle_deg = tse.status.az.driveangle_deg;
  tse.cmd_el_driveangle_deg = tse.status.el.driveangle_deg;

  {
    ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
    if(m_target_object)
      {
	tse.has_object = true;

	tse.obj_azel =
	  m_target_object->getAzEl(calc_mjd,calc_lmst,m_earth_position);
	
	target_az_driveangle = tse.obj_azel.phiRad();
	target_el_driveangle = tse.obj_azel.latitudeRad();
	
	bool target_is_reachable =
	  m_corrections.
	  doAzElCorrections(target_az_driveangle,target_el_driveangle,
			    tel_az_driveangle, 
			    m_target_object->useCorrections(),
			    m_direction_preference);
	
	target_in_limits = target_is_reachable &&
	  m_limits->isDriveAngleInsideLimits(target_az_driveangle, 
					     target_el_driveangle);

	// SJF 2006-04-04 -- why is this "if" here
	//if((tse.state==TS_TRACK)||(tse.state==TS_SLEW))
	//{
	tse.cmd_az_driveangle_deg = Angle::toDeg(target_az_driveangle);
	tse.cmd_el_driveangle_deg = Angle::toDeg(target_el_driveangle);
	//}

	if((m_corrections.enable_vff)&&(tse.last_has_object)&&
	   (!m_void_vff_positions))
	  {
	    double dt = (tse.mjd-tse.last_mjd)*86400.0;
	    double tar_daz = 
	      target_az_driveangle
	      - Angle::frDeg(tse.last_cmd_az_driveangle_deg);
	    double tar_del = 
	      target_el_driveangle
	      - Angle::frDeg(tse.last_cmd_el_driveangle_deg);
	    double tar_az_vel = tar_daz/dt;
	    double tar_el_vel = tar_del/dt;

	    double vff_az = 0;
	    double vff_el = 0;

	    if(tar_az_vel > m_corrections.az_pos_vff_t)
	      vff_az = 
		m_corrections.az_pos_vff_s*(tar_az_vel-m_corrections.az_pos_vff_t);
	    else if(tar_az_vel < m_corrections.az_neg_vff_t)
	      vff_az = 
		m_corrections.az_neg_vff_s*(tar_az_vel-m_corrections.az_neg_vff_t);

	    if(tar_el_vel > m_corrections.el_pos_vff_t)
	      vff_el = 
		m_corrections.el_pos_vff_s*(tar_el_vel-m_corrections.el_pos_vff_t);
	    else if(tar_el_vel < m_corrections.el_neg_vff_t)
	      vff_el = 
		m_corrections.el_neg_vff_s*(tar_el_vel-m_corrections.el_neg_vff_t);

	    target_az_driveangle += vff_az;
	    target_el_driveangle += vff_el;

#if 0
	    Debug::stream()
	      << dt << ' '
	      << Angle::toDeg(tar_az_vel) << ' ' 
	      << Angle::toDeg(tar_el_vel) << ' ' 
	      << Angle::toDeg(vff_az) << ' ' 
	      << Angle::toDeg(vff_el) << std::endl;
#endif
	  }
	else
	  {
#if 0
	    Debug::stream()
	      << m_corrections.enable_vff << ' ' 
	      << tse.last_has_object << ' ' 
	      << m_void_vff_positions << std::endl;
#endif
	    m_void_vff_positions = false;
	  }

	restricted_motion = 
	  m_limits->modifyTargetDriveAngle(tel_az_driveangle, 
					   tel_el_driveangle,
					   target_az_driveangle, 
					   target_el_driveangle,
					   az_speed, el_speed);
      }
    else
      {
	m_void_vff_positions = false;
	tse.has_object = false;
	tse.obj_azel = SphericalCoords::makeLatLongDeg(0,0);
	tse.req = REQ_STOP;
     }
  }

  if(target_az_driveangle>tel_az_driveangle+Angle::frDeg(45))
    target_az_driveangle = tel_az_driveangle+Angle::frDeg(45);
  else if(target_az_driveangle<tel_az_driveangle-Angle::frDeg(45))
    target_az_driveangle = tel_az_driveangle-Angle::frDeg(45);
  
  // --------------------------------------------------------------------------
  // ADJUST STATE TO ACCOMODATE REQUEST
  // --------------------------------------------------------------------------

  switch(tse.req)
    {
    case REQ_STOP:
      switch(tse.state)
	{
	case TS_STOP:
	  break;

	case TS_RESTRICTED_MOTION:
	case TS_SLEW:
	case TS_TRACK:
	  m_ramp_down_mjd = tse.mjd;
	  m_ramp_down_az = Angle::makeRad(tel_az_driveangle);
	  m_ramp_down_el = Angle::makeRad(tel_el_driveangle);
	case TS_RAMP_DOWN:
	  tse.state=TS_RAMP_DOWN;
	  break;

	case TS_COM_FAILURE:
	  break;
	}
      break; 

    case REQ_SLEW:
    case REQ_TRACK:
      switch(tse.state)
	{
	case TS_STOP:
	case TS_RESTRICTED_MOTION:
	case TS_RAMP_DOWN:
	case TS_SLEW:
	case TS_TRACK:
	  if(target_in_limits&&restricted_motion)
	    {
	      tse.state=TS_RESTRICTED_MOTION;
	    }
	  else if(target_in_limits)
	    {
	      tse.state=(tse.state==TS_TRACK)?TS_TRACK:TS_SLEW;
	    }
	  else if(tse.state==TS_RAMP_DOWN)
	    {
	      if(m_logger)
		{
		  AzElObject obj(tse.tel_azel);
		  m_logger->logCommand(PositionLogger::C_STOP, tse.mjd, tse.lmst, getEarthPos(), &obj);
		}
	      tse.req = REQ_STOP;
	    }
	  else 
	    {
	      if(m_logger)
		{
		  AzElObject obj(tse.tel_azel);
		  m_logger->logCommand(PositionLogger::C_STOP, tse.mjd, tse.lmst, getEarthPos(), &obj);
		}
	      tse.state = TS_RAMP_DOWN;
	      tse.req = REQ_STOP;
	    }
	  break;
	case TS_COM_FAILURE:
	  if(m_logger)
	    {
	      AzElObject obj(tse.tel_azel);
	      m_logger->logCommand(PositionLogger::C_STOP, tse.mjd, tse.lmst, getEarthPos(), &obj);
	    }
	  tse.req = REQ_STOP;
	  break;
	}
    }
  
  // --------------------------------------------------------------------------
  // SEND COMMANDS TO TELESCOPE
  // --------------------------------------------------------------------------

  try
    {
      switch(tse.state)
	{
	case TS_STOP:
	  m_scope_api->cmdStandby();
	  break;

	case TS_RESTRICTED_MOTION:
	case TS_SLEW:
	  if(closeEnough(tel_az_driveangle, tel_el_driveangle,
			 target_az_driveangle,target_el_driveangle))
	    {
	      if(m_close_count>10)
		{
		  if(tse.req==REQ_TRACK)
		    {
		      tse.state=TS_TRACK;
		      // SJF 2010-04-20: Forget operational direction
		      //                 preference when target reached
		      m_direction_preference =
			SEphem::CorrectionParameters::DP_NONE;
		    }
		  else 
		    {
		      tse.state = TS_STOP;
		      tse.req = REQ_STOP;
		    }
		  m_close_count=0;
		}
	      m_close_count++;
	    }
	  else m_close_count=0;
	  m_scope_api->cmdPoint(Angle::makeRad(target_az_driveangle), 
				az_speed,
				Angle::makeRad(target_el_driveangle), 
				el_speed);
	  break;

	case TS_TRACK:
	  if(!closeEnough(tel_az_driveangle, tel_el_driveangle,
			  target_az_driveangle,target_el_driveangle))
	    tse.state=TS_SLEW;
	  m_scope_api->cmdPoint(Angle::makeRad(target_az_driveangle),
				az_speed,
				Angle::makeRad(target_el_driveangle),
				el_speed);
	  break;

	case TS_RAMP_DOWN:
	  // Ramp down period ends after a maximum of 10 seconds or when
	  // the telescope has not moved significanly since the last iteration
	  if((tse.mjd - m_ramp_down_mjd > 10.0/86400.0)
	     ||(closeEnough(tel_az_driveangle, tel_el_driveangle,
			    Angle::frDeg(tse.last_az_driveangle_deg),
			    Angle::frDeg(tse.last_el_driveangle_deg))))
	    {
	      tse.state = TS_STOP;
	      tse.req = REQ_STOP;
	      m_scope_api->cmdStandby();
	      m_ramp_down_mjd = 0;
	      m_ramp_down_az = Angle::makeRad(0);
	      m_ramp_down_el = Angle::makeRad(0);
	    }
	  else
	    {
	      m_scope_api->cmdPoint(m_ramp_down_az, az_speed,
				    m_ramp_down_el, el_speed);
	    }
	  break;
	  
	case TS_COM_FAILURE:
	  m_scope_api->cmdStandby();
	  break;
	}
    }
  catch(const VTracking::Timeout& x)
    {
      if(tse.state!=TS_COM_FAILURE)
	{
	  Message message(x,Message::DR_GLOBAL,Message::PS_UNUSUAL);
	  message.detailsStream() << "Locator: 3";
	  Messenger::relay()->sendMessage(message);
	}
      tse.state=TS_COM_FAILURE;
    }
  catch(const Exception& x)
    {
      Message message(x,Message::DR_GLOBAL,Message::PS_EXCEPTIONAL);
      message.detailsStream() << "Locator: 3";
      Messenger::relay()->sendMessage(message);

      tse.state=TS_COM_FAILURE;
    }
  catch(...)
    {
      Message message(Message::DR_GLOBAL,Message::PS_EXCEPTIONAL,
		      "Unknown exception");
      message.messageStream() << "Caught unknown exception";
      message.detailsStream() << "Locator: 3";
      tse.state=TS_COM_FAILURE;
    }

  if(started_with_com_failure)
    tse.last_az_driveangle_deg = tse.status.az.driveangle_deg,
      tse.last_el_driveangle_deg = tse.status.el.driveangle_deg;

  tse.az_driveangle_estimated_speed_dps = 
    Angle::makeDeg(tse.status.az.driveangle_deg-tse.last_az_driveangle_deg).
    degPM()/(tse.mjd-tse.last_mjd)/86400;
  tse.el_driveangle_estimated_speed_dps =
    Angle::makeDeg(tse.status.el.driveangle_deg-tse.last_el_driveangle_deg).
    degPM()/(tse.mjd-tse.last_mjd)/86400;

  {
    ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

    // The user could have requested a new state while we were in this
    // function, so if we didn't change the requested state here then
    // let the user's request stand... otherwise override. This will
    // eliminate the need to double (triple...) click on the go button
    // from time to time.

    if(lastReq == tse.req)tse.req=m_tse.req;
    m_tse=tse;

    if(tse.req!=REQ_STOP)m_logger_last_loggable_event=tse.tv.tv_sec;
    if(m_logger)
      {
	if((m_logger_timeout_sec==0)
	   ||(tse.tv.tv_sec-m_logger_last_loggable_event<m_logger_timeout_sec))
	  {
	    if(!m_logger_active)
	      {
		m_logger_active = true;
		Message message(Message::DR_GLOBAL,Message::PS_ROUTINE,
				"Starting position status logging");
		message.messageStream() 
		  << "Logging of telescope positions has been restarted.";
		Messenger::relay()->sendMessage(message);
	      }
	    m_logger->logStatus(tse);
	  }
	else
	  {
	    if(m_logger_active)
	      {
		m_logger_active = false;
		Message message(Message::DR_GLOBAL,Message::PS_ROUTINE,
				"Pausing position status logging");
		message.messageStream() 
		  << "Logging of telescope positions has been paused as no\n"
		  << "telescope activity has taken place in the last "
		  << m_logger_timeout_sec << " seconds.\n"
	      << "Logging will restart automatically on receipt of a command.";
		Messenger::relay()->sendMessage(message);
	      }
	  }
      }
  }
}
  
void TelescopeControllerLocal::emergencyStop()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);  
  // Immediately command the mount to Standby, enable emergency stop mode
  // so that future iterations will continue to only send Standby commands
  // and finally flag the main thread to shut everything down. Finally send
  // a message in case the request comes remotely and the user is completely 
  // bewildered.
  m_scope_api->cmdStandby();
  m_emergency_stop = true;
  sendTerminateNotification();
  
  Message message(Message::DR_GLOBAL,Message::PS_UNUSUAL, 
		  "Emergency Stop");
  message.messageStream() 
    << "Emergency stop was called. Telescope controller loop has requested" 
    << std::endl
    << "that the program exit. Have a nice day!"
    << std::endl;
  Messenger::relay()->sendMessage(message);

}

void TelescopeControllerLocal::
setTargetObject(TargetObject* obj, DirectionPreference dp) 
{ 
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  if(m_tse.req != REQ_STOP)return;
  delete(m_target_object); 
  m_target_object=obj; 
  m_direction_preference=dp;
  if(!obj)m_tse.req = REQ_STOP; 
  // So that Vff does not get bogus target speed by using last target
  // position from previous target
  m_logger_last_loggable_event=m_tse.tv.tv_sec;
  m_void_vff_positions = true;
}

void TelescopeControllerLocal::reqStop() 
{ 
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  m_tse.req = REQ_STOP; 
  if(m_logger)
    {
      AzElObject obj(m_tse.tel_azel);
      m_logger->logCommand(PositionLogger::C_STOP, m_tse.mjd, m_tse.lmst, getEarthPos(), &obj);
    }
  m_logger_last_loggable_event=m_tse.tv.tv_sec;
}

void TelescopeControllerLocal::reqSlew() 
{ 
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  if(m_target_object)
    {
      m_tse.req = REQ_SLEW;
      if(m_logger)
	m_logger->logCommand(PositionLogger::C_SLEW, m_tse.mjd, m_tse.lmst, getEarthPos(),
			     m_target_object);
    }
  m_logger_last_loggable_event=m_tse.tv.tv_sec;
}

void TelescopeControllerLocal::reqTrack() 
{ 
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  if(m_target_object)
    {
      m_tse.req = REQ_TRACK;
      if(m_logger)
	m_logger->logCommand(PositionLogger::C_TRACK, m_tse.mjd, m_tse.lmst, getEarthPos(),
			     m_target_object);
    }
  m_logger_last_loggable_event=m_tse.tv.tv_sec;
}

void TelescopeControllerLocal::
setCorrectionParameters(const CorrectionParameters& cp) 
{ 
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  m_corrections=cp; 
  // So that Vff does not get bogus target speed by using last target
  // position from previous target
  m_void_vff_positions = true;
  m_logger_last_loggable_event=m_tse.tv.tv_sec;
}

void TelescopeControllerLocal::
addAnticipation(double& mjd, SEphem::Angle& lmst, bool started_with_comfailure)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
#if 0 // OLD anticipation calculation from before the PLL
  static double last_mjd = 0;
  if((!started_with_comfailure)&&(m_tse.state!=TS_COM_FAILURE))
    {
      m_anticipation = 
	sc_decay_rate*m_anticipation + (1-sc_decay_rate)*(mjd-last_mjd);
      double delta = m_anticipation/2.0;
      last_mjd=mjd;
      mjd+=delta;
      lmst=lmst+1.00273790935*Angle::frRot(delta);
    }
  else last_mjd=mjd;
#else
  m_anticipation = double(iterationPeriod())/86400000.0;
  mjd += m_anticipation;
  lmst = lmst + 1.00273790935*Angle::frRot(m_anticipation);
#endif
}

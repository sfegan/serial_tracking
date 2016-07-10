//-*-mode:c++; mode:font-lock;-*-

/**
 * \file TelescopeControllerRemote.cpp
 * \ingroup VTracking
 * \brief TelecopeController which does actual communication with telescope
 *
 * This derived class communiactes with the telescope, updating
 * and logging the position and accepting commands from the GUI, UI
 * and CORBA.
 *
 * Original Author: Stephen Fegan
 * $Author: sfegan $
 * $Date: 2010/10/28 14:48:06 $
 * $Revision: 2.14 $
 * $Tag$
 *
 **/

#include<sys/time.h>

#include<iostream>
#include<iomanip>
#include<fstream>
#include<sstream>
#include<string>

#include<zthread/Thread.h>
#include<zthread/Guard.h>

#include<Exception.h>
#include<Message.h>
#include<Messenger.h>
#include<Debug.h>

#include"DataStream.h"
#include"TelescopeControllerRemote.h"
#include"PositionLogger.h"
#include"NET_TCInterfaceServant.h"

using namespace ZThread;
using namespace SEphem;
using namespace VTracking;
using namespace VMessaging;

// ----------------------------------------------------------------------------
// TelescopeControllerRemote
// ----------------------------------------------------------------------------

TelescopeControllerRemote::
TelescopeControllerRemote(NET_TCInterface_ptr tc_interface, 
			  int sleep_time, int phase_time,
			  bool readonly, bool no_stop_on_entry_error_exit,
			  const SEphem::SphericalCoords& earth_pos_zero):
  TelescopeController(0, earth_pos_zero, sleep_time, phase_time,
		      CorrectionParameters()),
  m_ref_source(RS_PROVIDED), m_orb(), m_scope_num(-1), m_ior(),
  m_tc_interface(tc_interface), m_local_com_failure(true),
  m_readonly(readonly), 
  m_no_stop_on_entry_error_exit(no_stop_on_entry_error_exit)
{
  // nothing to see here 
}

TelescopeControllerRemote::
TelescopeControllerRemote(VCorba::VOmniORBHelper* orb, const std::string& ior,
			  int sleep_time, int phase_time, bool readonly, 
			  bool no_stop_on_entry_error_exit,
			  const SEphem::SphericalCoords& earth_pos_zero):
  TelescopeController(0, earth_pos_zero, sleep_time, phase_time,
		      CorrectionParameters()),
  m_ref_source(RS_IOR), m_orb(orb), m_scope_num(-1), m_ior(ior),
  m_tc_interface(), m_local_com_failure(true),
  m_readonly(readonly), 
  m_no_stop_on_entry_error_exit(no_stop_on_entry_error_exit)
{
  // nothing to see here 
}

TelescopeControllerRemote::
TelescopeControllerRemote(VCorba::VOmniORBHelper* orb, int scope_num,
			  int sleep_time, int phase_time, bool readonly, 
			  bool no_stop_on_entry_error_exit,
			  const SEphem::SphericalCoords& earth_pos_zero):
  TelescopeController(0, earth_pos_zero, sleep_time, phase_time,
		      CorrectionParameters()),
  m_ref_source(RS_NS), m_orb(orb), m_scope_num(scope_num), m_ior(),
  m_tc_interface(), m_local_com_failure(true),
  m_readonly(readonly), 
  m_no_stop_on_entry_error_exit(no_stop_on_entry_error_exit)
{
  // nothing to see here 
}

TelescopeControllerRemote::~TelescopeControllerRemote()
{
  delete m_target_object;
  delete m_limits;
}

std::string TelescopeControllerRemote::controllerName() const
{
  if(m_readonly)return std::string("CORBA READONLY");
  else return std::string("CORBA");
}

void TelescopeControllerRemote::loopIsTerminating()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(m_tc_interface==0)return;
  if((!m_readonly)&&(!m_no_stop_on_entry_error_exit))
    try
      {
	m_tc_interface->netReqStop();
	m_tse.req = REQ_STOP; 
      }
    catch(...)
      {
	// nothing to see here
      }
}

void TelescopeControllerRemote::iterate()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  try
    {
      if(m_tc_interface == 0)
	{
	  if(m_ref_source == RS_NS)
	    {
	      ZThread::Guard<RecursiveMutex,UnlockedScope> netguard(m_mtx);
	      m_tc_interface = 
		m_orb->nsGetNarrowedObject<NET_TCInterface>
		(NET_TCInterface::program_name,NET_TCInterface::object_name,
		 m_scope_num);
	    }
	  else if(m_ref_source == RS_IOR)
	    {
	      CORBA::ORB_var the_orb = m_orb->orb();
	      CORBA::Object_var obj = the_orb->string_to_object(m_ior.c_str());
	      m_tc_interface = NET_TCInterface::_narrow(obj);
	    }

	  if(m_tc_interface == 0)
	    {
	      Debug::stream()
		<< __PRETTY_FUNCTION__ << std::endl
		<< "Can not recover from CORBA TRANSIENT error "
		<< "condition" << std::endl;
	      assert(0);
	    }
	}
      
      if(m_local_com_failure)
	{
	  if((!m_readonly)&&(!m_no_stop_on_entry_error_exit))
	    try
	      {
		ZThread::Guard<RecursiveMutex,UnlockedScope> netguard(m_mtx);
		m_tc_interface->netReqStop();
	      }
	    catch(const TCIReadonly& x)
	      {
		// nothing to see here
	      }	      

	  NET_TCMiscData_var misc_data;
	  if(1)
	    {
	      ZThread::Guard<RecursiveMutex,UnlockedScope> netguard(m_mtx);
	      misc_data = m_tc_interface->netGetMiscData();
	    }

	  m_earth_position.setLatLongRad(misc_data->earth_pos_lat_rad,
					 misc_data->earth_pos_lon_rad);

	  delete m_limits;
	  m_limits = NET_TCInterfaceServant::unpackLimits(misc_data->limits);

	  m_corrections.enable_offsets   =
	    misc_data->corrections.enable_offsets;
	  m_corrections.enable_corrections = 
	    misc_data->corrections.enable_corrections;
	  m_corrections.enable_vff = 
	    misc_data->corrections.enable_vff;
	  m_corrections.az_ratio         = misc_data->corrections.az_ratio;
	  m_corrections.el_ratio         = misc_data->corrections.el_ratio;
	  m_corrections.az_offset        = misc_data->corrections.az_offset;
	  m_corrections.el_offset        = misc_data->corrections.el_offset;
	  m_corrections.az_ns            = misc_data->corrections.az_ns;
	  m_corrections.az_ew            = misc_data->corrections.az_ew;
	  m_corrections.el_udew          = misc_data->corrections.el_udew;
	  m_corrections.fp_az            = misc_data->corrections.fp_az;	  
	  m_corrections.flex_el_A        = misc_data->corrections.flex_el_A;
	  m_corrections.flex_el_B        = misc_data->corrections.flex_el_B;
	  m_corrections.el_pos_vff_s     = misc_data->corrections.el_pos_vff_s;
	  m_corrections.el_pos_vff_t     = misc_data->corrections.el_pos_vff_t;
	  m_corrections.el_neg_vff_s     = misc_data->corrections.el_neg_vff_s;
	  m_corrections.el_neg_vff_t     = misc_data->corrections.el_neg_vff_t;
	  m_corrections.az_pos_vff_s     = misc_data->corrections.az_pos_vff_s;
	  m_corrections.az_pos_vff_t     = misc_data->corrections.az_pos_vff_t;
	  m_corrections.az_neg_vff_s     = misc_data->corrections.az_neg_vff_s;
	  m_corrections.az_neg_vff_t     = misc_data->corrections.az_neg_vff_t;

	  m_local_com_failure=false;
	}

      NET_StateElements_var nse;
      if(1)
	{
	  ZThread::Guard<RecursiveMutex,UnlockedScope> netguard(m_mtx);
	  nse = m_tc_interface->netGetTelescopeState();
	}
      
      delete m_target_object;
      switch(nse->target._d())
	{
	case NET_TO_RADEC:
	  m_target_object = 
	    new RaDecObject(SEphem::SphericalCoords::
			    makeLatLongRad(nse->target.radec().dec_rad,
					   nse->target.radec().ra_rad),
			    nse->target.radec().epoch,
			    static_cast<const char*>(nse->target.radec().name),
			    NET_TCInterfaceServant::
			    unpackCoordinateOffset(nse->target.radec().offset));
	  break;
	case NET_TO_AZEL:
	  m_target_object = 
	    new AzElObject(SEphem::SphericalCoords::
			   makeLatLongRad(nse->target.azel().el_rad,
					  nse->target.azel().az_rad),
			   nse->target.azel().use_corrections,
			   nse->target.azel().stop_at_target);
	  break;
	case NET_TO_STOW:
	  m_target_object = 
	    new StowObject(static_cast<const char*>(nse->target.stow().name),
			   SEphem::SphericalCoords::
			   makeLatLongRad(nse->target.stow().el_rad,
					  nse->target.stow().az_rad));
	  break;
	case NET_TO_CV:
	  m_target_object = 
	    new CVObject(SEphem::SphericalCoords::
			 makeLatLongRad(nse->target.cv().el_rad,
					nse->target.cv().az_rad),
			 nse->target.cv().az_speed, nse->target.cv().el_speed,
			 nse->target.cv().mjd_zero);
	  break;
	case NET_TO_UNKNOWN:
	  m_target_object = 
	    new UnknownObject(SEphem::SphericalCoords::
			      makeLatLongRad(nse->target.azel().el_rad,
					     nse->target.azel().az_rad),
			      nse->target.azel().use_corrections,
			      nse->target.azel().stop_at_target);
	  break;
	case NET_TO_NULL:
	  m_target_object = 0;
	}

      switch(nse->direction_preference)
	{
	case NET_DP_NONE:
	  m_direction_preference = CorrectionParameters::DP_NONE; break;
	case NET_DP_CW:
	  m_direction_preference = CorrectionParameters::DP_CW; break;
	case NET_DP_CCW:
	  m_direction_preference = CorrectionParameters::DP_CCW; break;
	}
   
      m_tse.status.az.driveangle_deg = nse->status.az.driveangle_deg;
      switch(nse->status.az.driveMode)
	{
	case NET_DM_STANDBY: 
	  m_tse.status.az.driveMode = ScopeAPI::DM_STANDBY; break;
	case NET_DM_SLEW:
	  m_tse.status.az.driveMode = ScopeAPI::DM_SLEW; break;
	case NET_DM_POINT:
	  m_tse.status.az.driveMode = ScopeAPI::DM_POINT; break;
	case NET_DM_SPIN:
	  m_tse.status.az.driveMode = ScopeAPI::DM_SPIN; break;
	case NET_DM_SECTOR_SCAN:
	  m_tse.status.az.driveMode = ScopeAPI::DM_SECTOR_SCAN; break;
	case NET_DM_RASTER:
	  m_tse.status.az.driveMode = ScopeAPI::DM_RASTER; break;
	case NET_DM_CHANGING:
	  m_tse.status.az.driveMode = ScopeAPI::DM_CHANGING; break;
	case NET_DM_UNKNOWN:
	  m_tse.status.az.driveMode = ScopeAPI::DM_UNKNOWN; break;
	}
      m_tse.status.az.servo1Fail = nse->status.az.servo1Fail;
      m_tse.status.az.servo2Fail = nse->status.az.servo2Fail;
      m_tse.status.az.servoOn = nse->status.az.servoOn;
      m_tse.status.az.brakeReleased = nse->status.az.brakeReleased;
      m_tse.status.az.limitCwUp = nse->status.az.limitCwUp;
      m_tse.status.az.limitCcwDown = nse->status.az.limitCcwDown;
      m_tse.status.az.positionFault = nse->status.az.positionFault;
      m_tse.status.az.positionComplete = nse->status.az.positionComplete;
	  
      m_tse.status.el.driveangle_deg = nse->status.el.driveangle_deg;
      switch(nse->status.el.driveMode)
	{
	case NET_DM_STANDBY: 
	  m_tse.status.el.driveMode = ScopeAPI::DM_STANDBY; break;
	case NET_DM_SLEW:
	  m_tse.status.el.driveMode = ScopeAPI::DM_SLEW; break;
	case NET_DM_POINT:
	  m_tse.status.el.driveMode = ScopeAPI::DM_POINT; break;
	case NET_DM_SPIN:
	  m_tse.status.el.driveMode = ScopeAPI::DM_SPIN; break;
	case NET_DM_SECTOR_SCAN:
	  m_tse.status.el.driveMode = ScopeAPI::DM_SECTOR_SCAN; break;
	case NET_DM_RASTER:
	  m_tse.status.el.driveMode = ScopeAPI::DM_RASTER; break;
	case NET_DM_CHANGING:
	  m_tse.status.el.driveMode = ScopeAPI::DM_CHANGING; break;
	case NET_DM_UNKNOWN:
	  m_tse.status.el.driveMode = ScopeAPI::DM_UNKNOWN; break;
	}
      m_tse.status.el.servo1Fail = nse->status.el.servo1Fail;
      m_tse.status.el.servo2Fail = nse->status.el.servo2Fail;
      m_tse.status.el.servoOn = nse->status.el.servoOn;
      m_tse.status.el.brakeReleased = nse->status.el.brakeReleased;
      m_tse.status.el.limitCwUp = nse->status.el.limitCwUp;
      m_tse.status.el.limitCcwDown = nse->status.el.limitCcwDown;
      m_tse.status.el.positionFault = nse->status.el.positionFault;
      m_tse.status.el.positionComplete = nse->status.el.positionComplete;
	  
      m_tse.status.azTravelledCCW = nse->status.azTravelledCCW;
      m_tse.status.azCableWrap = nse->status.azCableWrap;
      m_tse.status.interlock = nse->status.interlock;
      m_tse.status.interlockAzPullCord = nse->status.interlockAzPullCord;
      m_tse.status.interlockAzStowPin = nse->status.interlockAzStowPin;
      m_tse.status.interlockElStowPin = nse->status.interlockElStowPin;
      m_tse.status.interlockAzDoorOpen = nse->status.interlockAzDoorOpen;
      m_tse.status.interlockElDoorOpen = nse->status.interlockElDoorOpen;
      m_tse.status.interlockSafeSwitch = nse->status.interlockSafeSwitch;
      m_tse.status.remoteControl = nse->status.remoteControl;
      m_tse.status.checksumOK = nse->status.checksumOK;
      m_tse.status.msgBadFrame = nse->status.msgBadFrame;
      m_tse.status.msgCommandInvalid = nse->status.msgCommandInvalid;
      m_tse.status.msgInputOverrun = nse->status.msgInputOverrun;
      m_tse.status.msgOutputOverrun = nse->status.msgOutputOverrun;
      m_tse.status.relay1 = nse->status.relay1;
      m_tse.status.relay2 = nse->status.relay2;
      m_tse.status.Analog1 = nse->status.Analog1;
      m_tse.status.Analog2 = nse->status.Analog2;

      m_tse.last_az_driveangle_deg = nse->last_az_driveangle_deg;
      m_tse.last_el_driveangle_deg = nse->last_el_driveangle_deg;
      m_tse.az_driveangle_estimated_speed_dps 
	= nse->az_driveangle_estimated_speed_dps;
      m_tse.el_driveangle_estimated_speed_dps
	= nse->el_driveangle_estimated_speed_dps;
      switch(nse->state) 
	{
	case NET_TS_STOP: 
	  m_tse.state = TelescopeController::TS_STOP; break;
	case NET_TS_SLEW:
	  m_tse.state = TelescopeController::TS_SLEW; break;
	case NET_TS_TRACK: 
	  m_tse.state = TelescopeController::TS_TRACK; break;
	case NET_TS_RESTRICTED_MOTION: 
	  m_tse.state = TelescopeController::TS_RESTRICTED_MOTION; break;
	case NET_TS_RAMP_DOWN:
	  m_tse.state = TelescopeController::TS_RAMP_DOWN; break;
	case NET_TS_COM_FAILURE:
	  m_tse.state = TelescopeController::TS_COM_FAILURE; break;
	}
      switch(nse->req)
	{
	case NET_REQ_STOP:
	  m_tse.req = TelescopeController::REQ_STOP; break;
	case NET_REQ_SLEW:
	  m_tse.req = TelescopeController::REQ_SLEW; break;
	case NET_REQ_TRACK:
	  m_tse.req = TelescopeController::REQ_TRACK; break;
	}
      m_tse.cf=CF_SCOPE;
      m_tse.tv.tv_sec = nse->tv_tv_sec;
      m_tse.tv.tv_usec = nse->tv_tv_usec;
      m_tse.mjd = nse->mjd;
      m_tse.last_mjd = nse->last_mjd;
      m_tse.timeangle.setRad(nse->timeangle_rad);
      m_tse.lmst.setRad(nse->lmst_rad);
      m_tse.last_has_object = nse->last_has_object;
      m_tse.has_object = nse->has_object;
      m_tse.tel_azel.setLatLongRad(nse->tel_azel_el_rad, nse->tel_azel_az_rad);
      m_tse.obj_azel.setLatLongRad(nse->obj_azel_el_rad, nse->obj_azel_az_rad);
      m_tse.anticipation = nse->anticipation;
      m_tse.last_cmd_az_driveangle_deg = nse->last_cmd_az_driveangle_deg;
      m_tse.last_cmd_el_driveangle_deg = nse->last_cmd_el_driveangle_deg;
      m_tse.cmd_az_driveangle_deg = nse->cmd_az_driveangle_deg;
      m_tse.cmd_el_driveangle_deg = nse->cmd_el_driveangle_deg;
      m_tse.az_slew_speed_dps = nse->az_slew_speed_dps;
      m_tse.el_slew_speed_dps = nse->el_slew_speed_dps;
    }
  catch(const CosNaming::NamingContext::NotFound)
    {
      if(m_tc_interface){ CORBA::release(m_tc_interface);m_tc_interface=0; }
      timeout("CORBA nameserver threw \"CosNaming::NamingContext::NotFound\"");
    }
  catch(const CORBA::TRANSIENT& x)
    {
      if(m_tc_interface){ CORBA::release(m_tc_interface);m_tc_interface=0; }
      timeout();
    }
  catch(const CORBA::Exception& x)
    {
      if(m_tc_interface){ CORBA::release(m_tc_interface);m_tc_interface=0; }
      exception(x);
    }
  catch(...)
    {
      if(m_tc_interface){ CORBA::release(m_tc_interface);m_tc_interface=0; }
      timeout("Unknown exception thrown");
    }
}
  
void TelescopeControllerRemote::emergencyStop()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  if(m_tc_interface == 0)return;
  try
    {
      m_tc_interface->netEmergencyStop();
      // should we terminate here ?
      // this->terminate();
    }
  catch(const CORBA::TRANSIENT& x)
    {
      timeout();
    }
  catch(const CORBA::Exception& x)
    {
      exception(x);
    }
}

void TelescopeControllerRemote::
setTargetObject(TargetObject* obj, DirectionPreference dp) 
{ 
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  if(m_tc_interface==0) { delete obj; return; }
  if(m_readonly) { readonly(false); delete obj; return; }
  try
    {
      bool found_object = false;
      if(!obj)
	{
	  m_tc_interface->netSetTargetObjectNull();
	  found_object=true;
	}

      NET_DirectionPreference net_dp;
      switch(dp)
	{
	case CorrectionParameters::DP_NONE:
	  net_dp = NET_DP_NONE; break;
	case CorrectionParameters::DP_CW:
	  net_dp = NET_DP_CW; break;
	case CorrectionParameters::DP_CCW:
	  net_dp = NET_DP_CCW; break;
	}
      
      if(!found_object)
	{
	  const RaDecObject* radec = dynamic_cast<const RaDecObject*>(obj);
	  if(radec)
	    {
	      NET_CoordinateOffset offset;
	      NET_TCInterfaceServant::
		packCoordinateOffset(offset,radec->offset());

	      m_tc_interface->
		netSetTargetObjectRaDec(radec->coords().longitudeRad(),
					radec->coords().latitudeRad(),
					radec->epoch(), offset,
					radec->name().c_str(),
					net_dp);
	      found_object=true;
	    };
	}
      
      if(!found_object)
	{
	  const StowObject* stow = dynamic_cast<const StowObject*>(obj);
	  if(stow)
	    {
	      m_tc_interface->netSetTargetObjectStow(stow->name().c_str());
	      found_object=true;
	    }
	}

      if(!found_object)
	{
	  const AzElObject* azel = dynamic_cast<const AzElObject*>(obj);
	  if(azel)
	    {
	      m_tc_interface->
		netSetTargetObjectAzEl(azel->coords().longitudeRad(),
				       azel->coords().latitudeRad(),
				       azel->useCorrections(),
				       !azel->objectMovesInAzEl(),
				       net_dp);
	      found_object=true;
	    }
	}

      if(!found_object)
	{
	  const CVObject* cv = dynamic_cast<const CVObject*>(obj);
	  if(cv)
	    {
	      m_tc_interface->
		netSetTargetObjectCV(cv->coords().longitudeRad(),
				     cv->coords().latitudeRad(),
				     cv->azSpeed(), cv->elSpeed(),
				     cv->mjdZero());
	      found_object=true;
	    }
	}
      
      if(!found_object)
	{
	  m_tc_interface->netSetTargetObjectNull();
	  Message message(Message::DR_LOCAL,Message::PS_UNUSUAL,
			  "Unsupported Target Object");
	  message.messageStream() 
	    << "Target object type \"" << obj->targetObjectType() 
	    << "\" is not supported by the CORBA" << std::endl
	    << "telscope controller interface." << std::endl;
	  Messenger::relay()->sendMessage(message);
	  delete obj;
	  obj = 0;
	  found_object=true;
	}

      delete m_target_object;
      m_target_object=obj;
      m_direction_preference = dp;
    }
  catch(const TCIReadonly& x)
    {
      readonly(true);
    }	      
  catch(const CORBA::TRANSIENT& x)
    {
      delete obj;
      timeout();
    }
  catch(const CORBA::Exception& x)
    {
      delete obj;
      exception(x);
    }
}

void TelescopeControllerRemote::reqStop() 
{ 
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  if(m_tc_interface==0)return;
  if(m_readonly) { readonly(false); return; }
  try
    {
      m_tc_interface->netReqStop();
      m_tse.req = REQ_STOP; 
    }
  catch(const TCIReadonly& x)
    {
      readonly(true);
    }	      
  catch(const CORBA::TRANSIENT& x)
    {
      timeout();
    }
  catch(const CORBA::Exception& x)
    {
      exception(x);
    }
}

  void TelescopeControllerRemote::reqSlew() 
{ 
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  if(m_tc_interface==0)return;
  if(m_readonly) { readonly(false); return; }
  try
    {
      m_tc_interface->netReqSlew();
      if(m_target_object)m_tse.req = REQ_SLEW;
    }
  catch(const TCIReadonly& x)
    {
      readonly(true);
    }	      
  catch(const CORBA::TRANSIENT& x)
    {
      timeout();
    }
  catch(const CORBA::Exception& x)
    {
      exception(x);
    }
}

void TelescopeControllerRemote::reqTrack() 
{ 

  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  if(m_tc_interface==0)return;
  if(m_readonly) { readonly(false); return; }
  try
    {
      m_tc_interface->netReqTrack();
      if(m_target_object)m_tse.req = REQ_TRACK;
    }
  catch(const TCIReadonly& x)
    {
      readonly(true);
    }	      
  catch(const CORBA::TRANSIENT& x)
    {
      timeout();
    }
  catch(const CORBA::Exception& x)
    {
      exception(x);
    }
}

void TelescopeControllerRemote::
setCorrectionParameters(const CorrectionParameters& cp) 
{ 
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  if(m_tc_interface==0)return;
  if(m_readonly) { readonly(false); return; }
  try
    {
      NET_CorrectionParameters corrections;
      corrections.enable_offsets     = cp.enable_offsets;
      corrections.enable_corrections = cp.enable_corrections;
      corrections.enable_vff         = cp.enable_vff;
      corrections.az_ratio           = cp.az_ratio;
      corrections.el_ratio           = cp.el_ratio;
      corrections.az_offset          = cp.az_offset;
      corrections.el_offset          = cp.el_offset;
      corrections.az_ns              = cp.az_ns;
      corrections.az_ew              = cp.az_ew;
      corrections.el_udew            = cp.el_udew;
      corrections.fp_az              = cp.fp_az;
      corrections.flex_el_A          = cp.flex_el_A;
      corrections.flex_el_B          = cp.flex_el_B;
      corrections.el_pos_vff_s       = cp.el_pos_vff_s;
      corrections.el_pos_vff_t       = cp.el_pos_vff_t;
      corrections.el_neg_vff_s       = cp.el_neg_vff_s;
      corrections.el_neg_vff_t       = cp.el_neg_vff_t;
      corrections.az_pos_vff_s       = cp.az_pos_vff_s;
      corrections.az_pos_vff_t       = cp.az_pos_vff_t;
      corrections.az_neg_vff_s       = cp.az_neg_vff_s;
      corrections.az_neg_vff_t       = cp.az_neg_vff_t;
      m_tc_interface->netSetCorrectionParameters(corrections);
      m_corrections=cp; 
    }
  catch(const TCIReadonly& x)
    {
      readonly(true);
    }	      
  catch(const CORBA::TRANSIENT& x)
    {
      timeout();
    }
  catch(const CORBA::Exception& x)
    {
      exception(x);
    }
}

void TelescopeControllerRemote::timeout(const std::string& text)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  if(m_local_com_failure == false)
    {
      // New Timeout -- write message
      Message message(Message::DR_LOCAL,Message::PS_UNUSUAL,
		      "CORBA Timeout");
      message.messageStream() 
	<< "A timeout occurred while attemting to communicate with the remote "
	<< std::endl
	<< "telescope controller using CORBA. The program will attempt to "
	<< std::endl
	<< "re-establish communication."
	<< std::endl;
      if(!text.empty())message.detailsStream()  << text << std::endl;
      Messenger::relay()->sendMessage(message);
    }

  m_local_com_failure=true;
  getTimes(m_tse.tv, m_tse.mjd, m_tse.timeangle, m_tse.lmst);
  m_tse.state=TS_COM_FAILURE;
  m_tse.cf=CF_SERVER;
  if((m_ref_source == RS_NS)||(m_ref_source == RS_IOR))
    CORBA::release(m_tc_interface);
  m_tc_interface = 0;
}

void TelescopeControllerRemote::exception(const CORBA::Exception& x)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  Message message(Message::DR_LOCAL,Message::PS_EXCEPTIONAL,
		  "CORBA Exception");
  message.messageStream() 
    << "A CORBA exception was thrown while attempting to communicate with "
    << std::endl
    << "the remote telescope controller." << std::endl
    << std::endl
    << "Exception: " << x._name() << std::endl;
  Messenger::relay()->sendMessage(message);
  
  m_local_com_failure = true;

  timeout();
}

void TelescopeControllerRemote::readonly(bool remote)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  Message message(Message::DR_LOCAL,Message::PS_UNUSUAL,
		  "Readonly");
  message.messageStream() 
    << "The command could not be completed as the CORBA interface is readonly."
    << std::endl;
  if(remote)
    message.messageStream() 
      << "To change this re-run the remote server without the "
      << "\"-corba_readonly\" option."
      << std::endl;
  else
    message.messageStream() 
      << "To change this re-run this interface without the "
      << "\"-corba_readonly\" option."
      << std::endl;
  Messenger::relay()->sendMessage(message);
}

// ----------------------------------------------------------------------------
// EXTENDED FUNCTIONALITY
// ----------------------------------------------------------------------------

bool TelescopeControllerRemote::hasTerminateRemoteCapability()
{
  return true;
}

void TelescopeControllerRemote::terminateRemote() 
  throw ( CapabilityNotSupported )
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  if(m_tc_interface==0)return;
  if(m_readonly) { readonly(false); return; }
  try
    {
      m_tc_interface->netTerminate();
    }
  catch(const TCIReadonly& x)
    {
      readonly(true);
    }	      
  catch(const CORBA::TRANSIENT& x)
    {
      timeout();
    }
  catch(const CORBA::Exception& x)
    {
      exception(x);
    }  
}

//-*-mode:c++; mode:font-lock;-*-

/**
 * \file TenMeterControlLoop.cpp
 * \ingroup VTracking
 * \brief This is a one-line description of this cpp file.
 *
 * Here is a tedious verbose multi-line description of all
 * the details of the code, more than you would
 * ever want to read. Generally, all the important documentation
 * goes in the .cpp files.
 *
 * Original Author: Deirdre Horan
 * $Author: sfegan $
 * $Date: 2006/08/28 23:00:30 $
 * $Revision: 2.10 $
 * $Tag$
 *
 **/

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <zthread/Thread.h>
#include <zthread/Guard.h>

#include <Exception.h>
#include <Debug.h>
#include <Angle.h>
#include <Message.h>
#include <Messenger.h>

#include "TenMeterControlLoop.h"

// #warning REMOVE ME

// #define CIO_2 0
// #define DIO_SET_MODE 0
// #define CNTL_A 0
// #define CNTL_B 0
// #define CNTL_C 0

// #warning INSERT ME
#include <dio48.h>

using namespace VMessaging;
using namespace VTracking;

// Constructor of TenMeterControlLoop
TenMeterControlLoop::
TenMeterControlLoop(unsigned scope, DataStream* ds, 
		    unsigned az_address, unsigned el_address,
		    unsigned sleep_time_ms):
  m_sleep_time(sleep_time_ms),
  m_mtx(), 
  m_exit_notification(false),
  m_scope_num(scope), 
  m_last_tv(),
  m_last_tv_cmdPoint(),
  
  m_status(),

  m_limitBits(),
  m_statusBits(),
  
  m_az_ang(0),
  m_el_ang(0),
  m_az_vel(0),
  m_el_vel(0),
  m_az_req_ang(0),
  m_el_req_ang(0),
  m_az_req_vel(0),
  m_el_req_vel(0),
  m_last_az_req_ang(0), 
  m_last_el_req_ang(0), 
  m_last_az_req_vel(0),  // UNUSED
  m_last_el_req_vel(0),  // UNUSED
  m_time_diff(0),
  m_az_req_max_vel(MAXSLEW_SEC), 
  m_el_req_max_vel(MAXSLEW_SEC), 
  m_az_abs_max_vel(MAXSLEW_SEC),
  m_el_abs_max_vel(MAXSLEW_SEC),

  m_cw_enable(),
  m_ccw_enable(),
  m_up_enable(),
  m_down_enable(),
  m_el_15(),
  m_az_window(),

  m_el_ready(),
  m_az_ready(),

  m_datastream(ds),
  m_azDrive(new BRUDrive(ds,az_address)),
  m_elDrive(new BRUDrive(ds,el_address)),

  m_CIO_1(),
  m_CIO_2(),

  m_azDriveInfo("AZ", AZ_DERIVATIVEGAIN, AZ_INTEGRALGAIN, AZ_PROPORTIONALGAIN,
		AZ_MAXRPS, AZ_RPD),
  m_elDriveInfo("EL", EL_DERIVATIVEGAIN, EL_INTEGRALGAIN, EL_PROPORTIONALGAIN,
		EL_MAXRPS, EL_RPD)

{
  // PID loop - don't need for now. Use Kevin's formula to figure out
  // what speed I should command the telescope to move at given the
  // distance it is from the target.

  //  m_az_ang_pid.setPGain(azDriveInfo.proportionalGain);
  //  m_az_ang_pid.setIGain(azDriveInfo.integralGain);
  //  m_az_ang_pid.setDGain(azDriveInfo.derivativeGain);
  //  m_az_ang_pid.setILimit(0.1);

  //  m_el_ang_pid.setPGain(elDriveInfo.proportionalGain);
  //  m_el_ang_pid.setIGain(elDriveInfo.integralGain);
  //  m_el_ang_pid.setDGain(elDriveInfo.derivativeGain);
  //  m_el_ang_pid.setILimit(0.1);


  // Set these to true for now - they will get set to whatever they
  // should be set to upon the first pass through tick() when
  // checkLimitAndStatusBits() gets called. They are not used prior to
  // this.
  m_cw_enable = true;
  m_ccw_enable = true;
  m_up_enable = true;
  m_down_enable = true;
  m_el_15 = true;
  m_az_window = true;

  m_el_ready = true;
  m_az_ready = true;

  // Set the values of m_status that will not get changed throughout the code
  // Most of them will not be used.

  m_status.interlock=false;
  m_status.interlockAzPullCord=false;
  m_status.interlockAzStowPin=false;
  m_status.interlockElStowPin=false;
  m_status.interlockAzDoorOpen=false;
  m_status.interlockElDoorOpen=false;
  m_status.interlockSafeSwitch=false;
  m_status.remoteControl = true;
  m_status.checksumOK = true;
  m_status.msgBadFrame=false;
  m_status.msgCommandInvalid=false;
  m_status.msgInputOverrun=false;
  m_status.msgOutputOverrun=false;
  m_status.relay1=false;
  m_status.relay2=false;
  m_status.Analog1=0;
  m_status.Analog2=0;

  // Use for the Az and El motors

  m_status.az.servo1Fail=false;
  m_status.az.servo2Fail=false;
  m_status.el.servo1Fail=false;
  m_status.el.servo2Fail=false;

  // When the drive is Enabled, the brake is released.
  // When the drive is Disabled, the brake is set.
  m_status.az.brakeReleased=false;
  m_status.el.brakeReleased=false;

  // I no longer call initialize here. It will get called on the first
  // pass through tick() because driveMode will be DM_UNKNOWN
  m_status.az.driveMode = ScopeAPI::DM_UNKNOWN;
  m_status.el.driveMode = ScopeAPI::DM_UNKNOWN;

  // Need to open RS232 port to controllers first -- This is done by main()

  //  m_status.serialPort=tty_set("DEVADDRESS");

  gettimeofday(&m_last_tv,0);

} // End of Constructor of TenMeterControlLoop

// Constructor of DriveInfo
TenMeterControlLoop::DriveInfo::
DriveInfo(const std::string& name, 
	  int derivative, int integral, int proportional, double maxRPS, 
	  double rpd):
  m_driveName(name), 
  m_driveVelocity(0),
  //  m_driveLastVelocity(0), 
  m_stateEnabledFlag(TenMeterControlLoop::DriveInfo::SE_DISABLED), 
  //  m_directionFlag(1),
  //  m_lastDirectionFlag(1),
  m_setpointMode(true),
  m_derivativeGain(derivative),
  m_integralGain(integral),
  m_proportionalGain(proportional), 
  m_maxRPS(maxRPS),
  m_RPD(rpd)
{
  
}

// Destructor of DriveInfo
TenMeterControlLoop::DriveInfo::
~DriveInfo()
{
  // nothing to see here
}


// Destructor of TenMeterControlLoop
TenMeterControlLoop::
~TenMeterControlLoop()
{
  // nothing to see here
}

// **********************************************************************
// Start of TenMeterControlLoop Member functions:
//
// virtual void reqStat(PositionerStatus& state);
// virtual void cmdStandby();
// virtual void cmdPoint(const SEphem::Angle& az_angle, double az_vel,
//		  const SEphem::Angle& el_angle, double el_vel);
// virtual void cmdSlew(double az_vel, double el_vel);
// virtual void reqAzOffset(SEphem::Angle& az_angle);
// virtual void reqElOffset(SEphem::Angle& el_angle);
// virtual void setAzOffset(const SEphem::Angle& az_angle);
// virtual void setElOffset(const SEphem::Angle& el_angle);
// virtual void reqAzPIDParameters(PIDParameters& az_param);
// virtual void reqElPIDParameters(PIDParameters& el_param);
// virtual void setAzPIDParameters(const PIDParameters& az_param);
// virtual void setElPIDParameters(const PIDParameters& el_param);
// virtual void resetCommunication();
// void terminate();
// virtual void run();
// void tick();
// bool initializeBRUDrive(BRUDrive* bruDrive, DriveInfo& driveInfo);
// void initializeDIOCards();
// bool readTelescopeInformation(ang_t& az, ang_t& el, uint8_t& limitBits, 
//                               uint8_t& statusBits);
// int checkLimitAndStatusBits();
// void load();
// void save();
// void initialize();
// int gb(int n, int m);
// void printbin(uint32_t n, int m);
// void split(uint32_t n, uint8_t *m);
// int32_t dps_to_int(const DriveInfo* drive_in)

// **********************************************************************

void TenMeterControlLoop::
reqStat(PositionerStatus& state)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  state = m_status;
}

void TenMeterControlLoop::
cmdStandby()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  m_status.az.positionFault=false;
  m_status.el.positionFault=false;
  m_status.az.driveMode=ScopeAPI::DM_STANDBY;
  m_status.el.driveMode=ScopeAPI::DM_STANDBY;
  // Any time driveMode is set to STANDBY, reset current and last
  // positions, velocities and times.
  m_az_req_ang=0;
  m_el_req_ang=0;
  m_az_req_vel=0;
  m_el_req_vel=0;
  m_last_az_req_ang=0;
  m_last_el_req_ang=0;
  m_last_az_req_vel=0;
  m_last_el_req_vel=0;
  m_last_tv_cmdPoint.tv_sec=0;
  m_last_tv_cmdPoint.tv_usec=0;
  m_time_diff=0;
}

// Steve's code calls this at 4Hz and sets the positions that the 10m
// should go to in it - I access these in tick and use Kevin's
// formulae to calculate velocity to command motors to run at.

void TenMeterControlLoop::
cmdPoint(const SEphem::Angle& az_angle, double az_vel,
	 const SEphem::Angle& el_angle, double el_vel)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  // Store the last requested position, velocity and time before
  // changing them

  m_last_az_req_ang=m_az_req_ang;
  m_last_el_req_ang=m_el_req_ang;
  m_last_az_req_vel=m_az_req_vel;
  m_last_el_req_vel=m_el_req_vel;

  // Calculate the time difference between now and last cmdPoint
  struct timeval tv;
  gettimeofday(&tv,0);
  if((m_last_tv_cmdPoint.tv_sec == 0) && (m_last_tv_cmdPoint.tv_usec == 0))
    {
      m_time_diff = 0;
    }
  else
    {
      m_time_diff = (double(tv.tv_sec-m_last_tv_cmdPoint.tv_sec)+
		     double(tv.tv_usec-m_last_tv_cmdPoint.tv_usec)/1000000.0);
    }
  m_last_tv_cmdPoint=tv;
  
  double az1=az_angle.deg();
  double az2=az1-360;

  if((az1>270)&&(az2<-270))abort(); // never happen 
  // why have it? should throw some exception or do something if this happens?
  else if(az1>270)m_az_req_ang=az2;
  else if(az2<-270)m_az_req_ang=az1;
  else if(fabs(az1-m_az_ang)<=fabs(az2-m_az_ang))m_az_req_ang=az1;
  else m_az_req_ang=az2;
  m_el_req_ang=el_angle.degPM();

  // If the requested velocity (**_vel) is greater than the MAX
  // allowed velocity (m_**_req_max_vel), set the requested velocity
  // (m_**_req_vel) to the max. allowed velocity
  
  az_vel=fabs(az_vel);
  if(az_vel>m_az_req_max_vel)az_vel=m_az_req_max_vel;
  m_az_req_vel=az_vel;

  el_vel=fabs(el_vel);
  if(el_vel>m_el_req_max_vel)el_vel=m_el_req_max_vel;
  m_el_req_vel=el_vel;
    
  // Check that requested position is within range
  
  bool lim_dn = 
    (m_el_req_ang < -1.0)
    ||((m_el_req_ang < 14.5)&&((m_az_req_ang > 2.5)||(m_az_req_ang < -2.5)));
  bool lim_up = 
    m_el_req_ang > 90.0;
  bool lim_cw = m_az_req_ang > 270.0;
  bool lim_cc = m_az_req_ang < -270.0;
  
  if(lim_dn||lim_up||lim_cw||lim_cc)
    {
      m_status.az.positionFault=true;
      m_status.el.positionFault=true;
      m_status.az.driveMode=ScopeAPI::DM_STANDBY;
      m_status.el.driveMode=ScopeAPI::DM_STANDBY;
      // Any time driveMode is set to STANDBY, reset current and last
      // positions, velocities and times.
      m_az_req_ang=0;
      m_el_req_ang=0;
      m_az_req_vel=0;
      m_el_req_vel=0;
      m_last_az_req_ang=0;
      m_last_el_req_ang=0;
      m_last_az_req_vel=0;
      m_last_el_req_vel=0;
      m_last_tv_cmdPoint.tv_sec=0;
      m_last_tv_cmdPoint.tv_usec=0;
      m_time_diff=0;
      
      Message message(Message::DR_LOCAL,Message::PS_UNUSUAL,
		      "Invalid requested position");
      message.messageStream() 
	<< "The requested position is outside the allowed range.";
      Messenger::relay()->sendMessage(message);
    }
  else
    {
      m_status.az.driveMode=ScopeAPI::DM_POINT;
      m_status.el.driveMode=ScopeAPI::DM_POINT;
    }
}

void TenMeterControlLoop::
cmdSlew(double az_vel, double el_vel)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  if(az_vel>m_az_req_max_vel)az_vel=m_az_req_max_vel;
  else if(az_vel<-m_az_req_max_vel)az_vel=-m_az_req_max_vel;
  m_az_req_vel=az_vel;

  if(el_vel>m_el_req_max_vel)el_vel=m_el_req_max_vel;
  else if(el_vel<-m_el_req_max_vel)el_vel=-m_el_req_max_vel;
  m_el_req_vel=el_vel;
  
  m_status.az.positionFault=false;
  m_status.el.positionFault=false;
  m_status.az.driveMode=ScopeAPI::DM_SLEW;
  m_status.el.driveMode=ScopeAPI::DM_SLEW;
}

void TenMeterControlLoop::
reqAzOffset(SEphem::Angle& az_angle)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  // ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  az_angle.setDeg(0);
}

void TenMeterControlLoop::
reqElOffset(SEphem::Angle& el_angle)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  // ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  el_angle.setDeg(0);
}

void TenMeterControlLoop::
setAzOffset(const SEphem::Angle& az_angle)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  // ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  // nothing to see here
}

void TenMeterControlLoop::
setElOffset(const SEphem::Angle& el_angle)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  // ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  // nothing to see here
}

void TenMeterControlLoop::
reqAzPIDParameters(PIDParameters& az_param)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  az_param.Kp   = 0;
  az_param.Ki   = 0;
  az_param.Kd   = 0;
  az_param.Ilim = 0;
  az_param.Kvff = 0;
  az_param.vlim = m_az_req_max_vel;
  az_param.alim = 0;
}

void TenMeterControlLoop::
reqElPIDParameters(PIDParameters& el_param)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  el_param.Kp   = 0;
  el_param.Ki   = 0;
  el_param.Kd   = 0;
  el_param.Ilim = 0;
  el_param.Kvff = 0;
  el_param.vlim = m_el_req_max_vel;
  el_param.alim = 0;
}

void TenMeterControlLoop::
setAzPIDParameters(const PIDParameters& az_param)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  m_az_req_max_vel = fabs(az_param.vlim);
  if(m_az_req_max_vel>m_az_abs_max_vel)m_az_req_max_vel=m_az_abs_max_vel;
}

void TenMeterControlLoop::
setElPIDParameters(const PIDParameters& el_param)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  m_el_req_max_vel = fabs(el_param.vlim);
  if(m_el_req_max_vel>m_el_abs_max_vel)m_el_req_max_vel=m_el_abs_max_vel;
}

void TenMeterControlLoop::
resetCommunication()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  // all ScopeAPIs must do this
  initialize();
}

void TenMeterControlLoop::
terminate()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  m_exit_notification=true;
} // end terminate

void TenMeterControlLoop::
run()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  struct timeval last_time;
  gettimeofday(&last_time,0);

  while(1)
    {
      {
	ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
	if(m_exit_notification)return;
      }
      try
	{
	  tick();
	}
      catch(const Exception& e)
	{
	  e.print(Debug::stream());
	  abort();
	}

      // Try to run at a fixed frequency given by 1/m_sleep_time by
      // calculating the time required to sleep until the next
      // iteration. If for some reason we have missed the next
      // iterations then just skip them.
      
      struct timeval now;
      gettimeofday(&now,0);
      
      int deltatime = 
	(now.tv_sec-last_time.tv_sec)*1000+
	(now.tv_usec-last_time.tv_usec)/1000+m_sleep_time;
      deltatime-=deltatime%m_sleep_time;

      last_time.tv_usec+=deltatime*1000;
      if(last_time.tv_usec>=1000000)
	last_time.tv_sec += last_time.tv_usec/1000000,
	  last_time.tv_usec = last_time.tv_usec%1000000;
      
      int sleeptime = 
	(last_time.tv_sec-now.tv_sec)*1000+
	(last_time.tv_usec-now.tv_usec)/1000;
      
#if 0
      std::ostringstream str;
      str << std::setw(3) << m_sleep_time << ' '
	  << std::setw(10) << last_time.tv_sec << ' '
	  << std::setw(6) << std::setfill('0') 
	  << last_time.tv_usec << ' '
	  << std::setw(10) << now.tv_sec << ' '
	  << std::setw(6) << std::setfill('0') 
	  << now.tv_usec  << ' '
	  << std::setw(4) << std::setfill(' ') << std::showpos
	  << sleeptime
	  << std::endl;
      Debug::stream() << str.str();
#endif
      
      if(sleeptime>0)ZThread::Thread::sleep(sleeptime);
    }
} // end run

#define CWCCW_WORKAROUND_FILE "/tmp/.azcwccw_workaround_cw"

void TenMeterControlLoop::
tick()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  // static means that it only gets defined as false the first time
  static bool azcwccw_workaround_init = false;
  static bool azcwccw_workaround_cw = false;
  static bool suppress_timeout_message = false;

  // Moved this check from further down in tick up to here.
  
  // If the driveMode is UNKNOWN, as it should be the first time that
  // tick() is run, call initialize.
  
  if((m_status.az.driveMode==ScopeAPI::DM_UNKNOWN)
     ||(m_status.el.driveMode==ScopeAPI::DM_UNKNOWN))
    initialize();
  
  // Initialize will set driveMode to STANDBY if everything was successful
  // It has three Main steps:
  // 1. Initialize each of the BRU Drives
  // 2. Check the status of each of the BRU Drives
  // 3. Initialize the DIO cards

  try
    {
      if(azcwccw_workaround_init == false)
	{
	  azcwccw_workaround_init=true;
	  std::ifstream cw_file(CWCCW_WORKAROUND_FILE);
	  if(cw_file)cw_file >> azcwccw_workaround_cw;
	  
	  Message message(Message::DR_LOCAL,Message::PS_ROUTINE,
			  "Using Az CW/CCW workaround");
	  message.messageStream() 
	    << "The mount does not supply its direction (CW/CCW). The "
	    << "tracking program will attempt to keep track of the direction. "
	    << "The last saved direction was: "
	    << (azcwccw_workaround_cw?"CW.":"CCW.");
	  Messenger::relay()->sendMessage(message);
	} // end if(azcwccw_workaround_init == false)
      
      // m_last_status = m_status; -- UNUSED
      
      // Get current position of the 10m
      
      if(readTelescopeInformation(m_az_ang, m_el_ang, m_limitBits, 
				  m_statusBits) == 0)
	{
	  // make sure you don't move !!
	  //dree
	  // Throw an exception
	  // Disable the drives
	  m_azDrive->writeHostDriveEnableDisable(0);
	  m_azDriveInfo.setStateEnabledFlag(DriveInfo::SE_DISABLED);
	  m_status.az.brakeReleased = false;
	  m_elDrive->writeHostDriveEnableDisable(0);
	  m_elDriveInfo.setStateEnabledFlag(DriveInfo::SE_DISABLED);
	  m_status.el.brakeReleased = false;
	  initializeDIOCards();
	  
	} //end if(readTelescopeInformation(m_az_ang ... m_statusBits) == 0)

      // Read the limit bits and the status bits
      
      if(checkLimitAndStatusBits() != 0) // 0 is good
	{
	  if((m_el_ready||m_az_ready)==false)
	    {
	      // Read the status byte again to check for error in read
	      checkLimitAndStatusBits();
	      if((m_el_ready||m_az_ready)==false)
		{
		  if(m_el_ready==false)
		    {
		      m_status.el.driveMode = ScopeAPI::DM_STANDBY;
		      // Any time driveMode is set to STANDBY, reset
		      // current and last positions, velocities and
		      // times.
		      m_el_req_ang=0;
		      m_el_req_vel=0;
		      m_last_el_req_ang=0;
		      m_last_el_req_vel=0;
		      m_last_tv_cmdPoint.tv_sec=0;
		      m_last_tv_cmdPoint.tv_usec=0;
		      m_time_diff=0;
		      Message message(Message::DR_GLOBAL,Message::PS_UNUSUAL,
				      "El. Drive not ready");
		      message.messageStream() 
			<< "After two reads of the status byte, the elevation "
			<< "drive is not ready. Setting El. Drive Mode to "
			<< " STANDBY.";
		      Messenger::relay()->sendMessage(message);
		    }
		  else
		    {
		      m_status.az.driveMode = ScopeAPI::DM_STANDBY;
		      // Any time driveMode is set to STANDBY, reset
		      // current and last positions, velocities and
		      // times.
		      m_az_req_ang=0;
		      m_az_req_vel=0;
		      m_last_az_req_ang=0;
		      m_last_az_req_vel=0;
		      m_last_tv_cmdPoint.tv_sec=0;
		      m_last_tv_cmdPoint.tv_usec=0;
		      m_time_diff=0;
		      Message message(Message::DR_GLOBAL,Message::PS_UNUSUAL,
				      "Az. Drive not ready");
		      message.messageStream() 
			<< "After two reads of the status byte, the azimuth "
			<< "drive is not ready. Setting Az. Drive Mode to "
			<< " STANDBY";
		      Messenger::relay()->sendMessage(message);
		    } // end if(m_el_ready==false)
		} // end ((m_el_ready||m_az_ready)==false)
	    } // end ((m_el_ready||m_az_ready)==false)
	}
      else
	{
	  m_status.az.positionFault=false;
	  m_status.el.positionFault=false;
	} // end if(checkLimitAndStatusBits() != 0)
      
      // Copy the position over
      m_status.az.driveangle_deg = m_az_ang;
      m_status.el.driveangle_deg = m_el_ang;
      
      // Convert 0 to 360 in m_az_ang into -270 to 270
      
      if(m_az_ang<88)azcwccw_workaround_cw=true;
      else if(m_az_ang>272)azcwccw_workaround_cw=false;
      else if(m_status.az.limitCwUp == true)azcwccw_workaround_cw=true;
      else if(m_status.az.limitCcwDown == true)azcwccw_workaround_cw=false;
      
      if(true)
	{
	  std::ofstream cw_file(CWCCW_WORKAROUND_FILE);
	  if(cw_file)cw_file << azcwccw_workaround_cw << std::endl;
	}
      
      if(!azcwccw_workaround_cw)m_az_ang = m_az_ang-360.0;
      
      // Fill in parts of m_status that I am using:
      
      // Already did DriveStatus: Azimuth & Elevation
      
      // These get set below in this function (tick)
      // m_status.azTravelledCCW
      // m_status.azCableWrap
      // m_status.az.drivemode
      // m_status.az.servoOn
      // m_status.az.brakeReleased
      // m_status.az.positionComplete
      // m_status.el.drivemode
      // m_status.el.servoOn
      // m_status.el.brakeReleased
      // m_status.el.positionComplete
      // These get set in checkLimitandStatusBits (called above)
      // m_status.az.limitCwUp
      // m_status.az.limitCcwDown
      // m_status.az.positionFault
      // m_status.el.limitCwUp
      // m_status.el.limitCcwDown
      // m_status.el.positionFault
      
      // Cable wrap and interlock
      m_status.azTravelledCCW = m_az_ang<0.0;
      m_status.azCableWrap = m_az_ang/270.0;
      
      // Position complete flag
      if((m_status.az.driveMode==ScopeAPI::DM_POINT)&&
	 (fabs(m_az_ang-m_az_req_ang)<0.0001)&&(fabs(m_az_vel)<0.00001))
	m_status.az.positionComplete=true;
      else m_status.az.positionComplete=false;
      
      if((m_status.el.driveMode==ScopeAPI::DM_POINT)&&
	 (fabs(m_el_ang-m_el_req_ang)<0.0001)&&(fabs(m_el_vel)<0.00001))
	m_status.el.positionComplete=true;
      else m_status.el.positionComplete=false;
      
      // Analogs
      // The values of the analogs get set in the contructor to 0
      m_status.Analog1 = 0; // These could be used to display something useful
      m_status.Analog2 = 0; // on the oscilloscope -- e.g. drive current.

      // Moved check for DM_UNKNOWN from here to nearer the top of tick.

      // If State is UNKNOWN, call initialize (see near top of tick
      // for description).
      
      if((m_azDriveInfo.getStateEnabledFlag() == DriveInfo::SE_UNKNOWN)
	 ||(m_elDriveInfo.getStateEnabledFlag() == DriveInfo::SE_UNKNOWN))
	initialize();

      // After enabling/disabling the drive, initializeBRUDrive checks
      // to make sure that it is enabled/disabled and then sets
      // stateEnabledFlag accordingly (to SE_ENABLED, SE_DISABLED or
      // SE_UNKNOWN).

      // If after we have been through initialize() the state is STILL
      // SE_UNKNOWN then there is something wrong and we should set the
      // servo fail lights and return.
      
      if((m_azDriveInfo.getStateEnabledFlag() == DriveInfo::SE_UNKNOWN)
	 ||(m_elDriveInfo.getStateEnabledFlag() == DriveInfo::SE_UNKNOWN))
	{
	  if(m_azDriveInfo.getStateEnabledFlag() == DriveInfo::SE_UNKNOWN)
	    m_status.az.servo1Fail = m_status.az.servo2Fail = true;
	  if(m_elDriveInfo.getStateEnabledFlag() == DriveInfo::SE_UNKNOWN)
	    m_status.az.servo1Fail = m_status.az.servo2Fail = true;
	  return;
	}
      else
	{
	  m_status.az.servo1Fail = m_status.az.servo2Fail = false;
	  m_status.az.servo1Fail = m_status.az.servo2Fail = false;
	}

      if(((m_status.az.driveMode==DM_POINT)
	  &&(m_status.el.driveMode==DM_POINT))||
	 ((m_status.az.driveMode==DM_SLEW)
	  &&(m_status.el.driveMode==DM_SLEW)))
	{
	  // cmdPoint calculates the following:
	  // m_az_req_ang
	  // m_el_req_ang
	  // m_az_req_vel
	  // m_el_req_vel
	  // It also stores the last values of these parameters:
	  // m_last_az_req_ang
	  // m_last_el_req_ang
	  // m_last_az_req_vel
	  // m_last_el_req_vel
	  // And the difference in time between cmdPoint being called
	  // m_time_diff
	  
	  ang_t azVel = 0;
	  ang_t elVel = 0;
	  
	  // ********
	  // DM_POINT
	  // ********
  
	  if(m_status.az.driveMode==DM_POINT)
	    {
	      // **************************************************************
	      // PUT ALL OF THIS INTO A SEPARATE FUNCTION??
	      
	      // Calculate the current velocity:
	      // m_time_diff could be 0

	      // changed "double" to "ang_t"
	      
	      ang_t target_az_vel = 0;
	      ang_t target_el_vel = 0;
	      
	      if(m_time_diff!=0)
		{
		  target_az_vel=
		    fabs(m_last_az_req_ang-m_az_req_ang)/m_time_diff;
		  target_el_vel=
		    fabs(m_last_el_req_ang-m_el_req_ang)/m_time_diff;
		}
	  
	      // Calculate the difference between the object's position and
	      // that of the telescope:
	  
	      ang_t dAz = m_az_ang-m_az_req_ang;
	      ang_t dEl = m_el_ang-m_el_req_ang;
	      
	      ang_t dAz2 = dAz*dAz; if(dAz<0)dAz2=-dAz2;
	      ang_t dEl2 = dEl*dEl; if(dEl<0)dEl2=-dEl2;
	  
	      // Using the forumla from the original 10m code, calcaulate the
	      // velocity at which to command the motors to slew
	      // Reference: in "telstatus" function in 10meter.c
	      // Lines ~234-262
	  
	      azVel = target_az_vel + (AZ_GAIN_SEC*dAz) + (AZ2_GAIN_SEC*dAz2);
	      elVel = target_el_vel + (EL_GAIN_SEC*dEl) + (EL2_GAIN_SEC*dEl2);
	      
	    } // END DM_POINT
	  
	  // ********
	  // DM_SLEW
	  // ********
	  
	  else // (m_status.az.driveMode==DM_SLEW) 
	    {
	      azVel = m_az_req_vel;
	      elVel = m_el_req_vel;
	    } // END DM_SLEW

	  // DO THE FOLLOWING FOR EITHER DM_POINT or DM_SLEW
	  
	  // Make sure that the velocity I calculated for the motors is
	  // less than the maximum velocity allowed:
	  
	  if(azVel > m_az_req_max_vel)azVel = m_az_req_max_vel;
	  else if(azVel < -m_az_req_max_vel)azVel = -m_az_req_max_vel;
	  if(elVel > m_el_req_max_vel)elVel = m_el_req_max_vel;
	  else if(elVel < -m_el_req_max_vel)elVel = -m_el_req_max_vel;
	  
	  if(azVel > m_az_abs_max_vel)azVel = m_az_abs_max_vel;
	  else if(azVel < -m_az_abs_max_vel)azVel = -m_az_abs_max_vel;
	  if(elVel > m_el_abs_max_vel)elVel = m_el_abs_max_vel;
	  else if(elVel < -m_el_abs_max_vel)elVel = -m_el_abs_max_vel;
	  
	  // Set the velocity member function to have the correct
	  // velocity.

	  m_az_vel=azVel;
	  m_el_vel=elVel;
	  
	  // Enable each of the drives if it is disabled.
	  // On Whipple, when you enable the drive, the brake gets
	  // released.
	  
	  if(m_azDriveInfo.getStateEnabledFlag() == DriveInfo::SE_DISABLED)
	    {
	      m_azDrive->writeHostDriveEnableDisable(1);
	      m_azDriveInfo.setStateEnabledFlag(DriveInfo::SE_ENABLED);
	      m_status.az.brakeReleased = true;
	    }
	  if(m_elDriveInfo.getStateEnabledFlag() == DriveInfo::SE_DISABLED)
	    {
	      m_elDrive->writeHostDriveEnableDisable(1);
	      m_elDriveInfo.setStateEnabledFlag(DriveInfo::SE_ENABLED);
	      m_status.el.brakeReleased = true;
	    }
	  // Set the velocity in the drive info class.
	  // Not sure this is necessary.

	  m_azDriveInfo.setVelocity(m_az_vel);
	  m_elDriveInfo.setVelocity(m_el_vel);
	  
	  // Convert the velocity into the deg-per-sec for motors
	  
	  int32_t velAzRPS = dps_to_int(&m_azDriveInfo);
	  int32_t velElRPS = dps_to_int(&m_elDriveInfo);
	  
	  m_azDrive->writeVelocitySetpoint(velAzRPS);
	  m_elDrive->writeVelocitySetpoint(velElRPS);

	} // END if DM_POINT or DM_SLEW

      else // DM_STANDBY or anything else
	{
	  // Set the velocity to 0:

	  m_az_vel=0;
	  m_el_vel=0;

	  // Set the velocity in the drive info class

	  m_azDriveInfo.setVelocity(m_az_vel);
	  m_elDriveInfo.setVelocity(m_el_vel);
	  
	  m_azDrive->writeVelocitySetpoint(0);
	  m_elDrive->writeVelocitySetpoint(0);
	  
	  // Disable each of the drives if it is enabled.
	  // When drives are disabled, brakes are set.
	  
	  if(m_azDriveInfo.getStateEnabledFlag() == DriveInfo::SE_ENABLED)
	    {
	      m_azDrive->writeHostDriveEnableDisable(0);
	      m_azDriveInfo.setStateEnabledFlag(DriveInfo::SE_DISABLED);
	      m_status.az.brakeReleased = false;
	    }

	  if(m_elDriveInfo.getStateEnabledFlag() == DriveInfo::SE_ENABLED)
	    {
	      m_elDrive->writeHostDriveEnableDisable(0);
	      m_elDriveInfo.setStateEnabledFlag(DriveInfo::SE_DISABLED);
	      m_status.el.brakeReleased = false;
	    }
	} // END DM_STANDBY or anything else but (DM_SLEW/DM_POINT)

      // Set the servo lights appropriately:
      // Whenever the BRU Drives are on, the servo lights should be on.
      // If the powerupStatus is anything but "0", set DM to UNKNOWN

      if(m_azDrive->readPowerupStatus()==0)
	{
	  m_status.az.servoOn = true;
	}
      else
	{
	  m_status.az.servoOn = false;
	  m_status.az.driveMode = ScopeAPI::DM_UNKNOWN;
	}
      if(m_elDrive->readPowerupStatus()==0)
	{
	  m_status.el.servoOn = true;
	}
      else
	{
	  m_status.el.servoOn = false;
	  m_status.el.driveMode = ScopeAPI::DM_UNKNOWN;
	}

      // Woo-hoo we made it all the way through -- make sure the 
      // "com error" lights are not on (see the catch, next)

      m_status.msgInputOverrun = m_status.msgOutputOverrun = false;
      m_status.msgBadFrame = m_status.msgCommandInvalid = false;

      // We did not have a "Timeout" this iteration, so if one happens
      // next time around we should not suppress the message from being
      // sent to the GUI

      suppress_timeout_message = false;
      
    } // end TRY

  catch(const VTracking::Timeout& x)
    {
      // This exception indicates that some communication failure has
      // occurred. We set the drive status to "unknown" so that the
      // code will try to "reinitialize" them next time around. We set
      // the "input overrun" and "output overrun" lights so that the
      // user knows something is up. We also send a message... if one
      // has not already been sent (to stop them being sent at 10Hz!)

      m_azDriveInfo.setStateEnabledFlag(DriveInfo::SE_UNKNOWN);
      m_elDriveInfo.setStateEnabledFlag(DriveInfo::SE_UNKNOWN);

      if(!suppress_timeout_message)
	{
	  Message message(x,Message::DR_GLOBAL,Message::PS_UNUSUAL);
	  Messenger::relay()->sendMessage(message);
	  suppress_timeout_message = true;
	}

      m_status.az.servo1Fail = m_status.az.servo2Fail = false;
      m_status.az.servo1Fail = m_status.az.servo2Fail = false;
      m_status.msgInputOverrun = m_status.msgOutputOverrun = true;
      m_status.msgBadFrame = m_status.msgCommandInvalid = false;
    } // end catch(const VTracking::Timeout& x)
  catch(const Exception& e)
    {
      // Some other (tracking code) exception occured.We set the drive
      // status to "unknown" so that the code will try to
      // "reinitialize" them next time around. We set the "bad frame"
      // and "command invalid" lights so that the user knows something
      // is up. We also send a message.

      m_azDriveInfo.setStateEnabledFlag(DriveInfo::SE_UNKNOWN);
      m_elDriveInfo.setStateEnabledFlag(DriveInfo::SE_UNKNOWN);

      Message message(e,Message::DR_GLOBAL,Message::PS_EXCEPTIONAL);
      Messenger::relay()->sendMessage(message);
      
      m_status.az.servo1Fail = m_status.az.servo2Fail = false;
      m_status.az.servo1Fail = m_status.az.servo2Fail = false;
      m_status.msgInputOverrun = m_status.msgOutputOverrun = false;
      m_status.msgBadFrame = m_status.msgCommandInvalid = true;

      suppress_timeout_message = false;
    } //(const Exception& e)
  catch(...)
    {
      // Some other (non-tracking code) exception occured. We set the
      // drive status to "unknown" so that the code will try to
      // "reinitialize" them next time around. We set the "bad frame"
      // and "command invalid" lights so that the user knows something
      // is up. We also send a message.

      m_azDriveInfo.setStateEnabledFlag(DriveInfo::SE_UNKNOWN);
      m_elDriveInfo.setStateEnabledFlag(DriveInfo::SE_UNKNOWN);

      Message message(Message::DR_GLOBAL,Message::PS_EXCEPTIONAL,
		      "Unknown exception");
      message.messageStream() << "Caught unknown exception";
      Messenger::relay()->sendMessage(message);
      
      m_status.az.servo1Fail = m_status.az.servo2Fail = false;
      m_status.az.servo1Fail = m_status.az.servo2Fail = false;
      m_status.msgInputOverrun = m_status.msgOutputOverrun = false;
      m_status.msgBadFrame = m_status.msgCommandInvalid = true;

      suppress_timeout_message = false;
    } // end (...)

} // end tick

bool TenMeterControlLoop::
initializeBRUDrive(BRUDrive* bruDrive, DriveInfo& driveInfo)
{

  // Adapted from controllers.c; fn "setup_motors" - init_motors option.
  // Capitalised words are the commands that are defined in controllers.h
  // The page number of the particular BRU command is given for each.

  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  bool successfulInitialization = true;
  DriveInfo::StateEnabled previousState=driveInfo.getStateEnabledFlag();

  // Make sure that the digital limits are set up properly. If not,
  // set them up, check again and, if still not set correctly, return
  // false
  
  if(readDigitalInputs(bruDrive, driveInfo)==false)
    {
      setDigitalInputs(bruDrive, driveInfo);
      if(readDigitalInputs(bruDrive, driveInfo)==false)
	{
	  successfulInitialization=false;
	}
    }

  // ***********************************************************************
  
  // Set the proportional, derivative and integral gains
  // VEL_PROP_GAIN_WRITE, VEL_DERIV_GAIN_WRITE, VEL_INTEG_GAIN_WRITE
  // Pages 26 & 27 of BRU Command Book
  
  bruDrive->writeVelocityLoopProportionalGain(driveInfo.getProportionalGain());
  bruDrive->writeVelocityLoopDerivativeGain(driveInfo.getDerivativeGain());
  bruDrive->writeVelocityLoopIntegralGain(driveInfo.getIntegralGain());

  // ***********************************************************************

  // Set the setpoint on or off
  // SETPOINT_ON / SETPOINT_OFF
  // Page 128 of BRU Command Book

  // Check if the setpoint is supposed to be set or not
  // and then enable/disable it accordingly

  if(driveInfo.getSetpointMode())bruDrive->writeSetpointControlEnableFlag(1);
  else bruDrive->writeSetpointControlEnableFlag(0);

  // ***********************************************************************

  // Am not going to do the direction monitoring the same way that
  // Kevin did

  // Set the direction of the motor
  // DIRECTION_WRITE_FORWARD / DIRECTION_WRITE_REVERSE (?? Do I need this ??)
  // Page 101 of BRU Command Book

  // ***********************************************************************

  // Restore the drive to whatever state it was in prior to the reset
  // except for SE_UNKNOWN

  if(previousState==DriveInfo::SE_ENABLED)
    {
      bruDrive->writeHostDriveEnableDisable(1);
      driveInfo.setStateEnabledFlag(DriveInfo::SE_ENABLED);
      if(driveInfo.getDriveName()=="AZ"){ m_status.az.brakeReleased = true;}
      else{m_status.el.brakeReleased = true;}
    }
  else if(previousState==DriveInfo::SE_DISABLED)
    {
      bruDrive->writeHostDriveEnableDisable(0);
      driveInfo.setStateEnabledFlag(DriveInfo::SE_DISABLED);
      if(driveInfo.getDriveName()=="AZ"){m_status.az.brakeReleased = false;}
      else{m_status.el.brakeReleased = false;}
    }
  else if(previousState==DriveInfo::SE_UNKNOWN)
    {
      // initialize() gets called when SE_UNKNOWN happens...  not sure
      // what to do? SE_UNKNOWN flag was set so that the system would
      // reinitialize. Although initialization is not complete by now,
      // should probably see what the state of the drive is and then
      // set SE accordingly? The rest of the initialization will still
      // continue.
      
      uint8_t driveState=bruDrive->readHostDriveEnableDisable();
      if(driveState==1)
	{
	  driveInfo.setStateEnabledFlag(DriveInfo::SE_ENABLED);
	}
      else if(driveState==0)
	{
	  driveInfo.setStateEnabledFlag(DriveInfo::SE_DISABLED);
	}
      else
	{
	  driveInfo.setStateEnabledFlag(DriveInfo::SE_UNKNOWN);
	}
    }

  
  // SET THE VELOCITY ??? NOT SURE IF THIS IS CORRECT ???
  // Set the velocity
  // VELOCITY_SETPOINT_WRITE
  // Page 127 of BRU Command Book

  // start off sending 0 as velocity
  // Is driveVelocity 0 -> not necessarily - define a zero velocity

  bruDrive->writeVelocitySetpoint(VELOCITY_ZERO);
  return successfulInitialization;
  
}// end initializeBRUDrive

bool TenMeterControlLoop::
resetBRUDrive(BRUDrive* bruDrive, DriveInfo& driveInfo)
{
  // Adapted from controllers.c; fn "setup_motors" - reset_motors option.
  // Capitalised words are the commands that are defined in controllers.h
  // The page number of the particular BRU command is given for each.

  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  bool successfulReset = true;
  DriveInfo::StateEnabled previousState=driveInfo.getStateEnabledFlag();

  // Enable the drive
  // DRIVE_ENABLE 
  // Page 124 of BRU Command Book

  bruDrive->writeHostDriveEnableDisable(1);
  driveInfo.setStateEnabledFlag(DriveInfo::SE_ENABLED);
  if(driveInfo.getDriveName()=="AZ"){m_status.az.brakeReleased = true;}
  else{m_status.el.brakeReleased = true;}
  
  // Reset the drive
  // RESET_DRIVE
  // Page 124 of BRU Command Book
  
  {
    bruDrive->writeResetDrive();
  }

  // Restore the drive to whatever state it was in prior to the reset

  if(previousState==DriveInfo::SE_ENABLED)
    {
      bruDrive->writeHostDriveEnableDisable(1);
      driveInfo.setStateEnabledFlag(DriveInfo::SE_ENABLED);
      if(driveInfo.getDriveName()=="AZ"){m_status.az.brakeReleased = true;}
      else{m_status.el.brakeReleased = true;}
    }
  else if(previousState==DriveInfo::SE_DISABLED)
    {
      bruDrive->writeHostDriveEnableDisable(0);
      driveInfo.setStateEnabledFlag(DriveInfo::SE_DISABLED);
      if(driveInfo.getDriveName()=="AZ"){m_status.az.brakeReleased = false;}
      else{m_status.el.brakeReleased = false;}
    }
  else if(previousState==DriveInfo::SE_UNKNOWN)
    {
      // this function may get called when SE_UNKNOWN happens...  not
      // sure what to do? SE_UNKNOWN flag was set so that the system
      // would reinitialize. Although initialization is not complete
      // by now, should probably see what the state of the drive is
      // and then set SE accordingly? The rest of the initialization
      // will still continue.
      
      uint8_t driveState=bruDrive->readHostDriveEnableDisable();
      if(driveState==1)
	{
	  driveInfo.setStateEnabledFlag(DriveInfo::SE_ENABLED);
	}
      else if(driveState==0)
	{
	  driveInfo.setStateEnabledFlag(DriveInfo::SE_DISABLED);
	}
      else
	{
	  driveInfo.setStateEnabledFlag(DriveInfo::SE_UNKNOWN);
	}
    }
  
  // SET THE VELOCITY ??? NOT SURE IF THIS IS CORRECT ???
  // Set the velocity
  // VELOCITY_SETPOINT_WRITE
  // Page 127 of BRU Command Book

  // start off sending 0 as velocity
  // Is driveVelocity 0 -> not necessarily - define a zero velocity

  bruDrive->writeVelocitySetpoint(VELOCITY_ZERO);

  return successfulReset;
} // end resetBRUDrive


bool TenMeterControlLoop::
setBRUDrive(BRUDrive* bruDrive, DriveInfo& driveInfo)
{

  // Adapted from controllers.c; fn "setup_motors" - part at end that
  // always gets done (even if neither reset/init options are selected)
  // Capitalised words are the commands that are defined in controllers.h
  // The page number of the particular BRU command is given for each.

  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  if(driveInfo.getStateEnabledFlag()==DriveInfo::SE_ENABLED)
    {
      bruDrive->writeHostDriveEnableDisable(1);
      if(driveInfo.getDriveName()=="AZ"){m_status.az.brakeReleased = true;}
      else{m_status.el.brakeReleased = true;}
    }
  else if(driveInfo.getStateEnabledFlag()==DriveInfo::SE_DISABLED)
    {
      bruDrive->writeHostDriveEnableDisable(0);
      if(driveInfo.getDriveName()=="AZ"){m_status.az.brakeReleased = false;}
      else{m_status.el.brakeReleased = false;}
    }
  else if(driveInfo.getStateEnabledFlag()==DriveInfo::SE_UNKNOWN)
    {
      // Find out what the state is
      uint8_t driveState=bruDrive->readHostDriveEnableDisable();
      if(driveState==1)
	{
	  driveInfo.setStateEnabledFlag(DriveInfo::SE_ENABLED);
	}
      else if(driveState==0)
	{
	  driveInfo.setStateEnabledFlag(DriveInfo::SE_DISABLED);
	}
      else
	{
	  driveInfo.setStateEnabledFlag(DriveInfo::SE_UNKNOWN);
	}
    }

  // SET THE VELOCITY ??? NOT SURE IF THIS IS CORRECT ???
  // Set the velocity
  // VELOCITY_SETPOINT_WRITE
  // Page 127 of BRU Command Book

  // start off sending 0 as velocity
  // Is driveVelocity 0 -> not necessarily - define a zero velocity

  bruDrive->writeVelocitySetpoint(VELOCITY_ZERO);

  return successfulSet;

} // end setBRUDrive


bool TenMeterControlLoop::
getBRUDriveStatus(BRUDrive* bruDrive, DriveInfo& driveInfo, 
		  bool& powerupSuccess)
{

  // Adapted from function "print_status" (in controllers.c) which calls
  // "get_status()"
  // ... this reads the drive parameters and stores them in az_info or 
  // ... el_info
  // Then, "print_status" calls
  // these three functions:
  //                "test_fault"
  //                "test_powerup"
  //                "test_runstate"
  // which are adapted below

  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  

  // should all return non-zero values
  // get_status: If 00 returned - no faults
  //             Any
  // faultstatus: If 00 returned - no faults
  //              Anything else, details below
  // runstate: If FF returned - Drive Enabled and ready
  //           If 00 returned - Drive Ready but not enabled
  //           Anything else, details below
  // powerupstatus: If 00 returned - Successful Power-Up
  //                Anything else, details below

  // These are in BRUDrive_members_cpp.txt

  uint32_t faultstatus = bruDrive->readFaultStatus();
  uint8_t runstate = bruDrive->readRunState();
  uint8_t powerupstatus = bruDrive->readPowerupStatus();

  // *************************************************************************
  // "test_fault" function in controller.c -> faultstatus

  bool bad_faultstate=true;

  // For the faults, more than one of them can be true so don't use
  // "case" in case more than one is true. Check each one using "if".

  if(faultstatus == 0x000000000) /* no bits set*/
    {
      Message message(Message::DR_GLOBAL,Message::PS_ROUTINE,
			 "Checked FaultStatus - No Bits Set.");
      message.messageStream() << "No Faults";
      Messenger::relay()->sendMessage(message);
      bad_faultstate=false;
    }
  else
    {
      if(faultstatus & 0x00000001U) /*bit 0*/
	{
	  Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
			  "Faultstatus Bit 0");
	  message.messageStream() << "+24VDC Fuse Blown";
	  Messenger::relay()->sendMessage(message);
	}
      
      if(faultstatus & 0x00000002U) /*bit 1*/
	{
	  Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
			  "Faultstatus Bit 1");
	  message.messageStream() << "+5VDC Fuse Blown";
	  Messenger::relay()->sendMessage(message);
	}
      if(faultstatus & 0x00000004U) /*bit 2*/
	{
	  Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
			  "Faultstatus Bit 2");
	  message.messageStream() << "Encoder Power Fuse Blown";
	  Messenger::relay()->sendMessage(message);
	}
      if(faultstatus & 0x00000008U) /*bit 3*/
	{
	  Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
			  "Faultstatus Bit 3");
	  message.messageStream() << "Motor Overtemperature Thermostat";
	  Messenger::relay()->sendMessage(message);
	}
      if(faultstatus & 0x00000010U) /*bit 4*/
	{
	  Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
			  "Faultstatus Bit 4");
	  message.messageStream() << "IPM fault (overtemp/overcurrent, "
				  << "short circuit)";
	  Messenger::relay()->sendMessage(message);
	}
      if(faultstatus & 0x00000020U) /*bit 5*/
	{
	  Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
			  "Faultstatus Bit 5");
	  message.messageStream() << "Channel IM line Break";
	  Messenger::relay()->sendMessage(message);
	}
      if(faultstatus & 0x00000040U) /*bit 6*/
	{
	  Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
			  "Faultstatus Bit 6");
	  message.messageStream() << "Channel BM line Break";
	  Messenger::relay()->sendMessage(message);
	}
      if(faultstatus & 0x00000080U) /*bit 7*/
	{
	  Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
			  "Faultstatus Bit 7");
	  message.messageStream() << "Channel AM line Break";
	  Messenger::relay()->sendMessage(message);
	}
      if(faultstatus & 0x00000100U) /*bit 8*/
	{
	  Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
			  "Faultstatus Bit 8");
	  message.messageStream() << "Bus Undervoltage";
	  Messenger::relay()->sendMessage(message);
	}
      if(faultstatus & 0x00000200U) /*bit 9*/
	{
	  Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
			  "Faultstatus Bit 9");
	  message.messageStream() << "Bus Overvoltage";
	  Messenger::relay()->sendMessage(message);
	}
      if(faultstatus & 0x00000400U) /*bit 10*/
	{
	  Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
			  "Faultstatus Bit 10");
	  message.messageStream() << "Illegal Halt State";
	  Messenger::relay()->sendMessage(message);
	}
      if(faultstatus & 0x00000800U) /*bit 11*/
	{
	  Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
			  "Faultstatus Bit 11");
	  message.messageStream() << "Sub processor Unused Interrupt";
	  Messenger::relay()->sendMessage(message);
	}
      if(faultstatus & 0x00001000U) /*bit 12*/
	{
	  Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
			  "Faultstatus Bit 12");
	  message.messageStream() << "Main processor Unused Interrupt";
	  Messenger::relay()->sendMessage(message);
	}
      if(faultstatus & 0x00010000U) /*bit 16*/
	{
	  Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
			  "Faultstatus Bit 16");
	  message.messageStream() << "Excessive Average Current";
	  Messenger::relay()->sendMessage(message);
	}
      if(faultstatus & 0x00020000U) /*bit 17*/
	{
	  Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
			  "Faultstatus Bit 17");
	  message.messageStream() << "Motor Overspeed";
	  Messenger::relay()->sendMessage(message);
	}
      if(faultstatus & 0x00040000U) /*bit 18*/
	{
	  Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
			  "Faultstatus Bit 18");
	  message.messageStream() << "Excessive Following Error";
	  Messenger::relay()->sendMessage(message);
	}
      if(faultstatus & 0x00080000U) /*bit 19*/
	{
	  Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
			  "Faultstatus Bit 19");
	  message.messageStream() << "Motor Encoder State Error";
	  Messenger::relay()->sendMessage(message);
	}
      if(faultstatus & 0x00100000U) /*bit 20*/
	{
	  Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
			  "Faultstatus Bit 20");
	  message.messageStream() << "Master Encoder State Error";
	  Messenger::relay()->sendMessage(message);
	}
      if(faultstatus & 0x00200000U) /*bit 21*/
	{
	  Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
			  "Faultstatus Bit 21");
	  message.messageStream() << "Motor Thermal Protection";
	  Messenger::relay()->sendMessage(message);
	}
      if(faultstatus & 0x00400000U) /*bit 22*/
	{
	  Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
			  "Faultstatus Bit 22");
	  message.messageStream() << "IPM Thermal Protection";
	  Messenger::relay()->sendMessage(message);
	}
      if(faultstatus & 0x08000000U) /*bit 27*/
	{
	  Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
			  "Faultstatus Bit 27");
	  message.messageStream() << "Enabled with no Motor Selected";
	  Messenger::relay()->sendMessage(message);
	}
      if(faultstatus & 0x10000000U) /*bit 28*/
	{
	  Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
			  "Faultstatus Bit 28");
	  message.messageStream() << "Motor Selection Not in Table";
	  Messenger::relay()->sendMessage(message);
	}
      if(faultstatus & 0x20000000U) /*bit 29*/
	{
	  Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
			  "Faultstatus Bit 29");
	  message.messageStream() << "Personality Write Error";
	  Messenger::relay()->sendMessage(message);
	}
      if(faultstatus & 0x40000000U) /*bit 30*/
	{
	  Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
			  "Faultstatus Bit 30");
	  message.messageStream() << "Service Write Error";
	  Messenger::relay()->sendMessage(message);
	}
      if(faultstatus & 0x80000000U) /*bit 31*/
	{
	  Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
			  "Faultstatus Bit 31");
	  message.messageStream() << "CPU Communications Error";
	  Messenger::relay()->sendMessage(message);
	}
      else // Unregonised fault state
	{
	  Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
			  "UNHANDLED FAULT STATE");
	  message.messageStream() << "Fault state returned "
				  << "is not recognised.";
	  Messenger::relay()->sendMessage(message);
	}
      bad_faultstate=true;
    }

  // *************************************************************************
  
  // "test_runstate" function in controller.c -> runstate

  char* the_text = "UNHANDLED RUN STATE";
  Message::PayloadSignificance the_significance = Message::PS_CRITICAL;

  bool bad_runstate=true;
  // "bad_runstate" is set to true at start. It will remain set this
  // way unless an acceptable run state has been returned.

  switch(runstate)
    {
    case 0x00:
      the_significance = Message::PS_UNUSUAL;
      the_text = "Drive ready...NOT ENABLED";
      bad_runstate=false;
      break;
    case 0x01:
      the_significance = Message::PS_CRITICAL;
      the_text = "+24VDC Fuse Blown";
      break;
    case 0x02:
      the_significance = Message::PS_CRITICAL;
      the_text = "+5VDC Fuse Blown";
      break;
    case 0x03:
      the_significance = Message::PS_CRITICAL;
      the_text = "Encoder Power Fuse Blown";
      break;
    case 0x04:
      the_significance = Message::PS_CRITICAL;
      the_text = "Motor Overtemperature Thermostat";
      break;
    case 0x05:
      the_significance = Message::PS_CRITICAL;
      the_text = "IPM fault (overtemp/overcurrent, short circuit";
      break;
    case 0x06:
      the_significance = Message::PS_CRITICAL;
      the_text = "Channel IM line Break";
      break;
    case 0x07:
      the_significance = Message::PS_CRITICAL;
      the_text = "Channel BM line Break";
      break;
    case 0x08:
      the_significance = Message::PS_CRITICAL;
      the_text = "Channel AM line Break";
      break;
    case 0x09:
      the_significance = Message::PS_CRITICAL;
      the_text = "Bus Undervoltage";
      break;
    case 0x0A:
      the_significance = Message::PS_CRITICAL;
      the_text = "Bus Overvoltage";
      break;
    case 0x0B:
      the_significance = Message::PS_CRITICAL;
      the_text = "Illegal Halt State";
      break;
    case 0x0C:
      the_significance = Message::PS_CRITICAL;
      the_text = "Sub processor Unused Interrupt";
      break;
    case 0x0D:
      the_significance = Message::PS_CRITICAL;
      the_text = "Main processor Unused Interrupt";
      break;
    case 0x11:
      the_significance = Message::PS_CRITICAL;
      the_text = "Excessive Average Current";
      break;
    case 0x12:
      the_significance = Message::PS_CRITICAL;
      the_text = "Motor Overspeed";
      break;
    case 0x13:
      the_significance = Message::PS_CRITICAL;
      the_text = "Excessive Following Error";
      break;
    case 0x14:
      the_significance = Message::PS_CRITICAL;
      the_text = "Motor Encoder State Error";
      break;
    case 0x15:
      the_significance = Message::PS_CRITICAL;
      the_text = "Master Encoder State Error";
      break;
    case 0x16:
      the_significance = Message::PS_CRITICAL;
      the_text = "Motor Thermal Protection";
      break;
    case 0x17:
      the_significance = Message::PS_CRITICAL;
      the_text = "IPM Thermal Protection";
      break;
    case 0x1C:
      the_significance = Message::PS_CRITICAL;
      the_text = "Enabled with no Motor Selected";
      break;
    case 0x1D:
      the_significance = Message::PS_CRITICAL;
      the_text = "Motor Selection Not in Table";
      break;
    case 0x1E:
      the_significance = Message::PS_CRITICAL;
      the_text = "Personality Write Error";
      break;
    case 0x1F:
      the_significance = Message::PS_CRITICAL;
      the_text = "Service Write Error";
      break;
    case 0x20:
      the_significance = Message::PS_CRITICAL;
      the_text = "CPU Communications Error";
      break;
    case 0xFF:
      the_significance = Message::PS_CRITICAL;
      the_text = "Drive Enabled and ready";
      bad_runstate=false;
    }

  Message message_rs(Message::DR_GLOBAL, the_significance,
		     "Checking runstate:");
  message_rs.messageStream() << the_text;
  Messenger::relay()->sendMessage(message_rs);
  
  // Print the run state.

  // *************************************************************************
  
  // "test_powerup" function in controller.c -> powerup

  the_text =  "UNHANDLED POWERUP STATE";
  the_significance = Message::PS_CRITICAL,

  // This will be the message that gets sent if none of the usual
  // powerup states, which are checked for below, are returned.

  powerupSuccess=false;
  // "powerupSuccess" is set to false at start. It will remain set
  // this way unless a successful powerup happens
  bool bad_powerup=true;
  // "bad_powerup" is set to true at start. It will remain set this
  // way unless an acceptable powerup state has been returned.
  
  switch(powerupstatus)
    {
    case 0x00:
      the_significance = Message::PS_ROUTINE;
      the_text = "Successful Power-Up";
      powerupSuccess=true;
      bad_powerup = false;
      break;
    case 0x33:
      the_significance = Message::PS_CRITICAL;
      the_text =  "Program Memory Boot Block Error";
      break;
    case 0x34:
      the_significance = Message::PS_CRITICAL;
      the_text = "Program Memory Main Block Error";
      break;
    case 0x35:
      the_significance = Message::PS_CRITICAL;
      the_text = "Uninitialized Personality EEPROM Error";
      break;
    case 0x36:
      the_significance = Message::PS_CRITICAL;
      the_text = "Personality EEPROM Read Error";
      break;
    case 0x37:
      the_significance = Message::PS_CRITICAL;
      the_text = "Personality EEPROM Data Corruption Error";
      break;
    case 0x38:
      the_significance = Message::PS_CRITICAL;
      the_text = "Main processor Watchdog Error";
      break;
    case 0x39:
      the_significance = Message::PS_CRITICAL;
      the_text = "Sub processor Watchdog Error";
      break;
    case 0x3A:
      the_significance = Message::PS_CRITICAL;
      the_text = "Main processor RAM Error";
      break;
    case 0x3B:
      the_significance = Message::PS_CRITICAL;
      the_text = "Sub processor RAM Error";
      break;
    case 0x3C:
      the_significance = Message::PS_CRITICAL;
      the_text = "Uninitialized Service EEPROM Error";
      break;
    case 0x3D:
      the_significance = Message::PS_CRITICAL;
      the_text = "Service EEPROM Read Error";
      break;
    case 0x3E:
      the_significance = Message::PS_CRITICAL;
      the_text = "Service EEPROM Data Corruption Error";
      break;
    case 0x3F:
      the_significance = Message::PS_CRITICAL;
      the_text = "Main processor A/D Converter Error";
      break;
    case 0x40:
      the_significance = Message::PS_CRITICAL;
      the_text = "Sub processor A/D Converter Error";
      break;
    case 0x41:
      the_significance = Message::PS_CRITICAL;
      the_text = "ANALOG1 Output Error";
      break;
    case 0x42:
      the_significance = Message::PS_CRITICAL;
      the_text = "Gate Array Error";
      break;
    case 0x43:
      the_significance = Message::PS_CRITICAL;
      the_text = "ANALOG2 Output Error";
      break;
    case 0x44:
      the_significance = Message::PS_CRITICAL;
      the_text = "Inter-processor Communication Error";
      break;
    case 0x45:
      the_significance = Message::PS_CRITICAL;
      the_text = "Sub processor Initialization Error";
      break;
    case 0x46:
      the_significance = Message::PS_CRITICAL;
      the_text = "Sub processor SRAM Error";
      break;
    case 0x47:
      the_significance = Message::PS_CRITICAL;
      the_text = "Sub processor Code Loading Error";
      break;
    case 0x48:
      the_significance = Message::PS_CRITICAL;
      the_text = "Sub processor Startup Error";
      break;
    case 0x49:
      the_significance = Message::PS_CRITICAL;
      the_text = "Sub processor Checksum Error";
      break;
    case 0x4A:
      the_significance = Message::PS_CRITICAL;
      the_text = "Personality EEPROM Write Error";
      break;
    case 0x4B:
      the_significance = Message::PS_CRITICAL;
      the_text = "Service EEPROM Write Error";
      break;
    case 0x4C:
      the_significance = Message::PS_CRITICAL;
      the_text = "Software Clock Error";
      break;
    case 0x4D:
      the_significance = Message::PS_CRITICAL;
      the_text = "Sub processor Communication Checksum Error";
      break;
    case 0x4E:
      the_significance = Message::PS_CRITICAL;
      the_text = "Sine Table Generation Error";
      break;
    case 0x4F:
      the_significance = Message::PS_CRITICAL;
      the_text = "Personality Data Out Of Range Error";
      break;
    case 0x50:
      the_significance = Message::PS_CRITICAL;
      the_text = "Service Data Out Of Range Error";
      break;
    case 0x51:
      the_significance = Message::PS_CRITICAL;
      the_text = "Motor Block Checksum Error";
      break;
    }

  Message message_ps(Message::DR_GLOBAL, the_significance,
		     "Checking powerupstate:");
  message_ps.messageStream() << the_text;
  Messenger::relay()->sendMessage(message_rs);

  // Print the powerup state.

  if(bad_faultstate && bad_runstate && bad_powerup)return true;
  else return false;  
}

void TenMeterControlLoop::
setDigitalInputs(BRUDrive* bruDrive, DriveInfo& driveInfo)
{
  // Adapted "set_digital_inputs()" from controllers.c

  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  // First, disable the drive
  // DRIVE_DISABLE
  // Page 124 of BRU Command Book

  bruDrive->writeHostDriveEnableDisable(0);
  driveInfo.setStateEnabledFlag(DriveInfo::SE_DISABLED);
  if(driveInfo.getDriveName()=="AZ"){m_status.az.brakeReleased = false;}
  else{m_status.el.brakeReleased = false;}

  // Write out the two bit streams to the motor
  // WRITE_DIGITAL_INPUT
  // Page 58 of BRU Command Book

  // For each drive, set limit switch monitoring to normal operating
  // mode

  bruDrive->writeDigitalInputConfigurationRegister(0x00,0x0008);
  bruDrive->writeDigitalInputConfigurationRegister(0x01,0x0010);

  // Enable the drive
  // DRIVE_ENABLE
  // Page 124 of BRU Command Book

  bruDrive->writeHostDriveEnableDisable(1);
  driveInfo.setStateEnabledFlag(DriveInfo::SE_ENABLED);
  if(driveInfo.getDriveName()=="AZ"){m_status.az.brakeReleased = true;}
  else{m_status.el.brakeReleased = true;}
}

bool TenMeterControlLoop::
readDigitalInputs(BRUDrive* bruDrive, DriveInfo& driveInfo)
{
  // Adapted "read_digital_inputs()" from controllers.c

  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  // Message message(Message::DR_LOCAL,Message::PS_CRITICAL,
  //	  "Checking Digital Inputs");
  // message.messageStream() << "UNHANDLED DIGITAL IP STATE";

  bool limitsOK=true;

  // Check that the limits are enabled
  // READ_DIGITAL_INPUT x 4 for each motor
  // Page 58 of BRU Command Book

  // should = 8
  uint16_t ip0 = bruDrive->readDigitalInputConfigurationRegister(0);
  if(ip0!=8)
    {
      limitsOK=false;
      if(driveInfo.getDriveName()=="AZ")
	{
	  Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
			  "Checking Digital Inputs.");
	  message.messageStream() << "Problem with digital IP 0 "
				  << "of Az. limit setup";
	  Messenger::relay()->sendMessage(message);
	}
      else
	{
	  Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
			  "Checking Digital Inputs.");
	  message.messageStream() << "Problem with digital IP 0 "
				  << "of El limit setup";
	  Messenger::relay()->sendMessage(message);
	}
    }
  
  // should = 10
  uint16_t ip1 = bruDrive->readDigitalInputConfigurationRegister(1); 
  if(ip1!=10)
    {
      limitsOK=false;
      if(driveInfo.getDriveName()=="AZ")
	{
	  Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
			  "Checking Digital Inputs.");
	  message.messageStream() << "Problem with digital IP 1 "
				  << "of Az limit setup";
	  Messenger::relay()->sendMessage(message);
	}
      else
	{
	  Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
			  "Checking Digital Inputs.");
	  message.messageStream() << "Problem with digital IP 1 "
				  << "of El limit setup";
	  Messenger::relay()->sendMessage(message);
	}
    }

  // should = 0
  uint16_t ip2 = bruDrive->readDigitalInputConfigurationRegister(2);
  if(ip2!=0)
    {
      limitsOK=false;
      if(driveInfo.getDriveName()=="AZ")
	{
	  Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
			  "Checking Digital Inputs.");
	  message.messageStream() << "Problem with digital IP 2 "
				  << "of Az limit setup";
	  Messenger::relay()->sendMessage(message);
	}
      else
	{
	  Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
			  "Checking Digital Inputs.");
	  message.messageStream() << "Problem with digital IP 2 "
				  << "of El limit setup";
	  Messenger::relay()->sendMessage(message);
	}
    }

  // should = 0
  uint16_t ip3 = bruDrive->readDigitalInputConfigurationRegister(3);
  if(ip3!=0)
    {
      limitsOK=false;
      if(driveInfo.getDriveName()=="AZ")
	{
	  Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
			  "Checking Digital Inputs.");
	  message.messageStream() << "Problem with digital IP 3 "
				  << "of Az limit setup";
	  Messenger::relay()->sendMessage(message);
	}
      else
	{
	  Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
			  "Checking Digital Inputs.");
	  message.messageStream() << "Problem with digital IP 3 "
				  << "of El limit setup";
	  Messenger::relay()->sendMessage(message);
	}
    }

  if(limitsOK)
    {
      Message message(Message::DR_GLOBAL,Message::PS_ROUTINE,
		      "Digital Inputs check complete.");
      message.messageStream() << "Digital IPs all OK";
      Messenger::relay()->sendMessage(message);
    }
  return limitsOK;
}


bool TenMeterControlLoop::
initializeDIOCards()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  char* DevNameIO;

  if(m_CIO_1)close(m_CIO_1);
  if(m_CIO_2)close(m_CIO_2);

  DevNameIO = "/dev/dio48/dio0";
  if ((m_CIO_1 = open(DevNameIO, O_RDWR )) < 0) {
    perror(DevNameIO);
    Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
		    "Opening digital IO device 0");
    message.messageStream() << "Error opening device "
			    << "/dev/dio48/dio0";
    Messenger::relay()->sendMessage(message);
    exit(2); // TEMPORARY dree
    return false;
  }
  
  DevNameIO = "/dev/dio48/dio1";
  if ((m_CIO_2 = open(DevNameIO, O_RDWR )) < 0) {
    perror(DevNameIO);
    Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
		    "Opening digital IO device 1");
    message.messageStream() << "Error opening device "
			    << "/dev/dio48/dio1";
    Messenger::relay()->sendMessage(message);
    exit(2); // TEMPORARY
    return false;
  }
  
  // In DIO Software bundle these values and functions are defined.
  // (details below)
 
  ioctl(m_CIO_1, DIO_SET_MODE, CNTL_A|CNTL_B|CNTL_C);
  ioctl(m_CIO_2, DIO_SET_MODE, CNTL_A|CNTL_B|CNTL_C);

  // The following ioctl() functions have been implemented (see dio48.h):

  // 1. ioctl(fd, DIO_SET_MODE, value);

  // Sets the mode of the 82C55.  The 8th bit (mode_set) is
  // automatically set, so the high bit of "value" is ignored.  The
  // port mode "value" can  be set to the following values:

  //      OUTPUT          (all channels configured for output)
  //      CNTL_A          (Port A configured for input)
  //      CNTL_B          (Port B configured for input)
  //      CNTL_C          (Port C configured for input)
  //      CNTL_CL         (Port CL (C, low 4 bits) configured for input)
  //      CNTL_CH         (Port CH (C, high 4 bits) configured for input)

  // These may be OR'd together for an arbitrary input/output
  // configuration.  The board defaults to all ports configured for
  // input.

  return true;
}// end initializeDIOCards

bool TenMeterControlLoop::
readTelescopeInformation(ang_t& az, ang_t& el, uint8_t& limitBits,
			 uint8_t& statusBits)
{

  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  // Read in the current position from the DIO cards
  // Adapted function "gettel" from 10meter.c

  int enc_bin, count=0;
  uint32_t data1=0, data2=0;
  uint8_t  port1[3], port2[3], temp=0;
  
  do{
    read(m_CIO_1, &data1, 3);
    // Splits the input 24 bits into an encoder 16 bits and a status 8
    // bits
    split(data1,port1);
    // split divides the 24-bit read from the card into three 8-bit
    // parts and stores these in the array "port1"
    limitBits=port1[2];
    read(m_CIO_1, &data1, 3);
    split(data1,port1);
    temp=port1[2];
    count++;
    limitBits &= 63; // 111111=63 (6 bits)
    temp &= 63;        // only interested in comparing the first 6 bits
  }while((limitBits != temp) && count < 10);
  
  if(count>=10)
    printf("Tried 10 times to get CIO_1 port.\n");
  // can I put an indication to this somewhere on the GUI?
  count=0;
  temp=0;
  
  do{
    read(m_CIO_2, &data2, 3);
    split(data2,port2);
    statusBits=port2[2];
    read(m_CIO_2, &data2, 3);
    split(data2,port2);
    temp=port2[2];
    count++;
  }while((statusBits != temp) && count < 10); // compare all bits this time
  
  if(count>=10)
    printf("Tried 10 times to get CIO_2 port.\n");
  // can I put an indication to this somewhere on the GUI??
  enc_bin = (int) port1[1];
  enc_bin <<= 8;
  enc_bin += (int) port1[0];
  
  // ?? is WORD = uint16_t?
  enc_bin = gb((uint16_t) enc_bin, 16);
  az = (float) enc_bin * ENCODER_TO_DEG;
  
  enc_bin = (int) port2[1];
  enc_bin <<= 8;
  enc_bin += (int) port2[0];
  enc_bin = gb((short) enc_bin, 16);
  
  el =(float) enc_bin * ENCODER_TO_DEG;

  return true;

} // end readTelescopeInformation

int TenMeterControlLoop::
checkLimitAndStatusBits()
{
  // Function "check_status()" from 10meter.c is adapted here

  // It returns 0 if all is fine.

  // The 6 Limit Bits:
  // 2^0: EL_DOWN
  // 2^1: EL_UP
  // 2^2: EL_15
  // 2^3: AZ_CCW 
  // 2^4: AZ_CW  
  // 2^5: WINDOW 

  // The 8 Status Bits:
  // 2^0: EL_OK
  // 2^1: EL_AT_SPEED
  // 2^2: AZ_OK
  // 2^3: AZ_AT_SPEED
  // 2^4: CW_CCW
  // 2^5: HT_ON
  // 2^6: EL_AUTO
  // 2^7: AZ_AUTO

  // If bit is set to 0; limit has been hit        i.e. Not OK
  // If bit is set to 1; limit has not been hit    i.e. OK
  
  int returnValue=0; // 0 is what gets returned if all if fine

  if(!(limitBits & AZ_CW))
    {
      m_cw_enable=false;
      m_status.az.limitCwUp=true;

      // dree do you really want to disable the drive ?

      // Disable the drive
      m_azDrive->writeHostDriveEnableDisable(0);
      m_azDriveInfo.setStateEnabledFlag(DriveInfo::SE_DISABLED);
      m_status.az.brakeReleased = false;

      m_status.az.driveMode = ScopeAPI::DM_STANDBY;
      // Any time driveMode is set to STANDBY, reset current and last
      // positions, velocities and times.
      m_az_req_ang=0;
      m_az_req_vel=0;
      m_last_az_req_ang=0;
      m_last_az_req_vel=0;
      m_last_tv_cmdPoint.tv_sec=0;
      m_last_tv_cmdPoint.tv_usec=0;
      m_time_diff=0;
      Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
		      "AZ_CW bit is set.");
      message.messageStream() 
	<< "Clockwise limit has been hit. Azimuth drive mode has "
	<< "been set to STANDBY. Azimuth drive has been disabled.";
      Messenger::relay()->sendMessage(message);
      returnValue=1;
    }
  else
    {
      m_cw_enable=true;
      m_status.az.limitCwUp=false;
    } // end if(!(limitBits & AZ_CW))
  
  if(!(limitBits & AZ_CCW))
    {
      m_ccw_enable=false;
      m_status.az.limitCcwDown=true;

      // dree do you really want to disable the drive ?

      // Disable the drive
      m_azDrive->writeHostDriveEnableDisable(0);
      m_azDriveInfo.setStateEnabledFlag(DriveInfo::SE_DISABLED);
      m_status.az.brakeReleased = false;

      m_status.az.driveMode = ScopeAPI::DM_STANDBY;
      // Any time driveMode is set to STANDBY, reset current and last
      // positions, velocities and times.
      m_az_req_ang=0;
      m_az_req_vel=0;
      m_last_az_req_ang=0;
      m_last_az_req_vel=0;
      m_last_tv_cmdPoint.tv_sec=0;
      m_last_tv_cmdPoint.tv_usec=0;
      m_time_diff=0;
      Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
		      "AZ_CCW bit is set.");
      message.messageStream() 
	<< "Counterclockwise limit has been hit. Azimuth drive mode has "
	<< "been set to STANDBY. Azimuth drive has been disabled.";
      Messenger::relay()->sendMessage(message);
      returnValue=1;
    }
  else
    {
      m_ccw_enable=true;
      m_status.az.limitCcwDown=false;
    } // end if(!(limitBits & AZ_CCW))

  if(!(limitBits & EL_UP))
    {
      m_up_enable=false;
      m_status.el.limitCwUp=true;  

      // dree do you really want to disable the drive ?

      // Disable the drive
      m_elDrive->writeHostDriveEnableDisable(0);
      m_elDriveInfo.setStateEnabledFlag(DriveInfo::SE_DISABLED);
      m_status.el.brakeReleased = false;

      m_status.el.driveMode = ScopeAPI::DM_STANDBY;
      // Any time driveMode is set to STANDBY, reset current and last
      // positions, velocities and times.
      m_el_req_ang=0;
      m_el_req_vel=0;
      m_last_el_req_ang=0;
      m_last_el_req_vel=0;
      m_last_tv_cmdPoint.tv_sec=0;
      m_last_tv_cmdPoint.tv_usec=0;
      m_time_diff=0;
      Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
		      "EL_UP bit is set.");
      message.messageStream() 
	<< "Up limit has been hit. Elevation drive mode has "
	<< "been set to STANDBY. Elevation drive has been disabled.";
      Messenger::relay()->sendMessage(message);
      returnValue=1;
    }
  else
    {
      m_up_enable=true;
      m_status.el.limitCwUp=false;
    } // end if(!(limitBits & EL_UP))

  if(!(limitBits & EL_DOWN))
    {
      m_down_enable=false;
      m_status.el.limitCcwDown=true;    

      // dree do you really want to disable the drive ?

      // Disable the drive
      m_elDrive->writeHostDriveEnableDisable(0);
      m_elDriveInfo.setStateEnabledFlag(DriveInfo::SE_DISABLED);
      m_status.el.brakeReleased = false;

      m_status.el.driveMode = ScopeAPI::DM_STANDBY;
      // Any time driveMode is set to STANDBY, reset current and last
      // positions, velocities and times.
      m_el_req_ang=0;
      m_el_req_vel=0;
      m_last_el_req_ang=0;
      m_last_el_req_vel=0;
      m_last_tv_cmdPoint.tv_sec=0;
      m_last_tv_cmdPoint.tv_usec=0;
      m_time_diff=0;
      Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
		      "EL_DOWN bit is set.");
      message.messageStream() 
	<< "Down limit has been hit. Elevation drive mode has "
	<< "been set to STANDBY. Elevation drive has been disabled.";
      Messenger::relay()->sendMessage(message);
      returnValue=1;
    }
  else
    {
      m_down_enable=true;
      m_status.el.limitCcwDown=false;
    } // end if(!(limitBits & EL_DOWN))

  if(!(limitBits & EL_15))
    {
      m_el_15=true;
      Message message(Message::DR_GLOBAL,Message::PS_ROUTINE,
		      "EL_15 bit is set.");
      message.messageStream() 
	<< "Elevation is less than 15deg. This is only allowed "
	<< "when the telescope is between AZ 2.5 and 357.5.";
      Messenger::relay()->sendMessage(message);
    }
  else
    {
      m_el_15=false;
    } // end if(!(limitBits & EL_15))

  if(!(limitBits & WINDOW))
    {
      // This is fine. It just means that we're allowed to go below 15
      // degrees.
      m_az_window=true;
      Message message(Message::DR_GLOBAL,Message::PS_ROUTINE,
		      "WINDOW bit is set.");
      message.messageStream() 
	<< "Azimuth is between 357.5 and 2.5. Telescope is allowed "
	<< "to go below 15deg in this window.";
      Messenger::relay()->sendMessage(message);
    }
  else
    {
      m_az_window=false;
    } // end if(!(limitBits & WINDOW))
  
  // Compare the elevation with the azimuth to make sure the telescope
  // is not below 15degrees outside of the key

  if(m_el_15==true && m_az_window==false)
    {
      // If we are below 15 degrees, outside the key, disable the
      // drives
      
      // dree do you really want to disable the drive ?

      // Disable the drives
      m_elDrive->writeHostDriveEnableDisable(0);
      m_elDriveInfo.setStateEnabledFlag(DriveInfo::SE_DISABLED);
      m_status.el.brakeReleased = false;

      m_azDrive->writeHostDriveEnableDisable(0);
      m_azDriveInfo.setStateEnabledFlag(DriveInfo::SE_DISABLED);
      m_status.az.brakeReleased = false;

      m_status.az.positionFault=true;
      m_status.el.positionFault=true;

      m_status.el.driveMode = ScopeAPI::DM_STANDBY;
      m_status.az.driveMode = ScopeAPI::DM_STANDBY;
      // Any time driveMode is set to STANDBY, reset current and last
      // positions, velocities and times.
      m_az_req_ang=0;
      m_el_req_ang=0;
      m_az_req_vel=0;
      m_el_req_vel=0;
      m_last_az_req_ang=0;
      m_last_el_req_ang=0;
      m_last_az_req_vel=0;
      m_last_el_req_vel=0;
      m_last_tv_cmdPoint.tv_sec=0;
      m_last_tv_cmdPoint.tv_usec=0;

      m_time_diff=0;

      Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
		      "PROBLEM WITH KEY.");
      message.messageStream() 
	<< "The Elevation is below 15 degrees but the Azimuth is not "
	<<" between 357.5 and 2.5. Telescope should not be in this position.";
      Messenger::relay()->sendMessage(message);
      returnValue=1;
    }
  else if(m_el_15==true && m_az_window==true)
    {
      Message message(Message::DR_GLOBAL,Message::PS_ROUTINE,
		      "TELESCOPE IN KEY.");
      message.messageStream() 
	<< "The Telescope is in the key.";
      Messenger::relay()->sendMessage(message);
    } // end if(m_el_15==true && m_az_window==false)
  
  if(statusBits & EL_OK)
    {
      m_el_ready=true;
    }
  else
    {
      m_el_ready=false;
      Message message(Message::DR_GLOBAL,Message::PS_UNUSUAL,
		      "EL_OK bit not set.");
      message.messageStream() 
	<< "Elevation not ready.";
      Messenger::relay()->sendMessage(message);
      returnValue=1;
    } // end if(statusBits & EL_OK)
  
  if(statusBits & AZ_OK)
    {
      m_az_ready=true;
    }
  else
    {
      m_az_ready=false;
      Message message(Message::DR_GLOBAL,Message::PS_UNUSUAL,
		      "AZ_OK bit not set.");
      message.messageStream() 
	<< "Azimuth not ready.";
      Messenger::relay()->sendMessage(message);
      returnValue=1;
    } // end if(statusBits & AZ_OK)

  if(!(statusBits & EL_AT_SPEED))
    {
      // dree take out all these messages !!

      Message message(Message::DR_GLOBAL,Message::PS_ROUTINE,
		      "EL_AT_SPEED bit is set.");
      message.messageStream() 
	<< "Elevation is at speed.";
      Messenger::relay()->sendMessage(message);
    }
  else
    {
      Message message(Message::DR_GLOBAL,Message::PS_ROUTINE,
		      "EL_AT_SPEED bit is not set.");
      message.messageStream() 
	<< "Elevation is not at speed.";
      Messenger::relay()->sendMessage(message);
    } // end if(!(statusBits & EL_AT_SPEED))

  if(!(statusBits & AZ_AT_SPEED))
    {
      Message message(Message::DR_GLOBAL,Message::PS_ROUTINE,
		      "AZ_AT_SPEED bit is set.");
      message.messageStream() 
	<< "Azimuth is at speed.";
      Messenger::relay()->sendMessage(message);
    }
  else
    {
      Message message(Message::DR_GLOBAL,Message::PS_ROUTINE,
		      "AZ_AT_SPEED bit is not set.");
      message.messageStream() 
	<< "Azimuth is not at speed.";
      Messenger::relay()->sendMessage(message);
    } // end if(!(statusBits & AZ_AT_SPEED))

  if(!(statusBits & CW_CCW))
    {
      Message message(Message::DR_GLOBAL,Message::PS_ROUTINE,
		      "Read CW_CCW bit.");
      message.messageStream() 
	<< "Azimuth rotation is Clockwise.";
      Messenger::relay()->sendMessage(message);
    }
  else
    {
      Message message(Message::DR_GLOBAL,Message::PS_ROUTINE,
		      "Read CW_CCW bit.");
      message.messageStream() 
	<< "Azimuth rotation is Counterclockwise.";
      Messenger::relay()->sendMessage(message);
    } // end if(!(statusBits & CW_CCW))

  m_status.remoteControl = true;
  m_status.interlock=false;

  if(!(statusBits & EL_AUTO))
    {
      Message message(Message::DR_GLOBAL,Message::PS_ROUTINE,
		      "Read EL_AUTO bit.");
      message.messageStream() 
	<< "Elevation is in automatic mode.";
      Messenger::relay()->sendMessage(message);
    }
  else
    {
      Message message(Message::DR_GLOBAL,Message::PS_EXCEPTIONAL,
		      "Read EL_AUTO bit.");
      message.messageStream() 
	<< "Elevation is in manual mode.";
      Messenger::relay()->sendMessage(message);
      m_status.remoteControl = false;
      m_status.interlock=true;
      } // end if(!(statusBits & EL_AUTO))
  
  if(!(statusBits & AZ_AUTO))
    {
      Message message(Message::DR_GLOBAL,Message::PS_ROUTINE,
		      "Read AZ_AUTO bit.");
      message.messageStream() 
	<< "Azimuth is in automatic mode.";
      Messenger::relay()->sendMessage(message);
    }
  else
    {
      Message message(Message::DR_GLOBAL,Message::PS_EXCEPTIONAL,
		      "Read AZ_AUTO bit.");
      message.messageStream() 
	<< "Azimuth is in manual mode.";
      Messenger::relay()->sendMessage(message);
      m_status.remoteControl = false;
      m_status.interlock=true;
    } // end if(!(statusBits & AZ_AUTO))

  if(!(statusBits & HT_ON))
    {
      // ???
    }
  else
    {
      // ???
    } // end if(!(statusBits & HT_ON))
  
  return returnValue;
  
} // end checkLimitAndStatusBits

bool TenMeterControlLoop::
initialize()
{
  // Three Main steps:
  // 1. Initialize each of the BRU Drives
  // 2. Check the status of each of the BRU Drives
  // 3. Initialize the DIO cards

  // Do the SE's whenever you call stateenable etc.
  // Do the DM's here 
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  // ************************************
  // 1. Initialize each of the BRU Drives
  // ************************************
  bool successInitialize=true;
  
  if(initializeBRUDrive(m_azDrive, m_azDriveInfo))
    {
      m_status.az.driveMode = ScopeAPI::DM_STANDBY;
      // Any time driveMode is set to STANDBY, reset current and last
      // positions, velocities and times.
      m_az_req_ang=0;
      m_az_req_vel=0;
      m_last_az_req_ang=0;
      m_last_az_req_vel=0;
      m_last_tv_cmdPoint.tv_sec=0;
      m_last_tv_cmdPoint.tv_usec=0;
      m_time_diff=0;
    }
  else
    {
      m_status.az.driveMode = ScopeAPI::DM_UNKNOWN;
      successInitialize=false;
    }
  
  if(initializeBRUDrive(m_elDrive, m_elDriveInfo))
    {
      m_status.el.driveMode = ScopeAPI::DM_STANDBY;
      // Any time driveMode is set to STANDBY, reset current and last
      // positions, velocities and times.
      m_el_req_ang=0;
      m_el_req_vel=0;
      m_last_el_req_ang=0;
      m_last_el_req_vel=0;
      m_last_tv_cmdPoint.tv_sec=0;
      m_last_tv_cmdPoint.tv_usec=0;
      m_time_diff=0;
    }
  else
    {
      m_status.el.driveMode = ScopeAPI::DM_UNKNOWN;
      successInitialize=false;
    }
  
  // *********************************************
  // 2. Check the status of each of the BRU Drives
  // *********************************************

  bool azPowerupSuccess;
  bool elPowerupSuccess;
  
  if(getBRUDriveStatus(m_azDrive,m_azDriveInfo,azPowerupSuccess))
    {
      if(azPowerupSuccess)
	{
	  // Successful powerup
	  m_status.az.servoOn = true;
	}
      else
	{
	  m_status.az.servoOn = false;
	}    
    }	
  else
    {
      successInitialize=false;
    }
  
  if(getBRUDriveStatus(m_elDrive,m_elDriveInfo,elPowerupSuccess))
    {      
      if(elPowerupSuccess)
	{
	  // Successful powerup
	  m_status.el.servoOn = true;
	}
      else
	{
	  m_status.el.servoOn = false;
	}
    }
  else
    {
      successInitialize=false;
    }

  // ***************************
  // 3. Initialize the DIO cards
  // ***************************
  
  // What do I set here?
  if(!initializeDIOCards())
    {
      successInitialize=false;
    }

  return successInitialize;
  
}
// End of TenMeterControlLoop Member functions


#ifdef TESTMAIN
int main(int argc, char**argv)
{
  argc--,argv++;
  TenMeterControlLoop::ang_t Kp;
  TenMeterControlLoop::ang_t Ki;
  TenMeterControlLoop::ang_t Kd;
  TenMeterControlLoop::ang_t Ilim;
  
  TenMeterControlLoop::ang_t ang = 0;
  TenMeterControlLoop::ang_t vel = 0;

  TenMeterControlLoop::ang_t req_ang = 5;
  TenMeterControlLoop::ang_t req_vel = 0.5;

  TenMeterControlLoop::ang_t max_vel = 2.0;
  TenMeterControlLoop::ang_t max_acc = 0.1;

  TenMeterControlLoop::ang_t tar_vel = 0;

  double t = 0;
  double deltat = 0.05;

  if(argc) { std::istringstream(*argv++) >> Kp; argc--; }
  if(argc) { std::istringstream(*argv++) >> Ki; argc--; }
  if(argc) { std::istringstream(*argv++) >> Kd; argc--; }
  if(argc) { std::istringstream(*argv++) >> Ilim; argc--; }
  if(argc) { std::istringstream(*argv++) >> tar_vel; argc--; }
  if(argc) { std::istringstream(*argv++) >> req_ang; argc--; }

  PID<TenMeterControlLoop::ang_t> pid;
  pid.setPGain(Kp);
  pid.setIGain(Ki);
  pid.setDGain(Kd);
  pid.setILimit(Ilim);

  Debug::stream()
    << pid.pGain() << ' ' << pid.iGain() << ' ' << pid.dGain() << ' '
    << pid.iLimit() << std::endl;

  int count=0;

  while(count<100)
    {
      Debug::stream()
	<< std::fixed 
	<< std::setw(6) << std::setprecision(2) << t << ' '
	<< std::setw(8) << std::setprecision(5) << ang << ' '
	<< std::setw(8) << std::setprecision(5) << req_ang << ' '
	<< std::setw(8) << std::setprecision(5) << ang-req_ang << ' '
	<< std::setw(8) << std::setprecision(5) << vel << std::endl;
      move(deltat, ScopeAPI::DM_POINT, pid, ang, vel,
	   req_ang, req_vel, max_vel, max_acc, false, false);
      req_ang+=tar_vel*deltat;
      t+=deltat;
      if(fabs(vel-tar_vel)<0.00001)count++;
      else count=0;
    }
} // end main
#endif


// Functions from 10meter.c

//  Convert m-bit graycode number n to binary
//
//  Formula:    b(MSB) = g(MSB)
//              b(i-1) = b(i) ^ g(i-1)

int TenMeterControlLoop::gb(int n, int m)
{
  int ans=0, bits=1<<(m-1);
  for( ans=n&bits;bits!=0;bits>>=1)
    ans |= (n^(ans>>1)) & bits;
  return ans;
}


//  Print out binary number n
//  with m bits

void TenMeterControlLoop::printbin(uint32_t n, int m)
{
  int i;
  for(i=1<<(m-1);i>0;i>>=1)
    (n&i) ? printf("1") : printf("0");
  return;
}

void TenMeterControlLoop::split(uint32_t n, uint8_t *m)
  // Split the input 24 bits into an encoder 16 bits and a status 8 bits */
{
  
  m[0]=(uint8_t) ((n) & (255));
  n>>=8; // shifts "n" 8 bits to the right.. essentially it divides n by 256
         // divide by 2^8
  m[1]=(uint8_t) ((n) & (255));
  n>>=8; // shifts "n" 8 bits to the right.. essentially it divides n by 256
         // divide by 2^8
  m[2]=(uint8_t) ((n) & (255));

}   

// From controllers.c

int32_t TenMeterControlLoop::dps_to_int(const DriveInfo* drive_in)
// convert the degrees per sec value to an 32-bit integer
{
  double rps = drive_in->getVelocity()*drive_in->getRPD();

  // Limit speed to maximum motor speed 
  if(rps > drive_in->getMaxRPS())rps = drive_in->getMaxRPS();
  else if(rps < -drive_in->getMaxRPS())rps = -drive_in->getMaxRPS();

  // Convert to motor shaft velocity -- ONE_RPS is conversion factor
  rps *= ONE_RPS;

  // Round to nearest integer to send to motors
  return int32_t(round(rps));
}

// These had been already commented out and were before the destructor
// of TenMeterControlLoop

// Getters and Setters for DriveInfo Class are all in .h file

//#ifdef THE_POPE_IS_CATHOLIC
//TenMeterControlLoop::DriveInfo::DriveInfoState 
//TenMeterControlLoop::DriveInfo::
//getDriveInfoState() const
//{
//  return DriveInfoState;
//}
//#endif

//#ifdef THE_POPE_IS_CATHOLIC
//void TenMeterControlLoop::DriveInfo::
//setDriveInfoState(TenMeterControlLoop::DriveInfoState infoState)
//{
//  DriveInfoState=state;
//}
//#endif

//void TenMeterControlLoop::DriveInfo::setVelocity(int velocity)
//{
//  // Move the current velocity to "driveLastVelocity"
//  //  m_driveLastVelocity=m_driveVelocity;
//  // Set the current velocity to be the new value
//  m_driveVelocity=velocity;
//}

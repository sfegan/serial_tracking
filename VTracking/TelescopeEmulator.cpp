//-*-mode:c++; mode:font-lock;-*-

/**
 * \file TelescopeEmulator.cpp
 * \ingroup VTracking
 * \brief This is a one-line description of this cpp file.
 *
 * Here is a tedious verbose multi-line description of all
 * the details of the code, more than you would
 * ever want to read. Generally, all the important documentation
 * goes in the .cpp files.
 *
 * Original Author: Stephen Fegan
 * $Author: sfegan $
 * $Date: 2006/04/10 18:01:11 $
 * $Revision: 2.1 $
 * $Tag$
 *
 **/

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

#include <zthread/Thread.h>
#include <zthread/Guard.h>

#include <Exception.h>
#include <Debug.h>
#include <Angle.h>

#include "TelescopeEmulator.h"
#include "TextMenu.h"

using namespace SEphem;
using namespace VTracking;
using namespace VMessaging;

#define PGAIN_FWD(x) (int(round(x)))
#define PGAIN_BWD(x) (double(x))
#define IGAIN_FWD(x) (int(round(x*256.0)))
#define IGAIN_BWD(x) (double(x)/256.0)
#define DGAIN_FWD(x) (int(round(x)))
#define DGAIN_BWD(x) (double(x))
#define VGAIN_FWD(x) (int(round(x*4.0)))
#define VGAIN_BWD(x) (double(x)/4.0)
#define ANG_FWD(x) (x)
#define ANG_BWD(x) (x)

TelescopeEmulator::
TelescopeEmulator(unsigned scope, unsigned sleep_time_ms):
  m_sleep_time(sleep_time_ms),
  m_mtx(), 
  m_exit_notification(false),
  m_scope_num(scope),
  m_last_update_time(),
  m_az_scheduler(),
  m_el_scheduler(),
  m_az_ang_pid(),
  m_el_ang_pid(),
  m_physics(),
  m_limits(),
  m_status(), 
  m_az_req_ang(0),
  m_el_req_ang(0),
  m_az_req_vel(0),
  m_el_req_vel(0),
  m_az_req_max_vel(1.0),
  m_el_req_max_vel(1.0), 
  m_az_offset(0),
  m_el_offset(0), 
  m_cw_soft_lim(271),
  m_cc_soft_lim(-271),
  m_up_soft_lim(91),
  m_dn_soft_lim(-1),
  m_key_cw_soft_lim(0),
  m_key_cc_soft_lim(0),
  m_key_dn_soft_lim(-1),
  m_pullcord_lim(272)
{
  m_az_scheduler = 
    new TrapezoidalSetpointScheduler(double(m_sleep_time)/1000.0, 
				     m_az_req_max_vel, 0.2);
  m_el_scheduler = 
    new TrapezoidalSetpointScheduler(double(m_sleep_time)/1000.0, 
				     m_el_req_max_vel, 0.2);
  
  m_physics = new SimpleTelescopePhysicsModel(double(m_sleep_time)/1000.0);

  m_az_ang_pid.setPGain(PGAIN_BWD(3));
  m_az_ang_pid.setIGain(IGAIN_BWD(2));
  m_az_ang_pid.setDGain(DGAIN_BWD(0));
  m_az_ang_pid.setILimit(50);
  m_az_ang_pid.setVffGain(VGAIN_BWD(0));

  m_el_ang_pid.setPGain(PGAIN_BWD(3));
  m_el_ang_pid.setIGain(IGAIN_BWD(2));
  m_el_ang_pid.setDGain(DGAIN_BWD(0));
  m_el_ang_pid.setILimit(50);
  m_el_ang_pid.setVffGain(VGAIN_BWD(0));

#if 0
  if(scope == 0)
    {
      m_dn_soft_lim = 14;
      m_key_cw_soft_lim = 2;
      m_key_cc_soft_lim = -2;
    }
#endif

  m_limits = new KeyLimits(0,0,
			   m_key_cw_soft_lim, m_key_cc_soft_lim,
			   m_key_dn_soft_lim,
			   m_dn_soft_lim, m_up_soft_lim,
			   m_cw_soft_lim, m_cc_soft_lim);

  m_status.remoteControl = true;
  m_status.checksumOK = true;
  m_status.az.driveMode = ScopeAPI::DM_STANDBY;
  m_status.el.driveMode = ScopeAPI::DM_STANDBY;

  load();
  save();
}
 
TelescopeEmulator::
~TelescopeEmulator()
{
  delete(m_az_scheduler);
  delete(m_el_scheduler);
  delete(m_physics);
  delete(m_limits);
}
    
void TelescopeEmulator::
reqStat(PositionerStatus& state)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  state = m_status;
}

void TelescopeEmulator::
cmdStandby()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  if(m_status.interlock)return;

  m_status.az.positionFault=false;
  m_status.el.positionFault=false;
  m_status.az.driveMode=DM_STANDBY;
  m_status.el.driveMode=DM_STANDBY;
}

void TelescopeEmulator::
cmdPoint(const SEphem::Angle& az_angle, double az_vel,
	 const SEphem::Angle& el_angle, double el_vel)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  if(m_status.interlock)return;

  double az_x;
  double el_x;
  double az_v;
  double el_v;
  double az_i;
  double el_i;
  m_physics->getState(az_x, el_x, az_v, el_v, az_i, el_i);
  
  double az1=az_angle.deg();
  double az2=az1-360;
  if((az1>270)&&(az2<-270))abort(); // never happen
  else if(az1>270)m_az_req_ang=az2;
  else if(az2<-270)m_az_req_ang=az1;
  else if(fabs(az1-az_x)<=fabs(az2-az_x))m_az_req_ang=az1;
  else m_az_req_ang=az2;
  m_el_req_ang=el_angle.degPM();

  az_vel=fabs(az_vel);
  if(az_vel>m_az_req_max_vel)az_vel=m_az_req_max_vel;
  m_az_req_vel=az_vel;

  el_vel=fabs(el_vel);
  if(el_vel>m_el_req_max_vel)el_vel=m_el_req_max_vel;
  m_el_req_vel=el_vel;

  bool lim_dn;
  bool lim_up;
  bool lim_cw;
  bool lim_cc;

  if((m_limits)&&
     (!m_limits->isDriveAngleInsideLimits(Angle::frDeg(m_az_req_ang),
					  Angle::frDeg(m_el_req_ang),
					  lim_cc, lim_cw, lim_dn, lim_up)))
    {
      m_status.az.positionFault=true;
      m_status.el.positionFault=true;
      m_status.az.driveMode=DM_STANDBY;
      m_status.el.driveMode=DM_STANDBY;
    }
  else
    {
      if(m_status.az.driveMode == DM_STANDBY)
	m_az_scheduler->syncScheduler(az_x, az_v);
      m_az_scheduler->setMaxSpeed(m_az_req_vel);
      m_az_scheduler->setXTarget(m_az_req_ang);
      if(m_status.el.driveMode == DM_STANDBY)
	m_el_scheduler->syncScheduler(el_x, el_v);
      m_el_scheduler->setMaxSpeed(m_el_req_vel);
      m_el_scheduler->setXTarget(m_el_req_ang);
      m_status.az.positionFault=false;
      m_status.el.positionFault=false;
      m_status.az.driveMode=DM_POINT;
      m_status.el.driveMode=DM_POINT;
    }
}

void TelescopeEmulator::
cmdSlew(double az_vel, double el_vel)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  if(m_status.interlock)return;

  // Set the requested speeds
  if(az_vel>m_az_req_max_vel)az_vel=m_az_req_max_vel;
  else if(az_vel<-m_az_req_max_vel)az_vel=-m_az_req_max_vel;
  m_az_req_vel=az_vel;
  if(el_vel>m_el_req_max_vel)el_vel=m_el_req_max_vel;
  else if(el_vel<-m_el_req_max_vel)el_vel=-m_el_req_max_vel;
  m_el_req_vel=el_vel;
  
  // Fake the requested positions
  if(m_az_req_vel>0)m_az_req_ang = m_cw_soft_lim;
  else m_az_req_ang = m_cc_soft_lim;
  if(m_el_req_vel>0)m_el_req_ang = m_up_soft_lim;
  else m_el_req_ang = m_dn_soft_lim;

  double az_x;
  double el_x;
  double az_v;
  double el_v;
  double az_i;
  double el_i;
  m_physics->getState(az_x, el_x, az_v, el_v, az_i, el_i);
  m_az_scheduler->syncScheduler(az_x, az_v);
  m_az_scheduler->setMaxSpeed(m_az_req_vel);
  m_az_scheduler->setXTarget(m_az_req_ang);
  m_el_scheduler->syncScheduler(el_x, el_v);
  m_el_scheduler->setMaxSpeed(m_el_req_vel);
  m_el_scheduler->setXTarget(m_el_req_ang);
  m_status.az.positionFault=false;
  m_status.el.positionFault=false;
  m_status.az.driveMode=DM_SLEW;
  m_status.el.driveMode=DM_SLEW;
}

void TelescopeEmulator::
reqAzOffset(SEphem::Angle& az_angle)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  az_angle.setDeg(m_az_offset);
}

void TelescopeEmulator::
reqElOffset(SEphem::Angle& el_angle)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  el_angle.setDeg(m_el_offset);
}

void TelescopeEmulator::
setAzOffset(const SEphem::Angle& az_angle)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  if(m_status.interlock)return;

  m_az_offset = az_angle.degPM();
}

void TelescopeEmulator::
setElOffset(const SEphem::Angle& el_angle)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  if(m_status.interlock)return;

  m_el_offset = el_angle.degPM();
}

void TelescopeEmulator::
reqAzPIDParameters(PIDParameters& az_param)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  az_param.Kp   = PGAIN_FWD(m_az_ang_pid.pGain());
  az_param.Ki   = IGAIN_FWD(m_az_ang_pid.iGain());
  az_param.Kd   = DGAIN_FWD(m_az_ang_pid.dGain());
  az_param.Ilim = m_az_ang_pid.iLimit();
  az_param.Kvff = VGAIN_FWD(m_az_ang_pid.vffGain());
  az_param.vlim = ANG_FWD(m_az_req_max_vel);
  az_param.alim = ANG_FWD(m_az_scheduler->getAMax());
}

void TelescopeEmulator::
reqElPIDParameters(PIDParameters& el_param)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  el_param.Kp   = PGAIN_FWD(m_el_ang_pid.pGain());
  el_param.Ki   = IGAIN_FWD(m_el_ang_pid.iGain());
  el_param.Kd   = DGAIN_FWD(m_el_ang_pid.dGain());
  el_param.Ilim = m_el_ang_pid.iLimit();
  el_param.Kvff = VGAIN_FWD(m_el_ang_pid.vffGain());
  el_param.vlim = ANG_FWD(m_el_req_max_vel);
  el_param.alim = ANG_FWD(m_el_scheduler->getAMax());
}

void TelescopeEmulator::
setAzPIDParameters(const PIDParameters& az_param)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  if(m_status.interlock)return;

  m_az_ang_pid.setPGain(PGAIN_BWD(az_param.Kp));
  m_az_ang_pid.setIGain(IGAIN_BWD(az_param.Ki));
  m_az_ang_pid.setDGain(DGAIN_BWD(az_param.Kd));
  m_az_ang_pid.setILimit(az_param.Ilim);
  m_az_ang_pid.setVffGain(VGAIN_BWD(az_param.Kvff));
  m_az_req_max_vel = ANG_BWD(fabs(az_param.vlim));
  if(m_az_req_max_vel<m_az_req_vel)m_az_scheduler->setMaxSpeed(m_az_req_vel);
  m_az_scheduler->setMaxAccel(ANG_BWD(fabs(az_param.alim)));
}

void TelescopeEmulator::
setElPIDParameters(const PIDParameters& el_param)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  if(m_status.interlock)return;

  m_el_ang_pid.setPGain(PGAIN_BWD(el_param.Kp));
  m_el_ang_pid.setIGain(IGAIN_BWD(el_param.Ki));
  m_el_ang_pid.setDGain(DGAIN_BWD(el_param.Kd));
  m_el_ang_pid.setILimit(el_param.Ilim);
  m_el_ang_pid.setVffGain(VGAIN_BWD(el_param.Kvff));
  m_el_req_max_vel = ANG_BWD(fabs(el_param.vlim));
  if(m_el_req_max_vel<m_el_req_vel)
    m_el_scheduler->setMaxSpeed(m_el_req_max_vel);
  m_el_scheduler->setMaxAccel(ANG_BWD(fabs(el_param.alim)));
}

void TelescopeEmulator::
resetCommunication()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  // nothing to see here
}

std::string TelescopeEmulator::apiName() const
{
  return std::string("EMULATOR");
}

void TelescopeEmulator::
terminate()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  m_exit_notification=true;
}

void TelescopeEmulator::
run()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  gettimeofday(&m_last_update_time,0);
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

      struct timeval now;
      gettimeofday(&now,0);
      
      int deltatime = 
	(now.tv_sec-m_last_update_time.tv_sec)*1000+
	(now.tv_usec-m_last_update_time.tv_usec)/1000+m_sleep_time;
      deltatime-=deltatime%m_sleep_time;

      m_last_update_time.tv_usec+=deltatime*1000;
      if(m_last_update_time.tv_usec>=1000000)
	m_last_update_time.tv_sec += m_last_update_time.tv_usec/1000000,
	  m_last_update_time.tv_usec = m_last_update_time.tv_usec%1000000;

      int sleeptime = 
	(m_last_update_time.tv_sec-now.tv_sec)*1000+
	(m_last_update_time.tv_usec-now.tv_usec)/1000;

      if(sleeptime>0)ZThread::Thread::sleep(sleeptime);
    }
}

void TelescopeEmulator::
tick()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  if(m_status.interlock)
    {  
      m_status.az.driveMode = ScopeAPI::DM_STANDBY;
      m_status.el.driveMode = ScopeAPI::DM_STANDBY;
    }

  double az_x;
  double el_x;
  double az_v;
  double el_v;
  double az_i;
  double el_i;
  m_physics->getState(az_x, el_x, az_v, el_v, az_i, el_i);

#if 0
  Debug::stream()
    << az_x << '\t' << az_v << '\t' << az_i << '\t'
    << el_x << '\t' << el_v << '\t' << el_i << std::endl;
#endif

  double az_V = 0;
  double el_V = 0;

  if(m_status.az.driveMode != ScopeAPI::DM_STANDBY)
    {
      double az_setpoint =
	az_V = m_az_scheduler->getNextSetpoint();
      az_V = m_az_ang_pid.update(az_setpoint-az_x,az_setpoint);
    }
  else
    {
      m_az_ang_pid.reset();
    }

  if(m_status.el.driveMode != ScopeAPI::DM_STANDBY)
    {
      double el_setpoint =
	el_V = m_el_scheduler->getNextSetpoint();
      el_V = m_el_ang_pid.update(el_setpoint-el_x,el_setpoint);
    }
  else
    {
      m_el_ang_pid.reset();
    }

  m_physics->iterate(az_V, el_V, 
		     m_status.az.driveMode==ScopeAPI::DM_STANDBY,
		     m_status.el.driveMode==ScopeAPI::DM_STANDBY);
  m_physics->getState(az_x, el_x, az_v, el_v, az_i, el_i);

  // Limits
  if(m_limits)
    m_limits->isDriveAngleInsideLimits(Angle::frDeg(az_x), 
				       Angle::frDeg(el_x),
				       m_status.az.limitCcwDown,
				       m_status.az.limitCwUp,
				       m_status.el.limitCcwDown,
				       m_status.el.limitCwUp);

  // Cable wrap and interlock
  m_status.azTravelledCCW = az_x<0;
  m_status.azCableWrap = az_x/270.0;
  m_status.interlockAzPullCord = 
    (az_x>m_pullcord_lim)||(az_x<-m_pullcord_lim);
  m_status.interlock =
    m_status.interlockAzPullCord||
    m_status.interlockAzStowPin||m_status.interlockElStowPin||
    m_status.interlockAzDoorOpen||m_status.interlockElDoorOpen||
    m_status.interlockSafeSwitch;

  // Set the brakes if we are in standby OR the interlock is on
  if((m_status.interlock)||(m_status.az.driveMode==ScopeAPI::DM_STANDBY))
    m_status.az.brakeReleased=false;
  else m_status.az.brakeReleased=true;
  m_status.az.servoOn = m_status.az.brakeReleased;

  if((m_status.interlock)||(m_status.el.driveMode==ScopeAPI::DM_STANDBY))
    m_status.el.brakeReleased=false;
  else m_status.el.brakeReleased=true;
  m_status.el.servoOn = m_status.el.brakeReleased;

  // Position complete flag
  if((fabs(az_x-m_az_req_ang)<0.0001)&&(fabs(az_v)<0.00001))
    m_status.az.positionComplete=true;
  else m_status.az.positionComplete=false;
  
  if((fabs(el_x-m_el_req_ang)<0.0001)&&(fabs(el_v)<0.00001))
    m_status.el.positionComplete=true;
  else m_status.el.positionComplete=false;
  
  // Copy the position over
  m_status.az.driveangle_deg = az_x;
  m_status.el.driveangle_deg = el_x;

  // Analogs
  m_status.Analog1 = az_i;
  //(0.15*double(random())/double(RAND_MAX)+0.3)+2*m_el_vel;
  m_status.Analog2 = el_i;
  //-(0.15*double(random())/double(RAND_MAX)+0.3)+2*m_el_vel;
}

void TelescopeEmulator::
load()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  std::ostringstream filename;
  filename << ".emulator" << m_scope_num;
  std::ifstream stream(filename.str().c_str());

  if(stream)
    {
      ang_t az_pgain;
      ang_t az_igain;
      ang_t az_dgain;
      unsigned az_ilimit;
      ang_t az_vgain;
      ang_t el_pgain;
      ang_t el_igain;
      ang_t el_dgain;
      unsigned el_ilimit;
      ang_t el_vgain;

      ang_t az_req_max_acc;
      ang_t el_req_max_acc;

      std::string what_you_talking_about_willis;
      
      stream 
	>> what_you_talking_about_willis >> az_pgain
	>> what_you_talking_about_willis >> az_igain
	>> what_you_talking_about_willis >> az_dgain
	>> what_you_talking_about_willis >> az_ilimit
	>> what_you_talking_about_willis >> az_vgain
	>> what_you_talking_about_willis >> el_pgain
	>> what_you_talking_about_willis >> el_igain
	>> what_you_talking_about_willis >> el_dgain
	>> what_you_talking_about_willis >> el_ilimit
	>> what_you_talking_about_willis >> el_vgain
	>> what_you_talking_about_willis >> m_az_offset
	>> what_you_talking_about_willis >> m_el_offset
	>> what_you_talking_about_willis >> m_az_req_max_vel
	>> what_you_talking_about_willis >> m_el_req_max_vel
	>> what_you_talking_about_willis >> az_req_max_acc
	>> what_you_talking_about_willis >> el_req_max_acc
	>> what_you_talking_about_willis >> m_cw_soft_lim
	>> what_you_talking_about_willis >> m_cc_soft_lim
	>> what_you_talking_about_willis >> m_up_soft_lim
	>> what_you_talking_about_willis >> m_dn_soft_lim
	>> what_you_talking_about_willis >> m_key_cw_soft_lim
	>> what_you_talking_about_willis >> m_key_cc_soft_lim
	>> what_you_talking_about_willis >> m_key_dn_soft_lim
	>> what_you_talking_about_willis >> m_pullcord_lim;

      m_physics->readState(stream);
      
      m_az_ang_pid.setPGain(az_pgain);
      m_az_ang_pid.setIGain(az_igain);
      m_az_ang_pid.setDGain(az_dgain);
      m_az_ang_pid.setILimit(az_ilimit);
      m_az_ang_pid.setVffGain(az_vgain);
      m_el_ang_pid.setPGain(el_pgain);
      m_el_ang_pid.setIGain(el_igain);
      m_el_ang_pid.setDGain(el_dgain);
      m_el_ang_pid.setILimit(el_ilimit);
      m_el_ang_pid.setVffGain(el_vgain);

      m_az_scheduler->setMaxAccel(az_req_max_acc);
      m_el_scheduler->setMaxAccel(el_req_max_acc);

      if(m_limits)delete(m_limits);
      m_limits = new KeyLimits(0,0,
			       m_key_cw_soft_lim, m_key_cc_soft_lim,
			       m_key_dn_soft_lim,
			       m_dn_soft_lim, m_up_soft_lim,
			       m_cw_soft_lim, m_cc_soft_lim);
    }
}

void TelescopeEmulator::
save()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  std::ostringstream filename;
  filename << ".emulator" << m_scope_num;
  std::ofstream stream(filename.str().c_str());
  if(stream)
    {
      stream 
	<< "AZPGAIN      " << m_az_ang_pid.pGain() << std::endl
	<< "AZIGAIN      " << m_az_ang_pid.iGain() << std::endl
	<< "AZDGAIN      " << m_az_ang_pid.dGain() << std::endl
	<< "AZILIMIT     " << m_az_ang_pid.iLimit() << std::endl
	<< "AZKVFF       " << m_az_ang_pid.vffGain() << std::endl
	<< "ELPGAIN      " << m_el_ang_pid.pGain() << std::endl
	<< "ELIGAIN      " << m_el_ang_pid.iGain() << std::endl
	<< "ELDGAIN      " << m_el_ang_pid.dGain() << std::endl
	<< "ELILIMIT     " << m_el_ang_pid.iLimit() << std::endl
	<< "ELKVFF       " << m_el_ang_pid.vffGain() << std::endl
	<< "AZOFFSET     " << m_az_offset << std::endl
	<< "ELOFFSET     " << m_el_offset << std::endl
	<< "AZMAXVEL     " << m_az_req_max_vel << std::endl
	<< "ELMAXVEL     " << m_el_req_max_vel << std::endl
	<< "AZMAXACC     " << m_az_scheduler->getAMax() << std::endl
	<< "ELMAXACC     " << m_el_scheduler->getAMax() << std::endl
	<< "LIMCWSFT     " << m_cw_soft_lim << std::endl
	<< "LIMCCSFT     " << m_cc_soft_lim << std::endl
	<< "LIMUPSFT     " << m_up_soft_lim << std::endl
	<< "LIMDNSFT     " << m_dn_soft_lim << std::endl
	<< "LIMKEYCWSFT  " << m_key_cw_soft_lim << std::endl
	<< "LIMKEYCCSFT  " << m_key_cc_soft_lim << std::endl
	<< "LIMKEYDNSFT  " << m_key_dn_soft_lim << std::endl
	<< "PULLCORDLIM  " << m_pullcord_lim << std::endl;
      m_physics->writeState(stream);
    }
}

void TelescopeEmulator::menu()
{
  typedef TextMenu::Item I;

  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  TextMenu menu("Emulator Menu");
  
  menu.addItem(I('s',"Save Settings to File"));
  menu.addItem(I('l',"Load Settings from File"));
  menu.addItem(I('i',"Interlock Menu"));
  if(m_physics->hasPhysicsMenu())
    menu.addItem(I('p',"Telescope Physics Menu"));
  menu.addItem(I('X',"Exit"));
  int c=0;

  while(c!='X')
    {
      c=menu.exec();
      if(!std::cin)break;

      switch(c)
	{
	case 's':
	  save();
	  TextMenu::pressAnyKey();
	  break;
	case 'l':
	  load();
	  TextMenu::pressAnyKey();
	  break;
	case 'i':
	  menuInterlock();
	  break;
	case 'p':
	  m_physics->doPhysicsMenu(&m_mtx);
	  break;
	case 'X':
	  break;
	default:
	  c=0;
	  break;
	}
    }
}

void TelescopeEmulator::menuInterlock()
{
  typedef TextMenu::Item I;

  static const std::string T("Enabled");
  static const std::string F("Disabled");

#define TF(x) ((x)?T:F)
#define TOGGLE(x) ((x)=!(x))

  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  int c=0;
  while(c!='x')
    {
      TextMenu menu("Interlock Menu");
      
      menu.addItem(I('s',"Toggle Az Stow Pin Interlock      - ",
		     TF(m_status.interlockAzStowPin)));
      menu.addItem(I('d',"Toggle Az Door Open Interlock     - ",
		     TF(m_status.interlockAzDoorOpen)));
      menu.addItem(I('S',"Toggle El Stow Pin Interlock      - ",
		     TF(m_status.interlockElStowPin)));
      menu.addItem(I('D',"Toggle El Door Open Interlock     - ",
		     TF(m_status.interlockElDoorOpen)));
      menu.addItem(I('r',"Toggle Safe/Run Switch Interlock  - ",
		     TF(m_status.interlockSafeSwitch)));
      
      menu.addItem(I('h',"Toggle Hand Paddle Indicator      - ",
		     TF(!m_status.remoteControl)));
      menu.addItem(I('C',"Toggle Checksum Fail Indicator    - ",
		     TF(!m_status.checksumOK)));
      
      menu.addItem(I('b',"Toggle Bad Frame Indicator        - ",
		     TF(m_status.msgBadFrame)));
      menu.addItem(I('c',"Toggle Command Invalid Indicator  - ",
		     TF(m_status.msgCommandInvalid)));
      menu.addItem(I('i',"Toggle Input Overrun Indicator    - ",
		 TF(m_status.msgInputOverrun)));
      menu.addItem(I('o',"Toggle Output Overrun Indicator   - ",
		     TF(m_status.msgOutputOverrun)));

      menu.addItem(I('1',"Toggle Az Servo 1 Fail Indicator  - ",
		     TF(m_status.az.servo1Fail)));
      menu.addItem(I('2',"Toggle Az Servo 2 Fail Indicator  - ",
		     TF(m_status.az.servo2Fail)));
      menu.addItem(I('A',"Toggle El Servo 1 Fail Indicator  - ",
		     TF(m_status.el.servo1Fail)));
      menu.addItem(I('B',"Toggle El Servo 2 Fail Indicator  - ",
		     TF(m_status.el.servo2Fail)));
      
      menu.addItem(I('x',"Exit"));
      
      c=menu.exec();
      if(!std::cin)break;

      switch(c)
	{
	case 's':
	  m_mtx.acquire();
	  TOGGLE(m_status.interlockAzStowPin);
	  m_mtx.release();
	  break;
	case 'd':
	  m_mtx.acquire();
	  TOGGLE(m_status.interlockAzDoorOpen);
	  m_mtx.release();
	  break;
	case 'S':
	  m_mtx.acquire();
	  TOGGLE(m_status.interlockElStowPin);
	  m_mtx.release();
	  break;
	case 'D':
	  m_mtx.acquire();
	  TOGGLE(m_status.interlockElDoorOpen);
	  m_mtx.release();
	  break;
	case 'r':
	  m_mtx.acquire();
	  TOGGLE(m_status.interlockSafeSwitch);
	  m_mtx.release();
	  break;

	case 'h':
	  m_mtx.acquire();
	  TOGGLE(m_status.remoteControl);
	  m_mtx.release();
	  break;
	case 'C':
	  m_mtx.acquire();
	  TOGGLE(m_status.checksumOK);
	  m_mtx.release();
	  break;

	case 'b':
	  m_mtx.acquire();
	  TOGGLE(m_status.msgBadFrame);
	  m_mtx.release();
	  break;
	case 'c':
	  m_mtx.acquire();
	  TOGGLE(m_status.msgCommandInvalid);
	  m_mtx.release();
	  break;
	case 'i':
	  m_mtx.acquire();
	  TOGGLE(m_status.msgInputOverrun);
	  m_mtx.release();
	  break;
	case 'o':
	  m_mtx.acquire();
	  TOGGLE(m_status.msgOutputOverrun);
	  m_mtx.release();
	  break;
	
	case '1':
	  m_mtx.acquire();
	  TOGGLE(m_status.az.servo1Fail);
	  m_mtx.release();
	  break;
	case '2':
	  m_mtx.acquire();
	  TOGGLE(m_status.az.servo2Fail);
	  m_mtx.release();
	  break;
	case 'A':
	  m_mtx.acquire();
	  TOGGLE(m_status.el.servo1Fail);
	  m_mtx.release();
	  break;
	case 'B':
	  m_mtx.acquire();
	  TOGGLE(m_status.el.servo2Fail);
	  m_mtx.release();
	  break;
  
	case 'x':
	  break;
	default:
	  c=0;
	  break;
	}
    }
}

#ifdef TESTMAIN
int main(int argc, char**argv)
{
  argc--,argv++;
  TelescopeEmulator::ang_t Kp;
  TelescopeEmulator::ang_t Ki;
  TelescopeEmulator::ang_t Kd;
  TelescopeEmulator::ang_t Ilim;
  
  TelescopeEmulator::ang_t ang = 0;
  TelescopeEmulator::ang_t vel = 0;

  TelescopeEmulator::ang_t req_ang = 5;
  TelescopeEmulator::ang_t req_vel = 0.5;

  TelescopeEmulator::ang_t max_vel = 2.0;
  TelescopeEmulator::ang_t max_acc = 0.1;

  TelescopeEmulator::ang_t tar_vel = 0;

  double t = 0;
  double deltat = 0.05;

  if(argc) { std::istringstream(*argv++) >> Kp; argc--; }
  if(argc) { std::istringstream(*argv++) >> Ki; argc--; }
  if(argc) { std::istringstream(*argv++) >> Kd; argc--; }
  if(argc) { std::istringstream(*argv++) >> Ilim; argc--; }
  if(argc) { std::istringstream(*argv++) >> tar_vel; argc--; }
  if(argc) { std::istringstream(*argv++) >> req_ang; argc--; }

  PID<TelescopeEmulator::ang_t> pid;
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
}
#endif

//-*-mode:c++; mode:font-lock;-*-

/**
 * \file TelescopeController.cpp
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
 * $Date: 2008/01/31 21:24:13 $
 * $Revision: 2.6 $
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

#include"DataStream.h"
#include"TelescopeController.h"
#include"PositionLogger.h"

using namespace SEphem;
using namespace VTracking;
using namespace VMessaging;

// ----------------------------------------------------------------------------
// TelescopeController::StateElements
// ----------------------------------------------------------------------------

TelescopeController::StateElements::StateElements():
  status(), last_az_driveangle_deg(), last_el_driveangle_deg(),
  az_driveangle_estimated_speed_dps(), el_driveangle_estimated_speed_dps(),
  state(TS_STOP), req(REQ_STOP), cf(CF_SERVER), tv(), mjd(), last_mjd(),
  timeangle(), lmst(), last_has_object(false), has_object(false), 
  tel_azel(), obj_azel(), anticipation(), 
  last_cmd_az_driveangle_deg(), last_cmd_el_driveangle_deg(),
  cmd_az_driveangle_deg(), cmd_el_driveangle_deg(),
  az_slew_speed_dps(), el_slew_speed_dps()
{
  // nothing to see here
}

// ----------------------------------------------------------------------------
// TelescopeController
// ----------------------------------------------------------------------------

TelescopeController::
TelescopeController(TelescopeMotionLimits* limits,
		    const SEphem::SphericalCoords& earth_pos,
		    int sleep_time, int phase_time, 
		    const CorrectionParameters& cp,
		    ZThread::Condition* terminate_notifier):
  PhaseLockedLoop(sleep_time, phase_time, terminate_notifier), 
  m_limits(limits), m_earth_position(earth_pos), m_tse(),
  m_target_object(0), 
  m_direction_preference(SEphem::CorrectionParameters::DP_NONE), 
  m_corrections(cp)
{
  m_tse.state=TS_COM_FAILURE;
  m_tse.req=REQ_STOP;
  m_tse.cf=CF_SERVER;
}

TelescopeController::~TelescopeController()
{
  // nothing to see here
}

bool TelescopeController::isInsideLimits(const SphericalCoords& azel,
					 double tel_az_driveangle,
					 bool do_corrections)
{
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  if(!m_limits)return true;
  double az = azel.phiRad();
  double el = azel.latitudeRad();
  if(!m_corrections.doAzElCorrections(az,el, tel_az_driveangle, 
				      do_corrections,m_direction_preference))
    return false;
  return m_limits->isDriveAngleInsideLimits(az,el);
}

TelescopeController::TrackingState 
TelescopeController::state()
{ 
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  return m_tse.state; 
}

TelescopeController::TrackingRequest 
TelescopeController::request()
{ 
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  return m_tse.req; 
}

TargetObject* TelescopeController::getTargetObject()
{ 
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  TargetObject* to = 0;
  if(m_target_object)to=m_target_object->copy();
  return to;
}

TargetObject* TelescopeController::getTargetObject(DirectionPreference& dp)
{ 
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  TargetObject* to = 0;
  if(m_target_object)to=m_target_object->copy();
  dp = m_direction_preference;
  return to;
}

TelescopeController::DirectionPreference 
TelescopeController::getDirectionPreference()
{ 
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  return m_direction_preference;
}

CorrectionParameters
TelescopeController::getCorrections()
{ 
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  return m_corrections;
}

TelescopeController::StateElements
TelescopeController::tse()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  return m_tse; 
}

TelescopeController::StateElements
TelescopeController::getTelescopeState()
{ 
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  return m_tse; 
}

bool TelescopeController::hasBeenUpdated(double mjd)
{ 
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  return(m_tse.mjd!=mjd);
}

void TelescopeController::getTimes(struct timeval& tv, double& mjd, 
				   Angle& timeangle, Angle& lmst) const
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  gettimeofday(&tv,0);

  mjd = 40587.0+(double(tv.tv_sec)+double(tv.tv_usec)/1000000)/86400;

  timeangle = 
    Angle::makeRot((double(tv.tv_sec%86400)+double(tv.tv_usec)/1000000)/86400);
  
  double T = (floor(mjd) - 51544.5)/36525;
  double th = 100.46061837+36000.770053608*T+0.000387933*T*T-T*T*T/38710000;
  lmst = Angle::makeDeg(th)+timeangle.rad()*1.00273790935+
    m_earth_position.phi();
}

// ----------------------------------------------------------------------------
// EXTENDED FUNCTIONALITY
// ----------------------------------------------------------------------------

bool TelescopeController::hasTerminateRemoteCapability()
{
  return false;
}

void TelescopeController::terminateRemote() throw ( CapabilityNotSupported )
{
  throw CapabilityNotSupported();
}


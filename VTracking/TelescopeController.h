//-*-mode:c++; mode:font-lock;-*-

/**
 * \file TelescopeController.h
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
 * $Revision: 2.8 $
 * $Tag$
 *
 **/

#ifndef VTRACKING_TELESCOPECONTROLLER_H
#define VTRACKING_TELESCOPECONTROLLER_H

#include<iostream>
#include<string>

#include<time.h>
#include<sys/times.h>

#include<zthread/RecursiveMutex.h>
#include<zthread/Runnable.h>
#include<zthread/Condition.h>

#include<Angle.h>
#include<SphericalCoords.h>
#include<PhaseLockedLoop.h>
#include<CorrectionParameters.h>

#include"ScopeAPI.h"
#include"TargetObject.h"
#include"TelescopeMotionLimits.h"

namespace VTracking 
{

  // As of version 1.1 TelescopeController is base class for "local"
  // and "remote" controller classes which talk to the telescope
  // directly or using CORBA

  class TelescopeController: public VTaskNotification::PhaseLockedLoop
  {
  public:
    // ------------------------------------------------------------------------
    // VARIOUS SUB-CLASSES
    // ------------------------------------------------------------------------

    enum TrackingState { TS_STOP, TS_SLEW, TS_TRACK, TS_RESTRICTED_MOTION,
			 TS_RAMP_DOWN, TS_COM_FAILURE };
    enum ComFailure { CF_SERVER, CF_SCOPE };
    enum TrackingRequest { REQ_STOP, REQ_SLEW, REQ_TRACK };

    typedef SEphem::CorrectionParameters::DirectionPreference 
    DirectionPreference;

    class StateElements
    {
    public:
      StateElements();
      ScopeAPI::PositionerStatus status;
      double last_az_driveangle_deg;
      double last_el_driveangle_deg;
      double az_driveangle_estimated_speed_dps;
      double el_driveangle_estimated_speed_dps;
      TrackingState state;
      TrackingRequest req;
      ComFailure cf;
      struct timeval tv;
      double mjd;
      double last_mjd;
      SEphem::Angle timeangle;
      SEphem::Angle lmst;
      bool last_has_object;
      bool has_object;
      SEphem::SphericalCoords tel_azel;
      SEphem::SphericalCoords obj_azel;
      double anticipation;
      double last_cmd_az_driveangle_deg;
      double last_cmd_el_driveangle_deg;
      double cmd_az_driveangle_deg;
      double cmd_el_driveangle_deg;
      double az_slew_speed_dps;
      double el_slew_speed_dps;
    };

    class TCException: public VMessaging::Exception
    {
    public:
      TCException(const std::string& message): 
	Exception("Telescope controller exception",message) {}
    };

    // ------------------------------------------------------------------------
    // CONSTRUCTOR AND DESTRUCTOR
    // ------------------------------------------------------------------------

    TelescopeController(TelescopeMotionLimits* limits,
			const SEphem::SphericalCoords& earth_pos, 
			int sleep_time, int phase_time,
			const SEphem::CorrectionParameters& cp=
			SEphem::CorrectionParameters(),
			ZThread::Condition* terminate_notifier = 0);
    virtual ~TelescopeController();    
    virtual std::string controllerName() const = 0;

    // ------------------------------------------------------------------------
    // GET VARIOUS STATE ELEMENTS
    // ------------------------------------------------------------------------

    TrackingState state();
    TrackingRequest request();
    TargetObject* getTargetObject();
    TargetObject* getTargetObject(DirectionPreference& dp);
    DirectionPreference getDirectionPreference();
    SEphem::CorrectionParameters getCorrections();
    StateElements tse();
    StateElements getTelescopeState();
    bool hasBeenUpdated(double mjd);

    bool isInsideLimits(const SEphem::SphericalCoords& azel,
			double tel_az_drivangle, bool do_corrections);

    const TelescopeMotionLimits* getLimits() const { return m_limits; }
    const SEphem::SphericalCoords& getEarthPos() const 
    { return m_earth_position; }

    // ------------------------------------------------------------------------
    // UTILITY FUNCTIONS
    // ------------------------------------------------------------------------

    void getTimes(struct timeval& tv, double& mjd, 
		  SEphem::Angle& timeangle, SEphem::Angle& lmst) const;

    // ------------------------------------------------------------------------
    // SET STATE
    // ------------------------------------------------------------------------

    virtual void setTargetObject(TargetObject* obj, 
	DirectionPreference dp = SEphem::CorrectionParameters::DP_NONE) = 0;
    virtual void reqStop() = 0;
    virtual void reqSlew() = 0;
    virtual void reqTrack() = 0;
    virtual void emergencyStop() = 0;
    virtual void 
    setCorrectionParameters(const SEphem::CorrectionParameters& cp) = 0;

    // ------------------------------------------------------------------------
    // EXTENDED FUNCTIONALITY
    // ------------------------------------------------------------------------

    class CapabilityNotSupported {  };

    virtual bool hasTerminateRemoteCapability();
    virtual void terminateRemote() throw ( CapabilityNotSupported );

  protected:

    // ------------------------------------------------------------------------
    // PRIMARY CONTROLLER FUNCTION
    // ------------------------------------------------------------------------

    virtual void iterate() = 0;

    TelescopeMotionLimits*         m_limits;
    SEphem::SphericalCoords        m_earth_position;

    StateElements                  m_tse;
    TargetObject*                  m_target_object;
    DirectionPreference            m_direction_preference;
    SEphem::CorrectionParameters   m_corrections;

  private:
    TelescopeController(const TelescopeController&);
    const TelescopeController& operator=(const TelescopeController&);
  }; // class TelescopeController

}

#endif // VTRACKING_TELESCOPECONTROLLER_H

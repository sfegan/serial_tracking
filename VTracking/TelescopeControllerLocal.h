//-*-mode:c++; mode:font-lock;-*-

/**
 * \file TelescopeControllerLocal.h
 * \ingroup VTracking
 * \brief TelecopeController which does actual communication with telescope
 *
 * This derived class communiactes with the telescope, updating
 * and logging the position and accepting commands from the GUI, UI
 * and CORBA.
 *
 * Original Author: Stephen Fegan
 * $Author: sfegan $
 * $Date: 2008/01/31 21:24:13 $
 * $Revision: 2.6 $
 * $Tag$
 *
 **/

#ifndef VTRACKING_TELESCOPECONTROLLERLOCAL_H
#define VTRACKING_TELESCOPECONTROLLERLOCAL_H

#include<iostream>
#include<string>

#include<time.h>
#include<sys/times.h>

#include<zthread/RecursiveMutex.h>
#include<zthread/Runnable.h>

#include<Angle.h>
#include<SphericalCoords.h>
#include<CorrectionParameters.h>

#include"ScopeAPI.h"
#include"TargetObject.h"
#include"TelescopeMotionLimits.h"
#include"TelescopeController.h"

namespace VTracking 
{

  // Avoid circular includes by making forward declaration and NOT
  // including PositionLogger.h
  class PositionLogger;

  // As of version 1.1 TelescopeController is base class for "local"
  // and "remote" controller classes which talk to the telescope
  // directly or using CORBA

  class TelescopeControllerLocal: public TelescopeController
  {
  public:
    // ------------------------------------------------------------------------
    // CONSTRUCTOR AND DESTRUCTOR
    // ------------------------------------------------------------------------

    TelescopeControllerLocal(ScopeAPI* scope_api, 
			     TelescopeMotionLimits* limits,
			     const SEphem::SphericalCoords& pos, 
			     int sleep_time, int phase_time,
			     const SEphem::CorrectionParameters& cp 
			       = SEphem::CorrectionParameters(),
			     PositionLogger* logger = 0, 
			     time_t logger_timeout = 0,
			     ZThread::Condition* terminate_notifier = 0,
			     bool realtime = false);
    virtual ~TelescopeControllerLocal();    
    virtual std::string controllerName() const;
    
    // ------------------------------------------------------------------------
    // SET STATE (FROM TELESCOPE CONTROLLER)
    // ------------------------------------------------------------------------

    virtual void setTargetObject(TargetObject* obj, 
	DirectionPreference dp = SEphem::CorrectionParameters::DP_NONE);
    virtual void reqStop();
    virtual void reqSlew();
    virtual void reqTrack();
    virtual void emergencyStop();
    virtual void 
    setCorrectionParameters(const SEphem::CorrectionParameters& cp);

  protected:

    // ------------------------------------------------------------------------
    // PRIMARY CONTROLLER FUNCTION (FROM PHASE LOCKED LOOP)
    // ------------------------------------------------------------------------

    virtual void loopIsStarting();
    virtual void loopIsTerminating();
    virtual void iterate();

    void addAnticipation(double& mjd, SEphem::Angle& lmst,
			 bool started_with_comfailure);
    
    ScopeAPI*                 m_scope_api;

    double                    m_anticipation;
    unsigned                  m_close_count;
    double                    m_ramp_down_mjd;
    SEphem::Angle             m_ramp_down_az;
    SEphem::Angle             m_ramp_down_el;

    time_t                    m_logger_last_loggable_event;
    time_t                    m_logger_timeout_sec;
    bool                      m_logger_active;
    PositionLogger*           m_logger;

    bool                      m_emergency_stop;
    bool                      m_void_vff_positions;

    bool                      m_realtime;

    static const double       sc_decay_rate = 0.95;
  }; // class TelescopeController
}

#endif // VTRACKING_TELESCOPECONTROLLERLOCAL_H

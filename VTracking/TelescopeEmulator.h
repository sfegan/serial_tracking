//-*-mode:c++; mode:font-lock;-*-

/**
 * \file TelescopeEmulator.h
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
 * $Date: 2006/04/04 17:06:58 $
 * $Revision: 2.0 $
 * $Tag$
 *
 **/

#ifndef VTRACKING_TELESCOPEEMULATOR_H
#define VTRACKING_TELESCOPEEMULATOR_H

#include<sys/time.h>

#include<zthread/RecursiveMutex.h>
#include<zthread/Runnable.h>

#include "PID.h"
#include "ScopeAPI.h"
#include "TelescopeMotionLimits.h"
#include "SetpointScheduler.h"
#include "TelescopePhysicsModel.h"

namespace VTracking
{
  class TelescopeEmulator: public ScopeAPI, public ZThread::Runnable
  {
  public:
    typedef double ang_t;

    TelescopeEmulator(unsigned scope, 
		      unsigned sleep_time_ms=10);
    virtual ~TelescopeEmulator();
    
    // ------------------------------------------------------------------------
    // ScopeAPI members
    // ------------------------------------------------------------------------
    virtual void reqStat(PositionerStatus& state);
    virtual void cmdStandby();
    virtual void cmdPoint(const SEphem::Angle& az_angle, double az_vel,
			  const SEphem::Angle& el_angle, double el_vel);
    virtual void cmdSlew(double az_vel, double el_vel);

    virtual void reqAzOffset(SEphem::Angle& az_angle);
    virtual void reqElOffset(SEphem::Angle& el_angle);
    virtual void setAzOffset(const SEphem::Angle& az_angle);
    virtual void setElOffset(const SEphem::Angle& el_angle);

    virtual void reqAzPIDParameters(PIDParameters& az_param);
    virtual void reqElPIDParameters(PIDParameters& el_param);
    virtual void setAzPIDParameters(const PIDParameters& az_param);
    virtual void setElPIDParameters(const PIDParameters& el_param);

    virtual void resetCommunication();

    virtual std::string apiName() const;

    // ------------------------------------------------------------------------
    // Thread related members
    // ------------------------------------------------------------------------
    void terminate();
    virtual void run();

    // ------------------------------------------------------------------------
    // Other members
    // ------------------------------------------------------------------------

    void menu();

  protected:
    void tick();

  private:
    void load();
    void save();

    void menuInterlock();

    // Thread related
    int m_sleep_time;
    ZThread::RecursiveMutex m_mtx;
    bool m_exit_notification;
    
    unsigned m_scope_num;
    struct timeval m_last_update_time;

    // ------
    // Motion
    // ------
    SetpointScheduler* m_az_scheduler;
    SetpointScheduler* m_el_scheduler;
    PIDVff<ang_t> m_az_ang_pid;
    PIDVff<ang_t> m_el_ang_pid;
    TelescopePhysicsModel* m_physics;

    TelescopeMotionLimits* m_limits;

    PositionerStatus m_status;

    // ---------------
    // Requested state
    // ---------------
    ang_t m_az_req_ang;
    ang_t m_el_req_ang;
    ang_t m_az_req_vel;
    ang_t m_el_req_vel;

    ang_t m_az_req_max_vel;
    ang_t m_el_req_max_vel;

    // ----
    // Misc
    // ----
    ang_t m_az_offset;
    ang_t m_el_offset;

    // Limits

    ang_t m_cw_soft_lim;
    ang_t m_cc_soft_lim;
    ang_t m_up_soft_lim;
    ang_t m_dn_soft_lim;
    
    ang_t m_key_cw_soft_lim;
    ang_t m_key_cc_soft_lim;
    ang_t m_key_dn_soft_lim;

    ang_t m_pullcord_lim;
  };
};


#endif // VTRACKING_TELESCOPEEMULATOR_H


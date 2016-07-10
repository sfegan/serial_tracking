//-*-mode:c++; mode:font-lock;-*-

/**
 * \file SetpointScheduler.h
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

#ifndef VTRACKING_SETPOINTSCHEDULER_H
#define VTRACKING_SETPOINTSCHEDULER_H

namespace VTracking
{
  // ==========================================================================
  // Base class for setpoint schedulers
  // ==========================================================================

  class SetpointScheduler
  {
  public:
    SetpointScheduler(const double& dt,
		      const double& v_max, const double& a_max,
		      const double& x_set = 0,
		      const double& x_now = 0, const double& v_now = 0):
      m_dt(fabs(dt)), m_v_max(fabs(v_max)), m_a_max(fabs(a_max)),
      m_x_set(x_set), m_x_now(x_now), m_v_now(v_now) 
    { /* nothing to see here */ }

    virtual ~SetpointScheduler();

    // Getters
    const double& getVMax() const { return m_v_max; }
    const double& getAMax() const { return m_a_max; }
    const double& getXTarget() const { return m_x_set; }
    const double& getXCurrent() const { return m_x_now; }
    const double& getVCurrent() const { return m_v_now; }

    // Setters
    void syncScheduler(const double& x_now, const double& v_now)
    { m_x_now = x_now; m_v_now = v_now; }
    void setMaxSpeed(const double& v_max) { m_v_max = fabs(v_max); }
    void setMaxAccel(const double& a_max) { m_a_max = fabs(a_max); }
    void setXTarget(const double& x_set) { m_x_set = x_set; }
    
    // Calculate the next setpoint
    virtual double getNextSetpoint() = 0;
      
  protected:
    double m_dt;
    double m_v_max;
    double m_a_max;
    double m_x_set;
    double m_x_now;
    double m_v_now;
  };
  
  // ==========================================================================
  // Setpoint scheduler which implements trapezoidal velocity profile
  // ==========================================================================

  class TrapezoidalSetpointScheduler: public SetpointScheduler
  {
  public:
    TrapezoidalSetpointScheduler(const double& dt,
				 const double& v_max, const double& a_max,
				 const double& x_set = 0,
				 const double& x_now = 0, 
				 const double& v_now = 0):
      SetpointScheduler(dt, v_max, a_max, x_set, x_now, v_now)
    { /* nothing to see here */ }

    virtual ~TrapezoidalSetpointScheduler();

    // Calculate the next setpoint
    virtual double getNextSetpoint();
  };
  

}; // namespace VTracking

#endif // VTRACKING_SETPOINTSCHEDULER_H

//-*-mode:c++; mode:font-lock;-*-

/**
 * \file MotionController.h
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

#ifndef VTRACKING_MOTIONCONTROLLER_H
#define VTRACKING_MOTIONCONTROLLER_H

#include <cmath>

#include"PID.h"
#include"TelescopeMotionLimits.h"

namespace VTracking
{
  template<class T> class MotionController
  {
  public:
    MotionController(const T& max_vel, const T& max_acc,
		     const T& p, const T& i, const T& d, unsigned l): 
      m_pid(p,i,d,l), m_max_vel(max_vel), m_max_acc(max_acc) {}
    virtual ~MotionController();
    
    T posMode(const T& req_ang, const T& req_vel, const T& req_max_vel,
	      const double& delta_t, const T& vel, const T& ang,
	      bool lim_neg, bool lim_pos);

    T velMode(const T& req_vel, const double& delta_t, const T& vel,
	      bool lim_neg, bool lim_pos);

    void reset() { m_pid.reset(); }

    T pGain() const { return m_pid.pGain(); }
    T iGain() const { return m_pid.iGain(); }
    T dGain() const { return m_pid.dGain(); }
    T iLimit() const { return m_pid.iLimit(); }
    T maxVel() const { return m_max_vel; }
    T maxAcc() const { return m_max_acc; }

    void setPGain(const T& p) { m_pid.setPGain(p); }
    void setIGain(const T& i) { m_pid.setIGain(i); }
    void setDGain(const T& d) { m_pid.setDGain(d); }
    void setILimit(unsigned l) { m_pid.setILimit(l); }
    void setMaxVel(const T& v) { m_max_vel=v; }
    void setMaxAcc(const T& a) { m_max_acc=a; }

  private:
    PID<T> m_pid;
    T m_max_vel;
    T m_max_acc;
  };

  class DualAxisController
  {
  public:
    DualAxisController(TelescopeMotionLimits& limits, 
		       double az_max_vel, double az_max_acc,
		       double az_p, double az_i, double az_d, unsigned az_l, 
		       double el_max_vel, double el_max_acc,
		       double el_p, double el_i, double el_d, unsigned el_l);
    virtual ~DualAxisController();

    void posMode(double& az_vel, double& el_vel,
		 double az_ang, double el_ang,
		 double az_req_ang, double az_req_vel, double az_req_max_vel,
		 double el_req_ang, double el_req_vel, double el_req_max_vel,
		 double delta_t);
    
    void velMode(double& az_vel, double& el_vel, double az_ang, double el_ang,
		 double az_req_vel, double el_req_vel, double delta_t);

    double azPGain() const { return m_az_axis.pGain(); }
    double azIGain() const { return m_az_axis.iGain(); }
    double azDGain() const { return m_az_axis.dGain(); }
    double azILimit() const { return m_az_axis.iLimit(); }
    double azMaxVel() const { return m_az_axis.maxVel(); }
    double azMaxAcc() const { return m_az_axis.maxAcc(); }

    double elPGain() const { return m_el_axis.pGain(); }
    double elIGain() const { return m_el_axis.iGain(); }
    double elDGain() const { return m_el_axis.dGain(); }
    double elILimit() const { return m_el_axis.iLimit(); }
    double elMaxVel() const { return m_el_axis.maxVel(); }
    double axMaxAcc() const { return m_el_axis.maxAcc(); }

    void setAzPGain(const double& p) { m_az_axis.setPGain(p); }
    void setAzIGain(const double& i) { m_az_axis.setIGain(i); }
    void setAzDGain(const double& d) { m_az_axis.setDGain(d); }
    void setAzILimit(unsigned l) { m_az_axis.setILimit(l); }
    void setAzMaxVel(const double& v) { m_az_axis.setMaxVel(v); }
    void setAzMaxAcc(const double& a) { m_az_axis.setMaxAcc(a); }

    void setElPGain(const double& p) { m_el_axis.setPGain(p); }
    void setElIGain(const double& i) { m_el_axis.setIGain(i); }
    void setElDGain(const double& d) { m_el_axis.setDGain(d); }
    void setElILimit(unsigned l) { m_el_axis.setILimit(l); }
    void setElMaxVel(const double& v) { m_el_axis.setMaxVel(v); }
    void setElMaxAcc(const double& a) { m_el_axis.setMaxAcc(a); }

    void reset() { m_az_axis.reset(); m_el_axis.reset(); }

  private:
    TelescopeMotionLimits& m_limits;
    MotionController<double> m_az_axis;
    MotionController<double> m_el_axis;
  };
}

template<class T> 
VTracking::MotionController<T>::
~MotionController()
{
  // nothing to see here
}

template<class T> 
T VTracking::MotionController<T>::
posMode(const T& req_ang, const T& req_vel, const T& req_max_vel,
	const double& delta_t, const T& vel, const T& ang,
	bool lim_neg, bool lim_pos)
{
  // Use PID to get new velocity
  T new_vel = req_vel + m_pid.update(req_ang-ang);

  // Check that it does not exceed user requested max velocity
  if(new_vel>fabs(req_max_vel))new_vel=fabs(req_max_vel);
  else if(new_vel<-fabs(req_max_vel))new_vel=-fabs(req_max_vel);
  
  return velMode(new_vel, delta_t, vel, lim_neg, lim_pos);
}

template<class T>
T VTracking::MotionController<T>::
velMode(const T& req_vel, const double& delta_t, const T& vel,
	bool lim_neg, bool lim_pos)
{
  T new_vel = req_vel;

  // Position limits
  if((lim_pos)&&(new_vel>0))new_vel=0;
  if((lim_neg)&&(new_vel<0))new_vel=0;

  // Velocity limit
  if(new_vel>fabs(m_max_vel))new_vel=fabs(m_max_vel);
  else if(new_vel<-fabs(m_max_vel))new_vel=-fabs(m_max_vel);

  // Acceleration limit
  double acc = (new_vel-vel)/delta_t;
  if(acc>fabs(m_max_acc))new_vel=vel+fabs(m_max_acc)*delta_t;
  else if(acc<-fabs(m_max_acc))new_vel=vel-fabs(m_max_acc)*delta_t;

  return new_vel;
}

#endif // defined VTRACKING_MOTIONCONTROLLER_H

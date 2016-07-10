//-*-mode:c++; mode:font-lock;-*-

/**
 * \file SetpointScheduler.cpp
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

#include<cmath>
#include<cassert>

#ifdef TEST_MAIN
#include<iostream>
#endif

#include<Debug.h>
#include"SetpointScheduler.h"

using namespace VMessaging;
using namespace VTracking;

// ============================================================================
// Base class for setpoint schedulers
// ============================================================================

SetpointScheduler::~SetpointScheduler()
{
  // nothing to see here
}

// ============================================================================
// Setpoint scheduler which implements trapezoidal velocity profile
// ============================================================================

TrapezoidalSetpointScheduler::~TrapezoidalSetpointScheduler()
{
  // nothing to see here
}

double TrapezoidalSetpointScheduler::getNextSetpoint()
{
  double t = 0;
  
  // If we are going faster than the maximum speed then slow down
  if(fabs(m_v_now)>m_v_max)
    { 
      const double a = m_v_now>m_v_max?-m_a_max:m_a_max;
      const double v = m_v_now>m_v_max?m_v_max:-m_v_max;
      t=(v-m_v_now)/a;
      if(t>m_dt)t=m_dt;
      m_x_now += (m_v_now+0.5*a*t)*t;
      m_v_now += a*t;
      if(t==m_dt)return m_x_now;
    }

  const double error = m_x_set-m_x_now;
  
  const double s_stop = 
    m_v_now>0?0.5*m_v_now*m_v_now/m_a_max:-0.5*m_v_now*m_v_now/m_a_max;

  // Acceleration should be positive if error>0 AND 

  bool apos = 
    ((error>0)&&((m_v_now<0)||(s_stop<error)))
    ||((error<0)&&(m_v_now<0)&&(s_stop<error));
  
  const double a = apos?m_a_max:-m_a_max;
  const double vmax = apos?m_v_max:-m_v_max;

  double t_accel = (vmax-m_v_now)/a;
  double s_accel = (m_v_now+0.5*a*t_accel)*t_accel;

  double t_decel = vmax/a;
  double s_decel = (vmax-0.5*a*t_decel)*t_decel;

  double t_free = (error-s_accel-s_decel)/vmax;

#ifdef TEST_MAIN
  Debug::stream() << "A: "
		  << m_x_now << ' ' << m_v_now << ' ' 
		  << apos << ' ' << a << ' ' << vmax << ' ' << error << ' ' 
		  << s_stop << ' '
		  << t_accel << ' ' << s_accel << ' '
		  << t_decel << ' ' << s_decel << ' ' << t_free
		  << std::endl;
#endif
  
  if(t_free < 0)
    {
      const double v_peak_2 = 0.5*m_v_now*m_v_now + a*error;
#ifdef TEST_MAIN
      Debug::stream() << "C: " << v_peak_2 << std::endl;
#endif
      if(v_peak_2>=0)
	{
#ifdef TEST_MAIN
	  Debug::stream() << "D:" << std::endl;
#endif
	  const double v_peak = apos?sqrt(v_peak_2):-sqrt(v_peak_2);
	  t_accel = (v_peak-m_v_now)/a;
	  t_decel = v_peak/a;
	}
      else if(m_v_now/a > 0)
	{
#ifdef TEST_MAIN
	  Debug::stream() << "E:" << std::endl;
#endif
	  t_accel = 0;
	  t_decel = m_v_now/a;
	}
      else
	{
#ifdef TEST_MAIN
	  Debug::stream() << "F:" << std::endl;
#endif
	  t_accel = -m_v_now/a;
	  t_decel = 0;
	}

      t_free = 0;
    }

#ifdef TEST_MAIN
  Debug::stream() << "B: "
		  << m_x_now << ' ' << m_v_now << ' ' 
		  << apos << ' ' << a << ' ' << vmax << ' ' << error << ' ' 
		  << s_stop << ' '
		  << t_accel << ' ' << ' '
		  << t_decel << ' ' << ' ' << t_free
		  << std::endl;
#endif

  double dt;
      
  dt = t_accel;
  if(dt > m_dt-t)dt=m_dt-t;
  m_x_now += (m_v_now+0.5*a*dt)*dt;
  m_v_now += a*dt;
  t += dt;

  dt = t_free;
  if(dt > m_dt-t)dt=m_dt-t;
  m_x_now += m_v_now*dt;
  t += dt;
  
  dt = t_decel;
  if(dt > m_dt-t)dt=m_dt-t;
  m_x_now += (m_v_now-0.5*a*dt)*dt;
  m_v_now += -a*dt;
  t += dt;

  return m_x_now;
}

#ifdef TEST_MAIN

#include<sstream>
#include<string>

int main(int argc, char**argv)
{
  std::string program(*argv);
  argv++,argc--;

  double dt = 1;
  double v_max = 100;
  double a_max = 10;
  double x_set = 0;
  double x_now = 0;
  double v_now = 0;
  
  if(argc)
    {
      std::istringstream s(*argv);
      s >> x_set;
      argv++;argc--;
    }

  if(argc)
    {
      std::istringstream s(*argv);
      s >> v_now;
      argv++;argc--;
    }

  TrapezoidalSetpointScheduler scheduler(dt,v_max,a_max,x_set,x_now,v_now);

  Debug::stream() << x_now << std::endl;
  int count = 0;
  while(count < 5)
    {
      double x = scheduler.getNextSetpoint();
      Debug::stream() << x << std::endl;;
      if(x == x_set)count++;
    }

  return EXIT_SUCCESS;
}

#endif

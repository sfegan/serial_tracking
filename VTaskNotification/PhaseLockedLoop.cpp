//-*-mode:c++; mode:font-lock;-*-

/**
 * \file PhaseLockedLoop.cpp
 * \ingroup VTracking
 * \brief Loop that runs at fixed frequency
 *
 * This class implements a loop that attempts to run at a fixed frequcncy
 * (by timing sleeps appropriately). Derived classes should imlement the
 * iterate() function to get some work done in the loop. Class can be paused
 * restarted or terminated as requested. The "loopIs" functions are called
 * whenever the state is changed to provide notification to derived classes.
 *
 * Original Author: Stephen Fegan
 * $Author: aune $
 * $Date: 2010/09/09 18:56:11 $
 * $Revision: 2.7 $
 * $Tag$
 *
 **/

#include<iostream>
#include<cassert>

#include<sys/time.h>

#include<zthread/Guard.h>
#include<zthread/Thread.h>
#include<stdint.h>

#include<Debug.h>
#include<Exception.h>

#include"PhaseLockedLoop.h"

using namespace ZThread;
using namespace VMessaging;
using namespace VTaskNotification;

PhaseLockedLoop::PhaseLockedLoop(int sleep_time, int phase_time,
				 Condition* terminate_notifier):
  m_mtx(), 
  m_req_state(PS_RUNNING),
  m_act_state(PS_NOT_STARTED),
  m_sleep_time(int64_t(sleep_time)),
  m_phase_time(int64_t(phase_time)),
  m_pause_notifier(m_mtx),
  m_terminate_notifier(terminate_notifier)
{
  // nothing to see here
}

PhaseLockedLoop::~PhaseLockedLoop()
{
  assert((m_act_state == PS_TERMINATED)||(m_act_state == PS_NOT_STARTED));
}

PhaseLockedLoop::PLLState PhaseLockedLoop::getState()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  Guard<RecursiveMutex> guard(m_mtx);
  return m_act_state;
}

void PhaseLockedLoop::terminate()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  Guard<RecursiveMutex> guard(m_mtx);
  m_req_state = PS_TERMINATING;
  if(m_act_state==PS_PAUSED)m_pause_notifier.broadcast();
}

void PhaseLockedLoop::setStatePaused()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  Guard<RecursiveMutex> guard(m_mtx);
  if(m_req_state == PS_TERMINATING)return;
  m_req_state = PS_PAUSED;
}

void PhaseLockedLoop::setStateRunning()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  Guard<RecursiveMutex> guard(m_mtx);
  if(m_req_state == PS_TERMINATING)return;
  m_req_state = PS_RUNNING;
  if(m_act_state==PS_PAUSED)m_pause_notifier.broadcast();
}

void PhaseLockedLoop::run()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  int64_t ms_start = 0;
  if(m_sleep_time!=0)
    {
      struct timeval tv_now;
      gettimeofday(&tv_now,0);

      int64_t ms_now = 
	int64_t(tv_now.tv_sec)*INT64_C(1000) 
	+ int64_t(tv_now.tv_usec)/INT64_C(1000);
      
      ms_start = ms_now;
      ms_start /= m_sleep_time;
      ms_start *= m_sleep_time;
      ms_start += m_phase_time%m_sleep_time;
      if(ms_start > ms_now)ms_now -= m_sleep_time;
    }

#if 0
  std::cerr << last_update_time.tv_sec << ' ' 
	    << last_update_time.tv_usec << '\n';
#endif

  try
    {
      loopIsStarting();
    }
  catch(...)
    {
      Debug::stream()
	<< __PRETTY_FUNCTION__ << ": uncaught exception" 
	<< std::endl;
    }

  int64_t ms_last(ms_start);

  Guard<RecursiveMutex> guard(m_mtx);
  assert(m_act_state == PS_NOT_STARTED);
  while(1)
    {
      while(m_act_state != m_req_state)
	{
	  m_act_state = m_req_state;
	  try
	    {
	      Guard<RecursiveMutex,UnlockedScope> ulguard(m_mtx);
	      switch(m_req_state)
		{
		case PS_NOT_STARTED:
		case PS_TERMINATED:
		  assert(0);
		case PS_RUNNING:
		  loopIsRunning();
		  break;
		case PS_PAUSED:
		  loopIsPausing();
		  break;
		case PS_TERMINATING:
		  loopIsTerminating();
		  break;
		}
	    }
	  catch(...)
	    {
	      Debug::stream()
		<< __PRETTY_FUNCTION__ << ": uncaught exception" 
		<< std::endl;
	      return;
	    }
	}

      if(m_act_state == PS_TERMINATING)
	{
	  try
	    {
	      Guard<RecursiveMutex,UnlockedScope> ulguard(m_mtx);
	      if(allowedToTerminate())
		{
		  loopIsTerminated();
		  m_act_state = PS_TERMINATED;
		  return;
		}
	    }
	  catch(...)
	    {
	      Debug::stream()
		<< __PRETTY_FUNCTION__ << ": uncaught exception" 
		<< std::endl;
	      return;
	    }
	}
      

      if(m_act_state == PS_PAUSED)
	{
	  m_pause_notifier.wait();
	  continue;
	}
      else if((m_act_state != PS_RUNNING)&&(m_act_state != PS_TERMINATING))
	{
	  assert(0);
	}

      // **********************************************************************
      // WARNING: The mutex lock is released for the remainder of the loop
      // **********************************************************************

      Guard<RecursiveMutex,UnlockedScope> ulguard(m_mtx);
      
      if(m_sleep_time)
	{
	  // Try to run at a fixed frequency given by 1/m_sleep_time by
	  // calculating the time required to sleep until the next
	  // iteration. If for some reason we have missed the next
	  // iterations then just skip them.

	  struct timeval tv_now;
	  gettimeofday(&tv_now,0);

	  int64_t ms_now = 
	    int64_t(tv_now.tv_sec)*INT64_C(1000) 
	    + int64_t(tv_now.tv_usec)/INT64_C(1000);
	  
	  int64_t ms_delta = ms_now - ms_start;
	  int64_t ms_sleep = ms_delta%m_sleep_time;
	  if(ms_sleep<0)ms_sleep = -ms_sleep;
	  else if(ms_sleep>0)ms_sleep = m_sleep_time - ms_sleep;

	  // Check that the time we are trying to delay to is not the
	  // same as the last time. If it is then just wait until the
	  // time of the iteration after the last. This is done so
	  // that if the user function takes a very short time we do
	  // not end up trying to repeat an iteration.. i.e. the time
	  // may not have advanced by 1 ms. DO NOT check for <= here
	  // since that will lock the system when the clock changes
	  // backwards.

	  if(ms_now + ms_sleep == ms_last)
	    ms_sleep = ms_last + m_sleep_time - ms_now;

	  ms_last = ms_now + ms_sleep;

	  int sleeptime(ms_sleep);

#if 0
	  std::cout << ms_now << ' ' << ms_last << ' ' 
		    << ms_last-ms_now << ' ' 
		    << sleeptime << ' ';

	  if(sleeptime>0)Thread::sleep(sleeptime);
#endif

	  if(sleeptime>0)
	    {
	      timespec req = { sleeptime/1000, (sleeptime%1000)*1000000 };
	      timespec rem = { 0, 0 };

	      nanosleep(&req,&rem);
	      while((rem.tv_sec>0)||(rem.tv_nsec>0))
		{
		  req.tv_sec  = rem.tv_sec;
		  req.tv_nsec = rem.tv_nsec;
		  rem.tv_sec  = 0;
		  rem.tv_nsec = 0;
		  nanosleep(&req,&rem);
		}
	    }
	}
      try
	{
	  iterate();
	}
      catch(...)
	{
	  Debug::stream()
	    << __PRETTY_FUNCTION__ << ": uncaught exception" 
	    << std::endl;
	}

      // **********************************************************************
      // END WARNING: The mutex lock is released for the remainder of the loop
      // **********************************************************************
    }
}

void PhaseLockedLoop::loopIsStarting()
{
  // nothing to see here
}

void PhaseLockedLoop::loopIsTerminating()
{
  // nothing to see here
}

void PhaseLockedLoop::loopIsTerminated()
{
  // nothing to see here
}

void PhaseLockedLoop::loopIsPausing()
{
  // nothing to see here
}

void PhaseLockedLoop::loopIsRunning()
{
  // nothing to see here
}

bool PhaseLockedLoop::allowedToTerminate()
{
  return true;
}

void PhaseLockedLoop::sendTerminateNotification()
{
  assert(m_terminate_notifier);
  m_terminate_notifier->broadcast();
}

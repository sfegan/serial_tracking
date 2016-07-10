//-*-mode:c++; mode:font-lock;-*-

/**
 * \file PhaseLockedLoop.h
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
 * $Revision: 2.4 $
 * $Tag$
 *
 **/

#ifndef VTASKNOTIFICATION_PHASELOCKEDLOOP_H
#define VTASKNOTIFICATION_PHASELOCKEDLOOP_H

#include<time.h>
#include<sys/times.h>
#include<cstdlib>

#include<zthread/RecursiveMutex.h>
#include<zthread/Runnable.h>
#include<zthread/Condition.h>

namespace VTaskNotification
{

  class PhaseLockedLoop: public ZThread::Runnable
  {
  public:
    enum PLLState 
      { PS_NOT_STARTED, PS_RUNNING, PS_PAUSED, PS_TERMINATING, PS_TERMINATED };

    // ------------------------------------------------------------------------
    // CONSTRUCTOR AND DESTRUCTOR
    // ------------------------------------------------------------------------

    PhaseLockedLoop(int sleep_time, int phase_time = 0,
		    ZThread::Condition* terminate_notifier = 0);
    virtual ~PhaseLockedLoop();    
    
    // ------------------------------------------------------------------------
    // GET STATE
    // ------------------------------------------------------------------------

    PLLState getState();
    int iterationPeriod() const { return m_sleep_time; }

    // ------------------------------------------------------------------------
    // SET STATE
    // ------------------------------------------------------------------------

    void terminate();
    void setStatePaused();
    void setStateRunning();

    // ------------------------------------------------------------------------
    // LOOP
    // ------------------------------------------------------------------------

    virtual void run();

  protected:
    // ------------------------------------------------------------------------
    // PRIMARY FUNCTION
    // ------------------------------------------------------------------------

    virtual void iterate() = 0;

    // ------------------------------------------------------------------------
    // ADDITIONAL OPTIONAL FUNCTIONS
    // ------------------------------------------------------------------------

    virtual void loopIsStarting();
    virtual void loopIsTerminating();
    virtual void loopIsTerminated();
    virtual void loopIsPausing();
    virtual void loopIsRunning();
    virtual bool allowedToTerminate();

    // ------------------------------------------------------------------------
    // UTILITY FUNCTIONS FOR DERIVED CLASSES
    // ------------------------------------------------------------------------

    void sendTerminateNotification();

    ZThread::RecursiveMutex   m_mtx;

  private:

    PLLState                  m_req_state;
    PLLState                  m_act_state;

    int64_t                   m_sleep_time;
    int64_t                   m_phase_time;
    ZThread::Condition        m_pause_notifier;
    ZThread::Condition*       m_terminate_notifier;

    PhaseLockedLoop(const PhaseLockedLoop&);
    const PhaseLockedLoop& operator=(const PhaseLockedLoop&);
  }; // class PhaseLockedLoop
}

#endif // VTASKNOTIFICATION_PHASELOCKEDLOOP_H

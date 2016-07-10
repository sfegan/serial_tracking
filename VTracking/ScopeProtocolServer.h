//-*-mode:c++; mode:font-lock;-*-

/**
 * \file ScopeProtocolServer.h
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

#ifndef VTRACKING_SCOPEPROTOCOLSERVER_H
#define VTRACKING_SCOPEPROTOCOLSERVER_H

#include<zthread/RecursiveMutex.h>
#include<zthread/Runnable.h>

#include "TelescopeEmulator.h"

namespace VTracking
{
  class ScopeProtocolServer
  {
  public:
    virtual ~ScopeProtocolServer();
    virtual void processOneCommand(long cmd_to_sec=0, long cmd_to_usec=0) = 0;
  };

  class ScopeProtocolServerLoop: public ZThread::Runnable
  {
  public:
    ScopeProtocolServerLoop(ScopeProtocolServer* ps);
    virtual ~ScopeProtocolServerLoop();
    
    // ------------------------------------------------------------------------
    // Thread related members
    // ------------------------------------------------------------------------
    void terminate();
    virtual void run();

  private:
    ZThread::RecursiveMutex m_mtx;
    bool m_exit_notification;

    ScopeProtocolServer* m_ps;

    static const long sc_cmd_to_sec = 0;
    static const long sc_cmd_to_usec = 100000;
  }; // ScopeProtocolServerLoop
}

#endif // VTRACKING_SCOPEPROTOCOLSERVER_H

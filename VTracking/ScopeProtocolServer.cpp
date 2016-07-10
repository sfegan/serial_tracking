//-*-mode:c++; mode:font-lock;-*-

/**
 * \file ScopeProtocolServer.cpp
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

#include <iostream>
#include <iomanip>

#include <sys/time.h>

#include <zthread/Thread.h>
#include <zthread/Guard.h>

#include <Debug.h>

#include "ScopeProtocolServer.h"

using namespace VTracking;
using namespace SEphem;
using namespace VMessaging;

// ----------------------------------------------------------------------------
// Scope Protocol Server
// ----------------------------------------------------------------------------

ScopeProtocolServer::~ScopeProtocolServer()
{
  // nothing to see here
}

// ----------------------------------------------------------------------------
// ProtocolServerLoop
// ----------------------------------------------------------------------------

ScopeProtocolServerLoop::~ScopeProtocolServerLoop()
{
  // nothing to see here
}

ScopeProtocolServerLoop::
ScopeProtocolServerLoop(ScopeProtocolServer* ps)
  : ZThread::Runnable(), m_mtx(), m_exit_notification(false), m_ps(ps)
{
  // nothing to see here
}

void VTracking::ScopeProtocolServerLoop::terminate()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  m_exit_notification=true;
}

void VTracking::ScopeProtocolServerLoop::run()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  while(1)
    {
      {
	ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
	if(m_exit_notification)return;
      }
      try
	{
	  m_ps->processOneCommand(sc_cmd_to_sec,sc_cmd_to_usec);
	}
      catch(const Exception& e)
	{
	  e.print(Debug::stream());
	  abort();
	}
    }
}


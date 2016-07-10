//-*-mode:c++; mode:font-lock;-*-

/**
 * \file Messenger.cpp
 * \ingroup VMessaging
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

#include <zthread/Guard.h>

#include "Messenger.h"

using namespace VMessaging;
using namespace ZThread;

std::auto_ptr<RelayMessenger> RelayMessenger::s_instance;

Messenger::~Messenger() throw()
{
  // nothing to see here
}

// ============================================================================
//
// RelayMessenger
//
// ============================================================================

RelayMessenger* Messenger::relay()
{
  return RelayMessenger::instance();
}

RelayMessenger::~RelayMessenger() throw()
{
  // nothing to see here
}

bool RelayMessenger::sendMessage(const Message& message) throw()
{
  Guard<RecursiveMutex> guard(m_mutex);
  if(m_hold)
    {
      m_backlog.push(message);
    }
  else
    {
      return doSendMessage(message);
    }
  return false;
}

void RelayMessenger::registerMessenger(Messenger* m) throw()
{
  Guard<RecursiveMutex> guard(m_mutex);
  m_set.insert(m);
}

void RelayMessenger::unRegisterMessenger(Messenger* m) throw()
{
  Guard<RecursiveMutex> guard(m_mutex);
  m_set.erase(m);
}

void RelayMessenger::holdMessages() throw()
{
  Guard<RecursiveMutex> guard(m_mutex);
  m_hold=true;
}

void RelayMessenger::releaseMessages() throw()
{
  Guard<RecursiveMutex> guard(m_mutex);
  if(m_hold)
    {
      while(!m_backlog.empty())
	{
	  doSendMessage(m_backlog.front());
	  m_backlog.pop();
	}
      m_hold=false;
    }
}

bool RelayMessenger::doSendMessage(const Message& message) throw()
{
  bool is_ok = true;
  for(std::set<Messenger*>::iterator i=m_set.begin(); i!=m_set.end(); i++)
    is_ok &= (*i)->sendMessage(message);
  return is_ok;
}

RelayMessenger* RelayMessenger::instance()
{
  if(!s_instance.get())s_instance.reset(new RelayMessenger);
  return s_instance.get();
}

// ============================================================================
//
// TaskMessenger
//
// ============================================================================

TaskMessenger::~TaskMessenger() throw()
{
  // nothing to see here
}

bool TaskMessenger::sendMessage(const Message& message) throw()
{
  VTaskNotification::Task* task = new SendMessageTask(message,m_messenger);
  m_tasklist->scheduleTask(task);
  return true;
}

TaskMessenger::SendMessageTask::~SendMessageTask()
{
  // nothing to see here
}

void TaskMessenger::SendMessageTask::doTask()
{
  m_messenger->sendMessage(m_message);
}

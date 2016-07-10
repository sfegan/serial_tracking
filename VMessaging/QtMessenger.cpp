//-*-mode:c++; mode:font-lock;-*-

/**
 * \file QtMessenger.cpp
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

#include<QtNotification.h>

#include"QtMessenger.h"

using namespace VMessaging;
using namespace VTaskNotification;

std::auto_ptr<QtMessenger> QtMessenger::s_instance;

QtMessenger::~QtMessenger() throw()
{
  // nothing to see here
}

bool QtMessenger::sendMessage(const Message& message) throw()
{
  Message* msg_copy = new Message(message);
  MessageNotification* notice = new MessageNotification(msg_copy,this);
  QtNotificationList::getInstance()->scheduleNotification(notice);
  return true;
}

void QtMessenger::emitMessage(const Message* m)
{
  emit(message(*m));
}

QtMessenger* QtMessenger::instance()
{
  if(s_instance.get() == 0)
    s_instance.reset(new QtMessenger(0,"QtMessenger default instance"));
  return s_instance.get();
}

QtMessenger::MessageNotification::~MessageNotification()
{
  // nothing to see here
}

void QtMessenger::MessageNotification::doNotification()
{
  m_messenger->emitMessage(m_message);
  delete m_message;
}

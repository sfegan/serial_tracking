//-*-mode:c++; mode:font-lock;-*-

/**
 * \file NET_Messenger.cpp
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

#include"NET_Message.h"
#include"NET_Messenger.h"

using namespace VMessaging;

const char* NET_Messenger::object_name = "EventChannel";

NET_Messenger::~NET_Messenger() throw()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_consumer_mtx);

  if(m_consumer)
    {
      CORBA::release(m_consumer);
      m_consumer = 0;
    }
}

bool NET_Messenger::sendMessage(const Message& message) throw()
{
  if(message.realm() == Message::DR_LOCAL)return true;

  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_consumer_mtx);

  try
    {
      if(!m_consumer)
	{
	  m_consumer = 
	    m_orb->eventGetProxyPushConsumer(m_program,m_object,m_scope_num);
	  CosEventComm::PushSupplier_var nil_supplier =
	    CosEventComm::PushSupplier::_nil();
	  m_consumer->connect_push_supplier(nil_supplier);

	  Message message(Message::DR_LOCAL,Message::PS_ROUTINE,
			  "Event channel");
	  message.messageStream() 
	    << "Contact with the event channel has been established."
	    << std::endl
	    << "Messages will be sent to the channel for remote display."
	    << std::endl;
	  Messenger::relay()->sendMessage(message);
	}
      
      NET_Message net_message;
      net_message.zone = m_scope_num;
      net_message.hostname = CORBA::string_dup(message.hostname().c_str());
      net_message.program = CORBA::string_dup(message.program().c_str());
      net_message.title = CORBA::string_dup(message.title().c_str());
      net_message.realm = NET_DR_GLOBAL;
      switch(message.significance())
	{
	case Message::PS_ROUTINE: 
	  net_message.significance=NET_PS_ROUTINE;
	  break;
	case Message::PS_UNUSUAL:
	  net_message.significance=NET_PS_UNUSUAL;
	  break;
	case Message::PS_EXCEPTIONAL:
	  net_message.significance=NET_PS_EXCEPTIONAL;
	  break;
	case Message::PS_CRITICAL:
	  net_message.significance=NET_PS_CRITICAL;
	  break;
	default:
	  assert(0);
	}
      net_message.message = CORBA::string_dup(message.message().c_str());
      net_message.details = CORBA::string_dup(message.details().c_str());
      net_message.function = CORBA::string_dup(message.function().c_str());
      net_message.tv_sec = message.time().tv_sec;
      net_message.tv_usec = message.time().tv_usec;

      CORBA::Any any;
      any <<= net_message;
      
      m_consumer->push(any);
      return true;
    }
  catch(const CORBA::Exception& x)
    {
      bool send_message = false;
      if(m_consumer)
	{
	  try
	    {
	      m_consumer->disconnect_push_consumer();
	    }
	  catch(...)
	    {
	      // nothing to see here
	    }
	  CORBA::release(m_consumer);
	  m_consumer=0;
	  send_message = true;
	}
      
      if(send_message)
	{
	  Message message(Message::DR_LOCAL,Message::PS_EXCEPTIONAL,
			  "CORBA Exception");
	  message.messageStream() 
	    << "A CORBA exception was thrown while attempting "
	    "to communicate with"
	    << std::endl
	    << "the event channel. Messages will not be sent to "
	    "the channel until"
	    << std::endl
	    << "contact is re-established" << std::endl;
	  message.detailsStream() 
	    << "CORBA Exception: " << x._name() << std::endl;
	  Messenger::relay()->sendMessage(message);
	}
      return false;
    }
  
}


//-*-mode:c++; mode:font-lock;-*-

/**
 * \file NET_MessageReceiverPolling.cpp
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

#include<iostream>

#include"NET_MessageReceiverPolling.h"

using namespace VMessaging;

NET_MessageReceiverPolling::~NET_MessageReceiverPolling()
{
  // nothing to see here
}

void NET_MessageReceiverPolling::iterate()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  try
    {
      if(!m_supplier)
	{
	  m_supplier = 
	    m_orb->eventGetProxyPullSupplier(m_program,m_object,m_scope_num);
	  CosEventComm::PullConsumer_var nil_consumer =
	    CosEventComm::PullConsumer::_nil();
	  m_supplier->connect_pull_consumer(nil_consumer);

	  Message message(Message::DR_LOCAL,Message::PS_ROUTINE,
			  "Event channel");
	  message.messageStream() 
	    << "Contact with the event channel has been established."
	    << std::endl
	    << "Asynchronous messages from the telescope will be displayed."
	    << std::endl;
	  Messenger::relay()->sendMessage(message);
	}
      
      CORBA::Any_var any;
      CORBA::Boolean has_event;
      any = m_supplier->try_pull(has_event);
      while(has_event)
	{
	  dispatchMessage(any);
	  any = m_supplier->try_pull(has_event);
	}
    }
  catch(const CORBA::Exception& x)
    {
      bool send_message = false;
      if(m_supplier)
	{
	  try
	    {
	      m_supplier->disconnect_pull_supplier();
	    }
	  catch(...)
	    {
	      // nothing to see here
	    }
	  CORBA::release(m_supplier);
	  m_supplier=0;
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
	    << "the event channel. Asynchronous messages from "
	    "the telescope will"
	    << std::endl
	    << "not be displayed until contact is re-established" << std::endl;
	  message.detailsStream() 
	    << "CORBA Exception: " << x._name() << std::endl;
	  Messenger::relay()->sendMessage(message);
	}
    }
}

void NET_MessageReceiverPolling::loopIsTerminating()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(m_supplier)
    {
      try
	{
	  m_supplier->disconnect_pull_supplier();
	}
      catch(...)
	{
	  // nothing to see here
	}
      CORBA::release(m_supplier);
      m_supplier=0;
    }
}

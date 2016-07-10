//-*-mode:c++; mode:font-lock;-*-

/**
 * \file NET_MessageReceiverServant.cpp
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
 * $Date: 2010/10/28 14:48:05 $
 * $Revision: 2.2 $
 * $Tag$
 *
 **/

#include"NET_MessageReceiverServant.h"
#include"NET_Message.h"

#include<omniORB4/CORBA.h>
#include<CosEventChannelAdmin.hh>

using namespace CORBA;
using namespace VMessaging;

NET_MessageReceiverServant::~NET_MessageReceiverServant()
{
  disconnect();
}

void NET_MessageReceiverServant::push(const CORBA::Any& any)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  dispatchMessage(any);
}

void NET_MessageReceiverServant::disconnect_push_consumer()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
#if 0
  ORB_var the_orb = m_orb->orb();
  Object_var obj = the_orb->resolve_initial_references("POACurrent");
  PortableServer::Current_var current =
    PortableServer::Current::_narrow(obj);
  PortableServer::POA_var poa = current->get_POA();
  PortableServer::ObjectId_var oid = current->get_object_id();
  poa->deactivate_object(oid);
#endif
  disconnect();
}

void NET_MessageReceiverServant::
connect(const char* program, const char* object, int telescopeNumber)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_supplier_mtx);
  disconnect();
  m_supplier = 
    m_orb->eventGetProxyPushSupplier(program,object,telescopeNumber);
  m_supplier->connect_push_consumer(_this());
}

void NET_MessageReceiverServant::disconnect()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_supplier_mtx);
  if(m_supplier)
    {
      try
	{
	  m_supplier->disconnect_push_supplier();
	}
      catch(...)
	{
	  // nothing to see here
	}
      CORBA::release(m_supplier);
      m_supplier=0;
    }
}

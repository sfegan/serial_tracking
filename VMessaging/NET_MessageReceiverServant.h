//-*-mode:c++; mode:font-lock;-*-

/**
 * \file NET_MessageReceiverServant.h
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
 * $Revision: 2.1 $
 * $Tag$
 *
 **/

#ifndef VMESSAGING_NET_MESSENGERSERVANT_H
#define VMESSAGING_NET_MESSENGERSERVANT_H

#include<set>

#include<CosEventChannelAdmin.hh>
#include<CosEventComm.hh>

#include<VOmniORBHelper.h>

#include"NET_MessageReceiver.h"

namespace VMessaging
{
  
  class NET_MessageReceiverServant:
    public virtual NET_MessageReceiver,
    public virtual POA_CosEventComm::PushConsumer
  {
  public:
    NET_MessageReceiverServant(VCorba::VOmniORBHelper* orb, 
			       Messenger* messenger):
      NET_MessageReceiver(orb, messenger), PushConsumer(), 
      m_supplier(0), m_supplier_mtx()
    { /* nothing to see here */ }
    virtual ~NET_MessageReceiverServant();
    virtual void push(const CORBA::Any& any);
    virtual void disconnect_push_consumer();

    void connect(const char* program, const char* object, 
		 int telescopeNumber = -1);
    void disconnect();
  private:
    CosEventChannelAdmin::ProxyPushSupplier_ptr m_supplier;
    ZThread::RecursiveMutex                     m_supplier_mtx;
  };

}

#endif // VMESSAGING_NET_MESSENGERSERVANT_H

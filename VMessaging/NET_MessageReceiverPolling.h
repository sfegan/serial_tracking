//-*-mode:c++; mode:font-lock;-*-

/**
 * \file NET_MessageReceiverPolling.h
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

#ifndef VMESSAGING_NET_MESSENGERPOLLING_H
#define VMESSAGING_NET_MESSENGERPOLLING_H

#include<set>

#include<CosEventChannelAdmin.hh>
#include<CosEventComm.hh>

#include<PhaseLockedLoop.h>
#include<VOmniORBHelper.h>

#include"NET_MessageReceiver.h"

namespace VMessaging
{
  
  class NET_MessageReceiverPolling:
    public virtual NET_MessageReceiver,
    public virtual VTaskNotification::PhaseLockedLoop
  {
  public:
    NET_MessageReceiverPolling(VCorba::VOmniORBHelper* orb, 
			      Messenger* messenger,
			      int sleep_time,
			      const char* program, const char* object, 
			      int telescopeNumber = -1):
      NET_MessageReceiver(orb, messenger), 
      PhaseLockedLoop(sleep_time, sleep_time/10, 0),
      m_supplier(0),
      m_program(program), m_object(object), m_scope_num(telescopeNumber)
    { /* nothing to see here */ }
    virtual ~NET_MessageReceiverPolling();

  protected:
    virtual void iterate();
    virtual void loopIsTerminating();

  private:
    CosEventChannelAdmin::ProxyPullSupplier_ptr m_supplier;
    const char*                                 m_program;
    const char*                                 m_object;
    int                                         m_scope_num;
  };

}

#endif // VMESSAGING_NET_MESSENGERPOLLING_H

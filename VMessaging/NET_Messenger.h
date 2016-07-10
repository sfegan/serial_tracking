//-*-mode:c++; mode:font-lock;-*-

/**
 * \file NET_Messenger.h
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

#ifndef VMESSAGING_NET_MESSENGER_H
#define VMESSAGING_NET_MESSENGER_H

#include<CosEventChannelAdmin.hh>
#include<CosEventComm.hh>

#include<VOmniORBHelper.h>

#include"Messenger.h"

namespace VMessaging
{
  class NET_Messenger: public Messenger
  {
  public:
    NET_Messenger(VCorba::VOmniORBHelper* orb, 
		  const char* program, const char* object, 
		  int telescopeNumber = -1):
      Messenger(), m_orb(orb),
      m_program(program), m_object(object), m_scope_num(telescopeNumber),
      m_consumer(0), m_consumer_mtx()
    { /* nothing to see here */ }
      
    virtual ~NET_Messenger() throw();
    virtual bool sendMessage(const Message& message) throw();

    static const char* object_name;
  private:
    VCorba::VOmniORBHelper*                     m_orb;
    const char*                                 m_program;
    const char*                                 m_object;
    int                                         m_scope_num;
    CosEventChannelAdmin::ProxyPushConsumer_ptr m_consumer;
    ZThread::RecursiveMutex                     m_consumer_mtx;
  };
} // namespace VMessaging

#endif // VMESSAGING_NET_MESSENGER_H

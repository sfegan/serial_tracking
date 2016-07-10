//-*-mode:c++; mode:font-lock;-*-

/**
 * \file NET_MessageReceiver.h
 * \ingroup VMessaging
 * \brief Base for classes capable of receiving messages from CORBA
 *
 * This base class implements functionality common to all classes
 * capable of receiving CORBA::Any types which contain NET_Message
 * instances. Derived classes should arrange for "dispatchMessage"
 * to be called for each Any received.
 *
 * Original Author: Stephen Fegan
 * $Author: aune $
 * $Date: 2010/09/09 18:56:11 $
 * $Revision: 2.1 $
 * $Tag$
 *
 **/

#ifndef VMESSAGING_NET_MESSAGERECEIVER_H
#define VMESSAGING_NET_MESSAGERECEIVER_H

#include<set>

#include<zthread/RecursiveMutex.h>
#include<omniORB4/CORBA.h>

#include<VOmniORBHelper.h>

#include"Messenger.h"

namespace VMessaging
{
  
  class NET_MessageReceiver
  {
  public:
    NET_MessageReceiver(VCorba::VOmniORBHelper* orb, Messenger* messenger):
      m_orb(orb), m_messenger(messenger), m_filter(), m_filter_mtx()
    { /* nothing to see here */ }
    virtual ~NET_MessageReceiver();
    
    void dispatchMessage(const CORBA::Any& any);

    void addToZoneFilter(int zone);
    void clearZoneFilter();

  protected:
    VCorba::VOmniORBHelper*        m_orb;
    Messenger*                     m_messenger;
    std::set<int>                  m_filter;
    ZThread::RecursiveMutex        m_filter_mtx;
  };

}

#endif // VMESSAGING_NET_MESSAGERECEIVER_H

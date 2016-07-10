//-*-mode:c++; mode:font-lock;-*-

/**
 * \file NET_MessageReceiver.cpp
 * \ingroup VMessaging
 * \brief Base for classes capable of receiving messages from CORBA
 *
 * This base class implements functionality common to all classes
 * capable of receiving CORBA::Any types which contain NET_Message
 * instances. Derived classes should arrange for "dispatchMessage"
 * to be called for each Any received.
 *
 * Original Author: Stephen Fegan
 * $Author: sfegan $
 * $Date: 2006/04/04 17:06:58 $
 * $Revision: 2.0 $
 * $Tag$
 *
 **/

#include<iostream>

#include"NET_MessageReceiver.h"
#include"NET_Message.h"

using namespace CORBA;
using namespace VMessaging;

NET_MessageReceiver::~NET_MessageReceiver()
{
  // nothing to see here
}

void NET_MessageReceiver::dispatchMessage(const CORBA::Any& any)
{ 
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  NET_Message* net_message = 0;
  if(any >>= net_message)
    {
      {
	ZThread::Guard<ZThread::RecursiveMutex> guard(m_filter_mtx);
	if((!m_filter.empty())
	   &&(m_filter.find(net_message->zone)==m_filter.end()))
	  return;
      }
      
      Message::DistributionRealm realm;
      switch(net_message->realm)
	{
	case NET_DR_LOCAL: realm = Message::DR_LOCAL; break;
	case NET_DR_GLOBAL: realm = Message::DR_GLOBAL; break;
	default: assert(0);
	}

      Message::PayloadSignificance significance;
      switch(net_message->significance)
	{
	case NET_PS_ROUTINE: significance = Message::PS_ROUTINE; break;
	case NET_PS_UNUSUAL: significance = Message::PS_UNUSUAL; break;
	case NET_PS_EXCEPTIONAL: significance = Message::PS_EXCEPTIONAL; break;
	case NET_PS_CRITICAL: significance = Message::PS_CRITICAL; break;
	default: assert(0);
	}

      Message message(net_message->hostname.in(),
		      Message::MO_REMOTE, net_message->zone,
		      net_message->program.in(),
		      net_message->title.in(), realm, significance, 
		      net_message->message.in(), net_message->details.in(),
		      net_message->function.in(),
		      net_message->tv_sec, net_message->tv_usec);
      m_messenger->sendMessage(message);
    }
}

void NET_MessageReceiver::addToZoneFilter(int zone)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_filter_mtx);
  m_filter.insert(zone); 
}

void NET_MessageReceiver::clearZoneFilter() 
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_filter_mtx);
  m_filter.clear(); 
}    

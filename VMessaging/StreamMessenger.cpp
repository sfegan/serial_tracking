//-*-mode:c++; mode:font-lock;-*-

/**
 * \file StreamMessenger.cpp
 * \ingroup VMessaging
 * \brief This is a one-line description of this cpp file.
 *
 * Here is a tedious verbose multi-line description of all
 * the details of the code, more than you would
 * ever want to read. Generally, all the important documentation
 * goes in the .cpp files.
 *
 * Original Author: Stephen Fegan
 * $Author: aune $
 * $Date: 2010/09/09 18:56:11 $
 * $Revision: 2.2 $
 * $Tag$
 *
 **/

#include "StreamMessenger.h"
#include <cstdio>
#include <stdlib.h>
#include <string.h>

VMessaging::StreamMessenger::
~StreamMessenger() throw()
{
  // nothing to see here
}

bool VMessaging::StreamMessenger::
sendMessage(const VMessaging::Message& message) throw()
{
#warning LOCKING
  std::string ps_str;
  switch(message.significance())
    {
    case Message::PS_ROUTINE:     ps_str="ROUTINE";     break;
    case Message::PS_UNUSUAL:     ps_str="UNUSUAL";     break;
    case Message::PS_EXCEPTIONAL: ps_str="EXCEPTIONAL"; break;
    case Message::PS_CRITICAL:    ps_str="CRITICAL";    break;
    }
  
  std::string msg = message.message();
  std::string det = message.details();
  std::string fun = message.function();
  
  char ctime_buf[26];
  ctime_r(&message.time().tv_sec,ctime_buf);
  ctime_buf[strlen(ctime_buf)-1]='\0';

  (*m_os) << std::string(80,'-') << std::endl
	  << message.title() << " (" << ps_str << ' ' 
	  << ctime_buf << ')' << std::endl;

  if(msg != "") 
    {
      (*m_os) << std::endl << msg;
      if(msg[msg.size()-1]!='\n')(*m_os) << std::endl;
    }

  if(det != "") 
    {
      (*m_os) << std::endl << "Details: " << std::endl << det;
      if(det[det.size()-1]!='\n')(*m_os) << std::endl;
    }

  if((fun != "")
     &&(message.significance() != Message::PS_ROUTINE))
    {
      (*m_os) << std::endl << "Function backtrace: " << std::endl << fun;
      if(fun[fun.size()-1]!='\n')(*m_os) << std::endl;
    }
  (*m_os) << std::string(80,'-') << std::endl;
  
  return *m_os;
}

//-*-mode:c++; mode:font-lock;-*-

/**
 * \file Message.cpp
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

#include<string>

#include"Exception.h"
#include"Message.h"

using namespace VMessaging;

std::string Message::s_this_program;
std::string Message::s_this_hostname;

Message::Message(const Message& m) throw():
  m_hostname(m.m_hostname), m_origin(m.m_origin), m_zone(m.m_zone),
  m_program(m.m_program), m_title(m.m_title), m_realm(m.m_realm),
  m_significance(m.m_significance), m_message_oss(m.m_message_oss.str()),
  m_details_oss(m.m_details_oss.str()), m_function(m.m_function),
  m_time(m.m_time)
{
  // nothing to see here
}


const Message& 
Message::operator= (const Message& m) throw()
{
  m_hostname=m.m_hostname;
  m_origin=m.m_origin;
  m_zone=m.m_zone;
  m_program=m.m_program;
  m_title=m.m_title;
  m_realm=m.m_realm,
  m_significance=m.m_significance;
  m_message_oss.str(m_message_oss.str());
  m_details_oss.str(m_details_oss.str());
  m_function=m.m_function;
  m_time.tv_sec=m.m_time.tv_sec;
  m_time.tv_usec=m.m_time.tv_usec;
  return *this;
}

Message::Message(const Exception& e, DistributionRealm realm, 
		 PayloadSignificance sig, bool highlight) throw():
  m_hostname(s_this_hostname), m_origin(MO_LOCAL), m_zone(-1),
  m_program(s_this_program), m_title(e.title()), m_realm(realm), 
  m_significance(sig), m_message_oss(e.message()), m_details_oss(e.details()), 
  m_function(e.function(highlight)), m_time()
{
  m_time.tv_sec=e.time().tv_sec;
  m_time.tv_usec=e.time().tv_usec;
}

Message::Message(const std::string& hostname, MessageOrigin origin, int zone,
		 const std::string& program, const std::string& title, 
		 DistributionRealm realm, PayloadSignificance significance, 
		 const std::string& message, const std::string& details, 
		 const std::string& function, long time_sec, int time_usec) 
  throw():
  m_hostname(hostname), m_origin(origin), m_zone(zone),
  m_program(program), m_title(title), m_realm(realm), 
  m_significance(significance), m_message_oss(message), m_details_oss(details),
  m_function(function), m_time()
{
  m_time.tv_sec=time_sec;
  m_time.tv_usec=time_usec;
}

Message::
Message(DistributionRealm realm, PayloadSignificance sig, 
	const std::string title) throw():
  m_hostname(s_this_hostname),  m_origin(MO_LOCAL), m_zone(-1),
  m_program(s_this_program), m_title(title), m_realm(realm), 
  m_significance(sig), m_message_oss(), m_details_oss(), 
  m_function(BackTrace::instance()->backTraceString()), m_time()
{
  gettimeofday(&m_time,0);
}

Message::
Message(DistributionRealm realm, PayloadSignificance sig, 
	const std::string title, const std::string& originating_fn) throw():
  m_hostname(s_this_hostname), m_origin(MO_LOCAL), m_zone(-1),
  m_program(s_this_program),  m_title(title), m_realm(realm), 
  m_significance(sig), m_message_oss(), m_details_oss(), 
  m_function(originating_fn), m_time()
{
  gettimeofday(&m_time,0);
}

Message::~Message() throw()
{
  // nothing to see here
}

void Message::setProgram(const std::string& p, const std::string& h)
{
  s_this_program=p;
  s_this_hostname=h;
  if(h.empty())
    {
      char hostname[256];
      gethostname(hostname,256);
      hostname[255]='\0';
      s_this_hostname=hostname;      
    }
}

void Message::print(std::ostream& os) const
{
  std::string or_str;
  switch(origin())
    {
    case MO_LOCAL: or_str="LOCAL"; break;
    case MO_REMOTE: or_str="REMOTE"; break;
    }

  std::string dr_str;
  switch(realm())
    { 
    case DR_LOCAL:  dr_str="LOCAL";  break;
    case DR_GLOBAL: dr_str="GLOBAL"; break;
    }

  std::string ps_str;
  switch(significance())
    {
    case PS_ROUTINE:     ps_str="ROUTINE";     break;
    case PS_UNUSUAL:     ps_str="UNUSUAL";     break;
    case PS_EXCEPTIONAL: ps_str="EXCEPTIONAL"; break;
    case PS_CRITICAL:    ps_str="CRITICAL";    break;
    }

  os << "Message" << std::endl
     << "-------" << std::endl
     << "Origin:       " << or_str << std::endl
     << "Hostname:     " << hostname() << std::endl
     << "Program:      " << program() << std::endl
     << "Title:        " << title() << std::endl
     << "Distribution: " << dr_str << std::endl
     << "Significance: " << ps_str << std::endl
     << "Time:         " << ctime(&time().tv_usec) << std::endl
     << "Message:      " << message() << std::endl
     << "Details:      " << details()  << std::endl
     << "Function:     " << function() << std::endl;
}

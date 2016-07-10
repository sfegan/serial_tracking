//-*-mode:c++; mode:font-lock;-*-

/**
 * \file Message.h
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

#ifndef VMESSAGING_MESSAGE_H
#define VMESSAGING_MESSAGE_H

#include<string>
#include<ostream>
#include<sstream>

#include<sys/time.h>

#include"Exception.h"

namespace VMessaging
{
  class Message
  {
  public:
    enum DistributionRealm
      { DR_LOCAL, DR_GLOBAL };
    enum PayloadSignificance
      { PS_ROUTINE, PS_UNUSUAL, PS_EXCEPTIONAL, PS_CRITICAL };
    enum MessageOrigin
      { MO_LOCAL, MO_REMOTE };
    
    Message(const Message& m) throw();
    const Message& operator= (const Message& m) throw();

    Message(const Exception& e, 
	    DistributionRealm realm, PayloadSignificance sig, 
	    bool highlight=false) throw();
    
    Message(const std::string& hostname, MessageOrigin origin, int zone,
	    const std::string& program, const std::string& title, 
	    DistributionRealm realm, PayloadSignificance significance, 
	    const std::string& message, const std::string& details, 
	    const std::string& function, long time_sec, int time_usec) throw();

    Message(DistributionRealm realm, PayloadSignificance sig, 
	    const std::string title) throw();
    Message(DistributionRealm realm, PayloadSignificance sig, 
	    const std::string title, const std::string& originating_fn) 
      throw();

    virtual ~Message() throw();

    std::string hostname() const throw() { return m_hostname; }
    MessageOrigin origin() const throw() { return m_origin; }
    int zone() const throw() { return m_zone; }
    std::string program() const throw() { return m_program; }
    std::string title() const throw() { return m_title; }
    DistributionRealm realm() const throw() { return m_realm; }
    PayloadSignificance significance() const throw() { return m_significance; }
    std::string message() const throw() { return m_message_oss.str(); }
    std::string details() const throw() { return m_details_oss.str(); }
    std::string function() const throw() { return m_function; }
    const struct timeval& time() const throw() { return m_time; }
    
    std::ostream& messageStream() throw() { return m_message_oss; }
    std::ostream& detailsStream() throw() { return m_details_oss; }

    void print(std::ostream& os) const;

    static void setProgram(const std::string& p, const std::string& h = "");

  private:
    std::string m_hostname;
    MessageOrigin m_origin;
    int m_zone;
    std::string m_program;
    std::string m_title;
    DistributionRealm m_realm;
    PayloadSignificance m_significance;
    std::ostringstream m_message_oss;
    std::ostringstream m_details_oss;
    std::string m_function;
    struct timeval m_time;

    static std::string s_this_program;
    static std::string s_this_hostname;

    Message();
  };

} // namespace VMessaging

#endif // VMESSAGING_MESSAGE_H

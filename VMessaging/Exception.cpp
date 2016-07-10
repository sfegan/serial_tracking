//-*-mode:c++; mode:font-lock;-*-

/**
 * \file Exception.cpp
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
 * $Revision: 2.3 $
 * $Tag$
 *
 **/

#include<iostream>
#include<sstream>

#include<sys/time.h>

#include"Debug.h"
#include"Exception.h"
#include <cstdlib>
#include <stdlib.h>
#include <cstdio>
#include <string.h>

using namespace VMessaging;

// ----------------------------------------------------------------------------
// BackTrace
// ----------------------------------------------------------------------------

ZThread::ThreadLocal<BackTrace*> BackTrace::s_instance;

BackTrace::~BackTrace()
{
  // nothing to see here
}

BackTrace* BackTrace::instance()
{
  if(s_instance.get() == 0)s_instance.set(new BackTrace);
  return s_instance.get();
}

std::vector<std::string> BackTrace::backTrace() const
{
  std::vector<std::string> bt;
  for(std::vector<const char*>::const_iterator i=m_backtrace.begin();
      i!=m_backtrace.end();i++)
    bt.push_back(std::string(*i));
  return bt;
}

std::string BackTrace::backTraceString() const
{
  std::ostringstream oss;
  for(std::vector<const char*>::const_iterator i=m_backtrace.begin();
      i!=m_backtrace.end();i++)
    oss << *i << std::endl;
  return oss.str();
}

void BackTrace::catchSignalPrintBacktraceAndAbort(int signum)
{
  signal(signum,&catchHandler);
}

void BackTrace::catchHandler(int signum)
{
  psignal(signum,"BackTrace::catchHandler caught signal");
  Debug::stream()
    << "Function backtrace:" << std::endl
    << instance()->backTraceString();
  abort();
}

RegisterThisFunction::RegisterThisFunction(const char* fn)
{
  //std::cerr << pthread_self() << ':' << fn << '\n';
  BackTrace::instance()->push(fn);
}

RegisterThisFunction::~RegisterThisFunction()
{
  BackTrace::instance()->pop();
}

// ----------------------------------------------------------------------------
// Exceptions
// ----------------------------------------------------------------------------

Throwable::~Throwable()
{
  // nothing to see here
}

Exception::Exception(const Exception& e) throw(): Throwable(),
  m_title(e.m_title), m_message_oss(e.m_message_oss.str()),
  m_details_oss(e.m_details_oss.str()), m_backtrace(e.m_backtrace),
  m_time(e.m_time)
{
  // nothing to see here
}

const Exception& Exception::operator= (const Exception& e) throw()
{
  m_title = e.m_title;
  m_message_oss.str(e.m_message_oss.str());
  m_details_oss.str(e.m_details_oss.str());
  m_backtrace=e.m_backtrace;
  m_time.tv_sec=e.m_time.tv_sec;
  m_time.tv_usec=e.m_time.tv_usec;
  return *this;
}

Exception::Exception(const std::string& title, const std::string& message,
		     const std::string& details) throw(): Throwable(),
  m_title(title), m_message_oss(message), m_details_oss(details),
  m_backtrace(BackTrace::instance()->backTrace()), m_time()
{
  gettimeofday(&m_time,0);
}

Exception::~Exception() throw()
{
  // nothing to see here
}

std::string Exception::function(bool highlight) const throw()
{
  std::ostringstream oss;

  unsigned highlight_num=m_backtrace.size()-1;
  if(highlight)highlight_num=BackTrace::instance()->backTrace().size();
  
  for(unsigned i=0; i<m_backtrace.size(); i++)
    {
      if((highlight)&&(m_backtrace[m_backtrace.size()-i-1] !=
		       BackTrace::instance()->backTrace().at(highlight_num-i)))
	highlight=false;
      
      if((highlight)&&(i==highlight_num))oss 
	<< m_backtrace[m_backtrace.size()-i-1] << " (*)" << std::endl;
      else oss << m_backtrace[m_backtrace.size()-i-1] << std::endl;
    }
  return oss.str();
}

void Exception::print(std::ostream& os) const
{
  os << m_title << std::endl
     << m_message_oss.str() << std::endl;
  if(m_details_oss.str()!="")
    os << "Details:" << std::endl
       << m_details_oss.str() << std::endl;
  if(m_backtrace.size()>0)
    for(unsigned i=0; i<m_backtrace.size(); i++)
      os << (i==0?"Function back trace: ":"                     ") 
	 << m_backtrace[m_backtrace.size()-i-1] << std::endl;
}

SystemError::SystemError(const std::string& message, int error_num) 
  throw(): Exception("System Error", message, strerror(error_num)),
	   m_error_num(error_num)
{
  // nothing to see here
}

SystemError::~SystemError() throw()
{
  // nothing to see here
}

//-*-mode:c++; mode:font-lock;-*-

/**
 * \file Exception.h
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

#ifndef VMESSAGING_EXCEPTION_H
#define VMESSAGING_EXCEPTION_H

#include<memory>
#include<vector>
#include<string>
#include<sstream>

#include<errno.h>
#include<signal.h>

#include<zthread/ThreadLocal.h>

namespace VMessaging
{
  class BackTrace
  {
  public:
    virtual ~BackTrace();

    void push(const char* fn) { m_backtrace.push_back(fn); }
    void pop() { m_backtrace.pop_back(); }
    std::vector<std::string> backTrace() const;
    std::string backTraceString() const;

    // Singleton
    static BackTrace* instance();

    // Signal catching
    static void catchSignalPrintBacktraceAndAbort(int signum);

  private:
    BackTrace(): m_backtrace() { } 
    std::vector<const char*> m_backtrace;

    // Singleton
    static ZThread::ThreadLocal<BackTrace*> s_instance;

    // Handler
    static void catchHandler(int signum);
  };

  class RegisterThisFunction
  {
  public:
    RegisterThisFunction(const char* fn);
    virtual ~RegisterThisFunction();
  private:
    RegisterThisFunction(const RegisterThisFunction&);
    RegisterThisFunction operator=(const RegisterThisFunction&);
  };

  class Throwable
  {
  public:
    virtual ~Throwable();
  };

  class Exception: public Throwable
  {
  public:
    Exception(const Exception& e) throw();
    const Exception& operator= (const Exception& e) throw();

    Exception(const std::string& title, 
	      const std::string& message="", const std::string& details="") 
      throw();

    virtual ~Exception() throw();

    std::string title() const throw() { return m_title; }
    std::string message() const throw() { return m_message_oss.str(); }
    std::string details() const throw() { return m_details_oss.str(); }
    std::string function(bool highlight=true) const throw();
    const std::vector<std::string> backTrace() const { return m_backtrace; }
    const struct timeval& time() const throw() { return m_time; }
    
    std::ostream& messageStream() throw() { return m_message_oss; }
    std::ostream& detailsStream() throw() { return m_details_oss; }

    void print(std::ostream& os) const;

  private:
    std::string m_title;
    std::ostringstream m_message_oss;
    std::ostringstream m_details_oss;
    std::vector<std::string> m_backtrace;
    struct timeval m_time;

    Exception();
  };

  class SystemError: public Exception
  {
  public:
    SystemError(const std::string& message="", int error_num=errno) 
      throw();
    virtual ~SystemError() throw();
    int errorNum() const { return m_error_num; }
  private:
    int m_error_num;
  };
} // namespace VMessaging

#endif // VMESSAGING_EXCEPTION_H

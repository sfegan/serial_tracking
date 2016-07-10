//-*-mode:c++; mode:font-lock;-*-

/**
 * \file StreamMessenger.h
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
 * $Date: 2006/04/10 18:01:11 $
 * $Revision: 2.1 $
 * $Tag$
 *
 **/

#ifndef VMESSAGING_STREAMMESSENGER_H
#define VMESSAGING_STREAMMESSENGER_H

#include<iostream>
#include<fstream>
#include<memory>

#include "Message.h"
#include "Debug.h"
#include "Messenger.h"

namespace VMessaging
{
  class StreamMessenger: public Messenger
  {
  public:
    StreamMessenger(std::ostream& stream = Debug::stream()) throw():
      Messenger(), m_os(&stream), m_my_os() { }
    StreamMessenger(const std::string filename): 
      Messenger(), m_os(new std::ofstream(filename.c_str())), m_my_os(m_os) {}
      
    virtual ~StreamMessenger() throw();
    virtual bool sendMessage(const Message& message) throw();
  private:
    std::ostream* m_os;
    std::auto_ptr<std::ostream> m_my_os;
  };
} // namespace VMessaging

#endif // VMESSAGING_STREAMMESSENGER_H

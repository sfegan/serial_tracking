//-*-mode:c++; mode:font-lock;-*-

/**
 * \file Debug.h
 * \ingroup VMessaging
 * \brief Global interface to debug stream
 *
 * Original Author: Stephen Fegan
 * $Author: sfegan $
 * $Date: 2006/04/10 18:01:11 $
 * $Revision: 2.1 $
 * $Tag$
 *
 **/

#ifndef VMESSAGING_DEBUG_H
#define VMESSAGING_DEBUG_H

#include<string>
#include<iostream>
#include<memory>

namespace VMessaging
{
  class Debug
  {
  public:
    static std::ostream& stream() { return *s_stream; }
    static void setFileStream(const char* filename);
    static void setFileStream(const std::string& filename);
    static void setStream(std::ostream* stream);
    static void setStream(std::ostream& stream);
    static void adoptStream(std::ostream* stream);
  private:
    static std::ostream*                    s_stream;
    static std::auto_ptr<std::ostream>      s_my_stream;
  };
}

#endif // VMESSAGING_DEBUG_H

//-*-mode:c++; mode:font-lock;-*-

/**
 * \file Debug.cpp
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

#include<fstream>

#include"Debug.h"

using namespace VMessaging;

std::ostream* Debug::s_stream = &std::cout;
std::auto_ptr<std::ostream> Debug::s_my_stream;

void Debug::setFileStream(const char* filename)
{
  std::ofstream* f_stream = new std::ofstream(filename);
  s_my_stream.reset(f_stream);
  s_stream=f_stream;
}

void Debug::setFileStream(const std::string& filename)
{
  setFileStream(filename.c_str());
}

void Debug::setStream(std::ostream* stream)
{
  s_my_stream.reset();
  s_stream=stream;  
}

void Debug::setStream(std::ostream& stream)
{
  s_my_stream.reset();
  s_stream=&stream;
}

void Debug::adoptStream(std::ostream* stream)
{
  s_my_stream.reset(stream);
  s_stream=stream;
}

//-*-mode:c++; mode:font-lock;-*-

/**
 * \file Global.cpp
 * \ingroup VTracking
 * \brief This is a one-line description of this cpp file.
 *
 * Here is a tedious verbose multi-line description of all
 * the details of the code, more than you would
 * ever want to read. Generally, all the important documentation
 * goes in the .cpp files.
 *
 * Original Author: Stephen Fegan
 * $Author: sfegan $
 * $Date: 2007/04/02 16:47:24 $
 * $Revision: 2.1 $
 * $Tag$
 *
 **/

#include<zthread/Guard.h>
#include<Global.h>

using namespace VTracking;

std::auto_ptr<Global> Global::s_instance;
ZThread::RecursiveMutex Global::s_instance_mutex;

Global::~Global()
{
  // nothing to see here
}

Global* Global::instance()
{
  ZThread::Guard<ZThread::RecursiveMutex> guard(s_instance_mutex);
  if(s_instance.get()==0)s_instance.reset(new Global);
  return s_instance.get();
}


//-*-mode:c++; mode:font-lock;-*-

/**
 * \file Global.h
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

#ifndef VTRACKING_GLOBAL_H
#define VTRACKING_GLOBAL_H

#include<memory>
#include<zthread/RecursiveMutex.h>

namespace VTracking 
{

  class Global
  {
  public:
    ~Global();

    ZThread::RecursiveMutex& dbMutex() { return m_db_mutex; }

    static Global* instance();
  protected:
    Global():
      m_db_mutex()
      { /* nothing to see here */ }

  private:
    static std::auto_ptr<Global>     s_instance;
    static ZThread::RecursiveMutex   s_instance_mutex;
    
    ZThread::RecursiveMutex          m_db_mutex;
  };

}

#endif

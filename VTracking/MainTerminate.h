//-*-mode:c++; mode:font-lock;-*-

/**
 * \file MainTerminate.h
 * \ingroup VTracking
 * \brief Simple main mode which sends terminate command to the CORBA server
 *
 * Here is a tedious verbose multi-line description of all the details
 * of the code, more than you would ever want to read. Generally, all
 * the important documentation goes in the .cpp files.
 *
 * Original Author: Stephen Fegan
 * $Author: sfegan $
 * $Date: 2006/04/10 18:01:11 $
 * $Revision: 2.1 $
 * $Tag$
 *
 **/

#ifndef VTRACKING_MAINTERMINATE_H
#define VTRACKING_MAINTERMINATE_H

#include"Main.h"

namespace VTracking
{

  // ==========================================================================
  // MAIN TERMINATE CLASS
  // ==========================================================================
  
  class MainTerminate: public Main
  {
  public:
    MainTerminate();
    virtual ~MainTerminate();
    virtual int main(int argc, char** argv);
    static void configure(VERITAS::VSOptions& options);
  private:
    unsigned                   m_scope_id;
    std::vector<std::string>   m_corba_args;
    std::string                m_corba_nameserver;
    std::string                m_remote_ior;
  };

}

#endif // VTRACKING_MAINTERMINATE_H

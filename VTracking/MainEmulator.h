//-*-mode:c++; mode:font-lock;-*-

/**
 * \file MainEmulator.h
 * \ingroup VTracking
 * \brief This is a one-line description of this cpp file.
 *
 * Here is a tedious verbose multi-line description of all the details
 * of the code, more than you would ever want to read. Generally, all
 * the important documentation goes in the .cpp files.
 *
 * Original Author: Stephen Fegan
 * $Author: sfegan $
 * $Date: 2006/04/04 17:06:58 $
 * $Revision: 2.0 $
 * $Tag$
 *
 **/

#ifndef VTRACKING_MAINEMULATOR_H
#define VTRACKING_MAINEMULATOR_H

#include"Main.h"

namespace VTracking
{
  
  // ==========================================================================
  // EMULATOR CLASS
  // ==========================================================================
  
  class MainEmulator: public Main
  {
  public:
    MainEmulator();
    virtual ~MainEmulator();
    virtual int main(int argc, char** argv);
    static void configure(VERITAS::VSOptions& options);
  private:
    unsigned m_verbose;
    unsigned m_scope_id;
    Protocol m_protocol;
    std::string m_datastream;
  };

}

#endif // VTRACKING_MAINEMULATOR_H

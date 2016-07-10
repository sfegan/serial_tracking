//-*-mode:c++; mode:font-lock;-*-

/**
 * \file serial_tracking.cpp
 * \ingroup VTracking
 * \brief This is a one-line description of this cpp file.
 *
 * Here is a tedious verbose multi-line description of all the details
 * of the code, more than you would ever want to read. Generally, all
 * the important documentation goes in the .cpp files.
 *
 * Original Author: Stephen Fegan
 * $Author: sfegan $
 * $Date: 2007/01/15 00:07:07 $
 * $Revision: 2.4 $
 * $Tag$
 *
 **/

#include<unistd.h>
#include<sys/types.h>

#include<Messenger.h>
#include<StreamMessenger.h>

#include"DataStream.h"
#include"Main.h"
#include"MainController.h"
#include"MainEmulator.h"
#include"MainFailsafe.h"

using namespace ZThread;
using namespace VERITAS;
using namespace VTracking;
using namespace VMessaging;
using namespace VCorba;

// ============================================================================
// MAIN
// ============================================================================

void usage(const std::string& progname, const VSOptions& options)
{
  std::cerr << "Usage: " << progname << " [options]" << std::endl;
  std::cerr << std::endl;
  std::cerr << "Options:" << std::endl;
  options.printUsage(std::cerr);
}

int main(int argc, char** argv)
{
  std::string program(*argv);

  if(geteuid()==0)
    {
      MainController::setRealtime();
      seteuid(getuid());
    }

  Message::setProgram(program);
  std::string fn_str = 
    std::string(program)+std::string(" -- ")+ std::string(__PRETTY_FUNCTION__);
  GuardedBuffer fn_c_str(fn_str.length()+1);
  strcpy(fn_c_str.get(),fn_str.c_str());
  RegisterThisFunction fnguard(fn_c_str.get());
  
  VSOptions options(argc,argv,true);

  bool print_usage = false;
  if(options.find("help","Print this message.")
     != VSOptions::FS_NOT_FOUND)print_usage = true;
  if(options.find("h","Print this message.")
     != VSOptions::FS_NOT_FOUND)print_usage = true;
  
  Main::configure(options);

  if(!options.assertNoOptions())
    {
      std::cerr << program << ": unknown options: ";
      for(unsigned i=1;i<(unsigned)argc;i++)std::cerr << argv[i];
      std::cerr << std::endl;
      std::cerr << std::endl;
      std::cerr << "Use: \"" << program 
		<< " --help\" for list of available options" << std::endl;
      exit(EXIT_FAILURE);
    }

  if(print_usage)
    {
      usage(program, options);
      exit(EXIT_SUCCESS);
    }

  time_t the_time = time(0);  
  srand(the_time);

  Main* main = Main::getMain();
  main->main(argc,argv);
  delete main;
}

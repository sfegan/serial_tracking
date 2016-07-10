// g++ -I. -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -I../VOmniORBHelper -I../utility -I /usr/local/include/omniORB4 -o test_simple_client test_simple_client.cpp NET_SerialTracking_data.o NET_SerialTracking.o -L../utility -L../VOmniORBHelper -L. -L/usr/lib64/mysql -L/usr/local/lib -lutility -lVOmniORBHelper -lmysqlclient -lomniDynamic4 -lomniORB4 -lomniEvents -lZThread

#include<iostream>
#include<string>
#include<cstdlib>
#include<cmath>

#include<VOmniORBHelper.h>
#include<VSOptions.hpp>
#include<NET_SerialTracking.h>
#include<text.h>

using namespace VERITAS;
using namespace VCorba;

void usage(const std::string& progname, const VSOptions& options)
{
  std::cerr << "Usage: " << progname << " [options]" << std::endl;
  std::cerr << std::endl;
  std::cerr << "Options:" << std::endl;
  options.printUsage(std::cerr);
}


VOmniORBHelper* initOrb(const std::vector<std::string>& args,
			const std::string& nameserver, 
			int portnumber)
{
  int argc = args.size();
  char** argv = new char*[argc+1];
  static std::vector<char*> s_args(argc+1);

  for(int iarg=0;iarg<argc;iarg++)
    {
      s_args[iarg] = argv[iarg] = new char[args[iarg].length()+1];
      strcpy(argv[iarg], args[iarg].c_str());
    }
  s_args[argc] = argv[argc] = 0;
  
  VOmniORBHelper* orb = new VOmniORBHelper;

  //orb->orbSetProperty("clientCallTimeOutPeriod","1500");
  orb->orbInit(argc, argv, nameserver.c_str(), portnumber);
  delete[] argv;
  orb->setGlobalClientTimeout(1500);

  return orb;
}

int main(int argc, char** argv)
{
  std::string program(*argv);

  VSOptions options(argc,argv,true);

  bool                     print_usage(false);

  if(options.find("help","Print this message.")
     != VSOptions::FS_NOT_FOUND)print_usage = true;
  if(options.find("h","Print this message.")
     != VSOptions::FS_NOT_FOUND)print_usage = true;

  // -------------
  // CORBA OPTIONS
  // -------------

  std::vector<std::string> corba_args;
  int                      corba_port(-1);
  std::string              corba_nameserver("corbaname::localhost");
  int                      scope_num(0);

  options.findWithValue("corba_args", corba_args, 
			OPT_MAIN_CORBA_ARGS);
  
  options.findWithValue("corba_port", corba_port,
			OPT_MAIN_CORBA_PORT);
  
  options.findWithValue("corba_nameserver", corba_nameserver,
			OPT_MAIN_CORBA_NAMESERVER);

  options.findWithValue("scope", scope_num,
			"Set telescope number to test.");

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
  
  try
    {
      // ---------------
      // CONSTRUCT CORBA
      // ---------------
      VOmniORBHelper* orb(0);
      orb = initOrb(corba_args, corba_nameserver, corba_port);

      VSerialTracking::SimpleCommand_ptr sc(0);
      sc = orb->nsGetNarrowedObject<VSerialTracking::SimpleCommand>
	(VSerialTracking::progName, VSerialTracking::SimpleCommand::objName,
	 scope_num);

      // ---------
      // MAIN LOOP
      // ---------

      while(1)
	{
	  sc->nAlive();

          double mjd;
	  bool error;
	  bool interlock;
	  bool limits_hit;
	  bool is_stopped;
	  TargetObjectType target_type;
	  double scope_az_rad;
	  double scope_el_rad;
	  double target_az_rad;
	  double target_el_rad;
	  double target_ra_rad;
	  double target_dec_rad;
	  double slew_time_sec;

	  sc->nGetStatus(mjd,
			 error,
			 interlock,
			 limits_hit,
			 is_stopped,
			 target_type,
			 scope_az_rad, scope_el_rad,
			 target_az_rad, target_el_rad,
			 target_ra_rad, target_dec_rad,
			 slew_time_sec);
	  
	  std::cout << mjd << " : " 
		    << (error?'E':'-')
		    << (interlock?'I':'-')
		    << (limits_hit?'L':'-')
		    << (is_stopped?'S':'-') << " : "
		    << scope_az_rad/M_PI*180.0 << ' '
		    << scope_el_rad/M_PI*180.0 << " : ";

	  switch(target_type)
	    {
	    case TO_RADEC:
	      std::cout << "RADEC ";
	      break;
	    case TO_AZEL:
	      std::cout << "AZEL ";
	      break;
	    case TO_UNKNOWN:
	      std::cout << "UNKNOWN ";
	      break;
	    case TO_NULL:
	      std::cout << "NULL ";
	      break;
	    }

	  std::cout << target_az_rad/M_PI*180.0 << ' '
		    << target_el_rad/M_PI*180.0 << " : "
		    << target_ra_rad/M_PI*180.0 << ' '
		    << target_dec_rad/M_PI*180.0 << " : "
		    << slew_time_sec;

	  switch(target_type)
	    {
	    case TO_RADEC:
	      {
		double ra_rad;
		double dec_rad;
		double epoch;
		double offset_time_sidereal_min;
		double wobble_offset_rad;
		double wobble_direction_rad;
		CORBA::String_var name;
		bool use_convergent_pointing;

		sc->nGetRADecTargetDetails(ra_rad,
					   dec_rad,
					   epoch,
					   offset_time_sidereal_min,
					   wobble_offset_rad,
					   wobble_direction_rad,
					   name,
					   use_convergent_pointing);
		
		std::cout << " : " 
			  << ra_rad/M_PI*180.0 << ' '
			  << dec_rad/M_PI*180.0 << ' '
			  << epoch << ' '
			  << offset_time_sidereal_min << ' '
			  << wobble_offset_rad/M_PI*180.0 << ' '
			  << wobble_direction_rad/M_PI*180.0 << ' '
			  << name << ' '
			  << (use_convergent_pointing?'V':'-');
	      }
	      break;
	    case TO_AZEL:
	      {
                double az_rad;
		double el_rad;
		bool use_corrections;
		bool use_convergent_pointing;

		sc->nGetAzElTargetDetails(az_rad,
					  el_rad,
					  use_corrections,
					  use_convergent_pointing);
		std::cout << " : " 
			  << az_rad/M_PI*180.0 << ' '
			  << el_rad/M_PI*180.0 << ' '
			  << (use_corrections?'C':'-')
			  << (use_convergent_pointing?'V':'-');
	      }
	      break;
	    case TO_UNKNOWN:
	    case TO_NULL:
	      break;
	    }

	  std::cout << '\n';

	  sleep(1);
	}

      // --------
      // CLEAN UP
      // --------

      CORBA::release(sc);
      delete orb;
    }
  catch(const CORBA::SystemException& x)
    {
      std::cerr
	<< "Exception: " 
	<< x._name() << ' ' << x.NP_minorString() << std::endl;
      exit(EXIT_FAILURE);
    }
  catch(const CosNaming::NamingContext::NotFound)
    {
      std::cerr
	<< "CosNaming::NamingContext::NotFound" << std::endl;
      exit(EXIT_FAILURE);
    }
  catch(const CORBA::Exception& x)
    {
      std::cerr
	<< "Exception: " 
	<< x._name()  << std::endl;
      exit(EXIT_FAILURE);
    }
}

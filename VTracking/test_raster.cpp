//-*-mode:c++; mode:font-lock;-*-

/**
 * \file test_raster.cpp
 * \ingroup VTracking
 *
 * Raster scan example program
 *
 * Original Author: Stephen Fegan
 * $Author: aune $
 * $Date: 2010/09/09 18:56:11 $
 * $Revision: 2.4 $
 * $Tag$
 *
 **/

//g++ -I. -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -I../VOmniORBHelper -I../utility -I../../packages/omniORB4/include -o test_raster test_raster.cpp NET_TCInterface.o -L../utility -L../../packages/omniORB4/lib -L../../packages/ZThread/lib -L../VOmniORBHelper -L. -L/usr/lib64/mysql -L/usr/local/lib -lutility -lVOmniORBHelper -lmysqlclient -lomniDynamic4 -lomniORB4 -lCOSDynamic4 -lCOS4 -lZThread

// OLD
// g++ -I. -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -I../VOmniORBHelper -I../utility -I /usr/local/include/omniORB4 -o test_raster test_raster.cpp NET_TCInterface.o -L../utility -L../VOmniORBHelper -L. -L/usr/lib64/mysql -L/usr/local/lib -lutility -lVOmniORBHelper -lmysqlclient -lomniDynamic4 -lomniORB4 -lomniEvents -lZThread

#include<iostream>
#include<string>
#include<cstdlib>
#include<cmath>

#include<VOmniORBHelper.h>
#include<VSOptions.hpp>
#include<NET_TCInterface.h>
#include<text.h>

using namespace VERITAS;
using namespace VTracking;
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

  // --------------
  // RASTER OPTIONS
  // --------------

  double                   raster_res(0.05);  // Raster resolution in degrees
  double                   raster_side(0.5);  // Size of raster square in deg

  options.findWithValue("res", raster_res,
			"Set the resolution of the raster in degrees.");
  options.findWithValue("size", raster_side,
			"Set the size of the raster scan (length of the "
			"sides of the square in degrees).");

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
      usage(program, options);
      exit(EXIT_FAILURE);
    }

  if(print_usage)
    {
      usage(program, options);
      exit(EXIT_SUCCESS);
    }

  // -----------------------
  // CHECK RASTER PARAMETERS
  // -----------------------

  if(raster_res < 0)
    {
      std::cerr << program << ": resolution must be greater than zero.\n";
      exit(EXIT_FAILURE);
    }

  if(raster_side < 0)
    {
      std::cerr << program << ": raster size must be greater than zero.\n";
      exit(EXIT_FAILURE);
    }

  raster_res *= M_PI/180.0; // Tracking program uses radians
  raster_side *= M_PI/180.0;

  unsigned nscan = lround(raster_side/raster_res)+1;
  double scan_offset = -0.5*double(nscan-1)*raster_res;
  double scan_scale = raster_res;

  try
    {
      // ---------------
      // CONSTRUCT CORBA
      // ---------------

      VOmniORBHelper* orb(0);
      orb = initOrb(corba_args, corba_nameserver, corba_port);

      NET_TCInterface_ptr sc(0);
      sc = orb->nsGetNarrowedObject<NET_TCInterface>
	(NET_TCInterface::program_name, NET_TCInterface::object_name,
	 scope_num);


      // -----------------------------------------------
      // GET INITIAL PARAMETERS AND STATE FROM TELESCOPE
      // -----------------------------------------------

      // Initial corrections to modify in raster loop - don't for get to
      // reset the parameters at the end (or restart the tracking server
      // if you ctrl-c the raster loop)

      NET_TCMiscData_var md = sc->netGetMiscData();
      NET_CorrectionParameters cp = md->corrections;

      double initial_el_offset = cp.el_offset;
      double initial_fp_az = cp.fp_az;

      // User is expected to start the telescopes tracking some target, so
      // make sure they are doing that

      NET_StateElements_var se = sc->netGetTelescopeState();
      if(se->req != NET_REQ_TRACK)
	{
	  std::cerr << program 
	  << ": telescope should be tracking target before scanning starts.\n";
	  CORBA::release(sc);
	  delete orb;
	  exit(EXIT_FAILURE);
	}

      // ---------
      // MAIN LOOP
      // ---------

      for(unsigned iyscan=0;iyscan<nscan;iyscan++)
	{
	  cp.el_offset = 
	    initial_el_offset + scan_offset + double(iyscan)*scan_scale;

	  for(unsigned ixscan=0;ixscan<nscan;ixscan++)
	    {
	      // To save time go up one scan line and down the next
	      unsigned iix = ixscan;
	      if(iyscan%2==1)iix = nscan-ixscan-1;

	      cp.fp_az = 
		initial_fp_az + scan_offset + double(iix)*scan_scale;
	      
	      sc->netSetCorrectionParameters(cp);

	      // Wait some amount of time for telescope to reach target
	      // and data to be taken
	      sleep(/* 20 */ 1);

	      // Print some stuff out
	      NET_TCMiscData_var loop_md = sc->netGetMiscData();
	      NET_StateElements_var loop_se = sc->netGetTelescopeState();
	      std::cout
		<< iix << ' ' << iyscan << ' '
		<< (scan_offset+double(iix)*scan_scale)*180.0/M_PI << ' '
		<< (scan_offset+double(iyscan)*scan_scale)*180.0/M_PI << ' '
		<< loop_md->corrections.fp_az*180.0/M_PI << ' '
		<< loop_md->corrections.el_offset*180.0/M_PI << ' '
		<< loop_se->last_az_driveangle_deg << ' '
		<< loop_se->last_el_driveangle_deg << '\n';
	    }
	}

      // It is important to restore the original corrections when done. Also
      // if you kill the loop with ctrl-c then you should restart the tracking
      // server as the corrections will be wrong

      cp.el_offset = initial_el_offset;
      cp.fp_az = initial_fp_az;
      sc->netSetCorrectionParameters(cp);

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

//-*-mode:c++; mode:font-lock;-*-

/**
 * \file Main.cpp
 * \ingroup VTracking
 * \brief This is a one-line description of this cpp file.
 *
 * Here is a tedious verbose multi-line description of all the details
 * of the code, more than you would ever want to read. Generally, all
 * the important documentation goes in the .cpp files.
 *
 * Original Author: Stephen Fegan
 * $Author: aune $
 * $Date: 2010/01/05 21:36:10 $
 * $Revision: 2.30 $
 * $Tag$
 *
 **/

#include<Debug.h>
#include<Message.h>
#include<Messenger.h>
#include<QtNotification.h>

#include"Main.h"
#include"MainController.h"
#include"MainArrayGUI.h"
#include"MainEmulator.h"
#include"MainFailsafe.h"
#include"MainTerminate.h"

#include"text.h"

using namespace ZThread;
using namespace VERITAS;
using namespace VTracking;
using namespace VMessaging;
using namespace VTaskNotification;
using namespace VCorba;
using namespace SEphem;

// ============================================================================
// QT HELPER CLASS
// ============================================================================

QtHelper::TerminateNotification::~TerminateNotification()
{
  // nothing to see here
}

void QtHelper::TerminateNotification::doNotification()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  m_app->exit(0);
}

QtHelper::QtHelper(const std::vector<std::string>& args,
		   ZThread::Condition* terminate_notifier):
  QObject(), Runnable(), 
  m_args(args), m_terminate_notifier(terminate_notifier), m_mutex(),
  m_running(false)
{
  // nothing to see here
}

QtHelper::~QtHelper()
{
  // nothing to see here
}

void QtHelper::sendTerminateNotification()
{
  if(m_terminate_notifier)m_terminate_notifier->broadcast();  
}

void QtHelper::terminate()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mutex);
  if(m_running)
    QtNotificationList::getInstance()->
      scheduleNotification(new TerminateNotification(qApp));
}

void QtHelper::run()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mutex);

  int argc = m_args.size();
  char** argv = new char*[argc+1];
  std::vector<char*> args(argc+1);

  //std::cerr << __PRETTY_FUNCTION__ << ": A" << std::endl;

  for(int iarg=0;iarg<argc;iarg++)
    {
      args[iarg] = argv[iarg] = new char[m_args[iarg].length()+1];
      strcpy(argv[iarg], m_args[iarg].c_str());
    }
  args[argc] = argv[argc] = 0;
  
  //std::cerr << __PRETTY_FUNCTION__ << ": B" << std::endl;
  try
    {
      QApplication app(argc,argv);
  //std::cerr << __PRETTY_FUNCTION__ << ": C" << std::endl;

      app.connect(&app,SIGNAL(lastWindowClosed()),
		  this,SLOT(sendTerminateNotification()));
  //std::cerr << __PRETTY_FUNCTION__ << ": D" << std::endl;

      createWidgets(&app);
  //std::cerr << __PRETTY_FUNCTION__ << ": E" << std::endl;

      QtNotificationList::getInstance();
  //std::cerr << __PRETTY_FUNCTION__ << ": F" << std::endl;

      m_running=true;

      ZThread::Guard<RecursiveMutex,UnlockedScope> running_guard(m_mutex);
      Messenger::relay()->releaseMessages();
  //std::cerr << __PRETTY_FUNCTION__ << ": G" << std::endl;
      app.exec();
  //std::cerr << __PRETTY_FUNCTION__ << ": H" << std::endl; 
   }
  catch(const Exception& x)
    {
      x.print(Debug::stream());
    }
  catch(const Throwable& x)
    {
      Debug::stream() << "Caught Throwable" << std::endl;;
    }

  m_running=false;

  delete[] argv;
  for(std::vector<char*>::iterator iarg = args.begin(); iarg!=args.end();
      iarg++)delete[] *iarg;
  //std::cerr << __PRETTY_FUNCTION__ << ": I" << std::endl;
}

// ============================================================================
// CORBA HELPER
// ============================================================================

CorbaHelper::
CorbaHelper(VOmniORBHelper* orb, ZThread::Condition* terminate_notifier): 
  Runnable(), m_orb(orb), m_terminate_notifier(terminate_notifier),
  m_mutex(), m_running(false)
{
  // nothing to see here
}

CorbaHelper::~CorbaHelper()
{
  // nothing to see here
}

void CorbaHelper::terminate()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mutex);
  m_orb->orbShutdown(false);
}

void CorbaHelper::run()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mutex);
  createServants(m_orb);
  m_running=true;
  if(1)
    {
      ZThread::Guard<RecursiveMutex,UnlockedScope> running_guard(m_mutex);
      m_orb->orbRun();
    }
  m_running=false;
}

VOmniORBHelper* CorbaHelper::initOrb(const std::vector<std::string>& args,
				     const std::string& nameserver, 
				     int portnumber)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

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

void CorbaHelper::sendTerminateNotification()
{
  if(m_terminate_notifier)m_terminate_notifier->broadcast();  
}

// ============================================================================
// MAIN MEMBER VARIBALES AND FUNCTIONS
// ============================================================================

Main*                    Main::s_main(0);
std::string              Main::s_program;
Main::MainMode           Main::s_default_mainmode(Main::MM_CONTROLLER);
unsigned                 Main::s_default_verbose(0);
unsigned                 Main::s_default_scope_id(0);
std::string              Main::s_default_protocol("scope_default");
std::string              Main::s_default_datastream("scope_default");
unsigned                 Main::s_default_controller_iter_period_ms(0);
std::vector<std::string> Main::s_default_qt_args(0);
bool                     Main::s_default_no_qt(false);
bool                     Main::s_default_suppress_db(false);
std::vector<std::string> Main::s_default_corba_args(0);
int                      Main::s_default_corba_port(-1);
std::string              Main::s_default_corba_nameserver("corbaname::localhost");
std::string              Main::s_default_remote_ior("");
bool                     Main::s_default_remote_readonly(false);
bool                     Main::s_default_simple_interface_readonly(false);
bool                     Main::s_default_remote_no_auto_stopping(false);
bool                     Main::s_default_no_corba(false);
bool                     Main::s_default_no_grb(false);

Main* Main::getMain()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  switch(s_default_mainmode)
    {
    case MM_CONTROLLER:
      return new MainController;
    case MM_ARRAYGUI:
      return new MainArrayGUI;
    case MM_EMULATOR:
      return new MainEmulator;
    case MM_FAILSAFE:
      return new MainFailsafe;
    case MM_TERMINATE:
      return new MainTerminate;
    }
  assert(0);
}

Main::~Main()
{
  // nothing to see here
}

void Main::configure(VSOptions& options)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  s_program = options.arg0();

  // -------------
  // Verbose level
  // -------------
  if(options.find("v",OPT_MAIN_VERBOSE_1) != VSOptions::FS_NOT_FOUND)
    s_default_verbose=1;
  if(options.find("vv",OPT_MAIN_VERBOSE_2) != VSOptions::FS_NOT_FOUND)
    s_default_verbose=2;

  // ---------
  // Main Mode
  // ---------
  s_default_mainmode = MM_CONTROLLER;
  if(options.find("controller", OPT_MAIN_CONTROLLER)!= VSOptions::FS_NOT_FOUND)
    s_default_mainmode = MM_CONTROLLER;
  if(options.find("array", OPT_MAIN_ARRAY)!= VSOptions::FS_NOT_FOUND)
    s_default_mainmode = MM_ARRAYGUI;
  if(options.find("emulator", OPT_MAIN_EMULATOR) != VSOptions::FS_NOT_FOUND)
    s_default_mainmode = MM_EMULATOR;
  if(options.find("failsafe", OPT_MAIN_FAILSAFE) != VSOptions::FS_NOT_FOUND)
    s_default_mainmode = MM_FAILSAFE;
  if(options.find("terminate", OPT_MAIN_TERMINATE) != VSOptions::FS_NOT_FOUND)
    s_default_mainmode = MM_TERMINATE;

  // ------------
  // Telescope ID
  // ------------
  options.findWithValue("scope", s_default_scope_id, OPT_MAIN_SCOPE);

  // --------
  // Protocol
  // --------
  options.findWithValue("proto", s_default_protocol, OPT_MAIN_PROTO);

  // ----------
  // Datastream
  // ----------
  options.findWithValue("datastream", s_default_datastream, 
			OPT_MAIN_DATASTREAM);

  // ----------
  // Controller
  // ----------

  std::ostringstream cp_stream;
  cp_stream << OPT_MAIN_CONTROLLER_CONTROLLER_PERIOD_1
	    << DEFAULT_LOCAL_CONTROLLER_ITER_PERIOD
	    << OPT_MAIN_CONTROLLER_CONTROLLER_PERIOD_2
	    << DEFAULT_REMOTE_CONTROLLER_ITER_PERIOD
	    << OPT_MAIN_CONTROLLER_CONTROLLER_PERIOD_3;

  options.findWithValue("controller_period", 
			s_default_controller_iter_period_ms,
			cp_stream.str());

  // --
  // Qt
  // --
  options.findWithValue("qt_args", s_default_qt_args, OPT_MAIN_QT_ARGS);

  if(options.find("no_qt", OPT_MAIN_NO_QT) != VSOptions::FS_NOT_FOUND)
    s_default_no_qt = true;

  // --------
  // Database
  // --------
  if(options.find("no_db", OPT_MAIN_CONTROLLER_NO_DB) 
     != VSOptions::FS_NOT_FOUND)s_default_suppress_db = true;

  // -----
  // CORBA
  // -----
  options.findWithValue("corba_args", s_default_corba_args, 
			OPT_MAIN_CORBA_ARGS);
  
  options.findWithValue("corba_port", s_default_corba_port,
			OPT_MAIN_CORBA_PORT);
  
  options.findWithValue("corba_nameserver", s_default_corba_nameserver,
			OPT_MAIN_CORBA_NAMESERVER);

  if(options.find("no_corba",OPT_MAIN_NO_CORBA) != VSOptions::FS_NOT_FOUND)
    s_default_no_corba = true;

  options.findWithValue("corba_remote_ior", s_default_remote_ior,
			OPT_MAIN_CORBA_REMOTE_IOR);

  if(options.find("corba_readonly", OPT_MAIN_CONTROLLER_CORBA_READONLY) 
     != VSOptions::FS_NOT_FOUND)s_default_remote_readonly = true;

  if(options.find("corba_simple_interface_readonly", 
		  OPT_MAIN_CONTROLLER_CORBA_SIMPLE_INTERFACE_READONLY) 
     != VSOptions::FS_NOT_FOUND)s_default_simple_interface_readonly = true;

  if(options.find("corba_no_auto_stopping", 
		  OPT_MAIN_CONTROLLER_CORBA_NO_AUTO_STOPPING) 
     != VSOptions::FS_NOT_FOUND)s_default_remote_no_auto_stopping = true;

  // ---
  // GRB
  // ---

  if(options.find("no_grb",OPT_MAIN_NO_GRB) != VSOptions::FS_NOT_FOUND)
    s_default_no_grb = true;

  // ----------------------
  // Configure Mode Clients
  // ----------------------
  MainController::configure(options);
  MainArrayGUI::configure(options);
  MainTerminate::configure(options);
  MainEmulator::configure(options);
  MainFailsafe::configure(options);
}

void Main::
setTelescopeInfo(unsigned scope_id, const std::string& protocol_string,
		 const MainMode mainmode,
		 Protocol& protocol, std::string& datastream,
		 SphericalCoords& earth_pos,
		 StowObjectVector& stow_pos,
		 TelescopeMotionLimits*& limits, QColor& color,
		 bool daytime_limits, bool override_key, double dn_limit)
{
  // Test that telescope ID is valid
  switch(scope_id)
    {
    case 0:
    case 1:
    case 2:
    case 3:
    case 10:
      // good
      break;
    default:
      Debug::stream() 
	<< s_program << ": Unknown telescope ID " << scope_id
	<< std::endl;
      exit(EXIT_FAILURE);
    }

  // Set the protocol type
  if(protocol_string == "scope_default")
    {
      switch(scope_id)
	{
	case 0:
	  //protocol = P_PIU_PROTO;
	  protocol = P_PIU; // SJF 2006-02-28
	  break;
	case 1:
	case 2:
	case 3:
	  protocol = P_PIU;
	  break;
	case 10:
	  protocol = P_10M;
	  break;
	}
    }
  else if(protocol_string == "PIU")
    protocol = P_PIU;
  else if(protocol_string == "PIU_PROTO")
    protocol = P_PIU_PROTO;
  else if(protocol_string == "EIA")
    protocol = P_EIA;
  else if(protocol_string == "10M")
    protocol = P_10M;  
  else
    {
      Debug::stream()
	<< s_program << ": Unknown protocol type " 
	<< protocol_string << std::endl;
      exit(EXIT_FAILURE);
    }

  if(datastream == "scope_default")
    {
      if(mainmode == MM_EMULATOR)
	{
	  std::ostringstream datastream_stream;
	  switch(protocol)
	    {
	    case P_PIU:
	    case P_PIU_PROTO:
	      datastream_stream << "udp:0.0.0.0/" << 5000+scope_id;
	      break;
	    case P_EIA:
	    case P_10M:
	      datastream_stream << "unix:/tmp/emulator_socket" << scope_id;
	      break;
	    }
	  datastream = datastream_stream.str();
	}
      else
	{
	  bool set_protocol = false;

	  switch(protocol)
	    {
	    case P_PIU:
	      switch(scope_id)
		{
		case 0:
		  datastream = "udp:192.168.1.50/5000";
		  set_protocol = true;
		  break;
		case 1:
		case 2:
		case 3:
		  datastream = "udp:192.168.125.112/5000";
		  set_protocol = true;
		  break;
		}
	    case P_PIU_PROTO:
	      if(scope_id==0)
		{
		  datastream = "udp:192.168.1.50/5000";
		  set_protocol = true;
		}
	      break;
	    case P_EIA:
	      if(scope_id==0)
		{
		  datastream = "serial:/dev/ttyS0";
		  set_protocol = true;
		}
	      break;
	    case P_10M:
	      if(scope_id==10)
		{
		  datastream = "serial:/dev/ttyS0";
		  set_protocol = true;
		}
	      break;
	    }

	  if(!set_protocol)
	    {
	      Debug::stream() << s_program << ": ";
	      switch(protocol)
		{
		case P_PIU:
		  Debug::stream() << "PIU";
		  break;
		case P_PIU_PROTO:
		  Debug::stream() << "PIU (PROTOTYPE)";
		  break;
		case P_EIA:
		  Debug::stream() << "EIA";
		  break;
		case P_10M:
		  Debug::stream() << "10M";
		  break;
		}
	      
	      Debug::stream()
		<< " protocol specified with telescope "
		<< scope_id << std::endl
		<< "No default datastream known for this "
		<< "configuration" << std::endl;
	      exit(EXIT_FAILURE);
	    }
	}
    }

  double az_speed = 0.3;
  double el_speed = 0.3;

  // Position, max speed etc..
  Angle lat;
  Angle lon;
  switch(scope_id)
    {
    case 0:
      lat.setFromDMSString("31d40m29.04s");
      lon.setFromDMSString("-110d57m10.08s");
      az_speed = 1.0;
      el_speed = 1.0;
      stow_pos.push_back(StowObject("Stow",
				    SphericalCoords::makeLatLongDeg(7, 0.42)));
      stow_pos.push_back(StowObject("Platform",
				    SphericalCoords::makeLatLongDeg(0.6, 0.42)));
#if 0
      stow_pos.push_back(StowObject("Stow",
				    SphericalCoords::makeLatLongDeg(0,0)));
      stow_pos.push_back(StowObject("Platform",
				    SphericalCoords::makeLatLongDeg(1,15)));
#endif
      color.setHsv(240,30,230);
      break;
    case 1:
      lat.setFromDMSString("31d40m29.04s");
      lon.setFromDMSString("-110d57m10.08s");
      az_speed = 1.0;
      el_speed = 1.0;
      stow_pos.push_back(StowObject("Stow",
				    SphericalCoords::makeLatLongDeg(7,0)));
      stow_pos.push_back(StowObject("Platform",
				    SphericalCoords::makeLatLongDeg(0,-32)));
      color.setHsv(120,30,230);
      break;
    case 2:
      lat.setFromDMSString("31d40m29.04s");
      lon.setFromDMSString("-110d57m10.08s");
      az_speed = 1.0;
      el_speed = 1.0;
      stow_pos.push_back(StowObject("Stow",
				    SphericalCoords::makeLatLongDeg(7,1)));
      stow_pos.push_back(StowObject("Platform",
				    SphericalCoords::makeLatLongDeg(2,1)));
      color.setHsv(60,30,230);
      break;
    case 3:
      lat.setFromDMSString("31d40m29.04s");
      lon.setFromDMSString("-110d57m10.08s");
      az_speed = 1.0;
      el_speed = 1.0;
      stow_pos.push_back(StowObject("Stow",
				    SphericalCoords::makeLatLongDeg(7,-3.5)));
      stow_pos.push_back(StowObject("Platform",
				    SphericalCoords::makeLatLongDeg(1.5,-3.5)));
      color.setHsv(280,30,230);
      break;
    case 10:
      lat.setFromDMSString("31d40m49.7s");
      lon.setFromDMSString("-110d52m44.6s");
      az_speed = 1.0;
      el_speed = 1.0;
      stow_pos.push_back(StowObject("Stow",
				    SphericalCoords::makeLatLongDeg(0,0)));
      color.setHsv(0,30,230);
      break;
    };

  earth_pos = SphericalCoords::makeLatLong(lat,lon);

  // Set up the basic protections -- these cannot be overridden
  LimitsList* limlist = new LimitsList(az_speed, el_speed);
  limits = limlist;
  if(daytime_limits)
    limlist->pushFront(new PrimaryLimits(45,-45,-0.5,45,az_speed,el_speed));
  else
    limlist->pushFront(new PrimaryLimits(270,-270,-0.5,90,az_speed,el_speed));
  limlist->
    pushFront(new RestrictedLowElevationMotion(15,0.3,az_speed,el_speed));

  // The default dn_limit is actually chosen in MainController - this just
  // ensures that some crazy value has not been requested
  if(dn_limit<10)dn_limit=10;

  if(override_key)
    {
      switch(scope_id)
	{
	case 0:
	  break;
	case 1:
	  break;
	case 2:
	  break;
	case 3:
	  break;
	case 10:
	  break;
	}
    }
  else
    {
      std::vector<NotchedInclusionLimits::Notch> notches;
      switch(scope_id)
	{
	case 0:
	  notches.push_back(NotchedInclusionLimits::Notch(-0.5,0.5,-0.5));
	  notches.push_back(NotchedInclusionLimits::Notch(14.5,15.5,-0.5));
	  break;
	case 1:
	  notches.push_back(NotchedInclusionLimits::Notch(-32.5,-31.5,0));
	  notches.push_back(NotchedInclusionLimits::Notch(-0.5,0.5,-0.1));
	  break;
	case 2:
	  notches.push_back(NotchedInclusionLimits::Notch(0.5,1.5,1.5));
	  break;
	case 3:
	  notches.push_back(NotchedInclusionLimits::Notch(-4.0,-3.0,0.5));
	  break;
	case 10:
	  notches.push_back(NotchedInclusionLimits::Notch(-0.5,0.5,-0.5));
	  break;
	}
      limlist->
	pushFront(new NotchedInclusionLimits(notches, dn_limit, 
					     az_speed, el_speed));
    }
}


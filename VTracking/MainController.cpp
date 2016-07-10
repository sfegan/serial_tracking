//-*-mode:c++; mode:font-lock;-*-

/**
 * \file MainController.cpp
 * \ingroup VTracking
 * \brief This is a one-line description of this cpp file.
 *
 * Here is a tedious verbose multi-line description of all the details
 * of the code, more than you would ever want to read. Generally, all
 * the important documentation goes in the .cpp files.
 *
 * Original Author: Stephen Fegan
 * $Author: sfegan $
 * $Date: 2007/07/19 04:35:12 $
 * $Revision: 2.14 $
 * $Tag$
 *
 **/

#include<qstylefactory.h>

#include<fcntl.h>

#include<Message.h>
#include<Messenger.h>
#include<StreamMessenger.h>
#include<NET_Messenger.h>
#include<NET_MessageReceiverPolling.h>
#include<Daemon.h>

#include"GUI.h"
#include"ScopeAPI.h"
#include"EIA422.h"
#include"PIUScopeAPI.h"
#include"PositionLogger.h"
#include"TelescopeControllerLocal.h"
#include"TelescopeControllerRemote.h"
#include"GUICorrectionDialogs.h"

#include"text.h"
#include"Main.h"
#include"MainController.h"

using namespace ZThread;
using namespace VERITAS;
using namespace VTracking;
using namespace VMessaging;
using namespace VTaskNotification;
using namespace VCorba;

// ============================================================================
// MAIN CONTROLLER QT HELPER CLASS
// ============================================================================

QtHelperMainController::
QtHelperMainController(int scope_num, TelescopeController* controller,
		       const SEphem::SphericalCoords& pos, 
		       const StowObjectVector& stow_pos,
		       int protection_level, bool security_override,
		       const QColor& color,
		       const std::vector<std::string>& args,
		       ZThread::Condition* terminate_notifier):
  QtHelper(args, terminate_notifier),
  m_scope_num(scope_num), m_controller(controller), m_pos(pos),
  m_stow_pos(stow_pos), 
  m_protection_level(protection_level), 
  m_security_override(security_override), m_color(color)
{
  // nothing to see here
}

QtHelperMainController::~QtHelperMainController()
{
  // nothing to see here
}

void QtHelperMainController::createWidgets(QApplication* app)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  QStyle* style = QStyleFactory::create("cde");
  if(!style)style = QStyleFactory::create("windows");
  if(style)QApplication::setStyle(style);
  QApplication::setPalette(QPalette(m_color,m_color));
  
  GUIWidget* gui = 
    new GUIWidget(m_scope_num, m_controller, m_pos, m_stow_pos,
		  m_protection_level, m_security_override);
  gui->show();
}

// ============================================================================
// MAIN CONTROLLER CORBA HELPER
// ============================================================================

CorbaHelperTCInterface::
CorbaHelperTCInterface(TelescopeController* controller,
		       const StowObjectVector& stow_pos,
		       int scope_num, VOmniORBHelper* orb,
		       ZThread::Condition* terminate_notifier,
		       bool readonly, bool simple_interface_readonly):
  CorbaHelper(orb, terminate_notifier),
  m_controller(controller), m_stow_pos(stow_pos),
  m_scope_num(scope_num), m_servant(), m_simple_interface_servant(),
  m_readonly(readonly), m_simple_interface_readonly(simple_interface_readonly)
{
  // nothing to see here
}

CorbaHelperTCInterface::~CorbaHelperTCInterface()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(m_servant)delete m_servant;
}

void CorbaHelperTCInterface::createServants(VOmniORBHelper* orb)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  CORBA::Object_var net_controller;
  CORBA::Object_var net_simple_interface_controller;
  
  try
    {
      m_servant = new NET_TCInterfaceServant(m_controller, m_stow_pos,
					     m_readonly, m_terminate_notifier);
      net_controller =
	orb->poaActivateObject(m_servant, 
			       NET_TCInterface::program_name,
			       NET_TCInterface::object_name,
			       m_scope_num);

      m_simple_interface_servant = 
	new NET_SerialTrackingServant(m_controller, m_stow_pos, 
				      m_readonly||m_simple_interface_readonly,
				      m_terminate_notifier);

      net_simple_interface_controller =
	orb->poaActivateObject(m_simple_interface_servant, 
			       VSerialTracking::progName,
			       VSerialTracking::SimpleCommand::objName,
			       m_scope_num);
    }
  catch(const CORBA::SystemException& x)
    {
      Message message(Message::DR_LOCAL,Message::PS_CRITICAL,
		      "CORBA servant activation error");
      message.messageStream() 
	<< "CORBA exception occurred while activating servant. Program will" 
	<< std::endl
	<< "halt. Please fix problem or re-run with \"-no_corba\" to disable "
	<< std::endl
	<< "CORBA server." << std::endl << std::endl
	<< "Exception: " 
	<< x._name() << ' ' << x.NP_minorString() << std::endl;
      Messenger::relay()->sendMessage(message);
      sendTerminateNotification();
      return;
    }
  
  try
    {
      orb->nsRegisterObject(net_controller, 
			    NET_TCInterface::program_name,
			    NET_TCInterface::object_name,
			    m_scope_num);

      orb->nsRegisterObject(net_simple_interface_controller, 
			    VSerialTracking::progName,
			    VSerialTracking::SimpleCommand::objName,
			    m_scope_num);
    }
  catch(const CORBA::SystemException& x)
    {
      Message message(Message::DR_LOCAL,Message::PS_CRITICAL,
		      "CORBA servant activation error");
      message.messageStream() 
	<< "CORBA exception occurred while registering servant with the "
	<< std::endl
	<< "nameserver: " << orb->nsGetNameserverIOR() << std::endl 
	<< std::endl
	<< "Program will halt. Please fix problem or re-run with "
	<< std::endl
	<< "\"-no_corba\" to disable CORBA server." << std::endl << std::endl
	<< "Exception: " 
	<< x._name() << ' ' << x.NP_minorString() << std::endl;
      Messenger::relay()->sendMessage(message);
      sendTerminateNotification();
      return;
    }

  try
    {
      CORBA::ORB_var the_orb = orb->orb();
      CORBA::String_var ref = the_orb->object_to_string(net_controller);
      CORBA::String_var ref_simple_interface
	= the_orb->object_to_string(net_simple_interface_controller);

      Message message(Message::DR_LOCAL,Message::PS_ROUTINE,
		      "Activated CORBA servant");
      message.messageStream() 
	<< "Tracking control servant has been activated and registed with "
	<< std::endl
	<< "the CORBA nameserver."
	<< std::endl
	<< ref;
      Messenger::relay()->sendMessage(message);
      
      Message message2(Message::DR_LOCAL,Message::PS_ROUTINE,
		      "Activated CORBA simple interface servant");
      message2.messageStream() 
	<<"Servant for simple interface to tracking control has been\n"
	<< "activated and registed with the CORBA nameserver."
	<< std::endl
	<< ref;
      Messenger::relay()->sendMessage(message2);
    }
  catch(const CORBA::SystemException& x)
    {
      // talk to someone who cares
    }
}

// ============================================================================
// CONTROLLER MEMBER VARIBALES AND FUNCTIONS
// ============================================================================

double       MainController::s_default_dn_limit(20.0);
bool         MainController::s_default_override_security(false);
bool         MainController::s_default_override_key(false);
std::string  MainController::s_default_record_file("");
unsigned     MainController::s_default_logger_timeout_sec(30*60);
bool         MainController::s_default_daytime_limits(false);
std::string  MainController::s_default_controller("local");
unsigned     MainController::s_default_db_log_period_iter(1);
unsigned     MainController::s_default_db_cache_size(10);
bool         MainController::s_default_daemon(false);
std::string  MainController::s_default_log_file("");

bool         MainController::s_realtime(false);

void MainController::configure(VSOptions& options)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  // --------------
  // Downward Limit
  // --------------
#if 0
  options.findWithValue("dn_limit", s_default_dn_limit,
			OPT_MAIN_CONTROLLER_DN_LIMIT);
#endif

  // --------------
  // Daytime Limits
  // --------------
  if(options.find("daytime", OPT_MAIN_CONTROLLER_DAYTIME) 
     != VSOptions::FS_NOT_FOUND)s_default_daytime_limits = true;

  // ------------
  // Override Key
  // ------------
#if 0
  if(options.find("override_key", OPT_MAIN_CONTROLLER_OVERRIDE_KEY) 
     != VSOptions::FS_NOT_FOUND)s_default_override_key = true;
#endif

  // -----------------
  // Override Security
  // -----------------
  if(options.find("override_security", OPT_MAIN_CONTROLLER_OVERRIDE_SECURITY)
     != VSOptions::FS_NOT_FOUND)s_default_override_security = true;

  // ---------------
  // Record Filename
  // ---------------
  options.findWithValue("record", s_default_record_file,
			OPT_MAIN_CONTROLLER_RECORD);

  // --------------
  // Logger Timeout
  // --------------
  if(!s_default_record_file.empty())s_default_logger_timeout_sec=0;
  options.findWithValue("logger_timeout", s_default_logger_timeout_sec,
			OPT_MAIN_CONTROLLER_LOGGER_TIMEOUT);

  // --------
  // Database
  // --------
  options.findWithValue("db_log_period", s_default_db_log_period_iter,
			OPT_MAIN_CONTROLLER_DB_LOG_PERIOD);

  options.findWithValue("db_cache_size", s_default_db_cache_size,
			OPT_MAIN_CONTROLLER_DB_CACHE_SIZE);

  // ----------
  // Controller
  // ----------
  options.findWithValue("controller", s_default_controller,
			OPT_MAIN_CONTROLLER_CONTROLLER);

  // ------
  // Daemon
  // ------
  if(options.find("daemon",OPT_MAIN_CONTROLLER_DAEMON) 
     != VSOptions::FS_NOT_FOUND)s_default_daemon = true;

  options.findWithValue("log_file", s_default_log_file,
			OPT_MAIN_CONTROLLER_LOG_FILE);
}

MainController::MainController():
  Main(),
  m_verbose(s_default_verbose),
  m_scope_id(s_default_scope_id),
  m_protocol(P_PIU),
  m_datastream(s_default_datastream),
  m_override_security(s_default_override_security),
  m_record_file(s_default_record_file),
  m_logger_timeout_sec(s_default_logger_timeout_sec),
  m_suppress_db(s_default_suppress_db),
  m_qt_args(s_default_qt_args),
  m_earth_pos(), m_stow_pos(), m_limits(0), 
  m_controller(C_LOCAL), 
  m_no_qt(s_default_no_qt), m_corba_args(s_default_corba_args),
  m_corba_port(s_default_corba_port), 
  m_corba_nameserver(s_default_corba_nameserver), 
  m_no_corba(s_default_no_corba), m_remote_ior(s_default_remote_ior),
  m_remote_readonly(s_default_remote_readonly), 
  m_simple_interface_readonly(s_default_simple_interface_readonly), 
  m_remote_no_auto_stopping(s_default_remote_no_auto_stopping),
  m_controller_iter_period_ms(s_default_controller_iter_period_ms),
  m_db_log_period_iter(s_default_db_log_period_iter),
  m_db_cache_size(s_default_db_cache_size),
  m_daemon(s_default_daemon), m_log_file(s_default_log_file),
  m_color()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  if(m_db_log_period_iter==0)m_suppress_db=true;

  setTelescopeInfo(m_scope_id, s_default_protocol, s_default_mainmode,
		   m_protocol, m_datastream, m_earth_pos, m_stow_pos,
		   m_limits, m_color, s_default_daytime_limits,
		   s_default_override_key, s_default_dn_limit);
  m_qt_args.insert(m_qt_args.begin(), s_program);
  
  if(s_default_controller == "local")m_controller = C_LOCAL;
  else if(s_default_controller == "remote")m_controller = C_REMOTE;
  else
    {
      std::cerr << "Unknown value for controller mode: " 
		<< s_default_controller << std::endl;
      exit(EXIT_FAILURE);
    }

  if(m_controller_iter_period_ms==0)
    if(m_controller == C_LOCAL)
      m_controller_iter_period_ms=DEFAULT_LOCAL_CONTROLLER_ITER_PERIOD;
    else 
      m_controller_iter_period_ms=DEFAULT_REMOTE_CONTROLLER_ITER_PERIOD;
}

MainController::~MainController()
{
  delete m_limits;
}

int MainController::main(int argc, char** argv)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  
  // --------------------------------------------------------------------------
  // Set up the primary message logger
  // --------------------------------------------------------------------------

  std::ofstream* log_stream = 0;
  StreamMessenger* log_messenger = 0;
  if((m_daemon)||(!m_log_file.empty()))
    {
      if(m_log_file.empty())
	{
	  time_t t = time(0);
	  struct tm* tm;
	  tm = gmtime(&t);
	  std::ostringstream stream;
	  stream << "/tmp/serial_tracking_t" << m_scope_id+1 << '_' 
		 << std::setw(2) << std::setfill('0') << tm->tm_year%100 
		 << std::setw(2) << std::setfill('0') << tm->tm_mon+1
		 << std::setw(2) << std::setfill('0') << tm->tm_mday << ".log";
	  m_log_file = stream.str();
	}

      log_stream = new std::ofstream(m_log_file.c_str(), std::ios_base::app);
      if(!*log_stream)
	{
	  std::cerr << "Could not open log file: " << m_log_file 
		    << std::endl;
	  exit(EXIT_FAILURE);
	}

      Debug::setStream(log_stream);
      log_messenger = new StreamMessenger(*log_stream);
    }
  else
    {
      log_messenger = new StreamMessenger;
    }

  // --------------------------------------------------------------------------
  // Sanity check. There must be some way to issue commands to the controller
  // --------------------------------------------------------------------------

  Messenger::relay()->registerMessenger(log_messenger);
  BackTrace::catchSignalPrintBacktraceAndAbort(SIGSEGV);

  if(m_no_qt && (m_no_corba || m_remote_readonly))
    {
      Message message(Message::DR_LOCAL,Message::PS_CRITICAL,
		      "No command interface");
      message.messageStream() 
	<< "Qt and CORBA interfaces are both disabled, there is no way for "
	<< std::endl
	<< "the controller to recieve commands. There is no point in "
	<< std::endl
	<< "continuing.";
      Messenger::relay()->sendMessage(message);
      exit(EXIT_FAILURE);
    }

  Messenger::relay()->holdMessages();

  // --------------------------------------------------------------------------
  // Start DAEMON if requested
  // --------------------------------------------------------------------------

  if(m_daemon)
    {
      if(Daemon::daemon_init("",false) == -1)
	{
	  std::cerr << "Could not fork daemon process: " << strerror(errno)
		    << std::endl;
	  exit(EXIT_FAILURE);
	}

      Message message(Message::DR_LOCAL,Message::PS_ROUTINE,
		      "Daemon startup");
      message.messageStream() 
	<< "Daemon is stating" << std::endl;
      log_messenger->sendMessage(message);
    }

  // --------------------------------------------------------------------------
  // If we are to run as a LOCAL controller then set up lock file
  // --------------------------------------------------------------------------

  int lock_fd = -1;
  if(m_controller == C_LOCAL)
    {
      std::string lockfile = m_datastream;
      unsigned nchar=lockfile.length();
      for(unsigned ichar=0;ichar<nchar;ichar++)
	if(lockfile[ichar]=='/')lockfile[ichar]='_';
      lockfile = 
	std::string("/tmp/serial_tracking_")+lockfile+std::string(".lock");
      
      lock_fd = open(lockfile.c_str(),O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR);
      if(lock_fd<0)
        {
	  Message message(Message::DR_GLOBAL,Message::PS_EXCEPTIONAL,
			  "Lockfile error");
	  message.messageStream()
	    << "Could not open lock file " << lockfile << std::endl
	    << "This may cause problems if another controller is started but is not serious" << std::endl
	    << "enough to require terminating the program." << std::endl;
	  message.detailsStream() 
	    << strerror(errno) << std::endl;
	  Messenger::relay()->sendMessage(message);
	}
      else
	{
	  struct flock lock;
	  lock.l_type   = F_WRLCK;
	  lock.l_whence = SEEK_SET;
	  lock.l_start  = 0;
	  lock.l_len    = 0;
	  lock.l_pid    = 0;

	  if(fcntl(lock_fd, F_SETLK, &lock)<0)
	    {
	      if((errno==EACCES)||(errno==EAGAIN))
		{
		  Message message(Message::DR_GLOBAL,Message::PS_CRITICAL,
				  "Datastream is locked");
		  message.messageStream() 
		    << "The telescope datastream is locked by another process. The program" << std::endl
		    << "will terminate. To override this lock, terminate the other controller" << std::endl
		    << "or remove the lockfile." << std::endl;
		  message.detailsStream() 
		    << "Datastream: " << m_datastream << std::endl
		    << "Lockfile:   " << lockfile << std::endl;
		  log_messenger->sendMessage(message);
		  exit(EXIT_FAILURE);
		}
	      else
		{
		  Message message(Message::DR_GLOBAL,Message::PS_EXCEPTIONAL,
				  "Locking error");
		  message.messageStream() 
		    << "An error occurred while locking " << lockfile << std::endl
		    << "This may cause problems if another controller is started but is not serious" << std::endl
		    << "enough to require terminating the program." << std::endl;
		  message.detailsStream() 
		    << strerror(errno) << std::endl;
		  Messenger::relay()->sendMessage(message);
		  close(lock_fd);
		  lock_fd = -1;
		}
	    }
	  
	  Message message(Message::DR_GLOBAL,Message::PS_ROUTINE,
			  "Locking successful");
	  message.messageStream() 
	    << "Lock file: " << lockfile << std::endl;
	  Messenger::relay()->sendMessage(message);
        }
    }
  
  // --------------------------------------------------------------------------
  // Create all the objects and threads needed
  // --------------------------------------------------------------------------

  // This mutex and condition variable allows the various threads signal
  // that the program should be terminated.
  ZThread::RecursiveMutex tn_mutex;
  ZThread::Condition tn_condition(tn_mutex);
  ZThread::Guard<ZThread::RecursiveMutex> guard(tn_mutex);
  
  try
    {
      // For LOCAL controller
      DataStream* datastream(0);
      EIA422Stub* stub(0);
      ScopeAPI* scopeapi(0);
      MultiPositionLogger* logger(0);
      VTaskNotification::TaskList* dblog_tasklist(0);

      TelescopeController* controller(0);
      QtHelper* qt(0);
      VOmniORBHelper* orb(0);
      CorbaHelperTCInterface* corbaserver(0);

      // ----------------------------------------------------------------------
      // Initialize the ORB if appropriate
      // ----------------------------------------------------------------------

      if((m_controller == C_REMOTE)||(!m_no_corba))
	orb = CorbaHelperTCInterface::
	  initOrb(m_corba_args, m_corba_nameserver, m_corba_port);

      // ----------------------------------------------------------------------
      // Start the CORBA message sender/receiver if appropriate
      // ----------------------------------------------------------------------
      
      NET_Messenger* event_messenger(0);
      NET_MessageReceiverPolling* event_polling(0);
      if((m_controller == C_LOCAL)&&(!m_no_corba))
	{
	  event_messenger = 
	    new NET_Messenger(orb, NET_TCInterface::program_name,
			      NET_Messenger::object_name, m_scope_id);
	  Messenger::relay()->registerMessenger(event_messenger);
	}
      else if(m_controller == C_REMOTE)
	{
	  event_polling =
	    new NET_MessageReceiverPolling(orb, Messenger::relay(), 100,
					   NET_TCInterface::program_name,
					   NET_Messenger::object_name, 
					   m_scope_id);
	  event_polling->addToZoneFilter(m_scope_id);
	}
	  
      // ----------------------------------------------------------------------
      // Create the controller - LOCAL or REMOTE
      // ----------------------------------------------------------------------

      if(m_controller == C_LOCAL)
	{
	  // ------------------------------------------------------------------
	  // Datastream
	  // ------------------------------------------------------------------

	  datastream = DataStream::makeDataStream(m_datastream,m_verbose);

	  switch(m_protocol)
	    {
	    case P_PIU:
	      scopeapi = new PIUScopeAPI(PIUScopeAPI::PV_ARRAY_050901,
					 datastream);
	      break;
	    case P_PIU_PROTO:
	      scopeapi = new PIUScopeAPI(PIUScopeAPI::PV_PROTOTYPE,
					 datastream);
	      break;
	    case P_EIA:
	      stub = new EIA422Stub(datastream);
	      scopeapi = new ScopeAPIToEIA422Adaptor(stub);
	    case P_10M:
	      std::cerr << "10m protocol unsupported at this time" 
			<< std::endl;
	      assert(0);
	    }
      
	  // ------------------------------------------------------------------
	  // Load telescope correction parameters
	  // ------------------------------------------------------------------

	  CorrectionParameters tcp;
#if 0
	  std::string tcp_filename = 
	    CorrectionParameters::loadFilename(m_scope_id);
	  if(!tcp.load(tcp_filename.c_str()))
	    {
	      Message message(Message::DR_GLOBAL,Message::PS_UNUSUAL,
			      "Load corrections");
	      message.messageStream() 
		<< "Could not load default tracking corrections" << std::endl
		<< "from file: " << tcp_filename;
	      Messenger::relay()->sendMessage(message);
	    }
#else
	  if(!GUICorrectionDialogs::loadDefault(tcp, m_scope_id))
	    {
	      Message message(Message::DR_GLOBAL,Message::PS_UNUSUAL,
			      "Load corrections");
	      message.messageStream() 
		<< "Could not load default tracking corrections." << std::endl;
	      Messenger::relay()->sendMessage(message);
	    }
#endif

	  // ------------------------------------------------------------------
	  // Database and file logging if appropriate
	  // ------------------------------------------------------------------

	  if((!m_suppress_db)||(!m_record_file.empty()))
	    {
	      logger = new MultiPositionLogger();
	      if(!m_suppress_db)
		{
		  dblog_tasklist = new VTaskNotification::TaskList(1);
		  logger->
		    addLogger(new DBPositionLogger(dblog_tasklist,m_scope_id,
						   m_db_log_period_iter,
						   m_db_cache_size));
		}

	      if(!m_record_file.empty())
		logger->addLogger(new FilePositionLogger(m_record_file));
	    }

	  // ------------------------------------------------------------------
	  // Local controller
	  // ------------------------------------------------------------------

	  controller = 
	    new TelescopeControllerLocal(scopeapi, m_limits, 
					 m_earth_pos, 
					 m_controller_iter_period_ms, 0,
					 tcp, logger, m_logger_timeout_sec,
					 &tn_condition,
					 s_realtime);
	}
      else
	{
	  // ------------------------------------------------------------------
	  // Remote controller
	  // ------------------------------------------------------------------

	  if(m_remote_ior.empty())
	    controller = 
	      new TelescopeControllerRemote(orb, m_scope_id, 
					    m_controller_iter_period_ms,
					    m_controller_iter_period_ms/5,
					    m_remote_readonly,
					    m_remote_no_auto_stopping,
					    m_earth_pos);
	  else
	    controller = 
	      new TelescopeControllerRemote(orb, m_remote_ior, 
					    m_controller_iter_period_ms,
					    m_controller_iter_period_ms/25,
					    m_remote_readonly,
					    m_remote_no_auto_stopping,
					    m_earth_pos);
	}

      // ----------------------------------------------------------------------
      // QT helper -- thread to set up widgets and runs QT event loop
      // ----------------------------------------------------------------------
      
      if(!m_no_qt)
	qt = new QtHelperMainController(m_scope_id, controller, m_earth_pos, 
					m_stow_pos,
					m_override_security?0:5, 
					m_override_security, m_color,
					m_qt_args, &tn_condition);

      // ----------------------------------------------------------------------
      // CORBA helper - create servants and run omniORB main loop
      // ----------------------------------------------------------------------

      if((m_controller == C_LOCAL)&&(!m_no_corba))
	corbaserver = new CorbaHelperTCInterface(controller, 
						 m_stow_pos,
						 m_scope_id, orb, 
						 &tn_condition,
						 m_remote_readonly,
						 m_simple_interface_readonly);
      
      // ----------------------------------------------------------------------
      // Threads
      // ----------------------------------------------------------------------

      ZThread::Thread* event_polling_thread(0);
      ZThread::Thread* controller_thread(0);
      ZThread::Thread* qt_thread(0);
      ZThread::Thread* corba_thread(0);

      if(m_controller == C_REMOTE)
	event_polling_thread = new ZThread::Thread(event_polling);
      controller_thread = new ZThread::Thread(controller);
      if(!m_no_qt)qt_thread = new ZThread::Thread(qt);
      if((m_controller == C_LOCAL)&&(!m_no_corba))
	corba_thread = new ZThread::Thread(corbaserver);

      // ----------------------------------------------------------------------
      // If we are not starting QT then release messages.. otherwise QT class
      // does it when it is ready to display them
      // ----------------------------------------------------------------------

      if(m_no_qt)Messenger::relay()->releaseMessages();

      // ----------------------------------------------------------------------
      // Wait on condition variable until we are signaled to shut down
      // ----------------------------------------------------------------------

      tn_condition.wait();

      // ----------------------------------------------------------------------
      // Terminate the QT main loop and wait for QT helper thread
      // ----------------------------------------------------------------------

      if(!m_no_qt)
	{
	  qt->terminate();
	  qt_thread->wait();
	  delete qt_thread;
	}

      // ----------------------------------------------------------------------
      // Delete CORBA messenger, terminate CORBA loop, wait for helper thread
      // ----------------------------------------------------------------------

      if((m_controller == C_LOCAL)&&(!m_no_corba))
	{
	  Messenger::relay()->unRegisterMessenger(event_messenger);
	  delete event_messenger;
	  corbaserver->terminate();
	  corba_thread->wait();
	  delete corba_thread;
	}

      // ----------------------------------------------------------------------
      // Stop the polling message thread and wait for it to terminate
      // ----------------------------------------------------------------------

      if(m_controller == C_REMOTE)
	{
	  event_polling->terminate();
	  event_polling_thread->wait();
	  delete event_polling_thread;
	}

      // ----------------------------------------------------------------------
      // Stop the controller and wait for the thread
      // ----------------------------------------------------------------------

      controller->terminate();
      controller_thread->wait();
      delete controller_thread;

      // ----------------------------------------------------------------------
      // Delete the logger and stop the TASK processor
      // ----------------------------------------------------------------------

      if(logger)delete logger;
      if(dblog_tasklist)
	{
	  dblog_tasklist->stopProcessing();
	  delete dblog_tasklist;
	}      

      // DO NOT DELETE THESE OBJECTS -- ZTHREADS DELETED THEM
      //delete controller;
      //delete qt;

      // ----------------------------------------------------------------------
      // Delete the ORB
      // ----------------------------------------------------------------------

      if(orb)delete orb;

      // ----------------------------------------------------------------------
      // Delete local datastream/scope API objects
      // ----------------------------------------------------------------------

      // One final standby command for luck
      if(scopeapi)scopeapi->cmdStandby();
      delete scopeapi;
      delete stub;
      delete datastream;
    }
  catch(const Exception& x)
    {
      Message message(x,Message::DR_LOCAL,Message::PS_EXCEPTIONAL);
      Messenger::relay()->sendMessage(message);
    }
  catch(const Throwable& x)
    {
      Message message(Message::DR_LOCAL,Message::PS_EXCEPTIONAL,
		      "Throwable");
      message.messageStream() << "Caught Throwable";
      Messenger::relay()->sendMessage(message);
    }
  catch(const CORBA::SystemException& x)
    {
      Message message(Message::DR_LOCAL,Message::PS_EXCEPTIONAL,
		      "CORBA system exception");
      message.messageStream() 
	<< "Caught CORBA exception" << std::endl
	<< x._name() << ' ' << x.NP_minorString() << std::endl;
      Messenger::relay()->sendMessage(message);
    }

  if(m_daemon)
    {
      Message message(Message::DR_LOCAL,Message::PS_ROUTINE,
		      "Daemon shutdown");
      message.messageStream() 
	<< "Daemon is shutting down" << std::endl;
      log_messenger->sendMessage(message);
    }

  Messenger::relay()->unRegisterMessenger(log_messenger);
  delete log_messenger;
  if(log_stream)Debug::setStream(std::cout);
  delete log_stream;

  return EXIT_SUCCESS;
}

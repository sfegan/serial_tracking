//-*-mode:c++; mode:font-lock;-*-

/**
 * \file MainArrayGUI.cpp
 * \ingroup VTracking
 * \brief Main class to set up and start array GUI
 *
 * Original Author: Stephen Fegan
 * Start Date: 2006-07-15
 * $Author: sfegan $
 * $Date: 2007/03/19 17:19:30 $
 * $Revision: 2.9 $
 * $Tag$
 *
 **/

#include<qstylefactory.h>

#include<Message.h>
#include<Messenger.h>
#include<NET_Messenger.h>
#include<StreamMessenger.h>
#include<NET_MessageReceiverPolling.h>

#include"GUIArray.h"
#include"ScopeAPI.h"
#include"TelescopeControllerRemote.h"

#include"text.h"
#include"Main.h"
#include"MainArrayGUI.h"

using namespace ZThread;
using namespace VERITAS;
using namespace VTracking;
using namespace VMessaging;
using namespace VTaskNotification;
using namespace VCorba;

// ============================================================================
// MAIN CONTROLLER QT HELPER CLASS
// ============================================================================

QtHelperMainArrayGUI::
QtHelperMainArrayGUI(std::vector<TelescopeController*>& controller_vec,
		     VCorba::VOmniORBHelper* orb,
		     const std::vector<QColor>& colors,
		     const SEphem::SphericalCoords& mean_earth_pos, 
		const std::vector<VTracking::StowObjectVector>& scope_stow_pos,
		     std::vector<unsigned> suppress_servo_fail,
		     const std::vector<std::string>& args, bool grb,
		     unsigned theme, ZThread::Condition* terminate_notifier):
  QtHelper(args, terminate_notifier),
  m_controller_vec(controller_vec), m_orb(orb), m_colors(colors),
  m_mean_earth_position(mean_earth_pos),
  m_scope_stow_pos(scope_stow_pos), m_suppress_servo_fail(suppress_servo_fail),
  m_grb(grb), m_theme(theme)
{
  // nothing to see here
}

QtHelperMainArrayGUI::~QtHelperMainArrayGUI()
{
  // nothing to see here
}

void QtHelperMainArrayGUI::createWidgets(QApplication* app)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  //QStyle* style = QStyleFactory::create("cde");
  //if(!style)style = QStyleFactory::create("windows");
  //if(style)QApplication::setStyle(style);
  
  GUIArrayWidget* gui = 
    new GUIArrayWidget(m_controller_vec, m_orb, m_colors,
		       m_mean_earth_position, m_scope_stow_pos, 
		       m_suppress_servo_fail, m_grb, m_theme);
  gui->show();
}

// ==========================================================================
// MAIN ARRAY GUI CLASS
// ==========================================================================

unsigned MainArrayGUI::s_default_theme = 0;
std::vector<unsigned> MainArrayGUI::s_default_suppress_servo_fail;

MainArrayGUI::MainArrayGUI():
  Main(),
  m_theme(s_default_theme), 
  m_suppress_servo_fail(s_default_suppress_servo_fail),
  m_suppress_db(s_default_suppress_db), m_qt_args(s_default_qt_args),
  m_corba_args(s_default_corba_args), m_corba_port(s_default_corba_port), 
  m_corba_nameserver(s_default_corba_nameserver),
  m_remote_readonly(s_default_remote_readonly), 
  m_remote_no_auto_stopping(s_default_remote_no_auto_stopping),
  m_controller_iter_period_ms(s_default_controller_iter_period_ms),
  m_no_grb(s_default_no_grb)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  m_qt_args.insert(m_qt_args.begin(), s_program);
  if(m_controller_iter_period_ms==0)
    m_controller_iter_period_ms=DEFAULT_REMOTE_CONTROLLER_ITER_PERIOD;
}

MainArrayGUI::~MainArrayGUI()
{
  // nothing to see here
}

int MainArrayGUI::main(int argc, char** argv)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  
  // --------------------------------------------------------------------------
  // Set up the primary message logger
  // --------------------------------------------------------------------------

  StreamMessenger* log_messenger = new StreamMessenger;
  Messenger::relay()->registerMessenger(log_messenger);
  BackTrace::catchSignalPrintBacktraceAndAbort(SIGSEGV);
  Messenger::relay()->holdMessages();

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
      // ----------------------------------------------------------------------
      // Initialize the ORB
      // ----------------------------------------------------------------------

      VOmniORBHelper* orb = 
	CorbaHelper::initOrb(m_corba_args, m_corba_nameserver, m_corba_port);
      
      // ----------------------------------------------------------------------
      // Start the CORBA message sender/receiver if appropriate
      // ----------------------------------------------------------------------
      
      NET_MessageReceiverPolling* event_polling =
	new NET_MessageReceiverPolling(orb, Messenger::relay(), 100,
				       NET_TCInterface::program_name,
				       NET_Messenger::object_name);

      // ----------------------------------------------------------------------
      // Create the controller vector - this defines the number of scopes
      // ----------------------------------------------------------------------

      std::vector<TelescopeController*> controllers(4);

      // ----------------------------------------------------------------------
      // Get some of the default telescope information
      // ----------------------------------------------------------------------
      
      std::vector<QColor> colors(controllers.size());
      double mean_theta = 0;
      double mean_phi = 0;
      std::vector<StowObjectVector> scope_stow_pos;
      std::vector<SEphem::SphericalCoords> scope_earth_pos;
      scope_stow_pos.resize(controllers.size());
      scope_earth_pos.resize(controllers.size());
      for(unsigned iscope=0; iscope<controllers.size(); iscope++)
	{
	  Protocol protocol;
	  std::string datastream;
	  SEphem::SphericalCoords earth_pos;
	  TelescopeMotionLimits* limits;
	  QColor color;
	  setTelescopeInfo(iscope, "scope_default", Main::MM_ARRAYGUI, 
			   protocol, datastream, scope_earth_pos[iscope],
			   scope_stow_pos[iscope], limits, color);
	  colors[iscope] = color;
	  mean_theta += scope_earth_pos[iscope].thetaRad();
	  mean_phi += scope_earth_pos[iscope].phiRad();
	}

      SEphem::SphericalCoords earth_pos(mean_theta/controllers.size(),
					mean_phi/controllers.size());
 
      // ----------------------------------------------------------------------
      // Create the controllers
      // ----------------------------------------------------------------------

      unsigned nscope = controllers.size();
      for(unsigned iscope=0; iscope<controllers.size(); iscope++)
	{
	  int phase = 
	    (iscope*m_controller_iter_period_ms)/nscope + 
	    m_controller_iter_period_ms/25;
	  controllers[iscope] =
	    new TelescopeControllerRemote(orb, iscope,
					  m_controller_iter_period_ms, phase,
					  m_remote_readonly,
					  m_remote_no_auto_stopping,
					  scope_earth_pos[iscope]);
	}

      // ----------------------------------------------------------------------
      // QT helper -- thread to set up widgets and runs QT event loop
      // ----------------------------------------------------------------------

      QtHelper* qt = 
	new QtHelperMainArrayGUI(controllers, orb, colors, earth_pos,
				 scope_stow_pos, m_suppress_servo_fail,
				 m_qt_args, !m_no_grb, m_theme, &tn_condition);

      // ----------------------------------------------------------------------
      // Threads
      // ----------------------------------------------------------------------

      ZThread::Thread* event_polling_thread = 
	new ZThread::Thread(event_polling);      
      std::vector<ZThread::Thread*> controller_threads(controllers.size());
      for(unsigned iscope=0; iscope<controllers.size(); iscope++)
	controller_threads[iscope] = new ZThread::Thread(controllers[iscope]);
      ZThread::Thread* qt_thread = new ZThread::Thread(qt);

      // ----------------------------------------------------------------------
      // Wait on condition variable until we are signaled to shut down
      // ----------------------------------------------------------------------

      tn_condition.wait();

      // ----------------------------------------------------------------------
      // Terminate the QT main loop and wait for QT helper thread
      // ----------------------------------------------------------------------

      qt->terminate();
      qt_thread->wait();
      delete qt_thread;

      // ----------------------------------------------------------------------
      // Stop the polling message thread and wait for it to terminate
      // ----------------------------------------------------------------------

      event_polling->terminate();
      event_polling_thread->wait();
      delete event_polling_thread;

      // ----------------------------------------------------------------------
      // Stop the controllers and wait for each thread
      // ----------------------------------------------------------------------

      for(unsigned iscope=0; iscope<controllers.size(); iscope++)
	{
	  controllers[iscope]->terminate();
	  controller_threads[iscope]->wait();
	  delete controller_threads[iscope];
	}

      // ----------------------------------------------------------------------
      // Delete the ORB
      // ----------------------------------------------------------------------

      if(orb)delete orb;
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

  Messenger::relay()->unRegisterMessenger(log_messenger);
  delete log_messenger;

  return EXIT_SUCCESS;      
}

void MainArrayGUI::configure(VERITAS::VSOptions& options)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  // -----
  // Theme
  // -----

  std::string theme = "default";
  options.findWithValue("gui_theme", theme, OPT_MAIN_ARRAY_THEME);
  s_default_theme=0;
  if(theme=="a-team")s_default_theme=1;
  else if(theme=="hobbit")s_default_theme=2;
  else if(theme=="fellowship")s_default_theme=3;
  else if(theme=="yellow_submarine")s_default_theme=4;
  else if(theme=="musketeers")s_default_theme=5;
  else if(theme=="johns")s_default_theme=6;

  // ----------------------
  // Suppress servo failure
  // ----------------------

  options.findWithValue("suppress_servo_fail",s_default_suppress_servo_fail,
			OPT_MAIN_ARRAY_SUPPRESS_SERVO_FAILURE);
}

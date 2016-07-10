//-*-mode:c++; mode:font-lock;-*-

/**
 * \file Main.h
 * \ingroup VTracking
 * \brief This is a one-line description of this cpp file.
 *
 * Here is a tedious verbose multi-line description of all the details
 * of the code, more than you would ever want to read. Generally, all
 * the important documentation goes in the .cpp files.
 *
 * Original Author: Stephen Fegan
 * $Author: sfegan $
 * $Date: 2007/01/27 12:55:34 $
 * $Revision: 2.5 $
 * $Tag$
 *
 **/

#ifndef VTRACKING_MAIN_H
#define VTRACKING_MAIN_H

#include<vector>
#include<string>

#include<zthread/RecursiveMutex.h>
#include<zthread/Condition.h>
#include<zthread/Runnable.h>

#include<qobject.h>
#include<qapplication.h>

#include<VSOptions.hpp>
#include<VOmniORBHelper.h>
#include<SphericalCoords.h>
#include<Notification.h>

#include"TargetObject.h"
#include"TelescopeMotionLimits.h"

namespace VTracking
{

  // ==========================================================================
  // QT HELPER CLASS
  // ==========================================================================

  class QtHelper: public QObject, public ZThread::Runnable
  {
    Q_OBJECT
  public:
    QtHelper(const std::vector<std::string>& args,
	     ZThread::Condition* terminate_notifier);
    virtual ~QtHelper();
    void terminate();
    virtual void run();
    
  protected:
    virtual void createWidgets(QApplication* app) = 0;
    
  protected:
    class TerminateNotification: public VTaskNotification::Notification
    {
    public:
      TerminateNotification(QApplication* app): 
	Notification(), m_app(app) { /* nothing to see here */ }
      virtual ~TerminateNotification();
      virtual void doNotification();
    private:
      QApplication*                  m_app;
    };
    
    std::vector<std::string>         m_args;
    ZThread::Condition*              m_terminate_notifier;
    ZThread::RecursiveMutex          m_mutex;
    bool                             m_running;
    
  private slots:
    void sendTerminateNotification();
  };

  // ==========================================================================
  // CORBA HELPER
  // ==========================================================================
  
  class CorbaHelper: public ZThread::Runnable
  {
  public:
    CorbaHelper(VCorba::VOmniORBHelper* orb, 
		ZThread::Condition* terminate_notifier);
    virtual ~CorbaHelper();
    virtual void terminate();
    virtual void run();
    
    static VCorba::VOmniORBHelper* 
    initOrb(const std::vector<std::string>& args,
	    const std::string& nameserver, int portnumber);
  protected:
    virtual void createServants(VCorba::VOmniORBHelper* orb) = 0;
    void sendTerminateNotification();
    
  protected:
    VCorba::VOmniORBHelper*          m_orb;
    ZThread::Condition*              m_terminate_notifier;
    ZThread::RecursiveMutex          m_mutex;
    bool                             m_running;
  };

  // ==========================================================================
  // BASE CLASS FOR MAIN FUNCTIONALITY
  // ==========================================================================
  
  class Main
  {
  public:
    virtual ~Main();
    virtual int main(int argc, char** argv) = 0;
    
    static Main* getMain(); // Factory method
    static void configure(VERITAS::VSOptions& options);
    
    enum MainMode { MM_CONTROLLER, MM_ARRAYGUI,
		    MM_EMULATOR, MM_FAILSAFE, MM_TERMINATE };
    enum Protocol { P_EIA, P_PIU, P_PIU_PROTO, P_10M };
    
  private:
    Main(const Main& o);
    Main& operator = (const Main& o);
    
    static Main* s_main;
    
  protected:
    Main() { /* nothing to see here */ }
    
    void setTelescopeInfo(unsigned scope_id, 
			  const std::string& protocol_string,
			  const MainMode mainmode,
			  Protocol& protocol, std::string& datastream,
			  SEphem::SphericalCoords& earth_pos,
			  StowObjectVector& stow_pos,
			  TelescopeMotionLimits*& limits, QColor& color,
			  bool daytime_limits=false, bool override_key=false,
			  double dn_limit=15.0);
    
    static std::string s_program;

    static MainMode                        s_default_mainmode;
    static unsigned                        s_default_verbose;
    static unsigned                        s_default_scope_id;
    static std::string                     s_default_protocol;
    static std::string                     s_default_datastream;
    static unsigned                        s_default_controller_iter_period_ms;
    static std::vector<std::string>        s_default_qt_args;
    static bool                            s_default_no_qt;
    static bool                            s_default_suppress_db;
    static std::vector<std::string>        s_default_corba_args;
    static int                             s_default_corba_port;
    static std::string                     s_default_corba_nameserver;
    static std::string                     s_default_remote_ior;
    static bool                            s_default_remote_readonly;
    static bool                            s_default_simple_interface_readonly;
    static bool                            s_default_remote_no_auto_stopping;
    static bool                            s_default_no_corba;
    static bool                            s_default_no_grb;
  };
  
} // namespace VTracking

#define DEFAULT_LOCAL_CONTROLLER_ITER_PERIOD 250
#define DEFAULT_REMOTE_CONTROLLER_ITER_PERIOD 250

#endif // VTRACKING_MAIN_H

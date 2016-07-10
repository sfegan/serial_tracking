//-*-mode:c++; mode:font-lock;-*-

/**
 * \file MainController.h
 * \ingroup VTracking
 * \brief This is a one-line description of this cpp file.
 *
 * Here is a tedious verbose multi-line description of all the details
 * of the code, more than you would ever want to read. Generally, all
 * the important documentation goes in the .cpp files.
 *
 * Original Author: Stephen Fegan
 * $Author: sfegan $
 * $Date: 2007/01/25 22:58:12 $
 * $Revision: 2.7 $
 * $Tag$
 *
 **/

#ifndef VTRACKING_MAINCONTROLLER_H
#define VTRACKING_MAINCONTROLLER_H

#include"TargetObject.h"
#include"TelescopeController.h"
#include"NET_TCInterfaceServant.h"
#include"NET_SerialTrackingServant.h"

#include"Main.h"

namespace VTracking
{

  // ==========================================================================
  // MAIN CONTROLLER QT HELPER CLASS
  // ==========================================================================

  class QtHelperMainController: public QtHelper
  {
  public:
    QtHelperMainController(int scope_num, TelescopeController* controller,
			   const SEphem::SphericalCoords& pos, 
			   const StowObjectVector& stow_pos,
			   int protection_level, bool security_override,
			   const QColor& color,
			   const std::vector<std::string>& args,
			   ZThread::Condition* terminate_notifier = 0);
    virtual ~QtHelperMainController();
    
  protected:
    virtual void createWidgets(QApplication* app);
    
  private:
    int                              m_scope_num;
    TelescopeController*             m_controller;
    SEphem::SphericalCoords          m_pos;
    const StowObjectVector&          m_stow_pos;
    int                              m_protection_level;
    bool                             m_security_override;
    QColor                           m_color;
  };
  
  // ==========================================================================
  // MAIN CONTROLLER CORBA HELPER
  // ==========================================================================
  
  class CorbaHelperTCInterface: public CorbaHelper
  {
  public:
    CorbaHelperTCInterface(TelescopeController* controller,
			   const StowObjectVector& stow_pos,
			   int scope_num, VCorba::VOmniORBHelper* orb,
			   ZThread::Condition* terminate_notifier,
			   bool readonly, bool simple_interface_readonly);
    virtual ~CorbaHelperTCInterface();
    //virtual void terminate();
    
  protected:
    virtual void createServants(VCorba::VOmniORBHelper* orb);
    
  private:
    TelescopeController*             m_controller;
    const StowObjectVector&          m_stow_pos;
    int                              m_scope_num;
    NET_TCInterfaceServant*          m_servant;
    NET_SerialTrackingServant*       m_simple_interface_servant;
    bool                             m_readonly;
    bool                             m_simple_interface_readonly;
  };

  // ==========================================================================
  // MAIN CONTROLLER CLASS
  // ==========================================================================
  
  class MainController: public Main
  {
  public:
    MainController();
    virtual ~MainController();
    virtual int main(int argc, char** argv);
    static void configure(VERITAS::VSOptions& options);
    static void setRealtime() { s_realtime = true; }
  private:
    enum Controller { C_LOCAL, C_REMOTE };
    
    unsigned                   m_verbose;
    unsigned                   m_scope_id;
    Protocol                   m_protocol;
    std::string                m_datastream;
    bool                       m_override_security;
    std::string                m_record_file;
    unsigned                   m_logger_timeout_sec;
    bool                       m_suppress_db;
    std::vector<std::string>   m_qt_args;
    SEphem::SphericalCoords    m_earth_pos;
    StowObjectVector           m_stow_pos;
    TelescopeMotionLimits*     m_limits;
    Controller                 m_controller;
    bool                       m_no_qt;
    std::vector<std::string>   m_corba_args;
    int                        m_corba_port;
    std::string                m_corba_nameserver;
    bool                       m_no_corba;
    std::string                m_remote_ior;
    bool                       m_remote_readonly;
    bool                       m_simple_interface_readonly;
    bool                       m_remote_no_auto_stopping;
    unsigned                   m_controller_iter_period_ms;
    unsigned                   m_db_log_period_iter;
    unsigned                   m_db_cache_size;
    bool                       m_daemon;
    std::string                m_log_file;

    QColor                     m_color;
    
    static double              s_default_dn_limit;
    static bool                s_default_daytime_limits;
    static bool                s_default_override_security;
    static bool                s_default_override_key;
    static std::string         s_default_record_file;
    static unsigned            s_default_logger_timeout_sec;
    static std::string         s_default_controller;
    static unsigned            s_default_db_log_period_iter;
    static unsigned            s_default_db_cache_size;
    static bool                s_default_daemon;
    static std::string         s_default_log_file;

    static bool                s_realtime;
  };

}

#endif // VTRACKING_MAINCONTROLLER_H

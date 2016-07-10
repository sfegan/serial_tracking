//-*-mode:c++; mode:font-lock;-*-

/**
 * \file MainArrayGUI.h
 * \ingroup VTracking
 * \brief Main class to set up and start array GUI
 *
 * Original Author: Stephen Fegan
 * Start Date: 2006-07-15
 * $Author: sfegan $
 * $Date: 2007/01/27 12:55:34 $
 * $Revision: 2.5 $
 * $Tag$
 *
 **/

#ifndef VTRACKING_MAINARRAYGUI_H
#define VTRACKING_MAINARRAYGUI_H

#include<vector>

#include<VOmniORBHelper.h>

#include"TargetObject.h"
#include"TelescopeController.h"
#include"NET_TCInterfaceServant.h"

#include"Main.h"

namespace VTracking
{

  // ==========================================================================
  // MAIN ARRAY GUI QT HELPER CLASS
  // ==========================================================================

  class QtHelperMainArrayGUI: public QtHelper
  {
  public:
    QtHelperMainArrayGUI(std::vector<TelescopeController*>& controller_vec,
			 VCorba::VOmniORBHelper* orb,
			 const std::vector<QColor>& colors,
			 const SEphem::SphericalCoords& mean_earth_pos, 
		const std::vector<VTracking::StowObjectVector>& scope_stow_pos,
			 std::vector<unsigned> suppress_servo_fail,
			 const std::vector<std::string>& args, 
			 bool grb, unsigned theme,
			 ZThread::Condition* terminate_notifier = 0);
    virtual ~QtHelperMainArrayGUI();
    
  protected:
    virtual void createWidgets(QApplication* app);
    
  private:
    std::vector<TelescopeController*>         m_controller_vec;
    VCorba::VOmniORBHelper*                   m_orb;
    std::vector<QColor>                       m_colors;
    SEphem::SphericalCoords                   m_mean_earth_position;
    std::vector<VTracking::StowObjectVector>  m_scope_stow_pos;
    std::vector<unsigned>                     m_suppress_servo_fail;
    bool                                      m_grb;
    unsigned                                  m_theme;
  };
  
  // ==========================================================================
  // MAIN ARRAY GUI CLASS
  // ==========================================================================
  
  class MainArrayGUI: public Main
  {
  public:
    MainArrayGUI();
    virtual ~MainArrayGUI();
    virtual int main(int argc, char** argv);
    static void configure(VERITAS::VSOptions& options);
  private:
    unsigned                     m_theme;
    std::vector<unsigned>        m_suppress_servo_fail;

    bool                         m_suppress_db;
    std::vector<std::string>     m_qt_args;
    std::vector<std::string>     m_corba_args;
    int                          m_corba_port;
    std::string                  m_corba_nameserver;
    bool                         m_remote_readonly;
    bool                         m_remote_no_auto_stopping;
    unsigned                     m_controller_iter_period_ms;
    bool                         m_no_grb;

    static unsigned              s_default_theme;
    static std::vector<unsigned> s_default_suppress_servo_fail;
  };

}

#endif // VTRACKING_MAINARRAYGUI_H

//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUIArray.h
 * \ingroup VTracking
 * \brief Array GUI
 *
 * Original Author: Stephen Fegan
 * Start Date: 2006-07-14
 * $Author: sfegan $
 * $Date: 2008/02/09 08:50:49 $
 * $Revision: 2.18 $
 * $Tag$
 *
 **/

#ifndef VTRACKING_GUIARRAY_H
#define VTRACKING_GUIARRAY_H

#include<qmainwindow.h>
#include<qwizard.h>
#include<qlabel.h>

#include<Message.h>
#include<Messenger.h>
#include<QtMessenger.h>
#include<QtDialogMessenger.h>
#include<Angle.h>
#include<SphericalCoords.h>
#include<VOmniORBHelper.h>

#include"ScopeAPI.h"
#include"TargetObject.h"
#include"TelescopeController.h"

#include"GUIMisc.h"
#include"GUIArrayPane.h"
#include"GUISummaryDetailsPane.h"
#include"GUITargetTablePane.h"
#include"GUIGRBMonitor.h"
#include"GUIMessengerPane.h"
#include"GUISunWarning.h"

#include"GUIAzElIndicator.h"

using namespace SEphem;
using namespace VTracking;

class GUIArrayWidget: public QMainWindow
{
  Q_OBJECT
public:
  GUIArrayWidget(const std::vector<TelescopeController*>& controller_vec,
		 VCorba::VOmniORBHelper* orb,
		 const std::vector<QColor>& colors,
		 const SEphem::SphericalCoords& mean_earth_pos, 
	 const std::vector<VTracking::StowObjectVector>& scope_stow_pos,
		 std::vector<unsigned> suppress_servo_fail, bool grb = true,
		 unsigned theme = 0,
		 QWidget* parent=0, const char* name=0, 
		 WFlags f=WType_TopLevel);
  ~GUIArrayWidget();
  
public slots:

  // Single telescope control methods -----------------------------------------

  void goOne(unsigned selector_id);
  void stopOne(unsigned selector_id);
  void emergencyOne(unsigned selector_id);
  void terminateServerOne(unsigned selector_id);

  void setTargetOne(unsigned id, const VTracking::TargetObject* obj);
  void setTargetOne(unsigned id, const VTracking::TargetObject* obj,
		    SEphem::CorrectionParameters::DirectionPreference dp);
  void sendSelectedTargetToScopeOne(unsigned selector_id);

  // All control methods ------------------------------------------------------

  void stopAll();
  void emergencyAll();
  void terminateServerAll();

  // Other methods ------------------------------------------------------------

  void routeTarget(int target);
  void loadTargetListOne(unsigned id); // for individual details page
  void loadTargetList();
  void routeGRB(int target);
  void recommendGRBObservation(const GRBTrigger* grb,
			       const VTracking::RaDecObject* obj);

private slots:
  void tick();
  void setTargetCV(int item);
  void changeTargetRouting(int item);
  void changeScopeValuesToDisplay(int item);
  void changePreferredDirection(int item);
  void catchTerminateServer(int item);
  void grbWizardPaneSelected(const QString& title);
  void syncWithGRBList(const GUIGRBMonitor::GRBTriggerList& grb_list);
  void addTargets();
  void manageTargets();

signals:
  void syncWithTargetList(const VTracking::TargetList&);
  void setScopeValuesToDisplay(ScopeValuesToDisplay);
  void changeDirPref(bool, SEphem::CorrectionParameters::DirectionPreference);

protected:
  void closeEvent(QCloseEvent * e);
  virtual void resizeEvent(QResizeEvent* e);

private:
  void doCV(double elspeed, double azspeed);
  void updateStatusBar(const std::vector<GUIUpdateData>& ud_vec);
  bool isControllable(unsigned id) { return true; }
  void getTargetTableUpdateData(GUIUpdateData& ud,
				const std::vector<GUIUpdateData>& ud_vec);
  void testForLargeClockDifference();

  // --------------------------------------------------------------------------
  // SUB CLASSES
  // --------------------------------------------------------------------------

  class TargetTableUDGetter: public GUITabPaneGetUpdateData
  {
  public:
    TargetTableUDGetter(GUIArrayWidget* array): 
      GUITabPaneGetUpdateData(), m_array(array) { }
    virtual ~TargetTableUDGetter();
    virtual std::vector<GUIUpdateData> 
    getUpdateData(const std::vector<GUIUpdateData>& ud_vec) const;
  private:
    GUIArrayWidget* m_array;
  };

  friend class TargetTableUDGetter;

  // --------------------------------------------------------------------------
  // SETTINGS
  // --------------------------------------------------------------------------

  std::vector<TelescopeController*>         m_controller_vec;
  VCorba::VOmniORBHelper*                   m_orb;
  std::vector<GUIArrayPane::ScopeConfig>    m_scope;
  SphericalCoords                           m_mean_earth_position;
  VTracking::TargetList                     m_target_list;
  std::vector<GUIUpdateData>                m_ud_vec;
  bool                                      m_slew_as_array;
  CorrectionParameters::DirectionPreference m_direction_preference;

  // --------------------------------------------------------------------------
  // MISCELLANEOUS
  // --------------------------------------------------------------------------

  std::vector<time_t>                       m_next_clock_check;

  // --------------------------------------------------------------------------
  // GUI WIDGET COMPONENTS
  // --------------------------------------------------------------------------

  GUITabWidget*                             m_tabwidget;

  GUIArrayPane*                             m_pane_array;
  std::vector<GUIDetailsPane*>              m_pane_details;
  GUITargetTablePane*                       m_pane_targettable;
  GUIGRBMonitor*                            m_pane_grb_monitor;
  GUIMessengerPane*                         m_pane_messenger;

  std::vector<QLineEdit*>                   m_stat_ind_tel;
  QPushButton*                              m_stat_cntrl_panic;
  int                                       m_stat_cntrl_panic_x;

  QTimer*                                   m_timer;

  VMessaging::QtMessenger*                  m_qt_messenger;
  VMessaging::QtDialogMessenger*            m_qt_dialog_messenger;

  GUISunWarning*                            m_sun;

  std::map<QString, int>                    m_menu_map_fwd;

  int                                       m_menu_tar_array;
  std::map<unsigned,int>                    m_menu_tar_scope;

  GUIGRBMonitor::GRBTriggerList             m_grb_list;
  const GRBTrigger*                         m_grb_recommended;
  QWizard*                                  m_grb_wizard;
  std::map<QString, QWidget*>               m_grb_wizard_panes;
  QLabel*                                   m_grb_info_gcn;
  QLabel*                                   m_grb_info_time;
  QLabel*                                   m_grb_info_experiment;
  QLabel*                                   m_grb_info_type;
  QLabel*                                   m_grb_info_elevation;
  QLabel*                                   m_grb_info_age;
};

#endif

//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUI.h
 * \ingroup VTracking
 * \brief This is a one-line description of this cpp file.
 *
 * Here is a tedious verbose multi-line description of all
 * the details of the code, more than you would
 * ever want to read. Generally, all the important documentation
 * goes in the .cpp files.
 *
 * Original Author: Stephen Fegan
 * $Author: sfegan $
 * $Date: 2008/01/31 21:24:13 $
 * $Revision: 2.4 $
 * $Tag$
 *
 **/

#ifndef VTRACKING_GUI_H
#define VTRACKING_GUI_H

#include<sys/time.h>

#include<vector>
#include<map>

#include<qmainwindow.h>
#include<qlineedit.h>
#include<qgroupbox.h>
#include<qtimer.h>
#include<qbuttongroup.h>
#include<qradiobutton.h>
#include<qcombobox.h>
#include<qpushbutton.h>
#include<qtabwidget.h>
#include<qspinbox.h>
#include<qhbox.h>
#include<qlabel.h>
#include<qcheckbox.h>

#include<Message.h>
#include<Messenger.h>
#include<QtMessenger.h>
#include<QtDialogMessenger.h>
#include<Angle.h>
#include<SphericalCoords.h>
#include<CorrectionParameters.h>

#include"ScopeAPI.h"
#include"TargetObject.h"
#include"TelescopeController.h"

#include"GUIMisc.h"
#include"GUISummaryDetailsPane.h"
#include"GUITargetTablePane.h"
#include"GUIMessengerPane.h"
#include"GUIOscilloscopePane.h"
#include"GUITCPPane.h"
#include"GUICPSolverPane.h"
#include"GUISunWarning.h"

using namespace SEphem;
using namespace VTracking;

const QString gui_security_pass = "OpenSesame";

class GUIWidget: public QMainWindow
{
  Q_OBJECT
public:
  GUIWidget(int scope_num, TelescopeController* controller,
	    const SEphem::SphericalCoords& pos, 
	    const VTracking::StowObjectVector& stow_pos,
	    int protection_level=1, bool security_override=false,
	    QWidget* parent=0, const char* name=0, WFlags f=WType_TopLevel );
  ~GUIWidget();
  
public slots:

  void go(unsigned selector_id);
  void stop(unsigned selector_id);
  void sendSelectedTargetToScope(unsigned selector_id);
  void loadTargetList(unsigned selector_id);

  void go();
  void stop();
  void emergency();
  void loadTargetList();

  void setFromTargetTable(int target);
  void setTargetCV(double elspeed, double azspeed);
  void setTargetSF(double elstep, double azstep);

private slots:
  void update();
  void setCorrections();
  void recordTrackingPosition(double raw_az, double raw_el,
			      double cor_az, double cor_el);

  void menuPrefDir(int index);
  void menuCVTarget(int index);
  void menuSFTarget(int index);
  void menuInterface(int index);
  void overrideSecurity();

signals:
  void syncWithTargetList(const VTracking::TargetList&);
  
protected:
  void closeEvent(QCloseEvent * e);
  bool eventFilter(QObject* o, QEvent* e);
  virtual void resizeEvent(QResizeEvent* e);
 
private:
  void updateStatusBar(const GUIUpdateData& ud);
  
  // --------------------------------------------------------------------------
  // SETTINGS
  // --------------------------------------------------------------------------

  int                                  m_scope_num;
  TelescopeController*                 m_controller;
  SphericalCoords                      m_earth_position;
  VTracking::StowObjectVector          m_stow_pos;
  VTracking::TargetList                m_target_list;
  std::vector<GUIUpdateData>           m_ud_vec;
  CorrectionParameters::DirectionPreference m_direction_preference;

  QWidget*                             m_default_control_widget;

  // --------------------------------------------------------------------------
  // GUI WIDGET COMPONENTS
  // --------------------------------------------------------------------------

  GUITabWidget*                        m_tabwidget;

  QPushButton*                         m_cntrl_emgcy_pb;
  int                                  m_cntrl_emgcy_x;

  QLineEdit*                           m_stat_el;
  QLineEdit*                           m_stat_az;
  QLineEdit*                           m_stat_req;
  QLineEdit*                           m_stat_cur;
  QLabel*                              m_stat_ind;
  
  double                               m_stat_ind_dist;
  int                                  m_stat_ind_frame;

  std::set<GUIObjectSelector*>*        m_selector_mirrors;
  GUISummaryPane*                      m_pane_summary;
  GUIDetailsPane*                      m_pane_details;
  GUITargetTablePane*                  m_pane_targettable;
  GUIMessengerPane*                    m_pane_messenger;
#ifndef GUI_NO_QWT
  GUIOscilloscopePane*                 m_pane_oscilloscope;
#endif
  GUITCPPane*                          m_pane_tcp;
  GUICPSolverPane*                     m_pane_solver;

  QTimer*                              m_timer;

  VMessaging::QtMessenger*             m_qt_messenger;
  VMessaging::QtDialogMessenger*       m_qt_dialog_messenger;

  GUISunWarning*                       m_sun;

  std::map<QString, int>               m_menu_map_fwd;
};

#endif // VTRACKING_GUI_H

//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUIObjectSelector.h
 * \ingroup VTracking
 * \brief This is a one-line description of this cpp file.
 *
 * Here is a tedious verbose multi-line description of all
 * the details of the code, more than you would
 * ever want to read. Generally, all the important documentation
 * goes in the .cpp files.
 *
 * Original Author: Stephen Fegan
 * $Author: aune $
 * $Date: 2011/10/06 02:14:28 $
 * $Revision: 2.7 $
 * $Tag$
 *
 **/

#ifndef VTRACKING_GUIOBJECTSELECTOR_H
#define VTRACKING_GUIOBJECTSELECTOR_H

#include<set>

#include<qlineedit.h>
#include<qlabel.h>
#include<qwidgetstack.h>
#include<qpushbutton.h>
#include<qradiobutton.h>
#include<qbuttongroup.h>

#include "TelescopeController.h"
#include "TargetObject.h"
#include "GUIMisc.h"
#include "GUIGRBMonitor.h"

class GUITrackingModeData
{
public:
  enum TrackingMode { TM_ONOFF, TM_WOBBLE, TM_ORBIT, TM_ELAZ, TM_POINT };
  enum OnOffMode { OOM_ON, OOM_OFF_AFTER_ON, OOM_OFF_BEFORE_ON };
  GUITrackingModeData(): 
    mode(TM_ONOFF), onoff(OOM_ON), 
    onoff_time(SEphem::Angle::makeHrs(SID_RATE*30.0/60.0)),
    wobble_coords(), elaz_coords(), 
    orbit_period(), orbit_coords(), orbit_free(false),
    name() { }
  TrackingMode            mode;
  OnOffMode               onoff;
  SEphem::Angle           onoff_time;
  SEphem::SphericalCoords wobble_coords;
  SEphem::SphericalCoords elaz_coords;
  double                  orbit_period;
  SEphem::SphericalCoords orbit_coords;
  bool                    orbit_free;
  std::string             name;
};

class GUIObjectSelector: public QFrame
{
  Q_OBJECT
public:
  GUIObjectSelector(unsigned identifier, bool details_pane, 
		    bool use_myqgroupbox, QFont f, int& height,
		    const VTracking::StowObjectVector& stow_pos,
		    std::set<GUIObjectSelector*>* mirrors, bool grb,
		    QWidget* parent=0, const char* name=0);

  virtual ~GUIObjectSelector();

  VTracking::TargetObject* getObject(double az_deg, double mjd) const;

  void updateButtons(bool controls_enabled,
		     VTracking::TelescopeController::TrackingState state,
		     VTracking::TelescopeController::TrackingRequest req);
  void animateButtons();

signals:
  void go(unsigned id);
  void stop(unsigned id);
  void loadNewTargetList(unsigned id);

  void setTarget(unsigned id);

public slots:
  void syncWithTargetList(const VTracking::TargetList& target_list);
  void selectTarget(int target);

  void syncWithGRBList(const GUIGRBMonitor::GRBTriggerList& grb_list);
  void selectGRB(int grb);
  
private slots:
  void sendGo();
  void sendStop();
  void sendSetTarget();
  void sendLoadNewTargetList();
  void mirror();
  void setModeToOn();
  void cycleWobble();

protected:
  void makeTrackingModeWidgets(QWidget* parent, const QString& name,
			       const QFont& f, bool no_pointing,
			       MyQComboBox*& mode,
			       QWidgetStack*& stack, 
			       MyQComboBox*& onoff,
			       MyQComboBox*& wobble_dir,
			       MyQComboBox*& wobble_off,
			       MyQComboBox*& el_off,
			       MyQComboBox*& az_off,
			       MyQComboBox*& orbit_per,
			       MyQComboBox*& orbit_off,
			       MyQComboBox*& orbit_dir);

  void setTrackingModeData(GUITrackingModeData& tm,
			   MyQComboBox* mode, MyQComboBox* onoff,
			   MyQComboBox* wobble_dir, MyQComboBox* wobble_off,
			   MyQComboBox* el_off, MyQComboBox* az_off,
			   MyQComboBox* orbit_per, MyQComboBox* orbit_off,
			   MyQComboBox* orbit_dir) const;

  unsigned                           m_identifier;
  bool                               m_details_pane;
  const VTracking::StowObjectVector  m_stow_pos;
  VTracking::TargetList              m_target_list;
  GUIGRBMonitor::GRBTriggerList      m_grb_list;
  std::set<GUIObjectSelector*>*      m_mirrors;
			   
  QWidgetStack*                      m_cmd_stack;
  QWidgetStack*                      m_cmd_buttons;
  QLineEdit*                         m_cmd_az;
  QLineEdit*                         m_cmd_el;
  QRadioButton*                      m_cmd_azel_no_use_corrections;
  QRadioButton*                      m_cmd_azel_no_stop_at_target;
  QLineEdit*                         m_cmd_ra;
  QLineEdit*                         m_cmd_dec;
  MyQComboBox*                       m_cmd_radec_mode;
  QWidgetStack*                      m_cmd_radec_stack;
  MyQComboBox*                       m_cmd_radec_onoff;
  MyQComboBox*                       m_cmd_radec_wobble_off;
  MyQComboBox*                       m_cmd_radec_wobble_dir;
  MyQComboBox*                       m_cmd_radec_el_off;
  MyQComboBox*                       m_cmd_radec_az_off;
  MyQComboBox*                       m_cmd_radec_orbit_period;
  MyQComboBox*                       m_cmd_radec_orbit_off;
  MyQComboBox*                       m_cmd_radec_orbit_dir;
  QLineEdit*                         m_cmd_epoch;
  MyQComboBox*                       m_cmd_target;
  QPushButton*                       m_cmd_load;
  MyQComboBox*                       m_cmd_tar_mode;
  QWidgetStack*                      m_cmd_tar_stack;
  MyQComboBox*                       m_cmd_tar_onoff;
  MyQComboBox*                       m_cmd_tar_wobble_off;
  MyQComboBox*                       m_cmd_tar_wobble_dir;  
  MyQComboBox*                       m_cmd_tar_el_off;
  MyQComboBox*                       m_cmd_tar_az_off;  
  MyQComboBox*                       m_cmd_tar_orbit_period;
  MyQComboBox*                       m_cmd_tar_orbit_off;
  MyQComboBox*                       m_cmd_tar_orbit_dir;
  MyQComboBox*                       m_cmd_grb;
  MyQComboBox*                       m_cmd_grb_mode;
  QWidgetStack*                      m_cmd_grb_stack;
  MyQComboBox*                       m_cmd_grb_onoff;
  MyQComboBox*                       m_cmd_grb_wobble_off;
  MyQComboBox*                       m_cmd_grb_wobble_dir;  
  MyQComboBox*                       m_cmd_grb_el_off;
  MyQComboBox*                       m_cmd_grb_az_off;  
  MyQComboBox*                       m_cmd_grb_orbit_period;
  MyQComboBox*                       m_cmd_grb_orbit_off;
  MyQComboBox*                       m_cmd_grb_orbit_dir;
  MyQComboBox*                       m_cmd_aux;
  QPushButton*                       m_cmd_go;
  QPushButton*                       m_cmd_stop;
  QRadioButton*                      m_cmd_azel_but;
  QRadioButton*                      m_cmd_radec_but;
  QRadioButton*                      m_cmd_target_but;
  QRadioButton*                      m_cmd_other_but;
  QRadioButton*                      m_cmd_grb_but;
  QButtonGroup*                      m_cmd_scheme_buttons;

  int                                m_go_button_index;
};

#endif

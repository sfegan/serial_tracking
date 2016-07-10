//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUIArrayPane.h
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
 * $Date: 2008/02/09 08:16:14 $
 * $Revision: 2.16 $
 * $Tag$
 *
 **/

#ifndef VTRACKING_GUIARRAYPANE_H
#define VTRACKING_GUIARRAYPANE_H

#include<set>

#include<qwidget.h>
#include<qframe.h>
#include<qlineedit.h>
#include<qlabel.h>
#include<qwidgetstack.h>
#include<qpushbutton.h>
#include<qradiobutton.h>
#include<qbuttongroup.h>

#if 0
#include<qgroupbox.h>
#include<qtimer.h>
#include<qtabwidget.h>
#include<qspinbox.h>
#include<qhbox.h>
#include<qcheckbox.h>
#endif

#include "GUIObjectSelector.h"
#include "GUITabWidget.h"
#include "GUIPositionerStatusWidget.h"
#include "GUIAzElIndicator.h"

#include "TargetObject.h"

//#define USE_INDIVIDUAL_PANIC_BUTTONS
#define USE_SOLICIT_TARGET

class GUITelIndicator: public QFrame
{
  Q_OBJECT
public:
  GUITelIndicator(unsigned id, bool suppress_servo_fail_error,
		  QWidget* parent=0, const char* name=0);
  virtual ~GUITelIndicator();

  bool isInArray() const { return m_cntrl_in_array->currentItem()==1 /* isOn() */; }
  bool canSendArrayCommands() const { return isInArray()&&(!m_com_fail); }
  bool canReTarget() const { return canSendArrayCommands()&&m_is_stopped; }

  void update(bool controls_enabled, const GUIUpdateData& ud);

public slots:
  void setScopeValuesToDisplay(ScopeValuesToDisplay sv);
  void setInArray(bool in_array);

signals:
  void go(unsigned id);
  void stop(unsigned id);
  void solicitTarget(unsigned id);
  void emergency(unsigned id);
  void inArrayClicked(unsigned id);

private slots:
  void sendGo();
  void sendStop();
  void sendSolicitTarget();
  void sendEmergency();
  void sendInArrayClicked();

private:
  GUITelIndicator(const GUITelIndicator&);
  const GUITelIndicator& operator=(const GUITelIndicator&);

  unsigned              m_id;

  QLineEdit*            m_ind_src_name;
  GUIAzElIndicator*     m_ind_azel;

  //  QRadioButton*         m_cntrl_in_array;
  QComboBox*            m_cntrl_in_array;

  QWidgetStack*         m_cntrl_start_stop_stack;
  QPushButton*          m_cntrl_start;
  QPushButton*          m_cntrl_stop;
#ifdef USE_SOLICIT_TARGET
  QPushButton*          m_cntrl_solicit_target;
#endif
  unsigned              m_start_button_index;

#ifdef USE_INDIVIDUAL_PANIC_BUTTONS
  QPushButton*          m_cntrl_panic;
  int                   m_cntrl_panic_x;
#endif

  bool                  m_com_fail;
  bool                  m_is_stopped;
};

class GUIArrayPane: public QFrame, public GUITabPane
{
  Q_OBJECT
public:

  struct ScopeConfig
  {
    ScopeConfig(): color(), suppress_servo_fail_error(), stow_pos() { }
    QColor color;
    bool suppress_servo_fail_error;
    VTracking::StowObjectVector stow_pos;
  };

  GUIArrayPane(const std::vector<ScopeConfig>& scope, 
	       const SEphem::SphericalCoords& mean_earth_pos, bool grb = true, 
	       unsigned theme = 0,
	       QWidget* parent=0, const char* name=0);
  virtual ~GUIArrayPane();
  
  virtual void updateArray(const std::vector<GUIUpdateData>& ud);

  bool isInArray(unsigned id) const
  { return (id<m_tel.size())?m_tel[id]->isInArray():false; }

  bool canReTargetArray() const
  {
    unsigned ntel = m_tel.size();
    for(unsigned itel=0; itel<ntel; itel++)
      if(m_tel[itel]->isInArray() && !m_tel[itel]->canReTarget())
	return false;
    return true;
  }

  void getMeanUpdateData(VTracking::TelescopeController::TrackingState& state,
			 VTracking::TelescopeController::TrackingRequest& req,
			 const std::vector<GUIUpdateData>& ud_vec) const;

  void squarifyAzELIndicators();

signals:
  void goOne(unsigned id);
  void stopOne(unsigned id);
  void emergencyOne(unsigned id);
  void inArrayClickedOne(unsigned id);
  void setTargetOne(unsigned id, const VTracking::TargetObject* obj);
  void setTargetOne(unsigned id, const VTracking::TargetObject* obj,
		    SEphem::CorrectionParameters::DirectionPreference dp);
  void loadNewTargetList(unsigned);

public slots:
  // Array control methods ----------------------------------------------------

  void goArray(unsigned selector_id);
  void stopArray(unsigned selector_id);
  void sendSelectedTargetToArray(unsigned selector_id);
  void setFromTargetTable(int target);
  void setFromGRBTable(int target);
  void setScopeValuesToDisplay(ScopeValuesToDisplay sv);
  void configureFullArray();
  void changeDirPref(bool set_direction_as_array,
		     SEphem::CorrectionParameters::DirectionPreference dp);

  // Telescope control methods

  void sendSelectedTargetToOne(unsigned id);

  // Target methods -----------------------------------------------------------

  void syncWithTargetList(const VTracking::TargetList& target_list);
  void syncWithGRBList(const GUIGRBMonitor::GRBTriggerList& grb_list);

private slots:
  void collectGoOne(unsigned id);
  void collectStopOne(unsigned id);
  void collectEmergencyOne(unsigned id);
  void collectInArrayClickedOne(unsigned id);
  void passOnLoadNewTargetList(unsigned);

  void sendATeamStart();
  void sendRandomATeamQuote();

private:
  GUIArrayPane(const GUIArrayPane&);
  const GUIArrayPane& operator=(const GUIArrayPane&);

  void setDirectionChoice();
  void scheduleATeamQuote();

  SEphem::SphericalCoords        m_mean_earth_pos;

  ScopeValuesToDisplay           m_sv_demand;
  ScopeValuesToDisplay           m_sv;
  bool                           m_set_direction_as_array;
  SEphem::CorrectionParameters::DirectionPreference m_direction_preference;
  SEphem::CorrectionParameters::DirectionPreference m_direction_choice;

  std::vector<GUIUpdateData>     m_ud_vec;
  std::vector<bool>              m_com_failure_vec;
  std::vector<bool>              m_interlock_vec;
  std::vector<double>            m_az_deg_vec;

  QLineEdit*                     m_ut_date;
  QLineEdit*                     m_ut_utc;
  QLineEdit*                     m_ut_lmst;
  QLineEdit*                     m_ut_mjd;

  QLineEdit*                     m_src_name;
  QLineEdit*                     m_src_ra;
  QLineEdit*                     m_src_dec;
  QLineEdit*                     m_src_az;
  QLineEdit*                     m_src_el;

  std::vector<GUITelIndicator*>  m_tel;

  int                            m_lastx;

  VTracking::TargetObject*       m_array_object;
  GUIObjectSelector*             m_selector;
};

#endif // VTRACKING_GUIARRAYPANE_H

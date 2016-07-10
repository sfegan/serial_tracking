//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUISummaryDetailsPane.h
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
 * $Date: 2007/01/27 06:51:52 $
 * $Revision: 2.3 $
 * $Tag$
 *
 **/

#ifndef VTRACKING_GUISUMMARYDETAILAPANE_H
#define VTRACKING_GUISUMMARYDETAILAPANE_H

#include<set>

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

#include "TargetObject.h"

class GUISummaryPane: public QFrame, public GUITabPane
{
  Q_OBJECT
public:
  GUISummaryPane(unsigned identifier, 
		 const VTracking::StowObjectVector& stow_pos,
		 std::set<GUIObjectSelector*>* mirrors, bool grb,
		 QWidget* parent=0, const char* name=0);
  virtual ~GUISummaryPane();
  
  void update(const GUIUpdateData& ud);

  GUIObjectSelector* selector() { return m_selector; }

public slots:
  void setScopeValuesToDisplay(ScopeValuesToDisplay sv);

private:
  ScopeValuesToDisplay       m_sv_demand;
  ScopeValuesToDisplay       m_sv;

  QLineEdit*                 m_ut_date;
  QLineEdit*                 m_ut_utc;
  QLineEdit*                 m_ut_lmst;
  QLineEdit*                 m_src_name;
  QLineEdit*                 m_tel_az;
  QLineEdit*                 m_tel_el;
  QLineEdit*                 m_src_az;
  QLineEdit*                 m_src_el;
  QLineEdit*                 m_err_total;
  QLineEdit*                 m_err_az;
  QLineEdit*                 m_err_el;
  QLineEdit*                 m_err_eta;
  QLineEdit*                 m_azspeed;
  QLineEdit*                 m_azlimit;
  QLineEdit*                 m_azdrmode;
  QLineEdit*                 m_elspeed;
  QLineEdit*                 m_ellimit;
  QLineEdit*                 m_eldrmode;
  QLabel*                    m_azdrangle;
  QLabel*                    m_eldrangle;
  QLineEdit*                 m_interlock;
  QLineEdit*                 m_error;

  TelescopeIndicator*        m_az_ind;
  TelescopeIndicator*        m_el_ind;
  QSize                      m_azel_ind_size;

  int                        m_lastx;

  GUIObjectSelector*         m_selector;
};

class GUIDetailsPane: public QFrame, public GUITabPane
{
  Q_OBJECT
public:
  GUIDetailsPane(unsigned identifier, 
		 const VTracking::StowObjectVector& stow_pos,
		 std::set<GUIObjectSelector*>* mirrors, bool grb,
		 QWidget* parent=0, const char* name=0);
  virtual ~GUIDetailsPane();
  
  void update(const GUIUpdateData& ud);
  
  GUIObjectSelector* selector() { return m_selector; }

public slots:
  void setScopeValuesToDisplay(ScopeValuesToDisplay sv);

private:
  ScopeValuesToDisplay       m_sv_demand;
  ScopeValuesToDisplay       m_sv;

  QLineEdit*                 m_ut_date;
  QLineEdit*                 m_ut_utc;
  QLineEdit*                 m_ut_lmst;
  QLineEdit*                 m_ut_mjd;

  QLineEdit*                 m_src_name;

  QLineEdit*                 m_tel_ra;
  QLineEdit*                 m_tel_dec;
  QLineEdit*                 m_tel_az;
  QLineEdit*                 m_tel_el;
  QLineEdit*                 m_tel_l;
  QLineEdit*                 m_tel_b;

  QLineEdit*                 m_src_ra;
  QLineEdit*                 m_src_dec;
  QLineEdit*                 m_src_az;
  QLineEdit*                 m_src_el;
  QLineEdit*                 m_src_l;
  QLineEdit*                 m_src_b;

  QLineEdit*                 m_err_total;
  QLineEdit*                 m_err_el;
  QLineEdit*                 m_err_az;
  QLineEdit*                 m_err_eta;

  PositionerStatusWidget*    m_ps;

  GUIObjectSelector*         m_selector;
};

#endif // VTRACKING_GUISUMMARYDETAILAPANE_H

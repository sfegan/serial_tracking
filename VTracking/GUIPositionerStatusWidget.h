//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUIPositionerStatusWidget.h
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
 * $Date: 2006/04/04 17:06:58 $
 * $Revision: 2.0 $
 * $Tag$
 *
 **/

#ifndef VTRACKING_GUIPOSITIONERSTATUSWIDGET_H
#define VTRACKING_GUIPOSITIONERSTATUSWIDGET_H

#include<qframe.h>
#include<qlineedit.h>

#include"ScopeAPI.h"
#include"GUIMisc.h"

class DriveStatusWidget: public MyQGroupBox
{
  Q_OBJECT

public:
  enum Drive { DR_AZ, DR_EL };

  DriveStatusWidget(Drive dr, QWidget* parent = 0, const char* name = 0);
  virtual ~DriveStatusWidget();

  void update(const VTracking::ScopeAPI::DriveStatus& status, double speed);
  void blank();

private:
  Drive m_dr;

  double m_avg_speed;

  QLineEdit* m_positionLE;
  QLineEdit* m_speedLE;
  QLineEdit* m_cwUpLimitLE;
  QLineEdit* m_ccwDownLimitLE;
  QLineEdit* m_brakeLE;
  QLineEdit* m_completeLE;
  QLineEdit* m_interlockLE;
  QLineEdit* m_positionFaultLE;
  QLineEdit* m_servoFaultLE;
  QLineEdit* m_servoOnLE;
  QLineEdit* m_servoModeLE;
};

class PositionerStatusWidget: public QFrame
{
  Q_OBJECT

public:
  PositionerStatusWidget(QWidget* parent = 0, const char* name = 0);
  virtual ~PositionerStatusWidget();

  void update(const VTracking::ScopeAPI::PositionerStatus& status,
	      double azspeed, double elspeed);
  void blank();

private:
  DriveStatusWidget* m_az_status;
  DriveStatusWidget* m_el_status;

  QLineEdit* m_interlock;
  QLineEdit* m_interlock_az_pull_cord;
  QLineEdit* m_interlock_az_stow_pin;
  QLineEdit* m_interlock_el_stow_pin;
  QLineEdit* m_interlock_az_door_open;
  QLineEdit* m_interlock_el_door_open;
  QLineEdit* m_interlock_safe_switch;

  QLineEdit* m_coms_badframe;
  QLineEdit* m_coms_commandinvalid;
  QLineEdit* m_coms_ipoverrun;
  QLineEdit* m_coms_opoverrun;

  QLineEdit* m_user_relay1;
  QLineEdit* m_user_relay2;
  QLineEdit* m_user_adc1;
  QLineEdit* m_user_adc2;

  QLineEdit* m_misc_checksum_bad;
  QLineEdit* m_misc_hand_paddle;
  QLineEdit* m_misc_cablewrap;
  QLineEdit* m_misc_ccw;
};

#endif // VTRACKING_GUIPOSITIONERSTATUSWIDGET_H

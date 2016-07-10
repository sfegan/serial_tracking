//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUIPositionerStatusWidget.cpp
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
 * $Date: 2007/01/23 01:36:05 $
 * $Revision: 2.3 $
 * $Tag$
 *
 **/

#include<string>
#include<sstream>
#include<iostream>
#include<iomanip>

#include<qgrid.h>
#include<qlayout.h>
#include<qlineedit.h>
#include<qstring.h>
#include<qhbox.h>
#include<qlabel.h>
#include<qtooltip.h>

#include<Angle.h>

#include"ScopeAPI.h"

#include"GUIMisc.h"
#include"GUIPositionerStatusWidget.h"

#include"text.h"

using namespace SEphem;
using namespace VTracking;

// ----------------------------------------------------------------------------
// DRIVE STATUS WIDGET
// ----------------------------------------------------------------------------

DriveStatusWidget::DriveStatusWidget(Drive dr, QWidget* parent, 
				     const char* name)
  : MyQGroupBox(1,Qt::Horizontal,"Hello",parent,name), 
    m_dr(dr), m_avg_speed(0),
    m_positionLE(), m_speedLE(), m_cwUpLimitLE(), m_ccwDownLimitLE(), 
    m_brakeLE(), m_completeLE(), m_interlockLE(), m_positionFaultLE(),
    m_servoFaultLE(), m_servoOnLE(), m_servoModeLE()
{
  struct status_entry
  {
    QLineEdit** le;
    QString label;
    QString tt;
  };

  QString cw_lim_string(dr==DR_AZ?"CW Limit":"Up Limit");
  QString cw_lim_tt_string(dr==DR_AZ?TT_PSW_CW_LIMIT:TT_PSW_UP_LIMIT);

  QString ccw_lim_string(dr==DR_AZ?"CCW Limit":"Down Limit");
  QString ccw_lim_tt_string(dr==DR_AZ?TT_PSW_CC_LIMIT:TT_PSW_DN_LIMIT);

  struct status_entry status_entries[] =
    { { &m_cwUpLimitLE,    cw_lim_string,  cw_lim_tt_string },
      { &m_ccwDownLimitLE, ccw_lim_string, ccw_lim_tt_string },
      { &m_servoModeLE,    "Unknown",      TT_PSW_MODE },
      { &m_servoOnLE,      "Servo On",     TT_PSW_SERVO_ON },
      { &m_servoFaultLE,   "Servo Fault",  TT_PSW_SERVO_FAULT },
      { &m_brakeLE,        "Brake On",     TT_PSW_BRAKES }
    };

  QHBox* posbox=new QHBox(this,QString(name)+QString(" position box"));

  new QLabel("Angle ",posbox,QString(name)+QString(" position label"));
  m_positionLE=new InfoQLineEdit(posbox,QString(name)+QString(" position le"));

  new QLabel("  Speed ",posbox,QString(name)+QString(" speed label"));
  m_speedLE=new InfoQLineEdit(posbox, QString(name)+QString(" offset sb"));

  QGrid* statusbox=new QGrid(2,this,QString(name)+QString(" status box"));
  statusbox->setSpacing(2);
  //statusbox->setFrameShape(QFrame::Box);    
  //statusbox->setFrameShadow(QFrame::Plain);

  for(unsigned i=0; i<sizeof(status_entries)/sizeof(*status_entries);i++)
    {
      (*status_entries[i].le)=
	new InfoQLineEdit(statusbox,QString(name)+status_entries[i].label);
      (*status_entries[i].le)->setEnabled(false);
      //(*status_entries[i].le)->setFrameShape(QFrame::NoFrame);
      (*status_entries[i].le)->setMinimumWidth(100);
      (*status_entries[i].le)->setAlignment(Qt::AlignHCenter);
      (*status_entries[i].le)->setText(status_entries[i].label);
      QToolTip::add(*status_entries[i].le,status_entries[i].tt);
    }

  switch(m_dr)
    {
    case DR_AZ:
      setTitle("Azimuth Drive");
      QToolTip::add(m_positionLE, TT_PSW_AZ_ANG);
      QToolTip::add(m_speedLE, TT_PSW_AZ_SPEED);
      break;

    case DR_EL:
      setTitle("Elevation Drive");
      QToolTip::add(m_positionLE, TT_PSW_EL_ANG);
      QToolTip::add(m_speedLE, TT_PSW_EL_SPEED);
      break;
    }

}

DriveStatusWidget::~DriveStatusWidget()
{

}

void 
DriveStatusWidget::update(const ScopeAPI::DriveStatus& status, double speed)
{
  std::ostringstream pos_stream;
  pos_stream << std::showpos << std::fixed << std::setprecision(4) 
	     << status.driveangle_deg;
  m_positionLE->setText(MAKEDEG(pos_stream.str()));
  m_positionLE->setEnabled(true);

  m_avg_speed = speed + 0.8*(m_avg_speed-speed);

  std::ostringstream spd_stream;
  spd_stream << std::showpos << std:: fixed << std::setprecision(4) 
	     << m_avg_speed;
  m_speedLE->setText(MAKEDEG(spd_stream.str())+QString("/s"));
  m_speedLE->setEnabled(true);

  activateLE(status.limitCwUp,m_cwUpLimitLE,color_fg_warn,color_bg_warn);
  activateLE(status.limitCcwDown,m_ccwDownLimitLE,color_fg_warn,color_bg_warn);
  activateLE(!status.brakeReleased,m_brakeLE,color_fg_on,color_bg_on);
  activateLE(status.servo1Fail||status.servo2Fail,
	     m_servoFaultLE,color_fg_warn,color_bg_warn);
  activateLE(status.servoOn,m_servoOnLE,color_fg_on,color_bg_on);

  switch(status.driveMode)
    {
    case ScopeAPI::DM_STANDBY:
      m_servoModeLE->setText("Standby");
      activateLE(false,m_servoModeLE,color_fg_on,color_bg_on);
      break;
    case ScopeAPI::DM_SLEW:
      m_servoModeLE->setText("Slew");
      activateLE(true,m_servoModeLE,color_fg_on,color_bg_on);
      break;
    case ScopeAPI::DM_POINT:
      m_servoModeLE->setText("Point");
      activateLE(true,m_servoModeLE,color_fg_on,color_bg_on);
      break;
    case ScopeAPI::DM_SPIN:
      m_servoModeLE->setText("Spin");
      activateLE(true,m_servoModeLE,color_fg_warn,color_bg_warn);
      break;
    case ScopeAPI::DM_SECTOR_SCAN:
      m_servoModeLE->setText("Scan");
      activateLE(true,m_servoModeLE,color_fg_warn,color_bg_warn);
      break;
    case ScopeAPI::DM_RASTER:
      m_servoModeLE->setText("Raster");
      activateLE(true,m_servoModeLE,color_fg_warn,color_bg_warn);
      break;
    case ScopeAPI::DM_CHANGING:
      m_servoModeLE->setText("Changing");
      activateLE(true,m_servoModeLE,color_fg_warn,color_bg_warn);
      break;
    case ScopeAPI::DM_UNKNOWN:
      m_servoModeLE->setText("Unknown");
      activateLE(true,m_servoModeLE,color_fg_warn,color_bg_warn);
      break;
    }
}

void DriveStatusWidget::blank()
{
  m_positionLE->setText("");
  m_positionLE->setEnabled(false);
  m_speedLE->setText("");
  m_speedLE->setEnabled(false);
  activateLE(false,m_cwUpLimitLE,color_fg_on,color_bg_on);
  activateLE(false,m_ccwDownLimitLE,color_fg_on,color_bg_on);
  activateLE(false,m_brakeLE,color_fg_on,color_bg_on);
  activateLE(false,m_servoFaultLE,color_fg_on,color_bg_on);
  activateLE(false,m_servoOnLE,color_fg_on,color_bg_on);
  activateLE(false,m_servoModeLE,color_fg_on,color_bg_on);
}


// ----------------------------------------------------------------------------
// POSITIONER STATUS WIDGET
// ----------------------------------------------------------------------------

PositionerStatusWidget::
PositionerStatusWidget(QWidget* parent, const char* name):
  QFrame(parent,name),
  m_az_status(), m_el_status(),
  m_interlock(), m_interlock_az_pull_cord(), m_interlock_az_stow_pin(),
  m_interlock_el_stow_pin(), m_interlock_az_door_open(), 
  m_interlock_el_door_open(), m_interlock_safe_switch(), 
  m_coms_badframe(), m_coms_commandinvalid(), m_coms_ipoverrun(),
  m_coms_opoverrun(), 
  m_user_relay1(), m_user_relay2(), m_user_adc1(), m_user_adc2(),
  m_misc_checksum_bad(), m_misc_hand_paddle(), m_misc_cablewrap(), m_misc_ccw()
{
  m_az_status = new DriveStatusWidget(DriveStatusWidget::DR_AZ,this,
				      QString(name)+QString(" az status"));

  MyQGroupBox* interlockbox = 
    new MyQGroupBox(1,Qt::Horizontal,"Interlocks",this,
		    QString(name)+QString(" interlock box"));

  m_interlock = new InfoQLineEdit(interlockbox,
				  QString(name)+QString(" interlock"));
  m_interlock->setEnabled(false);
  m_interlock->setAlignment(Qt::AlignHCenter);
  m_interlock->setText("Interlock");
  QToolTip::add(m_interlock, TT_PSW_INTERLOCK);

  QGrid* interlockdetailbox =
    new QGrid(2,interlockbox,QString(name)+QString(" interlock details box"));
  interlockdetailbox->setSpacing(2);

  MyQGroupBox* combox = new MyQGroupBox(2,Qt::Horizontal,"Communication Status",
					this,QString(name)+" com box");
  MyQGroupBox* etcbox = new MyQGroupBox(1,Qt::Horizontal,"Miscellaneous",
					this,QString(name)+" etc box");
  MyQGroupBox* usrbox = new MyQGroupBox(2,Qt::Horizontal,"User Inputs",
					this,QString(name)+" usr box");

  QHBox* usr1 = new QHBox(usrbox,QString(name)+" adc 1 frame");
  usr1->setSpacing(2);
  new QLabel("ADC1",usr1,QString(name)+" adc 1 lab");
  m_user_adc1 = new InfoQLineEdit(usr1,QString(name)+" adc 1 le");
  m_user_adc1->setEnabled(false);
  QToolTip::add(m_user_adc1, TT_PSW_ADC1);

  QHBox* usr2 = new QHBox(usrbox,QString(name)+" adc 2 frame");
  usr2->setSpacing(2);
  new QLabel("ADC2",usr2,QString(name)+" adc 2 lab");
  m_user_adc2 = new InfoQLineEdit(usr2,QString(name)+" adc 2 le");
  m_user_adc2->setEnabled(false);
  QToolTip::add(m_user_adc2, TT_PSW_ADC2);

  QHBox* wrap = new QHBox(etcbox,QString(name)+" cable wrap frame");
  wrap->setSpacing(2);
  new QLabel("Az Cable Wrap",wrap,QString(name)+" cable wrap lab");
  m_misc_cablewrap = new InfoQLineEdit(wrap,QString(name)+" cable wrap le");
  m_misc_cablewrap->setEnabled(false);
  QToolTip::add(m_misc_cablewrap, TT_PSW_WRAP);

  m_misc_ccw = new InfoQLineEdit(wrap, QString(name)+"CCW");
  m_misc_ccw->setEnabled(false);
  m_misc_ccw->setAlignment(Qt::AlignHCenter);
  m_misc_ccw->setText("CCW");
  m_misc_ccw->setMinimumWidth(15);
  QToolTip::add(m_misc_ccw, TT_PSW_CCW);

  QHBox* cksum = new QHBox(etcbox,QString(name)+" checksum frame");
  cksum->setSpacing(2);

  struct status_entry
  {
    QWidget* box;
    QLineEdit** le;
    QString label;
    QString tt;
  };

  struct status_entry status_entries[] =
    {
      { interlockdetailbox, &m_interlock_az_pull_cord, "AzPullCord", 
	TT_PSW_AZ_PULL },
      { interlockdetailbox, &m_interlock_safe_switch , "SafeSwitch", 
	TT_PSW_SAFE },
      { interlockdetailbox, &m_interlock_az_stow_pin,  "AzStowPin", 
	TT_PSW_AZ_STOW },
      { interlockdetailbox, &m_interlock_el_stow_pin,  "ElStowPin",
	TT_PSW_ElSTOW },
      { interlockdetailbox, &m_interlock_az_door_open, "AzDoor",
	TT_PSW_AZ_DOOR },
      { interlockdetailbox, &m_interlock_el_door_open, "ElDoor",
	TT_PSW_EL_DOOR },
      { combox, &m_coms_badframe, "Bad Frame", TT_PSW_BAD_FRAME },
      { combox, &m_coms_commandinvalid, "Cmd Invalid", TT_PSW_BAD_CMD },
      { combox, &m_coms_ipoverrun, "I/P Overrun", TT_PSW_IP_OVERRUN },
      { combox, &m_coms_opoverrun, "O/P Overrun", TT_PSW_OP_OVERRUN },
      { usrbox, &m_user_relay1, "Relay1", TT_PSW_RELAY1 },
      { usrbox, &m_user_relay2, "Relay2", TT_PSW_RELAY2 },
      { cksum, &m_misc_checksum_bad, "Checksum Fail", TT_PSW_CHECKSUM },
      { cksum, &m_misc_hand_paddle, "Hand Paddle", TT_PSW_HAND_PADDLE },
    };
  
  for(unsigned i=0; i<sizeof(status_entries)/sizeof(*status_entries);i++)
    {
      (*status_entries[i].le)=
	new InfoQLineEdit(status_entries[i].box,
			  QString(name)+status_entries[i].label);
      (*status_entries[i].le)->setEnabled(false);
      (*status_entries[i].le)->setAlignment(Qt::AlignHCenter);
      (*status_entries[i].le)->setText(status_entries[i].label);
      (*status_entries[i].le)->setMinimumWidth(80);
      //(*status_entries[i].le)->setMinimumHeight(status_entries[i].height);
      //QFont f=(*status_entries[i].le)->font();
      //f.setPointSize(10);
      //(*status_entries[i].le)->setFont(f);
      QToolTip::add(*status_entries[i].le,status_entries[i].tt);
    }

  m_el_status = new DriveStatusWidget(DriveStatusWidget::DR_EL,this,
				      QString(name)+QString(" el status"));

  QGridLayout* my_layout = 
    new QGridLayout(this,2,5,0,5,QString(name)+QString(" layout"));

  my_layout->addMultiCellWidget(m_az_status,0,0,0,1);
  my_layout->addMultiCellWidget(interlockbox,0,0,2,2);
  my_layout->addMultiCellWidget(m_el_status,0,0,3,4);
  my_layout->addMultiCellWidget(combox,1,1,0,1);
  my_layout->addMultiCellWidget(etcbox,1,1,2,2);
  my_layout->addMultiCellWidget(usrbox,1,1,3,4);

  my_layout->setColStretch(0,1);
  my_layout->setColStretch(1,1);
  my_layout->setColStretch(2,0);
  my_layout->setColStretch(3,1);
  my_layout->setColStretch(4,1);
}

PositionerStatusWidget::~PositionerStatusWidget()
{
  // nothing to see here
}

void PositionerStatusWidget::
update(const ScopeAPI::PositionerStatus& status,double azspeed,double elspeed)
{
  m_az_status->update(status.az,azspeed);
  m_el_status->update(status.el,elspeed);
  activateLE(status.interlock,
	     m_interlock,color_fg_warn,color_bg_warn);
  activateLE(status.interlockAzPullCord,
	     m_interlock_az_pull_cord,color_fg_warn,color_bg_warn);
  activateLE(status.interlockAzStowPin,
	     m_interlock_az_stow_pin,color_fg_warn,color_bg_warn);
  activateLE(status.interlockElStowPin,
	     m_interlock_el_stow_pin,color_fg_warn,color_bg_warn);
  activateLE(status.interlockAzDoorOpen,
	     m_interlock_az_door_open,color_fg_warn,color_bg_warn);
  activateLE(status.interlockElDoorOpen,
	     m_interlock_el_door_open,color_fg_warn,color_bg_warn);
  activateLE(status.interlockSafeSwitch,
	     m_interlock_safe_switch,color_fg_warn,color_bg_warn);

  activateLE(status.msgBadFrame,
	     m_coms_badframe,color_fg_warn,color_bg_warn);
  activateLE(status.msgCommandInvalid,
	     m_coms_commandinvalid,color_fg_warn,color_bg_warn);
  activateLE(status.msgInputOverrun,
	     m_coms_ipoverrun,color_fg_warn,color_bg_warn);
  activateLE(status.msgOutputOverrun,
	     m_coms_opoverrun,color_fg_warn,color_bg_warn);

  activateLE(status.relay1,
	     m_user_relay1,color_fg_warn,color_bg_warn);
  activateLE(status.relay2,
	     m_user_relay2,color_fg_warn,color_bg_warn);

  activateLE(!status.checksumOK,
	     m_misc_checksum_bad,color_fg_warn,color_bg_warn);

  activateLE(!status.remoteControl,
	     m_misc_hand_paddle,color_fg_warn,color_bg_warn);

  m_user_adc1->setEnabled(true);
  m_user_adc2->setEnabled(true);
  m_misc_cablewrap->setEnabled(true);

  activateLE(status.azTravelledCCW,m_misc_ccw,color_fg_on,color_bg_on);
  
  std::ostringstream adc1stream;
  adc1stream << std::showpos << std::fixed << std::setprecision(4) 
	     << status.Analog1 << 'V';
  m_user_adc1->setText(adc1stream.str());

  std::ostringstream adc2stream;
  adc2stream << std::showpos << std::fixed << std::setprecision(4) 
	     << status.Analog2 << 'V';
  m_user_adc2->setText(adc2stream.str());

  std::ostringstream wrapstream;
  wrapstream << std::showpos << std::fixed << std::setprecision(2) 
	     << status.azCableWrap;
  m_misc_cablewrap->setText(wrapstream.str());
}

void PositionerStatusWidget::blank()
{
  m_az_status->blank();
  m_el_status->blank();
  activateLE(false,m_interlock,color_fg_warn,color_bg_warn);
  activateLE(false,m_interlock_az_pull_cord,color_fg_warn,color_bg_warn);
  activateLE(false,m_interlock_az_stow_pin,color_fg_warn,color_bg_warn);
  activateLE(false,m_interlock_el_stow_pin,color_fg_warn,color_bg_warn);
  activateLE(false,m_interlock_az_door_open,color_fg_warn,color_bg_warn);
  activateLE(false,m_interlock_el_door_open,color_fg_warn,color_bg_warn);
  activateLE(false,m_interlock_safe_switch,color_fg_warn,color_bg_warn);

  activateLE(false,m_coms_badframe,color_fg_warn,color_bg_warn);
  activateLE(false,m_coms_commandinvalid,color_fg_warn,color_bg_warn);
  activateLE(false,m_coms_ipoverrun,color_fg_warn,color_bg_warn);
  activateLE(false,m_coms_opoverrun,color_fg_warn,color_bg_warn);
  activateLE(false,m_user_relay1,color_fg_warn,color_bg_warn);
  activateLE(false,m_user_relay2,color_fg_warn,color_bg_warn);
  activateLE(false,m_misc_checksum_bad,color_fg_warn,color_bg_warn);
  activateLE(false,m_misc_hand_paddle,color_fg_warn,color_bg_warn);
  
  m_user_adc1->setEnabled(false); m_user_adc1->setText("");
  m_user_adc2->setEnabled(false); m_user_adc2->setText("");
  m_misc_cablewrap->setEnabled(false); m_misc_cablewrap->setText("");
  activateLE(false,m_misc_ccw,color_fg_on,color_bg_on);
}


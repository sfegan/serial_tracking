//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUISummaryDetailsPane.cpp
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
 * $Date: 2010/10/28 14:48:06 $
 * $Revision: 2.9 $
 * $Tag$
 *
 **/

#include <iomanip>

#include<qlayout.h>
#include<qtooltip.h>
#include<qvalidator.h>

#include<Exception.h>

#include"text.h"
#include"GUIPixmaps.h"
#include"GUIObjectSelector.h"
#include"GUISummaryDetailsPane.h"

static QColor azel_ind_normcolor(128,128,128);

using namespace SEphem;
using namespace VMessaging;
using namespace VTracking;

// ****************************************************************************
// ****************************************************************************
// ****************************************************************************
// 
// GUISummaryPane
//
// ****************************************************************************
// ****************************************************************************
// ****************************************************************************

GUISummaryPane::GUISummaryPane(unsigned identifier,
			       const VTracking::StowObjectVector& stow_pos,
			       std::set<GUIObjectSelector*>* mirrors, bool grb,
			       QWidget* parent, const char* name):
  QFrame(parent,name), GUITabPane(this),
  m_sv_demand(SV_AUTO), m_sv(SV_AUTO),
  m_ut_date(0), m_ut_utc(0), m_ut_lmst(0), m_src_name(0), 
  m_tel_az(0), m_tel_el(0), m_src_az(0), m_src_el(0), 
  m_err_total(0), m_err_az(0), m_err_el(0), m_err_eta(0), 
  m_azspeed(0), m_azlimit(0), m_azdrmode(0), m_elspeed(0),
  m_ellimit(0), m_eldrmode(0), m_azdrangle(0), m_eldrangle(0), m_interlock(0),
  m_error(0), m_az_ind(0), m_el_ind(0), m_azel_ind_size(), m_lastx(-1),
  m_selector(0)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  QString basename(name);

  QGridLayout* outerlayout = 
    new QGridLayout(this,7,4,5,5,basename+" outer layout");


  QGroupBox* utcbox = 
    new MyQGroupBox(1,Qt::Horizontal,"Time and Date",
		  this,basename+" utc box");
  QFrame* utcframe = new QFrame(utcbox,basename+" utc frame");
  QGridLayout* utclayout = 
    new QGridLayout(utcframe,1,8,0,5,basename+" utc layout");

  QGroupBox* nambox = 
    new MyQGroupBox(1,Qt::Horizontal,"Target Name",
		  this,basename+" nam box");
  QFrame* namframe = new QFrame(nambox,basename+" nam frame");
  QGridLayout* namlayout = 
    new QGridLayout(namframe,1,2,0,5,basename+" nam layout");  

  QGroupBox* telbox =
    new MyQGroupBox(1,Qt::Horizontal,"Telescope Position",
		  this,basename+" tel box");  
  QFrame* telframe = new QFrame(telbox,basename+" tel frame");
  QGridLayout* tellayout = 
    new QGridLayout(telframe,2,2,0,5,basename+" tel layout");

  QGroupBox* tarbox =
    new MyQGroupBox(1,Qt::Horizontal,"Target Position",
		  this,basename+" tar box");  
  QFrame* tarframe = new QFrame(tarbox,basename+" tar frame");
  QGridLayout* tarlayout = 
    new QGridLayout(tarframe,2,2,0,5,basename+" tar layout");

  QGroupBox* errbox = 
    new MyQGroupBox(1,Qt::Horizontal,"Tracking Error",
		  this,basename+" err box");
  QFrame* errframe = new QFrame(errbox,basename+" err frame");
  QGridLayout* errlayout = 
    new QGridLayout(errframe,2,11,0,5,basename+" err layout");

  QGroupBox* azbox = 
    new MyQGroupBox(1,Qt::Horizontal,"Azimuth Drive Status",
		  this,basename+" az box");
  QFrame* azframe = new QFrame(azbox,basename+" az frame");
  QGridLayout* azlayout = 
    new QGridLayout(azframe,2,2,0,5,basename+" az layout");

  QGroupBox* elbox = 
   new MyQGroupBox(1,Qt::Horizontal,"Elevation Drive Status",
		 this,basename+" el box");
  QFrame* elframe = new QFrame(elbox,basename+" el frame");
  QGridLayout* ellayout = 
    new QGridLayout(elframe,2,2,0,5,basename+" el layout");

  QFrame* azsframe = new QFrame(azframe,basename+" az speed frame");
  QGridLayout* azslayout = 
    new QGridLayout(azsframe,1,2,0,5,basename+" az speed layout");

  QFrame* elsframe = new QFrame(elframe,basename+" az speed frame");
  QGridLayout* elslayout = 
    new QGridLayout(elsframe,1,2,0,5,basename+" az speed layout");

  QFrame* aziframe = new QFrame(azframe,basename+" az ind frame");
  QGridLayout* azilayout = 
    new QGridLayout(aziframe,2,1,0,2,basename+" az ind layout");

  QFrame* eliframe = new QFrame(elframe,basename+" az ind frame");
  QGridLayout* elilayout = 
    new QGridLayout(eliframe,2,1,0,2,basename+" az ind layout");

  QGroupBox* intbox = 
    new MyQGroupBox(1,Qt::Horizontal,"Interlock",
		  this,basename+" interlock box");
  QFrame* intframe = new QFrame(intbox,basename+" int frame");
  QGridLayout* intlayout = 
    new QGridLayout(intframe,4,1,0,2,basename+" int layout");

  struct status_entry
  {
    QGridLayout* layout;
    int row;
    int col;
    int width;
    QLineEdit** le;
    QString label;
    QString texttemplate;
    double fontratio;
  };
  
  struct status_entry status_entries[] = {
    { utclayout, 0, 1, 1, &m_ut_date,   "UT Date",   "8888-88-88",       1.3 },
    { utclayout, 0, 4, 1, &m_ut_utc,    "UTC",       "88:88:88",         1.3 },
    { utclayout, 0, 7, 1, &m_ut_lmst,   "LMST",      "88:88:88",         1.3 },
    { namlayout, 0, 1, 1, &m_src_name,  "Target Name", "xxx",            1.3 },
    { tellayout, 0, 1, 1, &m_tel_az,    "Azimuth",   MAKEDEG("+888.88"), 2.0 },
    { tellayout, 2, 1, 1, &m_tel_el,    "Elevation", MAKEDEG("+88.88"),  2.0 },
    { tarlayout, 0, 1, 1, &m_src_az,    "Azimuth",   MAKEDEG("+888.88"), 2.0 },
    { tarlayout, 1, 1, 1, &m_src_el,    "Elevation", MAKEDEG("+88.88"),  2.0 },
    { errlayout, 0, 1, 1, &m_err_total, "Total",     MAKEDEG("+888.88"), 1.3 },
    { errlayout, 0, 4, 1, &m_err_az,    "Azimuth",   MAKEDEG("+888.88"), 1.3 },
    { errlayout, 0, 7, 1, &m_err_el,    "Elevation", MAKEDEG("+88.88"),  1.3 },
    { errlayout, 0,10, 1, &m_err_eta,   "Slew Time", "888:88",           1.3 },
    { azslayout, 0, 1, 1, &m_azspeed,   "Speed",     MAKEDEG("+8.888")+"/s", 1.3 },
    { elslayout, 0, 1, 1, &m_elspeed,   "Speed",     MAKEDEG("+8.888")+"/s", 1.3 },
    { azilayout, 0, 0, 1, &m_azlimit,   QString(),   "CCW Limit",        1.3 },
    { azilayout, 1, 0, 1, &m_azdrmode,  QString(),   "Standby",          1.3 },
    { elilayout, 0, 0, 1, &m_ellimit,   QString(),   "CCW Limit",        1.3 },
    { elilayout, 1, 0, 1, &m_eldrmode,  QString(),   "Standby",          1.3 },
    { elilayout, 0, 0, 1, &m_ellimit,   QString(),   "CCW Limit",        1.3 },
    { elilayout, 1, 0, 1, &m_eldrmode,  QString(),   "Standby",          1.3 },
    { intlayout, 1, 0, 1, &m_interlock, QString(),   "  Interlock  ",    1.8 },
    { intlayout, 2, 0, 1, &m_error,     QString(),   "Error",            1.8 }
  };

  for(unsigned i=0; i<sizeof(status_entries)/sizeof(*status_entries);i++)
    {
      struct status_entry* entry = status_entries+i;

      QLabel* lab(0);
      if(!entry->label.isNull())
	{
	  lab = new QLabel(entry->label,entry->layout->mainWidget(), 
			   basename+""+entry->label+QString(" label"));
	  entry->layout->addWidget(lab,entry->row,entry->col-1);
	}
      
      (*entry->le) = 
	new InfoQLineEdit(entry->texttemplate, entry->fontratio, true, false,
		  entry->layout->mainWidget(), basename+" "+entry->label+" le");

      entry->layout->addMultiCellWidget(*entry->le,entry->row,entry->row,
					entry->col,entry->col+entry->width-1);

      if((lab)&&(lab->font() != (*entry->le)->font()))
	lab->setFont((*entry->le)->font());
    } 
  
  activateLE(true, m_tel_az, color_fg_attn, color_bg_attn);
  activateLE(true, m_tel_el, color_fg_attn, color_bg_attn);

  for(unsigned i=0;i<8;i++)utclayout->setColStretch(i,0);
  utclayout->setColStretch(2,1);
  utclayout->setColStretch(5,1);

  namlayout->setColStretch(0,0);
  namlayout->setColStretch(1,1);

  for(unsigned i=0;i<2;i++)tellayout->setColStretch(i,0);
  tellayout->setColStretch(1,1);

  for(unsigned i=0;i<2;i++)tarlayout->setColStretch(i,0);
  tarlayout->setColStretch(1,1);

  for(unsigned i=0;i<11;i++)errlayout->setColStretch(i,0);
  errlayout->setColStretch(2,1);
  errlayout->setColStretch(5,1);
  errlayout->setColStretch(8,1);

  intlayout->setRowStretch(0,1);
  intlayout->setRowStretch(3,1);

  m_azdrangle = new MyQLabel(azframe,basename+" az drive angle");

  m_azlimit->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
  m_azdrmode->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);

  azslayout->setColStretch(1,1);

  azlayout->addMultiCellWidget(m_azdrangle,0,1,0,0);
  azlayout->addMultiCellWidget(azsframe,0,0,1,1);
  azlayout->addMultiCellWidget(aziframe,1,1,1,1);
  azlayout->setColStretch(0,0);
  azlayout->setColStretch(1,1);
    
  m_az_ind = new TelescopeIndicator(0,0,360,0);
  m_el_ind = new TelescopeIndicator(270,0,360,0);

  int height = m_azspeed->sizeHint().height()+
    m_azlimit->sizeHint().height()+m_azdrmode->sizeHint().height()+7;
  m_azel_ind_size = QSize(height,height);
  GUIPixmaps::instance()->setSumAzElSize(height);

  m_azdrangle->setPixmap(m_az_ind->draw(0,m_azel_ind_size,false,
					true,azel_ind_normcolor,true));
  
  m_eldrangle = new MyQLabel(elframe,basename+" el drive angle");

  m_ellimit->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
  m_eldrmode->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);

  elslayout->setColStretch(1,1);

  ellayout->addMultiCellWidget(m_eldrangle,0,1,0,0);
  ellayout->addMultiCellWidget(elsframe,0,0,1,1);
  ellayout->addMultiCellWidget(eliframe,1,1,1,1);
  ellayout->setColStretch(0,0);
  ellayout->setColStretch(1,1);
  
  m_eldrangle->setPixmap(m_el_ind->draw(0,m_azel_ind_size,false,
					true,azel_ind_normcolor,true));

  m_interlock->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
  m_error->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
  
  outerlayout->addMultiCellWidget(utcbox,0,0,0,3);
  outerlayout->addMultiCellWidget(nambox,1,1,0,3);
  outerlayout->addMultiCellWidget(telbox,2,2,0,1);
  outerlayout->addMultiCellWidget(tarbox,2,2,2,3);
  outerlayout->addMultiCellWidget(errbox,3,3,0,3);
  outerlayout->addMultiCellWidget(azbox,4,4,0,0);
  outerlayout->addMultiCellWidget(intbox,4,4,1,2);
  outerlayout->addMultiCellWidget(elbox,4,4,3,3);

  QFont f = this->font();
  f.setPointSize(f.pointSize()*13/10);

  m_selector = new GUIObjectSelector(identifier,
				     false, true, f, height, stow_pos, mirrors,
				     grb, this, basename+" selector");

  outerlayout->addMultiCellWidget(m_selector,6,6,0,3);
  
  outerlayout->setColStretch(0,0);
  outerlayout->setColStretch(1,2);
  outerlayout->setColStretch(2,2);
  outerlayout->setColStretch(3,0);
  outerlayout->setRowStretch(5,1);

  struct { QWidget* widget; const QString tip; } thetips[] = {
    { m_ut_date,   QString(TT_SUMDET_DATE) },
    { m_ut_utc,    QString(TT_SUMDET_UTC) },
    { m_ut_lmst,   QString(TT_SUMDET_LMST) },
    { m_src_name,  QString(TT_SUMDET_TARGET) },
    { m_tel_az,    QString(TT_SUMDET_TEL_AZ) },
    { m_tel_el,    QString(TT_SUMDET_TEL_AZ) },
    { m_src_az,    QString(TT_SUMDET_SRC_AZ) },
    { m_src_el,    QString(TT_SUMDET_SRC_EL) },
    { m_err_total, QString(TT_SUMDET_ERR_TOT) },
    { m_err_az,    QString(TT_SUMDET_ERR_AZ) },
    { m_err_el,    QString(TT_SUMDET_ERR_EL) },
    { m_err_eta,   QString(TT_SUMDET_ERR_ETA) },
    { m_azspeed,   QString(TT_SUMDET_AZ_SPEED) },
    { m_azlimit,   QString(TT_SUMDET_AZ_LIMIT) },
    { m_azdrmode,  QString(TT_SUMDET_AZ_MODE) },
    { m_elspeed,   QString(TT_SUMDET_EL_SPEED) },
    { m_ellimit,   QString(TT_SUMDET_EL_LIMIT) },
    { m_eldrmode,  QString(TT_SUMDET_EL_MODE) },
    { m_azdrangle, QString(TT_SUMDET_AZ_ANG) },
    { m_eldrangle, QString(TT_SUMDET_EL_ANG) },
    { m_interlock, QString(TT_SUMDET_INTERLOCK) },
    { m_error,     QString(TT_SUMDET_ERROR) },
  };
  
  for(unsigned i=0; i<sizeof(thetips)/sizeof(*thetips); i++)
    QToolTip::add(thetips[i].widget, thetips[i].tip);
}

GUISummaryPane::~GUISummaryPane()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  delete m_az_ind;
  delete m_el_ind;
}

void GUISummaryPane::update(const GUIUpdateData& ud)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  if(!isVisible())return;

  TelescopeController::TrackingState state = ud.tse.state;

  if(m_sv_demand == SV_AUTO)
    {
      if((ud.tar_object!=0)&&(!ud.tar_object->useCorrections()))
	m_sv = SV_ENCODER;
      else if(ud.tar_object!=0)m_sv = SV_SKY;
    }
  else
    {
      m_sv = m_sv_demand;
    }

  if(state != TelescopeController::TS_COM_FAILURE)
    {
      int x=int(floor(30*(1+sin(fmod(ud.mjd*86400/2,1)*Angle::sc_twoPi))));
      if((x!=m_lastx)||(ud.full_update)||(ud.replay))
	{
	  QColor pcolor=QColor(255-x,x,x);
	  //QColor pcolor=QColor(255-60+x,60-x,60-x);
	  m_lastx=x;
	  
	  QColor azindcolor = azel_ind_normcolor;
	  if((ud.tse.status.az.driveangle_deg >= WARNANGLE_CW)||
	     (ud.tse.status.az.driveangle_deg <= WARNANGLE_CC))
	    azindcolor=pcolor;
	  
	  if((ud.full_update)||(ud.replay)||
	     (ud.tse.status.az.driveangle_deg >= WARNANGLE_CW)||
	     (ud.tse.status.az.driveangle_deg <= WARNANGLE_CC))
	    m_azdrangle->
	      setPixmap(m_az_ind->draw(ud.tse.status.az.driveangle_deg,
				       m_azel_ind_size,true,true,
				       azindcolor,true));
      
	  QColor elindcolor = azel_ind_normcolor;
	  if((ud.tse.status.el.driveangle_deg >= WARNANGLE_UP)||
	     (ud.tse.status.el.driveangle_deg <= WARNANGLE_DN))
	    elindcolor=pcolor;
	  
	  if((ud.full_update)||(ud.replay)||
	     (ud.tse.status.el.driveangle_deg >= WARNANGLE_UP)||
	     (ud.tse.status.el.driveangle_deg <= WARNANGLE_DN))
	    m_eldrangle->
	      setPixmap(m_el_ind->draw(ud.tse.status.el.driveangle_deg,
				       m_azel_ind_size,true,true,
				       elindcolor,true));
	}

      if((ud.full_update)||(ud.replay))
	{
	  m_azdrangle->setEnabled(true);
	  m_eldrangle->setEnabled(true);

	  QString tel_az;
	  QString tel_el;
	  if(m_sv == SV_ENCODER)
	    {
	      tel_az = QString("Enc: ") 
		+Angle::makeDeg(ud.tse.status.az.driveangle_deg)
		.degString(2);
	      tel_el = QString("Enc: ")
		+ Angle::makeDeg(ud.tse.status.el.driveangle_deg)
		.degPM180String(2);
	    }
	  else
	    {
	      tel_az = ud.tel_azel.phi().degString(2);
	      tel_el = ud.tel_azel.latitude().degPM180String(2);
	    }
	  
	  m_tel_az->setText(MAKEDEG(tel_az));
	  m_tel_az->setEnabled(true);
	  m_tel_el->setText(MAKEDEG(tel_el));
	  m_tel_el->setEnabled(true);
	  
	  std::ostringstream az_speed_stream;
	  az_speed_stream << std::showpos << std::fixed 
			  << std::setprecision(3) << ud.az_smooth_speed;
	  m_azspeed->setText(MAKEDEG(az_speed_stream.str())+"/s");
	  m_azspeed->setEnabled(true);
	  
	  std::ostringstream el_speed_stream;
	  el_speed_stream << std::showpos << std::fixed 
			  << std::setprecision(3) << ud.el_smooth_speed;
	  m_elspeed->setText(MAKEDEG(el_speed_stream.str())+"/s");
	  m_elspeed->setEnabled(true);
	  
	  if(ud.tse.status.az.limitCwUp && ud.tse.status.az.limitCcwDown)
	    m_azlimit->setText("CW & CCW Limits");
	  else if(ud.tse.status.az.limitCwUp)
	    m_azlimit->setText("CW Limit");
	  else if(ud.tse.status.az.limitCcwDown)
	    m_azlimit->setText("CCW Limit");
	  else m_azlimit->setText("CW/CCW Limit");
	  activateLE(ud.tse.status.az.limitCwUp||ud.tse.status.az.limitCcwDown,
		     m_azlimit,color_fg_warn,color_bg_warn);
	  
	  if(ud.tse.status.el.limitCwUp && ud.tse.status.el.limitCcwDown)
	    m_ellimit->setText("Up & Down Limits");
	  else if(ud.tse.status.el.limitCwUp)
	    m_ellimit->setText("Up Limit");
	  else if(ud.tse.status.el.limitCcwDown)
	    m_ellimit->setText("Down Limit");
	  else m_ellimit->setText("Up/Down Limit");
	  activateLE(ud.tse.status.el.limitCwUp||ud.tse.status.el.limitCcwDown,
		     m_ellimit,color_fg_warn,color_bg_warn);
	  
	  switch(ud.tse.status.az.driveMode)
	    {
	    case ScopeAPI::DM_STANDBY:
	      m_azdrmode->setText("Standby");
	      activateLE(false,m_azdrmode,color_fg_on,color_bg_on);
	      break;
	    case ScopeAPI::DM_SLEW:
	      m_azdrmode->setText("Slew");
	      activateLE(true,m_azdrmode,color_fg_on,color_bg_on);
	      break;
	    case ScopeAPI::DM_POINT:
	      m_azdrmode->setText("Point");
	      activateLE(true,m_azdrmode,color_fg_on,color_bg_on);
	      break;
	    case ScopeAPI::DM_SPIN:
	      m_azdrmode->setText("Spin");
	      activateLE(true,m_azdrmode,color_fg_warn,color_bg_warn);
	      break;
	    case ScopeAPI::DM_SECTOR_SCAN:
	      m_azdrmode->setText("Scan");
	      activateLE(true,m_azdrmode,color_fg_warn,color_bg_warn);
	      break;
	    case ScopeAPI::DM_RASTER:
	      m_azdrmode->setText("Raster");
	      activateLE(true,m_azdrmode,color_fg_warn,color_bg_warn);
	      break;
	    case ScopeAPI::DM_CHANGING:
	      m_azdrmode->setText("Changing");
	      activateLE(true,m_azdrmode,color_fg_warn,color_bg_warn);
	      break;
	    case ScopeAPI::DM_UNKNOWN:
	      m_azdrmode->setText("Unknown");
	      activateLE(true,m_azdrmode,color_fg_warn,color_bg_warn);
	      break;
	    }
	  
	  switch(ud.tse.status.el.driveMode)
	    {
	    case ScopeAPI::DM_STANDBY:
	      m_eldrmode->setText("Standby");
	      activateLE(false,m_eldrmode,color_fg_on,color_bg_on);
	      break;
	    case ScopeAPI::DM_SLEW:
	      m_eldrmode->setText("Slew");
	      activateLE(true,m_eldrmode,color_fg_on,color_bg_on);
	      break;
	    case ScopeAPI::DM_POINT:
	      m_eldrmode->setText("Point");
	      activateLE(true,m_eldrmode,color_fg_on,color_bg_on);
	      break;
	    case ScopeAPI::DM_SPIN:
	      m_eldrmode->setText("Spin");
	      activateLE(true,m_eldrmode,color_fg_warn,color_bg_warn);
	      break;
	    case ScopeAPI::DM_SECTOR_SCAN:
	      m_eldrmode->setText("Scan");
	      activateLE(true,m_eldrmode,color_fg_warn,color_bg_warn);
	      break;
	    case ScopeAPI::DM_RASTER:
	      m_eldrmode->setText("Raster");
	      activateLE(true,m_eldrmode,color_fg_warn,color_bg_warn);
	      break;
	    case ScopeAPI::DM_CHANGING:
	      m_eldrmode->setText("Changing");
	      activateLE(true,m_eldrmode,color_fg_warn,color_bg_warn);
	      break;
	    case ScopeAPI::DM_UNKNOWN:
	      m_eldrmode->setText("Unknown");
	      activateLE(true,m_eldrmode,color_fg_warn,color_bg_warn);
	      break;
	    }
      
	  bool interlock =
	    ud.tse.status.interlock||
	    ud.tse.status.interlockAzPullCord||
	    ud.tse.status.interlockAzStowPin||
	    ud.tse.status.interlockElStowPin||
	    ud.tse.status.interlockAzDoorOpen||
	    ud.tse.status.interlockElDoorOpen||
	    ud.tse.status.interlockSafeSwitch||
	    !ud.tse.status.remoteControl;
	  
	  activateLE(interlock,m_interlock,color_fg_warn,color_bg_warn);
	  
	  bool warn =
	    ud.tse.status.az.servo1Fail||
	    ud.tse.status.az.servo2Fail||
	    ud.tse.status.az.positionFault||
	    ud.tse.status.el.servo1Fail||
	    ud.tse.status.el.servo2Fail||
	    ud.tse.status.el.positionFault||
	    !ud.tse.status.checksumOK||
	    ud.tse.status.msgBadFrame||
	    ud.tse.status.msgCommandInvalid||
	    ud.tse.status.msgInputOverrun||
	    ud.tse.status.msgOutputOverrun;
	  
	  activateLE(warn,m_error,color_fg_warn,color_bg_warn);
	}
    }
  else if((ud.full_update)||(ud.replay))
    {
      m_tel_az->setText("");
      m_tel_az->setEnabled(false);
      m_tel_el->setText("");
      m_tel_el->setEnabled(false);
      
      m_azdrangle->
	setPixmap(m_az_ind->draw(ud.tse.status.az.driveangle_deg,
				 m_azel_ind_size,false));
      m_eldrangle->
	setPixmap(m_el_ind->draw(ud.tse.status.el.driveangle_deg,
				 m_azel_ind_size,false));
      
      m_azdrangle->setEnabled(false);
      m_eldrangle->setEnabled(false);

      m_azspeed->setText("");
      m_azspeed->setEnabled(false);
      m_elspeed->setText("");
      m_elspeed->setEnabled(false);
      
      m_azlimit->setText("CW/CCW Limit");
      activateLE(false,m_azlimit,color_fg_warn,color_bg_warn);
      m_ellimit->setText("Up/Down Limit");
      activateLE(false,m_ellimit,color_fg_warn,color_bg_warn);

      m_azdrmode->setText("Unknown");
      activateLE(false,m_azdrmode,color_fg_warn,color_bg_warn);
      m_eldrmode->setText("Unknown");
      activateLE(false,m_eldrmode,color_fg_warn,color_bg_warn);

      activateLE(false,m_interlock,color_fg_warn,color_bg_warn);
      activateLE(false,m_error,color_fg_warn,color_bg_warn);
    }

  m_ut_date->setText(ud.date_string);
  m_ut_utc->setText(ud.timeangle.hmsString(0));
  m_ut_lmst->setText(ud.lmst.hmsString(0));

  if(ud.tar_object)
    {
      if((ud.full_update)||(ud.replay))
	{
	  m_src_name->setText(ud.tar_object->targetName(ud.mjd));
	  m_src_name->setEnabled(true);
	}

      QString src_az;
      QString src_el;
      if(m_sv == SV_ENCODER)
	{
	  src_az = QString("Enc: ")
	    + Angle::makeRad(ud.tar_az_driveangle).degString(2);
	  src_el = QString("Enc: ")
	    + Angle::makeRad(ud.tar_el_driveangle).degPM180String(2);
	}
      else
	{
	  src_az = ud.tar_azel.phi().degString(2);
	  src_el = ud.tar_azel.latitude().degPM180String(2);
	}

      m_src_az->setText(MAKEDEG(src_az));
      m_src_az->setEnabled(true);
      m_src_el->setText(MAKEDEG(src_el));
      m_src_el->setEnabled(true);

      if(!ud.in_limits)
	{
	  m_src_az->setPaletteBackgroundColor(color_bg_warn);
	  m_src_az->setPaletteForegroundColor(color_fg_warn);
	  m_src_el->setPaletteBackgroundColor(color_bg_warn);
	  m_src_el->setPaletteForegroundColor(color_fg_warn);
	}
      else if((ud.target_moves)&&(fabs(ud.tar_az_speed)>ud.az_speed_limit))
	{
	  m_src_az->unsetPalette();
	  if(ud.tv.tv_usec/500000==0)
	    {
	      m_src_az->setPaletteBackgroundColor(color_bg_warn);
	      m_src_az->setPaletteForegroundColor(color_fg_warn);
	    }
	  m_src_el->unsetPalette();
	}
      else if(!ud.in_limits30)
	{
	  m_src_az->setPaletteBackgroundColor(color_bg_attn);
	  m_src_az->setPaletteForegroundColor(color_fg_attn);
	  m_src_el->setPaletteBackgroundColor(color_bg_attn);
	  m_src_el->setPaletteForegroundColor(color_fg_attn);
	}
      else if(((ud.target_moves)&&
	       (fabs(ud.tar_max_az_speed)>ud.az_speed_limit))||
	      ((Angle::toDeg(ud.tar_az_driveangle)>90)&&
	       (Angle::toDeg(fabs(ud.tar_az_driveangle-
				  ud.tar_az_driveangle30))>270)))
	{
	  m_src_az->unsetPalette();
	  if(ud.tv.tv_usec/500000==0)
	    {
	      m_src_az->setPaletteBackgroundColor(color_bg_attn);
	      m_src_az->setPaletteForegroundColor(color_fg_attn);
	    }
	  m_src_el->unsetPalette();
	}
      else
	{
	  m_src_az->unsetPalette();
	  m_src_el->unsetPalette();
	}

      if(state != TelescopeController::TS_COM_FAILURE)
	{
	  m_err_total->setText(ud.sep.deg180String(2));
          m_err_total->setEnabled(true);

	  if((ud.latest_req==TelescopeController::REQ_TRACK)&&
	     (ud.sep.deg()>0.05))
	    {
	      m_err_total->setPaletteBackgroundColor(color_bg_warn);
	      m_err_total->setPaletteForegroundColor(color_fg_warn);
	    }
	  else 
	    {
	      m_err_total->unsetPalette();
	    }

	  if(ud.az_sep>0)
	    m_err_az->setText(MAKEDEG(Angle::makeDeg(ud.az_sep).degString(2)));
	  else 
	    {
	      QString txt = MAKEDEG(Angle::makeDeg(-ud.az_sep).degString(2));
	      m_err_az->setText(QString("-")+txt.right(txt.length()-1));
	    }
	  m_err_az->setEnabled(true);

	  if(ud.el_sep>0)
	    m_err_el->setText(MAKEDEG(Angle::makeDeg(ud.el_sep).
				      degPM180String(2)));
	  else
	    {
	      QString txt = MAKEDEG(Angle::makeDeg(-ud.el_sep).
				    degPM180String(2));
	      m_err_el->setText(QString("-")+txt.right(txt.length()-1));
	    }
	  m_err_el->setEnabled(true);


	  if((state==TelescopeController::TS_TRACK)&&(ud.sep.deg()<0.05))
	    {
	      m_err_eta->setText("");
	      m_err_eta->setEnabled(false);
	    }
	  else
	    {
	      m_err_eta->setText(ud.eta_string);
	      m_err_eta->setEnabled(true);
	    }
	}
      else
	{
	  m_err_total->setText("");
	  m_err_total->setEnabled(false);
	  m_err_az->setText("");
	  m_err_az->setEnabled(false);
	  m_err_el->setText("");
	  m_err_el->setEnabled(false);
	  m_err_eta->setText("");
	  m_err_eta->setEnabled(false);
	}
    }
  else if((ud.full_update)||(ud.replay))
    {
      m_src_name->setText("");
      m_src_name->setEnabled(false);
      m_src_az->setText("");
      m_src_az->setEnabled(false);
      m_src_el->setText("");
      m_src_el->setEnabled(false);

      m_err_total->setText("");
      m_err_total->setEnabled(false);
      m_err_az->setText("");
      m_err_az->setEnabled(false);
      m_err_el->setText("");
      m_err_el->setEnabled(false);
      m_err_eta->setText("");
      m_err_eta->setEnabled(false);
    }
  
  if((ud.full_update)||(ud.replay))
    m_selector->updateButtons(controlsEnabled(), ud.tse.state, ud.tse.req);

  m_selector->animateButtons();
}

void GUISummaryPane::setScopeValuesToDisplay(ScopeValuesToDisplay sv)
{
  m_sv_demand = sv;
}

// ****************************************************************************
// ****************************************************************************
// ****************************************************************************
// 
// GUIDetailsPane
//
// ****************************************************************************
// ****************************************************************************
// ****************************************************************************

GUIDetailsPane::GUIDetailsPane(unsigned identifier,
			       const VTracking::StowObjectVector& stow_pos,
			       std::set<GUIObjectSelector*>* mirrors, 
			       bool grb, 
			       QWidget* parent, const char* name):
  QFrame(parent,name), GUITabPane(this),
  m_sv_demand(SV_AUTO), m_sv(SV_AUTO),
  m_ut_date(0), m_ut_utc(0), m_ut_lmst(0), m_ut_mjd(0), m_src_name(0),
  m_tel_ra(0), m_tel_dec(0), m_tel_az(0), m_tel_el(0), m_tel_l(0), m_tel_b(0),
  m_src_ra(0), m_src_dec(0), m_src_az(0), m_src_el(0), m_src_l(0), m_src_b(0), 
  m_err_total(0), m_err_el(0), m_err_az(0), m_err_eta(0), m_ps(0),
  m_selector(0)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  QString basename(name);

  QGridLayout* mainlayout = new QGridLayout(this,7,2,5,5,
					    basename+" main layout");

  MyQGroupBox* utcbox = new MyQGroupBox(1,Qt::Vertical,"Time and Date",
					this,basename+" ut box");
  MyQGroupBox* nambox = new MyQGroupBox(2,Qt::Horizontal,"Target Name",
					this,basename+" tel box");
  MyQGroupBox* telbox = new MyQGroupBox(6,Qt::Horizontal,"Telescope Position",
					this,basename+" tel box");
  MyQGroupBox* srcbox = new MyQGroupBox(6,Qt::Horizontal,"Target Position",
					this,basename+" src box");
  MyQGroupBox* errbox = new MyQGroupBox(1,Qt::Vertical,"Tracking Error",
					this,basename+" error box");

  struct status_entry
  {
    QFrame* box;
    QLineEdit** le;
    QString label;
    QString tooltip;
    int width;
  };
  
  struct status_entry status_entries[] = {
    { utcbox,  &m_ut_date,   "UT Date",      TT_SUMDET_DATE,              75 },
    { utcbox,  &m_ut_utc,    "UTC",          TT_SUMDET_UTC,               75 },
    { utcbox,  &m_ut_lmst,   "LMST",         TT_SUMDET_LMST,              75 },
    { utcbox,  &m_ut_mjd,    "MJD",          TT_SUMDET_MJD,               75 },
    { nambox,  &m_src_name,  "Target Name",  TT_SUMDET_TARGET,           400 },
    { telbox,  &m_tel_az,    "Az",           TT_SUMDET_TEL_AZ,            80 },
    { telbox,  &m_tel_ra,    "RA",           TT_SUMDET_TEL_RA,            70 },
    { telbox,  &m_tel_l,     "GLon",         TT_SUMDET_TEL_GLON,          70 },
    { telbox,  &m_tel_el,    "El",           TT_SUMDET_TEL_EL,            80 },
    { telbox,  &m_tel_dec,   "Dec",          TT_SUMDET_TEL_DEC,           70 },
    { telbox,  &m_tel_b,     "GLat",         TT_SUMDET_TEL_GLAT,          70 },
    { srcbox,  &m_src_az,    "Az",           TT_SUMDET_SRC_AZ,            80 },
    { srcbox,  &m_src_ra,    "RA",           TT_SUMDET_SRC_RA,            70 },
    { srcbox,  &m_src_l,     "GLon",         TT_SUMDET_SRC_GLON,          70 },
    { srcbox,  &m_src_el,    "El",           TT_SUMDET_SRC_EL,            80 },
    { srcbox,  &m_src_dec,   "Dec",          TT_SUMDET_SRC_DEC,           70 },
    { srcbox,  &m_src_b,     "GLat",         TT_SUMDET_SRC_GLAT,          70 },
    { errbox,  &m_err_total, "Combined",     TT_SUMDET_ERR_TOT,           65 },
    { errbox,  &m_err_az,    "Az",           TT_SUMDET_ERR_AZ,            65 },
    { errbox,  &m_err_el,    "El",           TT_SUMDET_ERR_EL,            65 },
    { errbox,  &m_err_eta,   "Slew Time",    TT_SUMDET_ERR_ETA,           65 }
  };

  for(unsigned i=0; i<sizeof(status_entries)/sizeof(*status_entries);i++)
    {
      new QLabel(status_entries[i].label,status_entries[i].box, 
		 basename+" "+status_entries[i].label+" label");
      (*status_entries[i].le)=
	new InfoQLineEdit(status_entries[i].box,
			  basename+" "+status_entries[i].label+" lineedit");
      //(*status_entries[i].le)->setEnabled(false);
      (*status_entries[i].le)->setMinimumWidth(status_entries[i].width);
      //(*status_entries[i].le)->setAlignment(Qt::AlignHCenter);
      if(status_entries[i].tooltip != "")
	QToolTip::add(*status_entries[i].le,status_entries[i].tooltip);
    } 
  
  activateLE(true, m_tel_az, color_fg_attn, color_bg_attn);
  activateLE(true, m_tel_el, color_fg_attn, color_bg_attn);
  
  // --------------------------------------------------------------------------
  // POSITIONER STATUS WIDGETS
  // --------------------------------------------------------------------------
  
  m_ps = new PositionerStatusWidget(this,basename+" positioner");
  
  // --------------------------------------------------------------------------
  // CONTROL BUTTONS
  // --------------------------------------------------------------------------

  int height;
  m_selector = new GUIObjectSelector(identifier, true, true, font(), height,
				     stow_pos, mirrors, grb,
				     this, basename+" selector");

  // --------------------------------------------------------------------------
  // MAKE THE LAYOUT
  // --------------------------------------------------------------------------
  
  mainlayout->addMultiCellWidget(utcbox,0,0,0,1);
  mainlayout->addMultiCellWidget(nambox,1,1,0,1);
  mainlayout->addWidget(telbox,2,0);
  mainlayout->addWidget(srcbox,2,1);
  mainlayout->addMultiCellWidget(errbox,3,3,0,1);
  mainlayout->addMultiCellWidget(m_ps,4,4,0,1);
  mainlayout->addMultiCellWidget(m_selector,6,6,0,1);
  mainlayout->setRowStretch(0,0);
  mainlayout->setRowStretch(1,0);
  mainlayout->setRowStretch(2,0);
  mainlayout->setRowStretch(3,0);
  mainlayout->setRowStretch(4,0);
  mainlayout->setRowStretch(5,1);
  mainlayout->setRowStretch(6,0);
}

GUIDetailsPane::~GUIDetailsPane()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
}

void GUIDetailsPane::update(const GUIUpdateData& ud)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  if(!isVisible())return;

  TelescopeController::TrackingState state = ud.tse.state;

  if(m_sv_demand == SV_AUTO)
    {
      if((ud.tar_object!=0)&&(!ud.tar_object->useCorrections()))
	m_sv = SV_ENCODER;
      else if(ud.tar_object!=0)m_sv = SV_SKY;
    }
  else
    {
      m_sv = m_sv_demand;
    }

  if((ud.full_update)||(ud.replay))
    {
      if(state != TelescopeController::TS_COM_FAILURE)
	m_ps->update(ud.tse.status,
		     ud.tse.az_driveangle_estimated_speed_dps,
		     ud.tse.el_driveangle_estimated_speed_dps);
      else
	m_ps->blank();
    }

  if(state != TelescopeController::TS_COM_FAILURE)
    {
      activateLE(true, m_tel_az, color_fg_attn, color_bg_attn);
      activateLE(true, m_tel_el, color_fg_attn, color_bg_attn);

      QString tel_az;
      QString tel_el;
      QString tel_ra;
      QString tel_dec;
      QString tel_l;
      QString tel_b;

      if(m_sv == SV_ENCODER)
	{
	  tel_az = QString("Enc: ") 
	    +Angle::makeDeg(ud.tse.status.az.driveangle_deg)
	    .degString(4);
	  tel_el = QString("Enc: ")
	    + Angle::makeDeg(ud.tse.status.el.driveangle_deg)
	    .degPM180String(4);
	  tel_ra = "Enc: N/A";
	  tel_dec = "Enc: N/A";
	  tel_l = "Enc: N/A";
	  tel_b = "Enc: N/A";
	}
      else
	{
	  tel_az = ud.tel_azel.phi().degString(4);
	  tel_el = ud.tel_azel.latitude().degPM180String(4);
	  tel_ra = ud.tel_radec.phi().hmsString(0);
	  tel_dec = ud.tel_radec.latitude().dmsString(0);
	  tel_l = MAKEDEG(ud.tel_gal.phi().degString(3));
	  tel_b = MAKEDEG(ud.tel_gal.latitude().degPM180String(3));
	}

      m_tel_ra->setText(tel_ra);
      m_tel_ra->setEnabled(true);
      m_tel_dec->setText(tel_dec);
      m_tel_dec->setEnabled(true);
      m_tel_az->setText(MAKEDEG(tel_az));
      m_tel_az->setEnabled(true);
      m_tel_el->setText(MAKEDEG(tel_el));
      m_tel_el->setEnabled(true);
      m_tel_l->setText(tel_l);
      m_tel_l->setEnabled(true);
      m_tel_b->setText(tel_b);
      m_tel_b->setEnabled(true);	  
    }
  else
    {
      activateLE(false, m_tel_az, color_fg_attn, color_bg_attn);
      activateLE(false, m_tel_el, color_fg_attn, color_bg_attn);
      m_tel_ra->setText("");
      m_tel_ra->setEnabled(false);
      m_tel_dec->setText("");
      m_tel_dec->setEnabled(false);
      m_tel_az->setText("");
      m_tel_az->setEnabled(false);
      m_tel_el->setText("");
      m_tel_el->setEnabled(false);
      m_tel_l->setText("");
      m_tel_l->setEnabled(false);
      m_tel_b->setText("");
      m_tel_b->setEnabled(false);
    }
  
  // --------------------------------------------------------------------------
  // Update Times Etc
  // --------------------------------------------------------------------------

  m_ut_date->setText(ud.date_string);
  m_ut_utc->setText(ud.timeangle.hmsString(1));
  m_ut_lmst->setText(ud.lmst.hmsString(1));
  m_ut_mjd->setText(QString::number(ud.mjd,'f',6));

  if(ud.tar_object)
    {
      QString src_az;
      QString src_el;

      QString src_ra;
      QString src_dec;
      QString src_l;
      QString src_b;
      if(m_sv == SV_ENCODER)
	{
	  src_az = QString("Enc: ")
	    + Angle::makeRad(ud.tar_az_driveangle).degString(4);
	  src_el = QString("Enc: ")
	    + Angle::makeRad(ud.tar_el_driveangle).degPM180String(4);
	  src_ra="Enc: N/A";
	  src_dec="Enc: N/A";
	  src_l="Enc: N/A";
	  src_b="Enc: N/A";
	}
      else
	{
	  src_az = ud.tar_azel.phi().degString(2);
	  src_el = ud.tar_azel.latitude().degPM180String(4);
	  src_ra=ud.tar_radec.phi().hmsString(0);
	  src_dec=ud.tar_radec.latitude().dmsString(0);
	  src_l=MAKEDEG(ud.tar_gal.phi().degString(3));
	  src_b=MAKEDEG(ud.tar_gal.latitude().degPM180String(3));
	}
	  
      m_src_az->setText(MAKEDEG(src_az));
      m_src_az->setEnabled(true);
      m_src_el->setText(MAKEDEG(src_el));
      m_src_el->setEnabled(true);

      if(!ud.in_limits)
	{
	  m_src_az->setPaletteBackgroundColor(color_bg_warn);
	  m_src_az->setPaletteForegroundColor(color_fg_warn);
	  m_src_el->setPaletteBackgroundColor(color_bg_warn);
	  m_src_el->setPaletteForegroundColor(color_fg_warn);
	}
      else if((ud.target_moves)&&(fabs(ud.tar_az_speed)>ud.az_speed_limit))
	{
	  m_src_az->unsetPalette();
	  if(ud.tv.tv_usec/500000==0)
	    {
	      m_src_az->setPaletteBackgroundColor(color_bg_warn);
	      m_src_az->setPaletteForegroundColor(color_fg_warn);
	    }
	  m_src_el->unsetPalette();
	}
      else if(!ud.in_limits30)
	{
	  m_src_az->setPaletteBackgroundColor(color_bg_attn);
	  m_src_az->setPaletteForegroundColor(color_fg_attn);
	  m_src_el->setPaletteBackgroundColor(color_bg_attn);
	  m_src_el->setPaletteForegroundColor(color_fg_attn);

	}
      else if(((ud.target_moves)&&
	       (fabs(ud.tar_max_az_speed)>ud.az_speed_limit))||
	      ((Angle::toDeg(ud.tar_az_driveangle)>90)&&
	       (Angle::toDeg(fabs(ud.tar_az_driveangle-
				  ud.tar_az_driveangle30))>270)))
	{
	  m_src_az->unsetPalette();
	  if(ud.tv.tv_usec/500000==0)
	    {
	      m_src_az->setPaletteBackgroundColor(color_bg_attn);
	      m_src_az->setPaletteForegroundColor(color_fg_attn);
	    }
	  m_src_el->unsetPalette();
	}
      else
	{
	  m_src_az->unsetPalette();
	  m_src_el->unsetPalette();
	}

      if((ud.full_update)||(ud.replay))
	{
	  m_src_name->setText(ud.tar_object->targetName(ud.mjd));
	  m_src_name->setEnabled(true);
	}
      m_src_ra->setText(src_ra);
      m_src_ra->setEnabled(true);
      m_src_dec->setText(src_dec);
      m_src_dec->setEnabled(true);
      m_src_l->setText(src_l);
      m_src_l->setEnabled(true);
      m_src_b->setText(src_b);
      m_src_b->setEnabled(true);

      if(state != TelescopeController::TS_COM_FAILURE)
	{
	  m_err_total->setText(ud.sep.deg180String(4));
	  m_err_total->setEnabled(true);

	  if((ud.latest_req==TelescopeController::REQ_TRACK)&&
	     (ud.sep.deg()>0.05))
	    {
	      m_err_total->setPaletteBackgroundColor(color_bg_warn);
	      m_err_total->setPaletteForegroundColor(color_fg_warn);
	    }
	  else 
	    {
	      m_err_total->unsetPalette();
	    }
	    
	  if(ud.az_sep>0)
	    m_err_az->setText(MAKEDEG(Angle::makeDeg(ud.az_sep).degString(4)));
	  else 
	    {
	      QString txt = MAKEDEG(Angle::makeDeg(-ud.az_sep).degString(4));
	      m_err_az->setText(QString("-")+txt.right(txt.length()-1));
	    }
	  m_err_az->setEnabled(true);

	  if(ud.el_sep>0)
	    m_err_el->setText(MAKEDEG(Angle::makeDeg(ud.el_sep).
				      degPM180String(4)));
	  else
	    {
	      QString txt = MAKEDEG(Angle::makeDeg(-ud.el_sep).
				    degPM180String(4));
	      m_err_el->setText(QString("-")+txt.right(txt.length()-1));
	    }
	  m_err_el->setEnabled(true);

	  if((state==TelescopeController::TS_TRACK)&&(ud.sep.deg()<0.05))
	    {
	      m_err_eta->setText("");
	      m_err_eta->setEnabled(false);
	    }
	  else
	    {
	      m_err_eta->setText(ud.eta_string);
	      m_err_eta->setEnabled(true);
	    }
	}
      else
	{
	  m_err_total->setText("");
	  m_err_total->setEnabled(false);
	  m_err_az->setText("");
	  m_err_az->setEnabled(false);
	  m_err_el->setText("");
	  m_err_el->setEnabled(false);
	  m_err_eta->setText("");
	  m_err_eta->setEnabled(false);
	}
    }
  else if((ud.full_update)||(ud.replay))
    {
      m_src_name->setText("");
      m_src_name->setEnabled(false);
      m_src_ra->setText("");
      m_src_ra->setEnabled(false);
      m_src_dec->setText("");
      m_src_dec->setEnabled(false);
      m_src_az->setText("");
      m_src_az->setEnabled(false);
      m_src_el->setText("");
      m_src_el->setEnabled(false);
      m_src_l->setText("");
      m_src_l->setEnabled(false);
      m_src_b->setText("");
      m_src_b->setEnabled(false);

      m_err_total->setText("");
      m_err_total->setEnabled(false);
      m_err_az->setText("");
      m_err_az->setEnabled(false);
      m_err_el->setText("");
      m_err_el->setEnabled(false);
      m_err_eta->setText("");
      m_err_eta->setEnabled(false);
    }

  if((ud.full_update)||(ud.replay))
    m_selector->updateButtons(controlsEnabled(), ud.tse.state, ud.tse.req);

  m_selector->animateButtons();
}

void GUIDetailsPane::setScopeValuesToDisplay(ScopeValuesToDisplay sv)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  m_sv_demand = sv;
}

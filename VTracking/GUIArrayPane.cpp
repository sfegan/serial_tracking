//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUIArray.cpp
 * \ingroup VTracking
 * \brief Array GUI
 *
 * Original Author: Stephen Fegan
 * Start Date: 2006-07-14
 * $Author: sfegan $
 * $Date: 2010/10/28 14:48:05 $
 * $Revision: 2.28 $
 * $Tag$
 *
 **/

#include<cstdlib>
#include<unistd.h>

#include<qfiledialog.h>
#include<qmenubar.h>
#include<qtimer.h>
#include<qtooltip.h>
#include<qpixmap.h>
#include<qimage.h>
#include<qlayout.h>
#include<qmessagebox.h>
#include<qsound.h>

#include<Messenger.h>
#include<VSDataConverter.hpp>
#include<Angle.h>
#include<Astro.h>

#include"GUIPixmaps.h"
#include"GUIArrayPane.h"
#include"text.h"

using namespace SEphem;
using namespace VERITAS;
using namespace VTracking;
using namespace VMessaging;

#include"pixmaps/padlock_pix_data.xpm"

// ============================================================================
//
// GUITelIndicator
//
// ============================================================================

GUITelIndicator::GUITelIndicator(unsigned id, bool suppress_servo_fail_error,
				 QWidget* parent, const char* name)
  : QFrame(parent,name),
    m_id(id), 
    m_ind_src_name(), m_ind_azel(), m_cntrl_in_array(), 
    m_cntrl_start_stop_stack(), m_cntrl_start(), m_cntrl_stop(), 
#ifdef USE_SOLICIT_TARGET
    m_cntrl_solicit_target(),
#endif
    m_start_button_index(0),
#ifdef USE_INDIVIDUAL_PANIC_BUTTONS
    m_cntrl_panic(), m_cntrl_panic_x(0),
#endif
    m_com_fail(true), m_is_stopped(false)
{
  QString basename(name);

  QGridLayout* layout = new QGridLayout(this,3,1,5,5,
					basename+" main layout");

  // Target name lineedit -----------------------------------------------------

  m_ind_src_name = new InfoQLineEdit(this,basename+" src name lineedit");
  //m_ind_src_name->setMinimumWidth(status_entries[i].width);
  QToolTip::add(m_ind_src_name,"Target Name");
  layout->addWidget(m_ind_src_name,                0,0);
  
  // Az/El indicator ----------------------------------------------------------

  m_ind_azel =  new GUIAzElIndicator(suppress_servo_fail_error,this,
				     basename+" az/el indicator");
  m_ind_azel->setMinimumSize(QSize(300,300));
  layout->addWidget(m_ind_azel,                    1,0);

  // Buttons ------------------------------------------------------------------

#define LOCK_BUTTON 1

#ifdef USE_INDIVIDUAL_PANIC_BUTTONS
#define PANIC_BUTTON 1
#else
#define PANIC_BUTTON 0
#endif

#ifdef USE_SOLICIT_TARGET
#define SOLICIT_TARGET 1
#else
#define SOLICIT_TARGET 0
#endif

  QFrame* cntrlframe = new QFrame(this,basename+" cntrl frame");
  QGridLayout* cntrllayout = 
    new QGridLayout(cntrlframe,1,2+PANIC_BUTTON+SOLICIT_TARGET+LOCK_BUTTON,0,5,
		    basename+" cntrl layout");

#if 0
  m_cntrl_in_array = 
    new QRadioButton("Include in sub-array",cntrlframe,
		     basename+" cntrl in array radio button");
#endif
  m_cntrl_in_array = 
    new QComboBox(false,cntrlframe,
		  basename+" cntrl in array combo box");
  //m_cntrl_in_array->insertItem("Stand alone operation",0);
  //m_cntrl_in_array->insertItem("Array participant",1);
  m_cntrl_in_array->insertItem("Stand-alone",0);
  m_cntrl_in_array->insertItem("Array",1);
  QToolTip::add(m_cntrl_in_array,TT_ARRAY_ARRAY_STANDALONE);
  unsigned height = 28;

#if LOCK_BUTTON==1
  QRadioButton* lock_in_array
    = new QRadioButton(/* "Lock", */ cntrlframe,
		       basename+" cntrl lock in array radio button");
  QPixmap pxl_padlock(const_cast<const char**>(padlock_pix_data));
  QImage iml_padlock(pxl_padlock.convertToImage());
  QImage ims_padlock(iml_padlock.smoothScale(height,height,QImage::ScaleMin));
  lock_in_array->setPixmap(QPixmap(ims_padlock));
  connect(lock_in_array,SIGNAL(toggled(bool)),
	  m_cntrl_in_array,SLOT(setDisabled(bool)));
  QToolTip::add(lock_in_array,TT_ARRAY_PANE_LOCK);
#endif

#if SOLICIT_TARGET==1
  GUIPixmaps::instance()->setTargetSize(height);
  m_cntrl_solicit_target =
    new MyQPushButton(cntrlframe, basename+" cntrl solicit target");
  m_cntrl_solicit_target->
    setPixmap(*GUIPixmaps::instance()->target_pixmaps(0));
  QToolTip::add(m_cntrl_solicit_target,TT_ARRAY_PANE_SOLICIT_TARGET);
#endif

  m_cntrl_start_stop_stack = 
    new QWidgetStack(cntrlframe,basename+" cntrl start stop stack");
  m_cntrl_start = 
    new MyQPushButton(m_cntrl_start_stop_stack, basename+" cntrl start");
  m_cntrl_stop = 
    new MyQPushButton(m_cntrl_start_stop_stack, basename+" cntrl stop");
  QToolTip::add(m_cntrl_start,TT_OBJSEL_GO);
  QToolTip::add(m_cntrl_stop, TT_OBJSEL_STOP);

  GUIPixmaps::instance()->setSumGoStopSize(height);
  m_cntrl_start->setPixmap(*GUIPixmaps::instance()->sum_go_pixmaps(0));
  m_cntrl_stop->setPixmap(*GUIPixmaps::instance()->sum_stop_pixmaps(0));

  m_cntrl_start_stop_stack->addWidget(m_cntrl_start,0);
  m_cntrl_start_stop_stack->addWidget(m_cntrl_stop,1);
  
#ifdef USE_INDIVIDUAL_PANIC_BUTTONS
  m_cntrl_panic =
    new QPushButton("Panic!",cntrlframe,
		     basename+" cntrl panic push button");
  m_cntrl_panic->setMinimumWidth(50);
  m_cntrl_panic->setMinimumHeight(22);
  m_cntrl_panic->setFocusPolicy(NoFocus);
  cntrllayout->addWidget(m_cntrl_panic,        0,0);
#endif

  cntrllayout->addWidget(m_cntrl_in_array,     0,PANIC_BUTTON);
#if LOCK_BUTTON==1
  cntrllayout->addWidget(lock_in_array,        0,PANIC_BUTTON+1);
#endif
#if SOLICIT_TARGET==1
  cntrllayout->addWidget(m_cntrl_solicit_target, 
			                       0,PANIC_BUTTON+1+LOCK_BUTTON);
#endif
  cntrllayout->addWidget(m_cntrl_start_stop_stack, 
			          0,PANIC_BUTTON+LOCK_BUTTON+1+SOLICIT_TARGET);
  cntrllayout->setColStretch(PANIC_BUTTON,1);

  layout->addWidget(cntrlframe,                2,0);
  
  // Layout -------------------------------------------------------------------

  layout->setRowStretch(1,1);
  layout->setColStretch(0,1);  

  // Make connections ---------------------------------------------------------

  connect(m_cntrl_start,SIGNAL(clicked()),this,SLOT(sendGo()));
  connect(m_cntrl_stop,SIGNAL(clicked()),this,SLOT(sendStop()));
#ifdef USE_INDIVIDUAL_PANIC_BUTTONS
  connect(m_cntrl_panic,SIGNAL(clicked()),this,SLOT(sendEmergency()));
#endif
  connect(m_cntrl_in_array,SIGNAL(activated(int)),
	  this,SLOT(sendInArrayClicked()));
#if SOLICIT_TARGET==1
  connect(m_cntrl_solicit_target,SIGNAL(clicked()),
	  this,SLOT(sendSolicitTarget()));
#endif
}

GUITelIndicator::~GUITelIndicator()
{
  // nothing to see here
}

void GUITelIndicator::update(bool controls_enabled, const GUIUpdateData& ud)
{
  // Az/El indicator ----------------------------------------------------------

  m_ind_azel->update(ud);

  // Target object ------------------------------------------------------------
  
  if(ud.tar_object)
    {
      if((ud.full_update)||(ud.replay))
	{
	  QString text(ud.tar_object->targetName(ud.mjd));
	  QFontMetrics fm = m_ind_src_name->fontMetrics();
	  unsigned tw = fm.boundingRect(text).width();
	  unsigned bw = m_ind_src_name->width() - 10;
	  if(tw>bw)
	    {
	      const unsigned tintro = 60;
	      const unsigned tscroll = 10;
	      unsigned maxchars = (bw*text.length())/tw;
	      unsigned surchars = text.length()-maxchars;
	      unsigned nticks = 2*tintro + surchars*tscroll;
	      if((ud.update_number%nticks)<tintro)
		text = text.mid(0,maxchars);
	      else if((ud.update_number%nticks)<nticks-tintro)
		text = text.mid(((ud.update_number%nticks)-tintro)/tscroll,
				   maxchars);
	      else 
		text = text.mid(surchars,maxchars);
	    }
	  m_ind_src_name->setText(text);
	  m_ind_src_name->setEnabled(true);
	}
    }
  else
    {
      m_ind_src_name->setText("");
      m_ind_src_name->setEnabled(false);
    }

  // Buttons ------------------------------------------------------------------

  m_com_fail = ud.tse.state==TelescopeController::TS_COM_FAILURE;
  m_is_stopped = ud.tse.req==TelescopeController::REQ_STOP;

  bool canstartstop = controls_enabled 
    && (ud.tse.state!=TelescopeController::TS_COM_FAILURE)
    && (!isInArray());

  if(canstartstop)
    {
      m_cntrl_start_stop_stack->setEnabled(true);
      if(m_is_stopped)
	{
#ifdef USE_SOLICIT_TARGET
	  m_cntrl_solicit_target->setEnabled(true);
#endif

	  if(m_cntrl_start_stop_stack->
	     id(m_cntrl_start_stop_stack->visibleWidget())==1)
	    {
	      bool change_focus = 
		m_cntrl_start_stop_stack->visibleWidget()->hasFocus();
	      m_cntrl_start_stop_stack->raiseWidget(0);
	      if(change_focus)
		m_cntrl_start_stop_stack->visibleWidget()->setFocus();
	    }
	  
	  if(m_cntrl_start_stop_stack->isEnabled())
	    {
	      m_cntrl_start->setPixmap(*GUIPixmaps::instance()->
				       sum_go_pixmaps(m_start_button_index++));
	      if(m_start_button_index==24)m_start_button_index=0;
	    }
	}
      else 
	{
#ifdef USE_SOLICIT_TARGET
	  m_cntrl_solicit_target->setEnabled(false);
#endif
	  if(m_cntrl_start_stop_stack->
	     id(m_cntrl_start_stop_stack->visibleWidget())==0)
	    {
	      bool change_focus = 
		m_cntrl_start_stop_stack->visibleWidget()->hasFocus();
	      m_cntrl_start_stop_stack->raiseWidget(1);
	      if(change_focus)
		m_cntrl_start_stop_stack->visibleWidget()->setFocus();
	    }
	}
    }
  else
    {
#ifdef USE_SOLICIT_TARGET
      m_cntrl_solicit_target->setEnabled(false);
#endif
      m_cntrl_start_stop_stack->setEnabled(false);
    }

  //m_cntrl_in_array->setEnabled(controls_enabled);

#ifdef USE_INDIVIDUAL_PANIC_BUTTONS
  int lastx=m_cntrl_panic_x;
  int x=int(floor(30*(1+sin(fmod(ud.mjd*86400/2,1)*Angle::sc_twoPi))));
  if(x!=lastx)
    {
      QColor pfcolor=QColor(255-x,x,x);
      QColor pbcolor=QColor(255-60+x,60-x,60-x);
      m_cntrl_panic->setPalette(QPalette(pfcolor,pbcolor));
      m_cntrl_panic_x=x;
    }  
#endif
}

void GUITelIndicator::setScopeValuesToDisplay(ScopeValuesToDisplay sv)
{
  m_ind_azel->setScopeValuesToDisplay(sv);
}

void GUITelIndicator::setInArray(bool in_array)
{
  m_cntrl_in_array->setCurrentItem(in_array?1:0);
}

void GUITelIndicator::sendGo()
{
  emit go(m_id);
}

void GUITelIndicator::sendStop()
{
  emit stop(m_id);
}

void GUITelIndicator::sendSolicitTarget()
{
  emit solicitTarget(m_id);
}

void GUITelIndicator::sendEmergency()
{
  emit emergency(m_id);
}

void GUITelIndicator::sendInArrayClicked()
{
  if(isInArray())m_cntrl_start_stop_stack->setEnabled(false);
  emit inArrayClicked(m_id);
}

// ============================================================================
//
// GUIArrayPane
//
// ============================================================================

GUIArrayPane::GUIArrayPane(const std::vector<ScopeConfig>& scope, 
			   const SEphem::SphericalCoords& mean_earth_pos, 
			   bool grb, unsigned theme,
			   QWidget* parent, const char* name)
  : QFrame(parent,name), GUITabPane(this),
    m_mean_earth_pos(mean_earth_pos), m_sv_demand(SV_AUTO), m_sv(SV_AUTO),
    m_set_direction_as_array(true), 
    m_direction_preference(CorrectionParameters::DP_NONE),
    m_direction_choice(CorrectionParameters::DP_NONE),
    m_ud_vec(), m_com_failure_vec(), m_interlock_vec(), m_az_deg_vec(), 
    m_ut_date(), m_ut_utc(), m_ut_lmst(), m_ut_mjd(),
    m_src_name(), m_src_ra(), m_src_dec(), m_src_az(), m_src_el(),
    m_tel(scope.size(),0), m_lastx(), m_array_object(), m_selector()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  QString basename(name);

  QGridLayout* mainlayout = new QGridLayout(this,4,1,5,5,
					    basename+" main layout");

  // --------------------------------------------------------------------------
  // LINE EDITS
  // --------------------------------------------------------------------------

  MyQGroupBox* utcbox = new MyQGroupBox(1,Qt::Vertical,"Time and Date",
					this,basename+" ut box");

  MyQGroupBox* srcbox = new MyQGroupBox(1,Qt::Vertical,"Target",
					this,basename+" src box");

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
    { srcbox,  &m_src_name,  "Name",         TT_SUMDET_TARGET,           400 },
    { srcbox,  &m_src_ra,    "RA",           TT_SUMDET_SRC_RA,           100 },
    { srcbox,  &m_src_dec,   "Dec",          TT_SUMDET_SRC_DEC,          100 },
    { srcbox,  &m_src_az,    "Az",           TT_SUMDET_SRC_AZ_DIR,       100 },
    { srcbox,  &m_src_el,    "El",           TT_SUMDET_SRC_EL,           100 }
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
  
  // --------------------------------------------------------------------------
  // TELESCOPE INDICATORS
  // --------------------------------------------------------------------------

  const char* tel_name[][4] = 
    { { "Hillas", "Jelley", "Porter", "Chudakov" },
      { "Hannibal", "B.A.", "Murdock", "Face" },
      { "Frodo", "Samwise", "Pippin", "Merry" },
      { "Gandalf", "Aragorn", "Legolas", "Gimli" },
      { "John", "George", "Paul", "Ringo" },
      { "Aramis", "Porthos", "Athos", "d'Artagnan" },
      { "John", "John", "John", "John" } };


  if(theme >= sizeof(tel_name)/sizeof(*tel_name))theme=0;
  if(theme==1)
    {
      sendATeamStart();
      scheduleATeamQuote();
    }

  unsigned nscope = scope.size();

  unsigned cols = 4;
  if(theme==4)cols=2;

  //unsigned nx = nscope<=2?nscope:(nscope+1)/2;
  //unsigned ny = nscope<=2?1:2;
  unsigned nx = nscope<=cols?nscope:(nscope+1)/2;
  unsigned ny = nscope<=cols?1:2;

  QFrame* telframe = new QFrame(this,basename+" tel frame");
  QGridLayout* tellayout = 
    new QGridLayout(telframe,ny,nx,0,5,basename+" tel layout");

  for(unsigned iy=0;iy<ny;iy++)
    for(unsigned ix=0;ix<nx;ix++)
      {
	unsigned iscope=iy*nx+ix;
	if(iscope>=nscope)break;

	std::string str_iscope = VSDataConverter::toString(iscope+1);
	std::string this_tel_name = std::string("Telescope ")+str_iscope;
 	if(iscope < sizeof(*tel_name)/sizeof(**tel_name))
	  this_tel_name += 
	    std::string(" (")+
	    std::string(tel_name[theme][iscope])+std::string(")");

	MyQGroupBox* telbox = 
	  new MyQGroupBox(1,Qt::Vertical,this_tel_name,telframe,
			  basename+std::string(" tel box ")+str_iscope);
	
	m_tel[iscope] = 
	  new GUITelIndicator(iscope, scope[iscope].suppress_servo_fail_error,
			      telbox,basename+std::string(" tel ")+str_iscope);

	m_tel[iscope]->
	  setPalette(QPalette(scope[iscope].color,scope[iscope].color));

	tellayout->addWidget(telbox,iy,ix);
      }

  // --------------------------------------------------------------------------
  // CONTROL BUTTONS
  // --------------------------------------------------------------------------

  StowObjectVector stow_pos;
  stow_pos.push_back(StowObject("Stow",
				SphericalCoords::makeLatLongDeg(0,0)));

  int height;
  m_selector = new GUIObjectSelector(0, true, true, font(), height,
				     stow_pos, 0, grb,
				     this, basename+" selector");

  // --------------------------------------------------------------------------
  // MAKE THE LAYOUT
  // --------------------------------------------------------------------------
  
  mainlayout->addWidget(utcbox,0,0);
  mainlayout->addWidget(srcbox,1,0);
  mainlayout->addWidget(telframe,2,0);
  mainlayout->addWidget(m_selector,3,0);
  mainlayout->setRowStretch(2,1);  

  // --------------------------------------------------------------------------
  // Make connections
  // --------------------------------------------------------------------------

  connect(m_selector,SIGNAL(go(unsigned)),this,SLOT(goArray(unsigned)));
  connect(m_selector,SIGNAL(stop(unsigned)),this,SLOT(stopArray(unsigned)));
  connect(m_selector,SIGNAL(loadNewTargetList(unsigned)),
	  this,SLOT(passOnLoadNewTargetList(unsigned)));
  connect(m_selector,SIGNAL(setTarget(unsigned)),
	  this,SLOT(sendSelectedTargetToArray(unsigned)));

  for(unsigned iscope=0; iscope<nscope; iscope++)
    {
      connect(m_tel[iscope],SIGNAL(go(unsigned)),
	      this,SLOT(collectGoOne(unsigned)));
      connect(m_tel[iscope],SIGNAL(stop(unsigned)),
	      this,SLOT(collectStopOne(unsigned)));
      connect(m_tel[iscope],SIGNAL(solicitTarget(unsigned)),
	      this,SLOT(sendSelectedTargetToOne(unsigned)));
      connect(m_tel[iscope],SIGNAL(emergency(unsigned)),
	      this,SLOT(collectEmergencyOne(unsigned)));
      connect(m_tel[iscope],SIGNAL(inArrayClicked(unsigned)),
	      this,SLOT(collectInArrayClickedOne(unsigned)));
    }
}
    
GUIArrayPane::~GUIArrayPane()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  delete m_array_object;
}

void GUIArrayPane::
getMeanUpdateData(TelescopeController::TrackingState& state,
		  TelescopeController::TrackingRequest& req,
		  const std::vector<GUIUpdateData>& ud_vec) const
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  state = TelescopeController::TS_STOP;
  req = TelescopeController::REQ_STOP;
  unsigned ntel = m_tel.size();
  for(unsigned itel=0; itel<ntel; itel++)
    {
      if(m_tel[itel]->isInArray())
	{
	  if(ud_vec[itel].tse.state == TelescopeController::TS_COM_FAILURE)
	    state = TelescopeController::TS_COM_FAILURE;
	  if((state != TelescopeController::TS_COM_FAILURE)&&
	     (ud_vec[itel].tse.req!=TelescopeController::REQ_STOP))
	    state = TelescopeController::TS_STOP, 
	      req = TelescopeController::REQ_TRACK;
	}
    }
}

// ----------------------------------------------------------------------------
//
// Array control methods
//
// ----------------------------------------------------------------------------

void GUIArrayPane::goArray(unsigned selector_id)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  unsigned ntel = m_tel.size();
  for(unsigned itel=0; itel<ntel; itel++)
    if(m_tel[itel]->canSendArrayCommands())
      emit goOne(itel);
}

void GUIArrayPane::stopArray(unsigned selector_id)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  unsigned ntel = m_tel.size();
  for(unsigned itel=0; itel<ntel; itel++)
    if(m_tel[itel]->canSendArrayCommands())
      emit stopOne(itel);
}

void GUIArrayPane::setDirectionChoice()
{
  m_direction_choice = CorrectionParameters::DP_NONE;
  if(!m_array_object)return;

  if(!m_set_direction_as_array)
    {
      m_direction_choice = m_direction_preference;
      return;
    }

#if 1
  // NEW ALGORITHM - SHOULD SOLVE PROBLEM WITH FAST MOVING SOURCES

  bool use_corrections = m_array_object->useCorrections();

  bool viable_cw = true;
  bool viable_ccw = true;

  double slew_cw_dist = 0;
  double slew_ccw_dist = 0;

  unsigned ntel = m_tel.size();
  std::vector<double> tel_cw(ntel);
  std::vector<double> tel_ccw(ntel);

  double min_az_cw = 2.0*M_PI;
  double max_az_cw = -2.0*M_PI;
  double min_az_ccw = 2.0*M_PI;
  double max_az_ccw = -2.0*M_PI;

  // First calculate distance of initial slew in CW and CCW cases
  for(unsigned itel=0; itel<ntel; itel++)
    if((m_tel[itel]->isInArray())
       &&(m_ud_vec[itel].tse.state
	  !=VTracking::TelescopeController::TS_COM_FAILURE))
      {	
	const double tel_az = 
	  Angle::frDeg(m_ud_vec[itel].tse.status.az.driveangle_deg);

	SphericalCoords tar_azel  = 
	  m_array_object->getAzEl(m_ud_vec[itel].mjd,m_ud_vec[itel].lmst,
				  m_mean_earth_pos);

	double tar_az_driveangle_cw   = tar_azel.longitudeRad();
	double tar_el_driveangle_cw   = tar_azel.latitudeRad();
	viable_cw &= m_ud_vec[itel].tcp.
	  doAzElCorrections(tar_az_driveangle_cw, tar_el_driveangle_cw,
			    tel_az, use_corrections, 
			    CorrectionParameters::DP_CW);
	double dist_cw = fabs(tar_az_driveangle_cw - tel_az);

	double tar_az_driveangle_ccw  = tar_azel.longitudeRad();
	double tar_el_driveangle_ccw  = tar_azel.latitudeRad();
	viable_ccw &= m_ud_vec[itel].tcp.
	  doAzElCorrections(tar_az_driveangle_ccw, tar_el_driveangle_ccw,
			    tel_az, use_corrections, 
			    CorrectionParameters::DP_CCW);
	double dist_ccw = fabs(tar_az_driveangle_ccw - tel_az);

	slew_cw_dist = std::max(slew_cw_dist, dist_cw);
	slew_ccw_dist = std::max(slew_ccw_dist, dist_ccw);

	max_az_cw = std::max(tar_az_driveangle_cw,max_az_cw);
	min_az_cw = std::min(tar_az_driveangle_cw,min_az_cw);
	max_az_ccw = std::max(tar_az_driveangle_ccw,max_az_ccw);
	min_az_ccw = std::min(tar_az_driveangle_ccw,min_az_ccw);
	
	tel_cw[itel] = tar_az_driveangle_cw;
	tel_ccw[itel] = tar_az_driveangle_ccw;
      }

  double first_spread_cw_dist = fabs(max_az_cw-min_az_cw);
  double first_spread_ccw_dist = fabs(max_az_ccw-min_az_ccw);

  double track_cw_dist = 0;
  double track_ccw_dist = 0;
  double spread_cw_dist = 0;
  double spread_ccw_dist = 0;

  for(unsigned idtsec = 0; idtsec<25*60; idtsec+=30)
    {
      min_az_cw = 2.0*M_PI;
      max_az_cw = -2.0*M_PI;
      min_az_ccw = 2.0*M_PI;
      max_az_ccw = -2.0*M_PI;

      for(unsigned itel=0; itel<ntel; itel++)
	if((m_tel[itel]->isInArray())
	   &&(m_ud_vec[itel].tse.state
	      !=VTracking::TelescopeController::TS_COM_FAILURE))
	  {	
	    double mjd = m_ud_vec[itel].mjd + double(idtsec)/86400.0;
	    double lmst = Astro::mjdToLMST(mjd, m_mean_earth_pos.phiRad());
	
	    SphericalCoords tar_azel  = 
	      m_array_object->getAzEl(mjd, lmst, m_mean_earth_pos);

	    double tar_az_driveangle_cw   = tar_azel.longitudeRad();
	    double tar_el_driveangle_cw   = tar_azel.latitudeRad();
	    viable_cw &= m_ud_vec[itel].tcp.
	      doAzElCorrections(tar_az_driveangle_cw, tar_el_driveangle_cw,
				tel_cw[itel], use_corrections, 
				CorrectionParameters::DP_CW);
	    double dist_cw = fabs(tar_az_driveangle_cw - tel_cw[itel]);

	    double tar_az_driveangle_ccw  = tar_azel.longitudeRad();
	    double tar_el_driveangle_ccw  = tar_azel.latitudeRad();
	    viable_ccw &= m_ud_vec[itel].tcp.
	      doAzElCorrections(tar_az_driveangle_ccw, tar_el_driveangle_ccw,
				tel_ccw[itel], use_corrections, 
				CorrectionParameters::DP_CCW);
	    double dist_ccw = fabs(tar_az_driveangle_ccw - tel_ccw[itel]);

	    track_cw_dist = std::max(track_cw_dist, dist_cw);
	    track_ccw_dist = std::max(track_ccw_dist, dist_ccw);

	    max_az_cw = std::max(tar_az_driveangle_cw,max_az_cw);
	    min_az_cw = std::min(tar_az_driveangle_cw,min_az_cw);
	    max_az_ccw = std::max(tar_az_driveangle_ccw,max_az_ccw);
	    min_az_ccw = std::min(tar_az_driveangle_ccw,min_az_ccw);

	    tel_cw[itel] = tar_az_driveangle_cw;
	    tel_ccw[itel] = tar_az_driveangle_ccw;
	  }
      spread_cw_dist = std::max(spread_cw_dist,fabs(max_az_cw-min_az_cw));
      spread_ccw_dist = std::max(spread_ccw_dist,fabs(max_az_ccw-min_az_ccw));
    }

  if((!viable_cw)&&(!viable_ccw))return;

  if(!viable_cw)
    m_direction_choice=CorrectionParameters::DP_CCW;
  else if(!viable_cw)
    m_direction_choice=CorrectionParameters::DP_CW;
  else if(first_spread_cw_dist>spread_ccw_dist+Angle::frDeg(45))
    m_direction_choice=CorrectionParameters::DP_CCW;
  else if(first_spread_ccw_dist>spread_cw_dist+Angle::frDeg(45))
    m_direction_choice=CorrectionParameters::DP_CW;
  else if(m_direction_preference==CorrectionParameters::DP_CCW)
    m_direction_choice=CorrectionParameters::DP_CCW;
  else if(m_direction_preference==CorrectionParameters::DP_CW)
    m_direction_choice=CorrectionParameters::DP_CW;
  else if(track_cw_dist>track_ccw_dist+Angle::frDeg(45))
    m_direction_choice=CorrectionParameters::DP_CCW;
  else if(track_ccw_dist>track_cw_dist+Angle::frDeg(45))
    m_direction_choice=CorrectionParameters::DP_CW;
  else if(spread_cw_dist>spread_ccw_dist+Angle::frDeg(45))
    m_direction_choice=CorrectionParameters::DP_CCW;
  else if(spread_ccw_dist>spread_cw_dist+Angle::frDeg(45))
    m_direction_choice=CorrectionParameters::DP_CW;
  else if(slew_cw_dist>slew_ccw_dist)
    m_direction_choice=CorrectionParameters::DP_CCW;
  else
    m_direction_choice=CorrectionParameters::DP_CW;
  
#if 0
  std::cout << viable_cw << ' ' << viable_ccw << ' '
	    << first_spread_cw_dist << ' ' << first_spread_ccw_dist << ' ' 
	    << slew_cw_dist << ' ' << slew_ccw_dist << ' ' 
	    << track_cw_dist << ' ' << track_ccw_dist << ' ' 
	    << spread_cw_dist << ' ' << spread_ccw_dist << ' ' 
	    << m_set_direction_as_array << ' '
	    << m_direction_preference << ' ' << m_direction_choice << '\n';
#endif
  
#else
  // OLD ALGORITHM WHICH HAS PROBLEMS WITH FAST MOVING SOURCES

  // Determine direction to slew
  bool has_one = false;
  bool slew_cw_acceptable = true;
  bool slew_ccw_acceptable = true;
  double slew_cw_dist = 0;
  double slew_ccw_dist = 0;

  SphericalCoords tar_azel  = 
    m_array_object->getAzEl(m_ud_vec[0].mjd,m_ud_vec[0].lmst,m_mean_earth_pos);
  bool use_corrections = m_array_object->useCorrections();

  unsigned ntel = m_tel.size();
  for(unsigned itel=0; itel<ntel; itel++)
    if(m_tel[itel]->isInArray())
      {
	const double tel_az = 
	  Angle::frDeg(m_ud_vec[itel].tse.status.az.driveangle_deg);

	double tar_az_driveangle_cw   = tar_azel.longitudeRad();
	double tar_el_driveangle_cw   = tar_azel.latitudeRad();
	bool limits_cw = m_ud_vec[itel].tcp.
	  doAzElCorrections(tar_az_driveangle_cw, tar_el_driveangle_cw,
			    tel_az, use_corrections, 
			    CorrectionParameters::DP_CW);

	double tar_az_driveangle_ccw  = tar_azel.longitudeRad();
	double tar_el_driveangle_ccw  = tar_azel.latitudeRad();
	bool limits_ccw = m_ud_vec[itel].tcp.
	  doAzElCorrections(tar_az_driveangle_ccw, tar_el_driveangle_ccw,
			    tel_az, use_corrections, 
			    CorrectionParameters::DP_CCW);

	if((limits_cw)&&(tar_az_driveangle_cw > 0))
	  {
	    double dist = fabs(tar_az_driveangle_cw - tel_az);
	    if((!has_one)||(dist > slew_cw_dist))slew_cw_dist = dist;
	  }
	else
	  {
	    slew_cw_acceptable = false;
	  }

	if((limits_ccw)&&(tar_az_driveangle_ccw < 0))
	  {
	    double dist = fabs(tar_az_driveangle_ccw - tel_az);
	    if((!has_one)||(dist > slew_ccw_dist))slew_ccw_dist = dist;
	  }
	else
	  {
	    slew_ccw_acceptable = false;
	  }

	has_one = true;
      }


  if((!has_one)||((!slew_cw_acceptable)&&(!slew_ccw_acceptable)))return;

  if((slew_cw_acceptable)
     &&((!slew_ccw_acceptable)
	||(m_direction_preference==CorrectionParameters::DP_CW)
	||((m_direction_preference==CorrectionParameters::DP_NONE)
	   &&(slew_cw_dist<slew_ccw_dist))))
    m_direction_choice=CorrectionParameters::DP_CW;
  else
    m_direction_choice=CorrectionParameters::DP_CCW;

#if 0
  std::cout << has_one << ' ' 
	    << slew_cw_acceptable << ' ' << slew_ccw_acceptable << ' '
	    << slew_cw_dist << ' ' << slew_ccw_dist << ' ' 
	    << m_set_direction_as_array << ' '
	    << m_direction_preference << ' ' << m_direction_choice << '\n';
#endif
#endif // OLD ALGORITHM
}

void GUIArrayPane::sendSelectedTargetToArray(unsigned selector_id)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  unsigned ntel = m_tel.size();

  // Compute mean azimuth for use by "getObject"
  double az_deg = 0;
  unsigned count = 0;
  for(unsigned itel=0; itel<ntel; itel++)
    if(m_tel[itel]->isInArray())az_deg += m_az_deg_vec[itel], count++;
  if(count == 0)az_deg=0;
  else az_deg /= double(count);

  // Get object from the selector
  delete m_array_object;
  m_array_object = m_selector->getObject(az_deg, m_ud_vec[0].mjd);
  setDirectionChoice();

  for(unsigned itel=0; itel<ntel; itel++)
    if(m_tel[itel]->canReTarget())
      emit setTargetOne(itel, m_array_object,m_direction_choice);
}

void GUIArrayPane::setFromTargetTable(int target)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  m_selector->selectTarget(target);
  sendSelectedTargetToArray(0);
}

void GUIArrayPane::setFromGRBTable(int target)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  m_selector->selectGRB(target);
  sendSelectedTargetToArray(0);
}
 
void GUIArrayPane::setScopeValuesToDisplay(ScopeValuesToDisplay sv)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  unsigned ntel = m_tel.size();
  for(unsigned itel=0; itel<ntel; itel++)
    m_tel[itel]->setScopeValuesToDisplay(sv);
  m_sv_demand = sv;
}

void GUIArrayPane::
changeDirPref(bool set_direction_as_array,
	      SEphem::CorrectionParameters::DirectionPreference dp)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  m_set_direction_as_array = set_direction_as_array;
  m_direction_preference   = dp;
  setDirectionChoice();
  unsigned ntel = m_tel.size();
  for(unsigned itel=0; itel<ntel; itel++)
    if(m_tel[itel]->canReTarget())
      emit setTargetOne(itel, m_array_object,m_direction_choice);
}

void GUIArrayPane::configureFullArray()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  unsigned ntel = m_tel.size();
  for(unsigned itel=0; itel<ntel; itel++)
    m_tel[itel]->setInArray((!m_com_failure_vec[itel])
			    &&(!m_interlock_vec[itel]));
}

// ----------------------------------------------------------------------------
//
// Telescope control methods
//
// ----------------------------------------------------------------------------

void GUIArrayPane::sendSelectedTargetToOne(unsigned id)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  emit setTargetOne(id, m_array_object);
}

// ----------------------------------------------------------------------------
//
// Target methods
//
// ----------------------------------------------------------------------------

void GUIArrayPane::
syncWithTargetList(const VTracking::TargetList& target_list)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  m_selector->syncWithTargetList(target_list);
}

void GUIArrayPane::
syncWithGRBList(const GUIGRBMonitor::GRBTriggerList& grb_list)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  m_selector->syncWithGRBList(grb_list);
}

void GUIArrayPane::updateArray(const std::vector<GUIUpdateData>& ud_vec)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  bool replay = false;
  bool full_update = false;

  unsigned nud = ud_vec.size();
  m_com_failure_vec.resize(nud);
  m_interlock_vec.resize(nud);
  m_az_deg_vec.resize(nud);
  for(unsigned iud=0;iud<nud;iud++)
    {
      const GUIUpdateData& ud(ud_vec[iud]);
      m_com_failure_vec[iud] =
	(ud.tse.state==TelescopeController::TS_COM_FAILURE);
      m_interlock_vec[iud] =
	(ud.tse.status.interlock
	 || ud.tse.status.interlockAzPullCord
	 || ud.tse.status.interlockAzStowPin
	 || ud.tse.status.interlockElStowPin
	 || ud.tse.status.interlockAzDoorOpen
	 || ud.tse.status.interlockElDoorOpen
	 || ud.tse.status.interlockSafeSwitch
	 || !ud.tse.status.remoteControl);
      m_az_deg_vec[iud] = ud.tse.status.az.driveangle_deg;
      if(ud.full_update)full_update=true;
      if(ud.replay)replay=true;
    }

  m_ud_vec = ud_vec;

  if(!isVisible())return;

  unsigned ntel = m_tel.size();
  assert(ud_vec.size() == ntel);
  m_ut_date->setText(ud_vec[0].date_string);
  m_ut_utc->setText(ud_vec[0].timeangle.hmsString(1));
  m_ut_lmst->setText(ud_vec[0].lmst.hmsString(1));
  m_ut_mjd->setText(QString::number(ud_vec[0].mjd,'f',6));

  if(m_array_object)
    {
#if 0
      if(m_sv_demand == SV_AUTO)
	{
	  if(!m_array_object->useCorrections())m_sv = SV_ENCODER;
	  else m_sv = SV_SKY;
	}
      else
	{
	  m_sv = m_sv_demand;
	}
#endif

      if(!m_array_object->useCorrections())m_sv = SV_ENCODER;
      else m_sv = SV_SKY;

      SphericalCoords tar_radec = 
	m_array_object->getRaDec(ud_vec[0].mjd,ud_vec[0].lmst,m_mean_earth_pos);
      SphericalCoords tar_azel  = 
	m_array_object->getAzEl(ud_vec[0].mjd,ud_vec[0].lmst,m_mean_earth_pos);

      QString az_txt;
      QString el_txt;

      if(m_sv == SV_ENCODER)
	{
	  az_txt = "Enc: ";
	  el_txt = "Enc: ";
	}
	  
      az_txt += MAKEDEG(tar_azel.phi().degString(4));
      el_txt += MAKEDEG(tar_azel.latitude().degPM180String(4));

      if((m_ud_vec[0].update_number/14)%2==1)
	{
	  if(m_set_direction_as_array)
	    switch(m_direction_preference)
	      {
	      case CorrectionParameters::DP_NONE: break;
	      case CorrectionParameters::DP_CW: az_txt += " (CW)"; break;
	      case CorrectionParameters::DP_CCW: az_txt += " (CCW)"; break;
	      }
	  else az_txt += " (FREE)";
	}

      m_src_az->setText(az_txt);
      m_src_az->setEnabled(true);
      m_src_el->setText(el_txt);
      m_src_el->setEnabled(true);

      bool target_moves = m_array_object->objectMovesInAzEl();

      bool in_limits = true;
      bool in_limits30 = true;
      bool too_fast = false;
      bool too_fast30 = false;
      bool cw_ccw_rotation = false;
      for(unsigned iud=0;iud<nud;iud++)if(m_tel[iud]->isInArray())
	{
	  if(!ud_vec[iud].in_limits)in_limits = false;
	  if(!ud_vec[iud].in_limits30)in_limits30 = false;
	  if(fabs(ud_vec[iud].tar_az_speed)>ud_vec[iud].az_speed_limit)
	    too_fast = true;
	  if(fabs(ud_vec[iud].tar_max_az_speed)>ud_vec[iud].az_speed_limit)
	    too_fast30=true;
	  if((Angle::toDeg(ud_vec[iud].tar_az_driveangle)>90)&&
	     (Angle::toDeg(fabs(ud_vec[iud].tar_az_driveangle-
				ud_vec[iud].tar_az_driveangle30))>270))
	    cw_ccw_rotation = true;
	}

      if(!in_limits)
	{
	  m_src_az->setPaletteBackgroundColor(color_bg_warn);
	  m_src_az->setPaletteForegroundColor(color_fg_warn);
	  m_src_el->setPaletteBackgroundColor(color_bg_warn);
	  m_src_el->setPaletteForegroundColor(color_fg_warn);
	}
      else if(target_moves && too_fast)
	{
	  m_src_az->unsetPalette();
	  if(ud_vec[0].tv.tv_usec/500000==0)
	    {
	      m_src_az->setPaletteBackgroundColor(color_bg_warn);
	      m_src_az->setPaletteForegroundColor(color_fg_warn);
	    }
	  m_src_el->unsetPalette();
	}
      else if(!in_limits30)
	{
	  m_src_az->unsetPalette();
	  m_src_el->unsetPalette();
	  if(ud_vec[0].tv.tv_usec/500000==0)
	    {
	      m_src_az->setPaletteBackgroundColor(color_bg_attn);
	      m_src_az->setPaletteForegroundColor(color_fg_attn);
	      m_src_el->setPaletteBackgroundColor(color_bg_attn);
	      m_src_el->setPaletteForegroundColor(color_fg_attn);
	    }
	}
      else if((target_moves && too_fast30)||(cw_ccw_rotation))
	{
	  m_src_az->unsetPalette();
	  if(ud_vec[0].tv.tv_usec/500000==0)
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

      if((full_update)||(replay))
	{
	  QString text(m_array_object->targetName(ud_vec[0].mjd));
	  QFontMetrics fm = m_src_name->fontMetrics();
	  unsigned tw = fm.boundingRect(text).width();
	  unsigned bw = m_src_name->width() - 10;
	  if(tw>bw)
	    {
	      const unsigned tintro = 60;
	      const unsigned tscroll = 10;
	      unsigned maxchars = (bw*text.length())/tw;
	      unsigned surchars = text.length()-maxchars;
	      unsigned nticks = 2*tintro + surchars*tscroll;
	      if((ud_vec[0].update_number%nticks)<tintro)
		text = text.mid(0,maxchars);
	      else if((ud_vec[0].update_number%nticks)<nticks-tintro)
		text = text.mid(((ud_vec[0].update_number%nticks)-tintro)
				/tscroll, maxchars);
	      else 
		text = text.mid(surchars,maxchars);
	    }
	  m_src_name->setText(text);
	  m_src_name->setEnabled(true);
	}

      if(m_sv == SV_ENCODER)
	{
	  m_src_ra->setText("Enc: N/A");
	  m_src_dec->setText("Enc: N/A");
	}
      else
	{
	  m_src_ra->setText(tar_radec.phi().hmsString(1));
	  m_src_dec->setText(tar_radec.latitude().dmsString(0));
	}
      m_src_ra->setEnabled(true);
      m_src_dec->setEnabled(true);
    }
  else if((full_update)||(replay))
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
    }

  for(unsigned itel=0; itel<ntel; itel++)
    m_tel[itel]->update(controlsEnabled(),ud_vec[itel]);

  TelescopeController::TrackingState state;
  TelescopeController::TrackingRequest req;
  getMeanUpdateData(state,req,ud_vec);

  m_selector->updateButtons(controlsEnabled(), state, req);
  m_selector->animateButtons();
}

void GUIArrayPane::collectGoOne(unsigned id)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  emit goOne(id);
}

void GUIArrayPane::collectStopOne(unsigned id)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  emit stopOne(id);
}

void GUIArrayPane::collectEmergencyOne(unsigned id)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  emit emergencyOne(id);
}

void GUIArrayPane::collectInArrayClickedOne(unsigned id)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(m_tel[id]->canReTarget())
    {
      if(m_direction_choice==CorrectionParameters::DP_NONE)
	setDirectionChoice();
      emit setTargetOne(id, m_array_object, m_direction_choice);
    }
  emit inArrayClickedOne(id);
}

void GUIArrayPane::passOnLoadNewTargetList(unsigned id)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  emit loadNewTargetList(id);
}

// ============================================================================
//
// A-Team methods
//
// ============================================================================

void GUIArrayPane::sendATeamStart()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  Message message(Message::DR_LOCAL,Message::PS_UNUSUAL,
		  "The A-Team");
  message.messageStream() 
    << "In 1972 a crack commando unit was sent to prison by a\n"
    << "military court for a crime they didn't commit. These men\n"
    "promptly escaped from a maximum security stockade to the\n"
    "Los Angeles underground. Today, still wanted by the government,\n"
    "they survive as soldiers of fortune. If you have a problem, if\n"
    "no one else can help, and if you can find them, maybe you can\n"
    "hire the A-Team.";
  Messenger::relay()->sendMessage(message);

#if 0
  if((QSound::isAvailable())&&(access("sounds/a-team.mp3",R_OK)==0))
    QSound::play("sounds/a-team.wav");
  else if(access("sounds/a-team.mp3",R_OK)==0)
    system("xmms sounds/a-team.mp3 &");
#else
  if(access("sounds/a-team.mp3",R_OK)==0)
    system("xmms sounds/a-team.mp3 &");
#endif
}

void GUIArrayPane::sendRandomATeamQuote()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  struct Quote
  {
    const char* who;
    const char* what;
  };

  Quote quotes[] = 
    { { "Hannibal", "I love it when a plan comes together!" },
      { "B.A." ,    "I thought you weren't crazy no more?" },
      { "Hannibal", "I love it when a corpse comes apart!" },
      { "Murdock",  "Looks like we're going to crash." },
      { "Face",     "Uh, Murdock, what's going to happen?" },
      { "Murdock",  "There isn't room for four of us?" },
      { "B.A.",     "What? What do you mean four, sucker?" },
      { "B.A.",     "I told you guys for the last time. I don't want\n"
	            "this sucker's blood in me. It's going to make me\n"
	            "crazy like him." }, 
      { "B.A.",     "We're flying this time, aren't we?" },
      { "Face",     "The magic word is... tuberculosis." },
      { "B.A.",     "I pity the fool who goes out tryin' a' take over da\n"
	             "world, then runs home cryin' to his momma!" },
      { "B.A.",      "Shut up, fool." },
      { "Murdock",   "I don't want to be a secret weapon - I want\n"
	             "to be an exposed weapon!" },
      { "B.A.",      "This is bad, Hannibal - real bad! Some guys are \n"
	             "coming around and busting heads saying they're the\n"
       	             "A-Team! There's only *one* A-Team! Us!" },
      { "Hannibal",  "I like mathematical progressions, but we're really\n"
	             "picky about whom we work for." },
      { "Murdock",   "I wish I could just jump in the water and live like a fish." },
      { "B.A.",      "Shut up fool, you ain't no fish!" },
      { "B.A.",      "I ain't flying Hannibal!" },
      { "B.A.",      "I'm gonna kill that crazy Murdock!" },
      { "B.A.",      "You put cake in my van?" },
      { "Hannibal",  "Murdock, how'd I ever let you talk me into this?" },
      { "Murdock",   "I don't know; I have intermittent memory loss." },
      { "Amy",       "Hannibal's plans never work right. They just work." },
      { "Murdock",   "Let's not get technical." },
      { "Murdock",   "I leave with you my Captain Bellybuster cap for\n"
	              "security. Now, you take good care of it. I promise\n"
	              "you, I will return for my Captain Bellybuster cap." },
      { "B.A.",       "I don't start no trouble. I mind my own business." },
      { "B.A.",       "You messed up, now I gotta mess you up. It's the law!" },
      { "B.A.",       "Everybody knows the mans a fool!, he's crazy, he \n"
	              "sees people that ain't there,and he's always talkin'\n"
	              "in circles!" },
      { "B.A.",       "That's it. You're going into the water." },
      { "B.A.",       "Gimme a cup of coffee!\n"
	              "[Diner Clerk: How do you want it?]\n"
	              "In a cup, fool!" },
      { "Hannibal",   "A set of lock picks! You know, sometimes Face, your\n"
	              "sense of larceny is your most attractive trait!" },
      { "Hannibal",   "Great, Murdock, just great" },
      { "Face",       "You know, Murdock, you look more attractive to me as\n"
	              "a woman than you do as a man!" },
      { "Murdock",    "Face usally does this, makes it look real easy. An\n"
	              "honest man doesn't really have the knack for this\n"
	              "sort of thing..." },
      { "Murdock",    "I got no fear. I'll go up in anything: except an elevator." },
      { "Face",       "Hannibal, I don't like it when you get that look." },
      { "Face",       "[to Hannibal] Don't you smile at me... that's not\n"
	              "even a real smile! It's just a bunch of teeth\n"
	              "playing with my mind." },
      { "Murdock",    "I'm a bird! I'm a plane! I'm a choo-choo train!" } 
    };

  unsigned iquote = rand()%(sizeof(quotes)/sizeof(*quotes));
  
  std::string title = std::string(quotes[iquote].who)+std::string(" says...");

  Message message(Message::DR_LOCAL,Message::PS_UNUSUAL,title);
  message.messageStream() << quotes[iquote].what;
  Messenger::relay()->sendMessage(message);
  scheduleATeamQuote();
}

void GUIArrayPane::scheduleATeamQuote()
{
  int dt = 1000*60*(15+rand()%15);
  QTimer::singleShot(dt,this,SLOT(sendRandomATeamQuote()));
}

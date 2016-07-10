//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUIObjectSelector.cpp
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
 * $Revision: 2.26 $
 * $Tag$
 *
 **/

#include<qlayout.h>
#include<qtooltip.h>
#include<qvalidator.h>

#include<Angle.h>
#include<Astro.h>
#include<Exception.h>

#include"GUIPixmaps.h"
#include"GUISummaryDetailsPane.h"

#include"text.h"

#define DEFAULT_WOBBLE 0.5
#define DISTANT_WOBBLE 4.0
#define DEFAULT_ORBIT_PERIOD_MINUTES 20

using namespace SEphem;
using namespace VTracking;
using namespace VMessaging;

static bool isValid(QLineEdit* le)
{
  int pos=0;
  QString str=le->text();
  return ((le->validator()==0)||
	  (le->validator()->validate(str,pos)==QValidator::Acceptable));
}


GUIObjectSelector::
GUIObjectSelector(unsigned identifier, bool details_pane, 
		  bool use_myqgroupbox, QFont f, int& height,
		  const VTracking::StowObjectVector& stow_pos,
		  std::set<GUIObjectSelector*>* mirrors, bool grb,
		  QWidget* parent, const char* name):
  QFrame(parent,name),
  m_identifier(identifier),
  m_details_pane(details_pane), m_stow_pos(stow_pos), 
  m_target_list(), m_grb_list(), m_mirrors(mirrors),

  m_cmd_stack(0), m_cmd_buttons(0), 

  m_cmd_az(0), m_cmd_el(0), m_cmd_azel_no_use_corrections(0),
  m_cmd_azel_no_stop_at_target(0),

  m_cmd_ra(0), m_cmd_dec(0), m_cmd_radec_mode(0), m_cmd_radec_stack(0),
  m_cmd_radec_onoff(0), m_cmd_radec_wobble_off(0), m_cmd_radec_wobble_dir(0), 
  m_cmd_radec_el_off(0), m_cmd_radec_az_off(0), 
  m_cmd_radec_orbit_period(0), m_cmd_radec_orbit_off(0), 
  m_cmd_radec_orbit_dir(0),
  m_cmd_epoch(0), 

  m_cmd_target(0), m_cmd_load(0), m_cmd_tar_mode(0),  m_cmd_tar_stack(0), 
  m_cmd_tar_onoff(0), m_cmd_tar_wobble_off(0), m_cmd_tar_wobble_dir(0),
  m_cmd_tar_el_off(0), m_cmd_tar_az_off(0),
  m_cmd_tar_orbit_period(0), m_cmd_tar_orbit_off(0), m_cmd_tar_orbit_dir(0),

  m_cmd_grb(0), m_cmd_grb_mode(0), m_cmd_grb_stack(0), m_cmd_grb_onoff(0),
  m_cmd_grb_wobble_off(0), m_cmd_grb_wobble_dir(0),
  m_cmd_grb_el_off(0), m_cmd_grb_az_off(0),
  m_cmd_grb_orbit_period(0), m_cmd_grb_orbit_off(0), m_cmd_grb_orbit_dir(0),

  m_cmd_aux(0), 

  m_cmd_go(0), m_cmd_stop(0), 

  m_cmd_azel_but(0), m_cmd_radec_but(0), m_cmd_target_but(0), 
  m_cmd_other_but(0), m_cmd_grb_but(0),

  m_cmd_scheme_buttons(0), m_go_button_index(0)
{ 
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  QString myname(name);

  QGridLayout* targetlayout = new QGridLayout(this,1,3,0,5,myname+" layout");

  // --------------------------------------------------------------------------
  // SCHEME SELECTION
  // --------------------------------------------------------------------------

  QGroupBox* schemebox;

  if(use_myqgroupbox)
    schemebox = new MyQGroupBox(grb?3:2,Qt::Vertical,"Target Type",
				this,myname+" scheme box");
  else 
    schemebox = new QGroupBox(grb?3:2,Qt::Vertical,"Target Type",
			      this,myname+" scheme box");
  
  m_cmd_azel_but = new QRadioButton("Az/El",schemebox);
  m_cmd_radec_but  = new QRadioButton("RA/Dec",schemebox);
  if(grb)
    {
      m_cmd_grb_but = new QRadioButton("GRB",schemebox);
      m_cmd_grb_but->setEnabled(false);
    }
  m_cmd_target_but = new QRadioButton("Target",schemebox);
  m_cmd_other_but = new QRadioButton("Misc",schemebox);
  m_cmd_azel_but->setChecked(true);

  m_cmd_azel_but->setFont(f);
  m_cmd_radec_but->setFont(f);
  m_cmd_target_but->setFont(f);
  m_cmd_other_but->setFont(f);
  if(grb)m_cmd_grb_but->setFont(f);

  m_cmd_scheme_buttons = new QButtonGroup(0);
  m_cmd_scheme_buttons->insert(m_cmd_azel_but,0);
  m_cmd_scheme_buttons->insert(m_cmd_radec_but,1);
  m_cmd_scheme_buttons->insert(m_cmd_target_but,2);
  m_cmd_scheme_buttons->insert(m_cmd_other_but,3);
  if(grb)m_cmd_scheme_buttons->insert(m_cmd_grb_but,4);
  
  // --------------------------------------------------------------------------
  // Target Details
  // --------------------------------------------------------------------------

  QGroupBox* cmdbox;

  if(use_myqgroupbox)
    cmdbox = 
      new MyQGroupBox(1,Qt::Horizontal,"Details",this,myname+" command box");
  else
    cmdbox = 
      new QGroupBox(1,Qt::Horizontal,"Details",this,myname+" command box");

  m_cmd_stack = new QWidgetStack(cmdbox,myname+" cmd stack");

  // --------------------------------------------------------------------------
  // AZ/EL
  // --------------------------------------------------------------------------

  QFrame* cmd_azel_frame = new QFrame(m_cmd_stack,myname+" cmd azel frame");
  QGridLayout* cmd_azel_layout = 
    new QGridLayout(cmd_azel_frame,2,3,0,5,myname+" cmd azel layout");

  QLabel* cmd_az_lab = new QLabel("Azimuth",cmd_azel_frame,
				  myname+" cmd az lab");
  cmd_az_lab->setFont(f);  

  m_cmd_az = new InfoQLineEdit("+888.8888",f,true,true,cmd_azel_frame,
			       myname+" cmd az");
  m_cmd_az->setText("");  

  QLabel* cmd_el_lab = new QLabel("Elevation",cmd_azel_frame,
				  myname+" cmd el lab");
  cmd_el_lab->setFont(f);  

  m_cmd_el = new InfoQLineEdit("+88.8888",f,true,true,cmd_azel_frame,
			       myname+" cmd el");
  m_cmd_el->setText("");  

  QHBoxLayout* cmd_azel_radio_layout =
    new QHBoxLayout(5,myname+" cmd azel radio layout");
  
  m_cmd_azel_no_use_corrections = 
    new QRadioButton("Do NOT Use Corrections",cmd_azel_frame,
		     myname+" cmd azel use corrections radio");
  m_cmd_azel_no_stop_at_target = 
    new QRadioButton("Do NOT Stop At Target",cmd_azel_frame,
		     myname+" cmd azel stop at target radio");
  cmd_azel_radio_layout->addWidget(m_cmd_azel_no_use_corrections);
  cmd_azel_radio_layout->addWidget(m_cmd_azel_no_stop_at_target);

  cmd_azel_layout->addWidget(cmd_az_lab,0,0);
  cmd_azel_layout->addWidget(m_cmd_az,0,1);
  cmd_azel_layout->addWidget(cmd_el_lab,1,0);
  cmd_azel_layout->addWidget(m_cmd_el,1,1);
  cmd_azel_layout->addMultiCellLayout(cmd_azel_radio_layout,2,2,0,1);

  cmd_azel_layout->setRowStretch(0,0);
  cmd_azel_layout->setRowStretch(1,0);
  cmd_azel_layout->setRowStretch(2,1);

  m_cmd_stack->addWidget(cmd_azel_frame);

  QRegExp azre("[+-]?[.][0-9]{0,4}|"
	       "[+-]?(([12]?[0-9]{2,1}|3[0-5][0-9])([.][0-9]{0,4})?|"
	       "[+-]?360([.]0{0,4}))");
  QValidator* azval = new QRegExpValidator(azre, m_cmd_az,
					   myname+" cmd az validator");
  m_cmd_az->setValidator(azval);

  QRegExp elre("[+-]?[.][0-9]{0,4}|"
	       "[+]?90([.]0{0,4})?|"
	       "[+]?[1-8]?[0-9]([.][0-9]{0,4})?|"
	       "[-]5([.]0{0,4})?|"
	       "[-][0-4]([.][0-9]{0,4})?");
  QValidator* elval = new QRegExpValidator(elre, m_cmd_el, 
					   myname+" cmd el validator");
  m_cmd_el->setValidator(elval);

  // --------------------------------------------------------------------------
  // RA/DEC
  // --------------------------------------------------------------------------

  QFrame* cmd_radec_frame = new QFrame(m_cmd_stack,myname+" cmd radec frame");
  QGridLayout* cmd_radec_layout = 
    new QGridLayout(cmd_radec_frame,3,3,0,5,myname+" cmd radec layout");

  QLabel* cmd_ra_lab = new QLabel("RA",cmd_radec_frame,myname+" cmd ra lab");
  cmd_ra_lab->setFont(f);

  m_cmd_ra = 
    new InfoQLineEdit("88:88:88.8",f,true,true,cmd_radec_frame,
		      myname+" cmd ra");
  m_cmd_ra->setText("");

  QLabel* cmd_dec_lab = new QLabel("Dec",cmd_radec_frame,
				   myname+" cmd dec lab");
  cmd_dec_lab->setFont(f);

  m_cmd_dec = 
    new InfoQLineEdit("+88:88:88.8",f,true,true,cmd_radec_frame,
		      myname+" cmd dec");
  m_cmd_dec->setText("");

  QLabel* cmd_epoch_lab = new QLabel("Epoch",cmd_radec_frame,
				     myname+" cmd epoch lab");
  cmd_epoch_lab->setFont(f);

  m_cmd_epoch = 
    new InfoQLineEdit("8888.8",f,true,true,cmd_radec_frame,
		      myname+" cmd epoch");
  m_cmd_epoch->setText("2000.0");

  makeTrackingModeWidgets(cmd_radec_frame,myname+" cmd radec",f,true,
			  m_cmd_radec_mode, m_cmd_radec_stack,
			  m_cmd_radec_onoff, 
			  m_cmd_radec_wobble_dir, m_cmd_radec_wobble_off,
			  m_cmd_radec_el_off, m_cmd_radec_az_off,
			  m_cmd_radec_orbit_period, 
			  m_cmd_radec_orbit_off, m_cmd_radec_orbit_dir);

  cmd_radec_layout->addWidget(cmd_ra_lab,0,0);
  cmd_radec_layout->addWidget(m_cmd_ra,0,1);
  cmd_radec_layout->addWidget(cmd_dec_lab,1,0);
  cmd_radec_layout->addWidget(m_cmd_dec,1,1);
  cmd_radec_layout->addWidget(cmd_epoch_lab,2,0);
  cmd_radec_layout->addWidget(m_cmd_epoch,2,1);
  cmd_radec_layout->addWidget(m_cmd_radec_mode,0,2);
  cmd_radec_layout->addMultiCellWidget(m_cmd_radec_stack,1,2,2,2);

  for(int i=0;i<4;i++)cmd_radec_layout->setColStretch(i,0);
  cmd_radec_layout->setColStretch(1,1);

  QValidator* ra_val = new QRegExpValidator(raValidatorRE(), m_cmd_ra, 
					    myname+" cmd ra validator");
  m_cmd_ra->setValidator(ra_val);

  QValidator* de_val = new QRegExpValidator(decValidatorRE(), m_cmd_dec, 
					    myname+" cmd dec validator");
  m_cmd_dec->setValidator(de_val);

  QValidator* epval = new QRegExpValidator(epochValidatorRE(), m_cmd_epoch, 
					   myname+" cmd ep validator");
  m_cmd_epoch->setValidator(epval);

  // --------------------------------------------------------------------------
  // TARGET
  // --------------------------------------------------------------------------

  QFrame* cmd_target_frame = new QFrame(m_cmd_stack,
					myname+" cmd target frame");
  QGridLayout* cmd_target_layout = 
    new QGridLayout(cmd_target_frame,3,2,0,5,myname+" cmd target layout");
  
  m_cmd_target = new MyQComboBox(false,cmd_target_frame,myname+" cmd target");
  m_cmd_target->setFont(f);

  m_cmd_load = new QPushButton("Load Targets",cmd_target_frame,
				   myname+" cmd target load");
  m_cmd_load->setFont(f);

  connect(m_cmd_load,SIGNAL(clicked()),this,SLOT(sendLoadNewTargetList()));

  makeTrackingModeWidgets(cmd_target_frame,myname+" cmd target",f,false,
			  m_cmd_tar_mode, m_cmd_tar_stack,
			  m_cmd_tar_onoff, 
			  m_cmd_tar_wobble_dir, m_cmd_tar_wobble_off,
			  m_cmd_tar_el_off, m_cmd_tar_az_off,
			  m_cmd_tar_orbit_period, 
			  m_cmd_tar_orbit_off, m_cmd_tar_orbit_dir);

  cmd_target_layout->addMultiCellWidget(m_cmd_target,0,0,0,1);
  cmd_target_layout->addWidget(m_cmd_load,1,0);
  cmd_target_layout->addWidget(m_cmd_tar_mode,2,0);
  cmd_target_layout->addMultiCellWidget(m_cmd_tar_stack,1,2,1,1);

  cmd_target_layout->setColStretch(0,1);
  cmd_target_layout->setColStretch(1,1);

  // --------------------------------------------------------------------------
  // AUX
  // --------------------------------------------------------------------------
  
  QFrame* cmd_aux_frame = new QFrame(m_cmd_stack,myname+" cmd aux frame");
  QGridLayout* cmd_aux_layout = 
    new QGridLayout(cmd_aux_frame,2,2,0,5,myname+" cmd aux layout");

  m_cmd_aux = new MyQComboBox(false,cmd_aux_frame,myname+" cmd aux");
  m_cmd_aux->setFont(f);

  int iposnum = 0;
  for(StowObjectVector::const_iterator ipos = m_stow_pos.begin();
      ipos != m_stow_pos.end(); ipos++, iposnum++)
    m_cmd_aux->insertItem(ipos->name().c_str(),iposnum);
  m_cmd_aux->insertItem("Zenith",iposnum++);
  m_cmd_aux->insertItem("Polaris",iposnum++);

  cmd_aux_layout->addMultiCellWidget(m_cmd_aux,0,0,0,1);
  cmd_aux_layout->setColStretch(0,1);

  // --------------------------------------------------------------------------
  // GRB
  // --------------------------------------------------------------------------

  QFrame* cmd_grb_frame = 0;
  if(grb)
    {
      cmd_grb_frame = 
	new QFrame(m_cmd_stack, myname+" cmd grb frame");
      QGridLayout* cmd_grb_layout = 
	new QGridLayout(cmd_grb_frame,3,2,0,5,myname+" cmd grb layout");
  
      m_cmd_grb = new MyQComboBox(false,cmd_grb_frame,myname+" cmd grb");
      m_cmd_grb->setFont(f);

      makeTrackingModeWidgets(cmd_grb_frame,myname+" cmd grb",f,true,
			      m_cmd_grb_mode, m_cmd_grb_stack,
			      m_cmd_grb_onoff, 
			      m_cmd_grb_wobble_dir, m_cmd_grb_wobble_off,
			      m_cmd_grb_el_off, m_cmd_grb_az_off,
			      m_cmd_grb_orbit_period, 
			      m_cmd_grb_orbit_off, m_cmd_grb_orbit_dir);

      cmd_grb_layout->addMultiCellWidget(m_cmd_grb,0,0,0,1);
      cmd_grb_layout->addWidget(m_cmd_grb_mode,1,0);
      cmd_grb_layout->addMultiCellWidget(m_cmd_grb_stack,1,2,1,1);
    }

  // --------------------------------------------------------------------------
  // ADD THE WIDGETS TO THE STACK
  // --------------------------------------------------------------------------

  m_cmd_stack->addWidget(cmd_azel_frame,0);
  m_cmd_stack->addWidget(cmd_radec_frame,1);
  m_cmd_stack->addWidget(cmd_target_frame,2);
  m_cmd_stack->addWidget(cmd_aux_frame,3);
  if(grb)m_cmd_stack->addWidget(cmd_grb_frame,4);

  connect(m_cmd_scheme_buttons, SIGNAL(pressed(int)),
	  m_cmd_stack, SLOT(raiseWidget(int)));

  // --------------------------------------------------------------------------
  // BUTTONS
  // --------------------------------------------------------------------------

  QGroupBox* actbox;
  if(use_myqgroupbox)
    actbox = 
      new MyQGroupBox(1,Qt::Horizontal,"Go",this,myname+" gostop box");
  else
    actbox = 
      new QGroupBox(1,Qt::Horizontal,"Go",this,myname+" gostop box");

  height = m_cmd_stack->sizeHint().height()-8;

  m_cmd_buttons = new QWidgetStack(actbox,myname+" sum cmd buttons");
  m_cmd_go = new MyQPushButton(m_cmd_buttons,myname+" sum cmd go");
  m_cmd_stop = new MyQPushButton(m_cmd_buttons,myname+" sum cmd stop");
  m_cmd_buttons->addWidget(m_cmd_go,0);
  m_cmd_buttons->addWidget(m_cmd_stop,1);
  
  if(m_details_pane)
    {
      GUIPixmaps::instance()->setGoStopSize(height);
      m_cmd_go->setPixmap(*GUIPixmaps::instance()->go_pixmaps(0));
      m_cmd_stop->setPixmap(*GUIPixmaps::instance()->stop_pixmaps(0));
    }
  else
    {
      GUIPixmaps::instance()->setSumGoStopSize(height);
      m_cmd_go->setPixmap(*GUIPixmaps::instance()->sum_go_pixmaps(0));
      m_cmd_stop->setPixmap(*GUIPixmaps::instance()->sum_stop_pixmaps(0));
    }

  // --------------------------------------------------------------------------
  // CONNECT THE LINE EDITS / COMBO BOXES / RADIO BUTTONS / PUSH BUTTONS
  // --------------------------------------------------------------------------

  QLineEdit* settarget_le[] = 
    { m_cmd_az, m_cmd_el, m_cmd_ra, m_cmd_dec, m_cmd_epoch };

  for(unsigned i=0;i<sizeof(settarget_le)/sizeof(*settarget_le);i++)
    connect(settarget_le[i],SIGNAL(returnPressed()),
	    this,SLOT(sendSetTarget()));
  
  QComboBox* settarget_cb[] = 
    { m_cmd_radec_mode, m_cmd_radec_onoff, 
      m_cmd_radec_wobble_dir, m_cmd_radec_wobble_off, 
      m_cmd_radec_el_off, m_cmd_radec_az_off, 
      m_cmd_radec_orbit_dir, m_cmd_radec_orbit_off, m_cmd_radec_orbit_period,
      m_cmd_tar_mode, m_cmd_tar_onoff, 
      m_cmd_tar_el_off, m_cmd_tar_az_off, 
      m_cmd_tar_wobble_dir, m_cmd_tar_wobble_off, 
      m_cmd_tar_orbit_dir, m_cmd_tar_orbit_off, m_cmd_tar_orbit_period,
      m_cmd_target, m_cmd_aux };

  QComboBox* settarget_grb_cb[] = 
    { m_cmd_grb_mode, m_cmd_grb_onoff, 
      m_cmd_grb_wobble_dir, m_cmd_grb_wobble_off, 
      m_cmd_grb_el_off, m_cmd_grb_az_off, 
      m_cmd_grb_orbit_dir, m_cmd_grb_orbit_off, m_cmd_grb_orbit_period,
      m_cmd_grb };

  connect(m_cmd_target,SIGNAL(activated(int)),
	  this,SLOT(setModeToOn()));
  
  for(unsigned i=0;i<sizeof(settarget_cb)/sizeof(*settarget_cb);i++)
    connect(settarget_cb[i],SIGNAL(activated(int)),
	    this,SLOT(sendSetTarget()));

  if(grb)
    for(unsigned i=0;i<sizeof(settarget_grb_cb)/sizeof(*settarget_grb_cb);i++)
      connect(settarget_grb_cb[i],SIGNAL(activated(int)),
	      this,SLOT(sendSetTarget()));
  
  QRadioButton* settarget_rb[] =
    { m_cmd_azel_but, m_cmd_radec_but, m_cmd_target_but, m_cmd_other_but,
      m_cmd_azel_no_use_corrections, m_cmd_azel_no_stop_at_target};
  
  for(unsigned i=0;i<sizeof(settarget_rb)/sizeof(*settarget_rb);i++)
    connect(settarget_rb[i],SIGNAL(clicked()),
	    this,SLOT(sendSetTarget()));
  if(grb)connect(m_cmd_grb_but,SIGNAL(clicked()),this,SLOT(sendSetTarget()));

  connect(m_cmd_go, SIGNAL(clicked()), this, SLOT(sendGo()));
  connect(m_cmd_stop, SIGNAL(clicked()), this, SLOT(sendStop()));

  // --------------------------------------------------------------------------
  // ADD WIDGETS TO LAYOUT
  // --------------------------------------------------------------------------

  targetlayout->addWidget(schemebox,0,0);
  targetlayout->addWidget(cmdbox,0,1);
  targetlayout->addWidget(actbox,0,2);
  targetlayout->setColStretch(0,0);
  targetlayout->setColStretch(1,1);
  targetlayout->setColStretch(2,0);

  struct { QWidget* widget; const QString tip; } thetips[] = {
    { m_cmd_az,                      QString(TT_OBJSEL_AZ) },
    { m_cmd_el,                      QString(TT_OBJSEL_EL) },
    { m_cmd_azel_no_use_corrections, QString(TT_OBJSEL_NO_CORR) },
    { m_cmd_azel_no_stop_at_target,  QString(TT_OBJSEL_NO_STOP) },
    { m_cmd_ra,                      QString(TT_OBJSEL_RA) },
    { m_cmd_dec,                     QString(TT_OBJSEL_DEC) },
    { m_cmd_radec_mode,              QString(TT_OBJSEL_RADEC_MODE) },
    { m_cmd_radec_onoff,             QString(TT_OBJSEL_ONOFF) },
    { m_cmd_radec_wobble_dir,        QString(TT_OBJSEL_WOB_DIR) },
    { m_cmd_radec_wobble_off,        QString(TT_OBJSEL_WOB_OFF) },
    { m_cmd_radec_el_off,            QString(TT_OBJSEL_EL_OFF) },
    { m_cmd_radec_az_off,            QString(TT_OBJSEL_AZ_OFF) },
    { m_cmd_radec_orbit_period,      QString(TT_OBJSEL_ORB_PER) },
    { m_cmd_radec_orbit_off,         QString(TT_OBJSEL_ORB_OFF) },
    { m_cmd_radec_orbit_dir,         QString(TT_OBJSEL_ORB_DIR) },
    { m_cmd_epoch,                   QString(TT_OBJSEL_EPOCH) },
    { m_cmd_target,                  QString(TT_OBJSEL_TARGET) },
    { m_cmd_load,                    QString(TT_OBJSEL_TARGET_LOAD) },
    { m_cmd_tar_mode,                QString(TT_OBJSEL_TARGET_MODE) },
    { m_cmd_tar_onoff,               QString(TT_OBJSEL_ONOFF) },
    { m_cmd_tar_wobble_dir,          QString(TT_OBJSEL_WOB_DIR) },
    { m_cmd_tar_wobble_off,          QString(TT_OBJSEL_WOB_OFF) },
    { m_cmd_tar_el_off,              QString(TT_OBJSEL_EL_OFF) },
    { m_cmd_tar_az_off,              QString(TT_OBJSEL_AZ_OFF) },
    { m_cmd_tar_orbit_period,        QString(TT_OBJSEL_ORB_PER) },
    { m_cmd_tar_orbit_off,           QString(TT_OBJSEL_ORB_OFF) },
    { m_cmd_tar_orbit_dir,           QString(TT_OBJSEL_ORB_DIR) },
    { m_cmd_aux,                     QString(TT_OBJSEL_MISC) },
    { m_cmd_go,                      QString(TT_OBJSEL_GO) },
    { m_cmd_stop,                    QString(TT_OBJSEL_STOP) }
  };

  struct { QWidget* widget; const QString tip; } thetips_grb[] = {
    { m_cmd_grb,                     QString(TT_OBJSEL_GRB) },
    { m_cmd_grb_mode,                QString(TT_OBJSEL_TARGET_MODE) },
    { m_cmd_grb_onoff,               QString(TT_OBJSEL_ONOFF) },
    { m_cmd_grb_wobble_dir,          QString(TT_OBJSEL_WOB_DIR) },
    { m_cmd_grb_wobble_off,          QString(TT_OBJSEL_WOB_OFF) },
    { m_cmd_grb_el_off,              QString(TT_OBJSEL_EL_OFF) },
    { m_cmd_grb_az_off,              QString(TT_OBJSEL_AZ_OFF) },
    { m_cmd_grb_orbit_period,        QString(TT_OBJSEL_ORB_PER) },
    { m_cmd_grb_orbit_off,           QString(TT_OBJSEL_ORB_OFF) },
    { m_cmd_grb_orbit_dir,           QString(TT_OBJSEL_ORB_DIR) },
  };

  for(unsigned i=0; i<sizeof(thetips)/sizeof(*thetips); i++)
    QToolTip::add(thetips[i].widget, thetips[i].tip);
  if(grb)
    for(unsigned i=0; i<sizeof(thetips_grb)/sizeof(*thetips_grb); i++)
      QToolTip::add(thetips_grb[i].widget, thetips_grb[i].tip);

  // --------------------------------------------------------------------------
  // SET UP MIRRORING
  // --------------------------------------------------------------------------

  if(m_mirrors)
    {
      m_mirrors->insert(this);

      // LINE EDITS
      connect(m_cmd_az, SIGNAL(textChanged(const QString&)), 
	      this, SLOT(mirror()));
      connect(m_cmd_el, SIGNAL(textChanged(const QString&)), 
	      this, SLOT(mirror()));
      connect(m_cmd_ra, SIGNAL(textChanged(const QString&)), 
	      this, SLOT(mirror()));
      connect(m_cmd_dec, SIGNAL(textChanged(const QString&)), 
	      this, SLOT(mirror()));
      connect(m_cmd_epoch, SIGNAL(textChanged(const QString&)), 
	      this, SLOT(mirror()));
      
      for(std::set<GUIObjectSelector*>::iterator i=m_mirrors->begin();
	  i!=m_mirrors->end(); i++)
	if(*i != this)
	  {
	    // RADIO BUTTONS

#define CONNECTRADIO_FWD(x)						\
	    connect(x, SIGNAL(toggled(bool)), (*i)->x, SLOT(setChecked(bool)))
#define CONNECTRADIO_BWD(x)						\
	    connect((*i)->x, SIGNAL(toggled(bool)), x, SLOT(setChecked(bool)))
	    
	    CONNECTRADIO_FWD(m_cmd_azel_but);
	    CONNECTRADIO_FWD(m_cmd_radec_but);
	    CONNECTRADIO_FWD(m_cmd_target_but);
 	    if(m_cmd_grb_but && (*i)->m_cmd_grb_but)
	      CONNECTRADIO_FWD(m_cmd_grb_but);
 	    CONNECTRADIO_FWD(m_cmd_other_but);
	    CONNECTRADIO_FWD(m_cmd_azel_no_use_corrections);
	    CONNECTRADIO_FWD(m_cmd_azel_no_stop_at_target);
	    
	    CONNECTRADIO_BWD(m_cmd_azel_but);
	    CONNECTRADIO_BWD(m_cmd_radec_but);
	    CONNECTRADIO_BWD(m_cmd_target_but);
 	    if(m_cmd_grb_but && (*i)->m_cmd_grb_but)
	      CONNECTRADIO_BWD(m_cmd_grb_but);
	    CONNECTRADIO_BWD(m_cmd_other_but);
	    CONNECTRADIO_BWD(m_cmd_azel_no_use_corrections);
	    CONNECTRADIO_BWD(m_cmd_azel_no_stop_at_target);
	    
	    // BUTTON STACK

	    connect(m_cmd_scheme_buttons,SIGNAL(pressed(int)),
		    (*i)->m_cmd_stack,SLOT(raiseWidget(int)));
	    
	    connect((*i)->m_cmd_scheme_buttons,SIGNAL(pressed(int)),
		    m_cmd_stack,SLOT(raiseWidget(int)));
	    
	    // COMBO BOXES
	    
#define CONNECTCOMBO_FWD(x)						\
	    connect(x, SIGNAL(activated(int)), \
		    (*i)->x, SLOT(setActivated(int)))
#define CONNECTCOMBO_BWD(x)						\
	    connect((*i)->x, SIGNAL(activated(int)), \
		    x, SLOT(setActivated(int)))
	    
	    CONNECTCOMBO_FWD(m_cmd_radec_mode);
	    CONNECTCOMBO_FWD(m_cmd_radec_onoff);
	    CONNECTCOMBO_FWD(m_cmd_radec_wobble_off);
	    CONNECTCOMBO_FWD(m_cmd_radec_wobble_dir);
	    CONNECTCOMBO_FWD(m_cmd_radec_el_off);
	    CONNECTCOMBO_FWD(m_cmd_radec_az_off);
	    CONNECTCOMBO_FWD(m_cmd_target);
	    CONNECTCOMBO_FWD(m_cmd_tar_mode);
	    CONNECTCOMBO_FWD(m_cmd_tar_onoff);
	    CONNECTCOMBO_FWD(m_cmd_tar_wobble_off);
	    CONNECTCOMBO_FWD(m_cmd_tar_wobble_dir);
	    CONNECTCOMBO_FWD(m_cmd_tar_el_off);
	    CONNECTCOMBO_FWD(m_cmd_tar_az_off);
	    if(m_cmd_grb && (*i)->m_cmd_grb)
	      {
		CONNECTCOMBO_FWD(m_cmd_grb);
		CONNECTCOMBO_FWD(m_cmd_grb_mode);
		CONNECTCOMBO_FWD(m_cmd_grb_onoff);
		CONNECTCOMBO_FWD(m_cmd_grb_wobble_off);
		CONNECTCOMBO_FWD(m_cmd_grb_wobble_dir);
		CONNECTCOMBO_FWD(m_cmd_grb_el_off);
		CONNECTCOMBO_FWD(m_cmd_grb_az_off);
	      }
	    CONNECTCOMBO_FWD(m_cmd_aux);
	    
	    CONNECTCOMBO_BWD(m_cmd_radec_mode);
	    CONNECTCOMBO_BWD(m_cmd_radec_onoff);
	    CONNECTCOMBO_BWD(m_cmd_radec_wobble_off);
	    CONNECTCOMBO_BWD(m_cmd_radec_wobble_dir);
	    CONNECTCOMBO_BWD(m_cmd_radec_el_off);
	    CONNECTCOMBO_BWD(m_cmd_radec_az_off);
	    CONNECTCOMBO_BWD(m_cmd_target);
	    CONNECTCOMBO_BWD(m_cmd_tar_mode);
	    CONNECTCOMBO_BWD(m_cmd_tar_onoff);
	    CONNECTCOMBO_BWD(m_cmd_tar_wobble_off);
	    CONNECTCOMBO_BWD(m_cmd_tar_wobble_dir);
	    CONNECTCOMBO_BWD(m_cmd_tar_el_off);
	    CONNECTCOMBO_BWD(m_cmd_tar_az_off);
	    if(m_cmd_grb && (*i)->m_cmd_grb)
	      {
		CONNECTCOMBO_BWD(m_cmd_grb);
		CONNECTCOMBO_BWD(m_cmd_grb_mode);
		CONNECTCOMBO_BWD(m_cmd_grb_onoff);
		CONNECTCOMBO_BWD(m_cmd_grb_wobble_off);
		CONNECTCOMBO_BWD(m_cmd_grb_wobble_dir);
		CONNECTCOMBO_BWD(m_cmd_grb_el_off);
		CONNECTCOMBO_BWD(m_cmd_grb_az_off);
	      }
	    CONNECTCOMBO_BWD(m_cmd_aux);
	    
	    connect(m_cmd_radec_mode, SIGNAL(activated(int)),
		    (*i)->m_cmd_radec_stack, SLOT(raiseWidget(int)));
	    connect((*i)->m_cmd_radec_mode, SIGNAL(activated(int)),
		    m_cmd_radec_stack, SLOT(raiseWidget(int)));	
	    connect(m_cmd_tar_mode, SIGNAL(activated(int)),
		    (*i)->m_cmd_tar_stack, SLOT(raiseWidget(int)));
	    connect((*i)->m_cmd_tar_mode, SIGNAL(activated(int)),
		    m_cmd_tar_stack, SLOT(raiseWidget(int)));	
	    if(m_cmd_grb_mode && (*i)->m_cmd_grb_mode)
	      {
		connect(m_cmd_grb_mode, SIGNAL(activated(int)),
			(*i)->m_cmd_grb_stack, SLOT(raiseWidget(int)));
		connect((*i)->m_cmd_grb_mode, SIGNAL(activated(int)),
			m_cmd_grb_stack, SLOT(raiseWidget(int)));
	      }
	  }
    }
}

GUIObjectSelector::~GUIObjectSelector()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  delete m_cmd_scheme_buttons;
  if(m_mirrors)m_mirrors->erase(this);
}

VTracking::TargetObject* GUIObjectSelector::
getObject(double az_deg, double mjd) const
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  TargetObject* obj=0;
  int scheme = m_cmd_scheme_buttons->selectedId();
  if(scheme==0)
    {
      if((isValid(m_cmd_az))&&(isValid(m_cmd_el)))
	{
	  Angle az=Angle::makeDeg(m_cmd_az->text().toDouble());
	  Angle el=Angle::makeDeg(m_cmd_el->text().toDouble());
	  SphericalCoords pos = SphericalCoords::makeLatLong(el,az);
	  obj = 
	    new AzElObject(pos,
			   !m_cmd_azel_no_use_corrections->isChecked(),
			   !m_cmd_azel_no_stop_at_target->isChecked());
	}
    }
  else if(scheme==1)
    {
      if((isValid(m_cmd_ra))&&(isValid(m_cmd_dec))&&(isValid(m_cmd_epoch)))
	{
	  GUITrackingModeData tm;
	  Angle ra;  ra.setFromHMSString(m_cmd_ra->text());
	  Angle dec; dec.setFromDMSString(m_cmd_dec->text());
	  double epoch=Astro::julianEpochToMJD(m_cmd_epoch->text().toDouble());
	  setTrackingModeData(tm,m_cmd_radec_mode,m_cmd_radec_onoff,
			      m_cmd_radec_wobble_dir,m_cmd_radec_wobble_off,
			      m_cmd_radec_el_off,m_cmd_radec_az_off,
			      m_cmd_radec_orbit_period, 
			      m_cmd_radec_orbit_off, m_cmd_radec_orbit_dir);
	  tm.name=std::string();
	  SphericalCoords pos = SphericalCoords::makeLatLong(dec,ra);
	  CoordinateOffset* off = 0;
	  switch(tm.mode)
	    {
	    case GUITrackingModeData::TM_ONOFF: 
	      switch(tm.onoff)
		{
		case GUITrackingModeData::OOM_ON: 
		  break;
		case GUITrackingModeData::OOM_OFF_AFTER_ON: 
		  off = new OnOffOffset(tm.onoff_time);
		  break;
		case GUITrackingModeData::OOM_OFF_BEFORE_ON: 
		  off = new OnOffOffset(-tm.onoff_time);
		  break;
		};
	      break;
	    case GUITrackingModeData::TM_WOBBLE:
	      off = new WobbleOffset(tm.wobble_coords);
	      break;
	    case GUITrackingModeData::TM_ORBIT:
	      if(tm.orbit_free)
		off = new OrbitOffset(SEphem::SphericalCoords::
				      makeDeg(tm.orbit_coords.theta().deg(),
					      180),
				      tm.orbit_period/24.0/60.0);
	      else
		off = new OrbitOffset(SEphem::SphericalCoords::
				      makeRot(tm.orbit_coords.theta().rot(),
					      tm.orbit_coords.phi().rot() +
					      mjd/(tm.orbit_period/24.0/60.0)),
				      tm.orbit_period/24.0/60.0);
	      break;
	    case GUITrackingModeData::TM_ELAZ:
	      off = new ElAzOffset(tm.elaz_coords);
	      break;
	    case GUITrackingModeData::TM_POINT:
	      break;
	    }
	  obj = new RaDecObject(pos,epoch,tm.name,off);
	}
    }
  else if(scheme==2)
    {
      int item = m_cmd_target->currentItem();
      if((item>=0)&&(item<m_cmd_target->count()))
	{
	  bool has_coords = true;
	  SEphem::SphericalCoords coords = m_target_list[item]->m_obj->coords();
	  double epoch = m_target_list[item]->m_obj->epoch();
	  std::string name = m_target_list[item]->m_name;
	  CoordinateOffset* off = 0;	  
	  GUITrackingModeData tm;
	  setTrackingModeData(tm,m_cmd_tar_mode,m_cmd_tar_onoff,
			      m_cmd_tar_wobble_dir,m_cmd_tar_wobble_off,
			      m_cmd_tar_el_off,m_cmd_tar_az_off,
			      m_cmd_tar_orbit_period, 
			      m_cmd_tar_orbit_off, m_cmd_tar_orbit_dir);
	  switch(tm.mode)
	    {
	    case GUITrackingModeData::TM_ONOFF:
	      switch(tm.onoff)
		{
		case GUITrackingModeData::OOM_ON:
		  break;
		case GUITrackingModeData::OOM_OFF_AFTER_ON:
		  off = new OnOffOffset(tm.onoff_time);
		  break;
		case GUITrackingModeData::OOM_OFF_BEFORE_ON: 
		  off = new OnOffOffset(-tm.onoff_time);
		  break;
		}
	      break;
	    case GUITrackingModeData::TM_WOBBLE:
	      off = new WobbleOffset(tm.wobble_coords);
	      break;
	    case GUITrackingModeData::TM_ORBIT:
	      if(tm.orbit_free)
		off = new OrbitOffset(SEphem::SphericalCoords::
				      makeDeg(tm.orbit_coords.theta().deg(),
					      180),
				      tm.orbit_period/24.0/60.0);
	      else
		off = new OrbitOffset(SEphem::SphericalCoords::
				      makeRot(tm.orbit_coords.theta().rot(),
					      tm.orbit_coords.phi().rot() +
					      mjd/(tm.orbit_period/24.0/60.0)),
				      tm.orbit_period/24.0/60.0);
	      break;
	    case GUITrackingModeData::TM_ELAZ:
	      off = new ElAzOffset(tm.elaz_coords);
	      break;
	    case GUITrackingModeData::TM_POINT:
	      has_coords = false;
	      if(m_target_list[item]->m_pnt != 0)
		{
		  has_coords = true;		  
		  coords = m_target_list[item]->m_pnt->coords();
		  epoch = m_target_list[item]->m_pnt->epoch();
		  name = std::string("Pointing [")+
		    m_target_list[item]->m_name+
		    std::string("]");
		}
	      break;
	    }
	  if(has_coords)
	    obj = new RaDecObject(coords, epoch, name, off);
	}
    }
  else if(scheme==3)
    {
      int auxid=m_cmd_aux->currentItem();
      int nstow=(int)m_stow_pos.size();

      if(auxid<nstow)
	{
	  obj = m_stow_pos[auxid].copy();
	}
      else if(auxid==nstow)
	{
	  obj = new AzElObject(SphericalCoords::makeLatLongDeg(85,az_deg),false,true);
	}
      else if(auxid==nstow+1)
	{
	  GUITrackingModeData tm;
	  tm.name="Polaris";
	  Angle ra; ra.setFromHMSString("02:31:50.5");
	  Angle dec; dec.setFromDMSString("+89:15:51");  
	  double epoch = Astro::julianEpochToMJD(2000.0);
	  SphericalCoords pos = SphericalCoords::makeLatLong(dec,ra);
	  obj = new RaDecObject(pos, epoch, "Polaris");
	}
    }      
  else if((scheme==4)&&(m_cmd_grb)&&(!m_grb_list.empty()))
    {
      int item = m_cmd_grb->currentItem();
      if((item>=0)&&(item<m_cmd_grb->count()))
	{
	  GUITrackingModeData tm;
	  setTrackingModeData(tm,m_cmd_grb_mode,m_cmd_grb_onoff,
			      m_cmd_grb_wobble_dir,m_cmd_grb_wobble_off,
			      m_cmd_grb_el_off,m_cmd_grb_az_off,
			      m_cmd_grb_orbit_period, 
			      m_cmd_grb_orbit_off, m_cmd_grb_orbit_dir);

	  GUIGRBMonitor::GRBTriggerList::const_iterator igrb = 
	    m_grb_list.begin();
	  for(int iitem = 0; iitem<item;iitem++)igrb++;
	  CoordinateOffset* off = 0;	  
	  
	  switch(tm.mode)
	    {
	    case GUITrackingModeData::TM_ONOFF:
	      switch(tm.onoff)
		{
		case GUITrackingModeData::OOM_ON:
		  break;
		case GUITrackingModeData::OOM_OFF_AFTER_ON: 
		  off = new OnOffOffset(tm.onoff_time);
		  break;
		case GUITrackingModeData::OOM_OFF_BEFORE_ON: 
		  off = new OnOffOffset(-tm.onoff_time);
		  break;
		}
	      break;
	    case GUITrackingModeData::TM_WOBBLE:
	      off = new WobbleOffset(tm.wobble_coords);
	      break;
	    case GUITrackingModeData::TM_ORBIT:
	      if(tm.orbit_free)
		off = new OrbitOffset(SEphem::SphericalCoords::
				      makeDeg(tm.orbit_coords.theta().deg(),
					      180),
				      tm.orbit_period/24.0/60.0);
	      else
		off = new OrbitOffset(SEphem::SphericalCoords::
				      makeRot(tm.orbit_coords.theta().rot(),
					      tm.orbit_coords.phi().rot() +
					      mjd/(tm.orbit_period/24.0/60.0)),
				      tm.orbit_period/24.0/60.0);
	      break;
	    case GUITrackingModeData::TM_ELAZ:
	      off = new ElAzOffset(tm.elaz_coords);
	      break;
	    case GUITrackingModeData::TM_POINT:
	      assert(0);
	      break;
	    }
	  obj = new RaDecObject(igrb->obj->coords(), igrb->obj->epoch(),
				igrb->obj->name(), off);
	}
    }  

  return obj;
}

void GUIObjectSelector::
updateButtons(bool controls_enabled,
	      TelescopeController::TrackingState state,
	      TelescopeController::TrackingRequest req)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(!isVisible())return;

  bool canchange = ((controls_enabled)&&
		    (req==TelescopeController::REQ_STOP)&&
		    (state!=TelescopeController::TS_COM_FAILURE));

  m_cmd_azel_but->setEnabled(canchange);
  m_cmd_radec_but->setEnabled(canchange);
  m_cmd_target_but->setEnabled(canchange);
  if(m_cmd_grb_but)
    m_cmd_grb_but->setEnabled(canchange && (!m_grb_list.empty()));
  m_cmd_other_but->setEnabled(canchange);
  m_cmd_stack->setEnabled(canchange);
  
  if(req==TelescopeController::REQ_STOP)
    {
      if(m_cmd_buttons->id(m_cmd_buttons->visibleWidget())==1)
	{
	  bool change_focus = m_cmd_buttons->visibleWidget()->hasFocus();
	  m_cmd_buttons->raiseWidget(0);
	  if(change_focus)m_cmd_buttons->visibleWidget()->setFocus();
	}
    }
  else
    {
      if(m_cmd_buttons->id(m_cmd_buttons->visibleWidget())==0)
	{
	  bool change_focus = m_cmd_buttons->visibleWidget()->hasFocus();
	  m_cmd_buttons->raiseWidget(1);
	  if(change_focus)m_cmd_buttons->visibleWidget()->setFocus();
	}
    }
  
  m_cmd_buttons->setEnabled((controls_enabled)&&
			    (state!=TelescopeController::TS_COM_FAILURE));
}

void GUIObjectSelector::animateButtons()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if((m_cmd_buttons->isVisible())&&(m_cmd_buttons->isEnabled()))
    {
      if(m_details_pane)
	m_cmd_go->setPixmap(*GUIPixmaps::instance()->
			    go_pixmaps(m_go_button_index++));
      else
	m_cmd_go->setPixmap(*GUIPixmaps::instance()->
			    sum_go_pixmaps(m_go_button_index++));
      if(m_go_button_index==24)m_go_button_index=0;
    }
}

void GUIObjectSelector::
syncWithTargetList(const VTracking::TargetList& target_list)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  m_target_list = target_list;
  m_cmd_target->clear();
  for(VTracking::TargetList::const_iterator i = target_list.begin();
      i != target_list.end(); i++)
    m_cmd_target->insertItem((*i)->m_name);
  if(m_cmd_target->isVisible())sendSetTarget();
}

void GUIObjectSelector::
syncWithGRBList(const GUIGRBMonitor::GRBTriggerList& grb_list)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(m_cmd_grb)
    {
      m_grb_list = grb_list;
      m_cmd_grb->clear();
      for(GUIGRBMonitor::GRBTriggerList::const_iterator igrb=grb_list.begin();
	  igrb != grb_list.end(); igrb++)
	if((strncmp(igrb->grb->trigger_type,"RETRACTION",10)!=0)
	   &&(igrb->retraction==0))
	  m_cmd_grb->insertItem(igrb->obj->name());
      m_cmd_grb_but->setEnabled(true);
      if(m_cmd_grb->isVisible())sendSetTarget();
    }
}

static void mirrorLE(const QLineEdit* src, QLineEdit* dst)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if((src->isVisible())&&(src->text() != dst->text()))
    dst->setText(src->text());
}

void GUIObjectSelector::mirror()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  static bool recursing=false;
  if(recursing)return;
  recursing=true;

  assert(m_mirrors);

  for(std::set<GUIObjectSelector*>::iterator i=m_mirrors->begin();
      i!=m_mirrors->end(); i++)
    if(*i != this)
      {
	mirrorLE(m_cmd_az, (*i)->m_cmd_az);
	mirrorLE(m_cmd_el, (*i)->m_cmd_el);
	mirrorLE(m_cmd_ra, (*i)->m_cmd_ra);
	mirrorLE(m_cmd_dec, (*i)->m_cmd_dec);
	mirrorLE(m_cmd_epoch,  (*i)->m_cmd_epoch);
      }

  recursing=false;
}

void GUIObjectSelector::setModeToOn()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  static bool recursing=false;
  if(recursing)return;
  recursing=true;

  if(m_cmd_tar_mode->currentItem() == 2)
    m_cmd_tar_mode->setCurrentItem(0),
      m_cmd_tar_stack->raiseWidget(0);
  m_cmd_tar_onoff->setCurrentItem(0);
  m_cmd_tar_wobble_off->setCurrentItem(0);
  m_cmd_tar_wobble_dir->setCurrentItem(0);
  m_cmd_tar_el_off->setCurrentItem(0);
  m_cmd_tar_az_off->setCurrentItem(0);

  if(m_cmd_grb)
    {
      if(m_cmd_grb_mode->currentItem() == 2)
	m_cmd_grb_mode->setCurrentItem(0),
	  m_cmd_grb_stack->raiseWidget(0);
      m_cmd_grb_onoff->setCurrentItem(0);
      m_cmd_grb_wobble_off->setCurrentItem(0);
      m_cmd_grb_wobble_dir->setCurrentItem(0);
      m_cmd_grb_el_off->setCurrentItem(0);
      m_cmd_grb_az_off->setCurrentItem(0);
    }

  if(m_mirrors)
    for(std::set<GUIObjectSelector*>::iterator i=m_mirrors->begin();
	i!=m_mirrors->end(); i++)
      {
	if((*i)->m_cmd_tar_mode->currentItem() == 2)
	  (*i)->m_cmd_tar_mode->setCurrentItem(0),
	    (*i)->m_cmd_tar_stack->raiseWidget(0);
	(*i)->m_cmd_tar_onoff->setCurrentItem(0);
	(*i)->m_cmd_tar_wobble_off->setCurrentItem(0);
	(*i)->m_cmd_tar_wobble_dir->setCurrentItem(0);
	(*i)->m_cmd_tar_el_off->setCurrentItem(0);
	(*i)->m_cmd_tar_az_off->setCurrentItem(0);


	if((*i)->m_cmd_grb)
	  {
	    if((*i)->m_cmd_grb_mode->currentItem() == 2)
	      (*i)->m_cmd_grb_mode->setCurrentItem(0),
		(*i)->m_cmd_grb_stack->raiseWidget(0);
	    (*i)->m_cmd_grb_onoff->setCurrentItem(0);
	    (*i)->m_cmd_grb_wobble_off->setCurrentItem(0);
	    (*i)->m_cmd_grb_wobble_dir->setCurrentItem(0);
	    (*i)->m_cmd_grb_el_off->setCurrentItem(0);
	    (*i)->m_cmd_grb_az_off->setCurrentItem(0);
	  }
      }
  
  recursing=false;
}

void GUIObjectSelector::cycleWobble()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  MyQComboBox* cb = 0;
  switch(m_cmd_scheme_buttons->selectedId())
    {
    case 1: cb = m_cmd_radec_wobble_dir; break;
    case 2: cb = m_cmd_tar_wobble_dir; break;
    case 4: cb = m_cmd_grb_wobble_dir; break;
    default: return;
    }
  
  unsigned setid = 0;
  switch(cb->currentItem())
    {
    case 0: setid = 1; break;
    case 1: setid = 2; break;
    case 2: setid = 3; break;
    case 3: setid = 0; break;
    }

  cb->setCurrentItem(setid);
  if(m_mirrors)
    for(std::set<GUIObjectSelector*>::iterator i=m_mirrors->begin();
	i!=m_mirrors->end(); i++)
      if(*i != this)
	{
	  switch(m_cmd_scheme_buttons->selectedId())
	    {
	    case 1: cb = (*i)->m_cmd_radec_wobble_dir; break;
	    case 2: cb = (*i)->m_cmd_tar_wobble_dir; break;
	    case 4: cb = (*i)->m_cmd_grb_wobble_dir; break;
	    default: return;
	    }
	  cb->setCurrentItem(setid);      
	}

  sendSetTarget();
}

void GUIObjectSelector::sendGo()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  sendSetTarget();
  emit go(m_identifier);
}

void GUIObjectSelector::sendStop()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  emit stop(m_identifier);
}

void GUIObjectSelector::sendLoadNewTargetList()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  emit loadNewTargetList(m_identifier);
}

void GUIObjectSelector::
setTrackingModeData(GUITrackingModeData& tm,
		    MyQComboBox* mode, MyQComboBox* onoff,
		    MyQComboBox* wobble_dir, MyQComboBox* wobble_off,
		    MyQComboBox* el_off, MyQComboBox* az_off,
		    MyQComboBox* orbit_per, MyQComboBox* orbit_off,
		    MyQComboBox* orbit_dir) const
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  int imode = mode->currentItem();

  tm.onoff_time = 0;
  tm.wobble_coords = SEphem::SphericalCoords();
  tm.elaz_coords = SEphem::SphericalCoords();
  tm.orbit_period = 0;
  tm.orbit_coords = SEphem::SphericalCoords();

  if(imode==0)
    {
      // On/Off
      tm.mode=GUITrackingModeData::TM_ONOFF;
      int ionoff = onoff->currentItem();
      unsigned offset = 0;
      switch(ionoff)
	{
	case 0: 
	  tm.onoff = GUITrackingModeData::OOM_ON; 
	  offset = 0;
	  break;

	case 1: 
	  tm.onoff = GUITrackingModeData::OOM_OFF_AFTER_ON; 
	  offset = 30;
	  break;

	case 2: 
	  tm.onoff = GUITrackingModeData::OOM_OFF_BEFORE_ON; 
	  offset = 30;
	  break;
	  
	case 3: case 4: case 5: case 6: case 7: case 8: case 9:
	case 10: case 11: case 12: case 13: case 14: 
	  tm.onoff = GUITrackingModeData::OOM_OFF_AFTER_ON; 
	  offset = (ionoff-3+1)*5;
	  break;
	  
	case 15: case 16: case 17: case 18: case 19: case 20: case 21:
	case 22: case 23: case 24: case 25: case 26: 
	  tm.onoff = GUITrackingModeData::OOM_OFF_BEFORE_ON;
	  offset = (ionoff-15+1)*5;
	  break;
	}

      tm.onoff_time = 
	SEphem::Angle::makeHrs(SID_RATE*double(offset)/60.0);
    }
  else if(imode==1)
    {
      // Wobble
      tm.mode=GUITrackingModeData::TM_WOBBLE;

      int idir = wobble_dir->currentItem();
      double dir = 0;
      switch(idir)
	{
	case 0: dir=0; break;
	case 1: dir=180; break;
	case 2: dir=90; break;
	case 3: dir=270; break;

	case 4: dir=45; break;
	case 5: dir=135; break;
	case 6: dir=225; break;
	case 7: dir=315; break;

	case 8: case 9: case 10: case 11: case 12:
	case 13: case 14: case 15: case 16: case 17:
	case 18: case 19: case 20: case 21: case 22:
	case 23: case 24: case 25: case 26: case 27:
	case 28: case 29: case 30: case 31:
	  dir = double(idir-8)*15.0;
	  break;
	}
      
      int ioff = wobble_off->currentItem();
      double off = 0;
      switch(ioff)
	{
	case 0: off = DEFAULT_WOBBLE; break;

	case 1: case 2: case 3: case 4: case 5:
	case 6: case 7: case 8: case 9: case 10:
	case 11: case 12: case 13: case 14: case 15:
	case 16: case 17: case 18: case 19: case 20:
	case 21: case 22: case 23: case 24: case 25:
	case 26: case 27: case 28: case 29: case 30:
	case 31: case 32: case 33: case 34: case 35:
	case 36: case 37: case 38: case 39: case 40:
	case 41: 
	  off = double(ioff-1)*0.05;
	  break;

	case 42: off = DISTANT_WOBBLE; break;
	}

      tm.wobble_coords = 
	SEphem::SphericalCoords::makeDeg(off,180-dir);
    }
  else if(imode==2)
    {
      // Orbit
      tm.mode=GUITrackingModeData::TM_ORBIT;

      int iper = orbit_per->currentItem();
      double per = 0;
      if(iper == 0)
	per = DEFAULT_ORBIT_PERIOD_MINUTES;
      else if(iper <= 20)
	per = iper*5;
      else
	per = -(iper-20)*5;

      int idir = orbit_dir->currentItem();
      double dir = 0;
      bool ofree = false;
      switch(idir)
	{
	case 0: ofree=true; dir=0; break;

	case 1: dir=0; break;

	case 2: dir=180; break;
	case 3: dir=90; break;
	case 4: dir=270; break;

	case 5: dir=45; break;
	case 6: dir=135; break;
	case 7: dir=225; break;
	case 8: dir=315; break;

	case 9: case 10: case 11: case 12:
	case 13: case 14: case 15: case 16: case 17:
	case 18: case 19: case 20: case 21: case 22:
	case 23: case 24: case 25: case 26: case 27:
	case 28: case 29: case 30: case 31: case 32:
	  dir = double(idir-9)*15.0;
	  break;
	}
      
      int ioff = orbit_off->currentItem();
      double off = 0;
      switch(ioff)
	{
	case 0: off = DEFAULT_WOBBLE; break;

	case 1: case 2: case 3: case 4: case 5:
	case 6: case 7: case 8: case 9: case 10:
	case 11: case 12: case 13: case 14: case 15:
	case 16: case 17: case 18: case 19: case 20:
	case 21: case 22: case 23: case 24: case 25:
	case 26: case 27: case 28: case 29: case 30:
	case 31: case 32: case 33: case 34: case 35:
	case 36: case 37: case 38: case 39: case 40:
	case 41: 
	  off = double(ioff-1)*0.05;
	  break;

	case 42: off = DISTANT_WOBBLE; break;
	}
      
      tm.orbit_free = ofree;
      tm.orbit_period = per;
      tm.orbit_coords = 
	SEphem::SphericalCoords::makeDeg(off,180-dir);
    }
  else if(imode==3)
    {
      // ElAz_Offset
      tm.mode=GUITrackingModeData::TM_ELAZ;

      int ioff_el = el_off->currentItem();
      double elOff = 0.0;
      switch(ioff_el)
	{
	case 0: elOff = 0.0; break;

	case 1: elOff = -DISTANT_WOBBLE; break;

	case 2: case 3: case 4: case 5:
	case 6: case 7: case 8: case 9: case 10:
	case 11: case 12: case 13: case 14: case 15:
	case 16: case 17: case 18: case 19: case 20:
	case 21: case 22: case 23: case 24: case 25:
	case 26: case 27: case 28: case 29: case 30:
	case 31: case 32: case 33: case 34: case 35:
	case 36: case 37: case 38: case 39: case 41:
	case 42:
	  elOff = double(ioff_el-22)*0.1;
	  break;
	  
	case 43: elOff = DISTANT_WOBBLE; 
	}
      
      int ioff_az = az_off->currentItem();
      double azOff = 0.0;
      switch(ioff_az)
	{
	case 0: azOff = 0.0; break;

	case 1: azOff = -DISTANT_WOBBLE; break;

	case 2: case 3: case 4: case 5:
	case 6: case 7: case 8: case 9: case 10:
	case 11: case 12: case 13: case 14: case 15:
	case 16: case 17: case 18: case 19: case 20:
	case 21: case 22: case 23: case 24: case 25:
	case 26: case 27: case 28: case 29: case 30:
	case 31: case 32: case 33: case 34: case 35:
	case 36: case 37: case 38: case 39: case 41:
	case 42:
	  azOff = double(ioff_az-22)*0.1;
	  break;

	case 43: azOff = DISTANT_WOBBLE; 
	}

      //std::cout << " offsets " << azOff << "  " << elOff << endl;
      tm.elaz_coords.setDeg(elOff+10.0,azOff+10.0);  
      // Add 10 to keep deltaEl from going negative and causing az to rotate by 180.
      // sperical coords aren't really the right container for differential angles.


    }
  else if(imode==4)
    {
      // Pointing star
      tm.mode=GUITrackingModeData::TM_POINT;
    }
  else
    {
      assert(0);
    }
}

void GUIObjectSelector::sendSetTarget()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  emit setTarget(m_identifier);
  return;
}

void GUIObjectSelector::selectTarget(int target)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  m_cmd_target_but->setChecked(true);  

  m_cmd_stack->raiseWidget(2);
  m_cmd_target->setCurrentItem(target);
  setModeToOn();
} 

void GUIObjectSelector::selectGRB(int grb)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(m_cmd_grb_but)
    {
      m_cmd_grb_but->setChecked(true);
      m_cmd_stack->raiseWidget(4);
      m_cmd_grb->setCurrentItem(grb);
      //m_cmd_grb_mode->setCurrentItem(0);

      // Default is now WOBBLE
      m_cmd_grb_mode->setCurrentItem(1);
      m_cmd_grb_stack->raiseWidget(1);
      m_cmd_grb_onoff->setCurrentItem(0);
      m_cmd_grb_wobble_off->setCurrentItem(0);
      m_cmd_grb_wobble_dir->setCurrentItem(0);
      m_cmd_grb_el_off->setCurrentItem(0);
      m_cmd_grb_az_off->setCurrentItem(0);

      if(m_mirrors)
	for(std::set<GUIObjectSelector*>::iterator i=m_mirrors->begin();
	    i!=m_mirrors->end(); i++)
	  (*i)->m_cmd_grb_mode->setCurrentItem(0);
      setModeToOn();
    }
}

void GUIObjectSelector::
makeTrackingModeWidgets(QWidget* parent, const QString& myname,
			const QFont& f, bool no_pointing,
			MyQComboBox*& mode, QWidgetStack*& stack,
			MyQComboBox*& onoff, 
			MyQComboBox*& wobble_dir,MyQComboBox*& wobble_off,
			MyQComboBox*& el_off,MyQComboBox*& az_off,
			MyQComboBox*& orbit_per,
			MyQComboBox*& orbit_off,
			MyQComboBox*& orbit_dir)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  // MAKE THE MAIN MODE SELECTION COMBO

  mode = new MyQComboBox(false,parent,myname+" mode");
  mode->setFont(f);
  mode->insertItem("On/Off",0);
  mode->insertItem("Wobble",1);
  mode->insertItem("Orbit",2);
  mode->insertItem("Up/Down/Sideways (El/Az)",3);
  if(!no_pointing)mode->insertItem("Pointing Star",4);

  
  // MAKE THE WIDGET STACK WHICH HOLDS THE MODE OPTIONS

  stack = new QWidgetStack(parent,myname+" stack");


  // MAKE THE ON OFF FRAME

  QFrame* onoff_frame = new QFrame(stack,myname+" onoff frame");
  QGridLayout* onoff_layout = 
    new QGridLayout(onoff_frame,2,1,0,5,myname+" onoff layout");
  
  onoff = new MyQComboBox(false,onoff_frame,myname+" onoff combo");
  onoff->setFont(f);
  
  onoff->insertItem("On Source",0);
  onoff->insertItem("Off (30 min after On)",1);
  onoff->insertItem("Off (30 min before On)",2);

  onoff->insertItem("Off (5 min after On)",3);
  onoff->insertItem("Off (10 min after On)",4);
  onoff->insertItem("Off (15 min after On)",5);
  onoff->insertItem("Off (20 min after On)",6);
  onoff->insertItem("Off (25 min after On)",7);
  onoff->insertItem("Off (30 min after On)",8);
  onoff->insertItem("Off (35 min after On)",9);
  onoff->insertItem("Off (40 min after On)",10);
  onoff->insertItem("Off (45 min after On)",11);
  onoff->insertItem("Off (50 min after On)",12);
  onoff->insertItem("Off (55 min after On)",13);
  onoff->insertItem("Off (60 min after On)",14);

  onoff->insertItem("Off (5 min before On)",15);
  onoff->insertItem("Off (10 min before On)",16);
  onoff->insertItem("Off (15 min before On)",17);
  onoff->insertItem("Off (20 min before On)",18);
  onoff->insertItem("Off (25 min before On)",19);
  onoff->insertItem("Off (30 min before On)",20);
  onoff->insertItem("Off (35 min before On)",21);
  onoff->insertItem("Off (40 min before On)",22);
  onoff->insertItem("Off (45 min before On)",23);
  onoff->insertItem("Off (50 min before On)",24);
  onoff->insertItem("Off (55 min before On)",25);
  onoff->insertItem("Off (60 min before On)",26);

  onoff_layout->addWidget(onoff,0,0);
  onoff_layout->setRowStretch(0,0);
  onoff_layout->setRowStretch(1,1);


  // MAKE THE WOBBLE FRAME`

  QFrame* wobble_frame = new QFrame(stack,myname+" wobble frame");
  QGridLayout* wobble_layout = 
    new QGridLayout(wobble_frame,2,2,0,5,myname+" wobble layout");
  
  wobble_dir = new MyQComboBox(false,wobble_frame,myname+" wobble dir combo");
  wobble_dir->setFont(f);

  wobble_dir->insertItem("North ("+MAKEDEG("0")+")",0);
  wobble_dir->insertItem("South ("+MAKEDEG("180")+")",1);
  wobble_dir->insertItem("East ("+MAKEDEG("90")+")",2);
  wobble_dir->insertItem("West ("+MAKEDEG("270")+")",3);

  wobble_dir->insertItem("NE ("+MAKEDEG("45")+")",4);
  wobble_dir->insertItem("SE ("+MAKEDEG("135")+")",5);
  wobble_dir->insertItem("SW ("+MAKEDEG("225")+")",6);
  wobble_dir->insertItem("NW ("+MAKEDEG("315")+")",7);

  wobble_dir->insertItem("Direction "+MAKEDEG("0"),8);
  wobble_dir->insertItem("Direction "+MAKEDEG("15"),9);
  wobble_dir->insertItem("Direction "+MAKEDEG("30"),10);
  wobble_dir->insertItem("Direction "+MAKEDEG("45"),11);
  wobble_dir->insertItem("Direction "+MAKEDEG("60"),12);
  wobble_dir->insertItem("Direction "+MAKEDEG("75"),13);
  wobble_dir->insertItem("Direction "+MAKEDEG("90"),14);
  wobble_dir->insertItem("Direction "+MAKEDEG("105"),15);
  wobble_dir->insertItem("Direction "+MAKEDEG("120"),16);
  wobble_dir->insertItem("Direction "+MAKEDEG("135"),17);
  wobble_dir->insertItem("Direction "+MAKEDEG("150"),18);
  wobble_dir->insertItem("Direction "+MAKEDEG("165"),19);
  wobble_dir->insertItem("Direction "+MAKEDEG("180"),20);
  wobble_dir->insertItem("Direction "+MAKEDEG("195"),21);
  wobble_dir->insertItem("Direction "+MAKEDEG("210"),22);
  wobble_dir->insertItem("Direction "+MAKEDEG("225"),23);
  wobble_dir->insertItem("Direction "+MAKEDEG("240"),24);
  wobble_dir->insertItem("Direction "+MAKEDEG("255"),25);
  wobble_dir->insertItem("Direction "+MAKEDEG("270"),26);
  wobble_dir->insertItem("Direction "+MAKEDEG("285"),27);
  wobble_dir->insertItem("Direction "+MAKEDEG("300"),28);
  wobble_dir->insertItem("Direction "+MAKEDEG("315"),29);
  wobble_dir->insertItem("Direction "+MAKEDEG("330"),30);
  wobble_dir->insertItem("Direction "+MAKEDEG("345"),31);

  wobble_off = new MyQComboBox(false,wobble_frame,myname+" wobble off combo");
  wobble_off->setFont(f);

  wobble_off->insertItem("Offset "+MAKEDEG(QString::number(DEFAULT_WOBBLE)),0);

  wobble_off->insertItem("On Source ("+MAKEDEG("0.00")+")",1);
  wobble_off->insertItem("Offset "+MAKEDEG("0.05"),2);
  wobble_off->insertItem("Offset "+MAKEDEG("0.10"),3);
  wobble_off->insertItem("Offset "+MAKEDEG("0.15"),4);
  wobble_off->insertItem("Offset "+MAKEDEG("0.20"),5);
  wobble_off->insertItem("Offset "+MAKEDEG("0.25"),6);
  wobble_off->insertItem("Offset "+MAKEDEG("0.30"),7);
  wobble_off->insertItem("Offset "+MAKEDEG("0.35"),8);
  wobble_off->insertItem("Offset "+MAKEDEG("0.40"),9);
  wobble_off->insertItem("Offset "+MAKEDEG("0.45"),10);
  wobble_off->insertItem("Offset "+MAKEDEG("0.50"),11);
  wobble_off->insertItem("Offset "+MAKEDEG("0.55"),12);
  wobble_off->insertItem("Offset "+MAKEDEG("0.60"),13);
  wobble_off->insertItem("Offset "+MAKEDEG("0.65"),14);
  wobble_off->insertItem("Offset "+MAKEDEG("0.70"),15);
  wobble_off->insertItem("Offset "+MAKEDEG("0.75"),16);
  wobble_off->insertItem("Offset "+MAKEDEG("0.80"),17);
  wobble_off->insertItem("Offset "+MAKEDEG("0.85"),18);
  wobble_off->insertItem("Offset "+MAKEDEG("0.90"),19);
  wobble_off->insertItem("Offset "+MAKEDEG("0.95"),20);
  wobble_off->insertItem("Offset "+MAKEDEG("1.00"),21);
  wobble_off->insertItem("Offset "+MAKEDEG("1.05"),22);
  wobble_off->insertItem("Offset "+MAKEDEG("1.10"),23);
  wobble_off->insertItem("Offset "+MAKEDEG("1.15"),24);
  wobble_off->insertItem("Offset "+MAKEDEG("1.20"),25);
  wobble_off->insertItem("Offset "+MAKEDEG("1.25"),26);
  wobble_off->insertItem("Offset "+MAKEDEG("1.30"),27);
  wobble_off->insertItem("Offset "+MAKEDEG("1.35"),28);
  wobble_off->insertItem("Offset "+MAKEDEG("1.40"),29);
  wobble_off->insertItem("Offset "+MAKEDEG("1.45"),30);
  wobble_off->insertItem("Offset "+MAKEDEG("1.50"),31);
  wobble_off->insertItem("Offset "+MAKEDEG("1.55"),32);
  wobble_off->insertItem("Offset "+MAKEDEG("1.60"),33);
  wobble_off->insertItem("Offset "+MAKEDEG("1.65"),34);
  wobble_off->insertItem("Offset "+MAKEDEG("1.70"),35);
  wobble_off->insertItem("Offset "+MAKEDEG("1.75"),36);
  wobble_off->insertItem("Offset "+MAKEDEG("1.80"),37);
  wobble_off->insertItem("Offset "+MAKEDEG("1.85"),38);
  wobble_off->insertItem("Offset "+MAKEDEG("1.90"),39);
  wobble_off->insertItem("Offset "+MAKEDEG("1.95"),40);
  wobble_off->insertItem("Offset "+MAKEDEG("2.00"),41);

  wobble_off->insertItem("Offset "+MAKEDEG(QString::number(DISTANT_WOBBLE)),42);

  QPushButton* wobble_cycle = 
    new QPushButton("NSEW", wobble_frame,myname+" wobble cycle");
  connect(wobble_cycle,SIGNAL(clicked()),this,SLOT(cycleWobble()));

  wobble_layout->addWidget(wobble_dir,0,0);
  //wobble_layout->addWidget(wobble_off,1,0);
  //wobble_layout->addMultiCellWidget(wobble_cycle,0,1,1,1);
  wobble_layout->addWidget(wobble_cycle,0,1);
  wobble_layout->addMultiCellWidget(wobble_off,1,1,0,1);
  wobble_layout->setColStretch(0,1);

  // MAKE THE EL/AZ OFFSET FRAME`

  QFrame* elaz_frame = new QFrame(stack,myname+" elaz frame");
  QGridLayout* elaz_layout = 
    new QGridLayout(elaz_frame,2,1,0,5,myname+" elaz offset layout");
  
  el_off = new MyQComboBox(false,elaz_frame,myname+" elevation offset combo");
  el_off->setFont(f);


  el_off->insertItem("Up/Down (Elevation) On Source ("+MAKEDEG("0.0")+")",0);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG(QString::number(-DISTANT_WOBBLE)),1);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("-2.0"),2);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("-1.9"),3);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("-1.8"),4);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("-1.7"),5);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("-1.6"),6);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("-1.5"),7);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("-1.4"),8);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("-1.3"),9);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("-1.2"),10);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("-1.1"),11);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("-1.0"),12);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("-0.9"),13);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("-0.8"),14);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("-0.7"),15);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("-0.6"),16);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("-0.5"),17);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("-0.4"),18);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("-0.3"),19);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("-0.2"),20);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("-0.1"),21);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("0.0"),22);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("0.1"),23);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("0.2"),24);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("0.3"),25);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("0.4"),26);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("0.5"),27);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("0.6"),28);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("0.7"),29);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("0.8"),30);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("0.9"),31);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("1.0"),32);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("1.1"),33);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("1.2"),34);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("1.3"),35);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("1.4"),36);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("1.5"),37);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("1.6"),38);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("1.7"),39);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("1.8"),40);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("1.9"),41);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG("2.0"),42);
  el_off->insertItem("Up/Down (El) Offset "+MAKEDEG(QString::number(DISTANT_WOBBLE)),43);

  az_off = new MyQComboBox(false,elaz_frame,myname+" azimuth off combo");
  az_off->setFont(f);


  az_off->insertItem("Sideways (Azimuth) On Source ("+MAKEDEG("0.0")+")",0);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG(QString::number(-DISTANT_WOBBLE)),1);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("-2.0"),2);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("-1.9"),3);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("-1.8"),4);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("-1.7"),5);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("-1.6"),6);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("-1.5"),7);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("-1.4"),8);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("-1.3"),9);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("-1.2"),10);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("-1.1"),11);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("-1.0"),12);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("-0.9"),13);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("-0.8"),14);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("-0.7"),15);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("-0.6"),16);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("-0.5"),17);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("-0.4"),18);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("-0.3"),19);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("-0.2"),20);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("-0.1"),21);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("0.0"),22);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("0.1"),23);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("0.2"),24);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("0.3"),25);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("0.4"),26);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("0.5"),27);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("0.6"),28);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("0.7"),29);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("0.8"),30);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("0.9"),31);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("1.0"),32);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("1.1"),33);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("1.2"),34);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("1.3"),35);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("1.4"),36);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("1.5"),37);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("1.6"),38);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("1.7"),39);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("1.8"),40);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("1.9"),41);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG("2.0"),42);
  az_off->insertItem("Sideways (Az) Offset "+MAKEDEG(QString::number(DISTANT_WOBBLE)),43);


  elaz_layout->addWidget(el_off,0,0);
  elaz_layout->addWidget(az_off,1,0);
  //elaz_layout->addMultiCellWidget(az_off,1,1,0,1);
  //elaz_layout->setColStretch(0,0);
  //elaz_layout->setColStretch(1,0);

  // MAKE THE POINTING STAR FRAME

  QFrame* pointing_frame = new QFrame(stack,myname+" pointing frame");
  

  // MAKE THE ORBIT FRAME

  QFrame* orbit_frame = new QFrame(stack,myname+" orbit frame");
  QGridLayout* orbit_layout = 
    new QGridLayout(orbit_frame,2,2,0,5,myname+" orbit layout");

  orbit_per = new MyQComboBox(false,orbit_frame,myname+" orbit per combo");
  orbit_per->setFont(f);

  orbit_per->insertItem(QString::number(DEFAULT_ORBIT_PERIOD_MINUTES)
			+" minutes (CW)",0);
  orbit_per->insertItem("5 minutes (CW)",1);
  orbit_per->insertItem("10 minutes (CW)",2);
  orbit_per->insertItem("15 minutes (CW)",3);
  orbit_per->insertItem("20 minutes (CW)",4);
  orbit_per->insertItem("25 minutes (CW)",5);
  orbit_per->insertItem("30 minutes (CW)",6);
  orbit_per->insertItem("35 minutes (CW)",7);
  orbit_per->insertItem("40 minutes (CW)",8);
  orbit_per->insertItem("45 minutes (CW)",9);
  orbit_per->insertItem("50 minutes (CW)",10);
  orbit_per->insertItem("55 minutes (CW)",11);
  orbit_per->insertItem("60 minutes (CW)",12);
  orbit_per->insertItem("65 minutes (CW)",13);
  orbit_per->insertItem("70 minutes (CW)",14);
  orbit_per->insertItem("75 minutes (CW)",15);
  orbit_per->insertItem("80 minutes (CW)",16);
  orbit_per->insertItem("85 minutes (CW)",17);
  orbit_per->insertItem("90 minutes (CW)",18);
  orbit_per->insertItem("95 minutes (CW)",19);
  orbit_per->insertItem("100 minutes (CW)",20);
  orbit_per->insertItem("5 minutes (CCW)",21);
  orbit_per->insertItem("10 minutes (CCW)",22);
  orbit_per->insertItem("15 minutes (CCW)",23);
  orbit_per->insertItem("20 minutes (CCW)",24);
  orbit_per->insertItem("25 minutes (CCW)",25);
  orbit_per->insertItem("30 minutes (CCW)",26);
  orbit_per->insertItem("35 minutes (CCW)",27);
  orbit_per->insertItem("40 minutes (CCW)",28);
  orbit_per->insertItem("45 minutes (CCW)",29);
  orbit_per->insertItem("50 minutes (CCW)",30);
  orbit_per->insertItem("55 minutes (CCW)",31);
  orbit_per->insertItem("60 minutes (CCW)",32);
  orbit_per->insertItem("65 minutes (CCW)",33);
  orbit_per->insertItem("70 minutes (CCW)",34);
  orbit_per->insertItem("75 minutes (CCW)",35);
  orbit_per->insertItem("80 minutes (CCW)",36);
  orbit_per->insertItem("85 minutes (CCW)",37);
  orbit_per->insertItem("90 minutes (CCW)",38);
  orbit_per->insertItem("95 minutes (CCW)",39);
  orbit_per->insertItem("100 minutes (CCW)",40);

  orbit_dir = new MyQComboBox(false,orbit_frame,myname+" orbit dir combo");
  orbit_dir->setFont(f);

  orbit_dir->insertItem("Free",0);

  orbit_dir->insertItem("North ("+MAKEDEG("0")+")",1);
  orbit_dir->insertItem("South ("+MAKEDEG("180")+")",2);
  orbit_dir->insertItem("East ("+MAKEDEG("90")+")",3);
  orbit_dir->insertItem("West ("+MAKEDEG("270")+")",4);

  orbit_dir->insertItem("NE ("+MAKEDEG("45")+")",5);
  orbit_dir->insertItem("SE ("+MAKEDEG("135")+")",6);
  orbit_dir->insertItem("SW ("+MAKEDEG("225")+")",7);
  orbit_dir->insertItem("NW ("+MAKEDEG("315")+")",8);

  orbit_dir->insertItem("Direction "+MAKEDEG("0"),9);
  orbit_dir->insertItem("Direction "+MAKEDEG("15"),10);
  orbit_dir->insertItem("Direction "+MAKEDEG("30"),11);
  orbit_dir->insertItem("Direction "+MAKEDEG("45"),12);
  orbit_dir->insertItem("Direction "+MAKEDEG("60"),13);
  orbit_dir->insertItem("Direction "+MAKEDEG("75"),14);
  orbit_dir->insertItem("Direction "+MAKEDEG("90"),15);
  orbit_dir->insertItem("Direction "+MAKEDEG("105"),16);
  orbit_dir->insertItem("Direction "+MAKEDEG("120"),17);
  orbit_dir->insertItem("Direction "+MAKEDEG("135"),18);
  orbit_dir->insertItem("Direction "+MAKEDEG("150"),19);
  orbit_dir->insertItem("Direction "+MAKEDEG("165"),20);
  orbit_dir->insertItem("Direction "+MAKEDEG("180"),21);
  orbit_dir->insertItem("Direction "+MAKEDEG("195"),22);
  orbit_dir->insertItem("Direction "+MAKEDEG("210"),23);
  orbit_dir->insertItem("Direction "+MAKEDEG("225"),24);
  orbit_dir->insertItem("Direction "+MAKEDEG("240"),25);
  orbit_dir->insertItem("Direction "+MAKEDEG("255"),26);
  orbit_dir->insertItem("Direction "+MAKEDEG("270"),27);
  orbit_dir->insertItem("Direction "+MAKEDEG("285"),28);
  orbit_dir->insertItem("Direction "+MAKEDEG("300"),29);
  orbit_dir->insertItem("Direction "+MAKEDEG("315"),30);
  orbit_dir->insertItem("Direction "+MAKEDEG("330"),31);
  orbit_dir->insertItem("Direction "+MAKEDEG("345"),32);

  orbit_off = new MyQComboBox(false,orbit_frame,myname+" orbit off combo");
  orbit_off->setFont(f);

  orbit_off->insertItem("Offset "
			+MAKEDEG(QString::number(DEFAULT_WOBBLE)),0);

  orbit_off->insertItem("On Source ("+MAKEDEG("0.00")+")",1);
  orbit_off->insertItem("Offset "+MAKEDEG("0.05"),2);
  orbit_off->insertItem("Offset "+MAKEDEG("0.10"),3);
  orbit_off->insertItem("Offset "+MAKEDEG("0.15"),4);
  orbit_off->insertItem("Offset "+MAKEDEG("0.20"),5);
  orbit_off->insertItem("Offset "+MAKEDEG("0.25"),6);
  orbit_off->insertItem("Offset "+MAKEDEG("0.30"),7);
  orbit_off->insertItem("Offset "+MAKEDEG("0.35"),8);
  orbit_off->insertItem("Offset "+MAKEDEG("0.40"),9);
  orbit_off->insertItem("Offset "+MAKEDEG("0.45"),10);
  orbit_off->insertItem("Offset "+MAKEDEG("0.50"),11);
  orbit_off->insertItem("Offset "+MAKEDEG("0.55"),12);
  orbit_off->insertItem("Offset "+MAKEDEG("0.60"),13);
  orbit_off->insertItem("Offset "+MAKEDEG("0.65"),14);
  orbit_off->insertItem("Offset "+MAKEDEG("0.70"),15);
  orbit_off->insertItem("Offset "+MAKEDEG("0.75"),16);
  orbit_off->insertItem("Offset "+MAKEDEG("0.80"),17);
  orbit_off->insertItem("Offset "+MAKEDEG("0.85"),18);
  orbit_off->insertItem("Offset "+MAKEDEG("0.90"),19);
  orbit_off->insertItem("Offset "+MAKEDEG("0.95"),20);
  orbit_off->insertItem("Offset "+MAKEDEG("1.00"),21);
  orbit_off->insertItem("Offset "+MAKEDEG("1.05"),22);
  orbit_off->insertItem("Offset "+MAKEDEG("1.10"),23);
  orbit_off->insertItem("Offset "+MAKEDEG("1.15"),24);
  orbit_off->insertItem("Offset "+MAKEDEG("1.20"),25);
  orbit_off->insertItem("Offset "+MAKEDEG("1.25"),26);
  orbit_off->insertItem("Offset "+MAKEDEG("1.30"),27);
  orbit_off->insertItem("Offset "+MAKEDEG("1.35"),28);
  orbit_off->insertItem("Offset "+MAKEDEG("1.40"),29);
  orbit_off->insertItem("Offset "+MAKEDEG("1.45"),30);
  orbit_off->insertItem("Offset "+MAKEDEG("1.50"),31);
  orbit_off->insertItem("Offset "+MAKEDEG("1.55"),32);
  orbit_off->insertItem("Offset "+MAKEDEG("1.60"),33);
  orbit_off->insertItem("Offset "+MAKEDEG("1.65"),34);
  orbit_off->insertItem("Offset "+MAKEDEG("1.70"),35);
  orbit_off->insertItem("Offset "+MAKEDEG("1.75"),36);
  orbit_off->insertItem("Offset "+MAKEDEG("1.80"),37);
  orbit_off->insertItem("Offset "+MAKEDEG("1.85"),38);
  orbit_off->insertItem("Offset "+MAKEDEG("1.90"),39);
  orbit_off->insertItem("Offset "+MAKEDEG("1.95"),40);
  orbit_off->insertItem("Offset "+MAKEDEG("2.00"),41);

  orbit_off->insertItem("Offset "
			+MAKEDEG(QString::number(DISTANT_WOBBLE)),42);

  orbit_layout->addMultiCellWidget(orbit_per,0,0,0,1);
  orbit_layout->addWidget(orbit_off,1,0);
  orbit_layout->addWidget(orbit_dir,1,1);
  //      orbit_layout->setColStretch(0,1);


  // CONNECT THE FRAMES TO THE STACK

  stack->addWidget(onoff_frame,0);
  stack->addWidget(wobble_frame,1);
  stack->addWidget(orbit_frame,2);
  stack->addWidget(elaz_frame,3);
  stack->addWidget(pointing_frame,4);


  connect(mode, SIGNAL(activated(int)),
	  stack, SLOT(raiseWidget(int)));
}

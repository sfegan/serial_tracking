//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUI.cpp
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
 * $Date: 2010/10/28 14:48:05 $
 * $Revision: 2.20 $
 * $Tag$
 *
 **/

#include<iostream>
#include<iomanip>
#include<fstream>
#include<sstream>
#include<string>
#include<algorithm>

#include<qobject.h>
#include<qwidget.h>
#include<qmainwindow.h>
#include<qlineedit.h>
#include<qpushbutton.h>
#include<qgroupbox.h>
#include<qlayout.h>
#include<qlabel.h>
#include<qhbox.h>
#include<qvbox.h>
#include<qgrid.h>
#include<qstring.h>
#include<qdatetime.h>
#include<qtimer.h>
#include<qcolor.h>
#include<qpalette.h>
#include<qradiobutton.h>
#include<qbuttongroup.h>
#include<qcombobox.h>
#include<qtabwidget.h>
#include<qtable.h>
#include<qpixmap.h>
#include<qregexp.h>
#include<qvalidator.h>
#include<qfiledialog.h>
#include<qmessagebox.h>
#include<qtooltip.h>
#include<qimage.h>
#include<qapplication.h>
#include<qscrollview.h>
#include<qtextedit.h>
#include<qmenubar.h>
#include<qinputdialog.h>

#include<time.h>
#include<sys/time.h>

#include<Debug.h>
#include<Message.h>
#include<Messenger.h>
#include<QtMessenger.h>
#include<QtDialogMessenger.h>
#include<QtTextEditMessenger.h>
#include<QtNotification.h>
#include<Angle.h>

#include"ScopeAPI.h"
#include"GUI.h"
#include"GUISummaryDetailsPane.h"
#include"GUITCPPane.h"
#include"GUIOscilloscopePane.h"
#include"GUICPSolverPane.h"
#include"GUIAboutPane.h"
#include"GUIPixmaps.h"
#include"Version.h"
#include"GUITargetDialogs.h"

#include"text.h"

#include"pixmaps/veritas.xpm"

#include"pixmaps/go01_pix_data.xpm"
#include"pixmaps/stop_pix_data.xpm"

#include"pixmaps/padlock_pix_data.xpm"

using namespace SEphem;
using namespace VTracking;
using namespace VMessaging;

struct status_entry
{
  QLineEdit** le;
  QString label;
};

static QColor azel_ind_normcolor(128,128,128);

static void makeCommandConnections(QWidget* main, QWidget* cmd)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  main->connect(cmd,SIGNAL(go(unsigned)),main,SLOT(go(unsigned)));
  main->connect(cmd,SIGNAL(stop(unsigned)),main,SLOT(stop(unsigned)));

  main->connect(cmd,SIGNAL(loadNewTargetList(unsigned)),
		main,SLOT(loadTargetList(unsigned)));

  main->connect(cmd,SIGNAL(setTarget(unsigned)),
		main,SLOT(sendSelectedTargetToScope(unsigned)));

  main->connect(main,SIGNAL(syncWithTargetList(const VTracking::TargetList&)),
		cmd,SLOT(syncWithTargetList(const VTracking::TargetList&)));
}

// ----------------------------------------------------------------------------
// GUI WIDGET
// ----------------------------------------------------------------------------

GUIWidget::
GUIWidget(int scope_num, TelescopeController* controller,
	  const SEphem::SphericalCoords& pos, 
	  const StowObjectVector& stow_pos,
	  int protection_level, bool security_override,
	  QWidget* parent, const char* name, WFlags f)
  : QMainWindow(parent,name,f|Qt::WDestructiveClose), m_scope_num(scope_num),
    m_controller(controller), m_earth_position(pos), m_stow_pos(stow_pos), 
    m_target_list(), m_ud_vec(1), 
    m_direction_preference(CorrectionParameters::DP_NONE),
    m_default_control_widget(0),
    m_tabwidget(0), m_cntrl_emgcy_pb(0), m_cntrl_emgcy_x(-1), 
    m_stat_el(0), m_stat_az(0), m_stat_req(0), 
    m_stat_cur(0), m_stat_ind(0), m_stat_ind_dist(0), m_stat_ind_frame(0),
    m_selector_mirrors(new std::set<GUIObjectSelector*>),
    m_pane_summary(0), m_pane_details(0), 
    m_pane_targettable(0), m_pane_messenger(0), 
#ifndef GUI_NO_QWT
    m_pane_oscilloscope(0),
#endif
    m_pane_tcp(0), m_pane_solver(0), m_timer(0), m_qt_messenger(0),
    m_qt_dialog_messenger(0), m_sun(), m_menu_map_fwd()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  QString basename(name);

  QFrame* outerframe = new QFrame(this,"outer frame");
  QGridLayout* outerlayout = new QGridLayout(outerframe,2,1,0,0,"main layout");
  
  m_tabwidget = new GUITabWidget(outerframe,"tab bar");

  // --------------------------------------------------------------------------
  // MESSENGER
  // --------------------------------------------------------------------------

  m_qt_messenger = 
    new QtMessenger(this,"Qt Messenger");
  Messenger::relay()->registerMessenger(m_qt_messenger);

  m_qt_dialog_messenger =
    new QtDialogMessenger(Message::PS_UNUSUAL, this, "Dialog Messenger");
  connect(m_qt_messenger,SIGNAL(message(const Message&)),
	  m_qt_dialog_messenger,SLOT(sendMessage(const Message&)));
  
  // --------------------------------------------------------------------------
  // SUMMARY PANE
  // --------------------------------------------------------------------------

  m_pane_summary = new GUISummaryPane(0, m_stow_pos, m_selector_mirrors,false,
				      m_tabwidget,"summary tab");
  makeCommandConnections(this,m_pane_summary->selector());
  m_default_control_widget = m_pane_summary;

  // --------------------------------------------------------------------------
  // DETAILS PANE
  // --------------------------------------------------------------------------

  m_pane_details = new GUIDetailsPane(1, m_stow_pos, m_selector_mirrors,false,
				      m_tabwidget,"details tab");
  makeCommandConnections(this,m_pane_details->selector());

  // --------------------------------------------------------------------------
  // TARGET TABLE PANE
  // --------------------------------------------------------------------------

  m_pane_targettable=new GUITargetTablePane(m_earth_position,
					    m_tabwidget,"target table");

  connect(m_pane_targettable,SIGNAL(setTarget(int)),
	  this,SLOT(setFromTargetTable(int)));

  connect(m_pane_targettable,SIGNAL(loadTargetList()),
	  this,SLOT(loadTargetList()));

  connect(this, SIGNAL(syncWithTargetList(const VTracking::TargetList&)),
	  m_pane_targettable, SLOT(syncWithTargetList(const VTracking::TargetList&)));

  // --------------------------------------------------------------------------
  // OSCILLOSCOPE PANE
  // --------------------------------------------------------------------------

#ifndef GUI_NO_QWT
  m_pane_oscilloscope = 
    new GUIOscilloscopePane(400,80,m_controller->iterationPeriod(),m_tabwidget,
			    "oscilloscope");
#endif

  // --------------------------------------------------------------------------
  // MESSANGER PANE
  // --------------------------------------------------------------------------

  m_pane_messenger = new GUIMessengerPane(m_tabwidget,"messenger");
  connect(m_qt_messenger,SIGNAL(message(const Message&)),
	  m_pane_messenger,SLOT(sendMessage(const Message&)));

  // --------------------------------------------------------------------------
  // TRACKING PARAMETERS PANE
  // --------------------------------------------------------------------------

  CorrectionParameters tcp;
  tcp=m_controller->getCorrections();

  m_pane_tcp = new GUITCPPane(tcp,m_scope_num,m_tabwidget, "tcp");
  connect(m_pane_tcp,SIGNAL(parametersChanged()),
	  this,SLOT(setCorrections()));
  connect(m_pane_tcp,SIGNAL(recordPosition(double, double, double, double)),
	  this,SLOT(recordTrackingPosition(double,double,double,double)));
  
  // --------------------------------------------------------------------------
  // CORRECTION PARAMETERS SOLVER
  // --------------------------------------------------------------------------
  
  m_pane_solver = new GUICPSolverPane(m_controller,m_scope_num,
				      m_tabwidget,"cpsolver");
  connect(m_pane_tcp,SIGNAL(recordPosition(double, double, double, double)),
	  m_pane_solver,SLOT(addData(double,double,double,double)));

  // --------------------------------------------------------------------------
  // ABOUT BOX
  // --------------------------------------------------------------------------

  GUIAboutPane* about = new GUIAboutPane(m_tabwidget,"about");

  // --------------------------------------------------------------------------
  // ADD PANES TO MANAGER
  // --------------------------------------------------------------------------

  m_tabwidget->setProtectionLevel(protection_level);
  m_tabwidget->addManagedPane("&Summary",      m_pane_summary,      0, 10, 7);
  m_tabwidget->addManagedPane("&Details",      m_pane_details,      0,  9, 7);
  m_tabwidget->addManagedPane("&Targets",      m_pane_targettable,  0, 10, 7);
#ifndef GUI_NO_QWT
  m_tabwidget->addManagedPane("&Oscilloscope", m_pane_oscilloscope, 0,  5, 5);
#endif
  m_tabwidget->addManagedPane("Messages",      m_pane_messenger,    0, 10, 7);
  m_tabwidget->addManagedPane("Corrections",   m_pane_tcp,          0,  5, 0);
  m_tabwidget->addManagedPane("Measurements",  m_pane_solver,       0,  5, 0);
  m_tabwidget->addManagedPane("About", about, 0, 0xFFFFFFFF, 0xFFFFFFFF);

  // --------------------------------------------------------------------------
  // STATUS BAR
  // --------------------------------------------------------------------------

  QHBox* statbox = new QHBox(outerframe,basename+" status box");
  statbox->setMargin(5);
  statbox->setSpacing(5);

  struct 
  {
    QFrame* box;
    QLineEdit** le;
    QString label;
    QString tooltip;
    int width;
  } status_entries[] = {
    { statbox, &m_stat_az,   "Az",        TT_SB_AZ,                       80 },
    { statbox, &m_stat_el,   "El",        TT_SB_EL,                       80 },
    { statbox, &m_stat_req,  "Request",   TT_SB_REQ,                      80 },
    { statbox, &m_stat_cur,  "State",     TT_SB_STAT,                     120},
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

  m_stat_ind = new MyQLabel(statbox,"stat ind");
  m_stat_ind->setPixmap(*GUIPixmaps::instance()->ind_cf_pixmaps(0));
  m_stat_ind->setFrameShape(QFrame::Box);
  m_ud_vec[0].tse.state=VTracking::TelescopeController::TS_COM_FAILURE;

  m_cntrl_emgcy_pb = new QPushButton("Panic!",statbox,"stat emergncy pb");
  m_cntrl_emgcy_pb->setMinimumWidth(50);
  m_cntrl_emgcy_pb->setMinimumHeight(22);
  m_cntrl_emgcy_pb->setFocusPolicy(NoFocus);

  connect(m_cntrl_emgcy_pb,SIGNAL(clicked()),this,SLOT(emergency()));

  // --------------------------------------------------------------------------
  // MENUS
  // --------------------------------------------------------------------------

  QMenuBar* menubar = menuBar();
  
  // *********************************
  // ********** MOTION MENU **********
  // *********************************

  QPopupMenu* motionmenu = new QPopupMenu(menubar);
  m_menu_map_fwd["mo go"] = 
    motionmenu->insertItem(QPixmap(go01_pix_data),"&Go",this,SLOT(go()));
  m_menu_map_fwd["mo stop"] = 
    motionmenu->insertItem(QPixmap(stop_pix_data),"&Stop",this,SLOT(stop()));

  motionmenu->insertSeparator();

  QPopupMenu* pdmenu = new QPopupMenu(motionmenu);
  m_menu_map_fwd["pref dir none"] = 
    pdmenu->insertItem("Fastest", this,SLOT(menuPrefDir(int)));
  m_menu_map_fwd["pref dir cw"] = 
    pdmenu->insertItem("Clockwise (CW)", this,SLOT(menuPrefDir(int)));
  m_menu_map_fwd["pref dir ccw"] = 
    pdmenu->insertItem("Counter clockwise (CCW)", this,SLOT(menuPrefDir(int)));
  pdmenu->setItemChecked(m_menu_map_fwd["pref dir none"],true);

  m_menu_map_fwd["pref dir menu"] = 
    motionmenu->insertItem("Preferred Direction", pdmenu);

  motionmenu->insertSeparator();
  
  QPopupMenu* cvmenu = new QPopupMenu(motionmenu);
  m_menu_map_fwd["cv up 0.300"] = 
    cvmenu->insertItem(MAKEDPS("Up 0.300"), this,SLOT(menuCVTarget(int)));
  m_menu_map_fwd["cv up 0.030"] = 
    cvmenu->insertItem(MAKEDPS("Up 0.030"), this,SLOT(menuCVTarget(int)));
  m_menu_map_fwd["cv up 0.003"] = 
    cvmenu->insertItem(MAKEDPS("Up 0.003"), this,SLOT(menuCVTarget(int)));
  m_menu_map_fwd["cv dn 0.003"] = 
    cvmenu->insertItem(MAKEDPS("Down 0.003"), this,SLOT(menuCVTarget(int)));
  m_menu_map_fwd["cv dn 0.030"] = 
    cvmenu->insertItem(MAKEDPS("Down 0.030"), this,SLOT(menuCVTarget(int)));
  m_menu_map_fwd["cv dn 0.300"] = 
    cvmenu->insertItem(MAKEDPS("Down 0.300"), this,SLOT(menuCVTarget(int)));

  cvmenu->insertSeparator();
  m_menu_map_fwd["cv cw 0.300"] = 
    cvmenu->insertItem(MAKEDPS("CW 0.300"), this,SLOT(menuCVTarget(int)));
  m_menu_map_fwd["cv cw 0.030"] = 
    cvmenu->insertItem(MAKEDPS("CW 0.030"), this,SLOT(menuCVTarget(int)));
  m_menu_map_fwd["cv cw 0.003"] = 
    cvmenu->insertItem(MAKEDPS("CW 0.003"), this,SLOT(menuCVTarget(int)));
  m_menu_map_fwd["cv cc 0.003"] = 
    cvmenu->insertItem(MAKEDPS("CCW 0.003"), this,SLOT(menuCVTarget(int)));
  m_menu_map_fwd["cv cc 0.030"] = 
    cvmenu->insertItem(MAKEDPS("CCW 0.030"), this,SLOT(menuCVTarget(int)));
  m_menu_map_fwd["cv cc 0.300"] = 
    cvmenu->insertItem(MAKEDPS("CCW 0.300"), this,SLOT(menuCVTarget(int)));

  cvmenu->insertSeparator();
  m_menu_map_fwd["cv custom"] = 
    cvmenu->insertItem("Custom", this,SLOT(menuCVTarget(int)));
  
  m_menu_map_fwd["cv menu"] = 
    motionmenu->insertItem("Constant Speed", cvmenu);

  QPopupMenu* sfmenu = new QPopupMenu(motionmenu);
  m_menu_map_fwd["sf up 1.0"] = 
    sfmenu->insertItem(MAKEDEG("Up 1.0"), this,SLOT(menuSFTarget(int)));
  m_menu_map_fwd["sf up 5.0"] = 
    sfmenu->insertItem(MAKEDEG("Up 5.0"), this,SLOT(menuSFTarget(int)));
  m_menu_map_fwd["sf up 10.0"] = 
    sfmenu->insertItem(MAKEDEG("Up 10.0"), this,SLOT(menuSFTarget(int)));
  m_menu_map_fwd["sf dn 1.0"] = 
    sfmenu->insertItem(MAKEDEG("Down 1.0"), this,SLOT(menuSFTarget(int)));
  m_menu_map_fwd["sf dn 5.0"] = 
    sfmenu->insertItem(MAKEDEG("Down 5.0"), this,SLOT(menuSFTarget(int)));
  m_menu_map_fwd["sf dn 10.0"] = 
    sfmenu->insertItem(MAKEDEG("Down 10.0"), this,SLOT(menuSFTarget(int)));

  sfmenu->insertSeparator();
  m_menu_map_fwd["sf cw 1.0"] = 
    sfmenu->insertItem(MAKEDEG("CW 1.0"), this,SLOT(menuSFTarget(int)));
  m_menu_map_fwd["sf cw 5.0"] = 
    sfmenu->insertItem(MAKEDEG("CW 5.0"), this,SLOT(menuSFTarget(int)));
  m_menu_map_fwd["sf cw 10.0"] = 
    sfmenu->insertItem(MAKEDEG("CW 10.0"), this,SLOT(menuSFTarget(int)));
  m_menu_map_fwd["sf cc 1.0"] = 
    sfmenu->insertItem(MAKEDEG("CCW 1.0"), this,SLOT(menuSFTarget(int)));
  m_menu_map_fwd["sf cc 5.0"] = 
    sfmenu->insertItem(MAKEDEG("CCW 5.0"), this,SLOT(menuSFTarget(int)));
  m_menu_map_fwd["sf cc 10.0"] = 
    sfmenu->insertItem(MAKEDEG("CCW 10.0"), this,SLOT(menuSFTarget(int)));

  sfmenu->insertSeparator();
  m_menu_map_fwd["sf custom"] = 
    sfmenu->insertItem("Custom", this,SLOT(menuSFTarget(int)));
  
  m_menu_map_fwd["sf menu"] = 
    motionmenu->insertItem("Step Function", sfmenu);

  m_menu_map_fwd["mo menu"] = 
    menubar->insertItem("&Motion", motionmenu);

  // ************************************
  // ********** INTERFACE MENU **********
  // ************************************

  QPopupMenu* interfacemenu = new QPopupMenu(menubar);
  interfacemenu->setCheckable(true);

  m_menu_map_fwd["if simple"] = 
    interfacemenu->insertItem("Simple",this,SLOT(menuInterface(int)));
  m_menu_map_fwd["if restricted"] = 
    interfacemenu->insertItem("Restricted", this,SLOT(menuInterface(int)));
  m_menu_map_fwd["if full"] = 
    interfacemenu->insertItem("Full", this,SLOT(menuInterface(int)));

  if(protection_level<1)
    interfacemenu->setItemChecked(m_menu_map_fwd["if full"],true);
  else if(protection_level<7)
    interfacemenu->setItemChecked(m_menu_map_fwd["if restricted"],true);
  else
    interfacemenu->setItemChecked(m_menu_map_fwd["if simple"],true);

  m_menu_map_fwd["if menu"] = 
    menubar->insertItem("Interface", interfacemenu);

  // ***********************************
  // ********** SECURITY MENU **********
  // ***********************************

  QPopupMenu* securitymenu = new QPopupMenu(menubar);
  m_menu_map_fwd["sec override"] = 
    securitymenu->insertItem(QPixmap(padlock_pix_data), "Disable Security",
			     this,SLOT(overrideSecurity()));
  
  m_menu_map_fwd["sec menu"] = 
    menubar->insertItem("Security", securitymenu);
  
  // --------------------------------------------------------------------------
  // PROTECT MENU ITEMS
  // --------------------------------------------------------------------------

  if(security_override)
    {
      menuBar()->setItemEnabled(m_menu_map_fwd["sec override"],false);
    }
  else
    {
      menuBar()->setItemEnabled(m_menu_map_fwd["if full"],false);
      menuBar()->setItemEnabled(m_menu_map_fwd["cv menu"],false);
      menuBar()->setItemEnabled(m_menu_map_fwd["sf menu"],false);
    }

  // --------------------------------------------------------------------------
  // SUN WARNING
  // --------------------------------------------------------------------------

  m_sun = new GUISunWarning(m_earth_position,this,"sun warning");
  QToolTip::add(m_sun,
		"Shows the elevation and angular distance from the optic\n"
		"axis of the sun and/or moon. Flashes red if the telescope\n"
		"is pointing too close to sun. The display disappears when\n"
		"both sun and moon are below threshold.");

  // --------------------------------------------------------------------------
  // FINISH UP
  // --------------------------------------------------------------------------

  outerlayout->addWidget(m_tabwidget,0,0);
  outerlayout->addWidget(statbox,1,0);
  
  setCentralWidget(outerframe /*m_tabwidget*/);
  resize(minimumSize());

  GUITargetDialogs::loadDefault(m_target_list,true);
  emit syncWithTargetList(m_target_list);

  setIcon(QPixmap(const_cast<const char**>(veritas)));

  std::ostringstream caption_stream;
  caption_stream << "VERITAS Telescope Controller (T"
		 << m_scope_num+1 << ' ' 
		 << m_controller->controllerName() << ')';
  setCaption(caption_stream.str().c_str());

  qApp->installEventFilter(this);

  updateStatusBar(m_ud_vec[0]);
  m_tabwidget->update(m_ud_vec);

  m_timer = new QTimer(this,"timer");
  connect(m_timer,SIGNAL(timeout()),this,SLOT(update()));
  m_timer->start(50);

  QWidget* cw = centralWidget();
  QPoint p = cw->rect().topRight();
  p=cw->mapTo(this,p);
  p-=QPoint(m_sun->width(),0);
  m_sun->move(p);
}

GUIWidget::~GUIWidget()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  Messenger::relay()->unRegisterMessenger(m_qt_messenger);
}


void GUIWidget::update()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  GUIUpdateData& ud(m_ud_vec[0]);
  ud.fillData(m_controller, m_earth_position);

  //if(ud.update_number%7 == 0)
  m_sun->update(ud.tse.tel_azel, ud.mjd, ud.lmst);      

  updateStatusBar(ud);
  m_tabwidget->update(m_ud_vec);
}

void GUIWidget::updateStatusBar(const GUIUpdateData& ud)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(ud.full_update)
    {
      if(ud.tse.state != TelescopeController::TS_COM_FAILURE)
	{
	  std::ostringstream az_pos_stream;
	  az_pos_stream << std::showpos << std::fixed << std::setprecision(4) 
			<< ud.tse.status.az.driveangle_deg;
	  m_stat_az->setText(MAKEDEG(az_pos_stream.str()));
	  m_stat_az->setEnabled(true);
	  
	  std::ostringstream el_pos_stream;
	  el_pos_stream << std::showpos << std::fixed << std::setprecision(4) 
			<< ud.tse.status.el.driveangle_deg;
	  m_stat_el->setText(MAKEDEG(el_pos_stream.str()));
	  m_stat_el->setEnabled(true);
	}
      else
	{
	  m_stat_az->setText("");
	  m_stat_az->setEnabled(false);
	  m_stat_el->setText("");
	  m_stat_el->setEnabled(false);
	}
      
      bool restricted = 
	menuBar()->isItemEnabled(m_menu_map_fwd["sec override"]);
				  
      switch(ud.tse.state)
	{
	case TelescopeController::TS_STOP:
	  menuBar()->setItemEnabled(m_menu_map_fwd["mo go"],true);
	  menuBar()->setItemEnabled(m_menu_map_fwd["mo stop"],false);
	  menuBar()->setItemEnabled(m_menu_map_fwd["cv menu"],!restricted);
	  menuBar()->setItemEnabled(m_menu_map_fwd["sf menu"],!restricted);
	  break;
	case TelescopeController::TS_SLEW:
	case TelescopeController::TS_TRACK:
	case TelescopeController::TS_RESTRICTED_MOTION:
	case TelescopeController::TS_RAMP_DOWN:
	  menuBar()->setItemEnabled(m_menu_map_fwd["mo go"],false);
	  menuBar()->setItemEnabled(m_menu_map_fwd["mo stop"],true);
	  menuBar()->setItemEnabled(m_menu_map_fwd["cv menu"],!restricted);
	  menuBar()->setItemEnabled(m_menu_map_fwd["sf menu"],!restricted);
	  break;
	case TelescopeController::TS_COM_FAILURE:
	  menuBar()->setItemEnabled(m_menu_map_fwd["mo go"],false);
	  menuBar()->setItemEnabled(m_menu_map_fwd["mo stop"],false);
	  menuBar()->setItemEnabled(m_menu_map_fwd["cv menu"],false);
	  menuBar()->setItemEnabled(m_menu_map_fwd["sf menu"],false);
	  break;
	}

      switch(ud.tse.state)
	{
	case TelescopeController::TS_STOP:
	  m_stat_cur->setText("STOPPED");
	  break;
	case TelescopeController::TS_SLEW:
	  m_stat_cur->setText("SLEWING");
	  break;
	case TelescopeController::TS_TRACK:
	  m_stat_cur->setText("TRACKING");
	  break;
	case TelescopeController::TS_RESTRICTED_MOTION:
	  m_stat_cur->setText("RESTRICTED");
	  break;
	case TelescopeController::TS_RAMP_DOWN:
	  m_stat_cur->setText("RAMPING DOWN");
	  break;
	case TelescopeController::TS_COM_FAILURE:
	  switch(ud.tse.cf)
	    {
	    case TelescopeController::CF_SERVER:
	      m_stat_cur->setText("NO SERVER");
	      break;
	    case TelescopeController::CF_SCOPE:
	      m_stat_cur->setText("SCOPE COM FAIL");
	      break;
	    }
	  break;
	}
    }  

  switch(ud.tse.req)
    {
    case TelescopeController::REQ_STOP:
      m_stat_req->setText("STOP");
      break;
    case TelescopeController::REQ_SLEW:
      m_stat_req->setText("SLEW");
      break;
    case TelescopeController::REQ_TRACK:
      m_stat_req->setText("TRACK");
      break;
    }
  
  switch(ud.tse.state)
    {
    case TelescopeController::TS_COM_FAILURE:
      if(ud.last_state!=ud.tse.state)m_stat_ind_dist=0, m_stat_ind_frame=0;
      
      m_stat_ind->setPixmap(*GUIPixmaps::instance()->
			    ind_cf_pixmaps(m_stat_ind_frame));
      m_stat_ind_frame++;
      if(m_stat_ind_frame==16)m_stat_ind_frame=0;
      break;
      
    case TelescopeController::TS_STOP:
      if(ud.last_state!=ud.tse.state)
	m_stat_ind->setPixmap(*GUIPixmaps::instance()->ind_stop_pixmaps(0));
      break;
      
    case TelescopeController::TS_SLEW:
    case TelescopeController::TS_TRACK:
    case TelescopeController::TS_RESTRICTED_MOTION:
    case TelescopeController::TS_RAMP_DOWN:
      if((ud.last_state == TelescopeController::TS_COM_FAILURE)||
	 (ud.last_state == TelescopeController::TS_STOP))
	{
	  m_stat_ind_dist=0, m_stat_ind_frame=0;
	  m_stat_ind->setPixmap(*GUIPixmaps::instance()->
				ind_go_pixmaps(m_stat_ind_frame));
	}
      else
	{
	  double azspeed = fabs(ud.tse.az_driveangle_estimated_speed_dps);
	  double elspeed = fabs(ud.tse.el_driveangle_estimated_speed_dps);
	  double speed = (azspeed>elspeed)?azspeed:elspeed;
	  double logspeed = log10(speed)+3;
	  if(logspeed<0)logspeed=0;
	  m_stat_ind_dist += logspeed;
	  int frames = int(round(m_stat_ind_dist));
	  if(frames > 0)
	    {
	      m_stat_ind_dist -= double(frames);
	      m_stat_ind_frame+=frames;
	      m_stat_ind->
		setPixmap(*GUIPixmaps::instance()->
			  ind_go_pixmaps(m_stat_ind_frame));
	    }
	}
    }
  
  int lastx=m_cntrl_emgcy_x;
  int x=int(floor(30*(1+sin(fmod(ud.mjd*86400/2,1)*Angle::sc_twoPi))));
  if(x!=lastx)
    {
      QColor pfcolor=QColor(255-x,x,x);
      QColor pbcolor=QColor(255-60+x,60-x,60-x);
      m_cntrl_emgcy_pb->setPalette(QPalette(pfcolor,pbcolor));
      m_cntrl_emgcy_x=x;
    }
}

void GUIWidget::menuPrefDir(int index)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  menuBar()->setItemChecked(m_menu_map_fwd["pref dir none"],false);
  menuBar()->setItemChecked(m_menu_map_fwd["pref dir cw"],false);
  menuBar()->setItemChecked(m_menu_map_fwd["pref dir ccw"],false);
  if(index == m_menu_map_fwd["pref dir none"])
    m_direction_preference = CorrectionParameters::DP_NONE;
  else if(index == m_menu_map_fwd["pref dir cw"])
    m_direction_preference = CorrectionParameters::DP_CW;
  else if(index == m_menu_map_fwd["pref dir ccw"])
    m_direction_preference = CorrectionParameters::DP_CCW;
  menuBar()->setItemChecked(index, true);
}

void GUIWidget::menuCVTarget(int index)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(index == m_menu_map_fwd["cv up 0.300"])setTargetCV(0.300,0.000);
  else if(index == m_menu_map_fwd["cv up 0.030"])setTargetCV(+0.030, 0.000);
  else if(index == m_menu_map_fwd["cv up 0.003"])setTargetCV(+0.003, 0.000);
  else if(index == m_menu_map_fwd["cv dn 0.003"])setTargetCV(-0.003, 0.000);
  else if(index == m_menu_map_fwd["cv dn 0.030"])setTargetCV(-0.030, 0.000);
  else if(index == m_menu_map_fwd["cv dn 0.300"])setTargetCV(-0.300, 0.000); 
  else if(index == m_menu_map_fwd["cv cw 0.300"])setTargetCV( 0.000,+0.300); 
  else if(index == m_menu_map_fwd["cv cw 0.030"])setTargetCV( 0.000,+0.030); 
  else if(index == m_menu_map_fwd["cv cw 0.003"])setTargetCV( 0.000,+0.003); 
  else if(index == m_menu_map_fwd["cv cc 0.003"])setTargetCV( 0.000,-0.003); 
  else if(index == m_menu_map_fwd["cv cc 0.030"])setTargetCV( 0.000,-0.030); 
  else if(index == m_menu_map_fwd["cv cc 0.300"])setTargetCV( 0.000,-0.300); 
  else if(index == m_menu_map_fwd["cv custom"])
    {
      bool ok;
      QString txt =
	QInputDialog::getText("Constant Speed", 
			      "Enter Az and El speeds separated by a space",
			      QLineEdit::Normal, QString::null, &ok, this);
      if(!ok)return;
      std::istringstream str(txt);
      double az_speed(0);
      double el_speed(0);
      str >> az_speed >> el_speed;
      if(!str)return;
      setTargetCV(el_speed,az_speed);
    }
  go();
}

void GUIWidget::menuSFTarget(int index)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(index == m_menu_map_fwd["sf up 1.0"])setTargetSF(+1.0,0.0);
  else if(index == m_menu_map_fwd["sf up 5.0"])setTargetSF(+5.0, 0.0);
  else if(index == m_menu_map_fwd["sf up 10.0"])setTargetSF(+10.0, 0.0);
  else if(index == m_menu_map_fwd["sf dn 1.0"])setTargetSF(-1.0, 0.0);
  else if(index == m_menu_map_fwd["sf dn 5.0"])setTargetSF(-5.0, 0.0);
  else if(index == m_menu_map_fwd["sf dn 10.0"])setTargetSF(-10.0, 0.0); 
  else if(index == m_menu_map_fwd["sf cw 1.0"])setTargetSF(0.0,+1.0); 
  else if(index == m_menu_map_fwd["sf cw 5.0"])setTargetSF(0.0,+5.0); 
  else if(index == m_menu_map_fwd["sf cw 10.0"])setTargetSF(0.0,+10.0); 
  else if(index == m_menu_map_fwd["sf cc 1.0"])setTargetSF( 0.000,-1.0); 
  else if(index == m_menu_map_fwd["sf cc 5.0"])setTargetSF( 0.000,-5.0); 
  else if(index == m_menu_map_fwd["sf cc 10.0"])setTargetSF( 0.000,-10.0); 
  else if(index == m_menu_map_fwd["sf custom"])
    {
      bool ok;
      QString txt =
	QInputDialog::getText("Step Function", 
			      "Enter Az and El steps separated by a space",
			      QLineEdit::Normal, QString::null, &ok, this);
      if(!ok)return;
      std::istringstream str(txt);
      double az_step(0);
      double el_step(0);
      str >> az_step >> el_step;
      if(!str)return;
      setTargetSF(el_step,az_step);
    }
  go();
}

void GUIWidget::menuInterface(int index)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  menuBar()->setItemChecked(m_menu_map_fwd["if full"],false);
  menuBar()->setItemChecked(m_menu_map_fwd["if restricted"],false);
  menuBar()->setItemChecked(m_menu_map_fwd["if simple"],false);

  if(index == m_menu_map_fwd["if simple"])
    m_tabwidget->setProtectionLevel(7);
  else if(index == m_menu_map_fwd["if restricted"])
    m_tabwidget->setProtectionLevel(1);
  else if(index == m_menu_map_fwd["if full"])
    m_tabwidget->setProtectionLevel(0);

  if(m_tabwidget->protectionLevel()<1)
    menuBar()->setItemChecked(m_menu_map_fwd["if full"],true);
  else if(m_tabwidget->protectionLevel()<7)
    menuBar()->setItemChecked(m_menu_map_fwd["if restricted"],true);
  else
    menuBar()->setItemChecked(m_menu_map_fwd["if simple"],true);
}

void GUIWidget::overrideSecurity()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  bool ok;
  QString pass = 
    QInputDialog::getText("Disable Security", 
			  QString("Enter password (")+gui_security_pass+')',
			  QLineEdit::Password, QString::null, &ok, this);
  if((ok)&&(pass==gui_security_pass))
    {
      menuBar()->setItemEnabled(m_menu_map_fwd["sec override"],false);
      menuBar()->setItemEnabled(m_menu_map_fwd["if full"],true);
      menuBar()->setItemEnabled(m_menu_map_fwd["cv menu"],true);
      menuBar()->setItemEnabled(m_menu_map_fwd["sf menu"],true);
      QMessageBox::warning(this,"Security","Security Disabled");
    }
  else if(ok)
    QMessageBox::warning(this,"Security","Incorrect Password");
}

void GUIWidget::closeEvent (QCloseEvent * e)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
#if 0
  VTaskNotification::QtNotificationList::getInstance()->
    synchronouslyTerminateNotificationProcessing();
#endif
  e->accept();
}

bool GUIWidget::eventFilter(QObject* o, QEvent* e)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if((e->type() == QEvent::KeyPress)||
     (e->type() == QEvent::MouseButtonPress)||
     (e->type() == QEvent::MouseButtonRelease)||
     (e->type() == QEvent::MouseButtonDblClick))
    {
      QObject* oo = o;
      while(oo)
	{
	  if(oo==m_pane_summary)m_default_control_widget=m_pane_summary;
	  else if(oo==m_pane_details)m_default_control_widget=m_pane_details;
	  oo=oo->parent();
	}
    } 
  return false;
}

void GUIWidget::resizeEvent(QResizeEvent* e)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  QMainWindow::resizeEvent(e);
  QWidget* cw = centralWidget();
  QPoint p = cw->rect().topRight();
  p=cw->mapTo(this,p);
  p-=QPoint(m_sun->width(),0);
  m_sun->move(p);
}

// --------------------------------------------------------------------------------------
//
// TARGET AND MOTION
//
// --------------------------------------------------------------------------------------

void GUIWidget::go(unsigned id)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  go();
}

void GUIWidget::stop(unsigned id)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  stop();
}

void GUIWidget::sendSelectedTargetToScope(unsigned selector_id)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  TargetObject* obj = 0;
  double az_deg = m_ud_vec[0].tse.status.az.driveangle_deg;
  if(selector_id == 0)
    obj=m_pane_summary->selector()->getObject(az_deg,m_ud_vec[0].mjd);
  else obj=m_pane_details->selector()->getObject(az_deg,m_ud_vec[0].mjd);
  m_controller->setTargetObject(obj,m_direction_preference);
}

void GUIWidget::loadTargetList(unsigned id)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  loadTargetList();
}

void GUIWidget::go()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  //  m_last_clicked_page=m_tabwidget->currentPageIndex();
  const TargetObject* obj = m_controller->getTargetObject();
  if(obj == 0)return;
  else if(obj->objectMovesInAzEl())m_controller->reqTrack();
  else m_controller->reqSlew();
  delete obj;
}

void GUIWidget::stop()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  //  m_last_clicked_page=m_tabwidget->currentPageIndex();
  m_controller->reqStop();
}

void GUIWidget::loadTargetList()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  if(GUITargetDialogs::loadFromDBCollection(m_target_list, false, this, 
					    "load targets dialogs"))
    emit syncWithTargetList(m_target_list);
}

void GUIWidget::setFromTargetTable(int target)
{
  m_pane_summary->selector()->selectTarget(target);
  m_pane_details->selector()->selectTarget(target);
  sendSelectedTargetToScope(0);
  m_tabwidget->setCurrentPage(m_default_control_widget);
}

void GUIWidget::setTargetCV(double elspeed, double azspeed)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  GUIUpdateData& ud(m_ud_vec[0]);
  if(ud.tse.state == VTracking::TelescopeController::TS_COM_FAILURE)return;

  SphericalCoords ic = 
    SphericalCoords::makeLatLongDeg(ud.tse.status.el.driveangle_deg,
				    ud.tse.status.az.driveangle_deg);
  TargetObject* obj = new CVObject(ic,azspeed,elspeed,ud.tse.mjd);
  m_controller->setTargetObject(obj);
}

void GUIWidget::setTargetSF(double elstep, double azstep)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  GUIUpdateData& ud(m_ud_vec[0]);
  if(ud.tse.state == VTracking::TelescopeController::TS_COM_FAILURE)return;

  SphericalCoords ic = 
    SphericalCoords::makeLatLongDeg(ud.tse.status.el.driveangle_deg+elstep,
				    ud.tse.status.az.driveangle_deg+azstep);
  TargetObject* obj = new AzElObject(ic, false, false);
  m_controller->setTargetObject(obj);
}

void GUIWidget::emergency()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  m_controller->emergencyStop();
}

void GUIWidget::setCorrections()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  CorrectionParameters tcp = m_pane_tcp->getParameters();
  m_controller->setCorrectionParameters(tcp);
}
 
void GUIWidget::
recordTrackingPosition(double raw_az, double raw_el,
		       double cor_az, double cor_el)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  Debug::stream()
    << std::fixed << std::setprecision(4) << std::showpos
    << Angle::toDeg(raw_az) << ' ' 
    << std::fixed << std::setprecision(4) << std::showpos 
    << Angle::toDeg(raw_el) << ' ' 
    << Angle::makeRad(cor_az).degString(4) << ' ' 
    << Angle::makeRad(cor_el).degPMString(4) 
    << std::endl;
}


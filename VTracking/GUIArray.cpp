//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUIArray.cpp
 * \ingroup VTracking
 * \brief Array GUI
 *
 * Original Author: Stephen Fegan
 * Start Date: 2006-07-14
 * $Author: aune $
 * $Date: 2011/04/13 10:17:47 $
 * $Revision: 2.38 $
 * $Tag$
 *
 **/

#include<qstatusbar.h>
#include<qfiledialog.h>
#include<qmenubar.h>
#include<qtimer.h>
#include<qtooltip.h>
#include<qpixmap.h>
#include<qlayout.h>
#include<qhbox.h>
#include<qinputdialog.h>
#include<qmessagebox.h>

#include<VSDataConverter.hpp>
#include<VATime.h>
#include<QtNotification.h>

#include"GUIAboutPane.h"
#include"GUIArray.h"
#include"GUITargetDialogs.h"
#include"GUITargetAddDialog.h"
#include"GUITargetCollectionManagerDialog.h"
#include"GUITargetDialogs.h"
#include"text.h"

#include"pixmaps/veritas.xpm"
//#include"pixmaps/go01_pix_data.xpm"
#include"pixmaps/stop_pix_data.xpm"

using namespace SEphem;
using namespace VERITAS;
using namespace VTracking;
using namespace VMessaging;

GUIArrayWidget::
GUIArrayWidget(const std::vector<TelescopeController*>& controller_vec,
	       VCorba::VOmniORBHelper* orb,
	       const std::vector<QColor>& colors,
	       const SEphem::SphericalCoords& mean_pos, 
	       const std::vector<VTracking::StowObjectVector>& scope_stow_pos,
	       std::vector<unsigned> suppress_servo_fail, bool grb,
	       unsigned theme, QWidget* parent, const char* name, WFlags f):
  QMainWindow(parent,name,f|Qt::WDestructiveClose),
  m_controller_vec(controller_vec), m_orb(orb), m_scope(controller_vec.size()),
  m_mean_earth_position(mean_pos),
  m_target_list(), m_ud_vec(m_controller_vec.size()),
  m_slew_as_array(true), m_direction_preference(CorrectionParameters::DP_NONE),
  m_next_clock_check(m_controller_vec.size()),
  m_tabwidget(), m_pane_array(),  m_pane_details(m_controller_vec.size()), 
  m_pane_targettable(), m_pane_grb_monitor(), m_pane_messenger(),
  m_stat_ind_tel(), m_stat_cntrl_panic(), m_stat_cntrl_panic_x(),
  m_timer(), m_qt_messenger(), m_qt_dialog_messenger(),
  m_sun(), m_menu_map_fwd(), m_menu_tar_array(0), m_menu_tar_scope(),
  m_grb_list(), m_grb_recommended(), m_grb_wizard(), m_grb_wizard_panes(),
  m_grb_info_gcn(), m_grb_info_time(), m_grb_info_experiment(),
  m_grb_info_type(), m_grb_info_elevation(), m_grb_info_age()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  QString basename(name);

  //QFrame* outerframe = new QFrame(this,"outer frame");
  //QGridLayout* outerlayout = 
  //new QGridLayout(outerframe,2,1,0,0,"main layout");
  
  m_tabwidget = new GUITabWidget(this /*outerframe*/,"tab bar");

  // --------------------------------------------------------------------------
  // MESSENGER
  // --------------------------------------------------------------------------

  m_qt_messenger = new QtMessenger(this,"Qt Messenger");
  Messenger::relay()->registerMessenger(m_qt_messenger);

  m_qt_dialog_messenger =
    new QtDialogMessenger(Message::PS_UNUSUAL, this, "Dialog Messenger");
  connect(m_qt_messenger,SIGNAL(message(const Message&)),
	  m_qt_dialog_messenger,SLOT(sendMessage(const Message&)));

  // --------------------------------------------------------------------------
  // SCOPE CONFIGURATION
  // --------------------------------------------------------------------------
  
  for(unsigned iscope=0; iscope<m_controller_vec.size();iscope++)
    {
      m_scope[iscope].color = colors[iscope];
      m_scope[iscope].suppress_servo_fail_error = false;
      for(unsigned ifail=0;ifail<suppress_servo_fail.size();ifail++)
	if(suppress_servo_fail[ifail]==iscope)
	  m_scope[iscope].suppress_servo_fail_error = true;
      m_scope[iscope].stow_pos = scope_stow_pos[iscope];
    }

  // --------------------------------------------------------------------------
  // THEME (WHAT A WASTE OF TIME!)
  // --------------------------------------------------------------------------

  if(theme==4)
    {
      if(m_scope.size()>0)m_scope[0].color=Qt::green;
      if(m_scope.size()>1)m_scope[1].color=Qt::red;
      if(m_scope.size()>2)m_scope[2].color=Qt::yellow;
      if(m_scope.size()>3)m_scope[3].color=Qt::blue;
    }

  // --------------------------------------------------------------------------
  // ARRAY PANE
  // --------------------------------------------------------------------------

  m_pane_array = 
    new GUIArrayPane(m_scope, mean_pos, grb, theme, this, "array pane");

  // --------------------------------------------------------------------------
  // DETAILS PANE(S)
  // --------------------------------------------------------------------------

  for(unsigned iscope=0; iscope<m_controller_vec.size();iscope++)
    if(m_controller_vec[iscope])
      {
	std::ostringstream pane_name_stream;
	pane_name_stream << "details tab " << iscope;
	m_pane_details[iscope] = 
	  new GUIDetailsPane(iscope, scope_stow_pos[iscope], 0, grb,
			     m_tabwidget, pane_name_stream.str().c_str());
	m_pane_details[iscope]->
	  setPalette(QPalette(m_scope[iscope].color,m_scope[iscope].color));
      }
  
  // --------------------------------------------------------------------------
  // TARGET TABLE PANE
  // --------------------------------------------------------------------------

  m_pane_targettable = new GUITargetTablePane(m_mean_earth_position,
					      m_tabwidget,"target table");
  
  // --------------------------------------------------------------------------
  // GRB MONITOR PANE
  // --------------------------------------------------------------------------

  if(grb)
    {
      m_pane_grb_monitor = new GUIGRBMonitor(m_orb, m_mean_earth_position, 
					     m_tabwidget, "grb monitor");
      connect(m_pane_grb_monitor,
	      SIGNAL(recommendObservation(const GRBTrigger*,
					  const VTracking::RaDecObject*)),
	      this,
	      SLOT(recommendGRBObservation(const GRBTrigger*,
					   const VTracking::RaDecObject*)));
    }

  // --------------------------------------------------------------------------
  // MESSANGER PANE
  // --------------------------------------------------------------------------

  m_pane_messenger = new GUIMessengerPane(m_tabwidget,"messenger");
  connect(m_qt_messenger,SIGNAL(message(const Message&)),
	  m_pane_messenger,SLOT(sendMessage(const Message&)));

  // --------------------------------------------------------------------------
  // ABOUT BOX
  // --------------------------------------------------------------------------

  GUIAboutPane* about = new GUIAboutPane(m_tabwidget,"about");

  // --------------------------------------------------------------------------
  // ADD PANES TO MANAGER
  // --------------------------------------------------------------------------

  m_tabwidget->addManagedPane("&Array", m_pane_array,
			      0, 0xFFFFFFFF, 0xFFFFFFFF);
  for(unsigned iscope=0; iscope<m_controller_vec.size();iscope++)
    if(m_pane_details[iscope])
      {
	std::ostringstream pane_name_stream;
	pane_name_stream << "T&" << iscope+1;
	m_tabwidget->
	  addManagedPane(pane_name_stream.str().c_str(), 
			 m_pane_details[iscope], iscope, 
			 0xFFFFFFFF, 0xFFFFFFFF);
      }
  m_tabwidget->addManagedPane("&Targets", m_pane_targettable,
			      0, 0xFFFFFFFF, 0xFFFFFFFF,
			      new TargetTableUDGetter(this));
  if(grb)
    m_tabwidget->addManagedPane("&GRB", m_pane_grb_monitor,
				0, 0xFFFFFFFF, 0xFFFFFFFF,
				new TargetTableUDGetter(this));

  m_tabwidget->addManagedPane("Messages", m_pane_messenger,
			      0, 0xFFFFFFFF, 0xFFFFFFFF);
  m_tabwidget->addManagedPane("About", about, 0, 0xFFFFFFFF, 0xFFFFFFFF);  

  // --------------------------------------------------------------------------
  // MENUS
  // --------------------------------------------------------------------------

  QMenuBar* menubar = menuBar();
  
  // *********************************
  // ********** MOTION MENU **********
  // *********************************

  QPopupMenu* motionmenu = new QPopupMenu(menubar);
#if 0
  m_menu_map_fwd["mo go"] = 
    motionmenu->insertItem(QPixmap(const_cast<const char**>(go01_pix_data)),
			   "&Go",this,SLOT(goAll()));
#endif
  m_menu_map_fwd["mo stop"] = 
    motionmenu->insertItem(QPixmap(const_cast<const char**>(stop_pix_data)),
			   "&Stop all",this,SLOT(stopAll()));

  QPopupMenu* cvmenu = new QPopupMenu(motionmenu);
  m_menu_map_fwd["cv up 0.300"] = 
    cvmenu->insertItem(MAKEDPS("Up 0.300"), this,SLOT(setTargetCV(int)));
  m_menu_map_fwd["cv up 0.030"] = 
    cvmenu->insertItem(MAKEDPS("Up 0.030"), this,SLOT(setTargetCV(int)));
  m_menu_map_fwd["cv up 0.003"] = 
    cvmenu->insertItem(MAKEDPS("Up 0.003"), this,SLOT(setTargetCV(int)));
  m_menu_map_fwd["cv dn 0.003"] = 
    cvmenu->insertItem(MAKEDPS("Down 0.003"), this,SLOT(setTargetCV(int)));
  m_menu_map_fwd["cv dn 0.030"] = 
    cvmenu->insertItem(MAKEDPS("Down 0.030"), this,SLOT(setTargetCV(int)));
  m_menu_map_fwd["cv dn 0.300"] = 
    cvmenu->insertItem(MAKEDPS("Down 0.300"), this,SLOT(setTargetCV(int)));

  cvmenu->insertSeparator();
  m_menu_map_fwd["cv cw 0.300"] = 
    cvmenu->insertItem(MAKEDPS("CW 0.300"), this,SLOT(setTargetCV(int)));
  m_menu_map_fwd["cv cw 0.030"] = 
    cvmenu->insertItem(MAKEDPS("CW 0.030"), this,SLOT(setTargetCV(int)));
  m_menu_map_fwd["cv cw 0.003"] = 
    cvmenu->insertItem(MAKEDPS("CW 0.003"), this,SLOT(setTargetCV(int)));
  m_menu_map_fwd["cv cc 0.003"] = 
    cvmenu->insertItem(MAKEDPS("CCW 0.003"), this,SLOT(setTargetCV(int)));
  m_menu_map_fwd["cv cc 0.030"] = 
    cvmenu->insertItem(MAKEDPS("CCW 0.030"), this,SLOT(setTargetCV(int)));
  m_menu_map_fwd["cv cc 0.300"] = 
    cvmenu->insertItem(MAKEDPS("CCW 0.300"), this,SLOT(setTargetCV(int)));

  cvmenu->insertSeparator();
  m_menu_map_fwd["cv custom"] = 
    cvmenu->insertItem("Custom", this,SLOT(setTargetCV(int)));
  
  m_menu_map_fwd["cv menu"] = 
    motionmenu->insertItem("Constant Speed", cvmenu);

  m_menu_map_fwd["mo menu"] = 
    menubar->insertItem("&Motion", motionmenu);

  // *********************************
  // ********** TARGET MENU **********
  // *********************************

  QPopupMenu* targetmenu = new QPopupMenu(menubar);

  m_menu_map_fwd["tar load"] = 
    targetmenu->insertItem("Load targets", this, SLOT(loadTargetList()));

  m_menu_map_fwd["tar add"] = 
    targetmenu->insertItem("Add new targets to DB", this, SLOT(addTargets()));

  m_menu_map_fwd["tar add"] = 
    targetmenu->insertItem("Manage target collections", 
			   this, SLOT(manageTargets()));
  
  m_menu_map_fwd["tar menu"] = menubar->insertItem("Database", targetmenu);

  // ***********************************
  // ********** SETTINGS MENU **********
  // ***********************************

  QPopupMenu* settingsmenu = new QPopupMenu(menubar);

  QPopupMenu* displayanglemenu = new QPopupMenu(settingsmenu);
  displayanglemenu->setCheckable(true);
  
  m_menu_map_fwd["settings coord auto"] = 
    displayanglemenu->
    insertItem("&Auto",
	       this,SLOT(changeScopeValuesToDisplay(int)));
  displayanglemenu->setItemChecked(m_menu_map_fwd["settings coord auto"],true);

  m_menu_map_fwd["settings coord sky"] = 
    displayanglemenu->insertItem("&Sky", 
				 this,SLOT(changeScopeValuesToDisplay(int)));
  displayanglemenu->setItemChecked(m_menu_map_fwd["settings coord sky"],false);

  m_menu_map_fwd["settings coord enc"] = 
    displayanglemenu->insertItem("&Encoder", 
				 this,SLOT(changeScopeValuesToDisplay(int)));
  displayanglemenu->setItemChecked(m_menu_map_fwd["settings coord enc"],false);

  m_menu_map_fwd["settings coord menu"] = 
    settingsmenu->insertItem("Display coordinates...", displayanglemenu);

  QPopupMenu* preferreddirectionmenu = new QPopupMenu(settingsmenu);
  preferreddirectionmenu->setCheckable(true);

  m_menu_map_fwd["settings pref dir array none"] = 
    preferreddirectionmenu->
    insertItem("Array - Fastest",this,SLOT(changePreferredDirection(int)));
  preferreddirectionmenu->
    setItemChecked(m_menu_map_fwd["settings pref dir array none"],true);  

  m_menu_map_fwd["settings pref dir array cw"] = 
    preferreddirectionmenu->
    insertItem("Array - Clockwise",this,SLOT(changePreferredDirection(int)));
  preferreddirectionmenu->
    setItemChecked(m_menu_map_fwd["settings pref dir array cw"],false);  

  m_menu_map_fwd["settings pref dir array ccw"] = 
    preferreddirectionmenu->
    insertItem("Array - Counter clockwise",this,SLOT(changePreferredDirection(int)));
  preferreddirectionmenu->
    setItemChecked(m_menu_map_fwd["settings pref dir array ccw"],false);  

  m_menu_map_fwd["settings pref dir scope none"] = 
    preferreddirectionmenu->
    insertItem("Individual - Fastest",this,SLOT(changePreferredDirection(int)));
  preferreddirectionmenu->
    setItemChecked(m_menu_map_fwd["settings pref dir scope none"],false);  

  m_menu_map_fwd["settings pref dir scope cw"] = 
    preferreddirectionmenu->
    insertItem("Individual - Clockwise",this,SLOT(changePreferredDirection(int)));
  preferreddirectionmenu->
    setItemChecked(m_menu_map_fwd["settings pref dir scope cw"],false);  

  m_menu_map_fwd["settings pref dir scope ccw"] = 
    preferreddirectionmenu->
    insertItem("Individual - Counter clockwise",this,SLOT(changePreferredDirection(int)));
  preferreddirectionmenu->
    setItemChecked(m_menu_map_fwd["settings pref dir scope ccw"],false);  

  m_menu_map_fwd["settings pref dir menu"] = 
    settingsmenu->insertItem("Preferred direction...", preferreddirectionmenu);

  QPopupMenu* targetsendmenu = new QPopupMenu(settingsmenu);
  targetsendmenu->setCheckable(true);
  
  m_menu_tar_array = m_menu_map_fwd["tar send array"] = 
    targetsendmenu->insertItem("&Array", this,SLOT(changeTargetRouting(int)));
  targetsendmenu->setItemChecked(m_menu_tar_array,true);

  for(unsigned iscope=0; iscope<m_controller_vec.size();iscope++)
    if(m_controller_vec[iscope])
      {
	std::string menu_item = 
	  std::string("tar send ")+VSDataConverter::toString(iscope+1);
	std::string menu_text = 
	  std::string("T&")+VSDataConverter::toString(iscope+1);
	int id = 
	  targetsendmenu->insertItem(menu_text,
				     this,SLOT(changeTargetRouting(int)));
	m_menu_map_fwd[menu_item] = id;	  
	m_menu_tar_scope[iscope] = id;
	targetsendmenu->setItemChecked(id,false);
      }

  m_menu_map_fwd["tar send menu"] = 
    settingsmenu->insertItem("Send selected object to...", 
			     targetsendmenu);

  m_menu_map_fwd["settings menu"] = 
    menubar->insertItem("Settings", settingsmenu);

  connect(this,SIGNAL(setScopeValuesToDisplay(ScopeValuesToDisplay)),
	  m_pane_array,SLOT(setScopeValuesToDisplay(ScopeValuesToDisplay)));

  connect(this,SIGNAL(changeDirPref(bool, SEphem::CorrectionParameters::DirectionPreference)),
	  m_pane_array,SLOT(changeDirPref(bool, SEphem::CorrectionParameters::DirectionPreference)));

  for(unsigned iscope=0; iscope<m_controller_vec.size();iscope++)
    if(m_pane_details[iscope])
      connect(this,SIGNAL(setScopeValuesToDisplay(ScopeValuesToDisplay)),
	      m_pane_details[iscope],
	      SLOT(setScopeValuesToDisplay(ScopeValuesToDisplay)));



  // ************************************
  // ********** SERVER CONTROL **********
  // ************************************

  bool enable_server_menu = false;
  for(unsigned iscope=0; iscope<m_controller_vec.size();iscope++)
    if((m_controller_vec[iscope])
       &&(m_controller_vec[iscope]->hasTerminateRemoteCapability()))
      {
	enable_server_menu = true;
	break;
      }

  QPopupMenu* servermenu = new QPopupMenu(menubar);

  QPopupMenu* serverterminatemenu = new QPopupMenu(servermenu);
  
  m_menu_map_fwd["server terminate all"] = 
    serverterminatemenu->
    insertItem("All",this,SLOT(catchTerminateServer(int)));

  for(unsigned iscope=0; iscope<m_controller_vec.size();iscope++)
    if((m_controller_vec[iscope])
       &&(m_controller_vec[iscope]->hasTerminateRemoteCapability()))
      {
	QString tX = QString("T")+QString::number(iscope+1);
	m_menu_map_fwd[QString("server terminate ")+tX] = 
	  serverterminatemenu->
	  insertItem(tX,this,SLOT(catchTerminateServer(int)));
      }

  m_menu_map_fwd["server terminate menu"] = 
    servermenu->insertItem("Terminate server...", serverterminatemenu);

  m_menu_map_fwd["server menu"] = menubar->insertItem("Server", servermenu);

  // --------------------------------------------------------------------------
  // STATUS BAR
  // --------------------------------------------------------------------------

  QHBox* statbox = new QHBox(this /*outerframe*/,basename+" status box");
  statbox->setMargin(5);
  statbox->setSpacing(5);

  m_stat_ind_tel.resize(m_controller_vec.size());

  for(unsigned iscope=0; iscope<m_controller_vec.size();iscope++)
    if(m_controller_vec[iscope])
      {
	std::ostringstream stat_ind_stream;
	stat_ind_stream << basename << " stat ind " << iscope;
	std::ostringstream tX_stream;
	tX_stream << "T" << iscope+1 << ':';
	new QLabel(tX_stream.str(),statbox);
	m_stat_ind_tel[iscope] = 
	  new InfoQLineEdit(statbox, stat_ind_stream.str().c_str());
	m_stat_ind_tel[iscope]->setAlignment(Qt::AlignHCenter);
  	QToolTip::add(m_stat_ind_tel[iscope],TT_ARRAY_SB_STAT);
      }

  m_stat_cntrl_panic = new QPushButton("Panic All!",statbox,"stat panic");
  m_stat_cntrl_panic->setMinimumWidth(80);
  m_stat_cntrl_panic->setMinimumHeight(22);
  m_stat_cntrl_panic->setFocusPolicy(NoFocus);

  connect(m_stat_cntrl_panic,SIGNAL(clicked()),this,SLOT(emergencyAll()));

  statusBar()->addWidget(statbox, 1, true);

  // --------------------------------------------------------------------------
  // MAKE COMMAND CONNECTIONS
  // --------------------------------------------------------------------------

  connect(m_pane_array,SIGNAL(goOne(unsigned)),this,SLOT(goOne(unsigned)));
  connect(m_pane_array,SIGNAL(stopOne(unsigned)),this,SLOT(stopOne(unsigned)));
  connect(m_pane_array,SIGNAL(emergencyOne(unsigned)),
	  this,SLOT(emergencyOne(unsigned)));
  connect(m_pane_array,SIGNAL(setTargetOne(unsigned, const VTracking::TargetObject*)),
	  this,SLOT(setTargetOne(unsigned, const VTracking::TargetObject*)));
  connect(m_pane_array,SIGNAL(setTargetOne(unsigned, const VTracking::TargetObject*,SEphem::CorrectionParameters::DirectionPreference)),
	  this,SLOT(setTargetOne(unsigned, const VTracking::TargetObject*,SEphem::CorrectionParameters::DirectionPreference)));
  connect(m_pane_array,SIGNAL(loadNewTargetList(unsigned)),
	  this,SLOT(loadTargetListOne(unsigned)));

  connect(m_pane_targettable,SIGNAL(setTarget(int)),
	  this,SLOT(routeTarget(int)));
  connect(m_pane_targettable,SIGNAL(loadTargetList()),
	  this,SLOT(loadTargetList()));

  connect(this,SIGNAL(syncWithTargetList(const VTracking::TargetList&)),
	  m_pane_array,SLOT(syncWithTargetList(const VTracking::TargetList&)));

  if(grb)
    {
      connect(m_pane_grb_monitor,
	      SIGNAL(reloadGRBList(const GUIGRBMonitor::GRBTriggerList&)),
	      m_pane_array,
	      SLOT(syncWithGRBList(const GUIGRBMonitor::GRBTriggerList&)));

      connect(m_pane_grb_monitor,
	      SIGNAL(reloadGRBList(const GUIGRBMonitor::GRBTriggerList&)),
	      this,
	      SLOT(syncWithGRBList(const GUIGRBMonitor::GRBTriggerList&)));

      connect(m_pane_grb_monitor,SIGNAL(setGRB(int)),this,SLOT(routeGRB(int)));
    }

  connect(this,SIGNAL(syncWithTargetList(const VTracking::TargetList&)),
	  m_pane_targettable,
	  SLOT(syncWithTargetList(const VTracking::TargetList&)));

  for(unsigned iscope=0; iscope<m_controller_vec.size();iscope++)
    if(m_pane_details[iscope])
      {
	GUIObjectSelector* cmd = m_pane_details[iscope]->selector();

	connect(cmd,SIGNAL(go(unsigned)),this,SLOT(goOne(unsigned)));
	connect(cmd,SIGNAL(stop(unsigned)),this,SLOT(stopOne(unsigned)));
      
	connect(cmd,SIGNAL(loadNewTargetList(unsigned)),
		this,SLOT(loadTargetListOne(unsigned)));

	connect(cmd, SIGNAL(setTarget(unsigned)),
		this, SLOT(sendSelectedTargetToScopeOne(unsigned)));
	
	connect(this,
		SIGNAL(syncWithTargetList(const VTracking::TargetList&)),
		cmd,
		SLOT(syncWithTargetList(const VTracking::TargetList&)));

	if(grb)
	  connect(m_pane_grb_monitor,
		  SIGNAL(reloadGRBList(const GUIGRBMonitor::GRBTriggerList&)),
		  cmd,
		  SLOT(syncWithGRBList(const GUIGRBMonitor::GRBTriggerList&)));
      }

  // --------------------------------------------------------------------------
  // SUN WARNING
  // --------------------------------------------------------------------------
  
  m_sun = new GUISunWarning(m_mean_earth_position,this,"sun warning");
  QToolTip::add(m_sun,
		"Shows the elevation and angular distance from the optic\n"
		"axis of the sun and/or moon. Flashes red if the telescope\n"
		"is pointing too close to sun. The display disappears when\n"
		"both sun and moon are below threshold.");

  // --------------------------------------------------------------------------
  // FINISH UP
  // --------------------------------------------------------------------------

#if 0
  for(unsigned iscope=0; iscope<m_controller_vec.size();iscope++)
    if(m_controller_vec[iscope])
      timerclear(m_next_clock_check[iscope]);
#endif

  //outerlayout->addWidget(m_tabwidget,0,0);
  //outerlayout->addWidget(statbox,1,0);
  
  setCentralWidget(m_tabwidget /* outerframe */);
  resize(minimumSize());

  GUITargetDialogs::loadDefault(m_target_list,true);
  emit syncWithTargetList(m_target_list);

  setIcon(QPixmap(const_cast<const char**>(veritas)));

  setCaption("VERITAS Array Controller");

  m_tabwidget->update(m_ud_vec);
  updateStatusBar(m_ud_vec);

  m_timer = new QTimer(this,"timer");
  connect(m_timer,SIGNAL(timeout()),this,SLOT(tick()));
  m_timer->start(50);

  QWidget* cw = centralWidget();
  QPoint p = cw->rect().topRight();
  p=cw->mapTo(this,p);
  p-=QPoint(m_sun->width(),0);
  m_sun->move(p);

  if(grb)m_pane_grb_monitor->startAcqisitionThread();
}

GUIArrayWidget::~GUIArrayWidget()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
}

// ============================================================================
//
// Single telescope control methods
//
// ============================================================================

void GUIArrayWidget::goOne(unsigned id)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(id >= m_controller_vec.size())return;
  TelescopeController* controller = m_controller_vec[id];
  if(controller == 0)return;
  const TargetObject* obj = controller->getTargetObject();
  if(obj == 0)return;
  else if(obj->objectMovesInAzEl())controller->reqTrack();
  else controller->reqSlew();
  delete obj;
}

void GUIArrayWidget::stopOne(unsigned id)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(id >= m_controller_vec.size())return;
  TelescopeController* controller = m_controller_vec[id];
  if(controller == 0)return;
  controller->reqStop();
}

void GUIArrayWidget::emergencyOne(unsigned id)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(id >= m_controller_vec.size())return;
  TelescopeController* controller = m_controller_vec[id];
  if(controller == 0)return;
  controller->emergencyStop();
}

void GUIArrayWidget::
setTargetOne(unsigned id, const VTracking::TargetObject* obj)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  setTargetOne(id,obj,m_direction_preference);
}

void GUIArrayWidget::
setTargetOne(unsigned id, const VTracking::TargetObject* obj,
	     SEphem::CorrectionParameters::DirectionPreference dp)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(id >= m_controller_vec.size())return;
  TelescopeController* controller = m_controller_vec[id];
  if(controller == 0)return;
  VTracking::TargetObject* obj_copy = 0;
  if(obj)obj_copy = obj->copy();
  controller->setTargetObject(obj_copy,dp);
}

void GUIArrayWidget::sendSelectedTargetToScopeOne(unsigned selector_id)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(selector_id >= m_controller_vec.size())return;
  TelescopeController* controller = m_controller_vec[selector_id];
  if(controller == 0)return;
  TargetObject* obj = 0;
  double az_deg = m_ud_vec[selector_id].tse.status.az.driveangle_deg;
  obj=m_pane_details[selector_id]->selector()->
    getObject(az_deg,m_ud_vec[selector_id].mjd);
  controller->setTargetObject(obj,m_direction_preference);
}

void GUIArrayWidget::terminateServerOne(unsigned selector_id)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(selector_id >= m_controller_vec.size())return;
  TelescopeController* controller = m_controller_vec[selector_id];
  if(controller == 0)return;
  try
    {
      controller->terminateRemote();
    }
  catch(const TelescopeController::CapabilityNotSupported&)
    {
      // nothing to see here
    }
}

// ============================================================================
//
// All control methods
//
// ============================================================================

void GUIArrayWidget::emergencyAll()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  std::vector<unsigned> deferred_iscopes;
  for(unsigned iscope=0; iscope<m_controller_vec.size();iscope++)
    if(m_controller_vec[iscope])
      {
	if((m_ud_vec[iscope].tse.state == TelescopeController::TS_COM_FAILURE)
	   &&(m_ud_vec[iscope].tse.cf == TelescopeController::CF_SERVER))
	  deferred_iscopes.push_back(iscope);
	else
	  emergencyOne(iscope);
      }
  for(unsigned iindex=0; iindex<deferred_iscopes.size();iindex++)
    emergencyOne(deferred_iscopes[iindex]);
}

void GUIArrayWidget::stopAll()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  std::vector<unsigned> deferred_iscopes;
  for(unsigned iscope=0; iscope<m_controller_vec.size();iscope++)
    if(m_controller_vec[iscope])
      {
	if((m_ud_vec[iscope].tse.state == TelescopeController::TS_COM_FAILURE)
	   &&(m_ud_vec[iscope].tse.cf == TelescopeController::CF_SERVER))
	  deferred_iscopes.push_back(iscope);
	else
	  stopOne(iscope);
      }
#if 0
  for(unsigned iindex=0; iindex<deferred_iscopes.size();iindex++)
    stopOne(deferred_iscopes[iindex]);
#endif
}

void GUIArrayWidget::terminateServerAll()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  std::vector<unsigned> deferred_iscopes;
  for(unsigned iscope=0; iscope<m_controller_vec.size();iscope++)
    if((m_controller_vec[iscope])
       &&(m_controller_vec[iscope]->hasTerminateRemoteCapability()))
      {
	if((m_ud_vec[iscope].tse.state == TelescopeController::TS_COM_FAILURE)
	   &&(m_ud_vec[iscope].tse.cf == TelescopeController::CF_SERVER))
	  deferred_iscopes.push_back(iscope);
	else
	  terminateServerOne(iscope);
      }
  for(unsigned iindex=0; iindex<deferred_iscopes.size();iindex++)
    terminateServerOne(deferred_iscopes[iindex]);
}


// ============================================================================
//
// Other methods
//
// ============================================================================

void GUIArrayWidget::routeTarget(int target)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  QMenuBar* menubar = menuBar();
  if(menubar->isItemChecked(m_menu_tar_array))
    {
      m_pane_array->setFromTargetTable(target);
      m_tabwidget->setCurrentPage(m_pane_array);
    }
  else
    {
      for(std::map<unsigned,int>::const_iterator 
	    iscope = m_menu_tar_scope.begin(); iscope!=m_menu_tar_scope.end();
	  iscope++)
	{
	  if(menubar->isItemChecked(iscope->second))
	    {
	      GUIObjectSelector* cmd = 
		m_pane_details[iscope->first]->selector();
	      cmd->selectTarget(target);
	      sendSelectedTargetToScopeOne(iscope->first);
	      m_tabwidget->setCurrentPage(m_pane_details[iscope->first]);
	      break;
	    }
	}
    }
}

void GUIArrayWidget::routeGRB(int target)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(m_pane_grb_monitor)
    {
      QMenuBar* menubar = menuBar();
      if(menubar->isItemChecked(m_menu_tar_array))
	{
	  m_pane_array->setFromGRBTable(target);
	  m_tabwidget->setCurrentPage(m_pane_array);
	}
      else
	{
	  for(std::map<unsigned,int>::const_iterator 
		iscope = m_menu_tar_scope.begin(); 
	      iscope!=m_menu_tar_scope.end();iscope++)
	    {
	      if(menubar->isItemChecked(iscope->second))
		{
		  GUIObjectSelector* cmd = 
		    m_pane_details[iscope->first]->selector();
		  cmd->selectGRB(target);
		  sendSelectedTargetToScopeOne(iscope->first);
		  m_tabwidget->setCurrentPage(m_pane_details[iscope->first]);
		  break;
		}
	    }
	}
    }
}

void GUIArrayWidget::grbWizardPaneSelected(const QString& title)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
 
  if(title == TTL_GRB_WIZARD_1)
    {
    }
  else if(title == TTL_GRB_WIZARD_2)
    {
      m_tabwidget->setCurrentPage(m_pane_array);
      m_grb_wizard->nextButton()->setText("Slew!");
      stopAll();
      m_pane_array->configureFullArray();
    }
  else if(title == TTL_GRB_WIZARD_3)
    {
      unsigned igrbno = 0;
      for(GUIGRBMonitor::GRBTriggerList::const_iterator igrb =
	    m_grb_list.begin(); igrb != m_grb_list.end(); igrb++, igrbno++)
	if(igrb->grb == m_grb_recommended)
	  {
	    m_pane_array->setFromGRBTable(igrbno);
	    break;
	  }
      m_pane_array->goArray(0);
    }
}
  
void GUIArrayWidget::
syncWithGRBList(const GUIGRBMonitor::GRBTriggerList& grb_list)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  m_grb_list=grb_list;
}

void GUIArrayWidget::recommendGRBObservation(const GRBTrigger* grb,
					     const VTracking::RaDecObject* obj)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  // Wizard only processes one GRB alert at a time
  if((m_grb_wizard)&&(m_grb_wizard->isVisible()))
    {
      Message message(Message::DR_LOCAL,Message::PS_UNUSUAL,
		      "Additional GRB Trigger");
      message.messageStream() 
	<< "Another observable GRB had occurred, but the GRB wizard\n"
	<< "is already active. Please complete the GRB wizard and\n"
	<< "check the GRB table for details on the latest GRB\n";
      message.detailsStream() 
	<< obj->name();
      Messenger::relay()->sendMessage(message);
      return;
    }

  if(!m_grb_wizard)
    {
      m_grb_wizard = new QWizard(this,"grb wizard");

      // Page 1 ---------------------------------------------------------------

      QFrame* frame1 = new QFrame(m_grb_wizard,"grb wizard frame 1");
      QGridLayout* layout1 = 
	new QGridLayout(frame1,7,2,0,0,"grb wizard frame 1 layout");
      
      layout1->addMultiCellWidget(new QLabel(LAB_GRB_WIZARD_1,frame1),
				  0,0,0,1);
      layout1->addWidget(new QLabel("GCN trigger number:",frame1),1,0);
      layout1->addWidget(new QLabel("Trigger time:",frame1),2,0);
      layout1->addWidget(new QLabel("Detector:",frame1),3,0);
      layout1->addWidget(new QLabel("Trigger type:",frame1),4,0);
      layout1->addWidget(new QLabel("Elevation:",frame1),5,0);
      layout1->addWidget(new QLabel("Trigger age:",frame1),6,0);

      m_grb_info_gcn = new QLabel("n/a",frame1);
      m_grb_info_time = new QLabel("n/a",frame1);
      m_grb_info_experiment = new QLabel("n/a",frame1);
      m_grb_info_type = new QLabel("n/a",frame1);
      m_grb_info_elevation = new QLabel("n/a",frame1);
      m_grb_info_age = new QLabel("n/a",frame1);
      
      layout1->addWidget(m_grb_info_gcn,1,1);
      layout1->addWidget(m_grb_info_time,2,1);
      layout1->addWidget(m_grb_info_experiment,3,1);
      layout1->addWidget(m_grb_info_type,4,1);
      layout1->addWidget(m_grb_info_elevation,5,1);
      layout1->addWidget(m_grb_info_age,6,1);

      m_grb_wizard->addPage(frame1,TTL_GRB_WIZARD_1);
      m_grb_wizard_panes[TTL_GRB_WIZARD_1] = frame1;

      // Page 2 ---------------------------------------------------------------
      
      QFrame* frame2 = new QFrame(m_grb_wizard,"grb wizard frame 2");
      QGridLayout* layout2 = 
	new QGridLayout(frame2,1,1,0,0,"grb wizard frame 2 layout");

      layout2->addWidget(new QLabel(LAB_GRB_WIZARD_2,frame2),0,0);

      m_grb_wizard->addPage(frame2,TTL_GRB_WIZARD_2);
      m_grb_wizard_panes[TTL_GRB_WIZARD_2] = frame2;

      // Page 3 ---------------------------------------------------------------
      
      QFrame* frame3 = new QFrame(m_grb_wizard,"grb wizard frame 3");
      QGridLayout* layout3 = 
	new QGridLayout(frame3,1,1,0,0,"grb wizard frame 3 layout");

      layout3->addWidget(new QLabel(LAB_GRB_WIZARD_3,frame3),0,0);

      m_grb_wizard->addPage(frame3,TTL_GRB_WIZARD_3);
      m_grb_wizard_panes[TTL_GRB_WIZARD_3] = frame3;

      QFont f = font();
      f.setPointSize(15*f.pointSize()/10);
      f.setBold(true);
      m_grb_wizard->setTitleFont(f);
      for(std::map<QString, QWidget*>::iterator ipane =
	    m_grb_wizard_panes.begin(); ipane != m_grb_wizard_panes.end();
	  ipane++)
	{
	  m_grb_wizard->setBackEnabled(ipane->second, false);
	  if(m_grb_wizard->indexOf(ipane->second) == 
	     m_grb_wizard->pageCount()-1)
	    {
	      m_grb_wizard->setNextEnabled(ipane->second, false);
	      m_grb_wizard->setFinishEnabled(ipane->second, true);
	    }
	  else
	    {
	      m_grb_wizard->setNextEnabled(ipane->second, true);
	      m_grb_wizard->setFinishEnabled(ipane->second, false);
	    }
	  m_grb_wizard->setHelpEnabled(ipane->second, false);
	}
      
      connect(m_grb_wizard,SIGNAL(selected(const QString&)),
	      this,SLOT(grbWizardPaneSelected(const QString&)));
    }

  m_grb_recommended = grb;

  Angle lmst;
  double mjd;

  for(unsigned iscope=0; iscope<m_controller_vec.size();iscope++)
    if(m_controller_vec[iscope])
      {
	GUIUpdateData& ud(m_ud_vec[iscope]);
	lmst = ud.lmst;
	mjd = ud.mjd;
	break;
      }

  SphericalCoords azel = obj->getAzEl(mjd, lmst, m_mean_earth_position);

  VATime t(grb->trigger_time_mjd_int,
	   uint64_t(grb->trigger_msec_of_day_int)*UINT64_C(1000000));
  
  QString age_txt = t.getAgeString();

  m_grb_info_gcn->setText(QString::number(grb->trigger_gcn_sequence_number));
  m_grb_info_time->setText(t.getString().substr(0,23));
  m_grb_info_experiment->setText(QString(grb->trigger_instrument));
  m_grb_info_type->setText(QString(grb->trigger_type));
  m_grb_info_elevation->setText(azel.latitude().degPM180String(1));
  m_grb_info_age->setText(age_txt);

  m_grb_wizard->showPage(m_grb_wizard->page(0));
  m_grb_wizard->show();
}

void GUIArrayWidget::loadTargetListOne(unsigned id)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  // This method just used by individual details pages
  loadTargetList();
}

void GUIArrayWidget::loadTargetList()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  if(GUITargetDialogs::loadFromDBCollection(m_target_list, false, 
					    this, "load targets dialogs"))
    emit syncWithTargetList(m_target_list);
}

void GUIArrayWidget::addTargets()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  GUITargetAddDialog::addTargets(m_orb,this,"add targets dialog");
}

void GUIArrayWidget::manageTargets()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  GUITargetCollectionManagerDialog::
    manageTargets(GUITargetDialogs::getDefaultDBCollectionName(),
		  this,"manage targets dialog");
}

// ============================================================================
//
// Internal methods
//
// ============================================================================

void GUIArrayWidget::testForLargeClockDifference()
{
  for(unsigned iscope=0; iscope<m_controller_vec.size();iscope++)
    if(m_controller_vec[iscope])
      {
	GUIUpdateData& ud(m_ud_vec[iscope]);
	if((ud.tse.state != TelescopeController::TS_COM_FAILURE)&&
	   (ud.tv.tv_sec>m_next_clock_check[iscope]))
	  {
	    if(m_next_clock_check[iscope]==0)
	      m_next_clock_check[iscope]=ud.tv.tv_sec;
	    m_next_clock_check[iscope]+=60;

	    int64_t deltat = 
	      (int64_t(ud.tse.tv.tv_sec)-int64_t(ud.tv.tv_sec))
	      *INT64_C(1000000)
	      +(int64_t(ud.tse.tv.tv_usec)-int64_t(ud.tv.tv_usec));
	    if((deltat>INT64_C(2000000))||(deltat<INT64_C(-2000000)))
	      {
		Message message(Message::DR_LOCAL,Message::PS_UNUSUAL,
				"Large time difference detected");
		message.messageStream() 
		  << "A large time difference ("
		  << fabs(double(deltat)/1000000.0)
		  << " sec) has been detected between the"
		  << std::endl
		  << "computer running this GUI and that running the T" 
		  << iscope+1 << " controller." << std::endl
		  << "It is important that the time be correct on the "
		  << "controller computer." << std::endl;
		message.detailsStream()
		  << "If you see this message popping up for each of the\n"
		  << "telescopes, it is likely that the clock on the GUI\n"
		  << "computer is wrong. This is not too serious, it will\n"
		  << "not cause a problem with the tracking, it will\n"
		  << "however mean that some of the information on the GUI\n"
		  << "is incorrect, e.g. the tracking error. If the message\n"
		  << "pops up only about one telescope it is possible that\n"
		  << "the PCS control computer for that telescope has the\n"
		  << "wrong system time. Log in as root to the appropriate\n"
		  << "computer and try to start the Network Time Protocol\n"
		  << "(NTP) program (try: \"service ntpd restart\"). If that\n"
		  << "does not work, find the correct time and use the UNIX\n"
		  << "date command to set the approximate time:\n"
		  << "\"date MMDDhhmmYYYY.ss\". Consult \"man date\" for\n"
		  << "further information\n";
		Messenger::relay()->sendMessage(message);
	      }
	  }
      }
}

void GUIArrayWidget::tick()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  std::vector<std::pair<unsigned,SphericalCoords> > scope_coords;

  Angle lmst;
  double mjd;
  unsigned update_number;
  for(unsigned iscope=0; iscope<m_controller_vec.size();iscope++)
    if(m_controller_vec[iscope])
      {
	GUIUpdateData& ud(m_ud_vec[iscope]);
	ud.fillData(m_controller_vec[iscope], m_mean_earth_position);
	if(ud.tse.state != TelescopeController::TS_COM_FAILURE)
	  scope_coords.
	    push_back(std::make_pair<unsigned,SphericalCoords>
		      (iscope,ud.tse.tel_azel));
	lmst = ud.lmst;
	mjd = ud.mjd;
	update_number = ud.update_number;
      }

  if(m_pane_grb_monitor)
    {
      static const unsigned ffreq = 14;
      if((update_number/ffreq)%2==1)
	{
	  if(((update_number%(ffreq*2))==ffreq)
	     &&(m_pane_grb_monitor->havePotentiallyObservableGRB()))
	    m_tabwidget->setHighlighting(m_pane_grb_monitor,true,QColor(255,120,40));
	}
      else if((update_number%(ffreq*2))==0)
	m_tabwidget->setHighlighting(m_pane_grb_monitor,false);
    }
    
  m_sun->update(scope_coords, mjd, lmst);
  m_tabwidget->update(m_ud_vec);
  updateStatusBar(m_ud_vec);
  testForLargeClockDifference();
}

void GUIArrayWidget::setTargetCV(int item)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(item == m_menu_map_fwd["cv up 0.300"])doCV(0.300,0.000);
  else if(item == m_menu_map_fwd["cv up 0.030"])doCV(+0.030, 0.000);
  else if(item == m_menu_map_fwd["cv up 0.003"])doCV(+0.003, 0.000);
  else if(item == m_menu_map_fwd["cv dn 0.003"])doCV(-0.003, 0.000);
  else if(item == m_menu_map_fwd["cv dn 0.030"])doCV(-0.030, 0.000);
  else if(item == m_menu_map_fwd["cv dn 0.300"])doCV(-0.300, 0.000); 
  else if(item == m_menu_map_fwd["cv cw 0.300"])doCV( 0.000,+0.300); 
  else if(item == m_menu_map_fwd["cv cw 0.030"])doCV( 0.000,+0.030); 
  else if(item == m_menu_map_fwd["cv cw 0.003"])doCV( 0.000,+0.003); 
  else if(item == m_menu_map_fwd["cv cc 0.003"])doCV( 0.000,-0.003); 
  else if(item == m_menu_map_fwd["cv cc 0.030"])doCV( 0.000,-0.030); 
  else if(item == m_menu_map_fwd["cv cc 0.300"])doCV( 0.000,-0.300); 
  else if(item == m_menu_map_fwd["cv custom"])
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
      doCV(el_speed,az_speed);
    }
}

void GUIArrayWidget::changeTargetRouting(int item)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  QMenuBar* menubar = menuBar();
  menubar->setItemChecked(m_menu_tar_array,item == m_menu_tar_array);
  for(std::map<unsigned,int>::const_iterator iscope = m_menu_tar_scope.begin();
      iscope != m_menu_tar_scope.end(); iscope++)
    menubar->setItemChecked(iscope->second, item == iscope->second);
}

void GUIArrayWidget::changeScopeValuesToDisplay(int item)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  QMenuBar* menubar = menuBar();
  if(item == m_menu_map_fwd["settings coord auto"])
    {
      menubar->setItemChecked(m_menu_map_fwd["settings coord auto"],true);
      emit setScopeValuesToDisplay(SV_AUTO);
    }
  else
    menubar->setItemChecked(m_menu_map_fwd["settings coord auto"],false);

  if(item == m_menu_map_fwd["settings coord sky"])
    {
      menubar->setItemChecked(m_menu_map_fwd["settings coord sky"],true);
      emit setScopeValuesToDisplay(SV_SKY);
    }
  else
    menubar->setItemChecked(m_menu_map_fwd["settings coord sky"],false);

  if(item == m_menu_map_fwd["settings coord enc"])
    {
      menubar->setItemChecked(m_menu_map_fwd["settings coord enc"],true);
      emit setScopeValuesToDisplay(SV_ENCODER);
    }
  else
    menubar->setItemChecked(m_menu_map_fwd["settings coord enc"],false);
}

void GUIArrayWidget::changePreferredDirection(int item)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  menuBar()->setItemChecked(m_menu_map_fwd["settings pref dir array none"],false);
  menuBar()->setItemChecked(m_menu_map_fwd["settings pref dir array cw"],false);
  menuBar()->setItemChecked(m_menu_map_fwd["settings pref dir array ccw"],false);
  menuBar()->setItemChecked(m_menu_map_fwd["settings pref dir scope none"],false);
  menuBar()->setItemChecked(m_menu_map_fwd["settings pref dir scope cw"],false);
  menuBar()->setItemChecked(m_menu_map_fwd["settings pref dir scope ccw"],false);

  if(item == m_menu_map_fwd["settings pref dir array none"])
    m_direction_preference = CorrectionParameters::DP_NONE,
      m_slew_as_array = true;
  else if(item == m_menu_map_fwd["settings pref dir array cw"])
    m_direction_preference = CorrectionParameters::DP_CW,
      m_slew_as_array = true;
  else if(item == m_menu_map_fwd["settings pref dir array ccw"])
    m_direction_preference = CorrectionParameters::DP_CCW,
      m_slew_as_array = true;
  else if(item == m_menu_map_fwd["settings pref dir scope none"])
    m_direction_preference = CorrectionParameters::DP_NONE,
      m_slew_as_array = false;
  else if(item == m_menu_map_fwd["settings pref dir scope cw"])
    m_direction_preference = CorrectionParameters::DP_CW,
      m_slew_as_array = false;
  else if(item == m_menu_map_fwd["settings pref dir scope ccw"])
    m_direction_preference = CorrectionParameters::DP_CCW,
      m_slew_as_array = false;
  menuBar()->setItemChecked(item,true);
  emit changeDirPref(m_slew_as_array,m_direction_preference);
}

void GUIArrayWidget::catchTerminateServer(int item)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(item == m_menu_map_fwd["server terminate all"])
    terminateServerAll();
  else
    {
      for(unsigned iscope=0; iscope<m_controller_vec.size();iscope++)
	if((m_controller_vec[iscope])
	   &&(m_controller_vec[iscope]->hasTerminateRemoteCapability()))
	  {
	    QString tX = QString("T")+QString::number(iscope+1);
	    if(item == m_menu_map_fwd[QString("server terminate ")+tX])
	      {
		terminateServerOne(iscope);
		break;
	      }
	  }
    }
}

void GUIArrayWidget::closeEvent(QCloseEvent * e)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(m_pane_grb_monitor)m_pane_grb_monitor->stopAcqisitionThread();  
#if 0
  VTaskNotification::QtNotificationList::getInstance()->
    synchronouslyTerminateNotificationProcessing();
#endif
  e->accept();
}

void GUIArrayWidget::resizeEvent(QResizeEvent* e)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  QMainWindow::resizeEvent(e);
  QWidget* cw = centralWidget();
  QPoint p = cw->rect().topRight();
  p=cw->mapTo(this,p);
  p-=QPoint(m_sun->width(),0);
  m_sun->move(p);
}

void GUIArrayWidget::doCV(double elspeed, double azspeed)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  int yesno = 
    QMessageBox::warning(this, "Constant Spee Warning",
			 "Selecting the constant speed target will \n"
			 "immediately initiate motion of the array.\n"
			 "Do you want to move the telescopes?",
			 QMessageBox::Yes, 
	     QMessageBox::No | QMessageBox::Default | QMessageBox::Escape);

  if((yesno == QMessageBox::Yes)&&(m_pane_array->canReTargetArray()))
    {
      unsigned ntel = m_ud_vec.size();
      for(unsigned itel=0; itel<ntel; itel++)if(m_pane_array->isInArray(itel))
	{
	  SphericalCoords ic = 
	    SphericalCoords::
	    makeLatLongDeg(m_ud_vec[itel].tse.status.el.driveangle_deg,
			   m_ud_vec[itel].tse.status.az.driveangle_deg);
	  TargetObject* obj = new CVObject(ic,azspeed,elspeed,
					   m_ud_vec[itel].tse.mjd);
	  setTargetOne(itel, obj, CorrectionParameters::DP_NONE);
	  goOne(itel);
	}
    }
}

void GUIArrayWidget::updateStatusBar(const std::vector<GUIUpdateData>& ud_vec)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  bool got_one = false;
  for(unsigned iscope=0; iscope<m_controller_vec.size();iscope++)
    if(m_controller_vec[iscope])
      {
	GUIUpdateData& ud(m_ud_vec[iscope]);

	QString message;
	enum { MC_NONE, MC_WARN, MC_STOP, MC_SLEW, MC_TRACK } message_color;

	if(ud.tse.state==TelescopeController::TS_COM_FAILURE)
	  {
	    if(ud.tse.cf == TelescopeController::CF_SERVER)
	      message = "NO SERVER";
	    else message = "SCOPE COM FAIL";
	    message_color=MC_WARN;
	  }
	else if(ud.tse.status.interlock
		|| ud.tse.status.interlockAzPullCord
		|| ud.tse.status.interlockAzStowPin
		|| ud.tse.status.interlockElStowPin
		|| ud.tse.status.interlockAzDoorOpen
		|| ud.tse.status.interlockElDoorOpen
		|| ud.tse.status.interlockSafeSwitch
		|| !ud.tse.status.remoteControl)
	  {
	    message = "INTERLOCK";
	    message_color=MC_WARN;      
	  }
	else if(ud.tse.status.el.limitCwUp
		|| ud.tse.status.el.limitCcwDown
		|| ud.tse.status.az.limitCwUp
		|| ud.tse.status.az.limitCcwDown)
	  {
	    message = "LIMIT";
	    message_color=MC_WARN;   
	  }
	else if((!m_scope[iscope].suppress_servo_fail_error 
		 && ( ud.tse.status.az.servo1Fail
		      || ud.tse.status.az.servo2Fail
		      || ud.tse.status.el.servo1Fail
		      || ud.tse.status.el.servo2Fail ) )
		|| ud.tse.status.az.positionFault
		|| ud.tse.status.el.positionFault
		|| !ud.tse.status.checksumOK
		|| ud.tse.status.msgBadFrame
		|| ud.tse.status.msgCommandInvalid
		|| ud.tse.status.msgInputOverrun
		|| ud.tse.status.msgOutputOverrun)
	  {
	    message = "ERROR";
	    message_color=MC_WARN;      
	  }
	else if(ud.tse.state==TelescopeController::TS_TRACK)
	  {
	    message = "TRACKING";
	    message_color=MC_TRACK;
	  }
	else if(ud.tse.state==TelescopeController::TS_SLEW)
	  {
	    message = "SLEWING";
	    message_color=MC_SLEW;
	  }
	else if(ud.tse.state==TelescopeController::TS_RESTRICTED_MOTION)
	  {
	    message = "RESTRICTED MOTION";
	    message_color=MC_SLEW;
	  }
	else if(ud.tse.state==TelescopeController::TS_RAMP_DOWN)
	  {
	    message = "RAMP DOWN";
	    message_color=MC_SLEW;
	  }
	else
	  {
	    message = "STOPPED";
	    message_color=MC_STOP;
	  }

	QColor fore;
	QColor back;

	switch(message_color)
	  {
	  case MC_NONE:
	    fore = black;
	    back = white;
	    break;
	  case MC_WARN:
	    fore = black;
	    back = color_bg_warn;
	    break;
	  case MC_STOP:
	    fore = color_bg_warn;
	    back = white;
	    break;	    
	  case MC_SLEW:
	    fore = color_bg_on;
	    back = white;
	    break;	    
	  case MC_TRACK:
	    fore = black;
	    back = color_bg_on;
	    break;	    
	  }

	m_stat_ind_tel[iscope]->setPaletteBackgroundColor(back);
	m_stat_ind_tel[iscope]->setPaletteForegroundColor(fore);
	m_stat_ind_tel[iscope]->setText(message);

	if(!got_one)
	  {
	    int x =
	      int(floor(30*(1+sin(fmod(ud.mjd*86400/2,1)*Angle::sc_twoPi))));
	    if(x!=m_stat_cntrl_panic_x)
	      {
		QColor pfcolor=QColor(255-x,x,x);
		QColor pbcolor=QColor(255-60+x,60-x,60-x);
		m_stat_cntrl_panic->setPalette(QPalette(pfcolor,pbcolor));
		m_stat_cntrl_panic_x=x;
	      }
	    got_one=true;
	  }
      }
}

void GUIArrayWidget::
getTargetTableUpdateData(GUIUpdateData& ud,
			 const std::vector<GUIUpdateData>& ud_vec)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  QMenuBar* menubar = menuBar();
  if(menubar->isItemChecked(m_menu_tar_array))
    {
      ud.replay = ud_vec[0].replay;
      m_pane_array->getMeanUpdateData(ud.tse.state,ud.tse.req,ud_vec);
      ud.mjd =  ud_vec[0].mjd;
      ud.lmst =  ud_vec[0].lmst;
    }
  else
    {
      for(std::map<unsigned,int>::const_iterator 
	    iscope = m_menu_tar_scope.begin(); iscope!=m_menu_tar_scope.end();
	  iscope++)
	{
	  if(menubar->isItemChecked(iscope->second))
	    {
	      ud.replay = ud_vec[iscope->first].replay;
	      ud.tse.req = ud_vec[iscope->first].tse.req;
	      ud.tse.state = ud_vec[iscope->first].tse.state;
	      ud.mjd =  ud_vec[iscope->first].mjd;
	      ud.lmst =  ud_vec[iscope->first].lmst;
	      break;
	    }
	}
    }
}

// ============================================================================
//
// TargetTableUDGetter
//
// ============================================================================

GUIArrayWidget::TargetTableUDGetter::~TargetTableUDGetter()
{
  // nothing to see here
}

std::vector<GUIUpdateData> GUIArrayWidget::TargetTableUDGetter::
getUpdateData(const std::vector<GUIUpdateData>& ud_vec) const
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  std::vector<GUIUpdateData> ud(1);
  m_array->getTargetTableUpdateData(ud[0],ud_vec);
  return ud;
}

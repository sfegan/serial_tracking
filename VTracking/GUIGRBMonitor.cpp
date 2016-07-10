//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUIGRBMonitor.cpp
 * \ingroup VTracking
 * \brief This is a one-line description of this cpp file.
 *
 * Here is a tedious verbose multi-line description of all
 * the details of the code, more than you would
 * ever want to read. Generally, all the important documentation
 * goes in the .cpp files.
 *
 * Original Author: Stephen Fegan
 * $Author: jperkins $
 * $Date: 2010/02/08 21:58:48 $
 * $Revision: 2.20 $
 * $Tag$
 *
 **/

#include<qhbox.h>
#include<qlayout.h>
#include<qtooltip.h>

#include<VATime.h>
#include<QtNotification.h>
#include<Message.h>
#include<Messenger.h>
#include<Exception.h>
#include<Astro.h>
#include<VSDataConverter.hpp>

#include"TargetObject.h"
#include"GUIGRBMonitor.h"
#include"GUIMisc.h"
#include"text.h"

using namespace VTaskNotification;
using namespace VMessaging;
using namespace VTracking;
using namespace VCorba;
using namespace VERITAS;
using namespace SEphem;

#define N(x) (sizeof(x)/sizeof(*x))

#define MINEL 20.0

GUIGRBMonitor::
GUIGRBMonitor(VOmniORBHelper* orb, const SphericalCoords& earth_position,
	      QWidget* parent, const char* name):
  QFrame(parent,name), GUITabPane(this), PhaseLockedLoop(1000),
  m_orb(orb), m_earth_position(earth_position), m_thread(), m_grb_monitor(), 
  m_last_ud(), 
  m_grb_connected_with_monitor(false), m_grb_last_id(), m_grb_list(),
  m_grb_last_processed(m_grb_list.end()),
  m_grb_gcn_connection_is_up(), m_grb_time_since_last_gcn_receipt_sec(),
  m_grb_server_uptime_sec(),
  m_grb_stat(), m_grb_uptime(), m_grb_gcn(), m_grb_packet(), m_stack(),
  m_grbtable(), /* m_sortorder(), m_displaycriteria(), */ m_selectbutton(),
  m_undelivered_notes()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  QString basename(name);

  QGridLayout* mainlayout = new QGridLayout(this,2,1,5,5,
					    basename+" main layout");

  // --------------------------------------------------------------------------
  // MAIN BOXES
  // --------------------------------------------------------------------------

  MyQGroupBox* statbox = new MyQGroupBox(1,Qt::Vertical,"GRB monitor status",
					this,basename+" status box");

  MyQGroupBox* grbbox = new MyQGroupBox(1,Qt::Vertical,"GRB targets",
					this,basename+" grb box");

  // --------------------------------------------------------------------------
  // LINE EDITS
  // --------------------------------------------------------------------------

  struct status_entry
  {
    QFrame* box;
    QLineEdit** le;
    QString label;
    QString tooltip;
    int width;
  };
  
  struct status_entry status_entries[] = {
    { statbox, &m_grb_stat,   "Server status",   TT_GRB_MONITOR_STATUS,   75 },
    { statbox, &m_grb_uptime, "Uptime",          TT_GRB_MONITOR_UPTIME,   75 },
    { statbox, &m_grb_gcn, "GCN connection status",TT_GRB_GCN_CONNECTION, 75 },
    { statbox, &m_grb_packet, "Last GCN packet", TT_GRB_GCN_PACK_TIME,    75 },
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
  // GRB FRAME, STACK AND TABLE
  // --------------------------------------------------------------------------

  QFrame* grb_frame = new QFrame(grbbox,basename+" grb frame");
  QGridLayout* grb_layout = 
    new QGridLayout(grb_frame,1,1,0,5,basename+" grb layout");

  m_stack = new QWidgetStack(grb_frame,basename+" grb stack");

  QLabel* no_grb_label =
    new QLabel("No GRBs available", m_stack, basename+"no grbs label");
  no_grb_label->setAlignment(Qt::AlignCenter);

  // --------------------------------------------------------------------------
  // GRB TABLE
  // --------------------------------------------------------------------------

  QFrame* table_frame = new QFrame(this,basename+" grb table frame");

  int colwidths[] = { 80, 150, 175, -1, 150, 150, 75, 150, 75 };
  QGridLayout* table_layout = 
    new QGridLayout(table_frame,2,1,0,5,basename+" grb table frame layout");

  m_grbtable = new MyQTable(table_frame,basename+" grb table");

  m_grbtable->setNumCols(N(colwidths));
  m_grbtable->setSelectionMode(QTable::SingleRow);
  m_grbtable->setReadOnly(true);
  m_grbtable->setShowGrid(false);

  for(unsigned i=0; i<N(colwidths); i++)
    {
      if(colwidths[i]<0)m_grbtable->setColumnStretchable(i,true);
      else m_grbtable->setColumnWidth(i,colwidths[i]);
    }

  QStringList tablelabels;
  tablelabels.append("GCN");
  tablelabels.append("Detector");
  tablelabels.append("Type");
  tablelabels.append("Trigger time");
  tablelabels.append("R.A.");
  tablelabels.append("Dec.");
  tablelabels.append("El.");
  tablelabels.append("Age");
  tablelabels.append("Observe");
  m_grbtable->setColumnLabels(tablelabels);

  connect(m_grbtable,SIGNAL(doubleClicked(int,int,int, const QPoint&)),
	  this,SLOT(selectButtonPressed()));

  QHBox* box2 = new QHBox(table_frame,basename+" grb table button frame");
  box2->setSpacing(5);
  m_selectbutton = 
    new QPushButton("Select Object",box2,basename+" grb select object");
  connect(m_selectbutton,SIGNAL(clicked()),this,SLOT(selectButtonPressed()));
  QPushButton* updatebutton = 
    new QPushButton("Update List",box2,basename+" target table update");
  connect(updatebutton,SIGNAL(clicked()),this,SLOT(updateTable()));
  QPushButton* wizardbutton =
    new QPushButton("GRB Wizard",box2,basename+" target table update");
  connect(wizardbutton,SIGNAL(clicked()),this,SLOT(wizardButtonPressed()));
  box2->setStretchFactor(m_selectbutton,3);
  box2->setStretchFactor(updatebutton,1);
  box2->setStretchFactor(wizardbutton,1);

  table_layout->addWidget(m_grbtable,0,0);
  table_layout->addWidget(box2,1,0);
  table_layout->setRowStretch(0,1);
  table_layout->setRowStretch(1,0);
  table_layout->setRowStretch(2,0);

  m_stack->addWidget(no_grb_label,0);
  m_stack->addWidget(table_frame,1);
  m_stack->raiseWidget(0);

  grb_layout->addWidget(m_stack,0,0);
  grb_layout->setColStretch(0,1);
  grb_layout->setRowStretch(0,1);

  // --------------------------------------------------------------------------
  // Add everything to the layout
  // --------------------------------------------------------------------------

  mainlayout->addWidget(statbox,0,0);
  mainlayout->addWidget(grbbox,1,0);
  mainlayout->setRowStretch(1,1);  
}

GUIGRBMonitor::~GUIGRBMonitor()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  assert(m_thread == 0);

  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  for(std::set<GRBNotification*>::iterator inote=m_undelivered_notes.begin();
      inote!=m_undelivered_notes.end();inote++)(*inote)->disavow();

  if(m_grb_monitor)
    {
      CORBA::release(m_grb_monitor);
      m_grb_monitor=0;
    }
  for(GRBTriggerList::iterator igrb = m_grb_list.begin();
      igrb != m_grb_list.end(); igrb++)
    {
      delete igrb->grb;
      delete igrb->obj;
    }
}

class RunnableProtector: public ZThread::Runnable
{
public:
  RunnableProtector(Runnable* runnable):
    ZThread::Runnable(), m_runnable(runnable) { }
  ~RunnableProtector() { }
  virtual void run() { m_runnable->run(); }
public:
  ZThread::Runnable* m_runnable;
};

void GUIGRBMonitor::startAcqisitionThread()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  m_thread = new ZThread::Thread(new RunnableProtector(this));
}

void GUIGRBMonitor::stopAcqisitionThread()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  terminate();
  m_thread->wait();
  delete m_thread;
  m_thread = 0;
}

void GUIGRBMonitor::iterate()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  bool connected_with_monitor  = false;
  CORBA::ULong last_id = m_grb_last_id;
  std::list<GRBTrigger*> pending_list;
  CORBA::Boolean gcn_connection_is_up = false;
  CORBA::ULong time_since_last_gcn_receipt_sec = 0;
  CORBA::ULong server_uptime_sec = 0;

  try
    {
      if(m_grb_monitor == 0)
	m_grb_monitor = 
	  m_orb->nsGetNarrowedObject<VGRBMonitor::Command>
	  (VGRBMonitor::progName, VGRBMonitor::Command::objName);

      m_grb_monitor->nGetStatus(gcn_connection_is_up, 
				time_since_last_gcn_receipt_sec,
				server_uptime_sec);
      
      CORBA::ULong next_id =
	m_grb_monitor->nGetNextTriggerSequenceNumber(last_id);
      while(next_id != 0)
	{
	  GRBTrigger* trigger = m_grb_monitor->nGetOneTrigger(next_id);
	  pending_list.push_back(trigger);
	  last_id = next_id;
	  next_id = m_grb_monitor->nGetNextTriggerSequenceNumber(last_id);
	}

      connected_with_monitor = true;
    }
  catch(const CosNaming::NamingContext::NotFound)
    {
      if(m_grb_monitor)
	{
	  CORBA::release(m_grb_monitor);
	  m_grb_monitor=0;
	}
    }
  catch(const CORBA::TRANSIENT& x)    
    {
      if(m_grb_monitor)
	{
	  CORBA::release(m_grb_monitor);
	  m_grb_monitor=0;
	}
    }
  catch(const CORBA::Exception& x)
    {
      if(m_grb_monitor)
	{
	  CORBA::release(m_grb_monitor);
	  m_grb_monitor=0;
	}
    }
  catch(...)
    {
      if(m_grb_monitor)
	{
	  CORBA::release(m_grb_monitor);
	  m_grb_monitor=0;
	}
    }

  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  m_grb_connected_with_monitor            = connected_with_monitor;
  m_grb_last_id                           = last_id;

  for(std::list<GRBTrigger*>::iterator igrb = pending_list.begin();
      igrb != pending_list.end(); igrb++)
    {
      SphericalCoords radec;
      radec.setLatLong(Angle::makeDeg((*igrb)->coord_dec_deg),
		       Angle::makeDeg((*igrb)->coord_ra_deg));

      RaDecObject* obj = 
	new RaDecObject(radec, Astro::julianEpochToMJD((*igrb)->coord_epoch_J),
			makeName(*igrb));
      
      GRBTrigger* retraction = 0;
      if(strncmp((*igrb)->trigger_type,"RETRACTION",10)==0)
	{
	  std::string type;
	  if(strlen((*igrb)->trigger_type)>11)
	    type = std::string((*igrb)->trigger_type).substr(11);

	  for(GRBTriggerList::iterator jgrb = m_grb_list.begin();
	      jgrb != m_grb_list.end(); jgrb++)
	    if((strcmp((*igrb)->trigger_instrument,
		       jgrb->grb->trigger_instrument)==0)
	       &&((*igrb)->trigger_gcn_sequence_number 
		  == jgrb->grb->trigger_gcn_sequence_number)
	       &&(type.empty() || (type == jgrb->grb->trigger_type)))
	      {
		retraction = jgrb->grb;
		jgrb->retraction = *igrb;
		break;
	      }
	}

      m_grb_list.push_front(GRBTriggerListDatum(*igrb,obj,retraction));
    }

  m_grb_gcn_connection_is_up              = gcn_connection_is_up;
  m_grb_time_since_last_gcn_receipt_sec   = time_since_last_gcn_receipt_sec;
  m_grb_server_uptime_sec                 = server_uptime_sec;

  GRBNotification* notice = new GRBNotification(this,!pending_list.empty());
  m_undelivered_notes.insert(notice);
  QtNotificationList::getInstance()->scheduleNotification(notice);
}

// Coordinate changes with GRBAlert

std::string GUIGRBMonitor::makeName(const GRBTrigger* grb)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  VATime t(grb->trigger_time_mjd_int,
	   uint64_t(grb->trigger_msec_of_day_int)*UINT64_C(1000000));
  std::string name = 
    std::string("GRB ")
    + t.getString().substr(0,19)
    + std::string(" ")
    + std::string(grb->trigger_instrument)
    + std::string(" ")
    + VSDataConverter::toString(grb->trigger_gcn_sequence_number)
    + std::string(" ")
    + std::string(grb->trigger_type);
  return name;
}

unsigned GUIGRBMonitor::getGRBNumber(const GRBTrigger* grb)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  unsigned igrbno = 0;
  for(GRBTriggerList::const_iterator igrb = m_grb_list.begin();
      igrb != m_grb_list.end(); igrb++, igrbno++)
    if(igrb->grb == grb)return igrbno;
  return m_grb_list.size();
}

bool GUIGRBMonitor::havePotentiallyObservableGRB()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  VATime now = VATime::now();
  double mjd = now.getMJDDbl();
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  for(GRBTriggerList::iterator igrb = m_grb_list.begin();
      igrb != m_grb_list.end(); igrb++)
    {
      const GRBTrigger* grb = igrb->grb;
      VATime then(grb->trigger_time_mjd_int,
		  uint64_t(grb->trigger_msec_of_day_int)
		  *UINT64_C(1000000));
      double hrs_diff = (mjd - then.getMJDDbl())*24.0;
	  
      if((grb->veritas_should_observe)
	 &&(!igrb->retraction)
	 &&(hrs_diff < grb->veritas_observation_window_hours))
	return true;
    }
  return false;
}

void GUIGRBMonitor::
doGRBNotification(GRBNotification* delivered_note, bool update_table)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  if(1)
    {
      ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
      m_undelivered_notes.erase(delivered_note);
    }

  updateStatus();

  if(update_table)
    {
      updateTable();

      ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
      emit reloadGRBList(m_grb_list);

      VATime now = VATime::now();
      double mjd = now.getMJDDbl();
      Angle lmst = Astro::mjdToLMST(mjd, m_earth_position.phiRad());

      SunObject sun;
      SphericalCoords sun_azel = sun.getAzEl(mjd, lmst, m_earth_position);

      MoonObject moon;
      SphericalCoords moon_azel = moon.getAzEl(mjd, lmst, m_earth_position);

      //Modified this on 2009/04/29 - JSP.  I think this will make it 
      //so the alerts always happen even when the moon is up.  If you 
      //need to revert to the old version, uncomment the following line and 
      //comment the next line.
      //if((sun_azel.latitudeDeg() < -12.0)&&(moon_azel.latitudeDeg() < 3.0))
      if((sun_azel.latitudeDeg() < -12.0)&&(moon_azel.latitudeDeg() < 90.0))
	{
	  for(GRBTriggerList::iterator igrb = m_grb_list.begin();
	      igrb != m_grb_last_processed; igrb++)
	    {
	      const GRBTrigger* grb = igrb->grb;
	      const RaDecObject* obj = igrb->obj;
	      VATime then(grb->trigger_time_mjd_int,
			  uint64_t(grb->trigger_msec_of_day_int)
			  *UINT64_C(1000000));
	      double hrs_diff = (mjd - then.getMJDDbl())*24.0;
	  
	      SEphem::SphericalCoords azel =
		obj->getAzEl(mjd, lmst, m_earth_position);
	      
	      if((grb->veritas_should_observe)
		 &&(!igrb->retraction)
		 &&(hrs_diff < grb->veritas_observation_window_hours)
		 &&(azel.latitudeDeg() > MINEL))
		{
		  emit recommendObservation(grb, obj);
		  break;
		}
	    }

	  for(GRBTriggerList::iterator igrb = m_grb_list.begin();
	      igrb != m_grb_last_processed; igrb++)
	    {
	      if((strncmp(igrb->grb->trigger_type,"RETRACTION",10)==0)
		 &&(igrb->retraction))
		{
		  // Search for the notice for which this is a retraction
		  
		  GRBTriggerList::iterator jgrb = m_grb_last_processed;
		  while(jgrb != m_grb_list.end())
		    {
		      if(jgrb->grb == igrb->retraction)break;
		      jgrb++;
		    }
		  
		  if(jgrb == m_grb_list.end())
		    {
		      // the retraction is for one of the new GRBs or
		      // one that had expired from the GRB list so no
		      // need to send a notice
		      continue;
		    }

		  const GRBTrigger* grb = jgrb->grb;
		  const RaDecObject* obj = jgrb->obj;
		  VATime then(grb->trigger_time_mjd_int,
			      uint64_t(grb->trigger_msec_of_day_int)
			      *UINT64_C(1000000));
		  double hrs_diff = (mjd - then.getMJDDbl())*24.0;
		    
		  SEphem::SphericalCoords azel =
		    obj->getAzEl(mjd, lmst, m_earth_position);

		  if((grb->veritas_should_observe)
		     &&(hrs_diff < grb->veritas_observation_window_hours)
		     &&(azel.latitudeDeg() > MINEL))
		    {
		      Message message(Message::DR_LOCAL,Message::PS_UNUSUAL,
				      "GRB Retraction");
		      message.messageStream() 
			<< "A GRB retraction to an observable burst was\n"
			<< "issued by "
			<< igrb->grb->trigger_instrument << ".\n\n"
			<< "Please check the VERITAS GRB webpage or\n"
			<< " the GCN burst information webpage for the\n"
			<< "appropriate instrument at:\n"
			<< "http://scipp.ucsc.edu/~stburst/veritas.html\n"
			<< "http://gcn.gsfc.nasa.gov/burst_info.html\n";
		      message.detailsStream() 
			<< "Trigger number: " 
			<< igrb->grb->trigger_gcn_sequence_number;
		      Messenger::relay()->sendMessage(message);
		      break;
		    }
		}
	    }

	  m_grb_last_processed = m_grb_list.begin();
	}
    }
}

void GUIGRBMonitor::update(const GUIUpdateData& ud)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if((ud.replay)||(isVisible()))m_last_ud = ud;
  if(ud.replay)updateTable();
  if(isVisible())
    {
      bool canchange = ((controlsEnabled())&&
			(ud.tse.req==TelescopeController::REQ_STOP)&&
			(ud.tse.state!=TelescopeController::TS_COM_FAILURE));
      m_selectbutton->setEnabled(canchange);
    }  
}

void GUIGRBMonitor::updateStatus()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  // --------------------------------------------------------------------------
  // Update the status box
  // --------------------------------------------------------------------------

  if(m_grb_connected_with_monitor)
    {
      m_grb_stat->setText("Connected");
      activateLE(true,m_grb_stat,color_fg_on,color_bg_on);

      QString uptime_txt;
      if(m_grb_server_uptime_sec<60)
	uptime_txt=QString::number(m_grb_server_uptime_sec)+" sec";
      else if(m_grb_server_uptime_sec<60*60)
	uptime_txt=QString::number(m_grb_server_uptime_sec/60)+" min "
	  + QString::number(m_grb_server_uptime_sec%60)+" sec";
      else if(m_grb_server_uptime_sec<24*60*60)
	uptime_txt=QString::number(m_grb_server_uptime_sec/3600)+" hr "
	  + QString::number(m_grb_server_uptime_sec/60 % 60)+" min "
	  + QString::number(m_grb_server_uptime_sec%60)+" sec";
      else
	uptime_txt=QString::number(m_grb_server_uptime_sec/86400)+" day "
	  + QString::number(m_grb_server_uptime_sec/3600 % 24)+" hr "
	  + QString::number(m_grb_server_uptime_sec/60 % 60)+" min";
      
      m_grb_uptime->setText(uptime_txt);
      m_grb_uptime->setEnabled(true);

      if(m_grb_gcn_connection_is_up)
	{
	  m_grb_gcn->setText("Connected");
	  activateLE(true,m_grb_gcn,color_fg_on,color_bg_on);
	}
      else
	{
	  m_grb_gcn->setText("Not connected");
	  activateLE(true,m_grb_gcn,color_fg_warn,color_bg_warn);
	}

      unsigned packet_time = m_grb_time_since_last_gcn_receipt_sec;
      QString packet_txt;
      if(packet_time<60)
	packet_txt=QString::number(packet_time)+" sec";
      else if(packet_time<60*60)
	packet_txt=QString::number(packet_time/60)+" min "
	  + QString::number(packet_time%60)+" sec";
      else if(packet_time<24*60*60)
	packet_txt=QString::number(packet_time/3600)+" hr "
	  + QString::number(packet_time/60 % 60)+" min "
	  + QString::number(packet_time%60)+" sec";
      else if(packet_time<365*24*60*60)
	packet_txt=QString::number(packet_time/86400)+" day "
	  + QString::number(packet_time/3600 % 24)+" hr "
	  + QString::number(packet_time/60 % 60)+" min";
      else
	packet_txt="never";

      m_grb_packet->setText(packet_txt);
      m_grb_packet->setEnabled(true);
    }
  else
    {
      m_grb_stat->setText("Not connected");
      activateLE(true,m_grb_stat,color_fg_warn,color_bg_warn);
      m_grb_uptime->setText("");
      m_grb_uptime->setEnabled(false);
      m_grb_gcn->setText("");
      activateLE(false,m_grb_gcn,color_fg_warn,color_bg_warn);
      //m_grb_gcn->setEnabled(false);
      m_grb_packet->setText("");
      m_grb_packet->setEnabled(false);
    }
}

void GUIGRBMonitor::updateTable()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  // --------------------------------------------------------------------------
  // Update the table
  // --------------------------------------------------------------------------

  VATime now;
  now.setFromMJDDbl(m_last_ud.mjd);

#if 0
  MoonObject sun;
  SphericalCoords sun_azel = 
    sun.getAzEl(m_last_ud.mjd, m_last_ud.lmst, m_earth_position);
  MoonObject moon;
  SphericalCoords moon_azel = 
    moon.getAzEl(m_last_ud.mjd, m_last_ud.lmst, m_earth_position);

  bool sun_moon_observe = 
    ( sun_azel.latitudeDeg() < -12 )&&( moon_azel.latitudeDeg() < 3 );
#else
  bool sun_moon_observe = true;
#endif

  unsigned ngrb = m_grb_list.size();
  m_grbtable->setNumRows(ngrb);
  GRBTriggerList::iterator ilist = m_grb_list.begin();
  for(unsigned igrb=0;igrb<ngrb;igrb++)
    {
      unsigned i = igrb;

      GRBTrigger* grb = ilist->grb;
      RaDecObject* obj = ilist->obj;
      SphericalCoords c = obj->coords();
      SphericalCoords azel = 
	obj->getAzEl(m_last_ud.mjd, m_last_ud.lmst, m_earth_position);

      VATime t(grb->trigger_time_mjd_int,
	       uint64_t(grb->trigger_msec_of_day_int)*UINT64_C(1000000));
      QString seq_no = QString::number(grb->trigger_gcn_sequence_number);

      QString age_txt = t.getAgeString(now);

      double age_hr = (m_last_ud.mjd-t.getMJDDbl())*24.0;
      bool observe = 
	sun_moon_observe && grb->veritas_should_observe
	&&(!ilist->retraction)
	&&(age_hr < grb->veritas_observation_window_hours)
	&&(azel.latitudeDeg()>MINEL);
      QString YESNO("NO");
      if(observe)YESNO = "YES";

      m_grbtable->setText(i,0,seq_no);
      m_grbtable->setText(i,1,QString(grb->trigger_instrument));
      m_grbtable->setText(i,2,QString(grb->trigger_type));
      m_grbtable->setText(i,3,t.getString().substr(0,23));
      m_grbtable->setText(i,4,c.longitude().hmsString(1));
      m_grbtable->setText(i,5,c.latitude().dmsPM180String(0));
      m_grbtable->setText(i,6,azel.latitude().degPM180String(1));
      m_grbtable->setText(i,7,age_txt);
      m_grbtable->setText(i,8,YESNO);
      m_grbtable->setRowStretchable(i,false);
      ilist++;
    }
  
  bool is_empty = m_grb_list.empty();
  unsigned visible_id = m_stack->id(m_stack->visibleWidget());

  if((is_empty)&&(visible_id==1))m_stack->raiseWidget(0);
  else if((!is_empty)&&(visible_id==0))m_stack->raiseWidget(1);
}

void GUIGRBMonitor::selectButtonPressed()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

#if 0
  if(m_selectbutton->isEnabled())
    {
      GRBTriggerList::iterator ilist = m_grb_list.begin();
      for(int iitem = 0; ilist != m_grb_list.end(); ilist++, iitem++)
	if(iitem == m_grbtable->currentRow())break;
      assert(ilist != m_grb_list.end());
      emit setGRB(ilist->grb->veritas_unique_sequence_number);
    }
#endif
  if(m_selectbutton->isEnabled())
    emit setGRB(m_grbtable->currentRow());
}

void GUIGRBMonitor::wizardButtonPressed()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  if(m_selectbutton->isEnabled())
    {
      GRBTriggerList::iterator ilist = m_grb_list.begin();
      for(int iitem = 0; ilist != m_grb_list.end(); ilist++, iitem++)
	if(iitem == m_grbtable->currentRow())break;
      assert(ilist != m_grb_list.end());
      emit recommendObservation(ilist->grb, ilist->obj);
    }
} 


// ----------------------------------------------------------------------------
// GRB notifications
// ----------------------------------------------------------------------------

GUIGRBMonitor::GRBNotification::~GRBNotification()
{
  // nothing to see here
}

void GUIGRBMonitor::GRBNotification::doNotification()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(!m_disavowed)m_monitor->doGRBNotification(this,m_update_table);
}

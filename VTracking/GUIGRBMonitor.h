//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUIGRBMonitor.h
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
 * $Revision: 2.9 $
 * $Tag$
 *
 **/

#ifndef VTRACKING_GUIGRBMONITOR_H
#define VTRACKING_GUIGRBMONITOR_H

#include <set>

#include <qwidget.h>
#include <qframe.h>
#include <qlineedit.h>
#include <qlabel.h>
#include <qwidgetstack.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>

#include <zthread/Thread.h>

#include <Notification.h>
#include <VOmniORBHelper.h>
#include <SphericalCoords.h>

#include "TargetObject.h"
#include "NET_VGRBMonitor.h"
#include "GUITabWidget.h"

class GUIGRBMonitor:
  public QFrame, public GUITabPane, 
  public VTaskNotification::PhaseLockedLoop
{
  Q_OBJECT
public:

  struct GRBTriggerListDatum
  {
    GRBTriggerListDatum(): grb(), obj(), retraction() { }
    GRBTriggerListDatum(GRBTrigger* _grb, VTracking::RaDecObject* _obj,
			GRBTrigger* _retraction = 0):
      grb(_grb), obj(_obj), retraction(_retraction) { }

    GRBTrigger*                   grb;
    VTracking::RaDecObject*       obj;
    GRBTrigger*                   retraction;
  };

  typedef std::list<GRBTriggerListDatum> GRBTriggerList;

  GUIGRBMonitor(VCorba::VOmniORBHelper* orb, 
		const SEphem::SphericalCoords& earth_position,
		QWidget* parent, const char* name);
  virtual ~GUIGRBMonitor();
  
  void startAcqisitionThread();
  void stopAcqisitionThread();

  virtual void update(const GUIUpdateData& ud);

  static std::string makeName(const GRBTrigger* grb);

  unsigned getGRBNumber(const GRBTrigger* grb);
  bool havePotentiallyObservableGRB();

signals:
  void setGRB(int);
  void reloadGRBList(const GUIGRBMonitor::GRBTriggerList&);
  void recommendObservation(const GRBTrigger*,const VTracking::RaDecObject*);

public slots:
  void updateStatus();
  void updateTable();  

protected:
  virtual void iterate();
  
private slots:
  void selectButtonPressed();
  void wizardButtonPressed();

private:

  class GRBNotification: public VTaskNotification::Notification
  {
  public:
    GRBNotification(GUIGRBMonitor* monitor, bool update_table):
      VTaskNotification::Notification(), 
      m_monitor(monitor), m_update_table(update_table), m_disavowed(false) { }
    virtual ~GRBNotification();
    virtual void doNotification();
    void disavow() { m_disavowed=true; }
  private:
    GUIGRBMonitor*  m_monitor;
    bool            m_update_table;
    bool            m_disavowed;
  };
  
  friend class GRBNotification;

  void doGRBNotification(GRBNotification* delivered_note, bool update_table);

  VCorba::VOmniORBHelper*     m_orb;
  SEphem::SphericalCoords     m_earth_position;
  ZThread::Thread*            m_thread;
  VGRBMonitor::Command_ptr    m_grb_monitor;

  GUIUpdateData               m_last_ud;

  bool                        m_grb_connected_with_monitor;
  CORBA::ULong                m_grb_last_id;
  GRBTriggerList              m_grb_list;
  GRBTriggerList::iterator    m_grb_last_processed;
  CORBA::Boolean              m_grb_gcn_connection_is_up;
  CORBA::ULong                m_grb_time_since_last_gcn_receipt_sec;
  CORBA::ULong                m_grb_server_uptime_sec;

  QLineEdit*                  m_grb_stat;
  QLineEdit*                  m_grb_uptime;
  QLineEdit*                  m_grb_gcn;
  QLineEdit*                  m_grb_packet;
  QWidgetStack*               m_stack;
  QTable*                     m_grbtable;
  //QComboBox*                  m_sortorder;
  //QComboBox*                  m_displaycriteria;
  QPushButton*                m_selectbutton;

  std::set<GRBNotification*>  m_undelivered_notes;
};

#endif // defined VTRACKING_GUIGRBMONITOR_H

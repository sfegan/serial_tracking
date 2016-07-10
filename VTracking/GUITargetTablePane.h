//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUITargetTablePane.h
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
 * $Date: 2007/07/05 20:14:00 $
 * $Revision: 2.5 $
 * $Tag$
 *
 **/

#ifndef VTRACKING_GUITARGETTABLEPANE_H
#define VTRACKING_GUITARGETTABLEPANE_H

#include<vector>

#include<qframe.h>
#include<qlineedit.h>
#include<qtable.h>
#include<qcombobox.h>
#include<qpushbutton.h>
#include<qlayout.h>
#include<qlabel.h>
#include<qwidgetstack.h>

#include<Angle.h>
#include<SphericalCoords.h>

#include"TelescopeController.h"
#include"TargetObject.h"
#include"GUITabWidget.h"
#include"GUIMisc.h"
#include"GUISummaryDetailsPane.h"

class TargetListItemInfo
{
public:
  TargetListItemInfo(): idx(), tli(), ra(), dec(), ha(), az(), el(), moon() { }
  int idx;
  const VTracking::TargetList::TargetListItem* tli;
  SEphem::Angle ra;
  SEphem::Angle dec;
  SEphem::Angle ha;
  SEphem::Angle az;
  SEphem::Angle el;
  SEphem::Angle moon;
};

class TLIIOrdering
{
public:
  TLIIOrdering(int ordering): m_ordering(ordering) { }
  bool operator() (const TargetListItemInfo& a, const TargetListItemInfo& b);
private:
  int m_ordering;
};

class GUITargetTablePane: public QWidgetStack, public GUITabPane
{
  Q_OBJECT

public:
  GUITargetTablePane(SEphem::SphericalCoords earth_position,
		     QWidget* parent=0, const char* name=0);
  ~GUITargetTablePane();

  virtual void update(const GUIUpdateData& ud);

public slots:
  void updateNow();
  void syncWithTargetList(const VTracking::TargetList& target_list);
  void clearTargetList();

signals:  
  void setTarget(int);
  void loadTargetList();

private slots:
  void selectButtonPressed();
  void loadTargetButtonPressed();

private:
  GUIUpdateData                        m_last_ud;
  SEphem::SphericalCoords              m_earth_position;

  VTracking::TargetList                m_targetlist;
  std::vector<TargetListItemInfo>      m_tlii;

  QTable*                              m_targettable;
  QComboBox*                           m_sortorder;
  QComboBox*                           m_displaycriteria;
  QPushButton*                         m_targetselectbutton;
}; // class GUITargetTablePane

#endif // VTRACKING_GUITARGETTABLEPANE_H

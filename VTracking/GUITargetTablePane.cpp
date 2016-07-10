//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUITargetTablePane.cpp
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
 * $Revision: 2.10 $
 * $Tag$
 *
 **/

#include<qhbox.h>
#include<qvbox.h>
#include<qlabel.h>

#include<Astro.h>

#include"GUITargetTablePane.h"
#include"TargetObject.h"

using namespace SEphem;
using namespace VTracking;

bool TLIIOrdering::operator() (const TargetListItemInfo& a, 
			       const TargetListItemInfo& b)
{
  switch(m_ordering)
    {
    case 0:
      return b.ha.radPM()<a.ha.radPM();

    case 1:
      return b.az.rad()>a.az.rad();

    case 2:
      return b.el.radPM180()<a.el.radPM180();

    case 3:
      return TargetList::TargetListItem::lt(a.tli->m_name,b.tli->m_name);

    default:
      return b.idx>a.idx;
    }
}

#define N(x) (sizeof(x)/sizeof(*x))

GUITargetTablePane::
GUITargetTablePane(SEphem::SphericalCoords earth_position,
		   QWidget* parent, const char* name):
  QWidgetStack(parent,name), GUITabPane(this),
  m_last_ud(), m_earth_position(earth_position), 
  m_targetlist(), m_tlii(0),
  m_targettable(), m_sortorder(), m_displaycriteria(), m_targetselectbutton()
{
  QFrame* targetframe = new QFrame(this,"target table frame");

  int colwidths[] = { -1, -1, 100, 100, 70, 100, 70, 70, 70 };
  QGridLayout* objectlayout = 
    new QGridLayout(targetframe,3,1,5,5,"target table layout");

  m_targettable = new MyQTable(targetframe,"target table");
  m_targettable->setNumCols(N(colwidths));
  m_targettable->setSelectionMode(QTable::SingleRow);
  m_targettable->setReadOnly(true);
  m_targettable->setShowGrid(false);
  for(unsigned i=0; i<N(colwidths); i++)
    {
      if(colwidths[i]<0)m_targettable->setColumnStretchable(i,true);
      else m_targettable->setColumnWidth(i,colwidths[i]);
    }
  
  QStringList tablelabels;
  tablelabels.append("Name");
  tablelabels.append("Description");
  tablelabels.append("RA");
  tablelabels.append("Dec");
  tablelabels.append("Epoch");
  tablelabels.append("-HA");
  tablelabels.append("El.");
  tablelabels.append("Az.");
  tablelabels.append("Moon");
  m_targettable->setColumnLabels(tablelabels);
  connect(m_targettable,SIGNAL(doubleClicked(int,int,int, const QPoint&)),
	  this,SLOT(selectButtonPressed()));

  QHBox* box=new QHBox(targetframe,"target table sort frame");
  box->setSpacing(5);
  new QLabel("Sort Order",box,"target table sort order label");
  m_sortorder = new QComboBox(false,box,"target table sort combobox");
  m_sortorder->setFocusPolicy(NoFocus);
  m_sortorder->insertItem("Hour Angle");
  m_sortorder->insertItem("Azimuth");
  m_sortorder->insertItem("Elevation");
  m_sortorder->insertItem("Source Name");
  m_sortorder->setCurrentItem(0);
  connect(m_sortorder,SIGNAL(activated(int)),this,SLOT(updateNow()));
  new QLabel("Display",box,"target table display label");
  m_displaycriteria = new QComboBox(false,box,"target table display combobox");
  m_displaycriteria->setFocusPolicy(NoFocus);
  m_displaycriteria->insertItem("All Targets");
  m_displaycriteria->insertItem(MAKEDEG("Targets at El>0"));
  m_displaycriteria->insertItem(MAKEDEG("Targets at El>15"));
  m_displaycriteria->insertItem(MAKEDEG("Targets at El>55"));
  m_displaycriteria->setCurrentItem(0);
  connect(m_displaycriteria,SIGNAL(activated(int)),this,SLOT(updateNow()));
  QPushButton* loadbutton = 
    new QPushButton("Load Targets",box,"target list load");
  connect(loadbutton,SIGNAL(clicked()),this,SLOT(loadTargetButtonPressed()));
  QPushButton* updatebutton = 
    new QPushButton("Update List",box,"target table update");
  connect(updatebutton,SIGNAL(clicked()),this,SLOT(updateNow()));
  box->setStretchFactor(m_sortorder,2);
  box->setStretchFactor(m_displaycriteria,2);
  box->setStretchFactor(updatebutton,1);
  box->setStretchFactor(loadbutton,1);

  m_targetselectbutton = 
    new QPushButton("Select Object",targetframe,"object but");
  connect(m_targetselectbutton,SIGNAL(clicked()),
	  this,SLOT(selectButtonPressed()));
  
  objectlayout->addWidget(m_targettable,0,0);
  objectlayout->addWidget(box,1,0);
  objectlayout->addWidget(m_targetselectbutton,2,0);
  objectlayout->setRowStretch(0,1);
  objectlayout->setRowStretch(1,0);
  objectlayout->setRowStretch(2,0);

  QVBox* box2=new QVBox(this,"no target box");
  QLabel* targetlabel =
    new QLabel("No targets loaded", box2, "no targets label");
  targetlabel->setAlignment(Qt::AlignCenter);
  QPushButton* loadbutton2 = 
    new QPushButton("Load Targets",box2,"target list load");
  connect(loadbutton2,SIGNAL(clicked()),this,SLOT(loadTargetButtonPressed()));
  
  addWidget(box2,0);
  addWidget(targetframe,1);
  raiseWidget(0);
}

GUITargetTablePane::~GUITargetTablePane()
{
  // nothing to see here
}

void GUITargetTablePane::selectButtonPressed()
{
  if(m_targetselectbutton->isEnabled())
    emit setTarget(m_tlii[m_targettable->currentRow()].idx);
}

void GUITargetTablePane::loadTargetButtonPressed()
{
  emit loadTargetList();
}

void GUITargetTablePane::update(const GUIUpdateData& ud)
{
  if((ud.replay)||(isVisible()))m_last_ud = ud;
  if(ud.replay)updateNow();
  if(isVisible())
    {
      bool canchange = ((controlsEnabled())&&
			(ud.tse.req==TelescopeController::REQ_STOP)&&
			(ud.tse.state!=TelescopeController::TS_COM_FAILURE));
      m_targetselectbutton->setEnabled(canchange);
    }
}

void GUITargetTablePane::
syncWithTargetList(const VTracking::TargetList& target_list)
{
  m_targetlist=target_list;
  updateNow();
}

void GUITargetTablePane::clearTargetList()
{
  m_targetlist.clear();
}

void GUITargetTablePane::updateNow()
{
  MoonObject moon;
  SphericalCoords moon_radec = 
    moon.getRaDec(m_last_ud.mjd,m_last_ud.lmst, m_earth_position);

  m_tlii.clear();
  for(unsigned i=0;i<m_targetlist.size();i++)
    {
      TargetListItemInfo tlii;
      SphericalCoords radec = 
	m_targetlist[i]->m_obj->
	getRaDec(m_last_ud.mjd,m_last_ud.lmst,m_earth_position);
      SphericalCoords azel = 
	m_targetlist[i]->m_obj->
	getAzEl(m_last_ud.mjd,m_last_ud.lmst,m_earth_position);
      tlii.idx = i;
      tlii.tli = m_targetlist[i];
      tlii.ra = radec.phi();
      tlii.dec = radec.latitude();
      tlii.ha = radec.phiRad() - m_last_ud.lmst.rad();
      tlii.az = azel.phi();
      tlii.el = azel.latitude();
      tlii.moon = radec.separation(moon_radec);

      if((m_displaycriteria->currentItem()==0)||
	 ((m_displaycriteria->currentItem()==1)&&(tlii.el.degPM180()>0))||
	 ((m_displaycriteria->currentItem()==2)&&(tlii.el.degPM180()>15))||
	 ((m_displaycriteria->currentItem()==3)&&(tlii.el.degPM180()>55)))
	m_tlii.push_back(tlii);
    }
  
  //VTracking::TelescopeController::StateElements se = m_controller->tse();
  std::sort(m_tlii.begin(), m_tlii.end(),
	    TLIIOrdering(m_sortorder->currentItem()));
  
  m_targettable->setNumRows(m_tlii.size());
  for(unsigned i=0;i<m_tlii.size();i++)
    {
      int idx = m_tlii[i].idx;
      SphericalCoords c = m_targetlist[idx]->m_obj->coords();
      double e = Astro::mjdToJulianEpoch(m_targetlist[idx]->m_obj->epoch());
      m_targettable->setText(i,0,m_targetlist[idx]->m_name);
      m_targettable->setText(i,1,m_targetlist[idx]->m_desc);
      m_targettable->setText(i,2,c.phi().hmsString(0));
      m_targettable->setText(i,3,c.latitude().dmsString(0));
      m_targettable->setText(i,4,QString::number(e,'f',1));
      m_targettable->setText(i,5,m_tlii[i].ha.hmsPMString(0));
      m_targettable->setText(i,6,m_tlii[i].el.degPM180String(1));
      m_targettable->setText(i,7,m_tlii[i].az.degString(1));
      m_targettable->setText(i,8,m_tlii[i].moon.degString(1));
      //m_targettable->setRowStretchable(i,false);
    }

  if((m_targetlist.size()>0)&&(id(visibleWidget())==0))raiseWidget(1);
  else if((m_targetlist.size()==0)&&(id(visibleWidget())==1))raiseWidget(0);
}

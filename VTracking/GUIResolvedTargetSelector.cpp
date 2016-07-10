//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUIResolvedTargetSelector.cpp
 * \ingroup VTracking
 * \brief Select one target out of list
 *
 * Original Author: Stephen Fegan
 * Start Date: 2007-04-06
 * $Author: sfegan $
 * $Date: 2007/04/09 16:57:10 $
 * $Revision: 2.2 $
 * $Tag$
 *
 **/

#include <Exception.h>
#include <Message.h>
#include <Messenger.h>
#include <Angle.h>
#include <SphericalCoords.h>
#include <Astro.h>

#include "GUIMisc.h"
#include "GUIResolvedTargetSelector.h"

using namespace SEphem;
//using namespace VTracking;
using namespace VMessaging;

GUIResolvedTargetSelector::Status GUIResolvedTargetSelector::
selectFromObjects(const VAstroDBResolver::ObjectInfoSeq& objects,
		  unsigned& selected, QWidget* parent, const char* name)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  
  GUIResolvedTargetSelector* dlg = 
    new GUIResolvedTargetSelector(objects, 
				  parent, name?name:__PRETTY_FUNCTION__, TRUE);
  int result = dlg->exec();

  switch(result)
    {
    case int(SELECTED):
      selected = dlg->tabObjects->currentRow();
      delete dlg;
      return SELECTED;
    case int(MANUAL):
      delete dlg;
      return MANUAL;
    default:
      delete dlg;
      return REJECTED;
    }

  delete dlg;
  return REJECTED;
}

GUIResolvedTargetSelector::~GUIResolvedTargetSelector()
{
  // nothing to see here
}

#define N(x) (sizeof(x)/sizeof(*x))

GUIResolvedTargetSelector::
GUIResolvedTargetSelector(const VAstroDBResolver::ObjectInfoSeq& objects,
			  QWidget* parent, const char* name, 
			  bool modal, WFlags fl):
  GUIResolvedTargetSelectorUI()
{
  unsigned nobject = objects.length();

  const char* fieldnames[] = { "Name","RA","Dec","Epoch","Type","Aliases" };
  tabObjects->setNumCols(N(fieldnames));
  tabObjects->setSelectionMode(QTable::SingleRow);
  tabObjects->setReadOnly(true);
  tabObjects->setShowGrid(false);
  QStringList tablelabels;
  for(unsigned icol=0; icol<N(fieldnames); icol++)
    {
      //if(colwidths[icol]<0)m_targettable->setColumnStretchable(icol,true);
      //else m_targettable->setColumnWidth(i,colwidths[icol]);
      tablelabels.append(fieldnames[icol]);
    }
  tabObjects->setColumnStretchable(5,true);

  tabObjects->setColumnLabels(tablelabels);
  connect(tabObjects,SIGNAL(doubleClicked(int,int,int, const QPoint&)),
	  this,SLOT(select()));
  connect(buttonSelect,SIGNAL(clicked()),this,SLOT(select()));
  connect(buttonManual,SIGNAL(clicked()),this,SLOT(manual()));
  
  tabObjects->setNumRows(nobject);
  for(unsigned irow=0;irow<nobject;irow++)
    {
      Angle ra(objects[irow].ra_rad);
      Angle dec(objects[irow].dec_rad);
      tabObjects->setText(irow,0,&*objects[irow].name);
      tabObjects->setText(irow,1,ra.hmsString(1));
      tabObjects->setText(irow,2,dec.dmsPM180String(1));
      tabObjects->setText(irow,3,QString::number(objects[irow].epoch_J));
      tabObjects->setText(irow,4,&*objects[irow].type);
      std::string aliases;
      unsigned nalias = objects[irow].aliases.length();
      for(unsigned ialias=0;ialias<nalias;ialias++)
	{
	  if(!aliases.empty())aliases+=",";
	  aliases+=objects[irow].aliases[ialias];
	}
      tabObjects->setText(irow,5,aliases);
    }
}

void GUIResolvedTargetSelector::select()
{
  done(int(SELECTED));
}

void GUIResolvedTargetSelector::manual()
{
  done(int(MANUAL));
}

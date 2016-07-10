//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUITargetAddDialog.cpp
 * \ingroup VTracking
 * \brief Add target to DB
 *
 * Original Author: Stephen Fegan
 * Start Date: 2007-04-03
 * $Author: sfegan $
 * $Date: 2007/09/19 05:39:04 $
 * $Revision: 2.9 $
 * $Tag$
 *
 **/

#include <set>
#include <qvalidator.h>
#include <qmessagebox.h>
#include <zthread/Guard.h>

#include <VDB/VDBArrayControl.h>

#include <Exception.h>
#include <Message.h>
#include <Messenger.h>
#include <Angle.h>
#include <SphericalCoords.h>
#include <Astro.h>

#include "Global.h"
#include "GUIMisc.h"
#include "GUITargetDialogs.h"
#include "GUITargetAddDialog.h"
#include "GUIResolvedTargetSelector.h"
#include "NET_VAstroDBResolver.h"

using namespace SEphem;
using namespace VTracking;
using namespace VMessaging;

void GUITargetAddDialog::addTargets(VCorba::VOmniORBHelper* orb,
				    QWidget* parent, const char* name)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  
  GUITargetAddDialog * dlg = 
    new GUITargetAddDialog(orb, parent, name?name:__PRETTY_FUNCTION__, TRUE);
  dlg->exec();
  delete dlg;
  return;
}

GUITargetAddDialog::
GUITargetAddDialog(VCorba::VOmniORBHelper* orb,
		   QWidget* parent, const char* name, bool modal, WFlags fl):
  GUITargetAddDialogUI(), m_orb(orb), m_collections()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  if(!m_orb)buttonSearch->setText("Validate object");

  QValidator* ra_val = new QRegExpValidator(raValidatorRE(), leRA, 
					    "target add dialog RA validator");
  leRA->setValidator(ra_val);

  QValidator* de_val = new QRegExpValidator(decValidatorRE(), leDec, 
					    "target add dialog Dec validator");
  leDec->setValidator(de_val);

  QValidator* epval = 
    new QRegExpValidator(epochValidatorRE(), leEpoch,
			 "target add dialog epoch validator");
  leEpoch->setValidator(epval);

  connect(buttonSearch,SIGNAL(clicked()),this,SLOT(searchForObject()));
  connect(buttonReset,SIGNAL(clicked()),this,SLOT(reset()));
  connect(buttonAdd,SIGNAL(clicked()),this,SLOT(add()));
  connect(leSource,SIGNAL(textChanged(const QString&)),
	  this,SLOT(sourceTextChanged(const QString&)));
  connect(leRA,SIGNAL(textChanged(const QString&)),
	  this,SLOT(positionTextChanged(const QString&)));
  connect(leDec,SIGNAL(textChanged(const QString&)),
	  this,SLOT(positionTextChanged(const QString&)));
  connect(leEpoch,SIGNAL(textChanged(const QString&)),
	  this,SLOT(positionTextChanged(const QString&)));

  cbColl->insertItem("<none>");
  m_collections.push_back("<none>");

  try
    {
      std::set<std::string> protectedCollection =
	GUITargetDialogs::getProtectedCollections();

      ZThread::Guard<ZThread::RecursiveMutex>
	guard(Global::instance()->dbMutex());

      std::vector<std::string> collections = VDBAC::getCollectionNames();

      for(unsigned icollection=0;icollection<collections.size();icollection++)
	{
	  std::string collection = collections[icollection];
	  if(protectedCollection.find(collection) != 
	     protectedCollection.end())continue;

	  VDBAC::CollectionInfo cinfo = VDBAC::getCollectionInfo(collection);
	  std::string comment = cinfo.comment;
	  if(!comment.empty())
	    {
	      cbColl->insertItem(comment);
	      m_collections.push_back(collection);
	    }
	}
    }
  catch(const VDBException& x)
    {
      Message message(Message::DR_GLOBAL,Message::PS_EXCEPTIONAL,
		      "VDB exception");
      message.messageStream()
	<< "A database exception was thrown and caught. The list\n"
	<< "of target collections could not be loaded from the\n"
	<< "database";
      message.detailsStream() << x;
      Messenger::relay()->sendMessage(message);
    }

  reset();
}

void GUITargetAddDialog::searchForObject()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  // Make sure object is not already in the database

  try
    {
      ZThread::Guard<ZThread::RecursiveMutex>
	guard(Global::instance()->dbMutex());

      VDBAC::getSourceInfo(leSource->text());
      
      if(QMessageBox::question(this,"Duplicate object",
			       "Object is already in the database",
			       "Try another name", "Abort input", 
			       QString::null, 0, 0) == 1)
	done(QDialog::Rejected);
      return;
    }
  catch(const VDBResultSetNotReturnedException& x)
    {
      // target not found
    }
  catch(const VDBException& x)
    {
      // should be an error but above does not work :-(
#if 0
      Message message(Message::DR_GLOBAL,Message::PS_EXCEPTIONAL,
		      "VDB exception");
      message.messageStream()
	<< "A database exception was thrown and caught while\n"
	<< "loading target information from the database.";
      message.detailsStream() << x;
      Messenger::relay()->sendMessage(message);
      return false;
#endif
    }

  // Search for object in SIMBAD
  
  bool set_fields = false;
  if(m_orb)
    {
      try
	{
	  VAstroDBResolver::Command_var resolver =
	    m_orb->nsGetNarrowedObject<VAstroDBResolver::Command>
	    (VAstroDBResolver::progName, VAstroDBResolver::Command::objName);
  
	  m_orb->setObjectClientTimeout(resolver,5000);

	  VAstroDBResolver::ObjectInfoSeq_var object_info =
	    resolver->resolve(leSource->text());	  
	  
	  if(object_info->length() > 0)
	    {
	      GUIResolvedTargetSelector::Status status = 
		GUIResolvedTargetSelector::SELECTED;
	      unsigned iobj = 0;

	      if(object_info->length() > 1)
		status = 
		  GUIResolvedTargetSelector::
		  selectFromObjects(object_info, iobj,
				    this, "resolved selector");
	      
	      if(status == GUIResolvedTargetSelector::SELECTED)
		{
		  Angle ra(object_info[iobj].ra_rad);
		  Angle dec(object_info[iobj].dec_rad);
		  leRA->setText(ra.hmsString(1));
		  leDec->setText(dec.dmsPM180String(1));
		  leEpoch->setText(QString::number(object_info[iobj].epoch_J));
		  QString desc = &(*object_info[iobj].type);
		  if(object_info[iobj].aliases.length()>0)
		    { 
		      desc += " "; 
		      desc += object_info[iobj].aliases[0];
		    }
		  leDesc->setText(desc);
		  set_fields = true;
		}
	      else if(status != GUIResolvedTargetSelector::MANUAL)
		return;
	    }
	  else
	    {
	      if(QMessageBox::
		 question(this,"Object not found",
			  "Object could not be found in SIMBAD",
			  "Try another name", "Continue anyway", 
			  QString::null, 0, 0) == 0)
		return;
	    }
	}
      catch(const VAstroDBResolver::ResolveFailed& x)
	{
	  if(QMessageBox::
	     question(this,"Resolver failed",
		      "Object resolver failed to connect to the database",
		      "Try another name", "Continue anyway", 
		      QString::null, 0, 0) == 0)
	    return;
	}
      catch(const CORBA::Exception& x)
	{
	  if(QMessageBox::
	     question(this,"CORBA error",
		      "Error trying to connect to object resolver",
		      "Try another name", "Continue anyway", 
		      QString::null, 0, 0) == 0)
	    return;
	}
    }


  if(!set_fields)
    {
      leRA->setText("");
      leDec->setText("");
      leEpoch->setText("");
      leDesc->setText("");
    }
  cbColl->setCurrentItem(0);

  leSource->setEnabled(false);
  buttonSearch->setEnabled(false);
  
  leRA->setEnabled(true);
  leDec->setEnabled(true);
  leEpoch->setEnabled(true);
  leDesc->setEnabled(true);
  cbColl->setEnabled(true);

  leRA->setFocus();
}

void GUITargetAddDialog::reset()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  leSource->setText("");
  leRA->setText("HH:MM:SS.S");
  leDec->setText("+/-DD:MM:SS.S");
  leEpoch->setText("YYYY.YY");
  leDesc->setText("Source class and proper SIMBAD name");
  cbColl->setCurrentItem(0);

  leSource->setEnabled(true);
  leRA->setEnabled(false);
  leDec->setEnabled(false);
  leEpoch->setEnabled(false);
  leDesc->setEnabled(false);
  cbColl->setEnabled(false);

  buttonSearch->setEnabled(false);
  buttonAdd->setEnabled(false);
  buttonFinished->setEnabled(true);
}

void GUITargetAddDialog::add()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  Angle ra;
  Angle dec;
  double epoch;
  bool valid;

  ra.setFromHMSString(leRA->text());
  dec.setFromDMSString(leDec->text());
  epoch = leEpoch->text().toDouble(&valid);

  SphericalCoords radec = SphericalCoords::makeLatLong(dec,ra);

  try
    {
      ZThread::Guard<ZThread::RecursiveMutex>
	guard(Global::instance()->dbMutex());

      std::vector<std::string> sources = VDBAC::getSourceNames("all");
      if(!sources.empty())
	{
	  std::string names;

	  unsigned nsource = sources.size();
	  for(unsigned isource=0;isource<nsource;isource++)
	    {
	      struct VDBAC::SourceInfo info;
	      info = VDBAC::getSourceInfo(sources[isource]);

	      if(info.epoch < 1900)continue;

	      SphericalCoords c =
		SphericalCoords::makeLatLong(info.decl,info.ra);
	      if(info.epoch != epoch)
		Astro::precess(Astro::julianEpochToMJD(epoch), c, 
			       Astro::julianEpochToMJD(info.epoch));

	      Angle sep = c.separation(radec);
	      if(sep.deg() < 0.1)
		{
		  if(!names.empty())names += ", ";
		  names+=info.source_id;
		}
	    }

	  if(!names.empty())
	    {
	      if(QMessageBox::
		 question(this,"Duplicate object",
			  QString("Another object has very similar "
				  "coordinates: ")+names.c_str(),
			  "Cancel add to DB", "Add object anyway",
			  QString::null, 0, 0) == 0)
		return;
	    }
	}
    }
  catch(const VDBResultSetNotReturnedException& x)
    {
      //
    }
  catch(const VDBException& x)
    {
      //
    }
  
  try
    {
      ZThread::Guard<ZThread::RecursiveMutex>
	guard(Global::instance()->dbMutex());
      VDBAC::putSourceInfo(leSource->text(), ra.rad(), dec.radPM(), epoch,
			   leDesc->text());

      if(cbColl->currentItem() > 0)
	VDBAC::putSourceinCollection(leSource->text(),
				     m_collections[cbColl->currentItem()]);
    }
  catch(const VDBException& x)
    {
      Message message(Message::DR_GLOBAL,Message::PS_EXCEPTIONAL,
		      "VDB exception");
      message.messageStream()
	<< "A database exception was thrown and caught while\n"
	<< "loading target information from the database.";
      message.detailsStream() << x;
      Messenger::relay()->sendMessage(message);
      
      return;
    }

  reset();
}

void GUITargetAddDialog::sourceTextChanged(const QString& txt)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  buttonSearch->setEnabled(!txt.isEmpty());
  buttonFinished->setEnabled(txt.isEmpty());
}

void GUITargetAddDialog::positionTextChanged(const QString& txt)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  buttonAdd->setEnabled(leRA->hasAcceptableInput() 
			&& leDec->hasAcceptableInput() 
			&& leEpoch->hasAcceptableInput());
}

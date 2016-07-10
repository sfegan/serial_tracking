//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUITargetDialogs.cpp
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
 * $Date: 2007/09/17 16:50:18 $
 * $Revision: 2.7 $
 * $Tag$
 *
 **/

#include<qfiledialog.h>
#include<qmessagebox.h>
#include<zthread/Guard.h>

#include<VDB/VDBArrayControl.h>

#include<Exception.h>
#include<Message.h>
#include<Messenger.h>
#include<Debug.h>
#include<Astro.h>

#include"Global.h"
#include"GUITargetDialogs.h"
#include"GUITargetCollectionDialog.h"

using namespace SEphem;
using namespace VTracking;
using namespace VMessaging;

std::set<std::string> GUITargetDialogs::getProtectedCollections()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  std::set<std::string> protectedCollection;

  // Default protected collections -- use if DB fails below
  protectedCollection.insert("all");
  protectedCollection.insert("blank_sky");
  protectedCollection.insert("GRB");
  protectedCollection.insert("survey_cygnus");
  protectedCollection.insert("mgro1908_survey");
  protectedCollection.insert("yale_bright_star");
  protectedCollection.insert("yale_bright_star_3.0");
  protectedCollection.insert("yale_bright_star_5.0");

  try
    {
      ZThread::Guard<ZThread::RecursiveMutex>
        guard(Global::instance()->dbMutex());

      std::vector<std::string> collections;
      collections = VDBAC::getCollectionNames();
      protectedCollection.clear();
      for(unsigned icollection=0;icollection<collections.size();icollection++)
	{
	  std::string collection = collections[icollection];
	  VDBAC::CollectionInfo cinfo = VDBAC::getCollectionInfo(collection);
	  if(cinfo.locked)protectedCollection.insert(collection);
	}
    }
  catch(const VDBResultSetNotReturnedException& x)
    {
      Message message(Message::DR_GLOBAL,Message::PS_UNUSUAL,
                      "No collections found");
      message.messageStream()
        << "No target collections found\n";
      message.detailsStream() << x;
      Messenger::relay()->sendMessage(message);
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
    }

  return protectedCollection;
}

bool GUITargetDialogs::
loadDefault(VTracking::TargetList& list, bool use_messenger_only)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  return loadFromDBCollection(getDefaultDBCollectionName(),list,
			      use_messenger_only);
  //return loadFromFile(getDefaultFileName(),list);
}

bool GUITargetDialogs::
loadFromDBCollection(VTracking::TargetList& list, 
		     bool use_messenger_only,
		     QWidget* parent, const char* name)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  std::string collection =
    GUITargetCollectionDialog::getCollection(getDefaultDBCollectionName(),
					     parent, name);
  
  if(collection.empty())return false;
  else if(collection=="<file>")return loadFromFile(list, parent, name);
  else return loadFromDBCollection(collection, list, use_messenger_only,
				   parent, name);
}

bool GUITargetDialogs::
loadFromDBCollection(const std::string& collection,
		     VTracking::TargetList& list,
		     bool use_messenger_only,
		     QWidget* parent, const char* name)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  
  try
    {
      ZThread::Guard<ZThread::RecursiveMutex>
	guard(Global::instance()->dbMutex());

      std::vector<std::string> sources = VDBAC::getSourceNames(collection);
      if(sources.empty())return false;

      unsigned nsource = sources.size();
      list.clear();
      for(unsigned isource=0;isource<nsource;isource++)
	{
	  struct VDBAC::SourceInfo info;
	  info = VDBAC::getSourceInfo(sources[isource]);

	  if(info.epoch < 1900)continue;

	  RaDecObject* obj = 
	    new RaDecObject(SphericalCoords::makeLatLong(info.decl,info.ra),
			    Astro::julianEpochToMJD(info.epoch),
			    info.source_id);

	  list.addTarget(info.source_id,info.description,obj,0);
	}
  
      list.sortTargets();
    }
  catch(const VDBResultSetNotReturnedException& x)
    {
      if(!use_messenger_only)
	{
	  QMessageBox::warning(parent,"No targets found",
			       QString("No targets found in collection: ")
			       + collection,
			       QMessageBox::Ok,QMessageBox::NoButton);
	}
      else
	{
	  Message message(Message::DR_GLOBAL,Message::PS_UNUSUAL,
			  "No targets found");
	  message.messageStream()
	    << "No targets found in collection: " << collection;
	  message.detailsStream() << x;
	  Messenger::relay()->sendMessage(message);
	}
      return false;      
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
      return false;
    }

  return true;
}
  
bool GUITargetDialogs::loadFromFile(VTracking::TargetList& list,
				    QWidget* parent, const char* name)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  QString fn = 
    QFileDialog::getOpenFileName(getDefaultFileName(),
				 "Tracking Files (*.trk);;All Files (*)",
				 parent, name, "Select target list");

  if(fn.isEmpty())return false;
  else return loadFromFile(fn,list);
}

bool GUITargetDialogs::loadFromFile(const std::string& file,
				    VTracking::TargetList& list)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  list.loadFromFile(file);
  return true;
}

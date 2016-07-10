//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUITargetCollectionManagerDialog.cpp
 * \ingroup VTracking
 * \brief Target collection manager selector
 *
 * Original Author: Stephen Fegan
 * Start Date: 2007-07-04
 * $Author: sfegan $
 * $Date: 2007/07/20 01:42:47 $
 * $Revision: 2.3 $
 * $Tag$
 *
 **/

#include<vector>

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
#include"GUITargetCollectionManagerDialog.h"

using namespace VTracking;
using namespace VMessaging;

void GUITargetCollectionManagerDialog::
manageTargets(const std::string& default_collection,
	      QWidget* parent, const char* name, 
	      const QString& caption)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  GUITargetCollectionManagerDialog * dlg = 
    new GUITargetCollectionManagerDialog(default_collection, 
				  parent, name?name:__PRETTY_FUNCTION__, TRUE);

  if(caption.isNull())dlg->setCaption("Modify targets in collection");
  else dlg->setCaption(caption);

  dlg->exec();

  delete dlg;
  return;
}

GUITargetCollectionManagerDialog::
GUITargetCollectionManagerDialog(const std::string& default_collection,
				 QWidget* parent, const char* name, 
				 bool modal, WFlags fl):
  GUITargetCollectionManagerDialogUI(parent, name, modal, fl),
  m_source_collections(), m_source_items(),
  m_target_collections(), m_target_items()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  std::set<std::string> protected_collections =
    GUITargetDialogs::getProtectedCollections();

  std::vector<std::string> collections;
  try
    {
      ZThread::Guard<ZThread::RecursiveMutex>
        guard(Global::instance()->dbMutex());
      collections = VDBAC::getCollectionNames();
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

  unsigned src_default = 0;
  unsigned tar_default = 0;
  for(unsigned icollection=0;icollection<collections.size();icollection++)
    {
      std::string collection = collections[icollection];
      VDBAC::CollectionInfo cinfo = VDBAC::getCollectionInfo(collection);
      std::string comment = cinfo.comment;
      if(!comment.empty())
	{
	  if(collection == "all")
	    {
	      if(collection == "all")
		src_default = comboSrc->count();
	      comboSrc->insertItem(comment);
	      m_source_collections.push_back(collection);
	    }
	  else if(protected_collections.find(collection) ==
		  protected_collections.end())
	    {
	      if(collection == default_collection)
		tar_default = comboTar->count();
	      comboTar->insertItem(comment);
	      m_target_collections.push_back(collection);
	    }
	}
    }

  comboSrc->setEnabled(false);

  connect(comboSrc,SIGNAL(activated(int)), 
	  this,SLOT(sourceCollectionChanged()));
  connect(comboTar,SIGNAL(activated(int)), 
	  this,SLOT(targetCollectionChanged()));
  connect(pushButtonTo,SIGNAL(clicked()),
	  this,SLOT(addButtonPressed()));
  connect(listSrc,SIGNAL(selected(int)),
	  this,SLOT(addButtonPressed()));
  connect(pushButtonFrom,SIGNAL(clicked()),
	  this,SLOT(removeButtonPressed()));
  connect(listTar,SIGNAL(selected(int)),
	  this,SLOT(removeButtonPressed()));

  sourceCollectionChanged();
  targetCollectionChanged();
}
  
void GUITargetCollectionManagerDialog::addButtonPressed()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if((m_target_collections.empty())||(listSrc->currentItem()==-1))
    return;

  std::string collection = m_target_collections[comboTar->currentItem()];
  std::string target = listSrc->text(listSrc->currentItem());
  try
    {
      ZThread::Guard<ZThread::RecursiveMutex>
        guard(Global::instance()->dbMutex());
      VDBAC::putSourceinCollection(target, collection);
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

  targetCollectionChanged();
}

void GUITargetCollectionManagerDialog::removeButtonPressed()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if((m_target_collections.empty())||(listTar->currentItem()==-1))
    return;

  std::string collection = m_target_collections[comboTar->currentItem()];
  std::string target = listTar->text(listTar->currentItem());
  try
    {
      ZThread::Guard<ZThread::RecursiveMutex>
        guard(Global::instance()->dbMutex());
      VDBAC::deleteSourcefromCollection(target, collection);
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

  int ci = listTar->currentItem();
  targetCollectionChanged();
  if(ci < (int)listTar->count())listTar->setCurrentItem(ci);
  else if(listTar->count() > 0)listTar->setCurrentItem(listTar->count()-1);
}

void GUITargetCollectionManagerDialog::sourceCollectionChanged()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(m_source_collections.empty())return;

  std::string collection = m_source_collections[comboSrc->currentItem()];
  
  std::vector<std::string> targets;
  try
    {
      ZThread::Guard<ZThread::RecursiveMutex>
        guard(Global::instance()->dbMutex());
      targets = VDBAC::getSourceNames(collection);
    }
  catch(const VDBResultSetNotReturnedException& x)
    { 
      QMessageBox::warning(0,"No targets found",
			   QString("No targets found in collection: ")
			   + collection,
			   QMessageBox::Ok,QMessageBox::NoButton);
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

  m_source_items.clear();
  listSrc->clear();

  if(targets.empty())return;

  std::sort(targets.begin(),targets.end());
  for(std::vector<std::string>::iterator itarget=targets.begin();
      itarget!=targets.end();itarget++)
    {
      int nitem = listSrc->count();
      m_source_items[*itarget] = nitem;
      listSrc->insertItem(*itarget,nitem);
      if(m_target_items.find(*itarget) != m_target_items.end())
	listSrc->item(nitem)->setSelectable(false);
    }
}

void GUITargetCollectionManagerDialog::targetCollectionChanged()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(m_target_collections.empty())return;

  std::string collection = m_target_collections[comboTar->currentItem()];
  
  std::vector<std::string> targets;
  try
    {
      ZThread::Guard<ZThread::RecursiveMutex>
        guard(Global::instance()->dbMutex());
      targets = VDBAC::getSourceNames(collection);
    }
  catch(const VDBResultSetNotReturnedException& x)
    { 
#if 0
      QMessageBox::warning(0,"No targets found",
			   QString("No targets found in collection: ")
			   + collection,
			   QMessageBox::Ok,QMessageBox::NoButton);
#endif
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

  m_target_items.clear();
  listTar->clear();

  if(!targets.empty())
    {
      std::sort(targets.begin(),targets.end());
      for(std::vector<std::string>::iterator itarget=targets.begin();
	  itarget!=targets.end();itarget++)
	{
	  int nitem = listTar->count();
	  m_target_items[*itarget] = nitem;
	  listTar->insertItem(*itarget,nitem);
	}
    }

  for(std::map<std::string,int>::iterator iitem=m_source_items.begin();
      iitem!=m_source_items.end();iitem++)
    {
      if(m_target_items.find(iitem->first) != m_target_items.end())
	listSrc->item(iitem->second)->setSelectable(false);
      else if(!listSrc->item(iitem->second)->isSelectable())
	listSrc->item(iitem->second)->setSelectable(true);
    }
}

//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUITargetCollectionDialog.cpp
 * \ingroup VTracking
 * \brief Target collection selector
 *
 * Original Author: Stephen Fegan
 * Start Date: 2007-04-03
 * $Author: sfegan $
 * $Date: 2007/04/06 23:46:15 $
 * $Revision: 2.2 $
 * $Tag$
 *
 **/

#include <qlistbox.h>
#include <zthread/Guard.h>

#include <VDB/VDBArrayControl.h>

#include <Exception.h>
#include <Message.h>
#include <Messenger.h>

#include "Global.h"
#include "GUITargetCollectionDialog.h"

using namespace VTracking;
using namespace VMessaging;

GUITargetCollectionDialog::
GUITargetCollectionDialog(const std::string& default_collection,
			  QWidget* parent, const char* name, 
			  bool modal, WFlags fl):
  GUITargetCollectionDialogUI(parent, name, modal, fl), m_collections()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  boxCollections->insertItem("<load from file>");
  m_collections.push_back("<file>");

  try
    {
      ZThread::Guard<ZThread::RecursiveMutex>
	guard(Global::instance()->dbMutex());

      std::vector<std::string> collections = VDBAC::getCollectionNames();

      unsigned idefault = 0;
      for(unsigned icollection=0;icollection<collections.size();icollection++)
	{
	  std::string collection = collections[icollection];
	  VDBAC::CollectionInfo cinfo = VDBAC::getCollectionInfo(collection);
	  std::string comment = cinfo.comment;
	  if(!comment.empty())
	    {
	      if(collection == default_collection)
		idefault = boxCollections->numRows();
	      boxCollections->insertItem(comment);
	      m_collections.push_back(collection);
	    }
	}
      boxCollections->setCurrentItem(idefault);
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
}

std::string GUITargetCollectionDialog::selected()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  if(boxCollections->currentItem()>=0)
    return m_collections[boxCollections->currentItem()];
  return std::string();
}

std::string GUITargetCollectionDialog::
getCollection(const std::string& default_collection,
	      QWidget *parent, const char* name, const QString& caption)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  GUITargetCollectionDialog * dlg = 
    new GUITargetCollectionDialog(default_collection, 
				  parent, name?name:__PRETTY_FUNCTION__, TRUE);

  if(caption.isNull())dlg->setCaption("Select collection");
  else dlg->setCaption(caption);

  if(dlg->exec() == QDialog::Accepted)
    {
      std::string collection = dlg->selected();
      delete dlg;
      return collection;
    }

  delete dlg;
  return std::string();
}

//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUICorrectionDateDialog.cpp
 * \ingroup VTracking
 * \brief Target collection selector
 *
 * Original Author: Stephen Fegan
 * Start Date: 2007-04-03
 * $Author: sfegan $
 * $Date: 2007/07/19 15:11:13 $
 * $Revision: 2.2 $
 * $Tag$
 *
 **/

#include <qlistbox.h>
#include <qlabel.h>
#include <zthread/Guard.h>

#include <VDB/VDBPositioner.h>

#include <Exception.h>
#include <Message.h>
#include <Messenger.h>

#include "Global.h"
#include "GUICorrectionDateDialog.h"

using namespace VERITAS;
using namespace VTracking;
using namespace VMessaging;

GUICorrectionDateDialog::
GUICorrectionDateDialog(unsigned scope_id,
			QWidget* parent, const char* name, 
			bool modal, WFlags fl):
  GUITargetCollectionDialogUI(parent, name, modal, fl), m_dates()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  boxCollections->insertItem("<load from file>");
  m_dates.push_back(loadFromFileTime());

  try
    {
      ZThread::Guard<ZThread::RecursiveMutex>
	guard(Global::instance()->dbMutex());

      std::vector<VDBdata::Datetime> db_dates = 
	VDBPOS::getCorrectionDates(scope_id);
      
      std::vector<VATime> va_dates;
      for(std::vector<VDBdata::Datetime>::const_iterator id = db_dates.begin();
	  id != db_dates.end(); id++)
	{
	  VATime t;
	  t.setFromDBTimeStamp(id->datetime);
	  va_dates.push_back(t);
	}
      std::sort(va_dates.begin(), va_dates.end());

      m_dates.resize(va_dates.size()+1);
      for(unsigned id = 0; id<va_dates.size(); id++)
	m_dates[id+1] = va_dates[va_dates.size()-1-id];

      for(unsigned id = 1; id<m_dates.size(); id++)
	{
	  std::string t = m_dates[id].toString().substr(0,19);
	  if(id==1)t+=std::string(" [current]");
	  boxCollections->insertItem(t);
	}
	 
      if(m_dates.size()>1)
	boxCollections->setCurrentItem(1);
      else
	boxCollections->setCurrentItem(0);
    }
  catch(const VDBException& x)
    {
      Message message(Message::DR_GLOBAL,Message::PS_EXCEPTIONAL,
		      "VDB exception");
      message.messageStream()
	<< "A database exception was thrown and caught. The list\n"
	<< "of corrections could not be loaded for scope: "
	<< scope_id+1;
      message.detailsStream() << x;
      Messenger::relay()->sendMessage(message);
    }

  QFont f = headerLabel->font();
  headerLabel->setText("Select corrections to load");
  headerLabel->setFont(f);
}

const VATime GUICorrectionDateDialog::selected() const
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  if(boxCollections->currentItem()>=0)
    return m_dates[boxCollections->currentItem()];
  return VATime();
}

VATime GUICorrectionDateDialog::
getCorrectionDate(unsigned scope_id,
		  QWidget *parent, const char* name, const QString& caption)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  GUICorrectionDateDialog * dlg = 
    new GUICorrectionDateDialog(scope_id,
				parent, name?name:__PRETTY_FUNCTION__, TRUE);

  if(caption.isNull())dlg->setCaption("Select corrections");
  else dlg->setCaption(caption);

  if(dlg->exec() == QDialog::Accepted)
    {
      VATime date = dlg->selected();
      delete dlg;
      return date;
    }
  else
    {
      delete dlg;
      return cancelLoadTime();
    }

  delete dlg;
  return VATime();
}

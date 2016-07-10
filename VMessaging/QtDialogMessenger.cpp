//-*-mode:c++; mode:font-lock;-*-

/**
 * \file QtDialogMessenger.cpp
 * \ingroup VMessaging
 * \brief This is a one-line description of this cpp file.
 *
 * Here is a tedious verbose multi-line description of all
 * the details of the code, more than you would
 * ever want to read. Generally, all the important documentation
 * goes in the .cpp files.
 *
 * Original Author: Stephen Fegan
 * $Author: sfegan $
 * $Date: 2007/01/23 01:36:05 $
 * $Revision: 2.1 $
 * $Tag$
 *
 **/

#include<iostream>

#include<qlayout.h>
#include<qhbox.h>
#include<qvbox.h>
#include<qpixmap.h>
#include<qmessagebox.h>
#include<qlabel.h>
#include<qpushbutton.h>
#include<qtabwidget.h>
#include<qtextedit.h>

#include"QtDialogMessenger.h"

using namespace VMessaging;

QtDialogMessenger::~QtDialogMessenger() throw()
{
  // nothing to see here
}

static bool operator< (const struct timeval& a, const struct timeval& b)
{
  if((a.tv_sec<b.tv_sec)||((a.tv_sec==b.tv_sec)&&(a.tv_usec<b.tv_usec)))
    return true;
  return false;
}

bool QtDialogMessenger::sendMessage(const Message& message) throw()
{
  Message::PayloadSignificance significance = message.significance();

  // Test whether to bother the user with a DialogBox or not
  if((significance==Message::PS_ROUTINE)&&
     ((m_min_sig==Message::PS_UNUSUAL)||(m_min_sig==Message::PS_EXCEPTIONAL)||
      (m_min_sig==Message::PS_CRITICAL)))return true;

  if((significance==Message::PS_UNUSUAL)&&
     ((m_min_sig==Message::PS_EXCEPTIONAL)||(m_min_sig==Message::PS_CRITICAL)))
    return true;
  
  if((significance==Message::PS_EXCEPTIONAL)&&
     (m_min_sig==Message::PS_CRITICAL))return true;

  std::string pro = 
    message.program()+std::string(" (")+message.hostname()+std::string(")");

  BlockKey key(pro, message.title());
  BlockDetails& block(m_blocks[key]);

  struct timeval now;
  gettimeofday(&now,0);

  if((significance!=Message::PS_CRITICAL)&&(now<block.next))return true;

  QtMessageDialog* dialog = 
    new QtMessageDialog(message,block,m_parent,"Dialog");
  dialog->exec();
  delete dialog;

  return true;
}

bool BlockKey::operator< (const BlockKey& o) const
{
  if((program<o.program)||
     ((program==o.program)&&(title<o.title)))return true;
  else return false;
}

QtMessageDialog::
QtMessageDialog(const Message& message, BlockDetails& block,
		QWidget* parent, const char* name):
  QDialog(parent,name), 
  m_block(block), m_block_cb(0), m_block_duration_le(0), m_expand(0)
{ 
  setOrientation(Qt::Vertical);

  std::ostringstream or_str;
  switch(message.origin())
    {
    case Message::MO_LOCAL: 
      or_str << "LOCAL";
      break;
    case Message::MO_REMOTE: 
      or_str << "REMOTE (" << message.zone() << ')';
      break;
    }

  std::string msg = message.message();
  std::string det = message.details();
  std::string fun = message.function();
  std::string pro = 
    message.program()+std::string(" (")+message.hostname()+std::string(")");
  bool zero_time = (message.time().tv_sec==0)&&(message.time().tv_usec==0);
  
  bool can_block = (message.significance()!=Message::PS_CRITICAL);
  bool have_details = (!zero_time)||(pro!="")||(det!="")||(fun!="");
  
  QGridLayout* layout=
    new QGridLayout(this,3+(can_block?1:0),2,10,5,
		    "QDM Layout");
  
  QMessageBox::Icon icon;
  switch(message.significance())
    {
    case Message::PS_ROUTINE:     icon=QMessageBox::Information; break;
    case Message::PS_UNUSUAL:     icon=QMessageBox::Information; break;
    case Message::PS_EXCEPTIONAL: icon=QMessageBox::Warning;     break;
    case Message::PS_CRITICAL:    icon=QMessageBox::Critical;    break;
    }

  QLabel* pix_lab = new QLabel(this,"QDM Pix Lab");
  pix_lab->setPixmap(QMessageBox::standardIcon(icon));
  layout->addWidget(pix_lab,0,0);
  
  QVBox* message_box = new QVBox(this,"QDM message box");
  message_box->setSpacing(5);
  QLabel* title_lab = 
    new QLabel(message.title().c_str(),message_box,"QDM title lab");
  QFont f=title_lab->font();
  f.setPointSize(f.pointSize()*12/10);
  f.setBold(true);
  title_lab->setFont(f);

  if(msg != "")
    new QLabel(msg.c_str(),message_box,"QDM message lab");

  layout->addMultiCellWidget(message_box,0,1,1,1);
  layout->setRowStretch(0,0);
  layout->setRowStretch(1,1);

  QHBox* button_box = new QHBox(this,"QDM button box");

  if(can_block)
    {
      QFrame* block_box = new QFrame(this,"QDM block box");
      QHBoxLayout* block_lay = 
	new QHBoxLayout(block_box,0,2,"QDM block layout");

      block_lay->addStretch(1);
#if 1
      m_block_cb = 
	new QCheckBox("Suppress this message for",block_box,"QDM block cb");
      block_lay->addWidget(m_block_cb,0);
      m_block_duration_le = 
	new QLineEdit("60",block_box,"QDM duration cb");
      QSize lesize = m_block_duration_le->sizeHint();
      m_block_duration_le->setMaximumWidth(lesize.width()/3);
      title_lab->setFont(f);
      block_lay->addWidget(m_block_duration_le,0);
      block_lay->addWidget(new QLabel("sec",block_box,"QDM sduration ec lab"));
      connect(m_block_cb,SIGNAL(toggled(bool)),
	      m_block_duration_le,SLOT(setEnabled(bool)));
      m_block_duration_le->setEnabled(false);
#else
      m_block_cb = 
	new QCheckBox("Suppress warning for 60 sec",block_box,"QDM block cb");
      block_lay->addWidget(m_block_cb,0);
#endif
      block_lay->addStretch(1);

      layout->addMultiCellWidget(block_box,2,2,0,1);
    }

  if(have_details)
    {
      QTabWidget* tab=new QTabWidget(this,"QDM tab");

      if((!zero_time)||(det!=""))
	{
	  QTextEdit* det_te = new QTextEdit(tab, "QDM details");

	  if(!zero_time)
	    {
	      char ctime_buf[26];
	      ctime_r(&message.time().tv_sec,ctime_buf);
	      ctime_buf[strlen(ctime_buf)-1]='\0';

	      det_te->setBold(true);
	      det_te->append("Time:");
	      det_te->setBold(false);
	      det_te->append(ctime_buf);
	    }
	  
	  if(det!="")
	    {
	      det_te->setBold(true);
	      det_te->append("Details:");
	      det_te->setBold(false);
	      det_te->append(det.c_str());
	    }
	  det_te->setReadOnly(true);
	  tab->addTab(det_te,"Details");
	}

      if((pro!="")||(fun!=""))
	{
	  QTextEdit* fun_te = new QTextEdit(tab, "QDM function");
	  if(pro!="")
	    {
	      fun_te->setBold(true);
	      fun_te->append(or_str.str());
	      fun_te->append(" Program:");
	      fun_te->setBold(false);
	      fun_te->append(pro.c_str());
	    }

	  if(fun!="")
	    {
	      fun_te->setBold(true);
	      fun_te->append("Function backtrace:");
	      fun_te->setBold(false);
	      fun_te->append(fun.c_str());
	    }
	  fun_te->setReadOnly(true);
	  tab->addTab(fun_te,"Sender");
	}
      
      setExtension(tab);
      
      m_expand = 
	new QPushButton("Expand >>>",button_box,"QDM details button");
      QSize esize=m_expand->sizeHint();
      m_expand->setMinimumWidth(esize.width()*12/10);
      m_expand->setToggleButton(true);
      connect(m_expand,SIGNAL(toggled(bool)),this,SLOT(showExtension(bool)));
    }

  QPushButton* dismiss = 
    new QPushButton("Dismiss",button_box,"QDM dismiss button");
  dismiss->setFocus();
  connect(dismiss,SIGNAL(clicked()),this,SLOT(dismiss()));
  
  layout->addMultiCellWidget(button_box,
			     2+(can_block?1:0),2+(can_block?1:0),0,1);

  layout->setColStretch(0,0);
  layout->setColStretch(1,1);
}

QtMessageDialog::~QtMessageDialog()
{
  // nothing to see here
}

void QtMessageDialog::showExtension(bool showIt)
{
  if(showIt==true)m_expand->setText("Collapse <<<");
  else m_expand->setText("Expand >>>");
  QDialog::showExtension(showIt);
}

void QtMessageDialog::dismiss()
{
  if((m_block_cb)&&(m_block_cb->isChecked()))
    {
      gettimeofday(&m_block.next,0);
      if(m_block_duration_le)
	m_block.duration=m_block_duration_le->text().toInt();
      else m_block.duration=60;
      m_block.next.tv_sec+=m_block.duration;
    }
  else
    {
      m_block.next.tv_sec=0;
      m_block.next.tv_usec=0;
    }
  accept();
}

#if 0
void QtMessageDialog::closeEvent(QCloseEvent * e)
{
  e->ignore();
}
#endif


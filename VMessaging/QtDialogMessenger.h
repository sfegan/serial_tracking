//-*-mode:c++; mode:font-lock;-*-

/**
 * \file QtDialogMessenger.h
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
 * $Date: 2006/04/04 17:06:58 $
 * $Revision: 2.0 $
 * $Tag$
 *
 **/

#ifndef VMESSAGING_QDIALOGMESSENGER_H
#define VMESSAGING_QDIALOGMESSENGER_H

#include<map>

#include<sys/times.h>

#include<qobject.h>
#include<qdialog.h>
#include<qcheckbox.h>
#include<qlineedit.h>
#include<qpushbutton.h>

#include"Message.h"
#include"Messenger.h"

namespace VMessaging
{
  class BlockKey
  {
  public:
    BlockKey(const std::string& p, const std::string& t): 
      program(p), title(t) { }
    std::string program;
    std::string title;	       
    bool operator< (const BlockKey& o) const;
  };
  
  class BlockDetails
  {
  public:
    BlockDetails(int d=60): next(), duration(d) {next.tv_sec=next.tv_usec=0;}
    BlockDetails(struct timeval& n, int d=60): next(n), duration(d) { }
    struct timeval next;
    int duration;
  };
  
  class QtMessageDialog: public QDialog
  {
    Q_OBJECT
  public:
    QtMessageDialog(const Message& message, BlockDetails& block,
		    QWidget* parent=0, const char* name=0);
    virtual ~QtMessageDialog();
  protected slots:
    void showExtension(bool showIt);
  private slots:
    void dismiss();
  private:
    BlockDetails& m_block;
    QCheckBox* m_block_cb;
    QLineEdit* m_block_duration_le;
    QPushButton* m_expand;
  };

  class QtDialogMessenger: public QObject, public Messenger
  {
    Q_OBJECT
  public:
    QtDialogMessenger(Message::PayloadSignificance min_sig,
		     QWidget* parent=0, const char* name=0) throw(): 
      QObject(parent, name), Messenger(), m_parent(parent),
      m_blocks(), m_min_sig(min_sig) { }
    virtual ~QtDialogMessenger() throw();
  public slots:
    virtual bool sendMessage(const Message& message) throw();
  private:
    QWidget* m_parent;
    std::map<BlockKey, BlockDetails> m_blocks;
    Message::PayloadSignificance m_min_sig;
  };
    
} // namespace QtDialogMessenger

#endif // VMESSAGING_QDIALOGMESSENGER_H

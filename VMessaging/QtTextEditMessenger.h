//-*-mode:c++; mode:font-lock;-*-

/**
 * \file QtTextEditMessenger.h
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

#ifndef VMESSAGING_QTEXTEDITMESSENGER_H
#define VMESSAGING_QTEXTEDITMESSENGER_H

#include<map>

#include<sys/times.h>

#include<qtextedit.h>

#include"Message.h"
#include"Messenger.h"

namespace VMessaging
{
  class QtTextEditMessenger: public QTextEdit, public Messenger
  {
    Q_OBJECT
  public:
    QtTextEditMessenger(QWidget* parent=0, const char* name=0) throw();
    virtual ~QtTextEditMessenger() throw();
  public slots:
    virtual bool sendMessage(const Message& message) throw();
  };
    
} // namespace QtTextEditMessenger

#endif // VMESSAGING_QTEXTEDITMESSENGER_H

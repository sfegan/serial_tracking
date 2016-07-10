//-*-mode:c++; mode:font-lock;-*-

/**
 * \file QtMessenger.h
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

#ifndef VMESSAGING_QMESSENGER_H
#define VMESSAGING_QMESSENGER_H

#include<memory>

#include<qobject.h>

#include<Notification.h>

#include"Message.h"
#include"Messenger.h"

namespace VMessaging
{
  class QtMessenger: public QObject, public Messenger
  {
    Q_OBJECT
  public:
    QtMessenger(QObject* parent=0, const char* name=0): 
      QObject(parent,name), Messenger() { }
    virtual ~QtMessenger() throw();
    virtual bool sendMessage(const Message& message) throw();

    static QtMessenger* instance();

  signals:
    void message(const Message& m);

  protected:
    void emitMessage(const Message* m);

  private:
    static std::auto_ptr<QtMessenger> s_instance;

    class MessageNotification: public VTaskNotification::Notification
    {
    public:
      MessageNotification(Message* message, QtMessenger* messenger):
	Notification(), m_message(message), m_messenger(messenger) { }
      virtual ~MessageNotification();
      virtual void doNotification();
    private:
      Message* m_message;
      QtMessenger* m_messenger;
    };

    friend class MessageNotification;
  };
} // namespace QDialogMessenger

#endif // VMESSAGING_QDIALOGMESSENGER_H

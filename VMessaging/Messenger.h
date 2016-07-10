//-*-mode:c++; mode:font-lock;-*-

/**
 * \file Messenger.h
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

#ifndef VMESSAGING_MESSENGER_H
#define VMESSAGING_MESSENGER_H

#include<memory>
#include<set>
#include<queue>

#include<zthread/RecursiveMutex.h>

#include<Task.h>

#include"Message.h"

namespace VMessaging
{
  class RelayMessenger;

  class Messenger
  {
  public:
    virtual ~Messenger() throw();
    virtual bool sendMessage(const Message& message) throw() = 0;
    static RelayMessenger* relay();
  protected:
    Messenger() { }
  private:
    Messenger(const Messenger&);
    const Messenger& operator= (const Messenger&);
  };

  class RelayMessenger: public Messenger
  {
  public:
    RelayMessenger(): 
      Messenger(), m_mutex(), m_backlog(), m_hold(), m_set() { }
    virtual ~RelayMessenger() throw();
    virtual bool sendMessage(const Message& message) throw();
    
    void registerMessenger(Messenger* m) throw();
    void unRegisterMessenger(Messenger* m) throw();

    void holdMessages() throw();
    void releaseMessages() throw();

    static RelayMessenger* instance();
  private:
    ZThread::RecursiveMutex m_mutex;
    std::queue<Message> m_backlog;
    bool m_hold;

    std::set<Messenger*> m_set;
    static std::auto_ptr<RelayMessenger> s_instance;

    bool doSendMessage(const Message& message) throw();
  };

  class TaskMessenger: public Messenger
  {
  public:
    TaskMessenger(Messenger* messenger,
		  VTaskNotification::TaskList* tasklist): 
      Messenger(), m_messenger(messenger), m_tasklist(tasklist) { }
    virtual ~TaskMessenger() throw();
    virtual bool sendMessage(const Message& message) throw();
    
    //static TaskMessenger* instance();
  private:

    class SendMessageTask: public VTaskNotification::Task
    {
    public:
      SendMessageTask(const Message& message, Messenger* messenger):
	Task(), m_message(message), m_messenger(messenger) 
      { /* nothing to see here */ }
      virtual ~SendMessageTask();
      virtual void doTask();
    private:
      Message                      m_message;
      Messenger*                   m_messenger;
    };

    Messenger*                     m_messenger;
    VTaskNotification::TaskList*   m_tasklist;

    //static std::auto_ptr<TaskMessenger> s_instance;
  };

} // namespace VMessaging

#endif // VMESSAGING_MESSENGER_H

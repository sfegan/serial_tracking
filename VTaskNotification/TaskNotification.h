//-*-mode:c++; mode:font-lock;-*-

/**
 * \file TaskNotification.h
 * \ingroup VTaskNotification
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

#ifndef VTASKNOTIFICATION_TASKNOTIFICATION_H
#define VTASKNOTIFICATION_TASKNOTIFICATION_H

#include <Task.h>
#include <Notification.h>

namespace VTaskNotification
{
  // --------------------------------------------------------------------------
  // TaskNotification
  // --------------------------------------------------------------------------

  //! Base class for all Task/Notifications
  class TaskNotification: public Task, public Notification
  {
  public:
    virtual ~TaskNotification();
    
    //! Override this in a derived class to do some task in thread.
    virtual void doTask() = 0;
    
    //! Override this in a derived class to do something when the task completes.
    virtual void doNotification() = 0;
  };
  
  // --------------------------------------------------------------------------
  // TNProxyTask
  // --------------------------------------------------------------------------
  
  //! Proxy for Task to execute the task and queue the notification
  class TNProxyTask: public Task
  {
  public:
    TNProxyTask(TaskNotification* tn, NotificationList* nl):
      Task(), m_mytasknotification(tn), m_notificationlist(nl) {}
    virtual ~TNProxyTask();
    
    //! Do the task then send the notification
    virtual void doTask();
    
  private:
    TaskNotification*      m_mytasknotification;
    NotificationList*      m_notificationlist;
  };
  
  // --------------------------------------------------------------------------
  // TNProxyTaskNotification
  // --------------------------------------------------------------------------

  //! Proxy for TaskNotification to execute the task and the notification
  class TNProxyTaskNotification: public TaskNotification
  {
  public:
    TNProxyTaskNotification(TaskNotification* tn):
      TaskNotification(), m_mytasknotification(tn) {}
    virtual ~TNProxyTaskNotification();
    
    //! Execute the task we are proxying for.
    virtual void doTask();
    
    //! Execute the notification we are proxying for.
    virtual void doNotification();
    
  private:
    TaskNotification*      m_mytasknotification;
  };

  // --------------------------------------------------------------------------
  // TaskNotificationSystem
  // --------------------------------------------------------------------------

  //! Front end to the Task/Notification system.
  class TaskNotificationSystem
  {
  public:
    TaskNotificationSystem(AsyncAlerter* alerter, int autothreads = 1):
      m_tasklist(new TaskList(autothreads)), m_mytl(true),
      m_notificationlist(new NotificationList(alerter)), m_mynl(true) 
    {}
    
    TaskNotificationSystem(NotificationList* nl, int autothreads = 1):
      m_tasklist(new TaskList(autothreads)), m_mytl(true),
      m_notificationlist(nl), m_mynl(false) 
    {}
    
    TaskNotificationSystem(NotificationList* nl, TaskList* tl):
      m_tasklist(tl), m_mytl(false), m_notificationlist(nl), m_mynl(false) 
    {}
    
    virtual ~TaskNotificationSystem();

    //! Add a Task/Notification to the queue
    bool scheduleTaskNotification(TaskNotification* tn)
    { return m_tasklist->scheduleTask(new TNProxyTask(tn,m_notificationlist)); }

    //! Add a Task/Notification to the queue but don't delete it when finished.
    bool scheduleTaskNotificationNoDelete(TaskNotification* tn)
    { return scheduleTaskNotification(new TNProxyTaskNotification(tn)); }
    
    //! Add a thread to the task processing system
    void addTaskProcessor() { addTaskProcessors(1); }
    
    //! Add a number of threads to the task processing system
    void addTaskProcessors(int n) { m_tasklist->addProcessors(n); }
    
    //! Stop the task processing system
    void stopProcessing() { m_tasklist->stopProcessing(); }
    
    //! Add a Task to the TaskList
    bool scheduleTaskOnly(Task* t) { return m_tasklist->scheduleTask(t); }
    
    //! Add a Notification to the NotificationList
    bool scheduleNotificationOnly(Notification* n) 
    { return m_notificationlist->scheduleNotification(n); }
    
  private:
    TaskList*           m_tasklist;
    bool                m_mytl;
    
    NotificationList*   m_notificationlist;
    bool                m_mynl;
  };
} // namespace VTaskNotification

#endif // VTASKNOTIFICATION_TASKNOTIFICATION_H

//-*-mode:c++; mode:font-lock;-*-

/**
 * \file Notification.h
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
 * $Date: 2007/02/03 17:33:37 $
 * $Revision: 2.1 $
 * $Tag$
 *
 **/

#ifndef VTASKNOTIFICATION_NOTIFICATION_H
#define VTASKNOTIFICATION_NOTIFICATION_H

#include <list>

#include <zthread/RecursiveMutex.h>

#include "TNExceptions.h"

namespace VTaskNotification
{
  // --------------------------------------------------------------------------
  // Notification
  // --------------------------------------------------------------------------

  //! Base class for all notifications.
  class Notification
  {
  public:
  virtual ~Notification();
    
    //! Override this function to do something when notified.
    virtual void doNotification() = 0;
  };

  // --------------------------------------------------------------------------
  // AsyncAlerter
  // --------------------------------------------------------------------------

  class NotificationListAlerterInterface; // forward declaration
  
  //! Base class of interface with various aynchronous (event-loop) systems.
  class AsyncAlerter
  {
  public:
    AsyncAlerter(): m_list(0) {}
    virtual ~AsyncAlerter();
    
  protected:
    //! Send an alert to the asynchronous side
    virtual void sendAlert() = 0;
    
    //! Receive the alert on the asynchronous side
    void receiveAlert();
    
    //! Protected function used by the NotificationList to register with Alerter
    void setListInterface(NotificationListAlerterInterface* list);
    NotificationListAlerterInterface* m_list;
    
    friend class NotificationList;
  };
  
  // --------------------------------------------------------------------------
  // NotificationList
  // --------------------------------------------------------------------------

  //! Interface to NotificationList used only by the AsyncAlerter
  class NotificationListAlerterInterface
  {
  public:
    //! Tell the NotificationList to send out all notifications
    virtual ~NotificationListAlerterInterface();
    virtual void processAllNotifications() = 0;

  protected:
    NotificationListAlerterInterface() {}
  };
  
  //! Front end to notification system.
  class NotificationList: 
    protected NotificationListAlerterInterface, 
    protected ZThread::RecursiveMutex
  {
  public:
    //! Initialise the notification-list.
    NotificationList(AsyncAlerter* alerter): 
      NotificationListAlerterInterface(), RecursiveMutex(),
      m_alerter(alerter), m_notificationlist(), m_quit(false),
      m_reentrant_mutex(), m_processing_notifications(false)
    { alerter -> setListInterface(this); }
    
    //! Destroy the notification-list and delete all unprocessed notifications.
    virtual ~NotificationList() throw();

    //! Add a notification to the list to be processed.
    bool scheduleNotification(Notification* notification);

    //! Immediately terminate all notification processing
    void synchronouslyTerminateNotificationProcessing();

  protected:
    //! Process the notifications in the main thread.
    virtual void processAllNotifications();
    
  private:
    Notification* fetchNotification();
    
    typedef std::list<Notification*> NotificationPtrList_t;

    AsyncAlerter*           m_alerter;
    
    NotificationPtrList_t   m_notificationlist;
    bool                    m_quit;

    ZThread::RecursiveMutex m_reentrant_mutex;
    bool                    m_processing_notifications;
  };
  
} // namespace VTaskNotification

#endif // VTASKNOTIFICATION_NOTIFICATION_H

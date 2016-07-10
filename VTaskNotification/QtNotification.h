//-*-mode:c++; mode:font-lock;-*-

/**
 * \file QtNotification.h
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

#ifndef VTASKNOTIFICATION_QTNOTIFICATION_H
#define VTASKNOTIFICATION_QTNOTIFICATION_H

#include <memory>

#include <Notification.h>
#include <QtGUIThreadNotifier.h>

namespace VTaskNotification
{
  // --------------------------------------------------------------------------
  // QtAsyncAlerter
  // --------------------------------------------------------------------------

  //! Send an alert from a thread to the Qt event-loop
  class QtAsyncAlerter: public QObject, public AsyncAlerter
  {
    Q_OBJECT
    
  public:
    QtAsyncAlerter(QObject* parent = 0);
    virtual ~QtAsyncAlerter();
    //! Send an alert to the Qt event-loop (asynchronous side)
    virtual void sendAlert();

  protected slots:
    //! Receive alert from threads (synchronous side)
    void gotGUIThreadNotifier();

  protected:
    QtGUIThreadNotifier* m_notifier;
};

  // --------------------------------------------------------------------------
  // QtNotificationList
  // --------------------------------------------------------------------------
  
  //! A NotificationList specialized to the Qt event-loop.
  class QtNotificationList: public NotificationList
  {
  public:
    virtual ~QtNotificationList() throw();
    
    //! Get the singleton instance of this class.
    static QtNotificationList* getInstance();
    
  protected:
    QtNotificationList(): NotificationList(new QtAsyncAlerter()) {}
    
  private:
    QtNotificationList(const QtNotificationList&);
    QtNotificationList& operator=(const QtNotificationList&);
  };
} // namespace VTaskNotification
#endif // VTASKNOTIFICATION_QTNOTIFICATION_H

//-*-mode:c++; mode:font-lock;-*-

/**
 * \file Notification.cpp
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
 * $Revision: 2.2 $
 * $Tag$
 *
 **/

#include <cassert>

#include <Notification.h>

#include <zthread/Guard.h>

using ZThread::Guard;
using ZThread::LockedScope;
using ZThread::UnlockedScope;

using namespace VTaskNotification;

// ----------------------------------------------------------------------------
// Notification
// ----------------------------------------------------------------------------

/*! \class Notification 
  
  This class is the base for all notifications that can be scheduled
  with the NotificationList system. It should be derived to
  accomplish the specific task you require from the
  notification. Instances of the Notification class should only be
  created on the heap (with <B>new</B>) as they are deleted by the
  NotificationList when completed.
*/

Notification::~Notification()
{
  // nothing to see here
}

// ----------------------------------------------------------------------------
// AsyncAlerter
// ----------------------------------------------------------------------------

/*! \class AsyncAlerter

  The AsyncAlerter provides the glue between the synchronous side
  (i.e.  the threads) and the asynchronous side (i.e. the
  event-loop). It is used by the NotificationList to transmit an \em
  alert to the event-loop that a notification is on the list and
  should be executed.
  
  This class works in behalf of the the NotificationList class. None
  of its methods are public, but the NotificationList is a friend
  class allowing it to use sendAlert() and setListInterface(). One
  instance of NotificationList can be registered with this class, and
  will be alerted by receiveAlert() when sendAlert() is called.

  The virtual function sendAlert() should be implemented in a derived
  class to transmit an alert into the event-loop for the system and
  call receiveAlert() from the main thread. See QtAsyncAlerter for an
  example in the Qt system. The purpose of this class is to make the
  Notification system work with many different event-driven systems,
  such as Qt or CORBA. Any single implementation of this class could
  easily have been designed into the NotificationList directly, but
  that would have limited the notification system to one event-driven
  system.

  \sa NotificationList
 */

AsyncAlerter::~AsyncAlerter()
{
  // nothing to see here
}

/*! \fn AsyncAlerter::sendAlert() = 0 
  It is the responsibility of this function to make sure that the
  receiveAlert() is called in the main thread of the system. How it
  does this depends on the event-driven system in question. See the
  Qt (QtAsyncAlerter) implementation for an example.
 */

/*!
  Function should be called, somehow, when the sendAlert() function
  is called. That "somehow" is implementation dependent. See
  QtAsyncAlerter for an example.
 */
void AsyncAlerter::receiveAlert()
{
  m_list -> processAllNotifications();
}

/*!
  Used by the NotificationList to register with the Alerter. It
  first checks that there isn't another object registered. The only
  place this should be called from is in the constructor for the
  NotificationList (NotificationList::NotificationList())

  \param list The NotificationList to alert when sendAlert() is called.
 */
void AsyncAlerter::setListInterface(NotificationListAlerterInterface* list) 
{ 
  assert(m_list == 0);
  m_list = list; 
}

// ----------------------------------------------------------------------------
// NotificationList
// ----------------------------------------------------------------------------

/*! \class NotificationListAlerterInterface 
  This pure interface is used by the AsyncAlerter to call the
  protected NotificationList::processAllNotifications() method in the
  thread of the main thread of the system.

  The purpose of this interface is to keep the TaskList internals
  away from the end-user and also from the AsyncAlerter. It could be
  done away with by making the AsyncAlerter a friend to the
  NotificationList, but since we don't know who might write
  AsyncAlerter classes, this class provides a better level of
  protection to the NotificationList..

  \sa NotificationList
 */

NotificationListAlerterInterface::~NotificationListAlerterInterface()
{
  // nothing to see here
}

/*! \class NotificationList 

  This class provides queuing of Notification objects on a
  notification list and arranges from them to be executed in the main
  thread of the application. It can be used with any event-driven
  system, the functionality to interact with the event-loop is
  provided by a specific sub-class of AsyncAlerter.

  Many GUI type applications will not want to use NotificationList
  directly, but will find the TaskNotificationSystem which provides a
  merging of functionality with the TaskList system more convenient.

  Internally, this class uses an STL list as a queue of
  notifications. A JTCMonitor ensures that the list is only used by
  one thread at a time. A notification is put on the list by
  scheduleNotification() which then uses the AsyncAlerter to tell the
  main thread that a notification is present. The alerter calls
  processAllNotifications() from the main thread. This executes
  Notification::doNotification() and deletes it when finished.

  Since the Notification is <b>delete</b>d when it is processed, it
  is essential that all notifications are created on the heap with
  <b>new</b>.
*/

/*! \fn NotificationList::NotificationList(AsyncAlerter* alerter)
  
   \param alerter The AsyncAlerter to use when alerting the
   event-driven system that a notification is on the list. For
   consistency, the alerter is deleted when the class is destroyed, so
   it should be allocated on the heap.
 */

/*! 
  Disallow scheduling of new notifications, delete all unprocessed
  notifications and destroy the alerter.
*/
NotificationList::~NotificationList() throw()
{
  Guard<RecursiveMutex> guard(*this);
  m_quit = true;
  for(NotificationPtrList_t::iterator i = m_notificationlist.begin();
      i != m_notificationlist.end(); i++)
    delete(*i);
  delete m_alerter;
}

/*!
  Add a notification to the list and alert the main thread of its
  presence. If the destructor has been invoked, i.e. the class is
  being destroyed then we just delete the notification and return.
  
  \param notification The notification to be added to the list,
  allocated on the heap with <b>new</b>.
*/
bool NotificationList::scheduleNotification(Notification* notification)
{
  Guard<RecursiveMutex> guard(*this);
  if(m_quit)
    {
      delete notification;
      return false;
    }

  m_notificationlist.push_back(notification);
  m_alerter -> sendAlert();
  return true;
}

/*!
  Immediately terminate all notification processing, discarding 
  those notifications that have not yet been executed.
  
  \param notification The notification to be added to the list,
  allocated on the heap with <b>new</b>.
*/
void NotificationList::synchronouslyTerminateNotificationProcessing()
{
  Guard<RecursiveMutex> guard(*this);
  m_quit = true;
}

/*
  Fetch all notifications from the list one at a time. Process each
  one and delete it when done.
*/
void NotificationList::processAllNotifications()
{
  // Initially this function was re-entrant but that can cause (potentially) 
  // undesirable effects, like multiple Qt event loops being run, so now we
  // guard access to the function and set the m_processing_notifications 
  // when it is active. That way, if a new notification arrives while
  // running the doNotification() function of a previous,, and if the
  // doNotification() somehow allows this function to be re-entered, the 
  // second notification will be deferred until the first is completed. This
  // occurs, for example, if the doNotification() function pops up a dialog
  // box in Qt and re-enters the Qt event loop, which can then allow further
  // notifications to be run. Perhaps these checks should be in the 
  // QtAsyncAlerter, or preferably, parameterized by a serialization class.

  // Careful about the locking in this function.. we don't want to screw
  // everything up if an exception is thrown. A mutex with a zero-timeout
  // would simplify the logic here by consolidating the functionality of
  // m_reentrant_mutex and m_processing_notifications... but this way works 
  // too :-)

  Guard<RecursiveMutex> guard(m_reentrant_mutex);
  try
    {
      if(m_processing_notifications)return;
      else m_processing_notifications=true;
      
      while(Notification* notification = fetchNotification())
	{
	  // Unlock while we process the notification to allow re-entrance
          // to the top of this function, i.e. to the test of whether
          // m_processing_notifications is true.... this is  not strictly
          // necessary if all the processing occurs in a single thread sunce 
          // we use a "RecursiveMutex
	  if(!m_quit)
	    {
	      Guard<RecursiveMutex,UnlockedScope> 
		sleeping_guard(m_reentrant_mutex);
	      notification -> doNotification();
	    }
	  delete(notification);
	}
      m_processing_notifications=false;
    }
  catch(...)
    {
      m_processing_notifications=false;
      throw;
    }
}

/*
  Provide the required locking to get a notification from the top of the list.
  
  \return The Notification at the top of the list or 0 if there are none.
*/
Notification* NotificationList::fetchNotification()
{
  Guard<RecursiveMutex> guard(*this);
  if(m_notificationlist.empty())return 0;
  
  Notification* notification = m_notificationlist.front();
  m_notificationlist.pop_front();
  return notification;
}

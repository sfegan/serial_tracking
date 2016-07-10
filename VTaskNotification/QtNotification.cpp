//-*-mode:c++; mode:font-lock;-*-

/**
 * \file QtNotification.cpp
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
 * $Date: 2008/03/01 22:31:56 $
 * $Revision: 2.2 $
 * $Tag$
 *
 **/

#include <QtNotification.h>

#include <zthread/RecursiveMutex.h>
#include <zthread/Guard.h>

using namespace VTaskNotification;

// ----------------------------------------------------------------------------
// QtAsyncAlerter
// ----------------------------------------------------------------------------

/*! \class QtAsyncAlerter

  An implementation of AsyncAlerter which alerts the Qt event-loop of
  a notification being placed on the list. This is done with a helper
  class QtGUIThreadNotifier. At this point the reader is probably
  screaming that this is the worst kind of C++ code, lots of classes
  in which nothing happens, and indeed I had thought of putting the Qt
  functionality directly in this class, but QtGUIThreadNotifier
  provides something useful, a mechanism of propagating simple SIGNALS
  from a thread into the main thread. It is useful in its own right,
  so I chose to implement it as a different class.

  So basically the functionality of this class is simple, when we are
  asked to sendAlert() we call the slot (method)
  QtGUIThreadNotifier::sendNotice() which emits the signal
  QtGUIThreadNotifier::gotNotice() in the main thread. That is
  connected to gotGUIThreadNotifier() here which then calls
  receiveAlert() as required by a AsyncAlerter implementation.

  It would be nice if Qt provided a threaded signal-slot mechanism for
  sending signals to other threads; it does provide something quite
  close, the Thread::postEvent method which allows

  \sa QtGUIThreadNotifier
*/

/*! 
  Create a QtGUIThreadNotifier and connect it up to this class in the 
  appropriate manner.

  \param parent Qt parent of this object
 */  
QtAsyncAlerter::QtAsyncAlerter(QObject* parent): 
  QObject(parent), AsyncAlerter(), m_notifier()
{
  m_notifier = new QtGUIThreadNotifier(this);
  QObject::connect(m_notifier, SIGNAL(gotNotice()),
		   this, SLOT(gotGUIThreadNotifier()));
}

QtAsyncAlerter::~QtAsyncAlerter()
{
  // nothing to see here
}

/*!
  Send the alert on to the GUI thread with the QtGUIThreadNotifier.
 */
void QtAsyncAlerter::sendAlert()
{
  m_notifier -> sendNotice();
}

/*!
  Get the alert in the GUI thread from the QtGUIThreadNotifier.
 */
void QtAsyncAlerter::gotGUIThreadNotifier()
{
  receiveAlert();
}

// ----------------------------------------------------------------------------
// QtNotificationList
// ----------------------------------------------------------------------------

/*! \class QtNotificationList

  A convenience class that links the NotificationList with the
  QtAsyncAlerter to create a notification processing system that
  works with the Qt event loop.

  Since there is only one instance of QApplication (and hence one
  event-loop) per application, there is only need for one instance of
  a QtNotificationList, all threads can put their notifications onto
  a single list and they will be propagated into the main GUI
  thread. Two or more notification-lists gain you nothing. So this
  class can only be instantiated through the static getInstance()
  function, this creates one instance of the class and returns a
  pointer to it there after. The static call is available globally and
  to all threads, allowing easy access to the notification-list from
  everywhere. This approach, known as the <b>singleton pattern</b> and
  described in [1], is better for many reasons than using a global
  variable.

  \par References
  [1] 
 */

/*! \fn QtNotificationList::QtNotificationList()
  Protected so users cannot create an object of this class except
  through the getInstance() function.
*/

QtNotificationList::~QtNotificationList() throw()
{
  // nothing to see here
}

/*!
  When first called it creates a new instance of this class on the
  heap. Thereafter it returns the instance to all those who need
  it. This method provides the only way to instantiate the class since
  the constructor is protected.

  \returns Singleton instance of this class

  \par Example
  In some thread we can send a notification to the GUI thread with
  the following code sample,

  \par
  \code
  // Assuming we have a class MyNotification which does something
  // in the GUI thread
  Notification* n = new MyNotification( whatever args );
  QtNotificationList::getInstance()->scheduleNotification(n);
  \endcode
 */
QtNotificationList* QtNotificationList::getInstance()
{
  static std::auto_ptr<QtNotificationList> instance(0);
  static ZThread::RecursiveMutex mutex;
  ZThread::Guard<RecursiveMutex> guard(mutex);
  
  if(instance.get() != 0)return instance.get();
  instance.reset(new QtNotificationList());
  return instance.get();
}


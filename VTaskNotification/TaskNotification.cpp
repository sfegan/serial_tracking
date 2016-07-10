//-*-mode:c++; mode:font-lock;-*-

/**
 * \file TaskNotification.cpp
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

#include <TaskNotification.h>

using namespace VTaskNotification;

// This file contains 5% code, 95% documentation

// ----------------------------------------------------------------------------
// TaskNotification
// ----------------------------------------------------------------------------

/*! \class TaskNotification
 
  This class combines the functionality of Task and Notification
  classes. Re-implement this in a derived class to provide the
  functionality of the task and notification. When used with
  TaskNotificationSystem::scheduleTaskNotification() the doTask()
  method will be called in a separate thread to the main thread of the
  application and after this completes the doNotification() method
  will be called in the main thread.
 */

/*! \fn TaskNotification::doTask() 
 
  The task processing system calls this function in a processing
  thread. Implementing this function in a derived class allows any
  code to be executed in the thread.
  
  There is no default exception handler in the task-processing system,
  so throwing any uncaught exceptions will likely result in the
  program aborting or getting into a
*/

TaskNotification::~TaskNotification()
{
  // nothing to see here
}

// ----------------------------------------------------------------------------
// TNProxyTask
// ----------------------------------------------------------------------------

/*! \class TNProxyTask

  This private class allows the task/notification system to overcome
  the fact that the task-list deletes tasks that are completed. Since
  we also require that a notification be sent out, this proxy class is
  put onto the task-list. It executes the task it is proxying for,
  sends out the notification and then is deleted by the task-list.

  Most people will not need to use this object directly, it is created
  automatically by the
  TaskNotificationSystem::scheduleTaskNotification() method and then
  deleted automatically by TaskProcessor::run().
 */

TNProxyTask::~TNProxyTask()
{
  // nothing to see here
}

/*! 
  Execute the task that we are proxying for then put the notification 
  onto the notification-list.
 */
void TNProxyTask::doTask()
{
  try
    {
      m_mytasknotification->doTask();
    }
  catch(...)
    {
      // If we get an exception we must delete the TaskNotification
      // we are proxying for as the notification will not be sent and
      // the object would otherwise be left on the heap. We just
      // rethrow the exception after we clean up... there is nothing
      // else we can do.
      delete m_mytasknotification;
      throw;
    }
  
  // Post the notification onto the list
  m_notificationlist->scheduleNotification(m_mytasknotification);
}

// ----------------------------------------------------------------------------
// TNProxyTaskNotification
// ----------------------------------------------------------------------------

/*! \class TNProxyTaskNotification

  This private class is used only by the
  TaskNotificationSystem::scheduleTaskNotificationNoDelete() method
  to schedule a TaskNotification without having it deleted when the
  task and notification are done. Since the notification processor
  deletes notifications when they have been executed, this class is
  used put on the notification-list a proxy and is deleted when
  finished leaving the original TaskNotification undeleted.
 */

TNProxyTaskNotification::~TNProxyTaskNotification()
{
  // nothing to see here
}

/*!
 */
void TNProxyTaskNotification::doTask()
{
  m_mytasknotification->doTask();
}

/*!
 */
void TNProxyTaskNotification::doNotification()
{
  m_mytasknotification->doNotification();
}

// ----------------------------------------------------------------------------
// TaskNotificationSystem
// ----------------------------------------------------------------------------

/*! \class TaskNotificationSystem

  Not documented yet. Sorry!
  
 */

/*! \fn TaskNotificationSystem::TaskNotificationSystem(AsyncAlerter* alerter, int autothreads=0)

  Initialize the system by creating a new TaskList and
  NotificationList. The notification-list constructor is passed the
  \a alerter parameter and the task-list constructor is passed the \a
  autothreads parameter. The task-list and notification-list will be
  deleted when the object is destroyed.

  \param alerter The alerter parameter is passed to the constructor of
  the NotificationList. It provides an interface to alert the
  event-loop that a notification has been placed on the list. See
  AsyncAlerter and NotificationList.

  \param authothreads Passed to the constructor of the TaskList.  The
  maximum number of threads that can exist and still allow the system
  to automatically spin off threads to process tasks as they are
  submitted. The default of \b 1 allows for the creation of one thread
  automatically when a task is added, unless the user explicitly
  creates one with addProcessors().

  In reality, users need not interact with this interface, they should
  create derived classes that are specialized to a particular
  event-driven system, e.g. the Qt system and
  QtTaskNotificationSystem.

  \sa NotificationList, AsyncAlerter, TaskList

 */

/*! \fn TaskNotificationSystem::TaskNotificationSystem(NotificationList* nl, int autothreads=0)

  Initialize the system by creating a new TaskList and using the
  provided NotificationList. The task-list constructor is passed the
  \a autothreads parameter. The task-list is deleted when the class is
  destroyed but the notification-list is not.

  \param nl The notification-list to use when sending notifications of
  tasks being completed. The list will not be deleted when the class
  is destroyed. The idea being that many TaskNotificationSystem
  objects could share one notification-list.

  \param authothreads Passed to the constructor of the TaskList.  The
  maximum number of threads that can exist and still allow the system
  to automatically spin off threads to process tasks as they are
  submitted. The default of \b 1 allows for the creation of one thread
  automatically when a task is added, unless the user explicitly
  creates one with addProcessors().

  In reality, users need not interact with this interface, they should
  create derived classes that are specialized to a particular
  event-driven system, e.g. the Qt system and
  QtTaskNotificationSystem.

  \sa NotificationList, AsyncAlerter, TaskList

*/

/*! \fn TaskNotificationSystem::TaskNotificationSystem(NotificationList* nl, TaskList* tl)
  
  Initialize the system using a previously create task- and
  notification-list. Neither will be deleted when the class is
  destroyed.

  \param nl The notification-list to use when sending notifications of
  tasks being completed. The list will not be deleted when the class
  is destroyed. The idea being that many TaskNotificationSystem
  objects could share one notification-list.

  \param tl The task-list to use when putting tasks onto the queue. I
  can't think of a reason for wanting to create your own task-list
  separately.

  In reality, users need not interact with this interface, they should
  create derived classes that are specialized to a particular
  event-driven system, e.g. the Qt system and
  QtTaskNotificationSystem.

  \sa NotificationList, AsyncAlerter, TaskList
*/

/*!  
  Delete the task-list and notification-list if we created them in
  the constructor. If we were given them then we don't delete them as
  they may be shared with other objects.
 */
TaskNotificationSystem::~TaskNotificationSystem()
{
  if(m_mytl)delete m_tasklist; 
  if(m_mynl)delete m_notificationlist;
}

// ----------------------------------------------------------------------------
// Main page documentation
// ----------------------------------------------------------------------------

/*! \mainpage VERITAS Task/Notification system

  \section intro Introduction

  The Task/Notification system allows an asynchronous, event-driven
  program to execute synchronous, blocking tasks in threads and have
  those tasks communicate a response (notification) back. This has
  primary application to GUIs where the event-loop would otherwise
  have to halt to execute blocking calls.

  \section specs Requirements

  The system should satisfy the following requirements:

    - Make it easy to queue tasks to be processed by a pool of threads.
    - Not require user to use extensive (or any if possible) mutex locking
      between threads and GUI thread.
    - Merge the asynchronous, event-driven, GUI approach to code with 
      synchronous, blocking, CORBA approach.
    - Thread safe.
    - Easy to use. Minimal knowledge of internals necessary.

  \section arch Architecture

  The system is composed of two primary components, the \em task-list
  and the \em notification-list. The two components can either be used
  separately or combined into a <em>task/notification system</em>
  providing a method of queuing tasks and automatically notifying the
  GUI on their completion.

  The task-list is implemented by the class TaskList which queues
  tasks (Task) for a pool of threads (or a single thread) to
  execute. Threads can be allocated to the task-list thread-pool,
  either directly by the user or automatically when a task is added
  and there is no thread ready to process it (up to some user defined
  maximum number of threads). The functionality of the task is
  achieved by implementing a sub-class of Task and passing it to the
  TaskList system to be processed.
  
  The complementary side is the notification-list, implemented by the
  class NotificationList which allows threads to send an asynchronous
  notification event (Notification) back to the main thread of the
  application, i.e. the thread running the event-loop. Again, by
  implementing a sub-class of Notification a thread can arrange for
  any code to be executed in the main-thread of the application. The
  notification system uses a helper class, AsyncAlerter, to alert the
  event-loop of a pending notification; this class contains all the
  code specific to a certain event-driven system, e.g. the
  QtAsyncAlerter send alerts to the Qt event-loop.
    
  These two components are combined into a single task/notification
  system (TaskNotificationSystem) which makes it easy to schedule a
  task to run in a separate thread and automatically send a
  notification back to the main thread upon completion of the
  task. TaskNotification combines Task and Notification into a
  single class. By implementing a derived class a task and
  notification can be combined in an easy fashion with data being
  shared between them.
    
  In a GUI environment this allows a blocking call to be executed
  outside of the event-loop and the results to be propagated back to
  the main-thread where it can be used to update the display without
  having to worry about locking the GUI library mutex.

  The flow of control through the various classes is illustrated as
  simple flow-charts in the \ref flow pages.
    
  \section qt The Qt based TaskNotification system
    
  The NotificationList class is specialized to the Qt event-loop by
  QtNotificationList. This class is basically a convenience wrapper
  around the TaskNotificationList constructor. It creates a
  QtAsyncAlerter with which the notification-list can alert the Qt
  event-loop of a pending event.
    
  Since every Qt based application can only have one Qt event-loop
  (i.e. a single QApplication can exist), only one QtNotificationList
  can be created per application. This provides a single object that
  all threads can use to send notifications to the Qt event-loop. The
  static QtNotification::getInstance() function can be used from
  anywhere in the application to get that single instance.
  
  The QtTaskNotificationSystem provides a specialization of the
  task/notification system to Qt. This is primary interface to the
  system in a Qt environment. It provides a convenient mechanism to
  queue tasks and notifications.

  Writing a GUI application which does blocking calls in separate
  threads then simply becomes a matter of writing a sub-class of
  TaskNotification, instantiating a QtTaskNotificationSystem object
  and using it to schedule the task/notification.

  \section specssatisy Satisfying the requirements

  \subsection specsat_task Easy to queue tasks to be processed by thread
  
  The TaskList and Task classes satisfy this requirement.

  \par Example
  
  The following code fragment shows how to define a task (to get the
  weather from a server) and schedule it to be processed by a
  thread. It is only an illustration, clearly one would want to so
  something with the weather details after getting them from the
  server.

  \par
  \code
  class GetWeatherTask : public Task
  {
  public:
    GetWeatherTask(WeatherServer_ptr server): m_server(server) {}
    void doTask() { WeatherDetails weather = m_server->getWeatherDetails(); }
  private:
    WeatherServer_ptr m_server;
  };

  main()
  {
    // initialise stuff

    TaskList weather_tasklist(1);
    weather_tasklist.scheduleTask(new GetWeatherTask(weatherserver));
  }
  \endcode

  \subsection specsat_mutex No mutex locking between threads and GUI thread.

  blah...

  \subsection specsat_merge Merge event-driven approach with blocking approach.

  blah...

  \subsection specsat_thsafe Thread safe.

  blah...
  
  \subsection specsat_easy Easy to use.

  blah...

  \section examples Examples
*/

/*! \example qttasknotification.cpp

  This is an example of a very simple Qt GUI application that uses the
  QtTaskNotificationSystem system to do a blocking task in a thread
  so that the GUI is still responsive. When the task ends it updates 
  the GUI with its results.

  In this case the task is simply a call to the system to get the time,
  convert it to a string and then pause for 2 seconds.
*/

/*! \page flow Flow charts

  The following flow-charts depict the program flow for the three main
  things a user may wish to do,

    -# Schedule and execute a task
    -# Schedule and execute a notification
    -# Schedule and execute a task/notification

  Additionally there is a flow-chart depicting the functionality of the
  QtAsyncAlerter and its interactions with the system.

  \subsection taskflow Scheduling a task

  Schedule and execute a task using TaskList.

  \image html Task.png
  \image latex Task.ps "" height=5in

  \subsection noteflow Schedule a notification

  Schedule and execute a notification using NotificationList

  \image html Notification.png
  \image latex Notification.ps "" height=5in

  \subsection tasknoteflow Schedule a task/notification

  Schedule and execute a task and notification using TaskNotificationList

  \image html TaskNotification.png
  \image latex TaskNotification.ps "" height=5in

  \subsection qtasyncalerterflow QtAsyncAlerter functionality

  Alert the Qt event loop to the presence of a notification. This
  chain is represented above in the \ref noteflow and \ref
  tasknoteflow sections by a dark green arrow labelled as "Somehow".

  \image html QtAsyncAlerter.png
  \image latex QtAsyncAlerter.ps "QtAsyncAlerter flow-chart" height=5in

*/

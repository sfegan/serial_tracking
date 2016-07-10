//-*-mode:c++; mode:font-lock;-*-

/**
 * \file Task.cpp
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
 * $Date: 2006/04/10 18:01:11 $
 * $Revision: 2.1 $
 * $Tag$
 *
 **/

#include <assert.h>
#include <iostream>
#include <algorithm>

#include <zthread/Guard.h>

#include <Task.h>
#include <Debug.h>
#include <Exception.h>

using ZThread::Runnable;
using ZThread::Thread;
using ZThread::Guard;
using ZThread::RecursiveMutex;

using namespace VMessaging;
using namespace VTaskNotification;

// ----------------------------------------------------------------------------
// Task
// ----------------------------------------------------------------------------

/*! \class Task
  This class is the base for all tasks that can be scheduled with the
  TaskList system. It should be derived to accomplish the specific
  task you require. Instances of the Task class should only be created
  on the heap (with <B>new</B>) as they are deleted by the
  TaskProcessorThread when completed.

  The idea is that you create a derived class to perform a task,
  submit the task to the TaskList object, it gets executed in a
  separate thread and then deleted.
*/

/*! \fn Task::doTask() 
 
  The task processing system calls this function in a processing
  thread. Implementing this function in a derived class allows any
  code to be executed in the thread.
  
  If you throw any exceptions in this function, you should be careful
  to catch them or they will propagate down to the threads package
  where they will cause an annoying message to be printed out and
  terminate the thread. This is not disastrous as another thread will
  take its place, but it is not very neat. So in general you should
  ensure that all exceptions are caught.

  \par example
  
  \code
  \endcode
*/

Task::~Task()
{
  // nothing to see here
}

// ----------------------------------------------------------------------------
// TaskList
//
// There are basically three classes involved with the task-list
// processing system, the TaskList, TaskListProcessorInteface and
// the TaskProcessorThread.
//
// Splitting the task-list interface into a public (TaskList) and
// private (TaskListProcessorInterface) part is meant to provide
// security to the system, which must maintain a strict locking
// protocol to ensure that the threads don't get fouled up. So we take
// on managing all threads and locking from within the TaskList. The
// processors are allowed to use the TaskListProcessorInteface but
// the user isn't. Additionally you can't instantiate a
// TaskListProcessor directly (its constructor is hidden) and you
// can't create a TaskProcessorThread without having a
// TaskListProcessorInterface for it to service. Friend classes could
// have been used to do this but I don't like their
// interface-subversion properties, i.e. that they don't have to go
// through the interface but can muck around with the insides of
// objects directly
// ----------------------------------------------------------------------------

TaskListProcessorInterface::~TaskListProcessorInterface()
{
  // nothing to see here
}

/*! \class TaskList
  The TaskList class provides all the functionality that an end user
  needs to start threads, add tasks to the queue and shut down
  processing when finished. The user creates a task list, assigns a
  number of threads to it and then submits tasks which are executed by
  the threads. There is no functionality to return status from the
  tasks to the caller, but it is easy for a task to implement that
  functionality itself or use the NotificationList.

  Many GUI type applications will not want to use TaskList and
  NotificationList directly, but will find the TaskNotificationSystem
  more convenient.
  
  The interface to task list is split into a public part, implemented
  by this class, and a private part implemented as protected
  base-class, TaskListProcessorInterface. The private part is
  accessible only to the TaskProcessorThread objects that contain the
  functionality of the threads.

  Internally, this class uses an STL list as a queue of tasks.  A
  JTCMonitor ensures that the list is only used by one thread at a
  time. There is also a STL list of threads (TaskProcessorThread)
  that are part of our thread-pool.

  \sa Task and TaskNotificationSystem
*/

/*! \fn TaskList::TaskList(int autothreads = 1) 
  Create a TaskList to process tasks. No threads are created by the
  constructor, they must be explicitly assigned to the task-list by
  the addProcessors() function or created automatically by
  setting the \e autothreads parameter to a non zero value and assigning
  some tasks to the task-list.

  \param autothreads Set the maximum number of threads that can exist
  and still allow the system to automatically spin off threads to
  process tasks as they are submitted. The default of \b 1 allows for
  the creation of one thread automatically when a task is added,
  unless the user explicitly creates one with addProcessors().
 */

/*! 
   The destructor calls stopProcessing() to ensure that
   all tasks submitted to the task-list are completed before the
   system finishes.
*/
TaskList::~TaskList() throw ()
{
  try
    {
      // Wait for all tasks to complete
      stopProcessing();
      
      // The task-list should always be empty!
      assert(m_tasklist.empty());
    }
  catch (...)
    {
      assert(false);
    }
}

/*! 
  The tasks will be processed by the system in the order they are
  received. They are removed from the queue by the processing threads,
  executed and then deleted.

  Since the task is <b>delete</b>d when processed by the
  TaskProcessorThread so it should be allocated by the caller on the heap
  with <b>new</b>. The task is guaranteed to be processed if submitted
  before stopProcessing is called by the user. If submitted after
  stopProcessing is invoked it will be <b>delete</b>d and ignored.
  
  If there is not a thread immediately available to process this task
  AND there are less threads assigned to this task-list than called
  for by the \e autothreads parameter to the constructor then a thread
  will be created to process this task. When finished, the thread
  joins the pool and is available to process other tasks. This
  provides for a way to have many tasks running simultaneously (up to
  a maximum) but not have to explicitly create them on start-up.

  \param task The task to be processed, allocated on the heap with
  <b>new</b>.
  
  \return true if the task is accepted for processing, false otherwise (i.e
  if the stopProcessing method had been called).
*/
bool TaskList::scheduleTask(Task* task)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  if(task == 0)
    {
      Debug::stream()
	<< "TaskList::scheduleTask: request to schedule NULL task ignored" 
	<< std::endl;
      return true;
    }

  Guard<RecursiveMutex> guard(*this);
  if(m_quit)
    {
      delete task;  // its debatable whether we should delete the task or not
      return false; 
    }

  // If there isn't enough threads waiting then create one. This isn't
  // an exact science since a thread can just become free in the nick
  // of time to handle this task, but we can't handle everything!
  if((m_nthreadswaiting <= m_ntasksonlist) &&
     (m_nthreads < m_autothreads))addProcessor();

  m_tasklist.push_back(task);
  m_ntasksonlist++;
  signal();
  return true;
}

/*! 
  Wait for all tasks to complete then shut down all threads. No
  tasks can be submitted to the task list after this method is
  called. It returns after all tasks have been completed and all
  processor threads are finished.
*/
void TaskList::stopProcessing()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  if(m_quit)return;

  {
    Guard<RecursiveMutex> guard(*this);
  
    // Check again after we got the mutex lock
    if(m_quit)return;
    
    // Stop all additions of Tasks
    m_quit = true;
    
    // No more tasks to be done, notify all sleeping threads that we're done
    broadcast();
  }
   
  // wait for all threads to complete. this must be done outside of the
  // mutex-locked section to allow other threads to finish up, so we drop
  // out of the JTCSynchronized code block

  for(ThreadsList_t::iterator t = m_threadslist.begin(); 
      t != m_threadslist.end(); t++)
    (*t)->wait();
}

/*! 
  Create a number of TaskProcessorThread objects and assign them to
  work on the task-list. This function provides the only way of
  assigning threads to the queue. TaskProcessorThread objects cannot be
  created directly by the user. 
  
  \param n The number of threads to <b>add</b> to the pool. 
*/
void TaskList::addProcessors(int n)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  Guard<RecursiveMutex> guard(*this);
  if(m_quit)return;
  while(n--)
    {
      Runnable* runner = new TaskProcessorThread(this);
      Thread* proc = new Thread(runner);
      m_threadslist.push_back(proc);
      m_nthreads++;
    }
}

/*! 
   This is where most of the action is, the TaskProcessorThread class
   uses this to wait for tasks to come in and return them to it. All
   threads in the system are at all times either processing a request
   or waiting for one. They are woken up by the scheduleTask()
   function when a new task is added to the list. Most of the magic
   occurs in the JTC wait()/notify() system. When a thread goes into
   the wait() it releases the lock it has on the monitor and goes to
   sleep. The wait() call returns (with a lock on the monitor again)
   when the another thread calls notify() on the monitor.

   If the user calls stopProcessing() all threads are woken up and
   return a null task to the caller which is an indication that there
   are no more tasks to be done.

   \return A pointer to a task to be done, or NULL if there are no
   more tasks on the list and the user has called stopProcessing().
*/

Task* TaskList::fetchTask()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  Guard<RecursiveMutex> guard(*this);

  // While the list is empty AND we are not quitting, wait on the monitor
  while((m_tasklist.empty()) && (!m_quit))
    {
      try
	{
	  m_nthreadswaiting++;
	  wait();
	  m_nthreadswaiting--;
	}
      catch(const ZThread::Interrupted_Exception&)
	{
	}
    }

  if(!m_tasklist.empty())
    {
      // There is a task on the list, return it for processing
      Task* task = m_tasklist.front();
      m_tasklist.pop_front();
      m_ntasksonlist--;
      return task;
    }
  else if(m_quit)
    {
      // Quit has been posted and there are no more tasks so exit thread
      return 0;
    }
  
  // Should never get down to here
  assert(0);
}

// ----------------------------------------------------------------------------
// TaskListProcessorInterface
// ----------------------------------------------------------------------------

/*! \class TaskListProcessorInterface
  The interface to task list is split into a public part,
  implemented by the TaskList class, and a private part implemented
  by this class. The private part is accessible only to the
  TaskProcessorThread objects that contain the functionality of the
  threads.

  This class is used by the TaskProcessorThread objects to wait for a task
  and remove it when it is submitted. It is "pure" in the sense that
  there is no code associated with it.

  \sa TaskList
*/

// ----------------------------------------------------------------------------
// TaskProcessorThread
// ----------------------------------------------------------------------------

/*! \class TaskProcessorThread
  The TaskProcessorThread class implements a JTCThread which sits in a loop
  getting tasks from the task-list, processing them and repeating. The
  loop is broken when a NULL task is returned by the task-list system.
*/

TaskProcessorThread::~TaskProcessorThread() throw()
{
  // nothing to see here
}

void TaskProcessorThread::run() throw ()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  while(true)
    {
      Task *task = m_tasklist -> fetchTask();
      if(task==0)break;

      try
	{
	  task->doTask();
	}
      catch(AbortTask)
	{
	  // This is the only exception that is OK to throw from the Task
	}
      catch(...)
	{
	  // For all other exceptions, we just delete the task and
	  // continue processing. Don't know what else we can do, 
	  // actually we /could/ print out an error also. It's easy to 
	  // change, we'll see how it goes.
	  Debug::stream()
	    << "TaskProcessorThread::run: uncaught exception"
	    << std::endl;
	}
      
      delete(task);
    }
}

//-*-mode:c++; mode:font-lock;-*-

/**
 * \file Task.h
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

#ifndef VTASKNOTIFICATION_TASK_H
#define VTASKNOTIFICATION_TASK_H

#include <list>

#include <zthread/RecursiveMutex.h>
#include <zthread/Condition.h>
#include <zthread/Lockable.h>
#include <zthread/Thread.h>
#include <zthread/Runnable.h>

#include "TNExceptions.h"

namespace VTaskNotification
{
  // --------------------------------------------------------------------------
  // Task
  // --------------------------------------------------------------------------
  
  //! Base class for all tasks.
  class Task
  {
  public:
    virtual ~Task();
    
    //! Override this function in a derived class to do something.
    virtual void doTask() = 0;
  };

  // --------------------------------------------------------------------------
  // TaskListProcessorInterface
  // --------------------------------------------------------------------------

  //! Class providing the private interface to the task list system.
  class TaskListProcessorInterface
  {
  public:
    virtual ~TaskListProcessorInterface();

    //! Called by TaskProcessorThread objects fetch tasks from the queue
    virtual Task* fetchTask() = 0;
    
  protected:
    //! Protected constructor stops user instantating this class directly.
    TaskListProcessorInterface() {}
  };
  
  // --------------------------------------------------------------------------
  // TaskList
  // --------------------------------------------------------------------------

  //! Front end to task processor system.
  class TaskList: protected TaskListProcessorInterface, 
		  protected ZThread::RecursiveMutex,
		  protected ZThread::Condition
  {
  public:
    TaskList(int autothreads = 1): 
      TaskListProcessorInterface(), RecursiveMutex(), 
      Condition(*static_cast<ZThread::Lockable*>(this)),
      m_tasklist(), m_ntasksonlist(0), m_quit(false),
      m_threadslist(), m_nthreads(0), m_autothreads(autothreads), 
      m_nthreadswaiting(0) {}
    
    //! Destroy the task-list if there are no tasks on it
    virtual ~TaskList() throw();
    
    //! Add a task to the queue.
    bool scheduleTask(Task* task);
    
    //! Shut down task processing.
    void stopProcessing();
    
    //! Add threads to the task processing pool.
    void addProcessors(int n);
    
    //! Shortcut to addProcessors(1)
    void addProcessor() { addProcessors(1); }
    
  protected:
    //! Called by TaskProcessorThread objects fetch tasks from the queue
    virtual Task* fetchTask();
    
  private:
    typedef std::list<Task*> TaskPtrList_t;
    TaskPtrList_t   m_tasklist;
    int             m_ntasksonlist;
    
    bool            m_quit;
    
    typedef std::list<ZThread::Thread*> ThreadsList_t;
    ThreadsList_t   m_threadslist;  // list of all threads
    int             m_nthreads;     // number of threads we have working for us
    int             m_autothreads;  // max number of threads to auto-create
    int             m_nthreadswaiting; // number waiting for a task
    
    TaskList(const TaskList&);
    TaskList& operator=(const TaskList&);
  };
  
  // --------------------------------------------------------------------------
  // TaskProcessorThread
  // --------------------------------------------------------------------------
  
  //! The thread based task processor class.
  class TaskProcessorThread: public ZThread::Runnable
  {
  public:
    TaskProcessorThread(TaskListProcessorInterface* tasklist)
      : Runnable(), m_tasklist(tasklist) {}
    virtual ~TaskProcessorThread() throw ();
    
    //! The function spun off as a thread by Thread class
    virtual void run() throw ();
    
  private:
    TaskListProcessorInterface* m_tasklist;
  };
  
} // namespace VTaskNotification

#endif // VTASKNOTIFICATION_TASK_H
  

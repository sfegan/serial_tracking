//-*-mode:c++; mode:font-lock;-*-

/**
 * \file TNExceptions.h
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

#ifndef VTASKNOTIFICATION_TNEXCEPTIONS_H
#define VTASKNOTIFICATION_TNEXCEPTIONS_H

namespace VTaskNotification
{
  //! Stop processing a task or task/notificstion immediately
  /*! 
    Throwing this class from withing Task::doTask() or
    TaskNotification::doTask() immediately stops the processing of the
    task. In the case of a TaskNotification the notification is \b not
    put onto the notification list, throwing this exception is the only
    way to cancel the notification.
    
    \par Example
    
    \code
    MyTaskNotification::doTask()
    {
      // do some processing
    
      if(some_error_occurred)throw AbortTask(); // notification will NOT be sent
    
      // do some more processing to prepare for the notification
    }
    \endcode
  */
  class AbortTask 
  { 
  public: 
  };
} // namespace VTaskNotification
#endif // VTASKNOTIFICATION_TNEXCEPTIONS_H

//-*-mode:c++; mode:font-lock;-*-

/**
 * \file QtTaskNotification.h
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

#ifndef VTASKNOTIFICATION_QTTASKNOTIFICATION_H
#define VTASKNOTIFICATION_QTTASKNOTIFICATION_H

#include <qobject.h>
#include <QtNotification.h>
#include <TaskNotification.h>

namespace VTaskNotification
{
  //! Blah..
  class QtTaskNotificationSystem: 
    public QObject,
    public TaskNotificationSystem
  {
    Q_OBJECT 
    
  public:
    QtTaskNotificationSystem(int autothreads = 1):
      TaskNotificationSystem(QtNotificationList::getInstance(), autothreads) {}
    virtual ~QtTaskNotificationSystem();
  };
}// namespace VTaskNotification

#endif // VTASKNOTIFICATION_QTTASKNOTIFICATION_H

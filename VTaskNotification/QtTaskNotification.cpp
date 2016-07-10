//-*-mode:c++; mode:font-lock;-*-

/**
 * \file QtTaskNotification.cpp
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

#include <QtTaskNotification.h>

using namespace VTaskNotification;

/*! \class QtTaskNotificationSystem

  Not documented yet. Sorry!

 */

/*! \fn QtTaskNotificationSystem::QtTaskNotificationSystem(int autothreads = 1)

  \param authothreads Passed to the constructor of the TaskList.  The
  maximum number of threads that can exist and still allow the system
  to automatically spin off threads to process tasks as they are
  submitted. The default of \b 1 allows for the creation of one thread
  automatically when a task is added, unless the user explicitly
  creates one with addProcessors().
*/

QtTaskNotificationSystem::~QtTaskNotificationSystem() 
{
  // nothing to see here
}

//-*-mode:c++; mode:font-lock;-*-

/**
 * \file QtGUIThreadNotifier.cpp
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
 * $Date: 2007/01/22 20:45:12 $
 * $Revision: 2.2 $
 * $Tag$
 *
 **/

#include <iostream>

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <Debug.h>

#include <QtGUIThreadNotifier.h>

using namespace VMessaging;
using namespace VTaskNotification;

/*! \class QtGUIThreadNotifier

  Not documented yet. Sorry!

 */

// ----------------------------------------------------------------------------
// QtGUIThreadNotifier
// ----------------------------------------------------------------------------

/*! \class QtGUIThreadNotifier
 */

QtGUIThreadNotifier::QtGUIThreadNotifier(QObject* parent):
  QObject(parent),
  m_readfd(), m_writefd(), m_notifier()
{
  int fds[2];
  if(::pipe(fds) != 0)
    {
      Debug::stream()
	<< "QtGUIThreadNotifier::QtGUIThreadNotifier: "
	<< "error opening pipe" << std::endl << strerror(errno) << std::endl;
      assert(false);
    }
  
  m_readfd  = fds[0];
  m_writefd = fds[1];

  m_notifier = new QSocketNotifier( m_readfd, QSocketNotifier::Read, this );

  QObject::connect( m_notifier, SIGNAL(activated(int)),
		    this, SLOT(gotReaderNotification()) );
};

QtGUIThreadNotifier::~QtGUIThreadNotifier()
{
  QObject::disconnect( m_notifier, SIGNAL(activated(int)),
		       this, SLOT(gotReaderNotification()) );
  ::close(m_writefd);
  ::close(m_readfd);
}

/*!
  
*/
void QtGUIThreadNotifier::sendNotice()
{
  char x=0;
  ::write(m_writefd,&x,1);
}

/*!
  
*/
void QtGUIThreadNotifier::gotReaderNotification()
{
  char x=0;
  ::read(m_readfd,&x,1);
  emit(gotNotice());
}


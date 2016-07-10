//-*-mode:c++; mode:font-lock;-*-

/**
 * \file QtGUIThreadNotifier.h
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
 * $Revision: 2.1 $
 * $Tag$
 *
 **/

#ifndef VTASKNOTIFICATION_QTGUITHREADNOTIFIER_H
#define VTASKNOTIFICATION_QTGUITHREADNOTIFIER_H

#include <qobject.h>
#include <qsocketnotifier.h>

namespace VTaskNotification
{
  // --------------------------------------------------------------------------
  // QtGUIThreadNotifier
  // --------------------------------------------------------------------------
  
  //! Class that facilitates sending SIGNALS back to the GUI thread
  class QtGUIThreadNotifier: public QObject
  {
    Q_OBJECT
    
  public:
    QtGUIThreadNotifier(QObject* parent = 0);
    virtual ~QtGUIThreadNotifier();
    
  public slots:
    //! Send a signal to the GUI thread
    void sendNotice();

  signals:
    //! Emitted in the GUI thread when sendNotice is invoked
    void gotNotice();

  protected slots:
    void gotReaderNotification();

  protected:
    QtGUIThreadNotifier(const QtGUIThreadNotifier&);
    const QtGUIThreadNotifier& operator=(const QtGUIThreadNotifier&);

    int               m_readfd;
    int               m_writefd;
    QSocketNotifier*  m_notifier;
  };
} // VTaskNotification

#endif // VTASKNOTIFICATION_QTGUITHREADNOTIFIER_H

#include <TaskNotification.h>
#include <QtTaskNotification.h>

#include <iostream>

// ----------------------------------------------------------------------------
// GetTimeTask
// ----------------------------------------------------------------------------

#include <string.h>
#include <time.h>
#include <unistd.h>

#include <qobject.h>
#include <qstring.h>

// 
class GetTimeTask:public QObject, public TaskNotification
{
  Q_OBJECT

public:
  GetTimeTask(): QObject(), m_timestring(0) {}
  ~GetTimeTask() { delete m_timestring; }

  void doTask();
  void doNotification();

signals:
  void newTime(const QString& time);

private:
  char* m_timestring;   // Store the time between doTask() and doNotification()
};

void GetTimeTask::doTask()
{
  // get the time somehow, this could be from a CORBA server
  time_t t = time(0);
  struct tm* ltm = localtime(&t);
  m_timestring = strdup(asctime(ltm));  // copy the time string for later

  sleep(2);                             // block for two seconds
}

void GetTimeTask::doNotification()
{
  emit newTime(QString(m_timestring));
}

// ----------------------------------------------------------------------------
// MyApplication
// ----------------------------------------------------------------------------

#include <qapplication.h>
#include <qvbox.h>
#include <qmainwindow.h>
#include <qpushbutton.h>
#include <qlabel.h>

class MyApplication: public QMainWindow
{
  Q_OBJECT

public:
  MyApplication(QWidget* parent = 0);
  ~MyApplication() { m_tasksystem->stopProcessing(); }

private slots:
  void sendRequest();

private:
  TaskNotificationSystem* m_tasksystem;
  QLabel* m_timelabel;
};

MyApplication::MyApplication(QWidget* parent): QMainWindow(parent)
{
  QVBox *box = new QVBox(this);

  QPushButton* button = new QPushButton(box);
  button->setText("Get Time");
  connect(button,SIGNAL(clicked()),this,SLOT(sendRequest()));

  m_timelabel = new QLabel(box);
  m_timelabel->setText("Unknown");

  m_tasksystem = new QtTaskNotificationSystem(1);

  setCentralWidget(box);
}

void MyApplication::sendRequest()
{
  GetTimeTask* task = new GetTimeTask();
  connect(task, SIGNAL(newTime(const QString&)),
	  m_timelabel, SLOT(setText(const QString&)));
  
  m_tasksystem -> scheduleTaskNotification(task);
}

#include "qttasknotification.moc"

int main(int argc, char**argv)
{
  try
    {
      QApplication app(argc,argv);
      
      MyApplication widget;
      app.setMainWidget(&widget);
      widget.show();
      app.exec();
    }
  catch(...)
    {
      std::cerr << "Uncaught exception" << std::endl;
    }
}

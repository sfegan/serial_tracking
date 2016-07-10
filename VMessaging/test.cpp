//-*-mode:c++; mode:font-lock;-*-

/**
 * \file test.cpp
 * \ingroup VMessaging
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

#include<qapplication.h>
#include<qvbox.h>
#include<qpushbutton.h>

#include"Message.h"
#include"StreamMessenger.h"
#include"QtMessenger.h"
#include"QtDialogMessenger.h"
#include"QtTextEditMessenger.h"

using namespace VMessaging;

class SillyWidget: public QVBox
{
  Q_OBJECT
public:
  SillyWidget(QWidget* parent=0,const char* name=0);
public slots:
  void send();
};

#include"test.moc"

SillyWidget::SillyWidget(QWidget* parent,const char* name): QVBox(parent,name)
{ 
  Messenger::relay()->registerMessenger(QtMessenger::instance());

  QtDialogMessenger* dm = 
    new QtDialogMessenger(Message::PS_ROUTINE, this, "dm");
  connect(QtMessenger::instance(),SIGNAL(message(const Message&)),
	  dm,SLOT(sendMessage(const Message&)));

  QtTextEditMessenger* lem = new QtTextEditMessenger(this, "lem");
  connect(QtMessenger::instance(),SIGNAL(message(const Message&)),
	  lem,SLOT(sendMessage(const Message&)));

  QPushButton* pb = new QPushButton("Press me", this, "pb");
  connect(pb,SIGNAL(clicked()),this,SLOT(send()));
}
  
void SillyWidget::send()
{
  Message msg(Message::DR_LOCAL, Message::PS_CRITICAL,"Test Message",
	      "SillyWidget::send()");
  msg.messageStream() << "This is a test message. Please ignore it.\n"
		      << "Hope you are having a nice day. Bye then.";
  msg.detailsStream() << "Details details details";
  
  Messenger::relay()->sendMessage(msg);
}

int main(int argc, char** argv)
{
  Message::setProgram(*argv);
  StreamMessenger cout_messenger;
  Messenger::relay()->registerMessenger(&cout_messenger);

  QApplication app(argc,argv);
  SillyWidget widget(0,"main");
  widget.show();
  app.exec();
}

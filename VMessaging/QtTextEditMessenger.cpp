//-*-mode:c++; mode:font-lock;-*-

/**
 * \file QtTextEditMessenger.cpp
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

#include<iostream>

#include<qcolor.h>

#include"QtTextEditMessenger.h"

using namespace VMessaging;

QtTextEditMessenger::
QtTextEditMessenger(QWidget* parent, const char* name) throw(): 
  QTextEdit(parent, name), Messenger()
{
  setReadOnly(true);
}

QtTextEditMessenger::~QtTextEditMessenger() throw()
{
  // nothing to see here
}

bool QtTextEditMessenger::sendMessage(const Message& message) throw()
{
  setColor(blue);
  setBold(true);

  std::string title;
  if((message.time().tv_sec!=0)||(message.time().tv_usec!=0))
    {
      char ctime_buf[26];
      ctime_r(&message.time().tv_sec,ctime_buf);
      ctime_buf[strlen(ctime_buf)-1]='\0';
      title+=std::string(ctime_buf)+=std::string(": ");
    }
  title+=message.title();
  append(title.c_str());

  Message::PayloadSignificance significance = message.significance();
  if(significance==Message::PS_CRITICAL)setColor(red);
  else setColor(black);

  if((significance==Message::PS_EXCEPTIONAL)||
     (significance==Message::PS_CRITICAL))setBold(true);
  else setBold(false);

  std::ostringstream or_str;
  switch(message.origin())
    {
    case Message::MO_LOCAL: 
      or_str << "LOCAL";
      break;
    case Message::MO_REMOTE: 
      or_str << "REMOTE (" << message.zone() << ')';
      break;
    }

  std::string msg = message.message();
  std::string det = message.details();
  std::string fun = message.function();
  std::string pro = 
    message.program()+std::string(" (")+message.hostname()+std::string(")");
  
  if(pro!="")
    {
      append(or_str.str()+std::string(" Program: ")+pro.c_str());
    }

  append(msg.c_str());

  if(det!="")
    {
      setUnderline(true);
      append("Details:\n");
      setUnderline(false);
      append(det.c_str());
    }

  if(fun!="")
    {
      setUnderline(true);
      append("Function backtrace:\n");
      setUnderline(false);
      append(fun.c_str());
    }

  append("\n");

  return true;
}

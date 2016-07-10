//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUIMessengerPane.h
 * \ingroup VTracking
 * \brief This is a one-line description of this cpp file.
 *
 * Here is a tedious verbose multi-line description of all
 * the details of the code, more than you would
 * ever want to read. Generally, all the important documentation
 * goes in the .cpp files.
 *
 * Original Author: Stephen Fegan
 * $Author: sfegan $
 * $Date: 2006/07/17 14:25:03 $
 * $Revision: 2.1 $
 * $Tag$
 *
 **/

#ifndef VTRACKING_GUIMESSENGERPANE_H
#define VTRACKING_GUIMESSENGERPANE_H

#include<vector>

#include<QtTextEditMessenger.h>
#include"GUITabWidget.h"

class GUIMessengerPane: public VMessaging::QtTextEditMessenger, public GUITabPane
{
  Q_OBJECT

public:
  GUIMessengerPane(QWidget* parent=0, const char* name=0):
    QtTextEditMessenger(parent,name), GUITabPane(this) { }
  virtual ~GUIMessengerPane() throw();

  virtual void update(const GUIUpdateData& ud);
}; // class GUIMessengerPane

#endif // VTRACKING_GUIMESSENGERPANE_H

//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUIAboutPane.h
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

#ifndef VTRACKING_GUIABOUTPANE_H
#define VTRACKING_GUIABOUTPANE_H

#include<qlineedit.h>
#include<qlabel.h>
#include<qwidgetstack.h>
#include<qpushbutton.h>
#include<qradiobutton.h>
#include<qbuttongroup.h>
#include<qframe.h>

#if 0
#include<qgroupbox.h>
#include<qtimer.h>
#include<qtabwidget.h>
#include<qspinbox.h>
#include<qhbox.h>
#include<qcheckbox.h>
#endif

#include"GUITabWidget.h"
#include"GUIMisc.h"

class GUIAboutPane: 
  public QFrame, public GUITabPane
{
  Q_OBJECT
public:
  GUIAboutPane(QWidget* parent=0, const char* name=0);
  virtual ~GUIAboutPane();
  
  void update(const GUIUpdateData& ud);
};

#endif // VTRACKING_GUIABOUTPANE_H

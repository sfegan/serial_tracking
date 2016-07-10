//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUIAboutPane.cpp
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

#include<qlayout.h>
#include<qlabel.h>
#include<qtabwidget.h>
#include<qimage.h>
#include<qtextedit.h>

#include "GUITabWidget.h"
#include "GUIAboutPane.h"
#include "GUIMisc.h"

#include"Version.h"

#include"gpl.h"
#include"asa_license.h"
#include"todo.h"

#include"pixmaps/veritas.xpm"

GUIAboutPane::GUIAboutPane(QWidget* parent, const char* name):
  QFrame(parent,name), GUITabPane(this)
{
  QString basename(name);

  QGridLayout* af1_l = 
    new QGridLayout(this,1,1,15,0,basename+" frame 1 layout");
  
  QFrame* af2 = new QFrame(this,basename+" frame 2");
  af2->setFrameShape(QFrame::Box);
  af2->setLineWidth(2);
  QGridLayout* af2_l = 
    new QGridLayout(af2,2,2,20,20,basename+" frame 2 layout");
  af1_l->addWidget(af2,0,0);
  
  QImage avim(const_cast<const char**>(veritas));
  QImage avsm=avim.smoothScale(90*avim.width()/avim.height(),
			       90,QImage::ScaleMin);
  QLabel* avlab = new QLabel(af2,basename+" pixmap");
  avlab->setPixmap(avsm);
  QLabel* alab =
    new QLabel("VERITAS Telescope Controller\n"
	       "Stephen Fegan\nVersion " VERSION ", " VERSIONDATE "\n",
	       af2,basename+" label");
  
  alab->setFont(QFont("Helvetica",18,75,false));
  //alab->setPaletteForegroundColor(QColor(0,51,102));
  
  QTabWidget* atw = new QTabWidget(af2,basename+" tabwidget");
  
  QTextEdit* gpltext = new QTextEdit(atw,basename+" gpl");
  gpltext->setReadOnly(true);
  gpltext->setBold(true);
  gpltext->append("This program is distributed under the "
		  "terms of the GPL v2.\n\n");
  gpltext->setBold(false);
  gpltext->setFamily("Fixed");
  gpltext->setPointSize(10);
  gpltext->setWordWrap(QTextEdit::NoWrap);
  gpltext->append(GPL);
  gpltext->setCursorPosition(0,0);
  atw->addTab(gpltext,"License");
  
  QTextEdit* asatext = new QTextEdit(atw,basename+" asa");
  asatext->setReadOnly(true);
  asatext->setBold(true);
  asatext->append("This program uses the Adaptive Simulated Annealing (ASA) "
		  "library by Lester Ingber.\nThese portions are disributed "
		  "under the following terms.\n\n");
  asatext->setBold(false);
  asatext->setFamily("Fixed");
  asatext->setPointSize(10);
  asatext->setWordWrap(QTextEdit::NoWrap);
  asatext->append(ASA_LICENSE);
  asatext->setCursorPosition(0,0);
  atw->addTab(asatext,"ASA");
  
  QTextEdit* todotext = new QTextEdit(atw,basename+" todo");
  todotext->setReadOnly(true);
  todotext->setFamily("Fixed");
  todotext->setPointSize(10);
  todotext->setWordWrap(QTextEdit::NoWrap);
  todotext->append(TODO);
  todotext->setCursorPosition(0,0);
  
  atw->addTab(todotext,"History");

  af2_l->addWidget(avlab,0,0);
  af2_l->addWidget(alab,0,1);
  af2_l->addMultiCellWidget(atw,1,1,0,1);
  
  af2_l->setColStretch(0,0);
  af2_l->setColStretch(1,1);
  af2_l->setRowStretch(0,0);
  af2_l->setRowStretch(1,1);
}

GUIAboutPane::~GUIAboutPane()
{
  // nothing to see here
}

void GUIAboutPane::update(const GUIUpdateData& ud)
{
  // nothing to see here
}

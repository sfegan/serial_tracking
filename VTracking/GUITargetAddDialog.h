//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUITargetAddDialog.h
 * \ingroup VTracking
 * \brief Add target to DB
 *
 * Original Author: Stephen Fegan
 * Start Date: 2007-04-03
 * $Author: sfegan $
 * $Date: 2007/04/06 16:04:48 $
 * $Revision: 2.2 $
 * $Tag$
 *
 **/

#include <vector>
#include <string>

#include <qlineedit.h>
#include <qcombobox.h>
#include <qpushbutton.h>

#include <VOmniORBHelper.h>

#include "GUITargetAddDialogUI.h"

class GUITargetAddDialog: public GUITargetAddDialogUI
{
  Q_OBJECT

public:
  static void addTargets(VCorba::VOmniORBHelper* orb = 0,
			 QWidget* parent = 0, const char* name = 0);

protected:
  GUITargetAddDialog(VCorba::VOmniORBHelper* orb = 0,
		     QWidget* parent = 0, const char* name = 0, 
		     bool modal = FALSE, WFlags fl = 0);

protected slots:
  void searchForObject();
  void reset();
  void add();
  void sourceTextChanged(const QString& txt);
  void positionTextChanged(const QString& txt);

private:
  VCorba::VOmniORBHelper*  m_orb;
  std::vector<std::string> m_collections;
};

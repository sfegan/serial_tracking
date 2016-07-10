//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUITargetCollectionManagerDialog.h
 * \ingroup VTracking
 * \brief Target collection manager selector
 *
 * Original Author: Stephen Fegan
 * Start Date: 2007-07-04
 * $Author: sfegan $
 * $Date: 2007/07/05 20:14:00 $
 * $Revision: 2.1 $
 * $Tag$
 *
 **/

#include <vector>
#include <map>
#include <string>

#include <qlineedit.h>
#include <qlistbox.h>
#include <qcombobox.h>
#include <qpushbutton.h>

#include "GUITargetDialogs.h"
#include "GUITargetCollectionManagerDialogUI.h"

class GUITargetCollectionManagerDialog: 
  public GUITargetCollectionManagerDialogUI
{
  Q_OBJECT

public:
  static void manageTargets(const std::string& default_collection = 
			    GUITargetDialogs::getDefaultDBCollectionName(),
			    QWidget* parent = 0, const char* name = 0,
			    const QString& caption = 
			    "Modify targets in collection");

protected:
  GUITargetCollectionManagerDialog(const std::string& default_collection = 
				GUITargetDialogs::getDefaultDBCollectionName(),
				   QWidget* parent = 0, const char* name = 0, 
				   bool modal = FALSE, WFlags fl = 0);
  
signals:
  void hasTargetCollection(bool);

protected slots:
  void addButtonPressed();
  void removeButtonPressed();
  void sourceCollectionChanged();
  void targetCollectionChanged();

private:
  std::vector<std::string>  m_source_collections;
  std::map<std::string,int> m_source_items;
  std::vector<std::string>  m_target_collections;
  std::map<std::string,int> m_target_items;
};

//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUITargetCollectionDialog.h
 * \ingroup VTracking
 * \brief Target collection selector
 *
 * Original Author: Stephen Fegan
 * Start Date: 2007-04-03
 * $Author: sfegan $
 * $Date: 2007/07/05 20:14:00 $
 * $Revision: 2.2 $
 * $Tag$
 *
 **/

#include <vector>
#include <string>

#include "GUITargetDialogs.h"
#include "GUITargetCollectionDialogUI.h"

class GUITargetCollectionDialog: public GUITargetCollectionDialogUI
{
  Q_OBJECT

public:
  GUITargetCollectionDialog(const std::string& default_collection = 
			    GUITargetDialogs::getDefaultDBCollectionName(),
			    QWidget* parent = 0, const char* name = 0, 
			    bool modal = FALSE, WFlags fl = 0);

  std::string selected();

  static std::string 
  getCollection(const std::string& default_collection = 
		GUITargetDialogs::getDefaultDBCollectionName(),
		QWidget* parent = 0, const char* name = 0,
		const QString& caption = "Select collection");

private:
  std::vector<std::string> m_collections;
};

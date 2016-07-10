//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUICorrectionDateDialog.h
 * \ingroup VTracking
 * \brief Target collection selector
 *
 * Original Author: Stephen Fegan
 * Start Date: 2007-04-03
 * $Author: sfegan $
 * $Date: 2007/07/19 04:35:12 $
 * $Revision: 2.1 $
 * $Tag$
 *
 **/

#include <vector>
#include <string>

#include <VATime.h>

#include "GUITargetDialogs.h"
#include "GUITargetCollectionDialogUI.h"

class GUICorrectionDateDialog: public GUITargetCollectionDialogUI
{
  Q_OBJECT

public:
  GUICorrectionDateDialog(unsigned scope_id,
			  QWidget* parent = 0, const char* name = 0, 
			  bool modal = FALSE, WFlags fl = 0);

  const VERITAS::VATime selected() const;

  static VERITAS::VATime loadFromFileTime() { return VERITAS::VATime(1,0); }
  static VERITAS::VATime cancelLoadTime() { return VERITAS::VATime(2,0); }

  static VERITAS::VATime
  getCorrectionDate(unsigned scope_id,
		    QWidget* parent = 0, const char* name = 0,
		    const QString& caption = "Select corrections");

private:
  std::vector<VERITAS::VATime> m_dates;
};

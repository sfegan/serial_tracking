//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUIResolvedTargetSelector.h
 * \ingroup VTracking
 * \brief Select one target out of list
 *
 * Original Author: Stephen Fegan
 * Start Date: 2007-04-06
 * $Author: sfegan $
 * $Date: 2007/04/06 22:57:17 $
 * $Revision: 2.1 $
 * $Tag$
 *
 **/

#include <vector>
#include <string>

#include <qlineedit.h>
#include <qcombobox.h>
#include <qpushbutton.h>

#include <VOmniORBHelper.h>

#include "NET_VAstroDBResolver.h"
#include "GUIResolvedTargetSelectorUI.h"

class GUIResolvedTargetSelector: public GUIResolvedTargetSelectorUI
{
  Q_OBJECT

public:
  enum Status { REJECTED, SELECTED, MANUAL };

  ~GUIResolvedTargetSelector();

  static Status 
  selectFromObjects(const VAstroDBResolver::ObjectInfoSeq& objects,
		    unsigned& selected,
		    QWidget* parent = 0, const char* name = 0);

protected:
  GUIResolvedTargetSelector(const VAstroDBResolver::ObjectInfoSeq& objects,
			    QWidget* parent = 0, const char* name = 0, 
			    bool modal = FALSE, WFlags fl = 0);

protected slots:
  void select();
  void manual();
};

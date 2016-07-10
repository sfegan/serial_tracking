//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUIMessengerPane.cpp
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

#include"GUIMessengerPane.h"

using namespace VTracking;

GUIMessengerPane::~GUIMessengerPane() throw()
{
  // nothing to see here
}

void GUIMessengerPane::update(const GUIUpdateData& ud)
{
  if(!isVisible())return;
  setEnabled(true);
}


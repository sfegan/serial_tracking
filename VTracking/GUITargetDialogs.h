//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUITargetDialogs.h
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
 * $Date: 2007/08/13 19:49:49 $
 * $Revision: 2.4 $
 * $Tag$
 *
 **/

#ifndef VTRACKING_GUITARGETDIALOGS_H
#define VTRACKING_GUITARGETDIALOGS_H

#include<set>
#include<vector>
#include<string>
#include<qwidget.h>

#include"TargetObject.h"

class GUITargetDialogs
{
public:

  static std::set<std::string> getProtectedCollections();

  static bool loadDefault(VTracking::TargetList& list, 
			  bool use_messenger_only = false);

  static std::string getDefaultDBCollectionName() { return "primary_targets"; }

  static bool loadFromDBCollection(VTracking::TargetList& list,
				   bool use_messenger_only = false,
				   QWidget* parent = 0, const char* name = 0);
  
  static bool loadFromDBCollection(const std::string& collection,
				   VTracking::TargetList& list,
				   bool use_messenger_only = false,
				   QWidget* parent = 0, const char* name = 0);
  
  static std::string getDefaultFileName() { return "sources.trk"; }

  static bool loadFromFile(VTracking::TargetList& list,
			   QWidget* parent = 0, const char* name = 0);

  static bool loadFromFile(const std::string& file,
			   VTracking::TargetList& list);
};

#endif // defined VTRACKING_GUITARGETDIALOGS_H

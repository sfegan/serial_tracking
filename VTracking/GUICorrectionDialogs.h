//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUICorrectionDialogs.h
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
 * $Date: 2007/07/19 22:30:13 $
 * $Revision: 2.2 $
 * $Tag$
 *
 **/

#ifndef VTRACKING_GUICORRECTIONDIALOGS_H
#define VTRACKING_GUICORRECTIONDIALOGS_H

#include<vector>
#include<string>
#include<qwidget.h>

#include"VATime.h"
#include"CorrectionParameters.h"

class GUICorrectionDialogs
{
public:

  enum Status { S_FAIL, S_GOOD, S_CANCEL };

  // --------------------------------------------------------------------------
  // GENERAL
  // --------------------------------------------------------------------------

  static Status loadDefault(SEphem::CorrectionParameters& cp, 
			    unsigned scope_id);

  static Status save(const SEphem::CorrectionParameters& cp, 
		     unsigned scope_id,
		     QWidget* parent = 0, const char* name = 0);

  // --------------------------------------------------------------------------
  // DATABASE
  // --------------------------------------------------------------------------

  static Status loadFromDB(SEphem::CorrectionParameters& cp,
			   unsigned scope_id,
			   QWidget* parent = 0, const char* name = 0);

  static Status loadFromDB(const VERITAS::VATime& ref_time,
			   SEphem::CorrectionParameters& cp, 
			   unsigned scope_id);

  static Status saveToDB(const SEphem::CorrectionParameters& cp, 
			 unsigned scope_id,
			 QWidget* parent = 0, const char* name = 0);

  static Status saveToDB(const SEphem::CorrectionParameters& cp, 
			 const std::string& comment,
			 unsigned scope_id);

  // --------------------------------------------------------------------------
  // FILENAME
  // --------------------------------------------------------------------------

  static std::string getDefaultLoadFileName(unsigned scope_id);
  static std::string getDefaultSaveFileName(unsigned scope_id);

  static Status loadFromFile(SEphem::CorrectionParameters& cp, 
			     unsigned scope_id,
			     QWidget* parent = 0, const char* name = 0);

  static Status loadFromFile(const std::string& file,
			     SEphem::CorrectionParameters& cp);

  static Status saveToFile(const SEphem::CorrectionParameters& cp, 
			   unsigned scope_id,
			   QWidget* parent = 0, const char* name = 0);

  static Status saveToFile(const std::string& file,
			   const SEphem::CorrectionParameters& cp);
};

#endif // defined VTRACKING_GUICORRECTIONDIALOGS_H

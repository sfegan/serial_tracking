//-*-mode:c++; mode:font-lock;-*-

/**
 * \file ASA.h
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
 * $Date: 2006/04/04 17:06:58 $
 * $Revision: 2.0 $
 * $Tag$
 *
 **/

#ifndef ASA_H
#define ASA_H

#define OPTIONS_FILE FALSE
#define OPTIONS_FILE_DATA FALSE
#define COST_FILE FALSE
#define ASA_LIB TRUE
#define OPTIONAL_DATA_PTR TRUE
#define OPTIONAL_PTR_TYPE void
#define ASA_PRINT FALSE

//#include"ASA/asa.h"
#include"ASA/asa_usr_asa.h"

namespace ASA
{
  void ASAMain(void* data_ptr,
	       double (*user_cost_function)
	       (double *, double *, double *, double *, double *, 
		ALLOC_INT *, int *, int *, int *, USER_DEFINES *),
	       ALLOC_INT nparameters, double *parameter_initial_final, 
	       double *parameter_minimum, double *parameter_maximum);
} // namespace ASA

#endif // defined ASA_H

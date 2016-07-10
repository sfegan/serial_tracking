//-*-mode:c++; mode:font-lock;-*-

/**
 * \file ASA.cpp
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

#include"ASA.h"
#include"RNG.h"

#include"ASA/asa.c"

static double my_rand(LONG_INT* seed)
{
  return RNG::NRRan2::makeLinDev();
}

void ASA::ASAMain(void* data_ptr,
		  double (*user_cost_function)
		  (double *, double *, double *, double *, double *, 
		   ALLOC_INT *, int *, int *, int *, USER_DEFINES *),
		  ALLOC_INT nparameters, double *parameter_initial_final, 
		  double *parameter_minimum, double *parameter_maximum)
{
  int* parameter_type = new int[nparameters];
  for (int index = 0; index < nparameters; ++index)
    parameter_type[index] = REAL_TYPE;

  double* tangents = new double[nparameters];
  double* curvature = new double[nparameters*nparameters];

  LONG_INT seed;

  USER_DEFINES options;

  options.Asa_Data_Dim_Ptr = 1;
  options.Asa_Data_Ptr=data_ptr;

  /* options.Limit_Acceptances = 10000; */
  options.Limit_Acceptances = 1000;
  options.Limit_Generated = 99999;
  options.Limit_Invalid_Generated_States = 1000;
  /* options.Accepted_To_Generated_Ratio = 1.0E-6; */
  options.Accepted_To_Generated_Ratio = 1.0E-4;

  options.Cost_Precision = 1.0E-18;
  options.Maximum_Cost_Repeat = 5;
  options.Number_Cost_Samples = 5;
  options.Temperature_Ratio_Scale = 1.0E-5;
  options.Cost_Parameter_Scale_Ratio = 1.0;
  options.Temperature_Anneal_Scale = 100.0;

  options.Include_Integer_Parameters = FALSE;
  options.User_Initial_Parameters = FALSE;
  options.Sequential_Parameters = -1;
  options.Initial_Parameter_Temperature = 1.0;

  options.Acceptance_Frequency_Modulus = 100;
  options.Generated_Frequency_Modulus = 10000;
  options.Reanneal_Cost = 1;
  options.Reanneal_Parameters = TRUE;

  options.Delta_X = 0.001;
  options.User_Tangents = FALSE;
  options.Curvature_0 = FALSE;

  int exit_status;
  int valid_state;

  asa(user_cost_function, 
      &my_rand, 
      &seed, 
      parameter_initial_final, 
      parameter_minimum, 
      parameter_maximum, 
      tangents, 
      curvature, 
      &nparameters, 
      parameter_type,
      &valid_state, 
      &exit_status, 
      &options);

  delete[] curvature;
  delete[] tangents;
  delete[] parameter_type;
}

//-*-mode:c++; mode:font-lock;-*-

/**
 * \file ScopeAPI.cpp
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

#include<iostream>
#include<sstream>
#include<iomanip>

#include"ScopeAPI.h"

using namespace VTracking;

const double ScopeAPI::sc_az_max_vel;
const double ScopeAPI::sc_el_max_vel;

ScopeAPI::~ScopeAPI()
{
  // nothing to see here
}

ScopeAPI::ScopeAPIError::~ScopeAPIError() throw()
{
  // nothing to see here
}

void ScopeAPI::PositionerStatus::print(std::ostream& stream)
{

}
    
void ScopeAPI::DriveStatus::print(std::ostream& stream)
{

}

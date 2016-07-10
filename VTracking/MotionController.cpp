//-*-mode:c++; mode:font-lock;-*-

/**
 * \file MotionController.cpp
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
#include<iomanip>
#include<sstream>

#include"MotionController.h"

using namespace VTracking;

// ----------------------------------------------------------------------------
// DualAxisController
// ----------------------------------------------------------------------------

DualAxisController::
DualAxisController(TelescopeMotionLimits& limits, 
		   double az_max_vel, double az_max_acc,
		   double az_p, double az_i, double az_d, unsigned az_l, 
		   double el_max_vel, double el_max_acc,
		   double el_p, double el_i, double el_d, unsigned el_l):
  m_limits(limits), 
  m_az_axis(az_max_vel, az_max_acc, az_p, az_i, az_d, az_l),
  m_el_axis(el_max_vel, el_max_acc, el_p, el_i, el_d, el_l)
{
  //  nothing to see here
}

DualAxisController::~DualAxisController()
{
  //  nothing to see here
}

void DualAxisController::
posMode(double& az_vel, double& el_vel, double az_ang, double el_ang,
	double az_req_ang, double az_req_vel, double az_req_max_vel,
	double el_req_ang, double el_req_vel, double el_req_max_vel,
	double delta_t)
{
  bool az_lim_neg;
  bool az_lim_pos;
  bool el_lim_neg; 
  bool el_lim_pos;
  m_limits.isDriveAngleInsideLimits(az_ang, el_ang, 
		       az_lim_neg, az_lim_pos, el_lim_neg, el_lim_pos);
  az_vel=m_az_axis.posMode(az_req_ang, az_req_vel, az_req_max_vel, 
			   delta_t, az_vel, az_ang, az_lim_neg, az_lim_pos);
  el_vel=m_el_axis.posMode(el_req_ang, el_req_vel, el_req_max_vel, 
			   delta_t, el_vel, el_ang, el_lim_neg, el_lim_pos);
}

void DualAxisController::
velMode(double& az_vel, double& el_vel, double az_ang, double el_ang,
	double az_req_vel, double el_req_vel, double delta_t)
{
  bool az_lim_neg;
  bool az_lim_pos;
  bool el_lim_neg; 
  bool el_lim_pos;
  m_limits.isDriveAngleInsideLimits(az_ang, el_ang, 
		       az_lim_neg, az_lim_pos, el_lim_neg, el_lim_pos);
  az_vel=m_az_axis.velMode(az_req_vel,
			   delta_t, az_vel, az_lim_neg, az_lim_pos);
  el_vel=m_el_axis.velMode(el_req_vel,
			   delta_t, el_vel, el_lim_neg, el_lim_pos);
}

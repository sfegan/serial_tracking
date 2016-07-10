//-*-mode:c++; mode:font-lock;-*-

/**
 * \file TelescopeMotionLimits.cpp
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
 * $Date: 2006/11/11 19:26:48 $
 * $Revision: 2.3 $
 * $Tag$
 *
 **/

#include<iostream>

#include<Debug.h>

#include"TelescopeMotionLimits.h"

using namespace VMessaging;
using namespace VTracking;

// ============================================================================
// TelescopeMotionLimits
// ============================================================================

TelescopeMotionLimits::~TelescopeMotionLimits()
{
  // nothing to see here
}

// ============================================================================
// PrimaryLimits
// ============================================================================

PrimaryLimits::~PrimaryLimits()
{
  // nothing to see here
}
    
bool PrimaryLimits::
isDriveAngleInsideLimits(double az_driveangle, double el_driveangle,
			 bool& az_lim_cc, bool& az_lim_cw, 
			 bool& el_lim_dn, bool& el_lim_up)
{
  el_lim_up = el_driveangle>m_up_lim;
  el_lim_dn = el_driveangle<m_dn_lim;
  az_lim_cw = az_driveangle>m_cw_lim;
  az_lim_cc = az_driveangle<m_cc_lim;
  return !(el_lim_up||el_lim_dn||az_lim_cw||az_lim_cc);
}

bool PrimaryLimits::
modifyTargetDriveAngle(double tel_az, double tel_el,
		       double& tar_az, double& tar_el,
		       double& az_speed, double& el_speed)
{
  // Strategy: If target position is out of bounds then bring it in.
  // If telescope is LOW then slew straight up from current position,
  // no motion in Az is allowed. If telescope is outside of other 
  // limits then we try to come back in.

  bool modified = false;
  if(tar_el>m_up_lim)tar_el = m_up_lim, modified = true;
  if(tar_el<m_dn_lim)tar_el = m_dn_lim, modified = true;
  if(tar_az>m_cw_lim)tar_az = m_cw_lim, modified = true;
  if(tar_az<m_cc_lim)tar_az = m_cc_lim, modified = true;

  if(tel_el<m_dn_lim)
    {
      if((!m_slewup)||(fabs(m_slewup_az-tel_az)>1.0*ANGLE_RADPERDEG))
	m_slewup = true, m_slewup_az = tel_az;
      tar_az = m_slewup_az;
#if 0
      tar_el = m_dn_lim; // see comment below
#endif
      return true;
    }
  else
    {
      m_slewup = false;
    }

#if 0
  // What do we gain from these tests? If the target was outside the
  // range then it was already been set to be back inside -- with
  // these in here a target that is convincingly inside the allowable
  // zone is set to be just at the edge if the telescope is outside
  // the allowable region.  How does that help anyone? We might as
  // well keep the convincingly inside target.
  if(tel_el>m_up_lim)tar_el = m_up_lim, modified = true;
  if(tel_az>m_cw_lim)tar_az = m_cw_lim, modified = true;
  if(tel_az<m_cc_lim)tar_az = m_cc_lim, modified = true;
#endif

  return modified;
}

// ============================================================================
// KeyLimits
// ============================================================================

KeyLimits::~KeyLimits()
{
  // nothing to see here
}

bool KeyLimits::
isDriveAngleInsideLimits(double az_driveangle, double el_driveangle,
			 bool& az_lim_cc, bool& az_lim_cw, 
			 bool& el_lim_dn, bool& el_lim_up)
{
  bool azkey = (az_driveangle<=m_key_az_cw)&&(az_driveangle>=m_key_az_cc);
  bool elkey = (el_driveangle<=m_lim_el_dn);

  el_lim_up = el_driveangle>m_lim_el_up;
  el_lim_dn = (((azkey)&&(el_driveangle<m_key_el_dn))
	       ||((!azkey)&&(el_driveangle<m_lim_el_dn)));
  az_lim_cw = (((elkey)&&(az_driveangle>m_key_az_cw))
	       ||(!elkey)&&(az_driveangle>m_lim_az_cw));
  az_lim_cc = (((elkey)&&(az_driveangle<m_key_az_cc))
	       ||(!elkey)&&(az_driveangle<m_lim_az_cc));

  return !(el_lim_up||el_lim_dn||az_lim_cw||az_lim_cc);
}
    
bool KeyLimits::
modifyTargetDriveAngle(double tel_az, double tel_el,
		       double& tar_az, double& tar_el,
		       double& az_speed, double& el_speed)
{
  if(tel_az<=m_key_az_cw && tel_az>=m_key_az_cc &&
     tar_az<=m_key_az_cw && tar_az>=m_key_az_cc)
    return false;
  else if
    ((tel_az<=m_key_az_cw && tel_az>=m_key_az_cc && tel_el<m_lim_el_dn)^
     (tar_az<=m_key_az_cw && tar_az>=m_key_az_cc && tar_el<m_lim_el_dn))
    {
      tar_az = 0.5*(m_key_az_cw + m_key_az_cc);
      if(tar_el < m_lim_el_dn)tar_el = m_lim_el_dn;
      return true;
    }
  else
    return false;
}

// ============================================================================
// RestrictedLowElevationMotion
// ============================================================================

RestrictedLowElevationMotion::~RestrictedLowElevationMotion()
{
  // nothing to see heretel_el<m_lo_el
}

bool RestrictedLowElevationMotion::
isDriveAngleInsideLimits(double az_driveangle, double el_driveangle,
			 bool& az_lim_cc, bool& az_lim_cw, 
			 bool& el_lim_dn, bool& el_lim_up)
{
  az_lim_cc = false;
  az_lim_cw = false;
  el_lim_dn = false;
  el_lim_up = false;
  return true;
}

bool RestrictedLowElevationMotion::
modifyTargetDriveAngle(double tel_az, double tel_el,
		       double& tar_az, double& tar_el,
		       double& az_speed, double& el_speed)
{
  // Strategy:
  //
  // 1) If telecope is below the minimum slew elevation (MSE) and the target
  //    is far in azimuth slew directly up. The initial azimuth is remembered
  //    so that we do not "chase" the servo position. If the "remembered"
  //    position is far then we toss it out and remember the current one.
  //    We slew slightly above the MSE to stop oscillations.
  // 2) Else, if the target position is below the MSE (+the oscillation
  //    margin) and we are far from the target then we slew to the target
  //    azimuth, slightly above the MSE
  // 3) Else, it must be the case that either we are above the MSE AND
  //    the target is above the MSE OR we are close in azimuth so we just
  //    slew to the target

  if((tel_el<m_low_el)&&(fabs(tel_az-tar_az)>m_max_az_slew))
    {
      if((!m_slewup)||(fabs(m_slewup_az-tel_az)>m_max_az_slew))
	m_slewup = true, m_slewup_az = tel_az;
      tar_az = m_slewup_az;
      tar_el = m_low_el + m_el_hysteresis;
      return true;
    }
  else if((tar_el<(m_low_el+m_el_hysteresis))
	  &&(fabs(tel_az-tar_az)>m_max_az_slew))
    {
      m_slewup = false;
      tar_el = m_low_el + m_el_hysteresis;
      return true;
    }
  else 
    {
      m_slewup = false;
      return false;
    }
}

// ============================================================================
// NotchedInclusionimits
// ============================================================================

NotchedInclusionLimits::~NotchedInclusionLimits()
{
  // nothing to see here
}

bool NotchedInclusionLimits::
isDriveAngleInsideLimits(double az_driveangle, double el_driveangle,
			 bool& az_lim_cc, bool& az_lim_cw, 
			 bool& el_lim_dn, bool& el_lim_up)
{
  double dn_limit = dnLimit(az_driveangle);
  az_lim_cc = false;
  az_lim_cw = false;
  el_lim_dn = el_driveangle<dn_limit;
  el_lim_up = false;
  return !el_lim_dn;
}

bool NotchedInclusionLimits::
modifyTargetDriveAngle(double tel_az, double tel_el,
		       double& tar_az, double& tar_el,
		       double& az_speed, double& el_speed)
{
  // Strategy: 
  // 1) If the target position (TP) is below the limit then set to the limit
  // 2) If the current position (CP) is below the limit then slew up
  // 3) Otherwise, if the CP is below the general slew limit and the TP is
  //    not in the same notch as the telescope slew up to the CP notch
  // 4) Else, if the target position is below the slew limit, in a different
  //    notch, then slew to the top of the TP notch
  // 5) Otherwise, slew directly to the target


  bool modified = false;

  std::vector<Notch>::const_iterator tar_notch;
  std::vector<Notch>::const_iterator tel_notch;
  double tar_dn_limit = dnLimit(tar_az, tar_notch);
  double tel_dn_limit = dnLimit(tel_az, tel_notch);

#ifdef DEBUG  
  Debug::stream()
    << "In:  " 
    << tar_az/ANGLE_RADPERDEG << ' ' << tar_el/ANGLE_RADPERDEG << ' ' 
    << tel_az/ANGLE_RADPERDEG << ' ' << tel_el/ANGLE_RADPERDEG << ' ' 
    << tel_dn_limit/ANGLE_RADPERDEG << ' ' 
    << tar_dn_limit/ANGLE_RADPERDEG << std::endl;
#endif

  if(tar_el < tar_dn_limit)tar_el = tar_dn_limit, modified=true;
 
  if(tel_el < tel_dn_limit)
    {
#ifdef DEBUG  
      Debug::stream() << 'A' << std::endl;
#endif
      if((!m_slewup)||(fabs(m_slewup_az-tel_az)>1.0*ANGLE_RADPERDEG))
	m_slewup = true, m_slewup_az = tel_az;
      tar_az = m_slewup_az;
      if(tar_el < (tel_dn_limit+m_el_hysteresis))
	tar_el = tel_dn_limit+m_el_hysteresis;
      modified = true;
    }
  else if((tel_el < m_dn_lim)&&(tel_notch != m_notches.end())
	  &&((tar_az<tel_notch->cc_limit)||(tar_az>tel_notch->cw_limit)))
    {
#ifdef DEBUG  
      Debug::stream() << 'B' << std::endl;
#endif
      if((!m_slewup)||(fabs(m_slewup_az-tel_az)>1.0*ANGLE_RADPERDEG))
	m_slewup = true, m_slewup_az = tel_az;
      tar_az = m_slewup_az;
      if(tar_el < (m_dn_lim+m_el_hysteresis))
	tar_el = m_dn_lim+m_el_hysteresis;
      modified = true;
    }
  else if((tar_el < m_dn_lim+m_el_hysteresis)
	  &&(tar_notch != m_notches.end())
	  &&((tel_az<tar_notch->cc_limit)||(tel_az>tar_notch->cw_limit)))
    {
#ifdef DEBUG  
      Debug::stream() << 'C' << std::endl;
#endif
      m_slewup = false;
      tar_el = tel_dn_limit+m_el_hysteresis;
      modified = true;      
    }
  else
    {
#ifdef DEBUG  
      Debug::stream() << 'D' << std::endl;
#endif
      m_slewup = false;      
      modified = false;
    }

#ifdef DEBUG  
  Debug::stream() << "Out: " 
		  << tar_az/ANGLE_RADPERDEG << ' ' 
		  << tar_el/ANGLE_RADPERDEG << ' '
		  << m_slewup << ' ' << m_slewup_az << std::endl;
#endif

  return modified;
}

// ==========================================================================
// LimitsList
// ==========================================================================

LimitsList::~LimitsList()
{
  for(iterator ilim=begin(); ilim!=end(); ilim++)
    delete *ilim;
}

bool LimitsList::
isDriveAngleInsideLimits(double az_driveangle, double el_driveangle,
			 bool& az_lim_cc, bool& az_lim_cw, 
			 bool& el_lim_dn, bool& el_lim_up)
{
  for(iterator ilim=begin(); ilim!=end(); ilim++)
    {
      if((*ilim)->isDriveAngleInsideLimits(az_driveangle, el_driveangle,
					   az_lim_cc, az_lim_cw, 
					   el_lim_dn, el_lim_up) == false)
	return false;
    }
  return true;
}

bool LimitsList::
modifyTargetDriveAngle(double tel_az, double tel_el,
		       double& tar_az, double& tar_el,
		       double& az_speed, double& el_speed)
{
  for(iterator ilim=begin(); ilim!=end(); ilim++)
    {
      if((*ilim)->modifyTargetDriveAngle(tel_az, tel_el, tar_az, tar_el, 
					 az_speed, el_speed) == true)
	return true;
    }
  return false;
}

//-*-mode:c++; mode:font-lock;-*-

/**
 * \file TelescopeMotionLimits.h
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

#ifndef VTRACKING_TELESCOPEMOTIONLIMITS_H
#define VTRACKING_TELESCOPEMOTIONLIMITS_H

#include<list>
#include<vector>

#include<Angle.h>

namespace VTracking 
{  

  // ==========================================================================
  // TelescopeMotionLimits
  // ==========================================================================

  class TelescopeMotionLimits
  {
  public:
    TelescopeMotionLimits(double max_az_speed, double max_el_speed):
      m_max_az_speed(max_az_speed), m_max_el_speed(max_el_speed)
    { /* nothing to see here */ }

    virtual ~TelescopeMotionLimits();

    virtual bool 
    isDriveAngleInsideLimits(double az_driveangle, double el_driveangle,
			     bool& az_lim_cc, bool& az_lim_cw, 
			     bool& el_lim_dn, bool& el_lim_up) = 0;
    
    virtual bool 
    modifyTargetDriveAngle(double tel_az, double tel_el,
			   double& tar_az, double& tar_el,
			   double& az_speed, double& el_speed) = 0;
    
    bool 
    isDriveAngleInsideLimits(double az_driveangle, double el_driveangle)
    {
      bool az_lim_neg;
      bool az_lim_pos;
      bool el_lim_neg;
      bool el_lim_pos;
      return isDriveAngleInsideLimits(az_driveangle, el_driveangle,
				      az_lim_neg, az_lim_pos,
				      el_lim_neg, el_lim_pos);
    }

    double maxAzSpeedDPS() const { return m_max_az_speed; }
    double maxElSpeedDPS() const { return m_max_el_speed; }

  private:
    double                    m_max_az_speed;
    double                    m_max_el_speed;
  };

  // ==========================================================================
  // PrimaryLimits
  // ==========================================================================

  class PrimaryLimits: public TelescopeMotionLimits
  {
  public:
    PrimaryLimits(double cw_limit = 270.0,  double cc_limit = -270.0,
		  double dn_limit = -0.5,   double up_limit = 90.5,
		  double max_az_speed = 1.0,  double max_el_speed = 1.0)
      : TelescopeMotionLimits(max_az_speed, max_el_speed),
	m_up_lim(up_limit * ANGLE_RADPERDEG),
	m_dn_lim(dn_limit * ANGLE_RADPERDEG),
	m_cw_lim(cw_limit * ANGLE_RADPERDEG),
	m_cc_lim(cc_limit * ANGLE_RADPERDEG),
	m_slewup(false), m_slewup_az()
    { /* nothing to see here */ }

    virtual ~PrimaryLimits();
    
    virtual bool 
    isDriveAngleInsideLimits(double az_driveangle, double el_driveangle,
			     bool& az_lim_cc, bool& az_lim_cw, 
			     bool& el_lim_dn, bool& el_lim_up);

    virtual bool 
    modifyTargetDriveAngle(double tel_az, double tel_el,
			   double& tar_az, double& tar_el,
			   double& az_speed, double& el_speed);

    double getLimAzCWDeg() const { return m_cw_lim / ANGLE_RADPERDEG; }
    double getLimAzCCDeg() const { return m_cc_lim / ANGLE_RADPERDEG; }
    double getLimElUPDeg() const { return m_up_lim / ANGLE_RADPERDEG; }
    double getLimElDNDeg() const { return m_dn_lim / ANGLE_RADPERDEG; }

  private:
    double                    m_up_lim;
    double                    m_dn_lim;
    double                    m_cw_lim;
    double                    m_cc_lim;

    bool                      m_slewup;
    double                    m_slewup_az;
  };

  // ==========================================================================
  // KeyLimits
  // ==========================================================================

  // Phase me out!

  class KeyLimits: public TelescopeMotionLimits
  {
  public:
    KeyLimits(double max_az_speed = 0.3,  double max_el_speed = 0.3,
	      double key_cw_limit = 2.0,  double key_cc_limit = -2.0,
	      double key_dn_limit = -0.5,
	      double out_dn_limit = 15,   double out_up_limit = 90,
	      double out_cw_limit = 270,  double out_cc_limit = -270)
      : TelescopeMotionLimits(max_az_speed, max_el_speed),
	m_key_az_cw(key_cw_limit * ANGLE_RADPERDEG),
	m_key_az_cc(key_cc_limit * ANGLE_RADPERDEG),
	m_key_el_dn(key_dn_limit * ANGLE_RADPERDEG),
	m_lim_el_up(out_up_limit * ANGLE_RADPERDEG),
	m_lim_el_dn(out_dn_limit * ANGLE_RADPERDEG),
	m_lim_az_cw(out_cw_limit * ANGLE_RADPERDEG),
	m_lim_az_cc(out_cc_limit * ANGLE_RADPERDEG) 
    { /* nothing to see here */ }

    virtual ~KeyLimits();
    
    virtual bool 
    isDriveAngleInsideLimits(double az_driveangle, double el_driveangle,
			     bool& az_lim_cc, bool& az_lim_cw, 
			     bool& el_lim_dn, bool& el_lim_up);

    virtual bool 
    modifyTargetDriveAngle(double tel_az, double tel_el,
			   double& tar_az, double& tar_el,
			   double& az_speed, double& el_speed);

    double getKeyAzCWDeg() const { return m_key_az_cw / ANGLE_RADPERDEG; }
    double getKeyAzCCDeg() const { return m_key_az_cc / ANGLE_RADPERDEG; }
    double getKeyElDNDeg() const { return m_key_el_dn / ANGLE_RADPERDEG; }
    double getLimAzCWDeg() const { return m_lim_az_cw / ANGLE_RADPERDEG; }
    double getLimAzCCDeg() const { return m_lim_az_cc / ANGLE_RADPERDEG; }
    double getLimElUPDeg() const { return m_lim_el_up / ANGLE_RADPERDEG; }
    double getLimElDNDeg() const { return m_lim_el_dn / ANGLE_RADPERDEG; }

  private:
    double                    m_key_az_cw;
    double                    m_key_az_cc;
    double                    m_key_el_dn;
    
    double                    m_lim_el_up;
    double                    m_lim_el_dn;
    double                    m_lim_az_cw;
    double                    m_lim_az_cc;
  };

  // ==========================================================================
  // RestrictedLowElevationMotion
  // ==========================================================================

  class RestrictedLowElevationMotion: public TelescopeMotionLimits
  {
  public:
    RestrictedLowElevationMotion(double low_el = 15, 
				 double max_az_slew = 0.1,
				 double max_az_speed = 1.0,
				 double max_el_speed = 1.0)
      : TelescopeMotionLimits(max_az_speed, max_el_speed),
	m_low_el(low_el * ANGLE_RADPERDEG), 
	m_max_az_slew(max_az_slew  * ANGLE_RADPERDEG),
	m_slewup(false), m_slewup_az(),
	m_el_hysteresis(0.3 * ANGLE_RADPERDEG)
    { /* nothing to see here */ }

    virtual ~RestrictedLowElevationMotion();

    virtual bool 
    isDriveAngleInsideLimits(double az_driveangle, double el_driveangle,
			     bool& az_lim_cc, bool& az_lim_cw, 
			     bool& el_lim_dn, bool& el_lim_up);

    virtual bool 
    modifyTargetDriveAngle(double tel_az, double tel_el,
			   double& tar_az, double& tar_el,
			   double& az_speed, double& el_speed);

    double getLowElDeg() const { return m_low_el / ANGLE_RADPERDEG; }
    double getMazAzSlewDeg() const { return m_max_az_slew / ANGLE_RADPERDEG; }
    double getElHysteresis() const { return m_el_hysteresis / ANGLE_RADPERDEG;}

  private:
    double                    m_low_el;
    double                    m_max_az_slew;
    bool                      m_slewup;
    double                    m_slewup_az;
    double                    m_el_hysteresis;
  };

  // ==========================================================================
  // NotchedInclusionimits
  // ==========================================================================

  class NotchedInclusionLimits: public TelescopeMotionLimits
  {
  public:
    struct Notch
    {
      Notch(): cc_limit(), cw_limit(), dn_limit() { /* nothing to see here */ }
      Notch(double cc, double cw, double dn):
	cc_limit(cc), cw_limit(cw), dn_limit(dn) { /* nothing to see here */ }
      double cc_limit;
      double cw_limit;
      double dn_limit;
    };

    NotchedInclusionLimits(const std::vector<Notch>& notches,
			   double dn_limit,
			   double max_az_speed = 1.0,
			   double max_el_speed = 1.0)
      : TelescopeMotionLimits(max_az_speed, max_el_speed),
	m_notches(), m_dn_lim(dn_limit * ANGLE_RADPERDEG),
	m_el_hysteresis(0.3 * ANGLE_RADPERDEG),
	m_slewup(false), m_slewup_az()
    { 
      for(std::vector<Notch>::const_iterator inotch = notches.begin();
	  inotch!=notches.end(); inotch++)
	{
	  Notch notch;
	  notch.cc_limit = inotch->cc_limit * ANGLE_RADPERDEG;
	  notch.cw_limit = inotch->cw_limit * ANGLE_RADPERDEG;
	  notch.dn_limit = inotch->dn_limit * ANGLE_RADPERDEG;
	  m_notches.push_back(notch);
	}
    }

    virtual ~NotchedInclusionLimits();

    virtual bool 
    isDriveAngleInsideLimits(double az_driveangle, double el_driveangle,
			     bool& az_lim_cc, bool& az_lim_cw, 
			     bool& el_lim_dn, bool& el_lim_up);

    virtual bool 
    modifyTargetDriveAngle(double tel_az, double tel_el,
			   double& tar_az, double& tar_el,
			   double& az_speed, double& el_speed);

    double getLimElDNDeg() const { return m_dn_lim / ANGLE_RADPERDEG; }
    std::vector<Notch> getNotches() const
    {
      std::vector<Notch> notches;
      for(std::vector<Notch>::const_iterator inotch = m_notches.begin();
	  inotch!=m_notches.end(); inotch++)
	{
	  Notch notch;
	  notch.cc_limit = inotch->cc_limit / ANGLE_RADPERDEG;
	  notch.cw_limit = inotch->cw_limit / ANGLE_RADPERDEG;
	  notch.dn_limit = inotch->dn_limit / ANGLE_RADPERDEG;
	  notches.push_back(notch);
	}
      return notches;
    }
    double getElHysteresis() const { return m_el_hysteresis / ANGLE_RADPERDEG;}
    
  private:
    double dnLimit(double az, 
		   std::vector<Notch>::const_iterator& inotch) const
    {
      double dn_limit = m_dn_lim;
      for(inotch = m_notches.begin(); inotch!=m_notches.end(); inotch++)
	if((az>=inotch->cc_limit)&&(az<=inotch->cw_limit))
	  {
	    dn_limit = inotch->dn_limit;
	    return dn_limit;
	  }
      return dn_limit;
    }

    double dnLimit(double az) const
    {
      std::vector<Notch>::const_iterator inotch;
      return dnLimit(az,inotch);
    }

    std::vector<Notch>        m_notches;
    double                    m_dn_lim;
    double                    m_el_hysteresis;
    bool                      m_slewup;
    double                    m_slewup_az;
  };

  // ==========================================================================
  // LimitsList
  // ==========================================================================

  class LimitsList: public TelescopeMotionLimits
  {
  public:
    typedef std::list<TelescopeMotionLimits*>::const_iterator iterator;

    LimitsList(double max_az_speed = 1.0, double max_el_speed = 1.0)
      : TelescopeMotionLimits(max_az_speed, max_el_speed), m_limits() { }
    virtual ~LimitsList();

    virtual bool 
    isDriveAngleInsideLimits(double az_driveangle, double el_driveangle,
			     bool& az_lim_cc, bool& az_lim_cw, 
			     bool& el_lim_dn, bool& el_lim_up);
    
    virtual bool 
    modifyTargetDriveAngle(double tel_az, double tel_el,
			   double& tar_az, double& tar_el,
			   double& az_speed, double& el_speed);

    void pushBack(TelescopeMotionLimits* l) { m_limits.push_back(l); }
    void pushFront(TelescopeMotionLimits* l) { m_limits.push_front(l); }

    const std::list<TelescopeMotionLimits*>& limits() const 
    { return m_limits; }
    iterator begin() const { return m_limits.begin(); }
    iterator end() const { return m_limits.end(); }
  private:
    std::list<TelescopeMotionLimits*> m_limits;
  };

} // namespace VTracking

#endif // VTRACKING_TELESCOPEMOTIONLIMITS_H

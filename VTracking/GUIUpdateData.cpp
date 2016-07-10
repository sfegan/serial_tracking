//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUIUpdateData.h
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
 * $Date: 2008/01/31 21:24:13 $
 * $Revision: 2.5 $
 * $Tag$
 *
 **/

#include <sstream>
#include <iomanip>

#include <time.h>

#include "GUIUpdateData.h"

using namespace SEphem;
using namespace VTracking;
using namespace VMessaging;

#define MAX_AZ_SPEED 0.3
#define MAX_EL_SPEED 0.3

void GUIUpdateData::fillData(TelescopeController* controller,
			     const SphericalCoords& earth_position)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  static const double mjd30min = Angle::toRot(Angle::frHrs(0.5));
  static const double sid30min = 1.00273790935*Angle::frHrs(0.5);

  last_state              = tse.state;

  // TIMES
  controller->getTimes(tv,mjd,timeangle,lmst);
  strftime(date_string,sizeof(date_string),
	   "%Y-%m-%d",gmtime(&tv.tv_sec));

  mjd30                   = mjd+mjd30min;
  lmst30                  = lmst+sid30min;

  // DATA WHICH ONLY CHANGES WITH UPDATED POSITION
  if(controller->hasBeenUpdated(last_update_mjd))
    {
      TelescopeController::TrackingState last_state = tse.state;
      double last_cmd_az_driveangle_deg = tse.cmd_az_driveangle_deg;
      double last_cmd_el_driveangle_deg = tse.cmd_el_driveangle_deg;

      tse                 = controller->tse();

      if(last_state != TelescopeController::TS_COM_FAILURE)
	{
	  // LAST state was not COM_FAILURE
	  last_cmd_az_driveangle = last_cmd_az_driveangle_deg;
	  last_cmd_el_driveangle = last_cmd_el_driveangle_deg;
	}
      else if(tse.state != TelescopeController::TS_COM_FAILURE)
	{
	  last_cmd_az_driveangle = tse.cmd_az_driveangle_deg;
	  last_cmd_el_driveangle = tse.cmd_el_driveangle_deg;
	}
      else
	{
	  last_cmd_az_driveangle = 0;
	  last_cmd_el_driveangle = 0;
	}

      tcp                 = controller->getCorrections();

      last_update_mjd     = tse.mjd;

      tel_azel            = tse.tel_azel;
      tel_position        = AzElObject(tel_azel);
      tel_radec           = tel_position.getRaDec(mjd,lmst,
						  earth_position);
      tel_gal             = tel_position.getGalCoord(mjd,lmst,
						     earth_position);

      if(tse.state == TelescopeController::TS_COM_FAILURE)
	{
	  az_smooth_speed = 0;
	  el_smooth_speed = 0;
	}
      else if(last_state == TelescopeController::TS_COM_FAILURE)
	{
	  az_smooth_speed = tse.az_driveangle_estimated_speed_dps;
	  el_smooth_speed = tse.el_driveangle_estimated_speed_dps;
	}
      else
	{
	  az_smooth_speed = tse.az_driveangle_estimated_speed_dps + 
	    0.8*(az_smooth_speed-tse.az_driveangle_estimated_speed_dps);
	  el_smooth_speed = tse.el_driveangle_estimated_speed_dps + 
	    0.8*(el_smooth_speed-tse.el_driveangle_estimated_speed_dps);
	}

      full_update         = true;
    }
  else
    {
      full_update         = false;
    }

  latest_req              = controller->request();

  delete tar_object;
  tar_object              = controller->getTargetObject(tar_dir_pref);

  if(tar_object)
    {
      bool use_corrections = tar_object->useCorrections();

      tar_radec           = tar_object->getRaDec(mjd,lmst,
						 earth_position);
      tar_gal             = tar_object->getGalCoord(mjd,lmst,
						    earth_position);
      tar_azel            = tar_object->getAzEl(mjd,lmst,
						earth_position);
      tcp                 = controller->getCorrections();
      
      tar_az_driveangle   = tar_azel.longitudeRad();
      tar_el_driveangle   = tar_azel.latitudeRad();

      tcp.doAzElCorrections(tar_az_driveangle,
			    tar_el_driveangle,
			    Angle::frDeg(tse.status.az.driveangle_deg),
			    use_corrections, tar_dir_pref);
      
      targetSpeed(tar_azel,earth_position,
		  tar_az_speed,tar_el_speed);
      

      in_limits           = 
	controller->isInsideLimits(tar_azel, 
				   Angle::frDeg(tse.status.az.driveangle_deg),
				   use_corrections);

      in_limits30         = in_limits;
      for(unsigned imin=1;imin<30;imin++)
	{
	  double x = double(imin)/30.0;
	  mjd30           = mjd + mjd30min*x;
	  lmst30          = lmst + sid30min*x;
	  tar_azel30      = tar_object->getAzEl(mjd30,lmst30,earth_position);
	  
	  tar_az_driveangle30 = tar_azel30.longitudeRad();
	  tar_el_driveangle30 = tar_azel30.latitudeRad();

	  tcp.doAzElCorrections(tar_az_driveangle30, tar_el_driveangle30,
				tar_az_driveangle, use_corrections,
				tar_dir_pref);

	  if(!controller->
	     isInsideLimits(tar_azel30,
			    Angle::frDeg(tse.status.az.driveangle_deg),
			    use_corrections))in_limits30 = false;
	}
      
      hamin = Angle::makeRad(tar_radec.phi()-lmst).hrsPM()*60;
      if(hamin<0)
	{
	  tar_max_az_speed = tar_az_speed;
	  tar_max_el_speed = tar_el_speed;
	}
      else if(hamin>30)
	{
	  targetSpeed(tar_azel30,earth_position,
		      tar_max_az_speed,tar_max_el_speed);
	}
      else
	{
	  SphericalCoords target_azelTRANS = 
	    SphericalCoords::makeLatLong(tar_radec.latitude(),0);
	  target_azelTRANS.rotate(Angle::sc_Pi,-earth_position.theta(),0);
	  targetSpeed(target_azelTRANS,earth_position,
		      tar_max_az_speed,tar_max_el_speed);
	}
      
      target_moves = tar_object->objectMovesInAzEl();
  
      if(tse.state != TelescopeController::TS_COM_FAILURE)
	{
	  az_sep          = 
	    Angle::toDeg(tar_az_driveangle) - tse.status.az.driveangle_deg;

	  el_sep          = 
	    Angle::toDeg(tar_el_driveangle) - tse.status.el.driveangle_deg;

	  here            = 
	    SphericalCoords::makeLatLongDeg(tse.status.el.driveangle_deg,
					    tse.status.az.driveangle_deg);

	  there           = 
	    SphericalCoords::makeLatLongRad(tar_el_driveangle,
					    tar_az_driveangle);

	  sep=here.separation(there);
	  
	  az_speed_limit  = tse.az_slew_speed_dps;
	  el_speed_limit  = tse.el_slew_speed_dps;

	  el_eta          = fabs(el_sep)/tse.el_slew_speed_dps;
	  az_eta          = fabs(az_sep)/tse.az_slew_speed_dps;
	  eta             = int(round((el_eta>az_eta)?el_eta:az_eta));

	  std::ostringstream eta_stream;
	  eta_stream << eta/60 << ':' 
		     << std::setfill('0') << std::setw(2) << eta%60;
	  eta_string      = eta_stream.str();
	}
      else
	{
	  az_sep          = 0;
	  el_sep          = 0;
	  here            = SphericalCoords::makeLatLongDeg(0,0);
	  there           = SphericalCoords::makeLatLongRad(0,0);

	  sep             = 0;
	  
	  el_eta          = 0;
	  az_eta          = 0;
	  eta             = 0;

	  eta_string      = std::string("??:??");
	}

      if(!use_corrections)
	{
	  double tar_sky_az = tar_az_driveangle;
	  double tar_sky_el = tar_el_driveangle;
	  tcp.undoAzElCorrections(tar_sky_az, tar_sky_el, true);
	  tar_azel.setLatLongRad(tar_sky_el,tar_sky_az);

	}
    }
  else
    {
      tar_radec           = SphericalCoords::makeLatLongDeg(0,0);
      tar_gal             = SphericalCoords::makeLatLongDeg(0,0);
      tar_azel            = SphericalCoords::makeLatLongDeg(0,0);

      tar_az_driveangle   = 0;
      tar_el_driveangle   = 0;

      tar_az_speed        = 0;
      tar_el_speed        = 0;

      tar_azel30          = SphericalCoords::makeLatLongDeg(0,0);

      tar_az_driveangle30 = 0;
      tar_el_driveangle30 = 0;

      hamin               = 0;
      tar_max_az_speed    = tar_az_speed;
      tar_max_el_speed    = tar_el_speed;

      in_limits           = false;
      in_limits30         = false;

      target_moves        = false;
      
      az_sep              = 0;
      el_sep              = 0;
      here                = SphericalCoords::makeLatLongDeg(0,0);
      there               = SphericalCoords::makeLatLongRad(0,0);
      
      sep                 = 0;

      az_speed_limit      = MAX_AZ_SPEED;
      el_speed_limit      = MAX_EL_SPEED;
      
      el_eta              = 0;
      az_eta              = 0;
      eta                 = 0;
      
      eta_string          = std::string("??:??");
    }

  replay                  = false;

#if 0
  SunObject sun;
  sun_azel                = sun.getAzEl(mjd, lmst, earth_position);
  MoonObject moon;
  moon_azel               = moon.getAzEl(mjd, lmst, earth_position);
#endif

  update_number++;
}

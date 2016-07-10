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
 * $Author: aune $
 * $Date: 2010/09/09 18:56:11 $
 * $Revision: 2.12 $
 * $Tag$
 *
 **/

#ifndef GUI_UPDATEDATA_H
#define GUI_UPDATEDATA_H

#include<time.h>
#include<sys/time.h>
#include<cstring>

#include<Angle.h>
#include<SphericalCoords.h>
#include<Exception.h>
#include<CorrectionParameters.h>

#include"TargetObject.h"
#include"TelescopeController.h"

class GUIUpdateData
{
public:
  GUIUpdateData(): 
    full_update(true), update_number(0), tse(), 
    last_state(VTracking::TelescopeController::TS_COM_FAILURE),
    latest_req(VTracking::TelescopeController::REQ_STOP),
    az_smooth_speed(), el_smooth_speed(), tv(),
    mjd(), timeangle(), lmst(), date_string(), last_update_mjd(), 
    tel_azel(), tel_radec(), tel_gal(), tel_position(), 
    tar_object(), tar_dir_pref(SEphem::CorrectionParameters::DP_NONE),
    tar_azel(), tar_radec(), tar_gal(), tcp(), 
    tar_az_driveangle(), tar_el_driveangle(),
    last_cmd_az_driveangle(), last_cmd_el_driveangle(),
    mjd30(), lmst30(), tar_az_speed(), tar_el_speed(), 
    tar_azel30(), tar_az_driveangle30(), tar_el_driveangle30(),
    tar_max_az_speed(), tar_max_el_speed(), hamin(), 
    in_limits(), in_limits30(),
    target_moves(), az_sep(), el_sep(), here(), there(), sep(),
    el_eta(), az_eta(), eta(), eta_string(), 
    az_speed_limit(), el_speed_limit(), replay() /*, sun_azel(), moon_azel()*/
  { 
    tse.state = VTracking::TelescopeController::TS_COM_FAILURE;
    tse.req = VTracking::TelescopeController::REQ_STOP;
    tse.cf = VTracking::TelescopeController::CF_SERVER;
  }

  ~GUIUpdateData() { delete tar_object; }

  bool                                  full_update;
  unsigned                              update_number;

  VTracking::TelescopeController::StateElements    tse;
  VTracking::TelescopeController::TrackingState    last_state;  
  VTracking::TelescopeController::TrackingRequest  latest_req;

  double                                az_smooth_speed;
  double                                el_smooth_speed;
  
  struct timeval                        tv;
  double                                mjd;
  SEphem::Angle                         timeangle;
  SEphem::Angle                         lmst;
  char                                  date_string[11];

  double                                last_update_mjd;

  SEphem::SphericalCoords               tel_azel;
  SEphem::SphericalCoords               tel_radec;
  SEphem::SphericalCoords               tel_gal;
  VTracking::AzElObject                 tel_position;

  VTracking::TargetObject*              tar_object;
  SEphem::CorrectionParameters::DirectionPreference tar_dir_pref;
  SEphem::SphericalCoords               tar_azel;
  SEphem::SphericalCoords               tar_radec;
  SEphem::SphericalCoords               tar_gal;

  SEphem::CorrectionParameters          tcp;

  double                                tar_az_driveangle;
  double                                tar_el_driveangle;

  double                                last_cmd_az_driveangle;
  double                                last_cmd_el_driveangle;

  double                                mjd30;
  SEphem::Angle                         lmst30;

  double                                tar_az_speed;
  double                                tar_el_speed;

  SEphem::SphericalCoords               tar_azel30;

  double                                tar_az_driveangle30;
  double                                tar_el_driveangle30;

  double                                tar_max_az_speed;
  double                                tar_max_el_speed;

  double                                hamin;

  bool                                  in_limits;
  bool                                  in_limits30;

  bool                                  target_moves;
  
  double                                az_sep; 
  double                                el_sep; 
  SEphem::SphericalCoords               here;
  SEphem::SphericalCoords               there;
  SEphem::Angle                         sep;

  double                                el_eta;
  double                                az_eta;
  int                                   eta;
  std::string                           eta_string;

  double                                az_speed_limit;
  double                                el_speed_limit;

  bool                                  replay;

#if 0
  SEphem::SphericalCoords               sun_azel;
  SEphem::SphericalCoords               moon_azel;
#endif

  GUIUpdateData(const GUIUpdateData& o):
    full_update(true), update_number(0), tse(), 
    last_state(VTracking::TelescopeController::TS_COM_FAILURE),
    latest_req(VTracking::TelescopeController::REQ_STOP),
    az_smooth_speed(), el_smooth_speed(), tv(),
    mjd(), timeangle(), lmst(), date_string(), last_update_mjd(), 
    tel_azel(), tel_radec(), tel_gal(), tel_position(), tar_object(),
    tar_dir_pref(),
    tar_azel(), tar_radec(), tar_gal(), tcp(), 
    tar_az_driveangle(), tar_el_driveangle(),
    last_cmd_az_driveangle(), last_cmd_el_driveangle(),
    mjd30(), lmst30(), tar_az_speed(), tar_el_speed(), 
    tar_azel30(), tar_az_driveangle30(), tar_el_driveangle30(),
    tar_max_az_speed(), tar_max_el_speed(), hamin(), 
    in_limits(), in_limits30(),
    target_moves(), az_sep(), el_sep(), here(), there(), sep(),
    el_eta(), az_eta(), eta(), eta_string(), 
    az_speed_limit(), el_speed_limit(), replay()
    /*, sun_azel(), moon_azel() */ { *this = o; }
  inline GUIUpdateData& operator= (const GUIUpdateData& o);

  void fillData(VTracking::TelescopeController* controller,
		const SEphem::SphericalCoords& earth_position);
};

inline GUIUpdateData& GUIUpdateData::operator=(const GUIUpdateData& o)
{
  VMessaging::RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(tar_object)delete tar_object;
  full_update              = o.full_update;
  update_number            = o.update_number;
  tse                      = o.tse;
  last_state               = o.last_state;  
  latest_req               = o.latest_req;
  az_smooth_speed          = o.az_smooth_speed;
  el_smooth_speed          = o.el_smooth_speed;
  tv                       = o.tv;
  mjd                      = o.mjd;
  timeangle                = o.timeangle;
  lmst                     = o.lmst;
  memcpy(date_string, o.date_string, sizeof(date_string));
  last_update_mjd          = o.last_update_mjd;
  tel_azel                 = o.tel_azel;
  tel_radec                = o.tel_radec;
  tel_gal                  = o.tel_gal;
  tel_position             = o.tel_position;
  tar_object               = o.tar_object?o.tar_object->copy():0;
  tar_dir_pref             = o.tar_dir_pref;
  tar_azel                 = o.tar_azel;
  tar_radec                = o.tar_radec;
  tar_gal                  = o.tar_gal;
  tcp                      = o.tcp;
  tar_az_driveangle        = o.tar_az_driveangle;
  tar_el_driveangle        = o.tar_el_driveangle;
  last_cmd_az_driveangle   = o.last_cmd_az_driveangle;
  last_cmd_el_driveangle   = o.last_cmd_el_driveangle;
  mjd30                    = o.mjd30;
  lmst30                   = o.lmst30;
  tar_az_speed             = o.tar_az_speed;
  tar_el_speed             = o.tar_el_speed;
  tar_azel30               = o.tar_azel30;
  tar_az_driveangle30      = o.tar_az_driveangle30;
  tar_el_driveangle30      = o.tar_el_driveangle30;
  tar_max_az_speed         = o.tar_max_az_speed;
  tar_max_el_speed         = o.tar_max_el_speed;
  hamin                    = o.hamin;
  in_limits                = o.in_limits;
  in_limits30              = o.in_limits30;
  target_moves             = o.target_moves;
  az_sep                   = o.az_sep; 
  el_sep                   = o.el_sep; 
  here                     = o.here;
  there                    = o.there;
  sep                      = o.sep;
  el_eta                   = o.el_eta;
  az_eta                   = o.az_eta;
  eta                      = o.eta;
  eta_string               = o.eta_string;
  az_speed_limit           = o.az_speed_limit;
  el_speed_limit           = o.el_speed_limit;
  replay                   = o.replay;
#if 0
  sun_azel                 = o.sun_azel;
  moon_azel                = o.moon_azel;
#endif
  return *this;
}

#endif // GUI_UPDATEDATA_H

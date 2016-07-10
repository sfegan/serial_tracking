//-*-mode:c++; mode:font-lock;-*-

/**
 * \file NET_TCInterfaceServant.cpp
 * \ingroup VTracking
 * \brief Servant code to loop CORBA commands to TelescopeController
 *
 * This class makes the local TelescoperController interface available
 * over CORBA.
 *
 * Original Author: Stephen Fegan
 * $Author: aune $
 * $Date: 2011/10/06 02:14:28 $
 * $Revision: 2.6 $
 * $Tag$
 *
 **/

#include<cassert>

#include"NET_TCInterfaceServant.h"

using namespace SEphem;
using namespace VTracking;
using namespace VMessaging;

NET_TCInterfaceServant::
NET_TCInterfaceServant(TelescopeController* controller,
		       const StowObjectVector& stow_pos,
		       bool readonly, ZThread::Condition* terminate_notifier)
  : POA_VTracking::NET_TCInterface(), PortableServer::RefCountServantBase(),
    m_controller(controller), m_stow_pos(stow_pos), m_readonly(readonly),
    m_terminate_notifier(terminate_notifier)
{
  // nothing to see here
}

NET_TCInterfaceServant::~NET_TCInterfaceServant()
{
  // nothing to see here
}

NET_TCMiscData* NET_TCInterfaceServant::netGetMiscData()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  const TelescopeMotionLimits* limits(m_controller->getLimits());
  const SEphem::SphericalCoords& earth_pos(m_controller->getEarthPos());
  CorrectionParameters corrections(m_controller->getCorrections());
  NET_TCMiscData* misc_data = new NET_TCMiscData;
  
  misc_data->earth_pos_lon_rad = earth_pos.longitudeRad();
  misc_data->earth_pos_lat_rad = earth_pos.latitudeRad();

  packLimits(misc_data->limits, limits);

  misc_data->corrections.enable_offsets     = corrections.enable_offsets;
  misc_data->corrections.enable_corrections = corrections.enable_corrections;
  misc_data->corrections.enable_vff         = corrections.enable_vff;
  misc_data->corrections.az_ratio           = corrections.az_ratio;
  misc_data->corrections.el_ratio           = corrections.el_ratio;
  misc_data->corrections.az_offset          = corrections.az_offset;
  misc_data->corrections.el_offset          = corrections.el_offset;
  misc_data->corrections.az_ns              = corrections.az_ns;
  misc_data->corrections.az_ew              = corrections.az_ew;
  misc_data->corrections.el_udew            = corrections.el_udew;
  misc_data->corrections.fp_az              = corrections.fp_az;
  misc_data->corrections.flex_el_A          = corrections.flex_el_A;
  misc_data->corrections.flex_el_B          = corrections.flex_el_B;
  misc_data->corrections.el_pos_vff_s       = corrections.el_pos_vff_s;
  misc_data->corrections.el_pos_vff_t       = corrections.el_pos_vff_t;
  misc_data->corrections.el_neg_vff_s       = corrections.el_neg_vff_s;
  misc_data->corrections.el_neg_vff_t       = corrections.el_neg_vff_t;
  misc_data->corrections.az_pos_vff_s       = corrections.az_pos_vff_s;
  misc_data->corrections.az_pos_vff_t       = corrections.az_pos_vff_t;
  misc_data->corrections.az_neg_vff_s       = corrections.az_neg_vff_s;
  misc_data->corrections.az_neg_vff_t       = corrections.az_neg_vff_t;

  return misc_data;
}

// methods corresponding to defined IDL attributes and operations
NET_StateElements* NET_TCInterfaceServant::netGetTelescopeState()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  NET_StateElements* nse = new NET_StateElements;
  TelescopeController::StateElements tse = m_controller->tse();

  CorrectionParameters::DirectionPreference dp;
  TargetObject* object = m_controller->getTargetObject(dp);
  bool found_object = false;

  if(!object)
    {
      nse->target.null(true);
      found_object=true;
    }

  if(!found_object)
    {
      const RaDecObject* radec = dynamic_cast<const RaDecObject*>(object);
      if(radec)
	{

	  NET_TargetObjectRADec radec_details;
	  radec_details.ra_rad = radec->coords().longitudeRad();
	  radec_details.dec_rad = radec->coords().latitudeRad();
	  radec_details.epoch = radec->epoch();
	  NET_CoordinateOffset offset_details;
	  packCoordinateOffset(offset_details, radec->offset());
	  radec_details.offset = offset_details;
	  radec_details.name = CORBA::string_dup(radec->name().c_str());
	  nse->target.radec(radec_details);
	  found_object=true;
	};
    }

  if(!found_object)
    {
      const StowObject* stow = dynamic_cast<const StowObject*>(object);
      if(stow)
	{
	  NET_TargetObjectStow stow_details;
	  stow_details.az_rad = stow->coords().longitudeRad();
	  stow_details.el_rad = stow->coords().latitudeRad();
	  stow_details.use_corrections = stow->useCorrections();
	  stow_details.stop_at_target = !stow->objectMovesInAzEl();
	  stow_details.name = CORBA::string_dup(stow->name().c_str());
	  nse->target.stow(stow_details);
	  found_object=true;
	}
    }

  if(!found_object)
    {
      const AzElObject* azel = dynamic_cast<const AzElObject*>(object);
      if(azel)
	{
	  NET_TargetObjectAzEl azel_details;
	  azel_details.az_rad = azel->coords().longitudeRad();
	  azel_details.el_rad = azel->coords().latitudeRad();
	  azel_details.use_corrections = azel->useCorrections();
	  azel_details.stop_at_target = !azel->objectMovesInAzEl();
	  nse->target.azel(azel_details);
	  nse->target._d(NET_TO_AZEL);
	  found_object=true;
	}
    }

  if(!found_object)
    {
      const CVObject* cv = dynamic_cast<const CVObject*>(object);
      if(cv)
	{
	  NET_TargetObjectCV cv_details;
	  cv_details.az_rad = cv->coords().longitudeRad();
	  cv_details.el_rad = cv->coords().latitudeRad();
	  cv_details.az_speed = cv->azSpeed();
	  cv_details.el_speed = cv->elSpeed();
	  cv_details.mjd_zero = cv->mjdZero();
	  nse->target.cv(cv_details);
	  found_object=true;
	}
    }

  if(!found_object)
    {
      NET_TargetObjectAzEl unknown_details;
      unknown_details.az_rad = tse.obj_azel.longitudeRad();
      unknown_details.el_rad = tse.obj_azel.latitudeRad();
      unknown_details.use_corrections = object->useCorrections();
      unknown_details.stop_at_target = !object->objectMovesInAzEl();
      nse->target.azel(unknown_details);
      nse->target._d(NET_TO_UNKNOWN);
      found_object=true;
    }

  delete object;

  switch(dp)
    {
    case CorrectionParameters::DP_NONE: 
      nse->direction_preference = NET_DP_NONE; break;
    case CorrectionParameters::DP_CW:
      nse->direction_preference = NET_DP_CW; break;
    case CorrectionParameters::DP_CCW:
      nse->direction_preference = NET_DP_CCW; break;
    }

  nse->status.az.driveangle_deg = tse.status.az.driveangle_deg;
  switch(tse.status.az.driveMode)
    {
    case ScopeAPI::DM_STANDBY: 
      nse->status.az.driveMode = NET_DM_STANDBY; break;
    case ScopeAPI::DM_SLEW:
      nse->status.az.driveMode = NET_DM_SLEW; break;
    case ScopeAPI::DM_POINT:
      nse->status.az.driveMode = NET_DM_POINT; break;
    case ScopeAPI::DM_SPIN:
      nse->status.az.driveMode = NET_DM_SPIN; break;
    case ScopeAPI::DM_SECTOR_SCAN:
      nse->status.az.driveMode = NET_DM_SECTOR_SCAN; break;
    case ScopeAPI::DM_RASTER:
      nse->status.az.driveMode = NET_DM_RASTER; break;
    case ScopeAPI::DM_CHANGING:
      nse->status.az.driveMode = NET_DM_CHANGING; break;
    case ScopeAPI::DM_UNKNOWN:
      nse->status.az.driveMode = NET_DM_UNKNOWN; break;
    }
  nse->status.az.servo1Fail = tse.status.az.servo1Fail;
  nse->status.az.servo2Fail = tse.status.az.servo2Fail;
  nse->status.az.servoOn = tse.status.az.servoOn;
  nse->status.az.brakeReleased = tse.status.az.brakeReleased;
  nse->status.az.limitCwUp = tse.status.az.limitCwUp;
  nse->status.az.limitCcwDown = tse.status.az.limitCcwDown;
  nse->status.az.positionFault = tse.status.az.positionFault;
  nse->status.az.positionComplete = tse.status.az.positionComplete;

  nse->status.el.driveangle_deg = tse.status.el.driveangle_deg;
  switch(tse.status.el.driveMode)
    {
    case ScopeAPI::DM_STANDBY: 
      nse->status.el.driveMode = NET_DM_STANDBY; break;
    case ScopeAPI::DM_SLEW:
      nse->status.el.driveMode = NET_DM_SLEW; break;
    case ScopeAPI::DM_POINT:
      nse->status.el.driveMode = NET_DM_POINT; break;
    case ScopeAPI::DM_SPIN:
      nse->status.el.driveMode = NET_DM_SPIN; break;
    case ScopeAPI::DM_SECTOR_SCAN:
      nse->status.el.driveMode = NET_DM_SECTOR_SCAN; break;
    case ScopeAPI::DM_RASTER:
      nse->status.el.driveMode = NET_DM_RASTER; break;
    case ScopeAPI::DM_CHANGING:
      nse->status.el.driveMode = NET_DM_CHANGING; break;
    case ScopeAPI::DM_UNKNOWN:
      nse->status.el.driveMode = NET_DM_UNKNOWN; break;
    }
  nse->status.el.servo1Fail = tse.status.el.servo1Fail;
  nse->status.el.servo2Fail = tse.status.el.servo2Fail;
  nse->status.el.servoOn = tse.status.el.servoOn;
  nse->status.el.brakeReleased = tse.status.el.brakeReleased;
  nse->status.el.limitCwUp = tse.status.el.limitCwUp;
  nse->status.el.limitCcwDown = tse.status.el.limitCcwDown;
  nse->status.el.positionFault = tse.status.el.positionFault;
  nse->status.el.positionComplete = tse.status.el.positionComplete;

  nse->status.azTravelledCCW = tse.status.azTravelledCCW;
  nse->status.azCableWrap = tse.status.azCableWrap;
  nse->status.interlock = tse.status.interlock;
  nse->status.interlockAzPullCord = tse.status.interlockAzPullCord;
  nse->status.interlockAzStowPin = tse.status.interlockAzStowPin;
  nse->status.interlockElStowPin = tse.status.interlockElStowPin;
  nse->status.interlockAzDoorOpen = tse.status.interlockAzDoorOpen;
  nse->status.interlockElDoorOpen = tse.status.interlockElDoorOpen;
  nse->status.interlockSafeSwitch = tse.status.interlockSafeSwitch;
  nse->status.remoteControl = tse.status.remoteControl;
  nse->status.checksumOK = tse.status.checksumOK;
  nse->status.msgBadFrame = tse.status.msgBadFrame;
  nse->status.msgCommandInvalid = tse.status.msgCommandInvalid;
  nse->status.msgInputOverrun = tse.status.msgInputOverrun;
  nse->status.msgOutputOverrun = tse.status.msgOutputOverrun;
  nse->status.relay1 = tse.status.relay1;
  nse->status.relay2 = tse.status.relay2;
  nse->status.Analog1 = tse.status.Analog1;
  nse->status.Analog2 = tse.status.Analog2;

  nse->last_az_driveangle_deg = tse.last_az_driveangle_deg;
  nse->last_el_driveangle_deg = tse.last_el_driveangle_deg;
  nse->az_driveangle_estimated_speed_dps 
    = tse.az_driveangle_estimated_speed_dps;
  nse->el_driveangle_estimated_speed_dps
    = tse.el_driveangle_estimated_speed_dps;
  switch(tse.state) 
    {
    case TelescopeController::TS_STOP: 
      nse->state = NET_TS_STOP; break;
    case TelescopeController::TS_SLEW:
      nse->state = NET_TS_SLEW; break;
    case TelescopeController::TS_TRACK: 
      nse->state = NET_TS_TRACK; break;
    case TelescopeController::TS_RESTRICTED_MOTION: 
      nse->state = NET_TS_RESTRICTED_MOTION; break;
    case TelescopeController::TS_RAMP_DOWN:
      nse->state = NET_TS_RAMP_DOWN; break;
    case TelescopeController::TS_COM_FAILURE:
      nse->state = NET_TS_COM_FAILURE; break;
    }
  switch(tse.req)
    {
    case TelescopeController::REQ_STOP:
      nse->req = NET_REQ_STOP; break;
    case TelescopeController::REQ_SLEW:
      nse->req = NET_REQ_SLEW; break;
    case TelescopeController::REQ_TRACK:
      nse->req = NET_REQ_TRACK; break;
    }
  nse->tv_tv_sec = tse.tv.tv_sec;
  nse->tv_tv_usec = tse.tv.tv_usec;
  nse->mjd = tse.mjd;
  nse->last_mjd = tse.last_mjd;
  nse->timeangle_rad = tse.timeangle.rad();
  nse->lmst_rad = tse.lmst.rad();
  nse->last_has_object = tse.last_has_object;
  nse->has_object = tse.has_object;
  nse->tel_azel_az_rad = tse.tel_azel.longitudeRad();
  nse->tel_azel_el_rad = tse.tel_azel.latitudeRad();
  nse->obj_azel_az_rad = tse.obj_azel.longitudeRad();
  nse->obj_azel_el_rad = tse.obj_azel.latitudeRad();
  nse->anticipation = tse.anticipation;
  nse->last_cmd_az_driveangle_deg = tse.last_cmd_az_driveangle_deg;
  nse->last_cmd_el_driveangle_deg = tse.last_cmd_el_driveangle_deg;
  nse->cmd_az_driveangle_deg = tse.cmd_az_driveangle_deg;
  nse->cmd_el_driveangle_deg = tse.cmd_el_driveangle_deg;
  nse->az_slew_speed_dps = tse.az_slew_speed_dps;
  nse->el_slew_speed_dps = tse.el_slew_speed_dps;

  return nse;
}

void NET_TCInterfaceServant::netSetTargetObjectNull()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  if(m_readonly)throw TCIReadonly();
  m_controller->setTargetObject(0);
}

void NET_TCInterfaceServant::netSetTargetObjectStow(const char* name)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  if(m_readonly)throw TCIReadonly();
  TargetObject* obj(0);
  for(StowObjectVector::const_iterator ipos = m_stow_pos.begin();
      ipos != m_stow_pos.end(); ipos++)
    if(ipos->name() == name)obj = ipos->copy();
  m_controller->setTargetObject(obj);
}

void NET_TCInterfaceServant::netSetTargetObjectZenith()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  if(m_readonly)throw TCIReadonly();
  TargetObject* obj(0);
  double az_deg=m_controller->tse().status.az.driveangle_deg;
  obj = new AzElObject(SphericalCoords::makeLatLongDeg(90,az_deg),
		       false,true);
  m_controller->setTargetObject(obj);
}

void NET_TCInterfaceServant::
netSetTargetObjectRaDec(CORBA::Double ra_rad, CORBA::Double dec_rad,
			CORBA::Double epoch, 
			const NET_CoordinateOffset& offset,
			const char* name,
			NET_DirectionPreference dir_pref)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  if(m_readonly)throw TCIReadonly();
  TargetObject* obj = 0;
  SphericalCoords pos = SphericalCoords::makeLatLongRad(dec_rad,ra_rad);
  CoordinateOffset* off = unpackCoordinateOffset(offset);
  obj = new RaDecObject(pos,epoch,name,off);
  CorrectionParameters::DirectionPreference dp;
  switch(dir_pref)
    {
    case NET_DP_NONE: dp = CorrectionParameters::DP_NONE; break;
    case NET_DP_CW:   dp = CorrectionParameters::DP_CW; break;
    case NET_DP_CCW:  dp = CorrectionParameters::DP_CCW; break;
    }
  m_controller->setTargetObject(obj, dp);
}

void NET_TCInterfaceServant::
netSetTargetObjectAzEl(CORBA::Double az_rad, CORBA::Double el_rad,
		       bool use_corrections, bool stop_at_target,
		       NET_DirectionPreference dir_pref)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  if(m_readonly)throw TCIReadonly();
  SphericalCoords pos = SphericalCoords::makeLatLongRad(el_rad,az_rad);
  TargetObject* obj =  new AzElObject(pos,use_corrections,stop_at_target);
  CorrectionParameters::DirectionPreference dp;
  switch(dir_pref)
    {
    case NET_DP_NONE: dp = CorrectionParameters::DP_NONE; break;
    case NET_DP_CW:   dp = CorrectionParameters::DP_CW; break;
    case NET_DP_CCW:  dp = CorrectionParameters::DP_CCW; break;
    }
  m_controller->setTargetObject(obj, dp);
}

void NET_TCInterfaceServant::
netSetTargetObjectCV(CORBA::Double az_rad, CORBA::Double el_rad,
		     CORBA::Double az_speed, CORBA::Double el_speed,
		     CORBA::Double mjd_zero)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  if(m_readonly)throw TCIReadonly();
  SphericalCoords pos = SphericalCoords::makeLatLongRad(el_rad,az_rad);
  TargetObject* obj =  new CVObject(pos,az_speed,el_speed,mjd_zero);
  m_controller->setTargetObject(obj);
}

void NET_TCInterfaceServant::
netSetCorrectionParameters(const VTracking::NET_CorrectionParameters& cp)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  if(m_readonly)throw TCIReadonly();
  CorrectionParameters corrections;
  corrections.enable_offsets     = cp.enable_offsets;
  corrections.enable_corrections = cp.enable_corrections;
  corrections.enable_vff         = cp.enable_vff;
  corrections.az_ratio           = cp.az_ratio;
  corrections.el_ratio           = cp.el_ratio;
  corrections.az_offset          = cp.az_offset;
  corrections.el_offset          = cp.el_offset;
  corrections.az_ns              = cp.az_ns;
  corrections.az_ew              = cp.az_ew;
  corrections.el_udew            = cp.el_udew;
  corrections.fp_az              = cp.fp_az;
  corrections.flex_el_A          = cp.flex_el_A;
  corrections.flex_el_B          = cp.flex_el_B;
  corrections.el_pos_vff_s       = cp.el_pos_vff_s;
  corrections.el_pos_vff_t       = cp.el_pos_vff_t;
  corrections.el_neg_vff_s       = cp.el_neg_vff_s;
  corrections.el_neg_vff_t       = cp.el_neg_vff_t;
  corrections.az_pos_vff_s       = cp.az_pos_vff_s;
  corrections.az_pos_vff_t       = cp.az_pos_vff_t;
  corrections.az_neg_vff_s       = cp.az_neg_vff_s;
  corrections.az_neg_vff_t       = cp.az_neg_vff_t;
  m_controller->setCorrectionParameters(corrections);  
}

void NET_TCInterfaceServant::netReqStop()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  if(m_readonly)throw TCIReadonly();
  m_controller->reqStop();
}

void NET_TCInterfaceServant::netReqSlew()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  if(m_readonly)throw TCIReadonly();
  m_controller->reqSlew();
}

void NET_TCInterfaceServant::netReqTrack()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  if(m_readonly)throw TCIReadonly();
  m_controller->reqTrack();
}

void NET_TCInterfaceServant::netEmergencyStop()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  if(m_readonly)throw TCIReadonly();
  m_controller->emergencyStop();
}

void NET_TCInterfaceServant::netTerminate()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  if(m_readonly)throw TCIReadonly();
  if(m_terminate_notifier)m_terminate_notifier->broadcast();
}

void NET_TCInterfaceServant::
packLimits(NET_MotionLimits& net, const TelescopeMotionLimits* lim)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  bool limits_set = false;

  if(lim == 0)
    {
      CORBA::Boolean none = true;
      net.none(none);
      limits_set = true;
    }

  if(!limits_set)
    {
      const KeyLimits* key = dynamic_cast<const KeyLimits*>(lim);
      if(key)
	{
	  NET_KeyLimits val;
	  val.key_az_cw = key->getKeyAzCWDeg();
	  val.key_az_cc = key->getKeyAzCCDeg();
	  val.key_el_dn = key->getKeyElDNDeg();
	  val.lim_el_up = key->getLimElUPDeg();
	  val.lim_el_dn = key->getLimElDNDeg();
	  val.lim_az_cw = key->getLimAzCWDeg();
	  val.lim_az_cc = key->getLimAzCCDeg();
	  val.max_az_speed = key->maxAzSpeedDPS();
	  val.max_el_speed = key->maxElSpeedDPS();
	  net.key(val);
	  limits_set = true;
	}
    }

  if(!limits_set)
    {
      const PrimaryLimits* pri = dynamic_cast<const PrimaryLimits*>(lim);
      if(pri)
	{
	  NET_PrimaryLimits val;
	  val.lim_az_cw = pri->getLimAzCWDeg();
	  val.lim_az_cc = pri->getLimAzCCDeg();
	  val.lim_el_up = pri->getLimElUPDeg();
	  val.lim_el_dn = pri->getLimElDNDeg();
	  val.max_az_speed = pri->maxAzSpeedDPS();
	  val.max_el_speed = pri->maxElSpeedDPS();
	  net.primary(val);
	  limits_set = true;
	}
    }

  if(!limits_set)
    {
      const RestrictedLowElevationMotion* lo = 
	dynamic_cast<const RestrictedLowElevationMotion*>(lim);
      
      if(lo)
	{
	  NET_RestrictedLowElevationMotionLimits val;
	  val.low_el        = lo->getLowElDeg();
	  val.max_az_slew   = lo->getMazAzSlewDeg();
	  val.el_hysteresis = lo->getElHysteresis();
	  val.max_az_speed  = lo->maxAzSpeedDPS();
	  val.max_el_speed  = lo->maxElSpeedDPS();
	  net.low_el(val);
	  limits_set = true;
	}
    }

  if(!limits_set)
    {
      const NotchedInclusionLimits* notched = 
	dynamic_cast<const NotchedInclusionLimits*>(lim);
      
      if(notched)
	{
	  NET_NotchedInclusionLimits val;

	  std::vector<NotchedInclusionLimits::Notch> notches = 
	    notched->getNotches();

	  unsigned nnotch = notches.size();
	  val.notches.length(nnotch);
	  for(unsigned inotch=0; inotch<nnotch; inotch++)
	    {
	      val.notches[inotch].cc_limit = notches[inotch].cc_limit;
	      val.notches[inotch].cw_limit = notches[inotch].cw_limit;
	      val.notches[inotch].dn_limit = notches[inotch].dn_limit;
	    }

	  val.dn_lim        = notched->getLimElDNDeg();
	  val.max_az_speed  = notched->maxAzSpeedDPS();
	  val.max_el_speed  = notched->maxElSpeedDPS();
	  net.notched_inclusion(val);
	  limits_set = true;
	}
    }

  if(!limits_set)
    {
      const LimitsList* list = dynamic_cast<const LimitsList*>(lim);
      
      if(list)
	{
	  NET_MotionLimits::NET_MLList val;
	  unsigned nlim = list->limits().size();
	  val.seq.length(nlim);
	  LimitsList::iterator iitem = list->begin();
	  for(unsigned ilim=0; ilim<nlim; ilim++,iitem++)
	    packLimits(val.seq[ilim], *iitem);

	  val.max_az_speed  = list->maxAzSpeedDPS();
	  val.max_el_speed  = list->maxElSpeedDPS();
	  net.list(val);
	  limits_set = true;
	}
    }

  assert(limits_set);
}

TelescopeMotionLimits* NET_TCInterfaceServant::
unpackLimits(const NET_MotionLimits& net)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  TelescopeMotionLimits* lim = 0;

  switch(net._d())
    {
    case NET_ML_NONE:
      break;
    case NET_ML_PRIMARY:
      lim = new PrimaryLimits(net.primary().lim_az_cw,
			      net.primary().lim_az_cc,
			      net.primary().lim_el_dn,
			      net.primary().lim_el_up,
			      net.primary().max_az_speed,
			      net.primary().max_el_speed);
      break;
    case NET_ML_RESTICTED_LOW_EL_MOTION:
      lim = new RestrictedLowElevationMotion(net.low_el().low_el,
					     net.low_el().max_az_slew,
					     net.low_el().max_az_speed,
					     net.low_el().max_el_speed);
      break;
    case NET_ML_NOTCHED_INCLUSION:
      {
	std::vector<NotchedInclusionLimits::Notch> notches;
	unsigned nnotch = net.notched_inclusion().notches.length();
	notches.resize(nnotch);
	for(unsigned inotch=0;inotch<nnotch;inotch++)
	  {
	    notches[inotch].cc_limit = 
	      net.notched_inclusion().notches[inotch].cc_limit;
	    notches[inotch].cw_limit = 
	      net.notched_inclusion().notches[inotch].cw_limit;
	    notches[inotch].dn_limit = 
	      net.notched_inclusion().notches[inotch].dn_limit;
	  }
	lim = new NotchedInclusionLimits(notches,
					 net.notched_inclusion().dn_lim,
					 net.notched_inclusion().max_az_speed,
					 net.notched_inclusion().max_el_speed);
      }
      break;
    case NET_ML_KEY:
      lim = new KeyLimits(net.key().max_az_speed,
			  net.key().max_el_speed,
			  net.key().key_az_cw,
			  net.key().key_az_cc,
			  net.key().key_el_dn,
			  net.key().lim_el_dn,
			  net.key().lim_el_up,
			  net.key().lim_az_cw,
			  net.key().lim_az_cc);
      break;
    case NET_ML_LIST:
      {
	LimitsList* lim_list = new LimitsList(net.list().max_az_speed,
					      net.list().max_el_speed);
	unsigned nlim = net.list().seq.length();
	for(unsigned ilim=0;ilim<nlim;ilim++)
	  lim_list->pushBack(unpackLimits(net.list().seq[ilim]));
	lim = lim_list;
      }
      break;
    }
  
  return lim;
}

void NET_TCInterfaceServant::
packCoordinateOffset(NET_CoordinateOffset& net, const CoordinateOffset* off)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  bool off_set = false;

  if(off == 0)
    {
      CORBA::Boolean none = true;
      net.null(none);
      off_set = true;
    }

  if(!off_set)
    {
      const OnOffOffset* onoff = dynamic_cast<const OnOffOffset*>(off);
      if(onoff)
	{
	  NET_CoordinateOffsetOnOff val;
	  val.offset_time_rad = onoff->getOffsetTime().rad();
	  net.onoff(val);
	  off_set = true;
	}
    }

  if(!off_set)
    {
      const WobbleOffset* wobble = dynamic_cast<const WobbleOffset*>(off);
      if(wobble)
	{
	  NET_CoordinateOffsetWobble val;
	  val.wobble_coords_theta_rad = wobble->getWobbleCoords().thetaRad();
	  val.wobble_coords_phi_rad   = wobble->getWobbleCoords().phiRad();
	  net.wobble(val);
	  off_set = true;
	}
    }

  if(!off_set)
    {
      const OrbitOffset* orbit = dynamic_cast<const OrbitOffset*>(off);
      if(orbit)
	{
	  NET_CoordinateOffsetOrbit val;
	  val.orbit_coords_theta_rad = orbit->getOrbitCoords().thetaRad();
	  val.orbit_coords_phi_rad   = orbit->getOrbitCoords().phiRad();
	  val.orbit_period_day       = orbit->getOffsetPeriodDay();
	  net.orbit(val);
	  off_set = true;
	}
    }

  if(!off_set)
    {
      const ElAzOffset* elaz = dynamic_cast<const ElAzOffset*>(off);
      if(elaz)
	{
	  NET_CoordinateOffsetElAz val;
	  val.elaz_coords_theta_rad = elaz->getElAzCoords().thetaRad();
	  val.elaz_coords_phi_rad   = elaz->getElAzCoords().phiRad();
	  net.elaz(val);
	  off_set = true;
	}
    }

  assert(off_set);
}

CoordinateOffset* NET_TCInterfaceServant::
unpackCoordinateOffset(const NET_CoordinateOffset& net)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  CoordinateOffset* off = 0;

  switch(net._d())
    {
    case NET_CO_NULL:
      // nothing to do
      break;
    case NET_CO_ONOFF:
      off = new OnOffOffset(SEphem::Angle::makeRad(net.onoff().
						   offset_time_rad));
      break;
    case NET_CO_WOBBLE:
      off = new WobbleOffset(SEphem::SphericalCoords::
			     makeRad(net.wobble().
				     wobble_coords_theta_rad,
				     net.wobble().
				     wobble_coords_phi_rad));
      break; 
    case NET_CO_ORBIT:
      off = new OrbitOffset(SEphem::SphericalCoords::
			    makeRad(net.orbit().
				    orbit_coords_theta_rad,
				    net.orbit().
				    orbit_coords_phi_rad),
			    net.orbit().orbit_period_day);
      break; 
    case NET_CO_ELAZOFF:
      off = new ElAzOffset(SEphem::SphericalCoords::
			     makeRad(net.elaz().
				     elaz_coords_theta_rad,
				     net.elaz().
				     elaz_coords_phi_rad));
    }
  return off;
}

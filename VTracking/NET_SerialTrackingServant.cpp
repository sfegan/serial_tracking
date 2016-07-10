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
 * $Author: sfegan $
 * $Date: 2010/10/28 14:48:06 $
 * $Revision: 2.5 $
 * $Tag$
 *
 **/

#include<cassert>

#include"NET_SerialTrackingServant.h"

using namespace SEphem;
using namespace VTracking;
using namespace VMessaging;

NET_SerialTrackingServant::
NET_SerialTrackingServant(TelescopeController* controller,
			  const StowObjectVector& stow_pos,
			  bool readonly, 
			  ZThread::Condition* terminate_notifier)
  : POA_VSerialTracking::SimpleCommand(), 
    PortableServer::RefCountServantBase(),
    m_controller(controller), m_stow_pos(stow_pos), m_readonly(readonly),
    m_terminate_notifier(terminate_notifier)
{
  // nothing to see here
}

NET_SerialTrackingServant::~NET_SerialTrackingServant()
{
  // nothing to see here
}

void NET_SerialTrackingServant::nAlive()
{
  // nothing to see here
}

void NET_SerialTrackingServant::
nGetAzElTargetDetails(CORBA::Double& az_rad, CORBA::Double& el_rad, 
		      CORBA::Boolean& use_corrections, 
		      CORBA::Boolean& use_convergent_pointing)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  TelescopeController::StateElements tse = m_controller->tse();

  TargetObject* object = m_controller->getTargetObject();
  if(!object)throw WrongTargetType();

  use_convergent_pointing = false;
#if 0
  const ConvergentPointinglObject* convergent 
    = dynamic_cast<const ConvergentPointinglObject*>(object);
  if(convergent)
    {
      use_convergent_pointing = true;
      TargetObject* sub_object = convergent->getTargetObject();
      delete object;
      object = sub_object;
      if(!object)throw(WrongTargetType);
    }
#endif

  const AzElObject* azel = dynamic_cast<const AzElObject*>(object);
  if(azel)
    {
      az_rad          = azel->coords().longitudeRad();
      el_rad          = azel->coords().latitudeRad();
      use_corrections = azel->useCorrections();
      delete object;
      return;
    }

  const StowObject* stow = dynamic_cast<const StowObject*>(object);
  if(stow)
    {
      az_rad          = stow->coords().longitudeRad();
      el_rad          = stow->coords().latitudeRad();
      use_corrections = stow->useCorrections();
      delete object;
      return;
    }

  delete object;
  throw WrongTargetType();
  return;
}

void NET_SerialTrackingServant::
nGetRADecTargetDetails(CORBA::Double& ra_rad, CORBA::Double& dec_rad,
		       CORBA::Double& epoch, 
		       CORBA::Double& offset_time_sidereal_min, 
		       CORBA::Double& wobble_offset_rad, 
		       CORBA::Double& wobble_direction_rad,
		       CORBA::String_out name, 
		       CORBA::Boolean& use_convergent_pointing)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  TelescopeController::StateElements tse = m_controller->tse();

  TargetObject* object = m_controller->getTargetObject();
  if(!object)throw WrongTargetType();

  use_convergent_pointing = false;
#if 0
  const ConvergentPointinglObject* convergent 
    = dynamic_cast<const ConvergentPointinglObject*>(object);
  if(convergent)
    {
      use_convergent_pointing = true;
      TargetObject* sub_object = convergent->getTargetObject();
      delete object;
      object = sub_object;
      if(!object)throw WrongTargetType();
    }

#endif

  const RaDecObject* radec = dynamic_cast<const RaDecObject*>(object);
  if(radec)
    {
      ra_rad                   = radec->coords().longitudeRad();
      dec_rad                  = radec->coords().latitudeRad();
      epoch                    = radec->epoch();

      const CoordinateOffset* off = radec->offset();

      const OnOffOffset* onoff = dynamic_cast<const OnOffOffset*>(off);
      if(onoff)
	{
	  offset_time_sidereal_min = onoff->getOffsetTime().hrsPM()*60;
	}

      const WobbleOffset* wobble = dynamic_cast<const WobbleOffset*>(off);
      if(wobble)
	{
	  Angle phi = Angle::frDeg(180) - wobble->getWobbleCoords().phiRad();
	  wobble_offset_rad    = wobble->getWobbleCoords().thetaRad();
	  wobble_direction_rad = phi.rad();
	}

      name                     = CORBA::string_dup(radec->name().c_str());
      delete object;
      return;
    };

  delete object;
  throw WrongTargetType();
  return;
}

void NET_SerialTrackingServant::
nGetStatus(CORBA::Double& mjd, 
	   CORBA::Boolean& error, CORBA::Boolean& interlock,
	   CORBA::Boolean& limits_hit, CORBA::Boolean& is_stopped, 
	   TargetObjectType& target_type, 
	   CORBA::Double& scope_az_rad, CORBA::Double& scope_el_rad, 
	   CORBA::Double& target_az_rad, CORBA::Double& target_el_rad,
	   CORBA::Double& target_ra_rad, CORBA::Double& target_dec_rad, 
	   CORBA::Double& slew_time_sec)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  TelescopeController::StateElements tse = m_controller->tse();

  mjd                 = tse.mjd;

  if(tse.state == TelescopeController::TS_COM_FAILURE)
    {
      error           = true;
      interlock       = false;
      limits_hit      = false;
      is_stopped      = true;
      target_type     = TO_UNKNOWN;
      scope_az_rad    = 0;
      scope_el_rad    = 0;
      target_az_rad   = 0;
      target_el_rad   = 0;
      target_ra_rad   = 0;
      target_dec_rad  = 0;
      slew_time_sec   = 0;
      return;
    }

  error               = ( tse.status.az.servo1Fail
			  || tse.status.az.servo2Fail
			  || tse.status.az.positionFault
			  || tse.status.el.servo1Fail
			  || tse.status.el.servo2Fail
			  || tse.status.el.positionFault
			  || !tse.status.checksumOK
			  || tse.status.msgBadFrame
			  || tse.status.msgCommandInvalid
			  || tse.status.msgInputOverrun
			  || tse.status.msgOutputOverrun );

  interlock           = ( tse.status.interlock
			  || tse.status.interlockAzPullCord
			  || tse.status.interlockAzStowPin
			  || tse.status.interlockElStowPin 
			  || tse.status.interlockAzDoorOpen
			  || tse.status.interlockElDoorOpen
			  || tse.status.interlockSafeSwitch
			  || !tse.status.remoteControl );

  limits_hit          = ( tse.status.az.limitCwUp
			  || tse.status.az.limitCcwDown
			  || tse.status.el.limitCwUp
			  || tse.status.el.limitCcwDown );

  is_stopped          = tse.state == TelescopeController::TS_STOP;

  scope_az_rad        = tse.tel_azel.longitudeRad();
  scope_el_rad        = tse.tel_azel.latitudeRad();

  CorrectionParameters::DirectionPreference dp;
  TargetObject* object = m_controller->getTargetObject(dp);
  if(!object)
    {
      target_type     = TO_NULL;
      target_az_rad   = 0;
      target_el_rad   = 0;
      target_ra_rad   = 0;
      target_dec_rad  = 0;
      slew_time_sec   = 0;
      return;
    }

  if(dynamic_cast<const AzElObject*>(object))
    {
      target_type     = TO_AZEL;
    }
  else if(dynamic_cast<const RaDecObject*>(object))
    {
      target_type     = TO_RADEC;
    }
  else
    {
      target_type     = TO_UNKNOWN;
    }

  target_az_rad       = tse.obj_azel.longitudeRad();
  target_el_rad       = tse.obj_azel.latitudeRad();

  SphericalCoords radec = 
    object->getRaDec(tse.mjd, tse.lmst, m_controller->getEarthPos());

  target_ra_rad       = radec.longitudeRad();
  target_dec_rad      = radec.latitudeRad();

  if(tse.state == TelescopeController::TS_TRACK)
    slew_time_sec     = 0;
  else
    {
      CorrectionParameters tcp = m_controller->getCorrections();
      double tar_az_driveangle = target_az_rad;
      double tar_el_driveangle = target_el_rad;

      tcp.doAzElCorrections(tar_az_driveangle,
			    tar_el_driveangle,
			    Angle::frDeg(tse.status.az.driveangle_deg),
			    object->useCorrections(), dp);
      
      const double az_sep = 
	Angle::toDeg(tar_az_driveangle) - tse.status.az.driveangle_deg;
      const double el_sep = 
	Angle::toDeg(tar_el_driveangle) - tse.status.el.driveangle_deg;
      const double el_eta = fabs(el_sep)/tse.el_slew_speed_dps;
      const double az_eta = fabs(az_sep)/tse.az_slew_speed_dps;
      slew_time_sec = (el_eta>az_eta)?el_eta:az_eta;
    }

  delete object;

  return;
}

void NET_SerialTrackingServant::nStop()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(m_readonly)throw CommandsNotPermitted();
  m_controller->reqStop();
}

void NET_SerialTrackingServant::nSlewToStow()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(m_readonly)throw CommandsNotPermitted();
  TelescopeController::StateElements tse = m_controller->tse();
  if(tse.state != TelescopeController::TS_STOP)throw NotStopped();
  for(StowObjectVector::const_iterator ipos = m_stow_pos.begin();
      ipos != m_stow_pos.end(); ipos++)
    if(ipos->name() == "STOW")
      {
	m_controller->setTargetObject(ipos->copy());
	m_controller->reqSlew();
	break;
      }
}

void NET_SerialTrackingServant::nSlewToZenith()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(m_readonly)throw CommandsNotPermitted();
  TelescopeController::StateElements tse = m_controller->tse();
  if(tse.state != TelescopeController::TS_STOP)throw NotStopped();
  double az_deg=tse.status.az.driveangle_deg;
  TargetObject * obj =
    new AzElObject(SphericalCoords::makeLatLongDeg(90,az_deg),false,true);
  m_controller->setTargetObject(obj);
  m_controller->reqSlew();
}

void NET_SerialTrackingServant::
nSlewToAzEl(CORBA::Double az_rad, CORBA::Double el_rad, 
	    CORBA::Boolean use_corrections, 
	    CORBA::Boolean use_convergent_pointing)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(m_readonly)throw CommandsNotPermitted();
  TelescopeController::StateElements tse = m_controller->tse();
  if(tse.state != TelescopeController::TS_STOP)throw NotStopped();
  SphericalCoords pos = SphericalCoords::makeLatLongRad(el_rad,az_rad);
  TargetObject* obj =  new AzElObject(pos,use_corrections,true);
#if 0
  if(use_convergent_pointing)
    obj = new ConvergentPointingObject(obj, blah, blah, blah);
#endif
  m_controller->setTargetObject(obj);
  m_controller->reqSlew();
}

void NET_SerialTrackingServant::
nTrackTargetOnOff(CORBA::Double ra_rad, CORBA::Double dec_rad, 
		  CORBA::Double epoch, 
		  CORBA::Double offset_time_sidereal_min, 
		  CORBA::Double wobble_offset_rad, 
		  CORBA::Double wobble_direction_rad, 
		  const char* name, 
		  CORBA::Boolean use_convergent_pointing)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(m_readonly)throw CommandsNotPermitted();
  TelescopeController::StateElements tse = m_controller->tse();
  if(tse.state != TelescopeController::TS_STOP)throw NotStopped();

  SphericalCoords pos = SphericalCoords::makeLatLongRad(dec_rad,ra_rad);

  CoordinateOffset* off = 0;
  if(wobble_offset_rad != 0)
    off = new WobbleOffset(SphericalCoords::
			   makeRad(wobble_offset_rad, wobble_direction_rad));
  else if(offset_time_sidereal_min != 0)
    off = new OnOffOffset(Angle::makeHrs(offset_time_sidereal_min/60.0));

  TargetObject* obj=0;
  obj = new RaDecObject(pos,epoch,name,off);

#if 0
  if(use_convergent_pointing)
    obj = new ConvergentPointingObject(obj, blah, blah, blah);
#endif
  m_controller->setTargetObject(obj);
  m_controller->reqTrack();
}

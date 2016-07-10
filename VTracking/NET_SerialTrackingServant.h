//-*-mode:c++; mode:font-lock;-*-

/**
 * \file NET_SerialTrackingServant.h
 * \ingroup VTracking
 * \brief Servant code for simpler CORBA interface used by ArrayControl
 * and other external control agents
 *
 * This class interfaces to some TelescoperController, translating the
 * simple CORBA control interface into its language
 *
 * Original Author: Stephen Fegan
 * $Author: sfegan $
 * $Date: 2007/01/23 01:36:05 $
 * $Revision: 2.2 $
 * $Tag$
 *
 **/

#ifndef VTRACKING_NETSERIALTRACKINGSERVANT_H
#define VTRACKING_NETSERIALTRACKINGSERVANT_H

#include"TargetObject.h"
#include"TelescopeController.h"
#include"NET_SerialTracking.h"

namespace VTracking 
{  
  
  class NET_SerialTrackingServant: 
    public POA_VSerialTracking::SimpleCommand,
    public PortableServer::RefCountServantBase
  {
  public:
    NET_SerialTrackingServant(TelescopeController* controller,
			      const StowObjectVector& stow_pos,
			      bool readonly = false,
			      ZThread::Condition* terminate_notifier = 0);
    virtual ~NET_SerialTrackingServant();

    // methods corresponding to defined IDL attributes and operations

    void nAlive();

    void nGetAzElTargetDetails(CORBA::Double& az_rad, CORBA::Double& el_rad, 
			       CORBA::Boolean& use_corrections, 
			       CORBA::Boolean& use_convergent_pointing);

    void nGetRADecTargetDetails(CORBA::Double& ra_rad, CORBA::Double& dec_rad,
				CORBA::Double& epoch, 
				CORBA::Double& offset_time_sidereal_min, 
				CORBA::Double& wobble_offset_rad, 
				CORBA::Double& wobble_direction_rad,
				CORBA::String_out name, 
				CORBA::Boolean& use_convergent_pointing);

    void nGetStatus(CORBA::Double& mjd, 
		    CORBA::Boolean& error, CORBA::Boolean& interlock,
		    CORBA::Boolean& limits_hit, CORBA::Boolean& is_stopped, 
		    TargetObjectType& target_type, 
		    CORBA::Double& scope_az_rad, CORBA::Double& scope_el_rad, 
		    CORBA::Double& target_az_rad, CORBA::Double& target_el_rad,
		    CORBA::Double& target_ra_rad, 
		    CORBA::Double& target_dec_rad, 
		    CORBA::Double& slew_time_sec);

    void nStop();

    void nSlewToStow();

    void nSlewToZenith();

    void nSlewToAzEl(CORBA::Double az_rad, CORBA::Double el_rad, 
		     CORBA::Boolean use_corrections, 
		     CORBA::Boolean use_convergent_pointing);

    void nTrackTargetOnOff(CORBA::Double ra_rad, CORBA::Double dec_rad, 
			   CORBA::Double epoch, 
			   CORBA::Double offset_time_sidereal_min, 
			   CORBA::Double wobble_offset_rad, 
			   CORBA::Double wobble_direction_rad, 
			   const char* name, 
			   CORBA::Boolean use_convergent_pointing);

  private:
    NET_SerialTrackingServant(const NET_SerialTrackingServant&);
    const NET_SerialTrackingServant& 
    operator=(const NET_SerialTrackingServant&);

    TelescopeController*   m_controller;
    StowObjectVector       m_stow_pos;
    bool                   m_readonly;
    ZThread::Condition*    m_terminate_notifier;
  };

};

#endif // VTRACKING_NETSERIALTRACKINGSERVANT_H

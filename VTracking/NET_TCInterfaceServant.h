//-*-mode:c++; mode:font-lock;-*-

/**
 * \file NET_TCInterfaceServant.h
 * \ingroup VTracking
 * \brief Servant code to loop CORBA commands to TelescopeController
 *
 * This class makes the local TelescoperController interface available
 * over CORBA.
 *
 * Original Author: Stephen Fegan
 * $Author: sfegan $
 * $Date: 2010/10/28 14:48:06 $
 * $Revision: 2.4 $
 * $Tag$
 *
 **/

#ifndef VTRACKING_NETTCINTERFACESERVANT_H
#define VTRACKING_NETTCINTERFACESERVANT_H

#include"TargetObject.h"
#include"TelescopeController.h"
#include"NET_TCInterface.h"

namespace VTracking 
{  
  
  class NET_TCInterfaceServant: 
    public POA_VTracking::NET_TCInterface,
    public PortableServer::RefCountServantBase
  {
  public:
    NET_TCInterfaceServant(TelescopeController* controller,
			   const StowObjectVector& stow_pos,
			   bool readonly = false,
			   ZThread::Condition* terminate_notifier = 0);
    virtual ~NET_TCInterfaceServant();

    // methods corresponding to defined IDL attributes and operations
    NET_TCMiscData* netGetMiscData();
    NET_StateElements* netGetTelescopeState();
    void netSetTargetObjectNull();
    void netSetTargetObjectStow(const char* name);
    void netSetTargetObjectZenith();
    void netSetTargetObjectRaDec(CORBA::Double ra_rad, CORBA::Double dec_rad,
				 CORBA::Double epoch, 
				 const NET_CoordinateOffset& offset,
				 const char* name,
				 NET_DirectionPreference dir_pref);
    void netSetTargetObjectAzEl(CORBA::Double az_rad, CORBA::Double el_rad,
				bool use_corrections, bool stop_at_target,
				NET_DirectionPreference dir_pref);
    void netSetTargetObjectCV(CORBA::Double az_rad, CORBA::Double el_rad,
			      CORBA::Double az_speed, CORBA::Double el_speed,
			      CORBA::Double mjd_zero);

    void netSetCorrectionParameters(const VTracking::
				    NET_CorrectionParameters& cp);
    void netReqStop();
    void netReqSlew();
    void netReqTrack();
    void netEmergencyStop();
    void netTerminate();

    static
    void packLimits(NET_MotionLimits& net, const TelescopeMotionLimits* lim);
    static TelescopeMotionLimits* 
    unpackLimits(const NET_MotionLimits& net);

    static void 
    packCoordinateOffset(NET_CoordinateOffset& net, 
			 const CoordinateOffset* off);
    static CoordinateOffset* 
    unpackCoordinateOffset(const NET_CoordinateOffset& net);

  private:
    NET_TCInterfaceServant(const NET_TCInterfaceServant&);
    const NET_TCInterfaceServant& operator=(const NET_TCInterfaceServant&);

    TelescopeController*   m_controller;
    StowObjectVector       m_stow_pos;
    bool                   m_readonly;
    ZThread::Condition*    m_terminate_notifier;
  };

};

#endif // VTRACKING_NETTCINTERFACESERVANT_H

//-*-mode:c++; mode:font-lock;-*-

/**
 * \file TelescopeControllerRemote.h
 * \ingroup VTracking
 * \brief TelecopeController which communicates with a remote controller
 * using CORBA.
 *
 * This derived class does not communicates directly with the telescope,
 * rather it is a proxy for a remote controller. This allows an array
 * GUI command individual telescope controllers without having to be on
 * the same network as the telescopes.
 *
 * Original Author: Stephen Fegan
 * $Author: sfegan $
 * $Date: 2010/10/28 14:48:06 $
 * $Revision: 2.8 $
 * $Tag$
 *
 **/

#ifndef VTRACKING_TELESCOPECONTROLLERREMOTE_H
#define VTRACKING_TELESCOPECONTROLLERREMOTE_H

#include<iostream>
#include<string>

#include<time.h>
#include<sys/times.h>

#include<zthread/RecursiveMutex.h>
#include<zthread/Runnable.h>

#include<Angle.h>
#include<SphericalCoords.h>
#include<VOmniORBHelper.h>

#include"TargetObject.h"
#include"TelescopeController.h"
#include"NET_TCInterface.h"

namespace VTracking 
{

  // Avoid circular includes by making forward declaration and NOT
  // including PositionLogger.h
  class PositionLogger;

  // As of version 1.1 TelescopeController is base class for "remote"
  // and "remote" controller classes which talk to the telescope
  // directly or using CORBA
  class TelescopeControllerRemote: public TelescopeController
  {
  public:

    // ------------------------------------------------------------------------
    // CONSTRUCTOR AND DESTRUCTOR
    // ------------------------------------------------------------------------

    TelescopeControllerRemote(NET_TCInterface_ptr tc_interface,
			      int sleep_time, int phase_time,
			      bool readonly = false,
			      bool no_stop_on_entry_error_exit = false,
			      const SEphem::SphericalCoords& earth_pos_zero =
			      SEphem::SphericalCoords(0,0));
    TelescopeControllerRemote(VCorba::VOmniORBHelper* orb, 
			      const std::string& ior, 
			      int sleep_time, int phase_time,
			      bool readonly = false, 
			      bool no_stop_on_entry_error_exit = false,
			      const SEphem::SphericalCoords& earth_pos_zero =
			      SEphem::SphericalCoords(0,0));
    TelescopeControllerRemote(VCorba::VOmniORBHelper* orb, int scope_num,
			      int sleep_time, int phase_time,
			      bool readonly = false,
			      bool no_stop_on_entry_error_exit = false,
			      const SEphem::SphericalCoords& earth_pos_zero =
			      SEphem::SphericalCoords(0,0));
    virtual ~TelescopeControllerRemote();    
    virtual std::string controllerName() const;
    
    // ------------------------------------------------------------------------
    // SET STATE
    // ------------------------------------------------------------------------

    virtual void setTargetObject(TargetObject* obj, 
	DirectionPreference dp = SEphem::CorrectionParameters::DP_NONE);
    virtual void reqStop();
    virtual void reqSlew();
    virtual void reqTrack();
    virtual void emergencyStop();
    virtual void 
    setCorrectionParameters(const SEphem::CorrectionParameters& cp);

    // ------------------------------------------------------------------------
    // EXTENDED FUNCTIONALITY
    // ------------------------------------------------------------------------

    virtual bool hasTerminateRemoteCapability();
    virtual void terminateRemote() throw ( CapabilityNotSupported );

  protected:
    enum RefSource { RS_PROVIDED, RS_IOR, RS_NS };

    virtual void loopIsTerminating();
    virtual void iterate();

    void timeout(const std::string& text = "");
    void exception(const CORBA::Exception& x);
    void readonly(bool remote = false);

    RefSource                        m_ref_source;
    VCorba::VOmniORBHelper*          m_orb;
    int                              m_scope_num;
    std::string                      m_ior;
    NET_TCInterface_ptr              m_tc_interface;
    bool                             m_local_com_failure;
    bool                             m_readonly;
    bool                             m_no_stop_on_entry_error_exit;
  }; // class TelescopeController
}

#endif // VTRACKING_TELESCOPECONTROLLERREMOTE_H

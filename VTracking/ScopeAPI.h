//-*-mode:c++; mode:font-lock;-*-

/**
 * \file ScopeAPI.h
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
 * $Date: 2007/01/23 01:36:05 $
 * $Revision: 2.1 $
 * $Tag$
 *
 **/

#ifndef VTRACKING_SCOPEAPI_H
#define VTRACKING_SCOPEAPI_H

#include<Angle.h>

#include<Exception.h>

namespace VTracking
{
  class ScopeAPI
  {
  public:
    enum DriveMode { DM_STANDBY=0x00, DM_SLEW=0x01, DM_POINT=0x02,
		     DM_SPIN=0x03, DM_SECTOR_SCAN=0x04, 
		     DM_RASTER=0x05, DM_CHANGING=0x0F,
		     DM_UNKNOWN = 0xFF };

    class ScopeAPIError: public VMessaging::Exception
    {
    public:
      ScopeAPIError(const std::string& title, const std::string& message, 
		    const std::string& details=""): 
	Exception(title,message,details) { }
      virtual ~ScopeAPIError() throw();
    };

    class DriveStatus
    {
    public:
      double driveangle_deg;

      DriveMode driveMode;
      bool servo1Fail;
      bool servo2Fail;
      bool servoOn;

      bool brakeReleased;

      bool limitCwUp;
      bool limitCcwDown;

      bool positionFault;
      bool positionComplete;

      DriveStatus(): 
	driveangle_deg(0), driveMode(DM_UNKNOWN), servo1Fail(false), 
	servo2Fail(false), servoOn(false), brakeReleased(false), 
	limitCwUp(false), limitCcwDown(false), positionFault(false),
	positionComplete(false) {}

      void print(std::ostream& stream);
    };

    class PositionerStatus
    {
    public:
      DriveStatus az;
      DriveStatus el;

      bool azTravelledCCW;
      double azCableWrap;

      bool interlock;
      bool interlockAzPullCord;
      bool interlockAzStowPin;
      bool interlockElStowPin;
      bool interlockAzDoorOpen;
      bool interlockElDoorOpen;
      bool interlockSafeSwitch;

      bool remoteControl;
      bool checksumOK;

      bool msgBadFrame;
      bool msgCommandInvalid;
      bool msgInputOverrun;
      bool msgOutputOverrun;

      bool relay1;
      bool relay2;

      double Analog1;
      double Analog2;

      PositionerStatus(): 
	az(), el(), azTravelledCCW(), azCableWrap(0), interlock(false),
	interlockAzPullCord(false), interlockAzStowPin(false),
	interlockElStowPin(false), interlockAzDoorOpen(false),
	interlockElDoorOpen(false), interlockSafeSwitch(false),
	remoteControl(false), checksumOK(false), msgBadFrame(false), 
	msgCommandInvalid(false), msgInputOverrun(false), 
	msgOutputOverrun(false), 
	relay1(false), relay2(false), Analog1(0), Analog2(0) {}

      void print(std::ostream& stream);
    };

    class PIDParameters
    {
    public:
      int Kp;
      int Ki;
      int Kd;
      int Kvff;
      int Ilim;
      double vlim;  // in degrees per second
      double alim;  // in degrees per second squared

      PIDParameters(): 
	Kp(0), Ki(0), Kd(0), Kvff(0), Ilim(0), vlim(0), alim(0) {}
      
      void print(std::ostream& stream);
    };

    virtual ~ScopeAPI();
    
    virtual void reqStat(PositionerStatus& state) = 0;
    virtual void cmdStandby() = 0;
    virtual void cmdPoint(const SEphem::Angle& az_angle, double az_vel,
			  const SEphem::Angle& el_angle, double el_vel) = 0;
    virtual void cmdSlew(double az_vel, double el_vel) = 0;

#if 0
    virtual void reqOffsets(SEphem::Angle& az_angle,
			    SEphem::Angle& el_angle) = 0;
    virtual void setOffsets(const SEphem::Angle& az_angle,
			    const SEphem::Angle& el_angle) = 0;

    virtual void reqPIDParameters(PIDParameters& az_param,
				  PIDParameters& el_param) = 0;
    virtual void setPIDParameters(const PIDParameters& az_param,
				  const PIDParameters& el_param) = 0;
#endif

    virtual void reqAzOffset(SEphem::Angle& az_angle) = 0;
    virtual void reqElOffset(SEphem::Angle& el_angle) = 0;
    virtual void setAzOffset(const SEphem::Angle& az_angle) = 0;
    virtual void setElOffset(const SEphem::Angle& el_angle) = 0;

    virtual void reqAzPIDParameters(PIDParameters& az_param) = 0;
    virtual void reqElPIDParameters(PIDParameters& el_param) = 0;
    virtual void setAzPIDParameters(const PIDParameters& az_param) = 0;
    virtual void setElPIDParameters(const PIDParameters& el_param) = 0;

    virtual void resetCommunication() = 0;

    virtual std::string apiName() const = 0;

    static const double sc_az_max_vel = 0.5; // degrees per second
    static const double sc_el_max_vel = 0.5; // degrees per second
  }; // class ScopeAPI

  enum PreferredScopeAPI { PSA_NONE, PSA_EIA, PSA_PIU };
  
} // namespace VTracking

#endif // VTRACKING_SCOPEAPI_H

//-*-mode:c++; mode:font-lock;-*-

/**
 * \file TenMeterControlLoop.h
 * \ingroup VTracking
 * \brief This is a one-line description of this cpp file.
 *
 * Here is a tedious verbose multi-line description of all
 * the details of the code, more than you would
 * ever want to read. Generally, all the important documentation
 * goes in the .cpp files.
 *
 * Original Author: Deirdre Horan
 * $Author: sfegan $
 * $Date: 2006/08/28 23:00:30 $
 * $Revision: 2.5 $
 * $Tag$
 *
 **/

// All member variables have an "m_" as the start of their name

// To Do:
// - Put stuff in main to set up serial port (050831 email from Steve)

#ifndef VTRACKING_TELESCOPEEMULATOR_H
#define VTRACKING_TELESCOPEEMULATOR_H
#include<sys/time.h>


#include<zthread/RecursiveMutex.h>
#include<zthread/Runnable.h>

#include "PID.h"
#include "ScopeAPI.h"
#include "TenMeter.h"

#define AZIMUTH   0
#define ELEVATION 1

// ** NB - make sure EL_MAXRPM is 2800 and AZ_MAXRPM is 2500**
#define AZ_DERIVATIVEGAIN    0
#define AZ_INTEGRALGAIN      5
#define AZ_PROPORTIONALGAIN  60
#define AZ_MAXRPS            2500.00/60.0

#define EL_DERIVATIVEGAIN    0
#define EL_INTEGRALGAIN      10
#define EL_PROPORTIONALGAIN  60
#define EL_MAXRPS            2800.00/60.0

#define MAXSLEW_SEC   70/60.0     // Max. telescope velocity in deg/sec

#define AZ_GEARS  15098.0        // Motor rotations per telescope rotation
#define AZ_RPD    AZ_GEARS/360.0 // Motor rotations per deg.of movement

#define EL_GEARS  25435.0        // Motor rotations per telescope rotation
#define EL_RPD    EL_GEARS/360.0 // Motor rotations per deg.of movement

#define ENCODER_MAX          32767 // Max count of 15 bit encoder
#define ENCODER_TO_DEG       360/ENCODER_MAX;

#define ONE_RPS 65359.48/60.0 // Number that makes the motors move one RPS

#define EL_GAIN_SEC     30/60.0
#define EL2_GAIN_SEC    10/60.0
#define AZ_GAIN_SEC     30/60.0
#define AZ2_GAIN_SEC    10/60.0

#define TRACKGAIN 4

#define EL_DOWN 1        // limit_bits
#define EL_UP 2          // limit_bits
#define EL_15 4          // limit_bits
#define AZ_CCW 8         // limit_bits
#define AZ_CW 16         // limit_bits
#define WINDOW 32        // limit_bits
#define EL_OK 1          // status_bits
#define EL_AT_SPEED 2    // status_bits
#define AZ_OK 4          // status_bits
#define AZ_AT_SPEED 8    // status_bits
#define CW_CCW 16        // status_bits
#define HT_ON 32         // status_bits
#define EL_AUTO 64       // status_bits
#define AZ_AUTO 128      // status_bits

#define VELOCITY_ZERO 0  // velocity for "initializeBRUDrive" function

#define DEVADDRESS /dev/ttyS1

namespace VTracking
{
  class TenMeterControlLoop: public ScopeAPI, public ZThread::Runnable
  {
  private:
    class DriveInfo; // This is a forward declaration
  public:
    typedef double ang_t;

    TenMeterControlLoop(unsigned scope, DataStream* ds, 
			unsigned az_address, unsigned el_address,
			unsigned sleep_time_ms=50);
    virtual ~TenMeterControlLoop();
    
    // ------------------------------------------------------------------------
    // ScopeAPI members
    // ------------------------------------------------------------------------
    virtual void reqStat(PositionerStatus& state);
    virtual void cmdStandby();
    virtual void cmdPoint(const SEphem::Angle& az_angle, double az_vel,
			  const SEphem::Angle& el_angle, double el_vel);
    virtual void cmdSlew(double az_vel, double el_vel);

    virtual void reqAzOffset(SEphem::Angle& az_angle);
    virtual void reqElOffset(SEphem::Angle& el_angle);
    virtual void setAzOffset(const SEphem::Angle& az_angle);
    virtual void setElOffset(const SEphem::Angle& el_angle);

    virtual void reqAzPIDParameters(PIDParameters& az_param);
    virtual void reqElPIDParameters(PIDParameters& el_param);
    virtual void setAzPIDParameters(const PIDParameters& az_param);
    virtual void setElPIDParameters(const PIDParameters& el_param);

    virtual void resetCommunication();

    // ------------------------------------------------------------------------
    // Thread related members
    // ------------------------------------------------------------------------
    void terminate();
    virtual void run();

    // ------------------------------------------------------------------------
    // Other members
    // ------------------------------------------------------------------------

  protected:
    void tick();

  private:
    bool initializeBRUDrive(BRUDrive* bruDrive, DriveInfo& driveInfo);
    bool resetBRUDrive(BRUDrive* bruDrive, DriveInfo& driveInfo);
    bool setBRUDrive(BRUDrive* bruDrive, DriveInfo& driveInfo);
    bool getBRUDriveStatus(BRUDrive* bruDrive, DriveInfo& driveInfo, 
			   bool& powerupSuccess);
    void setDigitalInputs(BRUDrive* bruDrive, DriveInfo& driveInfo);
    bool readDigitalInputs(BRUDrive* bruDrive, DriveInfo& driveInfo);
    bool initializeDIOCards();
    bool readTelescopeInformation(ang_t& az, ang_t& el, uint8_t& limitBits, 
				  uint8_t& statusBits);
    int checkLimitAndStatusBits();
    bool initialize();


    // Functions from the emulator which I don't think I need
    // move;
    // doLimits;
    static int gb(int n, int m);
    static void printbin(uint32_t n, int m);
    static void split(uint32_t n, uint8_t *m);
    static int32_t dps_to_int(const DriveInfo* drive_in);
    
    
    // Thread related stuff
    int m_sleep_time;
    ZThread::RecursiveMutex m_mtx;
    bool m_exit_notification;
    
    unsigned m_scope_num;

    struct timeval m_last_tv;
    struct timeval m_last_tv_cmdPoint;

    PositionerStatus m_status;
    PositionerStatus m_last_status;

    uint8_t m_limitBits;
    uint8_t m_statusBits;

    ang_t m_az_ang;
    ang_t m_el_ang;
    ang_t m_az_vel;
    ang_t m_el_vel;

    ang_t m_az_req_ang;
    ang_t m_el_req_ang;
    ang_t m_az_req_vel;
    ang_t m_el_req_vel;

    // For storing the last requested position and velocity (not sure
    // that I need the velocity??)

    ang_t m_last_az_req_ang;
    ang_t m_last_el_req_ang;
    ang_t m_last_az_req_vel;
    ang_t m_last_el_req_vel;

    double m_time_diff;

    // Maximum allowable velocity

    ang_t m_az_req_max_vel;
    ang_t m_el_req_max_vel;

    ang_t m_az_abs_max_vel;
    ang_t m_el_abs_max_vel;

    // Bools that I need for checking limit bits

    bool m_cw_enable;
    bool m_ccw_enable;
    bool m_up_enable;
    bool m_down_enable;
    bool m_el_15;
    bool m_az_window;

    // Bools that I need for checking limit bits

    bool m_el_ready;
    bool m_az_ready;

    // BRU Drive stuff from TenMeter.h

    DataStream* m_datastream;
    BRUDrive* m_azDrive;
    BRUDrive* m_elDrive;

    // The DIO Cards

    int m_CIO_1;
    int m_CIO_2;

    // Make a class for storing information about the status of the Drives
    // This class does not talk to the drives - it is used to store the
    // information about the drives. When the BRUDrive classes access the
    // drives, they can get the information they need from here.
    
    class DriveInfo
    {
    public:
      DriveInfo(const std::string& name, int derivative, int integral, 
		int proportional, double maxRPM, double maxRPD );
      virtual ~DriveInfo();

      enum StateEnabled { SE_DISABLED, SE_ENABLED, SE_UNKNOWN };

      // Getters
      const std::string& getDriveName() const {return m_driveName;}
      int    getVelocity() const {return m_driveVelocity;}
      StateEnabled getStateEnabledFlag() const {return m_stateEnabledFlag;}
      bool   getSetpointMode() const {return m_setpointMode;}
      int    getDerivativeGain() const {return m_derivativeGain;}
      int    getIntegralGain() const {return m_integralGain;}
      int    getProportionalGain() const {return m_proportionalGain;}
      double getMaxRPS() const {return m_maxRPS;}
      double getRPD() const {return m_RPD;}

      // Setters
      void setVelocity(int velocity) {m_driveVelocity = velocity;}
      void setStateEnabledFlag(StateEnabled state) {m_stateEnabledFlag=state;}
      void setSetpointMode(bool setpoint) {m_setpointMode = setpoint;}
      void setDerivativeGain(int gain) {m_derivativeGain = gain;}
      void setIntegralGain(int gain) {m_integralGain = gain;}
      void setProportionalGain(int gain) {m_proportionalGain = gain;}
      void setMaxRPS(int rps) {m_maxRPS = rps;}
      void setRPD (double rpd) {m_RPD = rpd;}

    private:
      const std::string m_driveName;
      int  m_driveVelocity;
      StateEnabled m_stateEnabledFlag; 
      bool m_setpointMode;
      int  m_derivativeGain;
      int  m_integralGain;
      int  m_proportionalGain;
      double  m_maxRPS;
      double  m_RPD;
    }; // end of class DriveInfo

    // Create the DriveInfo Classes 
    DriveInfo m_azDriveInfo;
    DriveInfo m_elDriveInfo;
  }; // end of class TenMeterControlLoop

}; // end of namespace VTracking


#endif // VTRACKING_TELESCOPEEMULATOR_H


// All the stuff that was commented out above:
      // enum DriveInfoState {STATE_STANDBY, STATE_POINT, STATE_SLEW, 
      //   STATE_UNKNOWN};
      // bool   getDirectionFlag() const {return m_directionFlag;}
      // bool   getLastDirectionFlag() const {return m_lastDirectionFlag;}
      // void setLastVelocity(int lastVelocity); 
      // don't think I need cos I only
      // set the last velocity when the new velocity gets set
      // void setDirectionFlag(bool direction) {m_directionFlag = direction;}
      // void setLastDirectionFlag(bool lastDirection)
      // int  m_driveLastVelocity;
      // bool m_directionFlag;     // CW/UP: 1; CCW/DOWN:0
      // bool m_lastDirectionFlag; // CW/UP: 1; CCW/DOWN:0



//-*-mode:c++; mode:font-lock;-*-

/**
 * \file EIA422.h
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

#ifndef VTRACKING_EIA422_H
#define VTRACKING_EIA422_H

#include<Exception.h>
#include<Angle.h>
//#include<SphericalCoords.h>

#include"DataStream.h"
#include"ScopeAPI.h"
#include"ScopeProtocolServer.h"

namespace VTracking
{
  class EIA422ScopeAPI
  {
  public:
    enum ServoMode { SM_POSITION=0x00, SM_RATE=0x04, SM_IDLE=0x0C };
    
    class Status
    {
    public:
      bool limitCwUp;
      bool limitCcwDown;
      bool brakeReleased;
      bool positionComplete;
      bool interlock;
      bool positionFault;
      bool servoAmpFault;
      bool servoOn;
      ServoMode servoMode;
      bool commandUnderOverFlow;
      bool checksumOK;
      bool directionSignBit;
      bool commandUnrecognised;
      
      void print(std::ostream& stream);
    };

    class ExtStatus
    {
    public:
      bool remoteControl;
      double ADCValue;
      bool interlockStowPin;
      bool interlockPullCord;
      bool interlockHatch;
      bool interlockSafe;
      
      void print(std::ostream& stream);
    };
    
    virtual ~EIA422ScopeAPI();
    
    virtual Status reqAzStat()                       = 0;
    virtual Status reqElStat()                       = 0;
    virtual SEphem::Angle reqAzPos()                 = 0;
    virtual SEphem::Angle reqElPos()                 = 0;
    virtual ExtStatus reqAzExtStat()                 = 0;
    virtual ExtStatus reqElExtStat()                 = 0;
    virtual void   cmdElSrvPwrOn()                   = 0;
    virtual void   cmdElSrvPwrOff()                  = 0;
    virtual void   cmdAzSrvPwrOn()                   = 0;
    virtual void   cmdAzSrvPwrOff()                  = 0;
    virtual SEphem::Angle reqAzOffset()              = 0;
    virtual SEphem::Angle reqElOffset()              = 0;
    virtual void   cmdAzRate(double)                 = 0;
    virtual void   cmdElRate(double)                 = 0;
    virtual void   setAzOffset(const SEphem::Angle&) = 0;
    virtual void   setElOffset(const SEphem::Angle&) = 0;
    virtual int    reqAzRevision()                   = 0;
    virtual int    reqElRevision()                   = 0;
    virtual void   cmdLatch1On()                     = 0;
    virtual void   cmdLatch1Off()                    = 0;
    virtual void   cmdLatch2On()                     = 0;
    virtual void   cmdLatch2Off()                    = 0;
    virtual void   cmdAzPosn(const SEphem::Angle&)   = 0;
    virtual void   cmdElPosn(const SEphem::Angle&)   = 0;
    
#if 0
    virtual void   reqAzMCP()                        = 0;
    virtual void   reqElMCP()                        = 0;
#endif

    virtual int    reqAzKp()                         = 0;
    virtual int    reqAzKi()                         = 0;
    virtual int    reqAzKd()                         = 0;
    virtual int    reqAzIl()                         = 0;
    virtual int    reqAzAcc()                        = 0;
    virtual int    reqAzVel()                        = 0;
    virtual int    reqElKp()                         = 0;
    virtual int    reqElKi()                         = 0;
    virtual int    reqElKd()                         = 0;
    virtual int    reqElIl()                         = 0;
    virtual int    reqElAcc()                        = 0;
    virtual int    reqElVel()                        = 0;
    
    virtual void   setAzKp(int i)                    = 0;
    virtual void   setAzKi(int i)                    = 0;
    virtual void   setAzKd(int i)                    = 0;
    virtual void   setAzIl(int i)                    = 0;
    virtual void   setAzAcc(int i)                   = 0;
    virtual void   setAzVel(int i)                   = 0;
    virtual void   setElKp(int i)                    = 0;
    virtual void   setElKi(int i)                    = 0;
    virtual void   setElKd(int i)                    = 0;
    virtual void   setElIl(int i)                    = 0;
    virtual void   setElAcc(int i)                   = 0;
    virtual void   setElVel(int i)                   = 0;

    virtual void   resetCommunication()              = 0;

    virtual std::string apiName() const              = 0;
    
    static const double sc_el_vel_ratio = 0.4*ANGLE_RADPERDEG;
    static const double sc_az_vel_ratio = 0.4*ANGLE_RADPERDEG;

    static const double sc_adc_volts_per_dc = 0.00488;
  }; // class EIA422ScopeAPI

  class EIA422Protocol: public EIA422ScopeAPI, public ScopeProtocolServer
  {
  public:
    EIA422Protocol(DataStream* ds): m_data_stream(ds) { }
    virtual ~EIA422Protocol();

    virtual void resetCommunication();

    virtual void processOneCommand(long cmd_to_sec=0, long cmd_to_usec=0);

  protected:
    DataStream* m_data_stream;
    
    enum Command { RESPONSE        = 0xFF,
		   CMD_AZSTAT      = 0x01,
		   CMD_ELSTAT      = 0x02,
		   CMD_REQAZP      = 0x03,
		   CMD_REQELP      = 0x04,
		   CMD_ELSRVPWRON  = 0x05,
		   CMD_ELSRVPWROFF = 0x06,
		   CMD_AZSRVPWRON  = 0x07,
		   CMD_AZSRVPWROFF = 0x08,
		   CMD_AZOFFSETR   = 0x0D,
		   CMD_ELOFFSETR   = 0x0E,
		   CMD_AZSTAT2     = 0x0F,
		   CMD_ELSTAT2     = 0x1C,
		   CMD_AZRATE      = 0x21,
		   CMD_ELRATE      = 0x22,
		   CMD_AZOFFSET    = 0x23,
		   CMD_ELOFFSET    = 0x24,
		   CMD_AZREVISION  = 0x25,
		   CMD_ELREVISION  = 0x26,
		   CMD_LATCH1ON    = 0x27,
		   CMD_LATCH1OFF   = 0x28,
		   CMD_LATCH2ON    = 0x29,
		   CMD_LATCH2OFF   = 0x2A,
		   CMD_AZPOSN      = 0x31,
		   CMD_ELPOSN      = 0x32,
		   
		   CMD_REQAMP      = 0x1E,
		   CMD_REQEMP      = 0x1F,

		   CMD_REQAZKP     = 0x10,
		   CMD_REQAZKI     = 0x11,
		   CMD_REQAZKD     = 0x12,
		   CMD_REQAZIL     = 0x13,
		   CMD_REQAZACC    = 0x14,
		   CMD_REQAZVEL    = 0x15,
		   CMD_REQELKP     = 0x16,
		   CMD_REQELKI     = 0x17,
		   CMD_REQELKD     = 0x18,
		   CMD_REQELIL     = 0x19,
		   CMD_REQELACC    = 0x1A,
		   CMD_REQELVEL    = 0x1B,
		   
		   CMD_SETAZKP     = 0x34,
		   CMD_SETAZKI     = 0x35,
		   CMD_SETAZKD     = 0x36,
		   CMD_SETAZIL     = 0x37,
		   CMD_SETAZACC    = 0x38,
		   CMD_SETAZVEL    = 0x39,
		   CMD_SETELKP     = 0x3A,
		   CMD_SETELKI     = 0x3B,
		   CMD_SETELKD     = 0x3C,
		   CMD_SETELIL     = 0x3D,
		   CMD_SETELACC    = 0x3E,
		   CMD_SETELVEL    = 0x3F };
    
    typedef unsigned long CmdDataType;
    
    datastring encodeCommand(Command cmd, size_t bytes, 
			     CmdDataType data) const;
    CmdDataType decodeCommandData(const datastring& buffer) const;
    
    void encodeAndSendCommand(Command cmd, size_t bytes, 
			      CmdDataType data) const;
    CmdDataType recvAndDecodeCommandData(size_t bytes, 
					 const std::string& leftover="") const;

    CmdDataType encodeShort(int i) const;
    int decodeShort(CmdDataType i) const;
    
    CmdDataType encodeAngle16(const SEphem::Angle& angle) const;
    SEphem::Angle decodeAngle16(CmdDataType angle) const;
    
    CmdDataType encodeAngle24(const SEphem::Angle& angle) const;
    SEphem::Angle decodeAngle24(CmdDataType angle) const;
    
    CmdDataType encodeStatus(const Status& status) const;
    Status decodeStatus(CmdDataType data) const;

    CmdDataType encodeAzExtStatus(const ExtStatus& status) const;
    ExtStatus decodeAzExtStatus(CmdDataType data) const;
    CmdDataType encodeElExtStatus(const ExtStatus& status) const;
    ExtStatus decodeElExtStatus(CmdDataType data) const;

    CmdDataType encodeRate(double rate) const;
    double decodeRate(CmdDataType data) const;
  }; // class EIA422Protocol

  class EIA422Stub: public EIA422Protocol
  {
  public:
    EIA422Stub(DataStream* ds): EIA422Protocol(ds) {}
    virtual ~EIA422Stub();
    
    virtual Status        reqAzStat();
    virtual Status        reqElStat();
    virtual SEphem::Angle reqAzPos();
    virtual SEphem::Angle reqElPos();
    virtual ExtStatus     reqAzExtStat();
    virtual ExtStatus     reqElExtStat();
    virtual void          cmdElSrvPwrOn();
    virtual void          cmdElSrvPwrOff();
    virtual void          cmdAzSrvPwrOn();
    virtual void          cmdAzSrvPwrOff();
    virtual SEphem::Angle reqAzOffset();
    virtual SEphem::Angle reqElOffset();
    virtual void          cmdAzRate(double);
    virtual void          cmdElRate(double);
    virtual void          setAzOffset(const SEphem::Angle&);
    virtual void          setElOffset(const SEphem::Angle&);
    virtual int           reqAzRevision();
    virtual int           reqElRevision();
    virtual void          cmdLatch1On();
    virtual void          cmdLatch1Off();
    virtual void          cmdLatch2On();
    virtual void          cmdLatch2Off();
    virtual void          cmdAzPosn(const SEphem::Angle&);
    virtual void          cmdElPosn(const SEphem::Angle&);

    virtual int           reqAzKp();
    virtual int           reqAzKi();
    virtual int           reqAzKd();
    virtual int           reqAzIl();
    virtual int           reqAzAcc();
    virtual int           reqAzVel();
    virtual int           reqElKp();
    virtual int           reqElKi();
    virtual int           reqElKd();
    virtual int           reqElIl();
    virtual int           reqElAcc();
    virtual int           reqElVel();
    
    virtual void          setAzKp(int i);
    virtual void          setAzKi(int i);
    virtual void          setAzKd(int i);
    virtual void          setAzIl(int i);
    virtual void          setAzAcc(int i);
    virtual void          setAzVel(int i);
    virtual void          setElKp(int i);
    virtual void          setElKi(int i);
    virtual void          setElKd(int i);
    virtual void          setElIl(int i);
    virtual void          setElAcc(int i);
    virtual void          setElVel(int i);

    virtual std::string   apiName() const;
  }; // class EIA422Stub

  // --------------------------------------------------------------------------
  // ScopeAPIToEIA422Adaptor
  // --------------------------------------------------------------------------

  class ScopeAPIToEIA422Adaptor: public ScopeAPI
  {
  public:
    ScopeAPIToEIA422Adaptor(EIA422ScopeAPI* api)
      : ScopeAPI(), m_eia422_api(api), m_el_vel_max(), m_az_vel_max() { }
    
    virtual ~ScopeAPIToEIA422Adaptor();

    virtual void reqStat(ScopeAPI::PositionerStatus& state);

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

#if 0
    virtual void reqOffsets(SEphem::Angle& az_angle, 
			    SEphem::Angle& el_angle);
    virtual void setOffsets(const SEphem::Angle& az_angle,
			    const SEphem::Angle& el_angle);

    virtual void reqPIDParameters(PIDParameters& az_param,
				  PIDParameters& el_param);
    virtual void setPIDParameters(const PIDParameters& az_param,
				  const PIDParameters& el_param);
#endif

    virtual void resetCommunication();

    virtual std::string apiName() const;

  private:
    EIA422ScopeAPI* m_eia422_api;

    double m_el_vel_max;
    double m_az_vel_max;
  };

  // --------------------------------------------------------------------------
  // EIA422ToScopeAPIAdaptor
  // --------------------------------------------------------------------------

  class EIA422ToScopeAPIAdaptor: public EIA422Protocol
  {
  public:
    EIA422ToScopeAPIAdaptor(ScopeAPI* scope_api, DataStream* ds=0):
      EIA422Protocol(ds),  m_scope_api(scope_api), m_stored(false),
      m_stored_rate(), m_stored_angle() { }
    virtual ~EIA422ToScopeAPIAdaptor();
    
    virtual Status        reqAzStat();
    virtual Status        reqElStat();
    virtual SEphem::Angle reqAzPos();
    virtual SEphem::Angle reqElPos();
    virtual ExtStatus     reqAzExtStat();
    virtual ExtStatus     reqElExtStat();
    virtual void          cmdElSrvPwrOn();
    virtual void          cmdElSrvPwrOff();
    virtual void          cmdAzSrvPwrOn();
    virtual void          cmdAzSrvPwrOff();
    virtual SEphem::Angle reqAzOffset();
    virtual SEphem::Angle reqElOffset();
    virtual void          cmdAzRate(double);
    virtual void          cmdElRate(double);
    virtual void          setAzOffset(const SEphem::Angle&);
    virtual void          setElOffset(const SEphem::Angle&);
    virtual int           reqAzRevision();
    virtual int           reqElRevision();
    virtual void          cmdLatch1On();
    virtual void          cmdLatch1Off();
    virtual void          cmdLatch2On();
    virtual void          cmdLatch2Off();
    virtual void          cmdAzPosn(const SEphem::Angle&);
    virtual void          cmdElPosn(const SEphem::Angle&);

    virtual int           reqAzKp();
    virtual int           reqAzKi();
    virtual int           reqAzKd();
    virtual int           reqAzIl();
    virtual int           reqAzAcc();
    virtual int           reqAzVel();
    virtual int           reqElKp();
    virtual int           reqElKi();
    virtual int           reqElKd();
    virtual int           reqElIl();
    virtual int           reqElAcc();
    virtual int           reqElVel();
    
    virtual void          setAzKp(int i);
    virtual void          setAzKi(int i);
    virtual void          setAzKd(int i);
    virtual void          setAzIl(int i);
    virtual void          setAzAcc(int i);
    virtual void          setAzVel(int i);
    virtual void          setElKp(int i);
    virtual void          setElKi(int i);
    virtual void          setElKd(int i);
    virtual void          setElIl(int i);
    virtual void          setElAcc(int i);
    virtual void          setElVel(int i);

    virtual void          resetCommunication();

    virtual std::string   apiName() const;

  private:
    ScopeAPI* m_scope_api;

    bool          m_stored;
    double        m_stored_rate;
    SEphem::Angle m_stored_angle;
  }; // class EIA422ToScopeAPIAdaptor


} // namespace VTracking

#endif // VTRACKING_EIA422_H

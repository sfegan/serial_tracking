//-*-mode:c++; mode:font-lock;-*-

/**
 * \file PIUScopeAPI.h
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
 * $Date: 2006/07/17 14:25:03 $
 * $Revision: 2.2 $
 * $Tag$
 *
 **/

#ifndef VTRACKING_PIUSCOPEAPI_H
#define VTRACKING_PIUSCOPEAPI_H

#include<zthread/RecursiveMutex.h>

#include<Exception.h>
#include<Angle.h>

#include"ScopeAPI.h"
#include"ScopeProtocolServer.h"
#include"DataStream.h"

namespace VTracking
{
  class PIUScopeAPI: public ScopeAPI, public ScopeProtocolServer
  {
  public:
    enum ProtocolVersion { PV_PROTOTYPE, PV_ARRAY_050901 };

    PIUScopeAPI(ProtocolVersion protocol_version, DataStream* ds, 
		unsigned com_tries=3, ScopeAPI* server = 0);
    
    virtual ~PIUScopeAPI();

    // ScopeAPI members
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

#if 0
    // Remainder of the PIU protocol - UNIMPLEMENTED
    void reqLimits(SEphem::Angle& az_center, SEphem::Angle& az_travel, 
		   SEphem::Angle& el_center, SEphem::Angle& el_travel);
    
    void cmdAzSector(const SEphem::Angle& az_center,
		     const SEphem::Angle& az_travel, double az_vel);
    
    void cmdElSector(const SEphem::Angle& el_center,
		     const SEphem::Angle& el_travel, double el_vel);
    
    void cmdRaster(const SEphem::Angle& az_center,
		   const SEphem::Angle& el_center,
		   const SEphem::Angle& az_travel,
		   const SEphem::Angle& el_travel,
		   const SEphem::Angle& increment, double vel);
    
    void setLimits(const SEphem::Angle& az_center, 
		   const SEphem::Angle& az_travel, 
		   const SEphem::Angle& el_center, 
		   const SEphem::Angle& el_travel);
#endif

    // ScopeProtocolServer members
    virtual void processOneCommand(long cmd_to_sec=0, long cmd_to_usec=0);

  private:
    struct PIUCmd
    {
      PIUCmd(char cc, unsigned cl, int ccomp, char rc, unsigned rl)
	: cmd_char(cc), cmd_len(cl), cmd_complexity(ccomp),
	  resp_char(rc), resp_len(rl) { /* nothing to see here */ }
      char        cmd_char;
      unsigned    cmd_len;
      int         cmd_complexity;
      char        resp_char;
      unsigned    resp_len;
    };

    enum ResponseStatus { RS_OK, RS_NAK, RS_NOT_UNDERSTOOD };

    void init();
    void recoverFromZeroSpeedCondition();

    void makeAndSendCommand(const datastring& cmd_data, const PIUCmd& cmd);
    ResponseStatus getAndCheckResponse(datastring& resp, const PIUCmd& cmd);
    ResponseStatus command(const datastring& cmd_data, 
			   datastring& resp, const PIUCmd& cmd);
    
    void getCommandData(datastring& data, const PIUCmd& cmd,
			long cmd_to_sec, long cmd_to_usec);
    void makeAndSendResponse(const datastring& cmd_data, const PIUCmd& cmd);

    bool                    m_init;

    ProtocolVersion         m_protocol_version;
    DataStream*             m_data_stream;
    unsigned                m_com_tries;
    ZThread::RecursiveMutex m_mtx;

    double                  m_max_az_vel;
    double                  m_max_el_vel;

    double                  m_protocol_az_vel_scale;
    double                  m_protocol_el_vel_scale;
    int                     m_protocol_az_vel_limit;
    int                     m_protocol_el_vel_limit;

    double                  m_protocol_az_acc_scale;
    double                  m_protocol_el_acc_scale;
    int                     m_protocol_az_acc_limit;
    int                     m_protocol_el_acc_limit;

    double                  m_adc_volts_per_dc;

    PIUCmd                  m_req_stat;
    PIUCmd                  m_req_limits;
    PIUCmd                  m_req_az_off;
    PIUCmd                  m_req_el_off;
    PIUCmd                  m_req_az_pid;
    PIUCmd                  m_req_el_pid;
    PIUCmd                  m_cmd_clear_bits;
    PIUCmd                  m_cmd_point;
    PIUCmd                  m_cmd_slew;
    PIUCmd                  m_cmd_az_sec;
    PIUCmd                  m_cmd_el_sec;
    PIUCmd                  m_cmd_raster;
    PIUCmd                  m_cmd_standby;
    PIUCmd                  m_set_az_off;
    PIUCmd                  m_set_el_off;
    PIUCmd                  m_set_limits;
    PIUCmd                  m_set_rf_chan;
    PIUCmd                  m_set_feed_pol;
    PIUCmd                  m_set_az_pid;
    PIUCmd                  m_set_el_pid;

    ScopeAPI*               m_server;

    static const char CHAR_SOM;
    static const char CHAR_EOM;
  }; // class PIUScopeAPI
  
}; // namespace VTracking

#endif // VTRACKING_PIUSCOPEAPI_H

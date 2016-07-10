//-*-mode:c++; mode:font-lock;-*-

/**
 * \file PIUScopeAPI.cpp
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
 * $Date: 2008/01/08 22:08:18 $
 * $Revision: 2.5 $
 * $Tag$
 *
 **/

#include<fstream>
#include<iostream>
#include<iomanip>
#include<sstream>

#include<zthread/Thread.h>
#include<zthread/Guard.h>

#include<Angle.h>
#include<Debug.h>
#include<Message.h>
#include<Messenger.h>
#include<VATime.h>

#include"PIUScopeAPI.h"

using namespace SEphem;
using namespace VTracking;
using namespace VMessaging;
using namespace VERITAS;

//#define USE_CWCCW_WORKAROUND
//#define USE_AZPULLCORD_WORKAROUND
#define USE_ZEROSPEED_WORKAROUND
#define USE_FIXED_AZMAXSPEED 0.4
#define USE_FIXED_ELMAXSPEED 0.4

const char PIUScopeAPI::CHAR_SOM = '[';  // SOM for telescope "1"
const char PIUScopeAPI::CHAR_EOM = '\r'; // End of message character

// ANL -- ACK/NAK response length
// 0 => Do not expect an ACK/NAK response 
// 5 => Expect ACK/NAK response, ie length of "SOM A C K EOM" / "SOM N A K EOM"
static const unsigned ANL=0 /* 5 */;

PIUScopeAPI::PIUScopeAPI(ProtocolVersion protocol_version, 
			 DataStream* ds, unsigned com_tries,
			 ScopeAPI* server): 
  m_init(false), m_protocol_version(protocol_version),
  m_data_stream(ds), m_com_tries(com_tries), m_mtx(), 
  m_max_az_vel(), m_max_el_vel(),
  m_protocol_az_vel_scale(), m_protocol_el_vel_scale(),
  m_protocol_az_vel_limit(), m_protocol_el_vel_limit(),
  m_protocol_az_acc_scale(), m_protocol_el_acc_scale(), 
  m_protocol_az_acc_limit(), m_protocol_el_acc_limit(), 
  m_adc_volts_per_dc(),
  
  m_req_stat(       '?',  3,  2,  '?',  33  ),
  m_req_limits(     '=',  3,  2,  '#',  27  ),
  m_req_az_off(     '<',  3,  1,  '!',  9   ),
#ifdef RPM_COULD_PRODUCE_CODE_ON_SPEC 
  m_req_el_off(     '>',  3,  1,  '"',  9   ),
#else
  m_req_el_off(     '>',  3,  1,  '!',  9   ),
#endif
  m_req_az_pid(     '(',  3,  7,  0x1E, 27  ),
  m_req_el_pid(     ')',  3,  7,  0x1F, 27  ),
  m_cmd_clear_bits( '@',  3,  1,  0x00, ANL ),
  m_cmd_point(      0x16, 19, 1,  0x00, ANL ),
  m_cmd_slew(       0x18, 7,  1,  0x00, ANL ),
  m_cmd_az_sec(     0x1A, 17, 1,  0x00, ANL ),
  m_cmd_el_sec(     0x1B, 17, 1,  0x00, ANL ),
  m_cmd_raster(     0x1C, 35, 1,  0x00, ANL ),
  m_cmd_standby(    0x1D, 3,  1,  0x00, ANL ),
  m_set_az_off(     '#',  9,  1,  0x00, ANL ),
  m_set_el_off(     '$',  9,  1,  0x00, ANL ),
  m_set_limits(     '%',  25, 1,  0x00, ANL ),
  m_set_rf_chan(    '&',  4,  1,  0x00, ANL ),
  m_set_feed_pol(   '\'', 4,  1,  0x00, ANL ),
  m_set_az_pid(     0x1E, 27, 12, 0x00, ANL ),
  m_set_el_pid(     0x1F, 27, 12, 0x00, ANL ),

  m_server(server)
{ 
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  switch(m_protocol_version)
    {
    case PV_PROTOTYPE:
      m_protocol_az_vel_scale = 0.5/250;
      m_protocol_el_vel_scale = 0.5/250;
      m_protocol_az_vel_limit = 250;
      m_protocol_el_vel_limit = 250;
      m_protocol_az_acc_scale = 1.0/25;
      m_protocol_el_acc_scale = 1.0/25;
      m_protocol_az_acc_limit = 250;
      m_protocol_el_acc_limit = 250;       
      m_adc_volts_per_dc = 0.00488;
      break;

    case PV_ARRAY_050901:
      m_protocol_az_vel_scale = 1.0/250;
      m_protocol_el_vel_scale = 1.0/250;
      m_protocol_az_vel_limit = 250;
      m_protocol_el_vel_limit = 250;
      m_protocol_az_acc_scale = 1.0/25;
      m_protocol_el_acc_scale = 1.0/25;
      m_protocol_az_acc_limit = 250;
      m_protocol_el_acc_limit = 250;       
      m_adc_volts_per_dc = 0.00488;

      m_cmd_point.cmd_complexity = 0;
      m_cmd_slew.cmd_complexity = 0;
      m_cmd_az_sec.cmd_complexity = 0;
      m_cmd_el_sec.cmd_complexity = 0;
      m_cmd_raster.cmd_complexity = 0;
      m_cmd_standby.cmd_complexity = 0;
      break;
    }

  if(server != 0)return;

#if defined(USE_FIXED_AZMAXSPEED)
  if(m_protocol_version == PV_PROTOTYPE)
    {
      Message message(Message::DR_LOCAL,Message::PS_ROUTINE,
		      "Using AZ maxspeed workaround");
      message.messageStream() 
	<< "The mount does not properly report the maximum Azimuth "
	<< "speed, using " << USE_FIXED_AZMAXSPEED << " deg/s";
      Messenger::relay()->sendMessage(message);
    }
#endif

#if defined(USE_FIXED_ELMAXSPEED)
  if(m_protocol_version == PV_PROTOTYPE)
    {
      Message message(Message::DR_LOCAL,Message::PS_ROUTINE,
		      "Using EL maxspeed workaround");
      message.messageStream() 
	<< "The mount does not properly report the maximum Elevation "
	<< "speed, using " << USE_FIXED_ELMAXSPEED << " deg/s";
      Messenger::relay()->sendMessage(message);
    }
#endif 

#if defined(USE_AZPULLCORD_WORKAROUND)
  if(m_protocol_version == PV_PROTOTYPE)
    {
      Message message(Message::DR_LOCAL,Message::PS_ROUTINE,
		      "Using CABLEWRAP workaround");
      message.messageStream() 
	<< "The mount does not properly report the cable wrap, "
	<< "its value will be calculated"
      Messenger::relay()->sendMessage(message);
    }
#endif 

  init(); 
}

PIUScopeAPI::~PIUScopeAPI()
{
  // nothing to see here
}

void PIUScopeAPI::reqStat(PositionerStatus& state)
{
  const std::string cf("PIUScopeAPI::reqStat");
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

#ifdef USE_CWCCW_WORKAROUND
  static bool azcwccw_workaround_init = false;
  static bool azcwccw_workaround_cw = false;

  if(azcwccw_workaround_init == false)
    {
      azcwccw_workaround_init=true;
      std::ifstream cw_file(".azcwccw_workaround_cw");
      if(cw_file)cw_file >> azcwccw_workaround_cw;

      Message message(Message::DR_LOCAL,Message::PS_ROUTINE,
		      "Using Az CW/CCW workaround");
      message.messageStream() 
	<< "The mount does not supply its direction (CW/CCW). The "
	<< "tracking program will attempt to keep track of the direction. "
	<< "The last saved direction was: "
	<< (azcwccw_workaround_cw?"CW.":"CCW.");
      Messenger::relay()->sendMessage(message);
    }
#endif // USE_AZCWCCW_WORKAROUND

  const PIUCmd& cmd(m_req_stat);

  datastring resp_str; 
  ResponseStatus rs = command("",resp_str,cmd);

  if(rs!=RS_OK)
    throw(ScopeAPIError("PIU protocol error",
			std::string("PIU response not recognised"),
			std::string("Response: "+resp_str)));
  
  int az_bar;
  int el_bar;
  int analog1;
  int analog2;
  int rf_mode;
  int relay_interlock;
  int limits_interlock;
  int errors_servo;
  int cable_wrap;

  int n;
  n=sscanf(resp_str.c_str(),"%6X%6X%4X%4X%2X%2X%2X%2X%2X", &az_bar, &el_bar, 
	   &analog1 , &analog2, &rf_mode, &relay_interlock, &limits_interlock, 
	   &errors_servo, &cable_wrap);
  if(n!=9)
    throw(ScopeAPIError("PIU protocol error",
			std::string("Could not sscanf response"),
			std::string("Response: ")+resp_str));

  state.az.driveangle_deg     = Angle::toDeg(Angle::frBAR(az_bar, 24));
  state.el.driveangle_deg     = Angle::toDeg(Angle::frBAR(el_bar, 24));

  switch(rf_mode&0x0F)
    {
    case 0:
      state.az.driveMode      = state.el.driveMode = DM_STANDBY;
      break;
    case 1:
      state.az.driveMode      = state.el.driveMode = DM_SLEW;
      break;
    case 2:
      state.az.driveMode      = state.el.driveMode = DM_POINT;
      break;
    case 3:
      state.az.driveMode      = state.el.driveMode = DM_SPIN;
      break;
    case 4:
      state.az.driveMode      = state.el.driveMode = DM_SECTOR_SCAN;
      break;
    case 5:
      state.az.driveMode      = state.el.driveMode = DM_SECTOR_SCAN;
      break;
    case 6:
      state.az.driveMode      = state.el.driveMode = DM_RASTER;
      break;
    case 15:
      state.az.driveMode      = state.el.driveMode = DM_CHANGING;
      break;
    default:
      state.az.driveMode      = state.el.driveMode = DM_UNKNOWN;
      break;
    }

  state.az.servo1Fail         = errors_servo&0x01;
  state.az.servo2Fail         = errors_servo&0x02;
  state.el.servo1Fail         = errors_servo&0x40;
  state.el.servo2Fail         = errors_servo&0x80;

  state.az.servoOn            = state.az.driveMode!=DM_STANDBY;
  state.el.servoOn            = state.el.driveMode!=DM_STANDBY;

  state.az.brakeReleased      = state.az.driveMode!=DM_STANDBY;
  state.el.brakeReleased      = state.el.driveMode!=DM_STANDBY;

  state.az.limitCwUp          = limits_interlock&0x01;
  state.az.limitCcwDown       = limits_interlock&0x02;
  state.el.limitCwUp          = limits_interlock&0x04;
  state.el.limitCcwDown       = limits_interlock&0x08;

  state.az.positionFault      = false;
  state.az.positionComplete   = false;
  state.el.positionFault      = false;
  state.el.positionComplete   = false;

  state.azTravelledCCW        = relay_interlock&0x40;
  state.azCableWrap           = 
    double((cable_wrap&0x80)?(cable_wrap-256):(cable_wrap))/100.0;

  state.interlock             = limits_interlock&0x10;
  state.interlockAzPullCord   = relay_interlock&0x08;
  state.interlockAzStowPin    = limits_interlock&0x40;
  state.interlockElStowPin    = limits_interlock&0x80;
  state.interlockAzDoorOpen   = relay_interlock&0x01;
  state.interlockElDoorOpen   = relay_interlock&0x02;
  state.interlockSafeSwitch   = limits_interlock&0x20;

  state.checksumOK            = true;
  state.remoteControl         = !(relay_interlock&0x80);
  
  state.msgBadFrame           = errors_servo & 0x10;
  state.msgCommandInvalid     = errors_servo & 0x20;
  state.msgInputOverrun       = errors_servo & 0x08;
  state.msgOutputOverrun      = errors_servo & 0x04;

  state.relay1                = relay_interlock & 0x10;
  state.relay2                = relay_interlock & 0x20;
  
  state.Analog1               = 
    double((analog1&0x0800)?(analog1-0x1000):(analog1))*m_adc_volts_per_dc;
  state.Analog2               =
    double((analog2&0x0800)?(analog2-0x1000):(analog2))*m_adc_volts_per_dc;

#ifdef USE_AZCWCCW_WORKAROUND
  if(state.az.driveangle_deg<88)azcwccw_workaround_cw=true;
  else if(state.az.driveangle_deg>272)azcwccw_workaround_cw=false;

  if(true)
    {
      std::ofstream cw_file(".azcwccw_workaround_cw");
      if(cw_file)cw_file << azcwccw_workaround_cw << std::endl;
    }

  if(azcwccw_workaround_cw)state.azTravelledCCW = false;
  else state.azTravelledCCW = true;
#endif // USE_AZCWCCW_WORKAROUND

  if(((state.azTravelledCCW)&&(state.az.driveangle_deg>45))||
     (state.az.driveangle_deg>315))
    state.az.driveangle_deg   = state.az.driveangle_deg-360;

#ifdef USE_AZPULLCORD_WORKAROUND
  if(m_protocol_version == PV_PROTOTYPE)
    state.azCableWrap = state.az.driveangle_deg/270.0;
#endif // USE_AZPULLCORD_WORKAROUND

  if(state.el.driveangle_deg>180)state.el.driveangle_deg -= 360;


  if(m_data_stream->loud()>=1)
    Debug::stream()
      << VATime::now().getMSTimeStamp() << ' ' << cf << "() = " 
      << state.az.driveangle_deg << ' ' 
      << state.az.servo1Fail << ' '
      << state.az.servo2Fail << ' '
      << state.az.limitCwUp << ' '
      << state.az.limitCcwDown << ' '
      << state.azTravelledCCW << ' '
      << state.azCableWrap << ' '
      << state.el.driveangle_deg << ' '
      << state.el.servo1Fail << ' '
      << state.el.servo2Fail << ' '
      << state.el.limitCwUp << ' '
      << state.el.limitCcwDown << ' '
      << state.interlock << ' ' 
      << state.interlockAzPullCord << ' '
      << state.interlockAzStowPin << ' '
      << state.interlockElStowPin << ' '
      << state.interlockAzDoorOpen << ' '
      << state.interlockElDoorOpen << ' '
      << state.interlockSafeSwitch << ' '
      << state.msgBadFrame << ' '
      << state.msgCommandInvalid << ' '
      << state.msgInputOverrun << ' '
      << state.msgOutputOverrun << ' '
      << state.Analog1 << ' '
      << state.Analog2 << std::endl;
}

void PIUScopeAPI::cmdStandby()
{
  const std::string cf("PIUScopeAPI::cmdStandby");
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  const PIUCmd& cmd(m_cmd_standby);

  datastring resp_str; 
  ResponseStatus rs = command("",resp_str,cmd);

  if(rs==RS_NAK)
    throw(ScopeAPIError("PIU protocol error",
			std::string("PIU response was NAK")));
  else if(rs!=RS_OK)
    throw(ScopeAPIError("PIU protocol error",
			std::string("PIU response not recognised"),
			std::string("Response: "+resp_str)));

  if(m_data_stream->loud()>=1)
    Debug::stream() << VATime::now().getMSTimeStamp() << ' ' << cf << "()" 
		    << std::endl;
}

void PIUScopeAPI::cmdPoint(const SEphem::Angle& az_angle, double az_vel,
			   const SEphem::Angle& el_angle, double el_vel)
{
  const std::string cf("PIUScopeAPI::cmdPoint");
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  init();

  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

#ifdef USE_ZEROSPEED_WORKAROUND
  if((m_max_az_vel == 0)||(m_max_el_vel == 0))recoverFromZeroSpeedCondition();
#endif

  if(az_vel>m_max_az_vel)az_vel=m_max_az_vel;
  else if(az_vel<0)az_vel=0;
  if(el_vel>m_max_el_vel)el_vel=m_max_el_vel;
  else if(el_vel<0)el_vel=0;  

  int azbar = az_angle.bar(24);
  int elbar = el_angle.bar(24);
  int azvel = int(round(az_vel/m_max_az_vel*100));
  int elvel = int(round(el_vel/m_max_el_vel*100));

  if((azvel<0)||(azvel>100)||(elvel<0)||(elvel>100))
    {
      std::ostringstream msg;
      msg << "AzSpeed=" << azvel << "% " 
	  << "ElSpeed=" << elvel << '%' << std::endl;
      throw(ScopeAPIError("PIU protocol error",
			  "Azimuth or elevation speed out of range",
			  msg.str()));
    }

  const PIUCmd& cmd(m_cmd_point);
  
  std::ostringstream cmd_stream;
  
  cmd_stream << std::hex << std::uppercase << std::setfill('0')
	     << std::setw(6) << std::setprecision(6) << azbar 
	     << std::setw(6) << std::setprecision(6) << elbar 
	     << std::setw(2) << std::setprecision(2) << azvel
	     << std::setw(2) << std::setprecision(2) << elvel;
					
  datastring resp_str; 
  ResponseStatus rs = command(cmd_stream.str(),resp_str,cmd);

  if(rs==RS_NAK)
    throw(ScopeAPIError("PIU protocol error",
			std::string("PIU response was NAK")));
  else if(rs!=RS_OK)
    throw(ScopeAPIError("PIU protocol error",
			std::string("PIU response not recognised"),
			std::string("Response: "+resp_str)));
  
  if(m_data_stream->loud()>=1)
    Debug::stream() << VATime::now().getMSTimeStamp() << ' ' << cf << '(' 
		    << az_angle.deg() << ',' << az_vel << ',' 
		    << el_angle.degPM() << ',' << el_vel << ')' << std::endl;
}

void PIUScopeAPI::cmdSlew(double az_vel, double el_vel)
{
  const std::string cf("PIUScopeAPI::cmdSlew");
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  init();

  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  if(az_vel>m_max_az_vel)az_vel=m_max_az_vel;
  else if(az_vel<-m_max_az_vel)az_vel=-m_max_az_vel;
  if(el_vel>m_max_el_vel)el_vel=m_max_el_vel;
  else if(el_vel<-m_max_el_vel)el_vel=-m_max_el_vel;

#ifdef USE_ZEROSPEED_WORKAROUND
  if((m_max_az_vel == 0)||(m_max_el_vel == 0))recoverFromZeroSpeedCondition();
#endif

  int azvel = int(round(az_vel/m_max_az_vel*100));
  int elvel = int(round(el_vel/m_max_el_vel*100));

  if((azvel<-100)||(azvel>100)||(elvel<-100)||(elvel>100))
    {
      std::ostringstream msg;
      msg << "AzSpeed=" << azvel << "% " 
	  << "ElSpeed=" << elvel << '%' << std::endl;
      throw(ScopeAPIError("PIU protocol error",
			  "Azimuth or elevation speed out of range",msg.str()));
    }

  if(azvel<0)azvel=256+azvel;
  if(elvel<0)elvel=256+elvel;

  const PIUCmd& cmd(m_cmd_slew);

  std::ostringstream cmd_stream;
  cmd_stream << std::hex << std::uppercase << std::setfill('0')
	     << std::setw(2) << std::setprecision(2) << azvel 
	     << std::setw(2) << std::setprecision(2) << elvel;

  datastring resp_str; 
  ResponseStatus rs = command(cmd_stream.str(),resp_str,cmd);

  if(rs==RS_NAK)
    throw(ScopeAPIError("PIU protocol error",
			std::string("PIU response was NAK")));
  else if(rs!=RS_OK)
    throw(ScopeAPIError("PIU protocol error",
			std::string("PIU response not recognised"),
			std::string("Response: "+resp_str)));

  if(m_data_stream->loud()>=1)
    Debug::stream() << VATime::now().getMSTimeStamp() << ' ' << cf << '(' 
		    << az_vel << ',' << el_vel << ')'
		    << std::endl;
}

void PIUScopeAPI::reqAzOffset(SEphem::Angle& az_angle)
{
  std::string cf("PIUScopeAPI::reqAzOffset");
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  const PIUCmd& azcmd(m_req_az_off);
  int az_bar;

  datastring cmd_str;
  datastring resp_str;
  ResponseStatus rs;

  // Azimuth offset
  rs = command("",resp_str,azcmd);

  if(rs!=RS_OK)
    throw(ScopeAPIError("PIU protocol error",
		    std::string("PIU response not recognised"),
		    std::string("Response: "+resp_str)));

  int n=sscanf(resp_str.c_str(),"%6X",&az_bar);
  if(n!=1)
    throw(ScopeAPIError("PIU protocol error",
		    std::string("Could not sscanf Az response"),
		    std::string("Response: "+resp_str)));

  az_angle = Angle::makeBAR(az_bar, 24);

  if(m_data_stream->loud()>=1)
    Debug::stream() << VATime::now().getMSTimeStamp() << ' ' << cf << "() = "
		    << az_angle.deg() << std::endl;
}

void PIUScopeAPI::reqElOffset(SEphem::Angle& el_angle)
{
  std::string cf("PIUScopeAPI::reqElOffset");
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  const PIUCmd& elcmd(m_req_el_off);

  int el_bar;

  datastring cmd_str;
  datastring resp_str;
  ResponseStatus rs;

  // Elevation offset
  rs = command("",resp_str,elcmd);

  if(rs!=RS_OK)
    throw(ScopeAPIError("PIU protocol error",
		    std::string("PIU response not recognised"),
		    std::string("Response: "+resp_str)));

  int n=sscanf(resp_str.c_str(),"%6X",&el_bar);
  if(n!=1)
    throw(ScopeAPIError("PIU protocol error",
		    std::string("Could not sscanf El response"),
		    std::string("Response: "+resp_str)));
  
  el_angle = Angle::makeBAR(el_bar, 24);

  if(m_data_stream->loud()>=1)
    Debug::stream() << VATime::now().getMSTimeStamp() << ' ' << cf << "() = " 
		    << el_angle.degPM() << std::endl;
}

void PIUScopeAPI::setAzOffset(const SEphem::Angle& az_angle)
{
  const std::string cf("PIUScopeAPI::setAzOffset");
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  
  int azbar = az_angle.bar(24);
  
  const PIUCmd& azcmd(m_set_az_off);
  
  std::ostringstream cmd_stream;
  datastring cmd_str;
  datastring resp_str;  
  ResponseStatus rs;

  // Azimuth
  cmd_stream.str("");
  cmd_stream << std::hex << std::uppercase << std::setfill('0')
	     << std::setw(6) << std::setprecision(6) << azbar;

  rs = command(cmd_stream.str(),resp_str,azcmd);

  if(rs==RS_NAK)
    throw(ScopeAPIError("PIU protocol error",
		    std::string("PIU response was NAK")));
  else if(rs!=RS_OK)
    throw(ScopeAPIError("PIU protocol error",
		    std::string("PIU response not recognised"),
		    std::string("Response: "+resp_str)));

  if(m_data_stream->loud()>=1)
    Debug::stream() << VATime::now().getMSTimeStamp() << ' ' << cf << '(' 
		    << az_angle.deg() << ')' << std::endl;
}


void PIUScopeAPI::setElOffset(const SEphem::Angle& el_angle)
{
  const std::string cf("PIUScopeAPI::setElOffset");
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  
  int elbar = el_angle.bar(24);
  
  const PIUCmd& elcmd(m_set_el_off);
  
  std::ostringstream cmd_stream;
  datastring cmd_str;
  datastring resp_str;  
  ResponseStatus rs;

  // Elevation
  cmd_stream.str("");
  cmd_stream << std::hex << std::uppercase << std::setfill('0')
	     << std::setw(6) << std::setprecision(6) << elbar;

  rs = command(cmd_stream.str(),resp_str,elcmd);

  if(rs==RS_NAK)
    throw(ScopeAPIError("PIU protocol error",
		    std::string("PIU response was NAK")));
  else if(rs!=RS_OK)
    throw(ScopeAPIError("PIU protocol error",
		    std::string("PIU response not recognised"),
		    std::string("Response: "+resp_str)));

  if(m_data_stream->loud()>=1)
    Debug::stream() << VATime::now().getMSTimeStamp() << ' ' << cf << '(' 
		    << el_angle.degPM() << ')' << std::endl;
}

void PIUScopeAPI::reqAzPIDParameters(PIDParameters& az_param)
{
  const std::string cf("PIUScopeAPI::reqAzPIDParameters");
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  int vlim;
  int alim;

  const PIUCmd& azcmd(m_req_az_pid);

  datastring cmd_str;
  datastring resp_str; 
  ResponseStatus rs;

  // Azimuth
  rs = command("",resp_str,azcmd);
  if(rs!=RS_OK)
    throw(ScopeAPIError("PIU protocol error",
		    std::string("PIU response not recognised"),
		    std::string("Response: "+resp_str)));

  int n=sscanf(resp_str.c_str(),"%4X%4X%4X%4X%4X%2X%2X", &az_param.Kp,
	       &az_param.Ki, &az_param.Kd, &az_param.Kvff, &az_param.Ilim,
	       &vlim, &alim);
  if(n!=7)
    throw(ScopeAPIError("PIU protocol error",
		    std::string("Could not sscanf Az response"),
		    std::string("Response: "+resp_str)));
  
  az_param.vlim = double(vlim)*m_protocol_az_vel_scale;
  az_param.alim = double(alim)*m_protocol_az_acc_scale;

#ifdef USE_FIXED_AZMAXSPEED
  if(m_protocol_version == PV_PROTOTYPE)
    m_max_az_vel = USE_FIXED_AZMAXSPEED;
  else
    m_max_az_vel = az_param.vlim;    
#else
  m_max_az_vel = az_param.vlim;
#endif

  if(m_data_stream->loud()>=1)
    Debug::stream() << VATime::now().getMSTimeStamp() << ' ' << cf << "() = " 
		    << az_param.Kp << ' ' << az_param.Ki << ' ' 
		    << az_param.Kd << ' ' << az_param.Kvff << ' ' 
		    << az_param.Ilim << ' ' << az_param.vlim << ' ' 
		    << az_param.alim << std::endl;
}

void PIUScopeAPI::reqElPIDParameters(PIDParameters& el_param)
{
  const std::string cf("PIUScopeAPI::reqElPIDParameters");
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  int vlim;
  int alim;

  const PIUCmd& elcmd(m_req_el_pid);

  datastring cmd_str;
  datastring resp_str; 
  ResponseStatus rs;

  // Elevation
  rs = command("",resp_str,elcmd);
  if(rs!=RS_OK)
    throw(ScopeAPIError("PIU protocol error",
		    std::string("PIU response not recognised"),
		    std::string("Response: "+resp_str)));

  int n=sscanf(resp_str.c_str(),"%4X%4X%4X%4X%4X%2X%2X", &el_param.Kp,
	       &el_param.Ki, &el_param.Kd, &el_param.Kvff, &el_param.Ilim,
	       &vlim, &alim);
  if(n!=7)
    throw(ScopeAPIError("PIU protocol error",
		    std::string("Could not sscanf El response"),
		    std::string("Response: "+resp_str)));
  
  el_param.vlim = double(vlim)*m_protocol_el_vel_scale;
  el_param.alim = double(alim)*m_protocol_el_acc_scale;

#ifdef USE_FIXED_ELMAXSPEED
  if(m_protocol_version == PV_PROTOTYPE)
    m_max_el_vel = USE_FIXED_ELMAXSPEED;
  else
    m_max_el_vel = el_param.vlim;
#else
  m_max_el_vel = el_param.vlim;
#endif

  if(m_data_stream->loud()>=1)
    Debug::stream() << VATime::now().getMSTimeStamp() << ' ' << cf << "() = " 
		    << el_param.Kp << ' ' << el_param.Ki << ' ' 
		    << el_param.Kd << ' ' << el_param.Kvff << ' ' 
		    << el_param.Ilim << ' ' << el_param.vlim << ' ' 
		    << el_param.alim << std::endl;
}

void PIUScopeAPI::setAzPIDParameters(const PIDParameters& az_param)
{
  const std::string cf("PIUScopeAPI::setAzPIDParameters");
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  const PIUCmd& azcmd(m_set_az_pid);
  
  int vlim;
  int alim;

  std::ostringstream cmd_stream;
  datastring cmd_str;
  datastring resp_str;  
  ResponseStatus rs;

  // Azimuth
  vlim = int(round(az_param.vlim/m_protocol_az_vel_scale)); 
  if(vlim>m_protocol_az_vel_limit)vlim=m_protocol_az_vel_limit;
  else if(vlim<0)vlim=0;
  
  alim = int(round(az_param.alim/m_protocol_az_acc_scale));
  if(alim>m_protocol_az_acc_limit)alim=m_protocol_az_acc_limit; 
  else if(alim<0)alim=0;

  m_max_az_vel = double(vlim)/m_protocol_az_vel_scale;

  cmd_stream.str("");
  cmd_stream << std::hex << std::uppercase << std::setfill('0')
	     << std::setw(4) << std::setprecision(4) << az_param.Kp
	     << std::setw(4) << std::setprecision(4) << az_param.Ki
	     << std::setw(4) << std::setprecision(4) << az_param.Kd
	     << std::setw(4) << std::setprecision(4) << az_param.Kvff
	     << std::setw(4) << std::setprecision(4) << az_param.Ilim
	     << std::setw(2) << std::setprecision(2) << vlim
	     << std::setw(2) << std::setprecision(2) << alim;
					
  rs = command(cmd_stream.str(),resp_str,azcmd);

  if(rs==RS_NAK)
    throw(ScopeAPIError("PIU protocol error",
		    std::string("PIU response was NAK")));
  else if(rs!=RS_OK)
    throw(ScopeAPIError("PIU protocol error",
		    std::string("PIU response not recognised"),
		    std::string("Response: "+resp_str)));

  if(m_data_stream->loud()>=1)
    Debug::stream() << VATime::now().getMSTimeStamp() << ' ' << cf << '(' 
		    << az_param.Kp << ',' << az_param.Ki << ',' 
		    << az_param.Kd << ',' << az_param.Kvff << ',' 
		    << az_param.Ilim << ',' << az_param.vlim << ',' 
		    << az_param.alim << std::endl;
}

void PIUScopeAPI::setElPIDParameters(const PIDParameters& el_param)
{
  const std::string cf("PIUScopeAPI::setElPIDParameters");
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  const PIUCmd& elcmd(m_set_el_pid);
  
  int vlim;
  int alim;

  std::ostringstream cmd_stream;
  datastring cmd_str;
  datastring resp_str;  
  ResponseStatus rs;

  // Elevation
  vlim = int(round(el_param.vlim/m_protocol_el_vel_scale));
  if(vlim>m_protocol_el_vel_limit)vlim=m_protocol_el_vel_limit; 
  else if(vlim<0)vlim=0;

  alim = int(round(el_param.alim/m_protocol_el_acc_scale));
  if(alim>m_protocol_el_acc_limit)alim=m_protocol_el_acc_limit; 
  else if(alim<0)alim=0;

  m_max_el_vel = double(vlim)*m_protocol_el_vel_scale;

  cmd_stream.str("");
  cmd_stream << std::hex << std::uppercase << std::setfill('0')
	     << std::setw(4) << std::setprecision(4) << el_param.Kp
	     << std::setw(4) << std::setprecision(4) << el_param.Ki
	     << std::setw(4) << std::setprecision(4) << el_param.Kd
	     << std::setw(4) << std::setprecision(4) << el_param.Kvff
	     << std::setw(4) << std::setprecision(4) << el_param.Ilim
	     << std::setw(2) << std::setprecision(2) << vlim
	     << std::setw(2) << std::setprecision(2) << alim;
					
  rs = command(cmd_stream.str(),resp_str,elcmd);

  if(rs==RS_NAK)
    throw(ScopeAPIError("PIU protocol error",
		    std::string("PIU response was NAK")));
  else if(rs!=RS_OK)
    throw(ScopeAPIError("PIU protocol error",
		    std::string("PIU response not recognised"),
		    std::string("Response: "+resp_str)));

  if(m_data_stream->loud()>=1)
    Debug::stream() << VATime::now().getMSTimeStamp() << ' ' << cf << '(' 
		    << el_param.Kp << ',' << el_param.Ki << ',' 
		    << el_param.Kd << ',' << el_param.Kvff << ',' 
		    << el_param.Ilim << ',' << el_param.vlim << ',' 
		    << el_param.alim << ')' <<std::endl;
}

#if 0
void PIUScopeAPI::reqOffsets(SEphem::Angle& az_angle,
			     SEphem::Angle& el_angle)
{
  std::string cf("PIUScopeAPI::reqOffsets");
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  const PIUCmd& azcmd(m_req_az_off);
  const PIUCmd& elcmd(m_req_el_off);

  int az_bar;
  int el_bar;

  datastring cmd_str;
  datastring resp_str;
  ResponseStatus rs;

  // Azimuth offset
  rs = command("",resp_str,azcmd);

  if(rs!=RS_OK)
    throw(ScopeAPIError("PIU protocol error",
		    std::string("PIU response not recognised"),
		    std::string("Response: "+resp_str)));

  int n;
  n=sscanf(resp_str.c_str(),"%6X",&az_bar);
  if(n!=1)
    throw(ScopeAPIError("PIU protocol error",
		    std::string("Could not sscanf Az response"),
		    std::string("Response: "+resp_str)));

  // Elevation offset
  rs = command("",resp_str,elcmd);

  if(rs!=RS_OK)
    throw(ScopeAPIError("PIU protocol error",
		    std::string("PIU response not recognised"),
		    std::string("Response: "+resp_str)));

  n=sscanf(resp_str.c_str(),"%6X",&el_bar);
  if(n!=1)
    throw(ScopeAPIError("PIU protocol error",
		    std::string("Could not sscanf El response")
		    std::string("Response: "+resp_str)));
  
  az_angle = Angle::makeBAR(az_bar, 24);
  el_angle = Angle::makeBAR(el_bar, 24);

  if(m_data_stream->loud()>=1)
    Debug::stream() << VATime::now().getMSTimeStamp() << ' ' << cf << "() = "
		    << az_angle.deg() << ' ' << el_angle.degPM() 
		    << std::endl;
}

void PIUScopeAPI::setOffsets(const SEphem::Angle& az_angle,
			     const SEphem::Angle& el_angle)
{
  const std::string cf("PIUScopeAPI::setOffsets");
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  
  int azbar = az_angle.bar(24);
  int elbar = el_angle.bar(24);
  
  const PIUCmd& azcmd(m_req_az_off);
  const PIUCmd& elcmd(m_req_el_off);
  
  std::ostringstream cmd_stream;
  datastring cmd_str;
  datastring resp_str;  
  ResponseStatus rs;

  // Azimuth
  cmd_stream.str("");
  cmd_stream << std::hex << std::uppercase << std::setfill('0')
	     << std::setw(6) << std::setprecision(6) << azbar;

  rs = command(cmd_stream.str(),resp_str,azcmd);

  if(rs==RS_NAK)
    throw(ScopeAPIError("PIU protocol error",
		    std::string("PIU response was NAK")));
  else if(rs!=RS_OK)
    throw(ScopeAPIError("PIU protocol error",
		    std::string("PIU response not recognised"),
		    std::string("Response: "+resp_str)));

  // Elevation
  cmd_stream.str("");
  cmd_stream << std::hex << std::uppercase << std::setfill('0')
	     << std::setw(6) << std::setprecision(6) << elbar;

  rs = command(cmd_stream.str(),resp_str,elcmd);

  if(rs==RS_NAK)
    throw(ScopeAPIError("PIU protocol error",
		    std::string("PIU response was NAK")));
  else if(rs!=RS_OK)
    throw(ScopeAPIError("PIU protocol error",
		    std::string("PIU response not recognised"),
		    std::string("Response: "+resp_str)));

  if(m_data_stream->loud()>=1)
    Debug::stream() << VATime::now().getMSTimeStamp() << ' ' << cf << '(' 
		    << az_angle.deg() << ',' << el_angle.degPM() << ')' 
		    << std::endl;
}

void PIUScopeAPI::reqPIDParameters(PIDParameters& az_param,
				   PIDParameters& el_param)
{
  const std::string cf("PIUScopeAPI::reqPIDParameters");
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  int vlim;
  int alim;

  const PIUCmd& azcmd(m_req_az_pid);
  const PIUCmd& elcmd(m_req_el_pid);

  datastring cmd_str;
  datastring resp_str; 
  ResponseStatus rs;

  // Azimuth
  rs = command("",resp_str,azcmd);
  if(rs!=RS_OK)
    throw(ScopeAPIError("PIU protocol error",
		    std::string("PIU response not recognised"),
		    std::string("Response: "+resp_str)));

  int n;
  n=sscanf(resp_str.c_str(),"%4X%4X%4X%4X%4X%2X%2X", &az_param.Kp,
	   &az_param.Ki, &az_param.Kd, &az_param.Kvff, &az_param.Ilim,
	   &vlim, &alim);
  if(n!=7)
    throw(ScopeAPIError("PIU protocol error",
		    std::string("Could not sscanf Az response"),
		    std::string("Response: "+resp_str)));
  
  az_param.vlim = double(vlim)*m_protocol_az_vel_scale;
  az_param.alim = double(alim)*m_protocol_az_acc_scale;

  // Elevation
  rs = command("",resp_str,elcmd);
  if(rs!=RS_OK)
    throw(ScopeAPIError("PIU protocol error",
		    std::string("PIU response not recognised"),
		    std::string("Response: "+resp_str)));

  n=sscanf(resp_str.c_str(),"%4X%4X%4X%4X%4X%2X%2X", &el_param.Kp,
	   &el_param.Ki, &el_param.Kd, &el_param.Kvff, &el_param.Ilim,
	   &vlim, &alim);
  if(n!=7)
    throw(ScopeAPIError("PIU protocol error",
		    std::string("Could not sscanf El response")
		    std::string("Response: "+resp_str)));
  
  el_param.vlim = double(vlim)*m_protocol_el_vel_scale;
  el_param.alim = double(alim)*m_protocol_el_acc_scale;

#ifdef USE_FIXED_AZMAXSPEED
  if(m_protocol_version == PV_PROTOTYPE)
    m_max_az_vel = USE_FIXED_AZMAXSPEED;
  else
    m_max_az_vel = az_param.vlim;
#else
  m_max_az_vel = az_param.vlim;
#endif

#ifdef USE_FIXED_ELMAXSPEED
  if(m_protocol_version == PV_PROTOTYPE)
    m_max_el_vel = USE_FIXED_ELMAXSPEED;
  else
    m_max_az_vel = az_param.vlim;
#else
  m_max_el_vel = el_param.vlim;
#endif

  if(m_data_stream->loud()>=1)
    Debug::stream() << VATime::now().getMSTimeStamp() << ' ' << cf << "() = " 
		    << az_param.Kp << ' ' << az_param.Ki << ' ' 
		    << az_param.Kd << ' ' << az_param.Kvff << ' ' 
		    << az_param.Ilim << ' ' << az_param.vlim << ' ' 
		    << az_param.alim << ' ' 
		    << el_param.Kp << ' ' << el_param.Ki << ' ' 
		    << el_param.Kd << ' ' << el_param.Kvff << ' ' 
		    << el_param.Ilim << ' ' << el_param.vlim << ' ' 
		    << el_param.alim << std::endl;
}

void PIUScopeAPI::setPIDParameters(const PIDParameters& az_param,
				   const PIDParameters& el_param)
{
  const std::string cf("PIUScopeAPI::setPIDParameters");
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);

  const PIUCmd& azcmd(m_set_az_pid);
  const PIUCmd& elcmd(m_set_el_pid);
  
  int vlim;
  int alim;

  std::ostringstream cmd_stream;
  datastring cmd_str;
  datastring resp_str;  
  ResponseStatus rs;

  // Azimuth
  vlim = int(round(az_param.vlim/m_protocol_az_vel_scale)); 
  if(vlim>m_protocol_az_vel_limit)vlim=m_protocol_az_vel_limit;
  else if(vlim<0)vlim=0;

  alim = int(round(az_param.alim/m_protocol_az_acc_scale));
  if(alim>m_protocol_az_acc_limit)alim=m_protocol_az_acc_limit;
  else if(alim<0)alim=0;

  m_max_az_vel = double(vlim)/mprotocol_az_vel_scale;

  cmd_stream.str("");
  cmd_stream << std::hex << std::uppercase << std::setfill('0')
	     << std::setw(4) << std::setprecision(4) << az_param.Kp
	     << std::setw(4) << std::setprecision(4) << az_param.Ki
	     << std::setw(4) << std::setprecision(4) << az_param.Kd
	     << std::setw(4) << std::setprecision(4) << az_param.Kvff
	     << std::setw(4) << std::setprecision(4) << az_param.Ilim
	     << std::setw(2) << std::setprecision(2) << vlim
	     << std::setw(2) << std::setprecision(2) << alim;
					
  rs = command(cmd_stream.str(),resp_str,azcmd);

  if(rs==RS_NAK)
    throw(ScopeAPIError("PIU protocol error",
		    std::string("PIU response was NAK")));
  else if(rs!=RS_OK)
    throw(ScopeAPIError("PIU protocol error",
		    std::string("PIU response not recognised"),
		    std::string("Response: "+resp_str)));

  // Elevation
  vlim = int(round(el_param.vlim/m_protocol_el_vel_scale));
  if(vlim>m_protocol_el_vel_limit)vlim=m_protocol_el_vel_limit; 
  else if(vlim<0)vlim=0;

  alim = int(round(el_param.alim/m_protocol_el_acc_scale));
  if(alim>m_protocol_el_acc_limit)alim=m_protocol_el_acc_limit; 
  else if(alim<0)alim=0;

  m_max_el_vel = double(vlim)*m_protocol_el_vel_scale;

  cmd_stream.str("");
  cmd_stream << std::hex << std::uppercase << std::setfill('0')
	     << std::setw(4) << std::setprecision(4) << el_param.Kp
	     << std::setw(4) << std::setprecision(4) << el_param.Ki
	     << std::setw(4) << std::setprecision(4) << el_param.Kd
	     << std::setw(4) << std::setprecision(4) << el_param.Kvff
	     << std::setw(4) << std::setprecision(4) << el_param.Ilim
	     << std::setw(2) << std::setprecision(2) << vlim
	     << std::setw(2) << std::setprecision(2) << alim;
					
  rs = command(cmd_stream.str(),resp_str,elcmd);

  if(rs==RS_NAK)
    throw(ScopeAPIError("PIU protocol error",
		    std::string("PIU response was NAK")));
  else if(rs!=RS_OK)
    throw(ScopeAPIError("PIU protocol error",
		    std::string("PIU response not recognised"),
		    std::string("Response: "+resp_str)));

  if(m_data_stream->loud()>=1)
    Debug::stream() << VATime::now().getMSTimeStamp() << ' ' << cf << '(' 
		    << az_param.Kp << ',' << az_param.Ki << ',' 
		    << az_param.Kd << ',' << az_param.Kvff << ',' 
		    << az_param.Ilim << ',' << az_param.vlim << ',' 
		    << az_param.alim << ',' 
		    << el_param.Kp << ',' << el_param.Ki << ',' 
		    << el_param.Kd << ',' << el_param.Kvff << ',' 
		    << el_param.Ilim << ',' << el_param.vlim << ',' 
		    << el_param.alim << ')' <<std::endl;
}
#endif

void PIUScopeAPI::resetCommunication()
{
  const std::string cf("PIUScopeAPI::resetCommunication");
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mtx);
  m_data_stream->resetDataStream();

  const PIUCmd& cmd(m_cmd_clear_bits);
  datastring resp_str; 
  ResponseStatus rs = command("",resp_str,cmd);

  if(rs==RS_NAK)
    throw(ScopeAPIError("PIU protocol error",
			std::string("PIU response was NAK")));
  else if(rs!=RS_OK)
    throw(ScopeAPIError("PIU protocol error",
			std::string("PIU response not recognised"),
			std::string("Response: "+resp_str)));
  
  if(m_data_stream->loud()>=1)
    Debug::stream() << VATime::now().getMSTimeStamp() << ' ' << cf << "()"
		    << std::endl;
}

std::string PIUScopeAPI::apiName() const
{
  return std::string("PIU ")+m_data_stream->udsl();
}

void PIUScopeAPI::processOneCommand(long cmd_to_sec, long cmd_to_usec)
{
  const std::string cf("PIUScopeAPI::processOneCommand");
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  datastring data;
  try
    {
      data=m_data_stream->recvData(2,cmd_to_sec,cmd_to_usec);
    }
  catch(const Timeout&)
    {
      return;
    }
  
  if(data.size()<2)return;
  if(data[0] != CHAR_SOM)return;

  const PIUCmd* cmd_ptr(0);

  char sel_char = data[1];

  if(sel_char == m_req_stat.cmd_char) { cmd_ptr = &m_req_stat; }
  else if(sel_char == m_req_az_off.cmd_char) { cmd_ptr = &m_req_az_off; }
  else if(sel_char == m_req_el_off.cmd_char) { cmd_ptr = &m_req_el_off; }
  else if(sel_char == m_req_az_pid.cmd_char) { cmd_ptr = &m_req_az_pid; }
  else if(sel_char == m_req_el_pid.cmd_char) { cmd_ptr = &m_req_el_pid; }
  else if(sel_char == m_cmd_clear_bits.cmd_char) {cmd_ptr = &m_cmd_clear_bits;}
  else if(sel_char == m_cmd_point.cmd_char) { cmd_ptr = &m_cmd_point; }
  else if(sel_char == m_cmd_slew.cmd_char) { cmd_ptr = &m_cmd_slew; }
  else if(sel_char == m_cmd_standby.cmd_char) { cmd_ptr = &m_cmd_standby; }
  else if(sel_char == m_set_az_off.cmd_char) { cmd_ptr = &m_set_az_off; }
  else if(sel_char == m_set_el_off.cmd_char) { cmd_ptr = &m_set_el_off; }
  else if(sel_char == m_set_az_pid.cmd_char) { cmd_ptr = &m_set_az_pid; }
  else if(sel_char == m_set_el_pid.cmd_char) { cmd_ptr = &m_set_el_pid; }
  else if(sel_char == m_req_limits.cmd_char) { cmd_ptr = &m_req_limits; }
  else if(sel_char == m_cmd_az_sec.cmd_char) { cmd_ptr = &m_cmd_az_sec; }
  else if(sel_char == m_cmd_el_sec.cmd_char) { cmd_ptr = &m_cmd_el_sec; }
  else if(sel_char == m_cmd_raster.cmd_char) { cmd_ptr = &m_cmd_raster; }
  else if(sel_char == m_set_limits.cmd_char) { cmd_ptr = &m_set_limits; }
  else if(sel_char == m_set_rf_chan.cmd_char) { cmd_ptr = &m_set_rf_chan; }
  else if(sel_char == m_set_feed_pol.cmd_char) { cmd_ptr = &m_set_feed_pol; }
  else
    {
      Debug::stream() << VATime::now().getMSTimeStamp() << ' ' << cf 
		      << ": Command code " << int(sel_char) 
		      << " not recognised" << std::endl;
      return;
    }

  try
    {
      getCommandData(data,*cmd_ptr,cmd_to_sec,cmd_to_usec);
    }
  catch(...)
    {
      m_data_stream->resetDataStream();
      return;
    }

  
  std::ostringstream stream;

  if(sel_char == m_req_stat.cmd_char)
    {
      PositionerStatus state;
      m_server->reqStat(state);

      int azbar=0;
      int elbar=0;
      int analog1=0;
      int analog2=0;
      int rf_mode=0;
      int relay_interlock=0;
      int limits_interlock=0;
      int errors_servo=0;
      int cable_wrap=0;
      
      azbar = SEphem::Angle::makeDeg(state.az.driveangle_deg).bar(24);
      elbar = SEphem::Angle::makeDeg(state.el.driveangle_deg).bar(24);
      analog1 = int(round(state.Analog1/m_adc_volts_per_dc));
      if(analog1>0x07FF)analog1=0x07FF;
      else if(analog1<-0x07FF)analog1=-0x07FF;
      if(analog1<0)analog1+=0x1000;
      analog2 = int(round(state.Analog2/m_adc_volts_per_dc));
      if(analog2>0x07FF)analog2=0x07FF;
      else if(analog2<-0x07FF)analog2=-0x07FF;
      if(analog2<0)analog2+=0x1000;
      
      if((state.az.driveMode==DM_STANDBY)&&
	 (state.el.driveMode==DM_STANDBY))rf_mode=0x00;
      else if((state.az.driveMode==DM_SLEW)&&
	      (state.el.driveMode==DM_SLEW))rf_mode=0x01;
      else if((state.az.driveMode==DM_POINT)&&
	      (state.el.driveMode==DM_POINT))rf_mode=0x02;
      else rf_mode = 0x0F;

      relay_interlock = ((state.remoteControl?0x00:0x80)|
			 (state.azTravelledCCW?0x40:0x00)|
			 (state.relay2?0x20:0x00)|
			 (state.relay1?0x10:0x00)|
			 (state.interlockAzPullCord?0x08:0x00)|
			 (state.interlockElDoorOpen?0x02:0x00)|
			 (state.interlockAzDoorOpen?0x01:0x00));

      limits_interlock = ((state.interlockElStowPin?0x80:0x00)|
			  (state.interlockAzStowPin?0x40:0x00)|
			  (state.interlockSafeSwitch?0x20:0x00)|
			  (state.interlock?0x10:0x00)|
			  (state.el.limitCcwDown?0x08:0x00)|
			  (state.el.limitCwUp?0x04:0x00)|
			  (state.az.limitCcwDown?0x02:0x00)|
			  (state.az.limitCwUp?0x01:0x00));

      errors_servo = ((state.el.servo2Fail?0x80:0x00)|
		      (state.el.servo1Fail?0x40:0x00)|
		      (state.msgCommandInvalid?0x20:0x00)|
		      (state.msgBadFrame?0x10:0x00)|
		      (state.msgInputOverrun?0x08:0x00)|
		      (state.msgOutputOverrun?0x04:0x00)|
		      (state.az.servo2Fail?0x02:0x00)|
		      (state.az.servo1Fail?0x01:0x00));

      cable_wrap = int(round(state.azCableWrap*100));
      if(cable_wrap>100)cable_wrap=100;
      else if(cable_wrap<-100)cable_wrap=-100;
      if(cable_wrap<0)cable_wrap+=256;

      stream << std::hex << std::uppercase << std::setfill('0')
	     << std::setw(6) << std::setprecision(6) << azbar
	     << std::setw(6) << std::setprecision(6) << elbar
	     << std::setw(4) << std::setprecision(4) << analog1
	     << std::setw(4) << std::setprecision(4) << analog2
	     << std::setw(2) << std::setprecision(2) << rf_mode
	     << std::setw(2) << std::setprecision(2) << relay_interlock
	     << std::setw(2) << std::setprecision(2) << limits_interlock
	     << std::setw(2) << std::setprecision(2) << errors_servo
	     << std::setw(2) << std::setprecision(2) << cable_wrap;

      if(m_data_stream->loud()>=1)
	Debug::stream()
	  << VATime::now().getMSTimeStamp() << ' ' << cf
	  << ": reqStatus() = " 
	  << state.az.driveangle_deg << ' ' 
	  << state.az.servo1Fail << ' '
	  << state.az.servo2Fail << ' '
	  << state.az.limitCwUp << ' '
	  << state.az.limitCcwDown << ' '
	  << state.azTravelledCCW << ' '
	  << state.azCableWrap << ' '
	  << state.el.driveangle_deg << ' '
	  << state.el.servo1Fail << ' '
	  << state.el.servo2Fail << ' '
	  << state.el.limitCwUp << ' '
	  << state.el.limitCcwDown << ' '
	  << state.interlock << ' ' 
	  << state.interlockAzPullCord << ' '
	  << state.interlockAzStowPin << ' '
	  << state.interlockElStowPin << ' '
	  << state.interlockAzDoorOpen << ' '
	  << state.interlockElDoorOpen << ' '
	  << state.interlockSafeSwitch << ' '
	  << state.msgBadFrame << ' '
	  << state.msgCommandInvalid << ' '
	  << state.msgInputOverrun << ' '
	  << state.msgOutputOverrun << ' '
	  << state.Analog1 << ' '
	  << state.Analog2 << std::endl;
    }
  else if(sel_char == m_req_az_off.cmd_char)
    {
      SEphem::Angle azoffset;
      m_server->reqAzOffset(azoffset);
      stream << std::hex << std::uppercase << std::setfill('0')
	     << std::setw(6) << std::setprecision(6) << azoffset.bar(24);

      if(m_data_stream->loud()>=1)
	Debug::stream() << VATime::now().getMSTimeStamp() << ' ' << cf 
			<< ": reqAzOffset() = " << azoffset.deg() << std::endl;
    }
  else if(sel_char == m_req_el_off.cmd_char)
    {
      SEphem::Angle eloffset;
      m_server->reqElOffset(eloffset);
      stream << std::hex << std::uppercase << std::setfill('0')
	     << std::setw(6) << std::setprecision(6) << eloffset.bar(24);

      if(m_data_stream->loud()>=1)
	Debug::stream() << VATime::now().getMSTimeStamp() << ' ' << cf 
			<< ": reqElOffset() = " << eloffset.deg() << std::endl;
    }
  else if(sel_char == m_req_az_pid.cmd_char)
    {
      PIDParameters az_param;
      m_server->reqAzPIDParameters(az_param);
      int vlim = int(round(az_param.vlim/m_protocol_az_vel_scale)); 
      if(vlim>m_protocol_az_vel_limit)vlim=m_protocol_az_vel_limit;
      else if(vlim<0)vlim=0;
      int alim = int(round(az_param.alim/m_protocol_az_acc_scale));
      if(alim>m_protocol_az_acc_limit)alim=m_protocol_az_acc_limit; 
      else if(alim<0)alim=0;
      stream << std::hex << std::uppercase << std::setfill('0')
	     << std::setw(4) << std::setprecision(4) << az_param.Kp
	     << std::setw(4) << std::setprecision(4) << az_param.Ki
	     << std::setw(4) << std::setprecision(4) << az_param.Kd
	     << std::setw(4) << std::setprecision(4) << az_param.Kvff
	     << std::setw(4) << std::setprecision(4) << az_param.Ilim
	     << std::setw(2) << std::setprecision(2) << vlim
	     << std::setw(2) << std::setprecision(2) << alim;

      if(m_data_stream->loud()>=1)
	Debug::stream() << VATime::now().getMSTimeStamp() << ' ' << cf
			<< ": reqAzPID() = " 
			<< az_param.Kp << ' ' << az_param.Ki << ' ' 
			<< az_param.Kd << ' ' << az_param.Kvff << ' ' 
			<< az_param.Ilim << ' ' << az_param.vlim << ' ' 
			<< az_param.alim << std::endl;
    }
  else if(sel_char == m_req_el_pid.cmd_char)
    {
      PIDParameters el_param;
      m_server->reqElPIDParameters(el_param);

      int vlim = int(round(el_param.vlim/m_protocol_el_vel_scale));
      if(vlim>m_protocol_el_vel_limit)vlim=m_protocol_el_vel_limit; 
      else if(vlim<0)vlim=0;
      int alim = int(round(el_param.alim/m_protocol_el_acc_scale));
      if(alim>m_protocol_el_acc_limit)alim=m_protocol_el_acc_limit; 
      else if(alim<0)alim=0;
      stream << std::hex << std::uppercase << std::setfill('0')
	     << std::setw(4) << std::setprecision(4) << el_param.Kp
	     << std::setw(4) << std::setprecision(4) << el_param.Ki
	     << std::setw(4) << std::setprecision(4) << el_param.Kd
	     << std::setw(4) << std::setprecision(4) << el_param.Kvff
	     << std::setw(4) << std::setprecision(4) << el_param.Ilim
	     << std::setw(2) << std::setprecision(2) << vlim
	     << std::setw(2) << std::setprecision(2) << alim;

      if(m_data_stream->loud()>=1)
	Debug::stream() << VATime::now().getMSTimeStamp() << ' ' << cf
			<< ": reqElPID() = " 
			<< el_param.Kp << ' ' << el_param.Ki << ' ' 
			<< el_param.Kd << ' ' << el_param.Kvff << ' ' 
			<< el_param.Ilim << ' ' << el_param.vlim << ' ' 
			<< el_param.alim << std::endl;
    }
  else if(sel_char == m_cmd_clear_bits.cmd_char)
    {
      stream << "ACK";
      if(m_data_stream->loud()>=1)
	Debug::stream() << VATime::now().getMSTimeStamp() << ' ' << cf 
			<< ": cmdClearBits()" << std::endl;
    }
  else if(sel_char == m_cmd_point.cmd_char)
    {
      int azbar;
      int elbar;
      int azvel;
      int elvel;
      if(sscanf(data.c_str(),"%6X%6X%2X%2X",&azbar,&elbar,&azvel,&elvel)==4)
	{
	  PIDParameters el_param; m_server->reqElPIDParameters(el_param);
	  PIDParameters az_param; m_server->reqAzPIDParameters(az_param);
	  SEphem::Angle az = SEphem::Angle::makeBAR(azbar,24);
	  SEphem::Angle el = SEphem::Angle::makeBAR(elbar,24);
	  double azv = double(azvel)/100.0*az_param.vlim;
	  double elv = double(elvel)/100.0*el_param.vlim;
	  m_server->cmdPoint(az,azv,el,elv);	  
	  stream << "ACK";

          if(m_data_stream->loud()>=1)
	    Debug::stream() << VATime::now().getMSTimeStamp() << ' ' << cf 
			    << ": cmdPoint(" << az.deg() << ',' << azv << ','
			    << el.degPM() << ',' << elv << ')' << std::endl; 
	}
      else stream << "NAK";
    }
  else if(sel_char == m_cmd_slew.cmd_char)
    {
      int azvel;
      int elvel;
      if(sscanf(data.c_str(),"%2X%2X",&azvel,&elvel)==2)
	{
	  PIDParameters el_param; m_server->reqElPIDParameters(el_param);
	  PIDParameters az_param; m_server->reqAzPIDParameters(az_param);
	  if(azvel>127)azvel=azvel-256;
	  if(elvel>127)elvel=elvel-256;
	  double azv = (azvel)/100.0*az_param.vlim;
	  double elv = (elvel)/100.0*el_param.vlim;
	  m_server->cmdSlew(azv,elv);
	  stream << "ACK";

          if(m_data_stream->loud()>=1)
	    Debug::stream() << VATime::now().getMSTimeStamp() << ' ' << cf 
			    << ": cmdSlew(" << azv << ',' << elv << ')' 
			    << std::endl; 
	}
      else stream << "NAK";
    }
  else if(sel_char == m_cmd_standby.cmd_char)
    {
      m_server->cmdStandby();
      stream << "ACK";
      if(m_data_stream->loud()>=1)
	Debug::stream() << VATime::now().getMSTimeStamp() << ' ' << cf 
			<< ": cmdStandby()" << std::endl;
    }
  else if(sel_char == m_set_az_off.cmd_char)
    {
      int bar;
      if(sscanf(data.c_str(),"%6X",&bar)==1)
	{
	  SEphem::Angle azoffset;
	  azoffset = SEphem::Angle::makeBAR(bar,24);
	  m_server->setAzOffset(azoffset);
	  stream << "ACK";

	  if(m_data_stream->loud()>=1)
	    Debug::stream() << VATime::now().getMSTimeStamp() << ' ' << cf 
			    << ": setAzOffset(" << azoffset.deg() << ')' 
			    << std::endl;
	}
      else stream << "NAK";
    }
  else if(sel_char == m_set_el_off.cmd_char)
    {
      int bar;
      if(sscanf(data.c_str(),"%6X",&bar)==1)
	{
	  SEphem::Angle eloffset;
	  eloffset = SEphem::Angle::makeBAR(bar,24);
	  m_server->setElOffset(eloffset);
	  stream << "ACK";

	  if(m_data_stream->loud()>=1)
	    Debug::stream() << VATime::now().getMSTimeStamp() << ' ' << cf 
			    << ": setElOffset(" << eloffset.deg() << ')' 
			    << std::endl;
	}
      else stream << "NAK";
    }
  else if(sel_char == m_set_az_pid.cmd_char)
    {
      PIDParameters az_param;
      int vlim;
      int alim;
      
      if(sscanf(data.c_str(),"%4X%4X%4X%4X%4X%2X%2X", &az_param.Kp,
		&az_param.Ki, &az_param.Kd, &az_param.Kvff, &az_param.Ilim,
		&vlim, &alim) == 7)
	{
	  az_param.vlim = double(vlim)*m_protocol_az_vel_scale;
	  az_param.alim = double(alim)*m_protocol_az_acc_scale;
	  m_server->setAzPIDParameters(az_param);
	  stream << "ACK";

	  if(m_data_stream->loud()>=1)
	    Debug::stream() << VATime::now().getMSTimeStamp() << ' ' << cf 
			    << ": setAzPID(" 
			    << az_param.Kp << ',' << az_param.Ki << ',' 
			    << az_param.Kd << ',' << az_param.Kvff << ',' 
			    << az_param.Ilim << ',' << az_param.vlim << ',' 
			    << az_param.alim << ')' << std::endl;
	}
      else stream << "NAK";
    }
  else if(sel_char == m_set_el_pid.cmd_char)
    {
      PIDParameters el_param;
      int vlim;
      int alim;
      
      if(sscanf(data.c_str(),"%4X%4X%4X%4X%4X%2X%2X", &el_param.Kp,
		&el_param.Ki, &el_param.Kd, &el_param.Kvff, &el_param.Ilim,
		&vlim, &alim) == 7)
	{
	  el_param.vlim = double(vlim)*m_protocol_el_vel_scale;
	  el_param.alim = double(alim)*m_protocol_el_acc_scale;

	  m_server->setElPIDParameters(el_param);
	  stream << "ACK";

	  if(m_data_stream->loud()>=1)
	    Debug::stream() << VATime::now().getMSTimeStamp() << ' ' << cf
			    << ": setElPID(" 
			    << el_param.Kp << ',' << el_param.Ki << ',' 
			    << el_param.Kd << ',' << el_param.Kvff << ',' 
			    << el_param.Ilim << ',' << el_param.vlim << ',' 
			    << el_param.alim << ')' << std::endl;
	}
      else stream << "NAK";
    }
  else
    {
      Debug::stream() << VATime::now().getMSTimeStamp() << ' ' << cf 
		      << ": Command code " << int(sel_char) 
		      << " not implemented" << std::endl;
      return;
    }

  makeAndSendResponse(stream.str(),*cmd_ptr);
}

void PIUScopeAPI::init()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(!m_init)
    {
      try
	{
	  PIDParameters az_param;
	  PIDParameters el_param;
	  reqAzPIDParameters(az_param);
	  reqElPIDParameters(el_param);
	  m_init=true;
	}
      catch(const Timeout& x)
	{
	  // defer initialization to later
	}
    }
}

void PIUScopeAPI::recoverFromZeroSpeedCondition()
{
#ifdef USE_ZEROSPEED_WORKAROUND
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  Debug::stream() << VATime::now().getMSTimeStamp() << ' ' 
		  << "PIUScopeAPI::recoverFromZeroSpeedCondition()"
		  << std::endl;
  
  // Temporarily set maximum az and el vel to 0.3 dps and then command
  // the telescope to slew at 0 speed. Maybe that will help
  m_max_az_vel=0.4;
  m_max_el_vel=0.4;
  cmdPoint(0,0,0,0);
  m_max_az_vel=0;
  m_max_el_vel=0;

  m_init=0;
  init();
  
  if((m_max_az_vel == 0)||(m_max_el_vel == 0))
    {
      std::ostringstream msg;
      msg << "Maximum azimuth or elevation speed is set to zero." << std::endl
	  << "Telescope motion not possible until this is reset." << std::endl
	  << "Try cycling the power to the mount, and make sure" <<  std::endl
	  << "only one copy of this program is running, and that " << std::endl
	  << "the TCU box is powered off." << std::endl << std::endl
	  << "MaxAzSpeed=" << m_max_az_vel << ' ' 
	  << "MaxElSpeed=" << m_max_el_vel << std::endl;
      throw(ScopeAPIError("PIU protocol error",msg.str()));
    }
#endif // USE_ZEROSPEED_WORKAROUND
}

void PIUScopeAPI::makeAndSendCommand(const datastring& cmd_data, 
				     const PIUCmd& cmd)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  std::ostringstream cmd_stream;
  cmd_stream << CHAR_SOM << cmd.cmd_char << cmd_data << CHAR_EOM;
  datastring cmd_stream_str = cmd_stream.str();

  if(cmd_stream_str.size() != cmd.cmd_len)
    {
      std::ostringstream msg;
      msg << "Command data length of " << cmd_stream_str.size() 
	  << " does not match required length " << cmd.cmd_len << std::endl
	  << "Command data: ";
      DataStream::printAsAscii(msg,cmd_stream_str);
      throw(ScopeAPIError("PIU protocol error",msg.str()));
    }

  m_data_stream->resetDataStream();
  m_data_stream->sendData(cmd_stream_str);
}

PIUScopeAPI::ResponseStatus 
PIUScopeAPI::getAndCheckResponse(datastring& resp, const PIUCmd& cmd)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  // If the response length is zero then we do not get (or need) a response
  if(cmd.resp_len==0)
    {
      if(cmd.cmd_complexity)
	ZThread::Thread::sleep(15*cmd.cmd_complexity); // sleep for 15ms quanta
      return RS_OK;
    }

  resp = m_data_stream->
    recvData(cmd.resp_len, 
	     m_data_stream->defaultTOSec()*cmd.cmd_complexity, 
	     m_data_stream->defaultTOuSec()*cmd.cmd_complexity);

  if((resp.size() != cmd.resp_len)||
     ((resp.size()>=1) && (resp[0]!=CHAR_SOM))||
     (resp[resp.size()-1]!=CHAR_EOM))return RS_NOT_UNDERSTOOD;

  if(cmd.resp_char==0x00)
    {
      // 0x00 is used as a code for ACK/NAK response
      if((resp[1]=='A')&&(resp[2]=='C')&&(resp[3]=='K'))
	{
	  resp=std::string("");
	  return RS_OK; 
	}
      else if((resp[1]=='N')&&(resp[2]=='A')&&(resp[3]=='K'))
	{
	  resp=std::string("");
	  return RS_NAK;
	}
      else return RS_NOT_UNDERSTOOD;
    }
  else
    {
      if((resp.size()>=2)&&(resp[1]==cmd.resp_char))
	{
	  resp=resp.substr(2,resp.size()-3);
	  return RS_OK;
	}
      else return RS_NOT_UNDERSTOOD;
    }
}

PIUScopeAPI::ResponseStatus 
PIUScopeAPI::command(const datastring& cmd_data, datastring& resp, 
		     const PIUCmd& cmd)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  for(unsigned com_tries=0; com_tries<m_com_tries; com_tries++)
    {
      try
	{
	  makeAndSendCommand(cmd_data,cmd);
	  return getAndCheckResponse(resp,cmd);
	}
      catch(const Timeout& x)
	{
	  // catch the timeout and try again
	}
    }
  throw Timeout();
}

void PIUScopeAPI::getCommandData(datastring& data, const PIUCmd& cmd,
				 long cmd_to_sec, long cmd_to_usec)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(data.size() < cmd.cmd_len)
    {
      data += m_data_stream->recvData(cmd.cmd_len-data.size(),
				      cmd_to_sec, cmd_to_usec);
    }
  
  if(data.size() != cmd.cmd_len)
    {
      std::ostringstream msg;
      msg << "Command length of " << data.size() 
	  << " does not match required length " << cmd.cmd_len;
      throw(ScopeAPIError("PIU protocol error",msg.str()));
    }

  if(data[data.size()-1] != CHAR_EOM)
    throw(ScopeAPIError("PIU protocol error","EOM not found"));
  
  data = data.substr(2,cmd.cmd_len-3);
}

void PIUScopeAPI::makeAndSendResponse(const datastring& resp_data, 
				      const PIUCmd& cmd)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  // If the response length is zero then we do not send a response
  if(cmd.resp_len==0)return;

  std::ostringstream resp_stream;

  if(cmd.resp_char == 0x00)
    {
      if((resp_data!="ACK")&&(resp_data!="NAK"))
	{
	  std::ostringstream msg;
	  msg << "ACK/NAK response not given: " << resp_data;
	  throw(ScopeAPIError("PIU protocol error",msg.str()));
	}
      resp_stream << CHAR_SOM << resp_data << CHAR_EOM;
    }
  else
    {
      if(resp_data.size() != cmd.resp_len-3)
	{
	  std::ostringstream msg;
	  msg << "Response data length of " << resp_data.size() 
	      << " does not match required length " << cmd.resp_len-3;
	  throw(ScopeAPIError("PIU protocol error",msg.str()));
	}
      resp_stream << CHAR_SOM << cmd.resp_char << resp_data << CHAR_EOM;
    }      
  
  m_data_stream->sendData(resp_stream.str());
}

#ifdef TESTMAIN
#include<unistd.h>
#include"DataStream.h"

int main(int argc, char** argv)
{
  long timeout = 100000;
  long delay = 100000;
  unsigned n=1000;
  unsigned tries=3;

  argc--,argv++;
  if(argc){ std::istringstream(*argv) >> timeout; argc--; argv++; }
  if(argc){ std::istringstream(*argv) >> delay; argc--; argv++; }
  if(argc){ std::istringstream(*argv) >> n; argc--; argv++; }
  if(argc){ std::istringstream(*argv) >> tries; argc--; argv++; }

  Debug::stream()
    << "Iterations: " << n << "  Timeout: " << timeout
    << "  Delay: " << delay << "  Tries: " << tries << std::endl;

  try
    {
      //UDPDataStream ds("127.0.0.1:5000",UDPDataStream::OM_CLIENT,1);
      UDPDataStream ds("192.168.1.50:5000",UDPDataStream::OM_CLIENT,1);
      PIUScopeAPI api(&ds,0,timeout,1,tries);

      unsigned ntimeout=0;
      unsigned nerror=0;

      for(unsigned i=0;i<n;i++)
	{
	  try
	    {
	      PIUScopeAPI::PositionerStatus status;
	      api.reqStat(status);
	    }
	  catch(const Timeout& x)
	    {
	      Debug::stream() << "Timeout" << std::endl;
	      ntimeout++;
	    }
	  catch(const Error& x)
	    {
	      Debug::stream() << "Error" << std::endl;
	      nerror++;
	    }
	  usleep(delay);
	}

      Debug::stream()
	<< "Iterations: " << n 
	<< "  Timeouts: " << ntimeout << "  Errors: " << nerror 
	<< std::endl;
    }
  catch(const Error& x)
    {
      x.print(Debug::stream());
    }
  catch(const Timeout& x)
    {
      Debug::stream() << "Timeout" << std::endl;
    }
}
#endif

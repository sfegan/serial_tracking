//-*-mode:c++; mode:font-lock;-*-

/**
 * \file EIA422.cpp
 * \ingroup VTracking
 * \brief Implementation of EIA-422 serial protocol from VERITAS prototype
 *
 * This code is now obselete
 *
 * Original Author: Stephen Fegan
 * $Author: sfegan $
 * $Date: 2006/04/10 18:01:11 $
 * $Revision: 2.1 $
 * $Tag$
 *
 **/

#include<iostream>
#include<iomanip>
#include<sstream>
#include<memory>
#include<cstdlib>
#include<unistd.h>
#include<fcntl.h>
#include<termios.h>
#include<sys/types.h>
#include<sys/time.h>
#include<sys/ioctl.h>
#include<sys/stat.h>
#include<sys/select.h>

#include<Exception.h>
#include<Debug.h>
#include<Angle.h>
//#include<SphericalCoords.h>

#include"EIA422.h"

using namespace SEphem;
using namespace VTracking;
using namespace VMessaging;

const double EIA422ScopeAPI::sc_el_vel_ratio;
const double EIA422ScopeAPI::sc_az_vel_ratio;
const double EIA422ScopeAPI::sc_adc_volts_per_dc;

#define USE24BIT

EIA422ScopeAPI::~EIA422ScopeAPI()
{
  // nothing to see here
}

void EIA422ScopeAPI::Status::print(std::ostream& stream)
{
  stream << limitCwUp << ' ' 
	 << limitCcwDown << ' '
	 << brakeReleased << ' ' 
	 << positionComplete << ' '
	 << interlock << ' '
	 << positionFault << ' '
	 << servoAmpFault << ' '
	 << servoOn << ' ';
  switch(servoMode)
    {
    case SM_IDLE:
      stream << "IDLE";
      break;
    case SM_POSITION:
      stream << "POSN";
      break;
    case SM_RATE:
      stream << "RATE";
      break;
    default:
      stream << "UNKN";
      break;
    }
  stream << ' '
	 << commandUnderOverFlow << ' '
	 << checksumOK << ' '
	 << directionSignBit << ' '
	 << commandUnrecognised;
}

void EIA422ScopeAPI::ExtStatus::print(std::ostream& stream)
{
  stream << remoteControl << ' ' 
	 << ADCValue << ' '
	 << interlockHatch << ' ' 
	 << interlockStowPin << ' '
	 << interlockSafe << ' '
	 << interlockPullCord;
}

// ----------------------------------------------------------------------------
// EIA422Protocol
// ----------------------------------------------------------------------------

EIA422Protocol::~EIA422Protocol()
{
  // nothing to see here
}

datastring EIA422Protocol::
encodeCommand(Command cmd, size_t bytes, CmdDataType data) const
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(m_data_stream->loud()>=2)
    Debug::stream() << "EIA422Protocol::encodeCommandData(): "
		    << std::setw(8) << std::setprecision(8) << std::hex 
		    << std::setfill('0') << data << std::endl;

  datastring out_buffer;
  if(cmd != RESPONSE)
    out_buffer.push_back((unsigned char)(cmd));
  for(unsigned i=0;i<bytes;i++)
    out_buffer.push_back((unsigned char)((data>>(i*6))&0x3F)|(((i%3)+1)<<6));
  return out_buffer;
}

EIA422Protocol::CmdDataType EIA422Protocol::
decodeCommandData(const datastring& buffer) const
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  CmdDataType data(0);
  for(unsigned i=0;i<buffer.size();i++)
    {
      unsigned char dbit = buffer[i]&0x3F;
      unsigned char dnum = (buffer[i]&0xC0)>>6;
      if(unsigned(dnum) != (i%3)+1)
	{
	  Exception exception("EIA422 protocol error");
	  exception.messageStream() 
	    << "Received byte code " << unsigned(dnum)
	    << " did not match expected " << (i%3)+1;
	  throw exception;
	}
      data=data|(CmdDataType(dbit)<<(i*6));
    }
  if(m_data_stream->loud()>=2)
    Debug::stream() << "EIA422Protocol::decodeCommandData(): "
		    << std::setw(8) << std::setprecision(8) << std::hex 
		    << std::setfill('0') << data << std::endl;
  return data;
}

void EIA422Protocol::
encodeAndSendCommand(Command cmd, size_t bytes, CmdDataType data) const
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  datastring out = encodeCommand(cmd, bytes, data);
  m_data_stream->sendData(out);
}

EIA422Protocol::CmdDataType EIA422Protocol::
recvAndDecodeCommandData(size_t bytes, const std::string& leftover) const
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  std::string cf("recvAndDecodeCommandData()");
  CmdDataType data;

  datastring in;

  if(leftover.size()>1)in=leftover.substr(1,leftover.size()-1);
  else in = m_data_stream->recvData(bytes);
  
  if(in.size()!=bytes)
    {
      Exception exception("EIA422 protocol error");
      exception.messageStream() 
	<< "Received byte count " << in.size() 
	<< " does not match expected " << bytes;
      throw exception;
    }
  data=decodeCommandData(in);
  
  return data;
}

EIA422Protocol::CmdDataType EIA422Protocol::encodeShort(int i) const
{
  return (i&0xffff)<<2;
}

int EIA422Protocol::decodeShort(CmdDataType i) const
{
  return (i>>2)&0xffff;
}

#define ROUNDER false

EIA422Protocol::CmdDataType 
EIA422Protocol::encodeAngle16(const SEphem::Angle& angle) const
{
  return angle.bar(16,ROUNDER)<<2;
}

SEphem::Angle EIA422Protocol::decodeAngle16(CmdDataType angle) const
{
  return SEphem::Angle::makeBAR(angle>>2, 16, ROUNDER);
}
  
EIA422Protocol::CmdDataType 
EIA422Protocol::encodeAngle24(const SEphem::Angle& angle) const
{
  return angle.bar(24,ROUNDER);
}

SEphem::Angle EIA422Protocol::decodeAngle24(CmdDataType angle) const
{
  return SEphem::Angle::makeBAR(angle, 24, ROUNDER);
}

EIA422Protocol::CmdDataType 
EIA422Protocol::encodeStatus(const Status& status) const
{
  CmdDataType d=0x0000;
  if(status.limitCwUp)d |= (0x0001<<2);
  if(status.limitCcwDown)d |= (0x0001<<3);
  if(status.brakeReleased)d |= (0x0001<<4);
  if(status.positionComplete)d |= (0x0001<<5);
  if(status.interlock)d |= (0x0001<<6);
  if(status.positionFault)d |= (0x0001<<7);
  if(status.servoAmpFault)d |= (0x0001<<8);
  if(status.servoOn)d |= (0x0001<<9);
  d |= ((CmdDataType(status.servoMode)&0x000f)<<10);
  if(status.commandUnderOverFlow)d |= (0x0001<<14);
  if(status.checksumOK)d |= (0x0001<<15);
  if(status.directionSignBit)d |= (0x0001<<16);
  if(status.commandUnrecognised)d |= (0x0001<<17);
  return d;
}

EIA422ScopeAPI::Status EIA422Protocol::decodeStatus(CmdDataType data) const
{
  Status status;
  status.limitCwUp=((data>>2)&0x01);
  status.limitCcwDown=((data>>3)&0x01);
  status.brakeReleased=((data>>4)&0x01);
  status.positionComplete=((data>>5)&0x01);
  status.interlock=((data>>6)&0x01);
  status.positionFault=((data>>7)&0x01);
  status.servoAmpFault=((data>>8)&0x01);
  status.servoOn=((data>>9)&0x01);
  switch((data>>10)&0x0C)
    {
    case SM_POSITION:
      status.servoMode=EIA422ScopeAPI::SM_POSITION;
      break;
    case SM_RATE:
      status.servoMode=EIA422ScopeAPI::SM_RATE;
      break;
    case SM_IDLE:
      status.servoMode=EIA422ScopeAPI::SM_IDLE;
      break;
    default:
      {
	RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
	Exception exception("EIA422 protocol error");
	exception.messageStream() 
	  << "Unknown servo mode: " << ((data>>10)&0x0C);
	throw exception;
      }
    }
  status.commandUnderOverFlow=((data>>14)&0x01);
  status.checksumOK=((data>>15)&0x01);
  status.directionSignBit=((data>>16)&0x01);
  status.commandUnrecognised=((data>>17)&0x01);
  return status;
}

EIA422Protocol::CmdDataType 
EIA422Protocol::encodeAzExtStatus(const ExtStatus& status) const
{
  CmdDataType d=0x0000;
  if(status.remoteControl)d |= (0x0001<<1);
  int adc = int(round(status.ADCValue/sc_adc_volts_per_dc));
  if(adc>2047)adc=2047; else if(adc<-2048)adc=-2048; if(adc<0)adc=4096+adc;
  d |= ((CmdDataType(adc)&0x0FFF)<<2);
  if(status.interlockStowPin)d |= (0x0001<<14);
  if(status.interlockPullCord)d |= (0x0001<<15);
  if(status.interlockHatch)d |= (0x0001<<16);
  if(status.interlockSafe)d |= (0x0001<<17);
  return d;
}

EIA422ScopeAPI::ExtStatus 
EIA422Protocol::decodeAzExtStatus(CmdDataType data) const
{
  ExtStatus status;
  status.remoteControl = ((data>>1)&0x01);
  short adc = (data>>2)&0x0FFF;
  if(adc>2047)adc=adc-4096;
  status.ADCValue = double(adc)*sc_adc_volts_per_dc;
  status.interlockStowPin = ((data>>14)&0x01);
  status.interlockPullCord = ((data>>15)&0x01);
  status.interlockHatch = ((data>>16)&0x01);
  status.interlockSafe = ((data>>17)&0x01);
  return status;
}

EIA422Protocol::CmdDataType 
EIA422Protocol::encodeElExtStatus(const ExtStatus& status) const
{
  CmdDataType d=0x0000;
  int adc = int(round(status.ADCValue/sc_adc_volts_per_dc));
  if(adc>2047)adc=2047; else if(adc<-2048)adc=-2048; if(adc<0)adc=4096+adc;
  d |= ((CmdDataType(adc)&0x0FFF)<<2);
  if(status.interlockHatch)d |= (0x0001<<14);
  if(status.interlockStowPin)d |= (0x0001<<15);
  return d;
}

EIA422ScopeAPI::ExtStatus 
EIA422Protocol::decodeElExtStatus(CmdDataType data) const
{
  ExtStatus status;
  status.remoteControl = 0;
  short adc = (data>>2)&0x0FFF;
  if(adc>2047)adc=adc-4096;
  status.ADCValue = double(adc)*sc_adc_volts_per_dc;
  status.interlockHatch = ((data>>14)&0x01);
  status.interlockStowPin = ((data>>15)&0x01);
  status.interlockPullCord = 0;
  status.interlockSafe = 0;
  return status;
}

EIA422Protocol::CmdDataType 
EIA422Protocol::encodeRate(double rate) const
{
  if(rate>1)rate=1;
  else if(rate<-1)rate=-1;
  short irate=short(floor(rate*7819+0.5));
  CmdDataType data=(((CmdDataType)irate)&0xFFFF)<<2;
#if 0
  Debug::stream() 
    << std::dec << rate << ' ' 
    << irate << ' ' <<  std::hex << irate << ' ' << data << std::endl;
#endif
  return data;
}

double 
EIA422Protocol::decodeRate(EIA422Protocol::CmdDataType data) const
{
  short irate = (short)((data>>2)&0xFFFF);
  double rate = double(irate)/7819.0;
#if 0
  Debug::stream()
    << std::hex << data << ' ' 
    << irate << ' ' << std::dec << irate << ' ' << rate << std::endl;
#endif
  return rate;
}

void 
EIA422Protocol::processOneCommand(long cmd_to_sec, long cmd_to_usec)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  const std::string cf("EIA422Protocol::processOneCommand()");

  datastring buffer;
  try
    {
      buffer=m_data_stream->recvData(1,cmd_to_sec,cmd_to_usec);
    }
  catch(const Timeout&)
    {
      return;
    }

  switch(int(buffer[0]))
    {
    case CMD_AZSTAT:
      {
	Status s=reqAzStat();
	encodeAndSendCommand(RESPONSE,3,encodeStatus(s));
	if(m_data_stream->loud()>=1)
	  {
	    Debug::stream()
	      << "EIA422Protocol::processOneCommand: reqAzStat() = ";
	    s.print(Debug::stream());
	    Debug::stream() << std::endl;
	  }
      }
      break;

    case CMD_ELSTAT:
      {
	Status s=reqElStat();
	encodeAndSendCommand(RESPONSE,3,encodeStatus(s));
	if(m_data_stream->loud()>=1)
	  {
	    Debug::stream()
	      << "EIA422Protocol::processOneCommand: reqElStat() = ";
	    s.print(Debug::stream());
	    Debug::stream() << std::endl;
	  }
      }
      break;

    case CMD_REQAZP:
      {
	Angle a=reqAzPos();
#ifdef USE24BIT
	encodeAndSendCommand(RESPONSE,4,encodeAngle24(a));
#else
	encodeAndSendCommand(RESPONSE,3,encodeAngle16(a));
#endif
	if(m_data_stream->loud()>=1)
	  Debug::stream() 
	    << "EIA422Protocol::processOneCommand: reqAzPos() = "
	    << a.deg() << std::endl;
      }
      break;

    case CMD_REQELP:
      {
	Angle a=reqElPos();
#ifdef USE24BIT
	encodeAndSendCommand(RESPONSE,4,encodeAngle24(a));
#else
	encodeAndSendCommand(RESPONSE,3,encodeAngle16(a));
#endif
	if(m_data_stream->loud()>=1)
	  Debug::stream() 
	    << "EIA422Protocol::processOneCommand: reqElPos() = "
	    << a.deg() << std::endl;
      }
      break;

    case CMD_ELSRVPWRON:
      cmdElSrvPwrOn();
      if(m_data_stream->loud()>=1)
	Debug::stream() 
	  << "EIA422Protocol::processOneCommand: cmdElSrvPwrOn()"
	  << std::endl;
      break;

    case CMD_ELSRVPWROFF:
      cmdElSrvPwrOff();
      if(m_data_stream->loud()>=1)
	Debug::stream()
	  << "EIA422Protocol::processOneCommand: cmdElSrvPwrOff()"
	  << std::endl;
      break;

    case CMD_AZSRVPWRON:
      cmdAzSrvPwrOn();
      if(m_data_stream->loud()>=1)
	Debug::stream() 
	  << "EIA422Protocol::processOneCommand: cmdAzSrvPwrOn()"
	  << std::endl;
      break;

    case CMD_AZSRVPWROFF:
      cmdAzSrvPwrOff();
      if(m_data_stream->loud()>=1)
	Debug::stream() 
	  << "EIA422Protocol::processOneCommand: cmdAzSrvPwrOff()"
	  << std::endl;
      break;

    case CMD_AZOFFSETR:
      {
	Angle a=Angle::sc_Pi-reqAzOffset();
	encodeAndSendCommand(RESPONSE,3,encodeAngle16(a));
	if(m_data_stream->loud()>=1)
	  {
	    Angle b = Angle::sc_Pi-a.rad();
	    Debug::stream()
	      << "EIA422Protocol::processOneCommand: reqAzOffset() = "
	      << b.degPM() << std::endl;
	  }
      }
      break;
      
    case CMD_ELOFFSETR:
      {
	Angle a=Angle::sc_Pi-reqElOffset();
	encodeAndSendCommand(RESPONSE,3,encodeAngle16(a));
	if(m_data_stream->loud()>=1)
	  {
	    Angle b = Angle::sc_Pi-a.rad();
	    Debug::stream()
	      << "EIA422Protocol::processOneCommand: reqElOffset() = "
	      << b.degPM() << std::endl;
	  }
      }
      break;

    case CMD_AZSTAT2:
      {
	ExtStatus s=reqAzExtStat();
	encodeAndSendCommand(RESPONSE,3,encodeAzExtStatus(s));
	if(m_data_stream->loud()>=1)
	  {
	    Debug::stream() 
	      << "EIA422Protocol::processOneCommand: reqAzExtStat() = ";
	    s.print(Debug::stream());
	    Debug::stream() << std::endl;
	  }
      }
      break;

    case CMD_ELSTAT2:
      {
	ExtStatus s=reqElExtStat();
	encodeAndSendCommand(RESPONSE,3,encodeElExtStatus(s));
	if(m_data_stream->loud()>=1)
	  {
	    Debug::stream() 
	      << "EIA422Protocol::processOneCommand: reqElExtStat() = ";
	    s.print(Debug::stream());
	    Debug::stream() << std::endl;
	  }
      }
      break;

    case CMD_AZRATE:
      {
	CmdDataType data=recvAndDecodeCommandData(3,buffer);
	double rate = decodeRate(data);
	cmdAzRate(rate);
	if(m_data_stream->loud()>=1)
	  Debug::stream() << "EIA422Protocol::processOneCommand: cmdAzRate("
			  << rate << ')' << std::endl;
      }
      break;

    case CMD_ELRATE:
      {
	CmdDataType data=recvAndDecodeCommandData(3,buffer);
	double rate = decodeRate(data);
	cmdElRate(rate);
	if(m_data_stream->loud()>=1)
	  Debug::stream() << "EIA422Protocol::processOneCommand: cmdElRate("
			  << rate << ')' << std::endl;
      }
      break;

    case CMD_AZOFFSET:
      {
	CmdDataType data=recvAndDecodeCommandData(3,buffer);
	SEphem::Angle angle = Angle::sc_Pi-decodeAngle16(data);
	setAzOffset(angle);
	if(m_data_stream->loud()>=1)
	  Debug::stream() << "EIA422Protocol::processOneCommand: cmdAzOffset("
			  << angle.degPM() << ')' << std::endl;
      }
      break;

    case CMD_ELOFFSET:
      {
	CmdDataType data=recvAndDecodeCommandData(3,buffer);
	SEphem::Angle angle = Angle::sc_Pi-decodeAngle16(data);
	setElOffset(angle);
	if(m_data_stream->loud()>=1)
	  Debug::stream() << "EIA422Protocol::processOneCommand: cmdElOffset("
			  << angle.degPM() << ')' << std::endl;
      }
      break;

    case CMD_AZREVISION:
#warning UNIMPLEMENTED: EIA422Protocol::processOneCommand(CMD_AZREVISION)
      break;

    case CMD_ELREVISION:
#warning UNIMPLEMENTED: EIA422Protocol::processOneCommand(CMD_ELREVISION)
      break;

    case CMD_AZPOSN:
      {
#ifdef USE24BIT
	CmdDataType data=recvAndDecodeCommandData(4,buffer);
	SEphem::Angle angle = decodeAngle24(data);
#else
	CmdDataType data=recvAndDecodeCommandData(3,buffer);
	SEphem::Angle angle = decodeAngle16(data);
#endif
	cmdAzPosn(angle);
	if(m_data_stream->loud()>=1)
	  Debug::stream() << "EIA422Protocol::processOneCommand: cmdAzPosn("
			  << angle.deg() << ')' << std::endl;
      }
      break;

    case CMD_ELPOSN:
      {
#ifdef USE24BIT
	CmdDataType data=recvAndDecodeCommandData(4,buffer);
	SEphem::Angle angle = decodeAngle24(data);
#else
	CmdDataType data=recvAndDecodeCommandData(3,buffer);
	SEphem::Angle angle = decodeAngle16(data);
#endif
	cmdElPosn(angle);
	if(m_data_stream->loud()>=1)
	  Debug::stream() << "EIA422Protocol::processOneCommand: cmdElPosn("
			  << angle.deg() << ')' << std::endl;
      }
      break;

    case CMD_LATCH1ON:
#warning UNIMPLEMENTED: EIA422Protocol::processOneCommand(CMD_LATCH1ON)
      break;
      
    case CMD_LATCH1OFF:
#warning UNIMPLEMENTED: EIA422Protocol::processOneCommand(CMD_LATCH1OFF)
      break;
      
    case CMD_LATCH2ON:
#warning UNIMPLEMENTED: EIA422Protocol::processOneCommand(CMD_LATCH2ON)
      break;

    case CMD_LATCH2OFF:
#warning UNIMPLEMENTED: EIA422Protocol::processOneCommand(CMD_LATCH2OFF)
      break;

    case CMD_REQAZKP:
      {
	int i = reqAzKp();
	encodeAndSendCommand(RESPONSE,3,encodeShort(i));
	if(m_data_stream->loud()>=1)
	  Debug::stream() << "EIA422Protocol::processOneCommand: reqAzKp() = "
			  << i << std::endl;
	
      }
      break;

    case CMD_REQAZKI:
      {
	int i = reqAzKi();
	encodeAndSendCommand(RESPONSE,3,encodeShort(i));
	if(m_data_stream->loud()>=1)
	  Debug::stream() << "EIA422Protocol::processOneCommand: reqAzKi() = "
			  << i << std::endl;
	
      }
      break;


    case CMD_REQAZKD:
      {
	int i = reqAzKd();
	encodeAndSendCommand(RESPONSE,3,encodeShort(i));
	if(m_data_stream->loud()>=1)
	  Debug::stream() << "EIA422Protocol::processOneCommand: reqAzKd() = "
			  << i << std::endl;
	
      }
      break;

    case CMD_REQAZIL:
      {
	int i = reqAzIl();
	encodeAndSendCommand(RESPONSE,3,encodeShort(i));
	if(m_data_stream->loud()>=1)
	  Debug::stream() << "EIA422Protocol::processOneCommand: reqAzIl() = "
			  << i << std::endl;
	
      }
      break;

    case CMD_REQAZACC:
      {
	int i = reqAzAcc();
	encodeAndSendCommand(RESPONSE,3,encodeShort(i));
	if(m_data_stream->loud()>=1)
	  Debug::stream() << "EIA422Protocol::processOneCommand: reqAzAcc() = "
			  << i << std::endl;
	
      }
      break;

    case CMD_REQAZVEL:
      {
	int i = reqAzVel();
	encodeAndSendCommand(RESPONSE,3,encodeShort(i));
	if(m_data_stream->loud()>=1)
	  Debug::stream() << "EIA422Protocol::processOneCommand: reqAzVel() = "
			  << i << std::endl;
	
      }
      break;

    case CMD_REQELKP:
      {
	int i = reqElKp();
	encodeAndSendCommand(RESPONSE,3,encodeShort(i));
	if(m_data_stream->loud()>=1)
	  Debug::stream() << "EIA422Protocol::processOneCommand: reqElKp() = "
			  << i << std::endl;
	
      }
      break;

    case CMD_REQELKI:
      {
	int i = reqElKi();
	encodeAndSendCommand(RESPONSE,3,encodeShort(i));
	if(m_data_stream->loud()>=1)
	  Debug::stream() << "EIA422Protocol::processOneCommand: reqElKi() = "
			  << i << std::endl;
	
      }
      break;

    case CMD_REQELKD:
      {
	int i = reqElKd();
	encodeAndSendCommand(RESPONSE,3,encodeShort(i));
	if(m_data_stream->loud()>=1)
	  Debug::stream() << "EIA422Protocol::processOneCommand: reqElKd() = "
			  << i << std::endl;
	
      }
      break;

    case CMD_REQELIL:
      {
	int i = reqElIl();
	encodeAndSendCommand(RESPONSE,3,encodeShort(i));
	if(m_data_stream->loud()>=1)
	  Debug::stream() << "EIA422Protocol::processOneCommand: reqElIl() = "
			  << i << std::endl;
	
      }
      break;

    case CMD_REQELACC:
      {
	int i = reqElAcc();
	encodeAndSendCommand(RESPONSE,3,encodeShort(i));
	if(m_data_stream->loud()>=1)
	  Debug::stream() << "EIA422Protocol::processOneCommand: reqElAcc() = "
			  << i << std::endl;
	
      }
      break;

    case CMD_REQELVEL:
      {
	int i = reqElVel();
	encodeAndSendCommand(RESPONSE,3,encodeShort(i));
	if(m_data_stream->loud()>=1)
	  Debug::stream() << "EIA422Protocol::processOneCommand: reqElVel() = "
			  << i << std::endl;
	
      }
      break;

    case CMD_SETAZKP:
      {
	CmdDataType data=recvAndDecodeCommandData(3,buffer);
	int i = decodeShort(data);
	setAzKp(i);
	if(m_data_stream->loud()>=1)
	  Debug::stream() << "EIA422Protocol::processOneCommand: setAzKp("
			  << i << ')' << std::endl;
      }
      break;

    case CMD_SETAZKI:
      {
	CmdDataType data=recvAndDecodeCommandData(3,buffer);
	int i = decodeShort(data);
	setAzKi(i);
	if(m_data_stream->loud()>=1)
	  Debug::stream() << "EIA422Protocol::processOneCommand: setAzKi("
			  << i << ')' << std::endl;
      }
      break;

    case CMD_SETAZKD:
      {
	CmdDataType data=recvAndDecodeCommandData(3,buffer);
	int i = decodeShort(data);
	setAzKd(i);
	if(m_data_stream->loud()>=1)
	  Debug::stream() << "EIA422Protocol::processOneCommand: setAzKd("
			  << i << ')' << std::endl;
      }
      break;

    case CMD_SETAZIL:
      {
	CmdDataType data=recvAndDecodeCommandData(3,buffer);
	int i = decodeShort(data);
	setAzIl(i);
	if(m_data_stream->loud()>=1)
	  Debug::stream() << "EIA422Protocol::processOneCommand: setAzIl("
			  << i << ')' << std::endl;
      }
      break;

    case CMD_SETAZACC:
      {
	CmdDataType data=recvAndDecodeCommandData(3,buffer);
	int i = decodeShort(data);
	setAzAcc(i);
	if(m_data_stream->loud()>=1)
	  Debug::stream() << "EIA422Protocol::processOneCommand: setAzAcc("
			  << i << ')' << std::endl;
      }
      break;

    case CMD_SETAZVEL:
      {
	CmdDataType data=recvAndDecodeCommandData(3,buffer);
	int i = decodeShort(data);
	setAzVel(i);
	if(m_data_stream->loud()>=1)
	  Debug::stream() << "EIA422Protocol::processOneCommand: setAzVel("
			  << i << ')' << std::endl;
      }
      break;

    case CMD_SETELKP:
      {
	CmdDataType data=recvAndDecodeCommandData(3,buffer);
	int i = decodeShort(data);
	setElKp(i);
	if(m_data_stream->loud()>=1)
	  Debug::stream() << "EIA422Protocol::processOneCommand: setElKp("
			  << i << ')' << std::endl;
      }
      break;

    case CMD_SETELKI:
      {
	CmdDataType data=recvAndDecodeCommandData(3,buffer);
	int i = decodeShort(data);
	setElKi(i);
	if(m_data_stream->loud()>=1)
	  Debug::stream() << "EIA422Protocol::processOneCommand: setElKi("
			  << i << ')' << std::endl;
      }
      break;

    case CMD_SETELKD:
      {
	CmdDataType data=recvAndDecodeCommandData(3,buffer);
	int i = decodeShort(data);
	setElKd(i);
	if(m_data_stream->loud()>=1)
	  Debug::stream() << "EIA422Protocol::processOneCommand: setElKd("
			  << i << ')' << std::endl;
      }
      break;

    case CMD_SETELIL:
      {
	CmdDataType data=recvAndDecodeCommandData(3,buffer);
	int i = decodeShort(data);
	setElIl(i);
	if(m_data_stream->loud()>=1)
	  Debug::stream() << "EIA422Protocol::processOneCommand: setElIl("
			  << i << ')' << std::endl;
      }
      break;

    case CMD_SETELACC:
      {
	CmdDataType data=recvAndDecodeCommandData(3,buffer);
	int i = decodeShort(data);
	setElAcc(i);
	if(m_data_stream->loud()>=1)
	  Debug::stream() << "EIA422Protocol::processOneCommand: setElAcc("
			  << i << ')' << std::endl;
      }
      break;

    case CMD_SETELVEL:
      {
	CmdDataType data=recvAndDecodeCommandData(3,buffer);
	int i = decodeShort(data);
	setElVel(i);
	if(m_data_stream->loud()>=1)
	  Debug::stream() << "EIA422Protocol::processOneCommand: setElVel("
			  << i << ')' << std::endl;
      }
      break;
		   
    case CMD_REQAMP:
    case CMD_REQEMP:
      {
	Exception exception("EIA422 protocol error");
	exception.messageStream() 
	  << "Command code " << int(buffer[0]) << " not implemented";
	throw exception;
      }
      break;

    default:
      {
	Exception exception("EIA422 protocol error");
	exception.messageStream() 
	  << "Unknown command code " << int(buffer[0]);
	throw exception;
      }
      break;
    }
}

void EIA422Protocol::resetCommunication()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  try
    {
      datastring buffer;
      do
	{
	  buffer=m_data_stream->recvData(1,0,100000);
	}while(buffer.size() != 0);
    }
  catch(const Timeout&)
    {
    }
}

// ----------------------------------------------------------------------------
// EIA422Stub
// ----------------------------------------------------------------------------

EIA422Stub::~EIA422Stub()
{

}

EIA422ScopeAPI::Status EIA422Stub::reqAzStat()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  encodeAndSendCommand(CMD_AZSTAT,0,0);
  EIA422ScopeAPI::Status status = decodeStatus(recvAndDecodeCommandData(3));
  if(m_data_stream->loud()>=1)
    {
      Debug::stream() << "EIA422Stub::reqAzStat() = ";
      status.print(Debug::stream());
      Debug::stream() << std::endl;
    }
  return status;
}

EIA422ScopeAPI::Status EIA422Stub::reqElStat()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  encodeAndSendCommand(CMD_ELSTAT,0,0);
  EIA422ScopeAPI::Status status = decodeStatus(recvAndDecodeCommandData(3));
  if(m_data_stream->loud()>=1)
    {
      Debug::stream() << "EIA422Stub::reqElStat() = ";
      status.print(Debug::stream());
      Debug::stream() << std::endl;
    }
  return status;
}

SEphem::Angle EIA422Stub::reqAzPos()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  encodeAndSendCommand(CMD_REQAZP,0,0);
  SEphem::Angle angle =
#ifdef USE24BIT
    decodeAngle24(recvAndDecodeCommandData(4));
#else
    decodeAngle16(recvAndDecodeCommandData(3));
#endif
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::reqAzPos() = " << angle.deg() << std::endl;
  return angle;
}

SEphem::Angle EIA422Stub::reqElPos()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  encodeAndSendCommand(CMD_REQELP,0,0);
  SEphem::Angle angle =
#ifdef USE24BIT
    decodeAngle24(recvAndDecodeCommandData(4));
#else
    decodeAngle16(recvAndDecodeCommandData(3));
#endif
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::reqElPos() = " << angle.deg() << std::endl;
  return angle;
}


EIA422ScopeAPI::ExtStatus EIA422Stub::reqAzExtStat()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  encodeAndSendCommand(CMD_AZSTAT2,0,0);
  EIA422ScopeAPI::ExtStatus status = 
    decodeAzExtStatus(recvAndDecodeCommandData(3));
  if(m_data_stream->loud()>=1)
    {
      Debug::stream() << "EIA422Stub::reqAzExtStat() = ";
      status.print(Debug::stream());
      Debug::stream() << std::endl;
    }
  return status;
}

EIA422ScopeAPI::ExtStatus EIA422Stub::reqElExtStat()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  encodeAndSendCommand(CMD_ELSTAT2,0,0);
  EIA422ScopeAPI::ExtStatus status = 
    decodeElExtStatus(recvAndDecodeCommandData(3));
  if(m_data_stream->loud()>=1)
    {
      Debug::stream() << "EIA422Stub::reqElExtStat() = ";
      status.print(Debug::stream());
      Debug::stream() << std::endl;
    }
  return status;

}

void EIA422Stub::cmdElSrvPwrOn()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  encodeAndSendCommand(CMD_ELSRVPWRON,0,0);
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::cmdElSrvPwrOn()" << std::endl;
}

void EIA422Stub::cmdElSrvPwrOff()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  encodeAndSendCommand(CMD_ELSRVPWROFF,0,0);
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::cmdElSrvPwrOff()" << std::endl;
}

void EIA422Stub::cmdAzSrvPwrOn()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  encodeAndSendCommand(CMD_AZSRVPWRON,0,0);
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::cmdAzSrvPwrOn()" << std::endl;
}

void EIA422Stub::cmdAzSrvPwrOff()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  encodeAndSendCommand(CMD_AZSRVPWROFF,0,0);
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::cmdAzSrvPwrOff()" << std::endl;
}

SEphem::Angle EIA422Stub::reqAzOffset()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  encodeAndSendCommand(CMD_AZOFFSETR,0,0);
  SEphem::Angle angle = 
    Angle::sc_Pi-decodeAngle16(recvAndDecodeCommandData(3));
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::reqAzOffset() = " << angle.degPM() 
		    << std::endl;
  return angle;
}

SEphem::Angle EIA422Stub::reqElOffset()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  encodeAndSendCommand(CMD_ELOFFSETR,0,0);
  SEphem::Angle angle = 
    Angle::sc_Pi-decodeAngle16(recvAndDecodeCommandData(3));
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::reqElOffset() = " << angle.degPM() 
		    << std::endl;
  return angle;
}

void EIA422Stub::cmdAzRate(double rate)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  encodeAndSendCommand(CMD_AZRATE,3,encodeRate(rate));
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::cmdAzRate(" << rate << ")" << std::endl;
}

void EIA422Stub::cmdElRate(double rate)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  encodeAndSendCommand(CMD_ELRATE,3,encodeRate(rate));
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::cmdElRate(" << rate << ")" << std::endl;
}

void EIA422Stub::setAzOffset(const SEphem::Angle& offset)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  SEphem::Angle b(Angle::sc_Pi-offset.rad());
  encodeAndSendCommand(CMD_AZOFFSET,3,encodeAngle16(b));
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::setAzOffset(" << offset.degPM() << ")" 
		    << std::endl;
}

void EIA422Stub::setElOffset(const SEphem::Angle& offset)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  SEphem::Angle b(Angle::sc_Pi-offset.rad());
  encodeAndSendCommand(CMD_ELOFFSET,3,encodeAngle16(b));
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::setElOffset(" << offset.degPM() << ")" 
		    << std::endl;
}

int EIA422Stub::reqAzRevision()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  int revision = 0;
  //    CMD_AZREVISION  = 0x25,
#warning UNIMPLEMENTED: EIA422Stub::reqAzRevision()
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::reqAzRevision() = " << revision 
		    << std::endl;
  return 0;
}

int EIA422Stub::reqElRevision()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  int revision = 0;
  //    CMD_ELREVISION  = 0x26,
#warning UNIMPLEMENTED: EIA422Stub::reqElRevision()
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::reqElRevision() = " << revision 
		    << std::endl;
  return 0;
}

void EIA422Stub::cmdLatch1On()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  encodeAndSendCommand(CMD_LATCH1ON,0,0);
}

void EIA422Stub::cmdLatch1Off()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  encodeAndSendCommand(CMD_LATCH1OFF,0,0);
}

void EIA422Stub::cmdLatch2On()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  encodeAndSendCommand(CMD_LATCH2ON,0,0);
}

void EIA422Stub::cmdLatch2Off()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  encodeAndSendCommand(CMD_LATCH2OFF,0,0);
}

void EIA422Stub::cmdAzPosn(const SEphem::Angle& angle)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
#ifdef USE24BIT
  encodeAndSendCommand(CMD_AZPOSN,4,encodeAngle24(angle));
#else
  encodeAndSendCommand(CMD_AZPOSN,3,encodeAngle16(angle));
#endif
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::cmdAzPosn(" << angle.deg() 
		    << ")" << std::endl;
}

void EIA422Stub::cmdElPosn(const SEphem::Angle& angle)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
#ifdef USE24BIT
  encodeAndSendCommand(CMD_ELPOSN,4,encodeAngle24(angle));
#else
  encodeAndSendCommand(CMD_ELPOSN,3,encodeAngle16(angle));
#endif
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::cmdElPosn(" << angle.degPM() 
		    << ")" << std::endl;
}

int EIA422Stub::reqAzKp()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  encodeAndSendCommand(CMD_REQAZKP,0,0);
  int p = decodeShort(recvAndDecodeCommandData(3));
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::reqAzKp() = " << p << std::endl;
  return p;
}

int EIA422Stub::reqAzKi()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  encodeAndSendCommand(CMD_REQAZKI,0,0);
  int p = decodeShort(recvAndDecodeCommandData(3));
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::reqAzKi() = " << p << std::endl;
  return p;
}

int EIA422Stub::reqAzKd()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  encodeAndSendCommand(CMD_REQAZKD,0,0);
  int p = decodeShort(recvAndDecodeCommandData(3));
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::reqAzKd() = " << p << std::endl;
  return p;
}

int EIA422Stub::reqAzIl()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  encodeAndSendCommand(CMD_REQAZIL,0,0);
  int p = decodeShort(recvAndDecodeCommandData(3));
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::reqAzIl() = " << p << std::endl;
  return p;
}

int EIA422Stub::reqAzAcc()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  encodeAndSendCommand(CMD_REQAZACC,0,0);
  int p = decodeShort(recvAndDecodeCommandData(3));
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::reqAzAcc() = " << p << std::endl;
  return p;
}

int EIA422Stub::reqAzVel()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  encodeAndSendCommand(CMD_REQAZVEL,0,0);
  int p = decodeShort(recvAndDecodeCommandData(3));
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::reqAzVel() = " << p << std::endl;
  return p;
}

int EIA422Stub::reqElKp()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  encodeAndSendCommand(CMD_REQELKP,0,0);
  int p = decodeShort(recvAndDecodeCommandData(3));
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::reqElKp() = " << p << std::endl;
  return p;
}

int EIA422Stub::reqElKi()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  encodeAndSendCommand(CMD_REQELKI,0,0);
  int p = decodeShort(recvAndDecodeCommandData(3));
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::reqElKi() = " << p << std::endl;
  return p;
}

int EIA422Stub::reqElKd()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  encodeAndSendCommand(CMD_REQELKD,0,0);
  int p = decodeShort(recvAndDecodeCommandData(3));
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::reqElKd() = " << p << std::endl;
  return p;
}

int EIA422Stub::reqElIl()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  encodeAndSendCommand(CMD_REQELIL,0,0);
  int p = decodeShort(recvAndDecodeCommandData(3));
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::reqElIl() = " << p << std::endl;
  return p;
}

int EIA422Stub::reqElAcc()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  encodeAndSendCommand(CMD_REQELACC,0,0);
  int p = decodeShort(recvAndDecodeCommandData(3));
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::reqElAcc() = " << p << std::endl;
  return p;
}

int EIA422Stub::reqElVel()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  encodeAndSendCommand(CMD_REQELVEL,0,0);
  int p = decodeShort(recvAndDecodeCommandData(3));
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::reqElVel() = " << p << std::endl;
  return p;
}

void EIA422Stub::setAzKp(int i)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(i<0)i=0;
  else if(i>=65536)i=65536;
  encodeAndSendCommand(CMD_SETAZKP,3,encodeShort(i));
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::setAzKp(" << i << ")" << std::endl;
}

void EIA422Stub::setAzKi(int i)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(i<0)i=0;
  else if(i>=65536)i=65536;
  encodeAndSendCommand(CMD_SETAZKI,3,encodeShort(i));
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::setAzKi(" << i << ")" << std::endl;
}

void EIA422Stub::setAzKd(int i)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(i<0)i=0;
  else if(i>=65536)i=65536;
  encodeAndSendCommand(CMD_SETAZKD,3,encodeShort(i));
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::setAzKd(" << i << ")" << std::endl;
}

void EIA422Stub::setAzIl(int i)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(i<0)i=0;
  else if(i>=65536)i=65536;
  encodeAndSendCommand(CMD_SETAZIL,3,encodeShort(i));
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::setAzIl(" << i << ")" << std::endl;
}

void EIA422Stub::setAzAcc(int i)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(i<0)i=0;
  else if(i>=65536)i=65536;
  encodeAndSendCommand(CMD_SETAZACC,3,encodeShort(i));
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::setAzAcc(" << i << ")" << std::endl;
}

void EIA422Stub::setAzVel(int i)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(i<0)i=0;
  else if(i>=65536)i=65536;
  encodeAndSendCommand(CMD_SETAZVEL,3,encodeShort(i));
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::setAzVel(" << i << ")" << std::endl;
}

void EIA422Stub::setElKp(int i)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(i<0)i=0;
  else if(i>=65536)i=65536;
  encodeAndSendCommand(CMD_SETELKP,3,encodeShort(i));
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::setElKp(" << i << ")" << std::endl;
}

void EIA422Stub::setElKi(int i)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(i<0)i=0;
  else if(i>=65536)i=65536;
  encodeAndSendCommand(CMD_SETELKI,3,encodeShort(i));
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::setElKi(" << i << ")" 
			 << std::endl;
}

void EIA422Stub::setElKd(int i)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(i<0)i=0;
  else if(i>=65536)i=65536;
  encodeAndSendCommand(CMD_SETELKD,3,encodeShort(i));
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::setElKd(" << i << ")" << std::endl;
}

void EIA422Stub::setElIl(int i)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(i<0)i=0;
  else if(i>=65536)i=65536;
  encodeAndSendCommand(CMD_SETELIL,3,encodeShort(i));
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::setElIl(" << i << ")" << std::endl;
}

void EIA422Stub::setElAcc(int i)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(i<0)i=0;
  else if(i>=65536)i=65536;
  encodeAndSendCommand(CMD_SETELACC,3,encodeShort(i));
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::setElAcc(" << i << ")"  << std::endl;
}

void EIA422Stub::setElVel(int i)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(i<0)i=0;
  else if(i>=65536)i=65536;
  encodeAndSendCommand(CMD_SETELVEL,3,encodeShort(i));
  if(m_data_stream->loud()>=1)
    Debug::stream() << "EIA422Stub::setElVel(" << i << ")" << std::endl;
}

std::string EIA422Stub::apiName() const
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  return std::string("EIA ")+m_data_stream->udsl();
}

// ----------------------------------------------------------------------------
// ScopeAPIToEIA422Adaptor
// ----------------------------------------------------------------------------

ScopeAPIToEIA422Adaptor::~ScopeAPIToEIA422Adaptor()
{
  // nothing to see here
}

void ScopeAPIToEIA422Adaptor::
reqStat(ScopeAPI::PositionerStatus& state)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  state.az.driveangle_deg       = m_eia422_api->reqAzPos().deg();
  state.el.driveangle_deg       = m_eia422_api->reqElPos().degPM();
  EIA422ScopeAPI::Status azstat = m_eia422_api->reqAzStat();
  EIA422ScopeAPI::Status elstat = m_eia422_api->reqElStat();
  EIA422ScopeAPI::ExtStatus azextstat = m_eia422_api->reqAzExtStat();
  EIA422ScopeAPI::ExtStatus elextstat = m_eia422_api->reqElExtStat();
  

  if(((azstat.directionSignBit)&&(state.az.driveangle_deg>45))||
     (state.az.driveangle_deg>315))state.az.driveangle_deg -= 360;

  if(state.el.driveangle_deg>180)state.el.driveangle_deg -= 360;

  // AZIMUTH DRIVE STATUS
  switch(azstat.servoMode)
    { 
    case EIA422ScopeAPI::SM_IDLE:     state.az.driveMode=DM_STANDBY; break;
    case EIA422ScopeAPI::SM_RATE:     state.az.driveMode=DM_SLEW;    break;
    case EIA422ScopeAPI::SM_POSITION: state.az.driveMode=DM_POINT;   break;
    }
  state.az.servo1Fail       = azstat.servoAmpFault;
  state.az.servo2Fail       = azstat.servoAmpFault;
  state.az.servoOn          = azstat.servoOn;
  state.az.brakeReleased    = azstat.brakeReleased;
  state.az.limitCwUp        = azstat.limitCwUp;
  state.az.limitCcwDown     = azstat.limitCcwDown;
  state.az.positionFault    = azstat.positionFault;
  state.az.positionComplete = azstat.positionComplete;
  
  // ELEVATION DRIVE STATUS
  switch(elstat.servoMode)
    { 
    case EIA422ScopeAPI::SM_IDLE:     state.el.driveMode=DM_STANDBY; break;
    case EIA422ScopeAPI::SM_RATE:     state.el.driveMode=DM_SLEW;    break;
    case EIA422ScopeAPI::SM_POSITION: state.el.driveMode=DM_POINT;   break;
    }
  state.el.servo1Fail       = elstat.servoAmpFault;
  state.el.servo2Fail       = elstat.servoAmpFault;
  state.el.servoOn          = elstat.servoOn;
  state.el.brakeReleased    = elstat.brakeReleased;
  state.el.limitCwUp        = elstat.limitCwUp;
  state.el.limitCcwDown     = elstat.limitCcwDown;
  state.el.positionFault    = elstat.positionFault;
  state.el.positionComplete = elstat.positionComplete;

  // POSITIONER STATUS
  //  state.azTravelledCCW      = state.az.driveangle_deg<0;
  //  state.azCableWrap         = state.az.driveangle_deg/270;  
  state.azTravelledCCW      = azstat.directionSignBit;
  state.azCableWrap         = 0;
  state.interlock           = azstat.interlock||elstat.interlock;
  state.interlockAzPullCord = azextstat.interlockPullCord;
  state.interlockAzStowPin  = azextstat.interlockStowPin;
  state.interlockElStowPin  = elextstat.interlockStowPin;
  state.interlockAzDoorOpen = azextstat.interlockHatch;
  state.interlockElDoorOpen = elextstat.interlockHatch;
  state.interlockSafeSwitch = azextstat.interlockSafe;
  state.remoteControl       = azextstat.remoteControl;
  state.checksumOK          = azstat.checksumOK&&elstat.checksumOK;
  state.msgBadFrame         = false;
  state.msgCommandInvalid   =
    azstat.commandUnrecognised||elstat.commandUnrecognised;
  state.msgInputOverrun     =
    azstat.commandUnderOverFlow||elstat.commandUnderOverFlow;
  state.msgOutputOverrun    =
    azstat.commandUnderOverFlow||elstat.commandUnderOverFlow;
  state.relay1              = false;
  state.relay2              = false;
  state.Analog1             = azextstat.ADCValue;
  state.Analog2             = elextstat.ADCValue;
}

void ScopeAPIToEIA422Adaptor::
cmdStandby()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  m_eia422_api->cmdAzSrvPwrOff();
  m_eia422_api->cmdElSrvPwrOff();
}

void ScopeAPIToEIA422Adaptor::
cmdPoint(const SEphem::Angle& az_angle, double az_vel,
	 const SEphem::Angle& el_angle, double el_vel)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  m_eia422_api->cmdAzPosn(az_angle);
  m_eia422_api->cmdElPosn(el_angle);
}

void ScopeAPIToEIA422Adaptor::
cmdSlew(double az_vel, double el_vel)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  m_eia422_api->cmdAzRate(az_vel/EIA422ScopeAPI::sc_az_vel_ratio);
  m_eia422_api->cmdElRate(el_vel/EIA422ScopeAPI::sc_el_vel_ratio);
}

void ScopeAPIToEIA422Adaptor::
reqAzOffset(SEphem::Angle& az_angle)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  az_angle = m_eia422_api->reqAzOffset();
}

void ScopeAPIToEIA422Adaptor::
reqElOffset(SEphem::Angle& el_angle)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  el_angle = m_eia422_api->reqElOffset();
}

void ScopeAPIToEIA422Adaptor::
setAzOffset(const SEphem::Angle& az_angle)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  m_eia422_api->setAzOffset(az_angle);
}

void ScopeAPIToEIA422Adaptor::
setElOffset(const SEphem::Angle& el_angle)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  m_eia422_api->setElOffset(el_angle);
}

void ScopeAPIToEIA422Adaptor::
reqAzPIDParameters(PIDParameters& az_param)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  az_param.Kp   = m_eia422_api->reqAzKp();
  az_param.Ki   = m_eia422_api->reqAzKi();
  az_param.Kd   = m_eia422_api->reqAzKd();
  az_param.Kvff = 0;
  az_param.Ilim = m_eia422_api->reqAzIl();
  az_param.vlim = double(m_eia422_api->reqAzVel())/7819*0.4;;
  az_param.alim = double(m_eia422_api->reqAzAcc())/5.0*0.16;
}

void ScopeAPIToEIA422Adaptor::
reqElPIDParameters(PIDParameters& el_param)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  el_param.Kp   = m_eia422_api->reqElKp();
  el_param.Ki   = m_eia422_api->reqElKi();
  el_param.Kd   = m_eia422_api->reqElKd();
  el_param.Kvff = 0;
  el_param.Ilim = m_eia422_api->reqElIl();
  el_param.vlim = double(m_eia422_api->reqElVel())/7819*0.4;
  el_param.alim = double(m_eia422_api->reqElAcc())/5.0*0.16;
}

void ScopeAPIToEIA422Adaptor::
setAzPIDParameters(const PIDParameters& az_param)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  m_eia422_api->setAzKp(az_param.Kp);
  m_eia422_api->setAzKi(az_param.Ki);
  m_eia422_api->setAzKd(az_param.Kd);
  //az_param.Kvff;
  m_eia422_api->setAzIl(az_param.Ilim);
  m_eia422_api->setAzVel(int(floor(az_param.vlim/0.4*7819)));
  m_eia422_api->setAzAcc(int(floor(az_param.alim/0.16*5)));
}

void ScopeAPIToEIA422Adaptor::
setElPIDParameters(const PIDParameters& el_param)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  m_eia422_api->setElKp(el_param.Kp);
  m_eia422_api->setElKi(el_param.Ki);
  m_eia422_api->setElKd(el_param.Kd);
  //el_param.Kvff;
  m_eia422_api->setElIl(el_param.Ilim);
  m_eia422_api->setElVel(int(floor(el_param.vlim/0.4*7819)));
  m_eia422_api->setElAcc(int(floor(el_param.alim/0.16*5)));
}


#if 0
void ScopeAPIToEIA422Adaptor::
setOffsets(const SEphem::Angle& az_angle, const SEphem::Angle& el_angle)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  m_eia422_api->setAzOffset(az_angle);
  m_eia422_api->setElOffset(el_angle);
}

void ScopeAPIToEIA422Adaptor::
reqOffsets(SEphem::Angle& az_angle, SEphem::Angle& el_angle)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  az_angle = m_eia422_api->reqAzOffset();
  el_angle = m_eia422_api->reqElOffset();
}

void ScopeAPIToEIA422Adaptor::
reqPIDParameters(PIDParameters& az_param, PIDParameters& el_param)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  az_param.Kp   = m_eia422_api->reqAzKp();
  az_param.Ki   = m_eia422_api->reqAzKi();
  az_param.Kd   = m_eia422_api->reqAzKd();
  az_param.Kvff = 0;
  az_param.Ilim = m_eia422_api->reqAzIl();
  az_param.vlim = double(m_eia422_api->reqAzVel())/7819*0.4;;
  az_param.alim = double(m_eia422_api->reqAzAcc())/5.0*0.16;
  el_param.Kp   = m_eia422_api->reqElKp();
  el_param.Ki   = m_eia422_api->reqElKi();
  el_param.Kd   = m_eia422_api->reqElKd();
  el_param.Kvff = 0;
  el_param.Ilim = m_eia422_api->reqElIl();
  el_param.vlim = double(m_eia422_api->reqElVel())/7819*0.4;
  el_param.alim = double(m_eia422_api->reqElAcc())/5.0*0.16;
}

void ScopeAPIToEIA422Adaptor::
setPIDParameters(const PIDParameters& az_param, const PIDParameters& el_param)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  m_eia422_api->setAzKp(az_param.Kp);
  m_eia422_api->setAzKi(az_param.Ki);
  m_eia422_api->setAzKd(az_param.Kd);
  //az_param.Kvff;
  m_eia422_api->setAzIl(az_param.Ilim);
  m_eia422_api->setAzVel(int(floor(az_param.vlim/0.4*7819)));
  m_eia422_api->setAzAcc(int(floor(az_param.alim/0.16*5)));
  m_eia422_api->setElKp(el_param.Kp);
  m_eia422_api->setElKi(el_param.Ki);
  m_eia422_api->setElKd(el_param.Kd);
  //el_param.Kvff;
  m_eia422_api->setElIl(el_param.Ilim);
  m_eia422_api->setElVel(int(floor(el_param.vlim/0.4*7819)));
  m_eia422_api->setElAcc(int(floor(el_param.alim/0.16*5)));
}
#endif

void ScopeAPIToEIA422Adaptor::resetCommunication()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  m_eia422_api->resetCommunication();
}

std::string ScopeAPIToEIA422Adaptor::apiName() const
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  return m_eia422_api->apiName();
}

// ----------------------------------------------------------------------------
// EIA422ToScopeAPIAdaptor
// ----------------------------------------------------------------------------

EIA422ToScopeAPIAdaptor::~EIA422ToScopeAPIAdaptor()
{
  // nothing to see here
}

EIA422ScopeAPI::Status EIA422ToScopeAPIAdaptor::reqAzStat()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ScopeAPI::PositionerStatus pstatus;
  m_scope_api->reqStat(pstatus);
  EIA422ScopeAPI::Status azstatus;

  azstatus.limitCwUp             = pstatus.az.limitCwUp;
  azstatus.limitCcwDown          = pstatus.az.limitCcwDown;
  azstatus.brakeReleased         = pstatus.az.brakeReleased;
  azstatus.positionComplete      = pstatus.az.positionComplete;
  azstatus.interlock             = 
    pstatus.interlock||pstatus.interlockAzPullCord||
    pstatus.interlockAzStowPin||pstatus.interlockElStowPin||
    pstatus.interlockAzDoorOpen||pstatus.interlockElDoorOpen||
    pstatus.interlockSafeSwitch;
  azstatus.positionFault         = 
    pstatus.az.positionFault||pstatus.el.positionFault;
  azstatus.servoAmpFault         = 
    pstatus.az.servo1Fail||pstatus.az.servo2Fail;
  azstatus.servoOn               = pstatus.az.servoOn;
  switch(pstatus.az.driveMode)
    {
    case ScopeAPI::DM_STANDBY:
    case ScopeAPI::DM_SPIN: 
    case ScopeAPI::DM_SECTOR_SCAN: 
    case ScopeAPI::DM_RASTER:
    case ScopeAPI::DM_CHANGING:
    case ScopeAPI::DM_UNKNOWN:
      azstatus.servoMode = SM_IDLE;
      break;
    case ScopeAPI::DM_SLEW:
      azstatus.servoMode = SM_RATE;
      break;
    case ScopeAPI::DM_POINT:
      azstatus.servoMode = SM_POSITION;
      break;
    }
  azstatus.commandUnderOverFlow  = 
    pstatus.msgInputOverrun||pstatus.msgOutputOverrun||pstatus.msgBadFrame;
  azstatus.checksumOK            = pstatus.checksumOK;
  azstatus.directionSignBit      = pstatus.az.driveangle_deg<0;
  azstatus.commandUnrecognised   = pstatus.msgCommandInvalid;
  return azstatus;
}

EIA422ScopeAPI::Status EIA422ToScopeAPIAdaptor::reqElStat()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ScopeAPI::PositionerStatus pstatus;
  m_scope_api->reqStat(pstatus);
  EIA422ScopeAPI::Status elstatus;

  elstatus.limitCwUp             = pstatus.el.limitCwUp;
  elstatus.limitCcwDown          = pstatus.el.limitCcwDown;
  elstatus.brakeReleased         = pstatus.el.brakeReleased;
  elstatus.positionComplete      = pstatus.el.positionComplete;
  elstatus.interlock             = 
    pstatus.interlock||pstatus.interlockAzPullCord||
    pstatus.interlockAzStowPin||pstatus.interlockElStowPin||
    pstatus.interlockAzDoorOpen||pstatus.interlockElDoorOpen||
    pstatus.interlockSafeSwitch;
  elstatus.positionFault         =
    pstatus.az.positionFault||pstatus.el.positionFault;
  elstatus.servoAmpFault         = 
    pstatus.el.servo1Fail||pstatus.el.servo2Fail;
  elstatus.servoOn               = pstatus.el.servoOn;
  switch(pstatus.el.driveMode)
    {
    case ScopeAPI::DM_STANDBY:
    case ScopeAPI::DM_SPIN: 
    case ScopeAPI::DM_SECTOR_SCAN: 
    case ScopeAPI::DM_RASTER:
    case ScopeAPI::DM_CHANGING:
    case ScopeAPI::DM_UNKNOWN:
      elstatus.servoMode = SM_IDLE;
      break;
    case ScopeAPI::DM_SLEW:
      elstatus.servoMode = SM_RATE;
      break;
    case ScopeAPI::DM_POINT:
      elstatus.servoMode = SM_POSITION;
      break;
    }
  elstatus.commandUnderOverFlow  = 
    pstatus.msgInputOverrun||pstatus.msgOutputOverrun||pstatus.msgBadFrame;
  elstatus.checksumOK            = pstatus.checksumOK;
  elstatus.directionSignBit      = pstatus.el.driveangle_deg<0;
  elstatus.commandUnrecognised   = pstatus.msgCommandInvalid;

  return elstatus;
}

SEphem::Angle EIA422ToScopeAPIAdaptor::reqAzPos()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ScopeAPI::PositionerStatus pstatus;
  m_scope_api->reqStat(pstatus);
  return SEphem::Angle::makeDeg(pstatus.az.driveangle_deg);
}

SEphem::Angle EIA422ToScopeAPIAdaptor::reqElPos()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ScopeAPI::PositionerStatus pstatus;
  m_scope_api->reqStat(pstatus);
  return SEphem::Angle::makeDeg(pstatus.el.driveangle_deg);
}

EIA422ScopeAPI::ExtStatus EIA422ToScopeAPIAdaptor::reqAzExtStat()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ScopeAPI::PositionerStatus pstatus;
  m_scope_api->reqStat(pstatus);
  EIA422ScopeAPI::ExtStatus azstatus;
  azstatus.remoteControl = pstatus.remoteControl;
  azstatus.ADCValue = pstatus.Analog1;
  azstatus.interlockStowPin = pstatus.interlockAzStowPin;
  azstatus.interlockPullCord = pstatus.interlockAzPullCord;
  azstatus.interlockHatch = pstatus.interlockAzDoorOpen;
  azstatus.interlockSafe = pstatus.interlockSafeSwitch;
  return azstatus;
}

EIA422ScopeAPI::ExtStatus EIA422ToScopeAPIAdaptor::reqElExtStat()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ScopeAPI::PositionerStatus pstatus;
  m_scope_api->reqStat(pstatus);
  EIA422ScopeAPI::ExtStatus elstatus;
  elstatus.remoteControl = 0;
  elstatus.ADCValue = pstatus.Analog2;
  elstatus.interlockStowPin = pstatus.interlockElStowPin;
  elstatus.interlockPullCord = false;
  elstatus.interlockHatch = pstatus.interlockElDoorOpen;
  elstatus.interlockSafe = pstatus.interlockSafeSwitch;
  return elstatus;
}

void EIA422ToScopeAPIAdaptor::cmdElSrvPwrOn()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ScopeAPI::PositionerStatus pstatus;
  m_scope_api->reqStat(pstatus);
  if((pstatus.az.driveMode==ScopeAPI::DM_STANDBY)&&
     (pstatus.el.driveMode==ScopeAPI::DM_STANDBY))
    m_scope_api->cmdSlew(0, 0);
}

void EIA422ToScopeAPIAdaptor::cmdElSrvPwrOff()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  m_scope_api->cmdStandby();
}

void EIA422ToScopeAPIAdaptor::cmdAzSrvPwrOn()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ScopeAPI::PositionerStatus pstatus;
  m_scope_api->reqStat(pstatus);
  if((pstatus.az.driveMode==ScopeAPI::DM_STANDBY)&&
     (pstatus.el.driveMode==ScopeAPI::DM_STANDBY))
    m_scope_api->cmdSlew(0, 0);
}

void EIA422ToScopeAPIAdaptor::cmdAzSrvPwrOff()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  m_scope_api->cmdStandby();
}

SEphem::Angle EIA422ToScopeAPIAdaptor::reqAzOffset()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  SEphem::Angle azangle;
  m_scope_api->reqAzOffset(azangle);
  return azangle;
}

SEphem::Angle EIA422ToScopeAPIAdaptor::reqElOffset()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  SEphem::Angle elangle;
  m_scope_api->reqElOffset(elangle);
  return elangle;
}

void EIA422ToScopeAPIAdaptor::cmdAzRate(double r)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(m_stored)
    {
      m_scope_api->cmdSlew(r, m_stored_rate);
      m_stored=false;
    }
  else
    {
      m_stored_rate=r;
      m_stored=true;
    }
}

void EIA422ToScopeAPIAdaptor::cmdElRate(double r)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(m_stored)
    {
      m_scope_api->cmdSlew(m_stored_rate, r);
      m_stored=false;
    }
  else
    {
      m_stored_rate=r;
      m_stored=true;
    }
}

void EIA422ToScopeAPIAdaptor::setAzOffset(const SEphem::Angle& a)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  m_scope_api->setAzOffset(a);
}

void EIA422ToScopeAPIAdaptor::setElOffset(const SEphem::Angle& a)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  m_scope_api->setElOffset(a);
}

int EIA422ToScopeAPIAdaptor::reqAzRevision()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  return 0;
}

int EIA422ToScopeAPIAdaptor::reqElRevision()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  return 0;
}

void EIA422ToScopeAPIAdaptor::cmdLatch1On()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
#warning UNIMPLEMENTED: EIA422ToScopeAPIAdaptor::cmdLatch1On()
}

void EIA422ToScopeAPIAdaptor::cmdLatch1Off()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
#warning UNIMPLEMENTED: EIA422ToScopeAPIAdaptor::cmdLatch1Off()
}

void EIA422ToScopeAPIAdaptor::cmdLatch2On()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
#warning UNIMPLEMENTED: EIA422ToScopeAPIAdaptor::cmdLatch2On()
}

void EIA422ToScopeAPIAdaptor::cmdLatch2Off()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
#warning UNIMPLEMENTED: EIA422ToScopeAPIAdaptor::cmdLatch2Off()
}

void EIA422ToScopeAPIAdaptor::cmdAzPosn(const SEphem::Angle& a)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(m_stored)
    {
      m_scope_api->cmdPoint(a,ScopeAPI::sc_az_max_vel,
			    m_stored_angle,ScopeAPI::sc_az_max_vel);
      m_stored=false;
    }
  else
    {
      m_stored_angle=a;
      m_stored=true;
    }
}

void EIA422ToScopeAPIAdaptor::cmdElPosn(const SEphem::Angle& a)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(m_stored)
    {
      m_scope_api->cmdPoint(m_stored_angle,ScopeAPI::sc_az_max_vel,
			    a,ScopeAPI::sc_az_max_vel);
      m_stored=false;
    }
  else
    {
      m_stored_angle=a;
      m_stored=true;
    }
}

int EIA422ToScopeAPIAdaptor::reqAzKp()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ScopeAPI::PIDParameters azparam;
  m_scope_api->reqAzPIDParameters(azparam);
  return azparam.Kp;
}

int EIA422ToScopeAPIAdaptor::reqAzKi()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ScopeAPI::PIDParameters azparam;
  m_scope_api->reqAzPIDParameters(azparam);
  return azparam.Ki;
}

int EIA422ToScopeAPIAdaptor::reqAzKd()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ScopeAPI::PIDParameters azparam;
  m_scope_api->reqAzPIDParameters(azparam);
  return azparam.Kd;
}

int EIA422ToScopeAPIAdaptor::reqAzIl()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ScopeAPI::PIDParameters azparam;
  m_scope_api->reqAzPIDParameters(azparam);
  return azparam.Ilim;
}

int EIA422ToScopeAPIAdaptor::reqAzAcc()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ScopeAPI::PIDParameters azparam;
  m_scope_api->reqAzPIDParameters(azparam);
  return int(floor(azparam.alim/0.16*5));
}

int EIA422ToScopeAPIAdaptor::reqAzVel()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ScopeAPI::PIDParameters azparam;
  m_scope_api->reqAzPIDParameters(azparam);
  return int(floor(azparam.vlim/0.4*7819));
}

int EIA422ToScopeAPIAdaptor::reqElKp()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ScopeAPI::PIDParameters elparam;
  m_scope_api->reqElPIDParameters(elparam);
  return elparam.Kp;
}

int EIA422ToScopeAPIAdaptor::reqElKi()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ScopeAPI::PIDParameters elparam;
  m_scope_api->reqElPIDParameters(elparam);
  return elparam.Ki;
}

int EIA422ToScopeAPIAdaptor::reqElKd()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ScopeAPI::PIDParameters elparam;
  m_scope_api->reqElPIDParameters(elparam);
  return elparam.Kd;
}

int EIA422ToScopeAPIAdaptor::reqElIl()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ScopeAPI::PIDParameters elparam;
  m_scope_api->reqElPIDParameters(elparam);
  return elparam.Ilim;
}

int EIA422ToScopeAPIAdaptor::reqElAcc()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ScopeAPI::PIDParameters elparam;
  m_scope_api->reqElPIDParameters(elparam);
  return int(floor(elparam.alim/0.16*5));
}

int EIA422ToScopeAPIAdaptor::reqElVel()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ScopeAPI::PIDParameters elparam;
  m_scope_api->reqElPIDParameters(elparam);
  return int(floor(elparam.vlim/0.4*7819));
}

void EIA422ToScopeAPIAdaptor::setAzKp(int i)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ScopeAPI::PIDParameters azparam;
  m_scope_api->reqAzPIDParameters(azparam);
  azparam.Kp=i;
  m_scope_api->setAzPIDParameters(azparam);
}

void EIA422ToScopeAPIAdaptor::setAzKi(int i)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ScopeAPI::PIDParameters azparam;
  m_scope_api->reqAzPIDParameters(azparam);
  azparam.Ki=i;
  m_scope_api->setAzPIDParameters(azparam);
}

void EIA422ToScopeAPIAdaptor::setAzKd(int i)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ScopeAPI::PIDParameters azparam;
  m_scope_api->reqAzPIDParameters(azparam);
  azparam.Kd=i;
  m_scope_api->setAzPIDParameters(azparam);
}

void EIA422ToScopeAPIAdaptor::setAzIl(int i)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ScopeAPI::PIDParameters azparam;
  m_scope_api->reqAzPIDParameters(azparam);
  azparam.Ilim=i;
  m_scope_api->setAzPIDParameters(azparam);
}

void EIA422ToScopeAPIAdaptor::setAzAcc(int i)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ScopeAPI::PIDParameters azparam;
  m_scope_api->reqAzPIDParameters(azparam);
  azparam.alim = double(i)/5.0*0.16;
  m_scope_api->setAzPIDParameters(azparam);
}

void EIA422ToScopeAPIAdaptor::setAzVel(int i)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ScopeAPI::PIDParameters azparam;
  m_scope_api->reqAzPIDParameters(azparam);
  azparam.vlim = double(i)/7819*0.4;
  m_scope_api->setAzPIDParameters(azparam);
}

void EIA422ToScopeAPIAdaptor::setElKp(int i)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ScopeAPI::PIDParameters elparam;
  m_scope_api->reqElPIDParameters(elparam);
  elparam.Kp=i;
  m_scope_api->setElPIDParameters(elparam);
}

void EIA422ToScopeAPIAdaptor::setElKi(int i)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ScopeAPI::PIDParameters elparam;
  m_scope_api->reqElPIDParameters(elparam);
  elparam.Ki=i;
  m_scope_api->setElPIDParameters(elparam);
}

void EIA422ToScopeAPIAdaptor::setElKd(int i)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ScopeAPI::PIDParameters elparam;
  m_scope_api->reqElPIDParameters(elparam);
  elparam.Kd=i;
  m_scope_api->setElPIDParameters(elparam);
}

void EIA422ToScopeAPIAdaptor::setElIl(int i)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ScopeAPI::PIDParameters elparam;
  m_scope_api->reqElPIDParameters(elparam);
  elparam.Ilim=i;
  m_scope_api->setElPIDParameters(elparam);
}

void EIA422ToScopeAPIAdaptor::setElAcc(int i)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ScopeAPI::PIDParameters elparam;
  m_scope_api->reqElPIDParameters(elparam);
  elparam.alim = double(i)/5.0*0.16;
  m_scope_api->setElPIDParameters(elparam);
}

void EIA422ToScopeAPIAdaptor::setElVel(int i)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  ScopeAPI::PIDParameters elparam;
  m_scope_api->reqElPIDParameters(elparam);
  elparam.vlim = double(i)/7819*0.4;
  m_scope_api->setElPIDParameters(elparam);
}

void EIA422ToScopeAPIAdaptor::resetCommunication()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  m_stored = false;
  m_scope_api->resetCommunication();
}

std::string EIA422ToScopeAPIAdaptor::apiName() const
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  return m_scope_api->apiName();
}

#ifdef TESTMAIN
#include<sys/time.h>
#include<zthread/Thread.h>
#include"DataStream.h"

int main(int argc, char** argv)
{
  argc--,argv++;

  try
    {
      SerialPortDataStream ds("/dev/ttyS0",2);
      EIA422Stub api(&ds,0,500000,2);

      api.cmdElRate(.3);
      ZThread::Thread::sleep(500);
      api.reqElPos();

      Debug::stream() << "Az Offset:  " << api.reqAzOffset() << std::endl
		      << "EL Offset:  " << api.reqElOffset() << std::endl;
      
#if 0
      Debug::stream() << "Az Kp:  " << api.reqAzKp() << std::endl
		      << "Az Ki:  " << api.reqAzKi() << std::endl
		      << "Az Kd:  " << api.reqAzKd() << std::endl
		      << "Az Il:  " << api.reqAzIl() << std::endl
		      << "Az Acc: " << api.reqAzAcc() << std::endl
		      << "Az Vel: " << api.reqAzVel() << std::endl
		      << "El Kp:  " << api.reqElKp() << std::endl
		      << "El Ki:  " << api.reqElKi() << std::endl
		      << "El Kd:  " << api.reqElKd() << std::endl
		      << "El Il:  " << api.reqElIl() << std::endl
		      << "El Acc: " << api.reqElAcc() << std::endl
		      << "El Vel: " << api.reqElVel() << std::endl;
#endif
    }
  catch(const Exception& x)
    {
      x.print(Debug::stream());
    }
  catch(const Timeout& x)
    {
      Debug::stream() << "Timeout" << std::endl;
    }
}
#endif

#ifdef TESTMAIN2
#include<sys/time.h>

#include"DataStream.h"

int main(int argc, char** argv)
{
  double rate=1.0;
  double posn=5;

  argc--,argv++;
  if(argc){ std::istringstream(*argv) >> rate; argv++; argc--; }
  if(argc){ std::istringstream(*argv) >> posn; argv++; argc--; }

  try
    {
      SerialPortDataStream ds("/dev/ttyS0",2);
      EIA422Stub api(&ds,0,500000,2);
      
      struct timeval tv0;
      gettimeofday(&tv0,0);
      
      SEphem::Angle el;
      api.cmdElRate(rate);
      do {
	struct timeval tv;
	gettimeofday(&tv,0);
	double t=(tv.tv_sec-tv0.tv_sec)+double(tv.tv_usec-tv0.tv_usec)/1000000;
	el = api.reqElPos();
	Debug::stream() << std::fixed << std::setprecision(4) << t 
			<< ' ' << el.degPM() << std::endl;
	usleep(50000);
      }while(((rate<0)&&(el.degPM()>posn))||((rate>0)&&(el.degPM()<posn)));
      api.cmdElRate(0);
    }
  catch(const Exception& x)
    {
      x.print(Debug::stream());
    }
}
#endif


#ifdef TESTMAIN3
#include<sys/time.h>

#include"DataStream.h"

int main(int argc, char** argv)
{
  double rate=1.0;
  double posn=5;

  argc--,argv++;
  if(argc){ std::istringstream(*argv) >> rate; argv++; argc--; }
  if(argc){ std::istringstream(*argv) >> posn; argv++; argc--; }

  try
    {
      SerialPortDataStream ds("/dev/ttyS0",2);
      EIA422Stub api(&ds,0,500000,2);
      
      struct timeval tv0;
      gettimeofday(&tv0,0);
      
      SEphem::Angle el;
      do {
	el = api.reqAzOffset();
	el = api.reqElOffset();

	usleep(50000);
      }while(0);
      api.cmdElRate(0);
    }
  catch(const Exception& x)
    {
      x.print(Debug::stream());
    }
}
#endif


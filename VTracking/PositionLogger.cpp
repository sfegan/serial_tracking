//-*-mode:c++; mode:font-lock;-*-

/**
 * \file PositionLogger.cpp
 * \ingroup VTracking
 * \brief This is a one-line description of this cpp file.
 *
 * Here is a tedious verbose multi-line description of all
 * the details of the code, more than you would
 * ever want to read. Generally, all the important documentation
 * goes in the .cpp files.
 *
 * Original Author: Stephen Fegan
 * $Author: aune $
 * $Date: 2011/10/06 02:14:28 $
 * $Revision: 2.10 $
 * $Tag$
 *
 **/

#include <iomanip>
#include <sys/time.h>
#include <time.h>
#include <limits>

#include <zthread/Guard.h>

#include <Angle.h>
#include <Astro.h>
#include <Global.h>

#include <Exception.h>
#include <Message.h>
#include <Messenger.h>

#include "TargetObject.h"
#include "PositionLogger.h"

using namespace SEphem;
using namespace VTracking;
using namespace VTaskNotification;
using namespace VMessaging;

// ----------------------------------------------------------------------------
// Position Logger Base Class
// ----------------------------------------------------------------------------

PositionLogger::~PositionLogger()
{
  // nothing to see here
}

// ----------------------------------------------------------------------------
// FilePositionLogger
// ----------------------------------------------------------------------------

FilePositionLogger::~FilePositionLogger()
{
  if(m_position_stream)
    {
      delete m_position_stream;
      m_position_stream = 0;
    }
}

void FilePositionLogger::
logCommand(Command cmd, double mjd,  const SEphem::Angle& lmst, 
	   const SEphem::SphericalCoords& earthPos, 
	   const TargetObject* object)
{
  // nothing to see here
}

void FilePositionLogger::
logStatus(const TelescopeController::StateElements& tse)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  
  if((tse.state == TelescopeController::TS_STOP)&&(m_close_on_standby == true))
    {
      if(m_position_stream)
	{
	  delete m_position_stream;
	  m_position_stream = 0;
	}
      return;
    }

  if(!m_position_stream)
    m_position_stream = new std::ofstream(m_position_file.c_str());

  if(m_position_stream)
    {
      (*m_position_stream) 
	<< std::setw(10) << std::setprecision(10) 
	<< tse.tv.tv_sec << "  " 
	<< std::setw(7) << std::setprecision(7) 
	<< tse.tv.tv_usec << "  " 
	<< std::fixed << std::setw(14) << std::setprecision(7) 
	<< tse.mjd << "  "	<< std::setw(9) << std::setprecision(4) 
	<< tse.status.az.driveangle_deg << "  "
	<< std::setw(9) << std::setprecision(4) 
	<< tse.status.el.driveangle_deg << "  "
	<< std::setw(9) << std::setprecision(4) 
	<< tse.cmd_az_driveangle_deg << "  "
	<< std::setw(9) << std::setprecision(4) 
	<< tse.cmd_el_driveangle_deg
	<< std::endl;
    }
}

// ----------------------------------------------------------------------------
// DBPoitionLogger
// ----------------------------------------------------------------------------

DBPositionLogger::~DBPositionLogger()
{
  if(!m_cache.empty())
    {
      Task* task = new DBFlushTask(m_scope_id,m_cache);
      m_tasklist->scheduleTask(task);
      m_cache.clear();
    }
}

void DBPositionLogger::
logCommand(Command cmd, double mjd, const SEphem::Angle& lmst, 
	   const SEphem::SphericalCoords& earthPos, 
	   const TargetObject* object)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  if(object==0)return;

  VDBPOS::TargetInfo info;
  info.mode           = "";
  info.angle1         = 0;
  info.angle2         = 0;
  info.epoch          = 0;
  info.source_id      = "";
  info.target_mode    = "";
  info.target_param1  = 0;
  info.target_param2  = 0;
  info.target_param3  = 0;
  info.config_mask    = 0;
  info.pointing_mode  = "parallel";
  info.pointing_param = 0;

  const RaDecObject* radec = dynamic_cast<const RaDecObject*>(object);
  if(radec)
    {
      SEphem::SphericalCoords c = radec->getDBReportedRaDec(mjd,lmst,earthPos);

      info.mode               = "tracking";
      info.angle1             = c.longitudeRad();
      info.angle2             = c.latitudeRad();
      info.epoch              = Astro::mjdToJulianEpoch(mjd);
      info.source_id          = radec->name();

      info.target_mode        = "on";

      const CoordinateOffset* off = radec->offset();
      bool off_set = false;
      if(off == 0)
	off_set = true;

      if(!off_set)
	{
	  const OnOffOffset* onoff = dynamic_cast<const OnOffOffset*>(off);
	  if(onoff)
	    {
	      if(onoff->getOffsetTime().rad() != 0)
		{
		  info.target_mode    = "off";
		  info.target_param1  = onoff->getOffsetTime().radPM();
		}
	      off_set = true;
	    }
	}

      if(!off_set)
	{
	  const WobbleOffset* wobble = dynamic_cast<const WobbleOffset*>(off);
	  if(wobble)
	    {
	      if(wobble->getWobbleCoords().thetaRad() != 0)
		{
		  info.target_mode    = "wobble";
		  info.target_param1  = wobble->getWobbleCoords().thetaRad();
		  SEphem::Angle a = M_PI - wobble->getWobbleCoords().phi();
		  info.target_param2  = a.rad();
		}
	      off_set = true;
	    }
	}

      if(!off_set)
	{
	  const OrbitOffset* orbit = dynamic_cast<const OrbitOffset*>(off);
	  if(orbit)
	    {
	      if(orbit->getOrbitCoords().thetaRad() != 0)
		{
		  info.target_mode    = "orbit";
		  info.target_param1 = orbit->getOrbitCoords().thetaRad();
		  SEphem::Angle a = M_PI - orbit->getOrbitCoords().phiRad();
		  info.target_param2 = a.rad();
		  info.target_param3 = orbit->getOffsetPeriodDay();
		}
	      off_set = true;
	    }
	}

      if(!off_set)
	{
	  const ElAzOffset* elaz = dynamic_cast<const ElAzOffset*>(off);
	  if(elaz)
	    {
	      if(elaz->getElAzCoords().thetaRad() != 0 || elaz->getElAzCoords().phiRad() != 0)
		{
		  info.target_mode    = "elazOffset";
		  info.target_param1  = elaz->getElAzCoords().thetaRad();
		  SEphem::Angle a = M_PI - elaz->getElAzCoords().phi();
		  info.target_param2  = a.rad();
		}
	      off_set = true;
	    }
	}

      assert(off_set);

      Task* task = new DBCmdTask(m_scope_id,info);
      m_tasklist->scheduleTask(task);
      return;
    }

  const FixedObject* azel = dynamic_cast<const FixedObject*>(object);
  if(azel)
    {
      info.mode               = "fixed";
      info.angle1             = azel->coords().longitudeRad();
      info.angle2             = azel->coords().latitudeRad();
      if(cmd==C_STOP)
	info.source_id        = "stop";
      else
	info.source_id        = "drift";

      Task* task = new DBCmdTask(m_scope_id,info);
      m_tasklist->scheduleTask(task);
      return;
    }

  return;
}

void DBPositionLogger::
logStatus(const TelescopeController::StateElements& tse)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  m_iter++;
  if(m_iter == m_period_iter)m_iter=0;
  else return;

  const char* modestring;
  switch(tse.state)
    {
    case TelescopeController::TS_STOP:
    case TelescopeController::TS_RAMP_DOWN:
      modestring = "fixed";
      break;
    case TelescopeController::TS_SLEW:
    case TelescopeController::TS_RESTRICTED_MOTION:
      modestring = "slewing";
      break;
    case TelescopeController::TS_TRACK:
      modestring = "tracking";
      break;
    case TelescopeController::TS_COM_FAILURE:
      // what do we do here ?? probably better to put nothing into the DB than
      // put something that is just wrong
      return;
      break;
    }

  // round time to nearest millisecond
  time_t time_s = tse.tv.tv_sec + (tse.tv.tv_usec+500)/1000000;
  unsigned time_ms = (tse.tv.tv_usec+500)/1000 % 1000;
  struct tm* time_tm = gmtime(&time_s);

  VDBPOS::StatusInfo info;
  info.timestamp        = 
    (time_tm->tm_year+1900) * 10000000000000ULL
    +(time_tm->tm_mon+1)    *   100000000000ULL
    +(time_tm->tm_mday)     *     1000000000ULL
    +(time_tm->tm_hour)     *       10000000ULL
    +(time_tm->tm_min)      *         100000ULL
    +(time_tm->tm_sec)      *           1000ULL
    +time_ms;
  info.mode             = modestring;
  info.elevation_raw    = tse.status.el.driveangle_deg/180.0*M_PI;
  info.azimuth_raw      = tse.status.az.driveangle_deg/180.0*M_PI;
  info.elevation_meas   = tse.tel_azel.latitudeRad();
  info.azimuth_meas     = tse.tel_azel.longitudeRad();
  info.elevation_target = tse.obj_azel.latitudeRad();
  info.azimuth_target   = tse.obj_azel.longitudeRad();

  Task* task = new DBLogTask(this,info);
  m_tasklist->scheduleTask(task);
}

DBPositionLogger::DBLogTask::~DBLogTask()
{
  // nothing to see here
}

void DBPositionLogger::DBLogTask::doTask()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  m_logger->m_cache.push_back(m_info);
  if(m_logger->m_cache.size() == m_logger->m_cache_size)
    {
      try
	{
	  ZThread::Guard<ZThread::RecursiveMutex> 
	    guard(Global::instance()->dbMutex());
	  VDBPOS::putStatus(m_logger->m_scope_id, m_logger->m_cache);
	}
      catch(const VDBException& x)
	{
	  Message message(Message::DR_GLOBAL,Message::PS_EXCEPTIONAL,
			  "VDB exception");
	  message.messageStream() 
	    << "A database exception was thrown and caught. Some\n"
	    << "tracking information was probably lost. Refer to\n" 
	    << "the details tab for further information about the\n"
	    << "exception. If this occurs frequently, it may indicate\n"
	    << "an error with the database server. You can choose to\n"
	    << "ignore this message for a long period of time\n"
	    << " (e.g. 36000 seconds).";
	  message.detailsStream() << x;
	  Messenger::relay()->sendMessage(message);
	}

      m_logger->m_cache.clear();
    }
}

DBPositionLogger::DBFlushTask::~DBFlushTask()
{
  // nothing to see here
}

void DBPositionLogger::DBFlushTask::doTask()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  if(m_cache.size() > 0)
    {
      try
	{
	  ZThread::Guard<ZThread::RecursiveMutex> 
	    guard(Global::instance()->dbMutex());
	  VDBPOS::putStatus(m_scope_id, m_cache);
	}
      catch(const VDBException& x)
	{
	  Message message(Message::DR_GLOBAL,Message::PS_EXCEPTIONAL,
			  "VDB exception");
	  message.messageStream() 
	    << "A database exception was thrown and caught. Some\n"
	    << "tracking information was probably lost. Refer to\n" 
	    << "the details tab for further information about the\n"
	    << "exception. If this occurs frequently, it may indicate\n"
	    << "an error with the database server. You can choose to\n"
	    << "ignore this message for a long period of time\n"
	    << " (e.g. 36000 seconds).";
	  message.detailsStream() << x;
	  Messenger::relay()->sendMessage(message);
	}
    }
}

DBPositionLogger::DBCmdTask::~DBCmdTask()
{
  // nothing to see here
}

void DBPositionLogger::DBCmdTask::doTask()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  unsigned itry = 0;

  while(1)
    {
      try
	{
	  ZThread::Guard<ZThread::RecursiveMutex> 
	    guard(Global::instance()->dbMutex());
	  VDBPOS::putTarget(m_scope_id, m_info);
	  return;
	}
      catch(const VDBException& x)
	{
	  if(itry==4)
	    {
	      Message message(Message::DR_GLOBAL,Message::PS_ROUTINE,
			      "VDB exception");
	      message.messageStream() 
		<< "A database exception was thrown and caught. Some\n"
		<< "tracking information was probably lost. Refer to\n" 
		<< "the details tab for further information about the\n"
		<< "exception. If this occurs frequently, it may indicate\n"
		<< "an error with the database server. You can choose to\n"
		<< "ignore this message for a long period of time\n"
		<< " (e.g. 36000 seconds).";
	      message.detailsStream() << x;
	      Messenger::relay()->sendMessage(message);
	      return;
	    }
	}
      usleep(500000);
      itry++;
    }
}

// ----------------------------------------------------------------------------
// MultiPositionLogger
// ----------------------------------------------------------------------------

MultiPositionLogger::~MultiPositionLogger()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  for(std::set<PositionLogger*>::iterator ilogger=m_loggers.begin();
      ilogger!=m_loggers.end(); ilogger++)delete *ilogger;
}

void MultiPositionLogger::
logCommand(Command cmd, double mjd,  const SEphem::Angle& lmst, 
	   const SEphem::SphericalCoords& earthPos, 
	   const TargetObject* object)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  for(std::set<PositionLogger*>::iterator ilogger=m_loggers.begin();
      ilogger!=m_loggers.end(); ilogger++)
    (*ilogger)->logCommand(cmd,mjd,lmst,earthPos,object);
}

void MultiPositionLogger::
logStatus(const TelescopeController::StateElements& tse)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  for(std::set<PositionLogger*>::iterator ilogger=m_loggers.begin();
      ilogger!=m_loggers.end(); ilogger++)
    (*ilogger)->logStatus(tse);
}

void MultiPositionLogger::addLogger(PositionLogger* logger) 
{
  m_loggers.insert(logger); 
}

void MultiPositionLogger::removeLogger(PositionLogger* logger)
{
  std::set<PositionLogger*>::iterator ilog = m_loggers.find(logger);
  if(ilog != m_loggers.end())m_loggers.erase(ilog);
}

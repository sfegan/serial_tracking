//-*-mode:c++; mode:font-lock;-*-

/**
 * \file PositionLogger.h
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
 * $Revision: 2.3 $
 * $Tag$
 *
 **/

#ifndef VTRACKING_POSITIONLOGGER_H
#define VTRACKING_POSITIONLOGGER_H

#include<set>
#include<string>
#include<fstream>

#include<VDB/VDBPositioner.h>
#include<Task.h>

#include"TelescopeController.h"

namespace VTracking 
{  

  class PositionLogger
  {
  public:
    enum Command { C_STOP, C_SLEW, C_TRACK };
    virtual ~PositionLogger();
    virtual void logCommand(Command cmd, double mjd, const SEphem::Angle& lmst, 
			    const SEphem::SphericalCoords& earthPos, 
			    const TargetObject* object = 0) = 0;
    virtual void logStatus(const TelescopeController::StateElements& tse) = 0;
  };

  class FilePositionLogger: public PositionLogger
  {
  public:
    FilePositionLogger(const std::string filename, 
		       bool close_on_standby = true):
      PositionLogger(),
      m_close_on_standby(close_on_standby), m_position_file(filename),
      m_position_stream(0) { /* nothing to see here */ }
    virtual ~FilePositionLogger();
    virtual void logCommand(Command cmd, double mjd, const SEphem::Angle& lmst, 
			    const SEphem::SphericalCoords& earthPos, 
			    const TargetObject* object = 0);
    virtual void logStatus(const TelescopeController::StateElements& tse);
  private:
    bool           m_close_on_standby;
    std::string    m_position_file;
    std::ofstream* m_position_stream;
  };

  class DBPositionLogger: public PositionLogger
  {
  public:
    DBPositionLogger(VTaskNotification::TaskList* tasklist,
		     unsigned scope_id, unsigned period_iter = 1,
		     unsigned cache_size = 10):
      PositionLogger(),
      m_scope_id(scope_id), m_period_iter(period_iter), 
      m_cache_size(cache_size), m_iter(0), m_cache(),
      m_tasklist(tasklist)
    { assert(period_iter>0); assert(cache_size>0); 
      m_cache.reserve(cache_size); }
    virtual ~DBPositionLogger();
    virtual void logCommand(Command cmd, double mjd, const SEphem::Angle& lmst, 
			    const SEphem::SphericalCoords& earthPos, 
			    const TargetObject* object = 0);
    virtual void logStatus(const TelescopeController::StateElements& tse);
  private:
    
    class DBLogTask: public VTaskNotification::Task
    {
    public:
      DBLogTask(DBPositionLogger* logger, const VDBPOS::StatusInfo& info):
	Task(), m_logger(logger), m_info(info) { /* nothing to see here */ }
      virtual ~DBLogTask();
      virtual void doTask();
    private:
      DBPositionLogger*  m_logger;
      VDBPOS::StatusInfo m_info;
    };

    class DBFlushTask: public VTaskNotification::Task
    {
    public:
      DBFlushTask(unsigned scope_id, std::vector<VDBPOS::StatusInfo> cache):
	Task(), m_scope_id(scope_id), m_cache(cache)
      { /* nothing to see here */ }
      virtual ~DBFlushTask();
      virtual void doTask();
    private:
      unsigned                         m_scope_id;
      std::vector<VDBPOS::StatusInfo>  m_cache;
    };

    class DBCmdTask: public VTaskNotification::Task
    {
    public:
      DBCmdTask(unsigned scope_id, const VDBPOS::TargetInfo& info):
	Task(), m_scope_id(scope_id), m_info(info)
      { /* nothing to see here */ }
      virtual ~DBCmdTask();
      virtual void doTask();
    private:
      unsigned                       m_scope_id;
      VDBPOS::TargetInfo             m_info;
    };

    friend class DBLogTask;

    unsigned                         m_scope_id;
    unsigned                         m_period_iter;
    unsigned                         m_cache_size;
    unsigned                         m_iter;
    std::vector<VDBPOS::StatusInfo>  m_cache;

    VTaskNotification::TaskList*     m_tasklist;
  };

  class MultiPositionLogger: public PositionLogger
  {
  public:
    MultiPositionLogger(): 
      PositionLogger(), m_loggers() { /* nothing to see here */ }
    virtual ~MultiPositionLogger();
    virtual void logCommand(Command cmd, double mjd, const SEphem::Angle& lmst, 
			    const SEphem::SphericalCoords& earthPos, 
			    const TargetObject* object = 0);
    virtual void logStatus(const TelescopeController::StateElements& tse);
    void addLogger(PositionLogger* logger);
    void removeLogger(PositionLogger* logger);
  private:
    std::set<PositionLogger*> m_loggers;
  };

}

#endif // VTRACKING_POSITIONLOGGER_H

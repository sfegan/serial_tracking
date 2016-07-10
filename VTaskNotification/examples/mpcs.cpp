#include<Task.h>

using namespace VTaskNotification;

class SharedData
{
public:
  VPCS* getPCS() { Guard(lock); return thePCS; }
  void setPCS(VPCS* pcs) { Guard(lock); thePCS=pcs; }

  // .. ra dec ... 

private:
  // mutex

  //  data
  //  data
};

class ClientTask: public Task
{
public:
  ClientTask(SharedData* d): Task(), m_share(d) {}
  virtual ~ClientTask();
  virtual void doTask() = 0;

protected: 
  SharedData* m_share;
};

// ----------------------------------------------------------------------------
//  TASKS
// ----------------------------------------------------------------------------

class CTStop: public ClientTask
{
public:
  CTStop(SharedData* d): ClientTask(d) {}
  virtual ~CTStop();
  virtual void doTask();
};

void CTStop::doTask()
{
  VPCS* thePCS = m_share->getPCS(); // get the remote end out of the share
  thePCS->stop();
}

class CTGoto: public ClientTask
{
public:
  CTGoto(SharedData* d, double ra, double dec): 
    ClientTask(d), m_ra(ra), m_dec(dec) {}
  virtual ~CTStop();
  virtual void doTask();

private:
  double m_ra;
  double m_dec;
};

void CTGoto::doTask()
{
  VPCS* thePCS = m_share->getPCS(); // get the remote end out of the share
  thePCS->trackLocation(m_ra,m_dec);
}

class MPSC_Server
{
public:
  MPSC_Server();
  
  // ....

  void trackSource(CORBA::String source_name);
  void stopAllTracking();

  // ....

private:
  TaskList*   m_tasklist[4];
  SharedData* m_shares[4];
};

MPSC_Server::MPSC_Server()
{
  // stuff...

  for(unsigned i=0;i<4;i++)
    {
      m_tasklist[i] = new TaskList(1);
      m_shares[i] = new SharedData(/* whatever */);
    }

  // stuff ....

}

void MPSC_Server::stopAllTracking()
{
  for(unsigned i=0;i<4;i++)
    {
      Task* stopTask = new CTStop(m_shares[i]);
      m_tasklist[i]->scheduleTask(stopTask);
    }
}

void MPSC_Server::trackSource(CORBA::String source_name);
{
  double ra;
  double dec;
  double epoch;
  
  m_database -> getCoordinates(source_name;ra,dec,epoch);
  precessCoords(ra,dec,epoch,now_epoch);

  for(unsigned i=0;i<4;i++)
    {
      Task* gotoTask = new CTGoto(m_shares[i],ra,dec);
      m_tasklist[i]->scheduleTask(gotoTask);
    }
  
}

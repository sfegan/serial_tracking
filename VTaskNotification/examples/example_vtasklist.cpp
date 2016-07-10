#include <iostream>
#include <unistd.h>

#include <Task.h>

class StupidTask: public Task
{
public:
  StupidTask(): m_tasknum(m_taskcount++) {};
  virtual void doTask();
  
private:
  int m_tasknum;
  static int m_taskcount;
};

int StupidTask::m_taskcount=0;

void StupidTask::doTask()
{
  cerr << "Starting task " << m_tasknum << endl;
  usleep(200000);
  cerr << "Finished task " << m_tasknum << endl;
}

void main()
{
  TaskList tasklist;
  tasklist.addProcessor();

  for(int i=0;i<3;i++)
    {
      StupidTask* t = new StupidTask;
      tasklist.postTask(t);
    }

  tasklist.postQuit();
}


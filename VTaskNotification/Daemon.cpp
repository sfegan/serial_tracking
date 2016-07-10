//-*-mode:c++; mode:font-lock;-*-

/**
 * \file Daemon.cpp
 * \ingroup VMessaging
 * \brief Fork a daemon process
 *
 * Original Author: Stephen Fegan
 * $Author: sfegan $
 * $Date: 2007/02/12 17:50:00 $
 * $Revision: 2.3 $
 * $Tag$
 *
 **/

#include<string>

#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<cstdlib>
#include<errno.h>

#include<Daemon.h>

using namespace VTaskNotification;

int Daemon::s_default_lock_fd = 0;

int Daemon::daemon_init(const std::string& cd, bool close_all_fd,
			std::set<int> no_close_fds)
{
  pid_t pid = fork();
  if(pid < 0)return -1;       /* failed to fork child process */
  else if (pid != 0)exit(EXIT_SUCCESS);  /* parent goes bye-bye */
  
  /* child process */
  
  setsid();                         /* become session leader */
  if(!cd.empty())chdir(cd.c_str()); /* change working directory */
  umask(0);                         /* clear our file creation mask */
  
  if(close_all_fd)
    {
      /* close all open FDs */
      int nfd = sysconf(_SC_OPEN_MAX);
      if(nfd<0)nfd=256;
      for(int ifd=0; ifd<nfd; ifd++)
	if(no_close_fds.find(ifd) == no_close_fds.end())close(ifd);
    }
  
  return 0;
}

bool Daemon::lock_file(const std::string& lockfile, int* lock_fd, int* _errno)
{ 
  if(lock_fd == 0)lock_fd = &s_default_lock_fd;

  *lock_fd = open(lockfile.c_str(),O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR);
  if(*lock_fd<0)
    {
      if(_errno)*_errno=errno;
      *lock_fd = 0;
      return false;
    }

  struct flock lock;
  lock.l_type   = F_WRLCK;
  lock.l_whence = SEEK_SET;
  lock.l_start  = 0;
  lock.l_len    = 0;
  lock.l_pid    = 0;

  if(fcntl(*lock_fd, F_SETLK, &lock) < 0)
    {
      if(_errno)
	if((errno==EACCES)||(errno==EAGAIN))*_errno = 0;
	else *_errno = errno;
      close(*lock_fd);
      *lock_fd = 0;
      return false;
    }

  if(_errno)*_errno = 0;
  return true;
}

bool Daemon::unlock_file(int* lock_fd, int* _errno)
{
  if(lock_fd == 0)lock_fd = &s_default_lock_fd;

  struct flock lock;
  lock.l_type   = F_UNLCK;
  lock.l_whence = SEEK_SET;
  lock.l_start  = 0;
  lock.l_len    = 0;
  lock.l_pid    = 0;

  if(fcntl(*lock_fd, F_SETLK, &lock) < 0)
    {
      if(_errno)*_errno = errno;
      close(*lock_fd);
      *lock_fd = 0;
      return false;
    }

  close(*lock_fd);
  *lock_fd = 0;
  if(_errno)*_errno = 0;
  return true;
}

//-*-mode:c++; mode:font-lock;-*-

/**
 * \file Daemon.h
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

#ifndef VMESSAGING_DAEMON_H
#define VMESSAGING_DAEMON_H

#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>

#include<set>

namespace VTaskNotification
{

  class Daemon
  {
  public:
    static int 
    daemon_init(const std::string& cd = "", bool close_all_fd = true,
		std::set<int> no_close_fds = std::set<int>());

    static bool lock_file(const std::string& lockfile, 
			  int* lock_fd = 0, int* _errno = 0);

    static bool unlock_file(int* lock_fd = 0, int* _errno = 0);
    
  private:
    static int s_default_lock_fd;
  };

}

#endif // VMESSAGING_DAEMON_H

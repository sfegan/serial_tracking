//-*-mode:c++; mode:font-lock;-*-

/**
 *   Extract tracking data from database
 *   Stephen Fegan, December 1, 2005
 */

// make extractfromdb.o
// g++ -o extractfromdb extractfromdb.o /usr/local/veritas/lib/libVDBnothreads.a `mysql_config --libs` -L ../utility/ -lutility -L ../VMessaging -lVMessaging -lZThread

#include <sys/time.h>

#include<vector>
#include<iostream>
#include<iomanip>
#include<cmath>
#include<memory>

#include<VDB/VDBPositioner.h>

#include<VSOptions.hpp>
#include<Exception.h>
#include<Message.h>
#include<Messenger.h>
#include<StreamMessenger.h>

using namespace VERITAS;
using namespace VMessaging;
using namespace VDBPOS;

void usage(const std::string& progname, const VSOptions& options)
{
  std::cerr << "Usage: " << progname << " [options]" << std::endl;
  std::cerr << std::endl;
  std::cerr << "Options:" << std::endl;
  options.printUsage(std::cerr);
}

int main(int argc, char** argv)
{
  // STUPID
  setenv("TZ","UTC",1);
  tzset();

  std::string program(*argv);

  Message::setProgram(program);
  std::string fn_str = 
    std::string(program)+std::string(" -- ")+ std::string(__PRETTY_FUNCTION__);
  std::auto_ptr<char> fn_c_str(new char[fn_str.length()+1]);
  strcpy(fn_c_str.get(),fn_str.c_str());
  RegisterThisFunction fnguard(fn_c_str.get());

  VSOptions options(argc,argv,true);
  
  StreamMessenger cout_messenger;
  Messenger::relay()->registerMessenger(&cout_messenger);

  BackTrace::catchSignalPrintBacktraceAndAbort(SIGSEGV);

  bool print_usage = false;
  if(options.find("help","Print this message")
     != VSOptions::FS_NOT_FOUND)print_usage = true;
  if(options.find("h","Print this message")
     != VSOptions::FS_NOT_FOUND)print_usage = true;

  unsigned long long start_time = 0;
  unsigned long long stop_time = 0;
  unsigned int scope_id = 0;
  bool subtract_zero = false;

  struct timeval tv;
  gettimeofday(&tv, 0);
  time_t time_s = tv.tv_sec + (tv.tv_usec+500)/1000000;
  unsigned time_ms = (tv.tv_usec+500)/1000;

  struct tm* time_tm = gmtime(&time_s);

  stop_time                 = 
    (time_tm->tm_year+1900) * 10000000000000ULL
    +(time_tm->tm_mon+1)    *   100000000000ULL
    +(time_tm->tm_mday)     *     1000000000ULL
    +(time_tm->tm_hour)     *       10000000ULL
    +(time_tm->tm_min)      *         100000ULL
    +(time_tm->tm_sec)      *           1000ULL
    +time_ms;

  options.findWithValue("start",start_time,
			"First time stamp to print out. The time should "
			"be specified in the form \"YYYYMMDD\", "
			"\"YYYYMMDDhhmmss\" or \"YYYYMMDDhhmmssxxx\" "
			"with the last three digits specifying the number "
			"of milliseconds. A value of 0 will specify to dump "
			"from the begining of the database");

  if((start_time>=10000000ULL)&&(start_time<=99999999ULL))
    start_time *= 1000000000ULL;
  else  if((start_time>=10000000000000ULL)&&(start_time<=99999999999999ULL))
    start_time *= 1000ULL;

  options.findWithValue("stop",stop_time,
			"Last time to print out. The time should "
			"be specified as in the -start option");

  if((stop_time>=10000000ULL)&&(stop_time<=99999999ULL))
    stop_time *= 1000000000ULL + 999999999ULL;
  else  if((stop_time>=10000000000000ULL)&&(stop_time<=99999999999999ULL))
    stop_time *= 1000ULL + 999ULL;

  options.findWithValue("scope",scope_id,
			"Telescope ID starting with zero");

  if(options.find("zero_time", "Print time relative to first timestamp")
     != VSOptions::FS_NOT_FOUND)subtract_zero = true;

  if(!options.assertNoOptions())
    {
      std::cerr << program << ": unknown options: ";
      for(unsigned i=1;i<(unsigned)argc;i++)std::cerr << argv[i];
      std::cerr << std::endl;
      std::cerr << std::endl;
      exit(EXIT_FAILURE);
    }

  if(print_usage)
    {
      usage(program, options);
      exit(EXIT_SUCCESS);
    }

  std::vector<struct StatusInfo> records;
  records = getStatus(scope_id, start_time, stop_time);

  for(std::vector<struct StatusInfo>::const_iterator 
	istatus = records.begin(); istatus != records.end(); istatus++)
    {
      static time_t zero_time_s = 0;
      static uint32_t zero_time_ms = 0;
      static bool zero_found = false;

      unsigned long long timestamp = istatus->timestamp;
      if((timestamp < 10000000000000000ULL)
	 ||(timestamp >= 100000000000000000ULL))continue;
      
      struct tm timestamp_tm;

      timestamp_tm.tm_year  = (timestamp/10000000000000ULL % 10000ULL) - 1900;
      timestamp_tm.tm_mon   = (timestamp/  100000000000ULL % 100ULL) - 1;
      timestamp_tm.tm_mday  = (timestamp/    1000000000ULL % 100ULL);
      timestamp_tm.tm_hour  = (timestamp/      10000000ULL % 100ULL);
      timestamp_tm.tm_min   = (timestamp/        100000ULL % 100ULL);
      timestamp_tm.tm_sec   = (timestamp/          1000ULL % 100ULL);
      timestamp_tm.tm_isdst = 0;
      
      time_t   time_s       = mktime(&timestamp_tm);
      uint32_t time_ms      = (timestamp                    % 1000ULL);

#if 0
      std::cerr << timestamp << ' ' 
		<< timestamp_tm.tm_year << ' '
		<< timestamp_tm.tm_mon << ' '
		<< timestamp_tm.tm_mday << ' '
		<< timestamp_tm.tm_hour << ' '
		<< timestamp_tm.tm_min << ' '
		<< timestamp_tm.tm_sec << ' '
		<< time_s << std::endl;
#endif

      if(zero_found == false)
	{
	  zero_time_s = time_s;
	  zero_time_ms = time_ms;
	  zero_found = true;
	}

      double mjd;

      if(subtract_zero)
	{
	  time_s -= zero_time_s;
	  if(time_ms > zero_time_ms)time_ms -= zero_time_ms;
	  else { time_ms += 1000 - zero_time_ms; time_s--; }
	  mjd = (double(time_s)+double(time_ms)/1000.0)/86400.0;
	}
      else
	{
	  mjd = (double(time_s)+double(time_ms)/1000.0)/86400.0 + 40587;
	}

      double time = time_s + double(time_ms)/1000.0;
      
      double el_raw = istatus->elevation_raw/M_PI*180.0;
      double az_raw = istatus->azimuth_raw/M_PI*180.0;

      double el_cor = istatus->elevation_meas/M_PI*180.0;
      double az_cor = istatus->azimuth_meas/M_PI*180.0;
      if(az_cor>180)az_cor-=360;

      double el_tar = istatus->elevation_target/M_PI*180.0;
      double az_tar = istatus->azimuth_target/M_PI*180.0;
      if(az_tar>180)az_tar-=360;

      std::cout << timestamp << ' ' 
		<< std::fixed 
		<< std::setprecision(3) << time << ' '
		<< std::setprecision(9) << mjd << ' '
		<< istatus->mode << ' '
		<< std::setprecision(5) << el_raw << ' ' 
		<< std::setprecision(5) << az_raw << ' '
		<< std::setprecision(5) << el_cor << ' '
		<< std::setprecision(5) << az_cor << ' '
		<< std::setprecision(5) << el_tar << ' '
		<< std::setprecision(5) << az_tar << std::endl;
    }
}

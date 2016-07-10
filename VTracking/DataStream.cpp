//-*-mode:c++; mode:font-lock;-*-

/**
 * \file DataStream.cpp
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
 * $Revision: 2.3 $
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
#include<sys/socket.h>
#include<sys/un.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<ctype.h>
#include<errno.h>

#include<memory>

#include"Debug.h"
#include"DataStream.h"
#include<VATime.h>

using namespace VMessaging;
using namespace VTracking;
using namespace VERITAS;

// ----------------------------------------------------------------------------
// Various Communications Related Exceptions
// ----------------------------------------------------------------------------

Timeout::~Timeout() throw()
{
  // nothing to see here
}

CommunicationError::~CommunicationError() throw()
{
  // nothing to see here
}

EOFException::~EOFException() throw() 
{
  // nothing to see here
}

// ----------------------------------------------------------------------------
// Data Stream Base Class
// ----------------------------------------------------------------------------

DataStream::~DataStream()
{
  // nothing to see here  
}

datastring 
DataStream::recvToEOM(const datastring& eom, long to_sec, long to_usec)
{
  const std::string::size_type neom = eom.size();
  datastring data;
  std::string::size_type idata = 0;
  std::string::size_type ieom = 0;
  while(1)
    {
      datastring new_data = recvData(1, to_sec, to_usec);
      data += new_data;
      const std::string::size_type ndata = data.size();
      do
	{
	  std::string::size_type ifind = idata + ieom;
	  while((ifind != ndata)&&(ieom != neom)&&(data[ifind] == eom[ieom]))
	    ifind++,ieom++;
	  if(ieom == neom)return data;
	  else if(ifind == ndata)break;
	  else idata++,ieom=0;
	}while(idata != ndata);
    }
  assert(0);
}

void DataStream::printAsHexOctets(std::ostream& stream, const datastring& data)
{
  std::ostringstream s;
  bool print_space=false;
  for(datastring::const_iterator i=data.begin(); i!=data.end(); i++)
    {
      if(print_space)s << ' ';
      s << std::hex << std::uppercase 
	<< std::setw(2) << std::setfill('0') << int(*i);
      print_space=true;
    }
  stream << s.str();
}

bool DataStream::printAsAscii(std::ostream& stream, const datastring& data, 
			      bool print_space, bool last_hex)
{
  std::ostringstream s;
  for(datastring::const_iterator i=data.begin(); i!=data.end(); i++)
    {
      if(isprint(*i))
	{
	  if(last_hex)s << ' ';
	  s << std::dec << std::setw(1) << char(*i);
	  last_hex=false;
	}
      else 
	{
	  if(print_space)s << ' ';
	  s << "0x" << std::hex << std::uppercase 
	    << std::setw(2) << std::setfill('0') 
	    << int(*i);
	  last_hex=true;
	}
      print_space=true;
    }
  stream << s.str();
  return last_hex;
}

DataStream* DataStream::
makeDataStream(const std::string& udsl, int loud, OpenMode mode,
	       long to_sec, long to_usec)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  std::string::size_type colon = udsl.find(':');

  std::string scheme;
  std::string name;

  if(colon!=std::string::npos)
    scheme=udsl.substr(0,colon), name=udsl.substr(colon+1);
  else scheme=udsl, name="";

  if(scheme == "serial")
    {
      if(name.size()==0)name="/dev/ttyS0";
      return new SerialPortDataStream(name,loud,to_sec,to_usec);
    }
  else if(scheme == "udp")
    {
      if(name.size()==0)
	if(mode==OM_CLIENT)name="192.168.1.50/5000";
	else name="0.0.0.0/5000";
      return new UDPDataStream(name,mode,loud,to_sec,to_usec);
    }
  else if(scheme == "unix")
    {
      if(name.size()==0)name="/tmp/tracking_server.sock";
      return new UNIXDomainDataStream(name,mode,to_sec,to_usec);
    }
  else
    {
      Exception error("Unknown DataStream scheme");
      error.messageStream() 
	<< "Unknown uniform datastream locator scheme \"" 
	<< scheme << '"' << std::endl <<  "UDSL=" << udsl;
      throw error;
    }
}

// ----------------------------------------------------------------------------
// Generic FD Data Stream Base Class
// ----------------------------------------------------------------------------

GenericFDDataStream::~GenericFDDataStream()
{
  if(m_fd!=-1)close(m_fd);
}

void GenericFDDataStream::
sendData(const datastring& out, long to_sec, long to_usec)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  unsigned i=0;
  if(loud()>=2)
    Debug::stream() << VATime::now().getMSTimeStamp() << ' ' 
		    << "SEND(" << std::dec << out.size() << ',' 
		    << to_sec << ',' << to_usec << "):";
  while(i<out.size())
    {
      fd_set ofds;
      FD_ZERO(&ofds);
      FD_SET(m_fd, &ofds);
      timeval tv;
      tv.tv_sec=to_sec;
      tv.tv_usec=to_usec;
      timeval* tvp=&tv;
      if((to_sec==0)&&(to_usec==0))tvp=0;
      int n=select(m_fd+1,0,&ofds,0,tvp);
      if(n==0)
	{
	  if(loud()>=2)
	    Debug::stream() << " Timeout" << std::endl;
	  throw Timeout();
	}
      else if(n==-1)
	throw CommunicationError("Select call returned an error");

      n=write(m_fd,out.c_str()+i,/* out.size()-i */ 1);
      if(n<=0)throw CommunicationError("Write did not send any data");

      if(loud()>=2)
	{
	  std::ostringstream s; 
	  s << ' ' << std::setw(2) << std::setprecision(2) 
	    << std::setfill('0') << std::hex << std::uppercase 
	    << (int(*(out.c_str()+i))&0xff);
	  Debug::stream() << s.str();
	}
      i+=n;
    }
  if(loud()>=2)Debug::stream() << std::endl;
}

datastring GenericFDDataStream::
recvData(size_t req_count, long to_sec, long to_usec)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  if(loud()>=2)
    Debug::stream() << VATime::now().getMSTimeStamp() << ' ' 
		    << "RECV(" << std::dec << req_count << ',' 
		    << to_sec << ',' << to_usec << "):";
  datastring data;
  for(unsigned i=0; i<req_count; i++)
    {
      fd_set ifds;
      FD_ZERO(&ifds);
      FD_SET(m_fd, &ifds);
      struct timeval tv;
      tv.tv_sec=to_sec;
      tv.tv_usec=to_usec;
      timeval* tvp=&tv;
      if((to_sec==0)&&(to_usec==0))tvp=0;
      int n=select(m_fd+1,&ifds,0,0,tvp);
      if(n==0)
	{
	  if(loud()>=2)Debug::stream() << " Timeout" << std::endl;
	  throw Timeout();
	}
      else if(n==-1)
	throw CommunicationError("Select call returned an error");
      unsigned char c;
      n=read(m_fd,&c,1);
      if(n==0)throw EOFException();
      else if((n<0)&&(errno==ECONNRESET))throw EOFException();
      else if(n<0)throw CommunicationError("Read returned error");
      
      if(loud()>=2)
	{
	  std::ostringstream s;
	  s << ' ' << std::setw(2) << std::setprecision(2) 
	    << std::setfill('0') << std::hex << std::uppercase 
	    << (int(c)&0xff);
	  Debug::stream() << s.str();
	}
      data.push_back(c);
    }
  if(loud()>=2)Debug::stream() << std::endl;
  return data;
}

void GenericFDDataStream::resetDataStream()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  // nothing to see here
}

// ----------------------------------------------------------------------------
// Serial Port Data Stream
// ----------------------------------------------------------------------------

SerialPortDataStream::
SerialPortDataStream(const std::string& filename,
		     int loud, long to_sec, long to_usec,
		     const struct termios* port_p)
  : GenericFDDataStream(loud,to_sec,to_usec), m_filename(filename)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  
  m_fd=open(filename.c_str(),O_RDWR | O_NOCTTY);
  if(m_fd==-1)
    throw CommunicationError(std::string("Could not open ")+filename);

  struct termios port;
  memset(&port,0,sizeof(port));
  port.c_iflag = IGNBRK;
  // port.c_oflag = 0;
  port.c_cflag = B19200 | CS8 | CLOCAL | CREAD | !CRTSCTS;
  port.c_lflag = NOFLSH | !ISIG;
  
  if(port_p==0)port_p=&port;

  if(tcsetattr(m_fd, TCSANOW, port_p) < 0)
    throw CommunicationError("Could not configure the serial port");
}

SerialPortDataStream::~SerialPortDataStream()
{
  // nothing to see here
}

void SerialPortDataStream::resetDataStream()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  if(tcflush(m_fd,TCIOFLUSH) == -1)
    throw CommunicationError("Could not flush serial port");
}

std::string SerialPortDataStream::udsl() const
{
  return std::string("serial:")+m_filename;
}

// ----------------------------------------------------------------------------
// Dual Mode Data Stream Base
// ----------------------------------------------------------------------------

DualModeDataStream::~DualModeDataStream()
{
  // nothing to see here
}

// ----------------------------------------------------------------------------
// Stream Socket Data Steam (Base for Unix/TCPIP)
// ----------------------------------------------------------------------------

StreamSocketDataStream::~StreamSocketDataStream()
{
  if(m_sockfd!=-1)close(m_sockfd);
}

datastring StreamSocketDataStream::
StreamSocketDataStream::recvData(size_t req_count, long to_sec, long to_usec)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  while(1)
    {
      if((m_fd==-1)&&(m_sockfd!=-1))
	{
	  if(loud()>=2)
	    Debug::stream() << VATime::now().getMSTimeStamp() << ' ' 
			    << "RECV(" << std::dec << req_count << ',' 
			    << to_sec << ',' << to_usec 
			    << "): Accepting Connections" << std::endl;
	  fd_set ifds;
	  FD_ZERO(&ifds);
	  FD_SET(m_sockfd, &ifds);
	  struct timeval tv;
	  tv.tv_sec=to_sec;
	  tv.tv_usec=to_usec;
	  timeval* tvp=&tv;
	  if((to_sec==0)&&(to_usec==0))tvp=0;
	  int n=select(m_sockfd+1,&ifds,0,0,tvp);
	  if(n<0)
	    throw CommunicationError("Select on socket returned error");
	  else if(n==0)throw Timeout();
	  
	  m_fd=accept(m_sockfd,0,0);
	  if(m_fd==-1)
	    throw CommunicationError("Could not listen to socket");
	}
      
      try
	{
	  return GenericFDDataStream::recvData(req_count,to_sec,to_usec);
	}
      catch(const EOFException&)
	{
	  if(m_sockfd==-1)throw;
	  close(m_fd);
	  m_fd=-1;
	}
    }
}

// ----------------------------------------------------------------------------
// Unix Domain Data Stream
// ----------------------------------------------------------------------------

UNIXDomainDataStream::UNIXDomainDataStream(const std::string& filename, 
					   OpenMode mode, 
					   int loud, long to_sec, long to_usec)
  : StreamSocketDataStream(loud,to_sec,to_usec), m_filename(filename)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  switch(mode)
    {
    case OM_CLIENT:
      m_fd = socket(PF_UNIX, SOCK_STREAM, 0);
      if(m_fd==-1)
	throw CommunicationError(std::string("Could not make socket"));
      else
	{
	  struct sockaddr_un sa;
	  memset(&sa,0,sizeof(sa));
	  sa.sun_family = AF_UNIX;
	  strcpy(sa.sun_path,filename.c_str());
	  if(connect(m_fd,reinterpret_cast<sockaddr*>(&sa),sizeof(sa))==-1)
	    throw CommunicationError(std::string("Could not connect to: ")+filename);
	}
      break;

    case OM_SERVER:
      unlink(filename.c_str());
      m_sockfd = socket(PF_UNIX, SOCK_STREAM, 0);
      if(m_sockfd==-1)
	throw CommunicationError(std::string("Could not make socket"));
      else
	{
	  struct sockaddr_un sa;
	  memset(&sa,0,sizeof(sa));
	  sa.sun_family = AF_UNIX;
	  strcpy(sa.sun_path,filename.c_str());
	  if(bind(m_sockfd,reinterpret_cast<sockaddr*>(&sa),sizeof(sa))<0)
	    throw CommunicationError(std::string("Could not bind to: ")+filename);
	  if(listen(m_sockfd,1)<0)
	    throw CommunicationError(std::string("Could not listen on socket"));
	}
      break;
    }
}

UNIXDomainDataStream::~UNIXDomainDataStream()
{
  // nothing to see here
}

std::string UNIXDomainDataStream::udsl() const
{
  return std::string("unix:")+m_filename;
}

// ----------------------------------------------------------------------------
// UDP Data Stream -- Each messsage in a different packet
// ----------------------------------------------------------------------------
  
static bool 
decodeIPAddressAndPort(const std::string& address, struct sockaddr_in& sa)
{
  std::string::size_type index = address.find('/');
  if(index == address.size())return false; // no port number

  std::string host_str = address.substr(0,index);
  std::string port_str = address.substr(index+1);

  struct hostent* he = gethostbyname(host_str.c_str());
  if((he == 0)||(he->h_length==0))return false;

  int port_num;
  if(!(std::istringstream(port_str) >> port_num))return false;
  if((port_num>65535)||(port_num<0))return false;

  sa.sin_family  = he->h_addrtype;
  sa.sin_port    = htons(port_num);
  sa.sin_addr    = *reinterpret_cast<struct in_addr*>(he->h_addr_list[0]);

  return true;
}

UDPDataStream::UDPDataStream(const std::string& address, OpenMode mode, 
			     int loud, long to_sec, long to_usec): 
  DualModeDataStream(loud,to_sec,to_usec), m_mode(mode), m_remote_addr(),
  m_address(address)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  switch(mode)
    {
    case OM_CLIENT:
      if(!decodeIPAddressAndPort(address, m_remote_addr))
	throw Exception(std::string("Could not decode or resolve address: ")+address);

      m_fd = socket(PF_INET, SOCK_DGRAM, 0);
      if(m_fd==-1)
	throw CommunicationError(std::string("Could not make socket"));

      break;

    case OM_SERVER:
      m_fd = socket(PF_INET, SOCK_DGRAM, 0);
      if(m_fd==-1)
	throw CommunicationError(std::string("Could not make socket"));
      else
	{
	  struct sockaddr_in sa;
	  memset(&sa,0,sizeof(sa));

	  if(!decodeIPAddressAndPort(address, sa))
	    throw Exception("Resolve failed",std::string("Could not decode or resolve address: ")+address);
	  
	  if(bind(m_fd,reinterpret_cast<sockaddr*>(&sa),sizeof(sa))<0)
	    throw CommunicationError(std::string("Could not bind to: ")+address);
	}
      break;
    }
}

UDPDataStream::~UDPDataStream()
{
  // nothing to see here
}

void UDPDataStream::sendData(const datastring& out, long to_sec, long to_usec)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  if(loud()>=2)
    Debug::stream() << VATime::now().getMSTimeStamp() << ' ' 
		    << "SEND(" << std::dec << out.size() << ',' 
		    << to_sec << ',' << to_usec << "): ";

  fd_set ofds;
  FD_ZERO(&ofds);
  FD_SET(m_fd, &ofds);
  timeval tv;
  tv.tv_sec=to_sec;
  tv.tv_usec=to_usec;
  timeval* tvp=&tv;
  if((to_sec==0)&&(to_usec==0))tvp=0;
  
  int n=select(m_fd+1,0,&ofds,0,tvp);
  if(n==0)
    {
      if(loud()>=2)Debug::stream() << " Timeout" << std::endl;
      throw Timeout();
    }
  else if(n==-1)
    throw CommunicationError("Select call returned an error");

  n=sendto(m_fd,out.c_str(),out.size(),MSG_DONTWAIT|MSG_NOSIGNAL,
	   reinterpret_cast<sockaddr*>(&m_remote_addr),sizeof(m_remote_addr));
  
  if((n<0)&&(errno==EAGAIN))throw Timeout();
  else if((n>=0)&&(n<int(out.size())))
    {
      std::ostringstream msg;
      msg << "Request to send " << out.size() 
	  << " characters by UDP could not be fulfilled";
      throw Exception("UDP send failed",msg.str());
    }
  else if(n<0)throw CommunicationError("sendto() returned error");
  
  if(loud()>=2)
    {
      printAsAscii(Debug::stream(), out);
      Debug::stream() << std::endl;
    }
}

datastring UDPDataStream::recvData(size_t req_count, long to_sec, long to_usec)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  if(loud()>=2)
    Debug::stream() << VATime::now().getMSTimeStamp() << ' ' 
		    << "RECV(" << std::dec << req_count << ',' 
		    << to_sec << ',' << to_usec << "): ";

  fd_set ifds;
  FD_ZERO(&ifds);
  FD_SET(m_fd, &ifds);
  struct timeval tv;
  tv.tv_sec=to_sec;
  tv.tv_usec=to_usec;
  timeval* tvp=&tv;
  if((to_sec==0)&&(to_usec==0))tvp=0;

  int n=select(m_fd+1,&ifds,0,0,tvp);
  if(n==0)
    {
      if(loud()>=2)Debug::stream() << "Timeout" << std::endl;
      throw Timeout();
    }
  else if(n==-1)
    throw CommunicationError("Select call returned an error");

  GuardedBuffer buffer(sc_maxMsgLen);
  struct sockaddr_in sa;
  socklen_t sa_len = sizeof(sa);
  n = recvfrom(m_fd, buffer.get(), sc_maxMsgLen, MSG_NOSIGNAL, 
	       reinterpret_cast<sockaddr*>(&sa), &sa_len);

  if((n<0)&&(errno==EAGAIN))throw Timeout();
  else if(n<0)throw CommunicationError("recvfrom() returned error");
  
  if(m_mode==OM_SERVER)m_remote_addr = sa;

  datastring data(buffer.get(),n);

  if(loud()>=2)
    {
      printAsAscii(Debug::stream(), data);
      Debug::stream() << std::endl;
    }
  return data;
}

void UDPDataStream::resetDataStream()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  int n;
  GuardedBuffer buffer(sc_maxMsgLen);

  if(loud()>=2)
    Debug::stream() << VATime::now().getMSTimeStamp() << ' ' 
		    << "UDPDataStream::resetDataStream()" << std::endl;

  do
    {
      fd_set ifds;
      FD_ZERO(&ifds);
      FD_SET(m_fd, &ifds);
      struct timeval tv;
      //  tv.tv_sec=to_sec;
      //  tv.tv_usec=to_usec;
      tv.tv_sec=0;
      tv.tv_usec=0;
      
      n=select(m_fd+1,&ifds,0,0,&tv);

      if(n==-1)
	throw CommunicationError("Select call returned an error");

      if(n>0)
	{
	  struct sockaddr_in sa;
	  socklen_t sa_len = sizeof(sa);
	  int nn = recvfrom(m_fd, buffer.get(), sc_maxMsgLen, MSG_NOSIGNAL, 
			    reinterpret_cast<sockaddr*>(&sa), &sa_len);
	  
	  if((nn<0)&&(errno!=EAGAIN))
	    throw CommunicationError("recvfrom() returned error");

	  if(loud()>=2)
	    {
	      Debug::stream() << VATime::now().getMSTimeStamp() << ' ' 
			      << "UDPDataStream::resetDataStream(): ";
	      printAsAscii(Debug::stream(), buffer.get());
	      Debug::stream() << std::endl;
	    }
	}
    }while(n>0);
}

std::string UDPDataStream::udsl() const
{
  return std::string("udp:")+m_address;
}

// ----------------------------------------------------------------------------
// Test Data Stream
// ----------------------------------------------------------------------------

TestDataStream::~TestDataStream()
{
  // nothing to see here
}

void TestDataStream::sendData(const datastring& out, long to_sec, long to_usec)
{
  write(1,out.c_str(),out.length());
}

datastring TestDataStream::recvData(size_t req_count, long to_sec, long to_usec)
{
  static char c=0;
  datastring str;
  for(unsigned i=0;i<req_count;i++)
    {
      read(0,&c,1);
      str += c;
    }
  return str;
}

void TestDataStream::resetDataStream()
{
  // nothing to see here
}
 
#ifdef TESTMAIN

int main(int argc, char** argv)
{
  argc--,argv++;
  while(argc)
    {
      struct sockaddr_in addr;
      bool good = decodeIPAddressAndPort(*argv, addr);
      if(good)
	Debug::stream() 
	  << std::setw(30) << std::left << *argv << " = " 
	  << std::setw(15) << std::left << inet_ntoa(addr.sin_addr) << ' '
	  << std::setw(5) << ntohs(addr.sin_port) << ' ' 
	  << addr.sin_family << std::endl;
      else 
	Debug::stream() << *argv << ": Could not resolve IP addr and port"
			<< std::endl;
      argc--,argv++;
    }
}

#endif

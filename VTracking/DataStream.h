//-*-mode:c++; mode:font-lock;-*-

/**
 * \file DataStream.h
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
 * $Date: 2007/10/11 18:50:42 $
 * $Revision: 2.1 $
 * $Tag$
 *
 **/

#ifndef VTRACKING_DATASTREAM_H
#define VTRACKING_DATASTREAM_H

#include<ostream>
#include<string>
#include<deque>

#include<termios.h>
#include<netinet/in.h>

#include<Exception.h>

namespace VTracking
{
  // --------------------------------------------------------------------------
  // Guarded char* array
  // --------------------------------------------------------------------------

  class GuardedBuffer
  {
  public:
    GuardedBuffer(unsigned length):
      m_buffer(new char[length]) { /* N T S H */ }
    ~GuardedBuffer() { delete[] m_buffer; }
    char* get() { return m_buffer; }
  private:
    char* m_buffer;
  };

  // --------------------------------------------------------------------------
  // Various Communications Related Exceptions
  // --------------------------------------------------------------------------

  class Timeout: public VMessaging::Exception
  {
  public:
    Timeout(): Exception("Timeout") { }
    virtual ~Timeout() throw();
  };
  
  class CommunicationError: public VMessaging::SystemError
  {
  public:
    CommunicationError(const std::string& msg, int error_num=errno): 
      SystemError(msg,error_num) { }
    virtual ~CommunicationError() throw();
  };
  
  class EOFException: public VMessaging::Exception
  {
  public:
    EOFException(): Exception("End of file returned") { }
    virtual ~EOFException() throw();
  };

  // --------------------------------------------------------------------------
  // Data Stream Base Class
  // --------------------------------------------------------------------------
  
  typedef std::basic_string<char> datastring;
  
  class DataStream
  {
  public:
    DataStream(int loud=0, long to_sec=0, long to_usec=0): 
      m_loud(loud), m_default_to_sec(to_sec), m_default_to_usec(to_usec) { }
    virtual ~DataStream();
    virtual void sendData(const datastring& out, long to_sec, long to_usec)=0;
    virtual datastring recvData(size_t req_count, long to_sec, long to_usec)=0;
    virtual void resetDataStream()=0;
    virtual std::string udsl() const=0;

    void sendData(const datastring& out) { sendData(out,m_default_to_sec,m_default_to_usec); }
    datastring recvData(size_t req_count) { return recvData(req_count,m_default_to_sec,m_default_to_usec); }
    
    datastring recvToEOM(const datastring& eom, long to_sec, long to_usec);
    datastring recvToEOM(const datastring& eom) { return recvToEOM(eom,m_default_to_sec,m_default_to_usec); }

    int loud() const { return m_loud; }
    void setLoud(int loud) { m_loud=loud; }

    long defaultTOSec() const { return m_default_to_sec; }
    long defaultTOuSec() const { return m_default_to_usec; }
    void setDefaultTO(long to_sec, long to_usec) { m_default_to_sec=to_sec; m_default_to_usec=to_usec; }

    static void printAsHexOctets(std::ostream& stream, const datastring& data);
    static bool printAsAscii(std::ostream& stream, const datastring& data, bool print_space=false, bool last_hex=false);

    enum OpenMode { OM_CLIENT, OM_SERVER };

    static DataStream* makeDataStream(const std::string& udsl, int loud,
				      OpenMode mode = OM_CLIENT, 
				      long to_sec=0, long to_usec=100000);
  protected:
    int                       m_loud;
    long                      m_default_to_sec;
    long                      m_default_to_usec;
  }; // class DataStream

  // --------------------------------------------------------------------------
  // Generic FD Data Stream Base Class
  // --------------------------------------------------------------------------
  
  class GenericFDDataStream: public DataStream
  {
  public:
    virtual ~GenericFDDataStream();
    virtual void sendData(const datastring& out, long to_sec, long to_usec);
    virtual datastring recvData(size_t req_count, long to_sec, long to_usec);
    virtual void resetDataStream();

  protected:
    GenericFDDataStream(int loud, long to_sec=0, long to_usec=0): 
      DataStream(loud,to_sec,to_usec), m_fd(-1) { }

    int                       m_fd;

  private:
    GenericFDDataStream();
    GenericFDDataStream& operator=(const GenericFDDataStream&);
  }; // class GenericFDDataStream
  
  // --------------------------------------------------------------------------
  // Serial Port Data Stream
  // --------------------------------------------------------------------------

  class SerialPortDataStream: public GenericFDDataStream
  {
  public:
    SerialPortDataStream(const std::string& filename, 
			 int loud=0, long to_sec=0, long to_usec=100000,
			 const struct termios* port_p=0);
    virtual ~SerialPortDataStream();
    virtual void resetDataStream();
    virtual std::string udsl() const;
  private:
    std::string               m_filename;
  };  
  
  // --------------------------------------------------------------------------
  // Dual Mode Data Stream Base
  // --------------------------------------------------------------------------

  class DualModeDataStream: public GenericFDDataStream
  {
  public:
    virtual ~DualModeDataStream();
    
  protected:
    DualModeDataStream(int loud=0, long to_sec=0, long to_usec=0): 
      GenericFDDataStream(loud,to_sec,to_usec) {}
  };

  // --------------------------------------------------------------------------
  // Stream Socket Data Steam (Base for Unix/TCPIP)
  // --------------------------------------------------------------------------
  
  class StreamSocketDataStream: public DualModeDataStream
  {
  public:
    virtual ~StreamSocketDataStream();
    virtual datastring recvData(size_t req_count, long to_sec, long to_usec);
    
  protected:
    StreamSocketDataStream(int loud=0, long to_sec=0, long to_usec=0): 
      DualModeDataStream(loud,to_sec,to_usec), m_sockfd(-1) {}

    int                       m_sockfd;
  };  
  
  // --------------------------------------------------------------------------
  // Unix Domain Data Stream
  // --------------------------------------------------------------------------
  
  class UNIXDomainDataStream: public StreamSocketDataStream
  {
  public:
    UNIXDomainDataStream(const std::string& filename, OpenMode mode,
			 int loud=0, long to_sec=0, long to_usec=100000);
    virtual ~UNIXDomainDataStream();
    virtual std::string udsl() const;
  private:
    std::string               m_filename;
  };  

  // --------------------------------------------------------------------------
  // UDP Data Stream -- Each messsage in a different packet
  // --------------------------------------------------------------------------
  
  class UDPDataStream: public DualModeDataStream
  {
  public:
    UDPDataStream(const std::string& address, OpenMode mode, 
		  int loud = 0, long to_sec=0, long to_usec=100000);
    virtual ~UDPDataStream();
    virtual void sendData(const datastring& out, long to_sec, long to_usec);
    virtual datastring recvData(size_t req_count, long to_sec, long to_usec);
    virtual void resetDataStream();
    virtual std::string udsl() const;
  private:
    OpenMode                  m_mode;
    struct sockaddr_in        m_remote_addr;
    std::string               m_address;

    static const int sc_maxMsgLen = 2048;
  };

  // --------------------------------------------------------------------------
  // Test Data Stream
  // --------------------------------------------------------------------------
  
  class TestDataStream: public DualModeDataStream
  {
  public:
    TestDataStream() { }
    virtual ~TestDataStream();
    virtual void sendData(const datastring& out, long to_sec, long to_usec);
    virtual datastring recvData(size_t req_count, long to_sec, long to_usec);
    virtual void resetDataStream();
  };

} //namespace VTracking

#endif // VTRACKING_DATASTREAM_H

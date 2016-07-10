//-*-mode:c++; mode:font-lock;-*-

/**
 * \file VOmniORBHelper.h
 * \ingroup VOmniOrmHelper
 * \brief This is a one-line description of this cpp file.
 *
 * Here is a tedious verbose multi-line description of all
 * the details of the code, more than you would
 * ever want to read. Generally, all the important documentation
 * goes in the .cpp files.
 *
 * Original Author: Stephen Fegan
 * $Author: sfegan $
 * $Date: 2010/10/28 14:48:05 $
 * $Revision: 2.4 $
 * $Tag$
 *
 **/

#ifndef VOMNIORBHELPER_H
#define VOMNIORBHELPER_H

#include<string>
#include<map>

#include<omniORB4/CORBA.h>

#ifndef NOCOSEVENT
#include<CosEventChannelAdmin.hh>
#include<CosEventComm.hh>
#endif

#include<zthread/RecursiveMutex.h>
#include<zthread/Singleton.h>

namespace VCorba
{

  class VOmniORBHelper: 
    virtual public ZThread::Singleton<VOmniORBHelper>
  {
  public:
    VOmniORBHelper(const std::string& project = "VERITAS") throw ();
    ~VOmniORBHelper() throw ();
    
    // Access to the undelying orb
    CORBA::ORB_ptr orb() throw(CORBA::SystemException);
    
    // Specific OmniORB configuration methods
    const char* orbGetProperty(const char* prop) throw ();
    void orbSetProperty(const char* prop, const char* val) throw();
    
    void orbSetConcurrancyModelThreadPerConnection() throw ();
    void orbSetConcurrancyModelThreadPool(unsigned int nthreads=0) 
      throw();
    
    // The VCorbaHelper interface methods
    
    void orbInit(int& argc, char** argv, 
		 const char* nameserver = 0, int portnumber = -1)
      throw(CORBA::SystemException);
    
    void orbDestroy()
      throw(CORBA::SystemException);
    
    void orbRun()
      throw(CORBA::SystemException);
    
    void orbShutdown(CORBA::Boolean wait)
      throw(CORBA::SystemException);
    
    PortableServer::POA_ptr poaRootPoa()
      throw(CORBA::SystemException);
    
    char* poaPathToPoaName(const char* program, const char* object,
			   int telescopenumber = -1)
      throw(CORBA::SystemException);
    
    PortableServer::POA_ptr 
    poaGetOrCreatePoa(const char* program, const char* object,
		      int telescopenumber = -1)
      throw(CORBA::SystemException);
    
    PortableServer::ObjectId* 
    poaPathToObjectId(const char* program, const char* object,
		      int telescopenumber = -1)
      throw(CORBA::SystemException);
    
    CORBA::Object_ptr 
    poaActivateObject(PortableServer::Servant p_servant,
		      const char* program, const char* object,
		      int telescopenumber = -1)
      throw(CORBA::SystemException);
    
    CosNaming::NamingContext_ptr nsRootContext()
      throw(CORBA::SystemException);
    
    CosNaming::Name* 
    nsPathToContextName(const char* program, const char* object,
			int telescopenumber = -1)
      throw(CORBA::SystemException);
    
    CosNaming::Name* 
    nsPathToObjectName(const char* program, const char* object,
		       int telescopenumber = -1)
      throw(CORBA::SystemException);
    
    CosNaming::NamingContext_ptr 
    nsGetContext(const char* program, const char* object,
		 int telescopenumber = -1)
      throw(CORBA::SystemException, 
	    CosNaming::NamingContext::NotFound,
	    CosNaming::NamingContext::CannotProceed,
	    CosNaming::NamingContext::InvalidName);
    
    CORBA::Object_ptr 
    nsGetObject(const char* program, const char* object,
		int telescopenumber = -1)
      throw(CORBA::SystemException, 
	    CosNaming::NamingContext::NotFound,
	    CosNaming::NamingContext::CannotProceed,
	    CosNaming::NamingContext::InvalidName);
    
    template<class T>
    typename T::_ptr_type 
    nsGetNarrowedObject(const char* program, const char* object,
			int telescopenumber = -1)
      throw(CORBA::SystemException, 
	    CosNaming::NamingContext::NotFound,
	    CosNaming::NamingContext::CannotProceed,
	    CosNaming::NamingContext::InvalidName);
    
    CosNaming::NamingContext_ptr 
    nsGetOrCreateContext(const char* program, const char* object,
			 int telescopenumber = -1)
      throw(CORBA::SystemException, 
	    CosNaming::NamingContext::NotFound,
	    CosNaming::NamingContext::CannotProceed,
	    CosNaming::NamingContext::InvalidName,
	    CosNaming::NamingContext::AlreadyBound);
    
    void 
    nsRegisterObject(CORBA::Object_ptr obj,
		     const char* program, const char* object,
		     int telescopenumber = -1)
      throw(CORBA::SystemException, 
	    CosNaming::NamingContext::NotFound,
	    CosNaming::NamingContext::CannotProceed,
	    CosNaming::NamingContext::InvalidName,
	    CosNaming::NamingContext::AlreadyBound);
    
    const std::string& nsGetNameserverIOR() const { return m_nameserver_ior; }

#ifndef NOCOSEVENT
    CosEventChannelAdmin::ProxyPushConsumer_ptr
    eventGetProxyPushConsumer(const char* program, const char* object,
			      int telescopenumber = -1);
    
    CosEventChannelAdmin::ProxyPushSupplier_ptr
    eventGetProxyPushSupplier(const char* program, const char* object,
			      int telescopenumber = -1);

    CosEventChannelAdmin::ProxyPullConsumer_ptr
    eventGetProxyPullConsumer(const char* program, const char* object,
			      int telescopenumber = -1);
    
    CosEventChannelAdmin::ProxyPullSupplier_ptr
    eventGetProxyPullSupplier(const char* program, const char* object,
			      int telescopenumber = -1);
#endif    

    void setGlobalClientTimeout(unsigned timeout);
    void setObjectClientTimeout(CORBA::Object_ptr obj, unsigned timeout);

  protected:
    std::string                                     m_project;
    ZThread::RecursiveMutex                         m_mutex;

    std::map<std::string, std::string>              m_properties;

    CORBA::ORB_var                                  m_orb;
    PortableServer::POA_var                         m_rootPoa;
    PortableServer::POAManager_var                  m_rootPoaManager;
    std::map<std::string, PortableServer::POA_var>  m_poaMap;

    CosNaming::NamingContext_var                    m_rootNamingContext;

    std::string                                     m_nameserver_ior;
  }; // class VOmniORBHelper

  template<class T>
  typename T::_ptr_type
  VOmniORBHelper::nsGetNarrowedObject(const char* program, const char* object,
				      int telescopenumber)
    throw(CORBA::SystemException, 
	  CosNaming::NamingContext::NotFound,
	  CosNaming::NamingContext::CannotProceed,
	  CosNaming::NamingContext::InvalidName)
  {
    CORBA::Object_var base_obj = nsGetObject(program, object, telescopenumber);
    return T::_narrow(base_obj);
  }
  
} // namespace VCorba

#endif // VOMNIORBHELPER_H

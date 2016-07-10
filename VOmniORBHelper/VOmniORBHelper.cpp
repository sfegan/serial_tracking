//-*-mode:c++; mode:font-lock;-*-

/**
 * \file VOmniORBHelper.cpp
 * \ingroup VOmniOrmHelper
 * \brief This is a one-line description of this cpp file.
 *
 * Here is a tedious verbose multi-line description of all
 * the details of the code, more than you would
 * ever want to read. Generally, all the important documentation
 * goes in the .cpp files.
 *
 * Original Author: Stephen Fegan
 * $Author: aune $
 * $Date: 2010/09/09 18:56:11 $
 * $Revision: 2.4 $
 * $Tag$
 *
 **/

#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <cstdlib>

#include <omniORB4/CORBA.h>

#include "VOmniORBHelper.h"

using namespace VCorba;

VOmniORBHelper::VOmniORBHelper(const std::string& project) throw ():
  ZThread::Singleton<VOmniORBHelper>(),
  m_project(project), m_mutex(), m_properties(), m_orb(CORBA::ORB::_nil()), 
  m_rootPoa(PortableServer::POA::_nil()),
  m_rootPoaManager(PortableServer::POAManager::_nil()), m_poaMap(),
  m_rootNamingContext(CosNaming::NamingContext::_nil()),
  m_nameserver_ior()
{
  // nothing to see here
}

VOmniORBHelper::~VOmniORBHelper() throw ()
{
  orbDestroy();
}

CORBA::ORB_ptr
VOmniORBHelper::orb() throw(CORBA::SystemException)
{
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mutex);
  return CORBA::ORB::_duplicate(m_orb);
}

const char* 
VOmniORBHelper::orbGetProperty(const char* prop) throw ()
{
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mutex);
  std::map<std::string, std::string>::const_iterator propFound = 
    m_properties.find(prop);
  
  if(propFound == m_properties.end())return 0;
  else return propFound->second.c_str();
}

void 
VOmniORBHelper::orbSetProperty(const char* prop, const char* val) throw()
{
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mutex);
  m_properties[prop]=val;
}

void 
VOmniORBHelper::orbSetConcurrancyModelThreadPerConnection() throw()
{
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mutex);
  orbSetProperty("threadPerConnection","1");
}

void 
VOmniORBHelper::orbSetConcurrancyModelThreadPool(unsigned int nthreads) throw()
{
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mutex);
  orbSetProperty("threadPerConnection","0");
  if(nthreads)
    {
      std::ostringstream nthread_stream;
      nthread_stream << nthreads;
      orbSetProperty("maxServerThreadPool",nthread_stream.str().c_str());
    }
}

void 
VOmniORBHelper::orbInit(int& argc, char** argv,	
			const char* nameserver, int portnumber)
  throw(CORBA::SystemException)
{
  if((nameserver!=0)&&(orbGetProperty("InitRef")==0))
    {
      std::string nsname=std::string("NameService=")+nameserver;
      orbSetProperty("InitRef",nsname.c_str());
      m_nameserver_ior = nameserver;
    }

  if((portnumber!=-1)&&(orbGetProperty("endPoint")==0))
    {
      std::ostringstream ep_stream;
      ep_stream << "giop:tcp::" << portnumber;
      orbSetProperty("endPoint",ep_stream.str().c_str());
    }

  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mutex);
  if(CORBA::is_nil(m_orb))
    {
      int nprop = m_properties.size();
      int i = 0;
      char* (*props)[2] = new char*[2*(nprop+1)][2];
      for(std::map<std::string,std::string>::const_iterator p = 
	    m_properties.begin(); p != m_properties.end(); p++)
	{
	  props[i][0] = strdup(p->first.c_str());
	  props[i][1] = strdup(p->second.c_str());
	  i++;
	}
      props[i][0]=props[i][1]=0;
      
      m_orb = CORBA::ORB_init(argc, argv, "omniORB4",
			      // why do i need this next cast ?
			      reinterpret_cast<const char* (*)[2]>(props));

      for(i=0; i<nprop; i++)free(props[i][0]),free(props[i][1]);
      delete[] props;
    }
}

void 
VOmniORBHelper::orbDestroy() throw(CORBA::SystemException)
{
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mutex);
  for(std::map<std::string,PortableServer::POA_var>::iterator poa = 
	m_poaMap.begin(); poa != m_poaMap.end(); poa++)
    {
      poa->second = PortableServer::POA::_nil();
    }
  
  if(!CORBA::is_nil(m_rootPoa))
    {
      m_rootPoa        = PortableServer::POA::_nil();
      m_rootPoaManager = PortableServer::POAManager::_nil();
    }

  if(!CORBA::is_nil(m_orb))
    {
      m_orb->destroy();
      m_orb = CORBA::ORB::_nil();
    }
}

void 
VOmniORBHelper::orbRun() throw(CORBA::SystemException)
{
  // This line gets the root POA and activates the POA manager in case
  // it has not been done by the user
  PortableServer::POA_var root_poa = poaRootPoa();

  if(!CORBA::is_nil(m_orb))m_orb -> run();
}

void 
VOmniORBHelper::orbShutdown(CORBA::Boolean wait) throw(CORBA::SystemException)
{
  if(!CORBA::is_nil(m_orb))m_orb -> shutdown(wait);
}

PortableServer::POA_ptr 
VOmniORBHelper::poaRootPoa() throw(CORBA::SystemException)
{
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mutex);
  if(CORBA::is_nil(m_rootPoa))
    {
      CORBA::Object_var poaObj = 
	m_orb -> resolve_initial_references("RootPOA");
      m_rootPoa        = PortableServer::POA::_narrow(poaObj);
      m_rootPoaManager = m_rootPoa -> the_POAManager();
      m_rootPoaManager->activate();
    }

  return PortableServer::POA::_duplicate(m_rootPoa);
}

char* 
VOmniORBHelper::poaPathToPoaName(const char* program, const char* object,
				 int telescopenumber)
  throw(CORBA::SystemException)
{
  std::ostringstream poa_stream;
  if(telescopenumber!=-1)
    poa_stream << m_project << "::" << program << telescopenumber;
  else 
    poa_stream << m_project << "::" << program;
  return CORBA::string_dup(poa_stream.str().c_str());
}

PortableServer::POA_ptr 
VOmniORBHelper::poaGetOrCreatePoa(const char* program, const char* object,
				  int telescopenumber)
  throw(CORBA::SystemException)
{
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mutex);

  // Munge the path name into a unique POA name
  char* poa_name = poaPathToPoaName(program, object, telescopenumber);

  // Check to see if we created this one already, if so then return it now
  std::map<std::string,PortableServer::POA_var>::iterator poaFound = 
    m_poaMap.find(poa_name);
  
  if(poaFound != m_poaMap.end())
    return PortableServer::POA::_duplicate(poaFound->second);

  // Grab the root POA
  PortableServer::POA_var root_poa = poaRootPoa();

  // Must create the POA, use the PERSISTENT and USER_ID policies
  PortableServer::LifespanPolicy_var lifespan = 
    root_poa -> create_lifespan_policy(PortableServer::PERSISTENT);
  PortableServer::IdAssignmentPolicy_var assign =
    root_poa -> create_id_assignment_policy(PortableServer::USER_ID);

  CORBA::PolicyList policy_list;
  policy_list.length(2);
  policy_list[0] = PortableServer::LifespanPolicy::_duplicate(lifespan);
  policy_list[1] = PortableServer::IdAssignmentPolicy::_duplicate(assign);

  // Root POA manager
  PortableServer::POAManager_var root_poa_manager = 
    PortableServer::POAManager::_duplicate(m_rootPoaManager);

  // Create the POA
  PortableServer::POA_var poa = 
    root_poa -> create_POA(poa_name, root_poa_manager, policy_list);

  // Make a copy of the POA object and remember it in case we are asked
  // for it again some time
  m_poaMap[poa_name] = PortableServer::POA::_duplicate(poa);

  CORBA::string_free(poa_name);

  lifespan->destroy();
  assign->destroy();

  return poa._retn();
}

PortableServer::ObjectId* 
VOmniORBHelper::poaPathToObjectId(const char* program, const char* object,
				  int telescopenumber)
  throw(CORBA::SystemException)
{
  return PortableServer::string_to_ObjectId(object);
}

CORBA::Object_ptr 
VOmniORBHelper::poaActivateObject(PortableServer::Servant p_servant,
				  const char* program, const char* object,
				  int telescopenumber)
  throw(CORBA::SystemException)
{
  PortableServer::ObjectId_var oid = 
    poaPathToObjectId(program,object,telescopenumber);
  PortableServer::POA_var poa = 
    poaGetOrCreatePoa(program,object,telescopenumber);
  poa->activate_object_with_id(oid,p_servant);
  return poa->id_to_reference(oid);
}

CosNaming::NamingContext_ptr 
VOmniORBHelper::nsRootContext() throw(CORBA::SystemException)
{
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mutex);
  if(CORBA::is_nil(m_rootNamingContext))
    {
      CORBA::Object_var rncObj = 
	m_orb -> resolve_initial_references("NameService");
      m_rootNamingContext      =  CosNaming::NamingContext::_narrow(rncObj);
    }

  return CosNaming::NamingContext::_duplicate(m_rootNamingContext);
}

CosNaming::Name* 
VOmniORBHelper::nsPathToContextName(const char* program, const char* object,
				    int telescopenumber)
  throw(CORBA::SystemException)
{
  std::ostringstream program_stream;
  if(telescopenumber!=-1)
    program_stream << program << telescopenumber;
  else 
    program_stream << program;
  
  CosNaming::Name* name = new CosNaming::Name;
  name->length(2);
  
  (*name)[0].id   = CORBA::string_dup(m_project.c_str());
  (*name)[0].kind = CORBA::string_dup("");

  (*name)[1].id   = CORBA::string_dup(program_stream.str().c_str());
  (*name)[1].kind = CORBA::string_dup("Program");

  return name;
}
  
CosNaming::Name* 
VOmniORBHelper::nsPathToObjectName(const char* program, const char* object,
				   int telescopenumber)
  throw(CORBA::SystemException)
{
  std::ostringstream program_stream;
  if(telescopenumber!=-1)
    program_stream << program << telescopenumber;
  else 
    program_stream << program;
  
  CosNaming::Name* name = new CosNaming::Name;
  name->length(3);
  
  (*name)[0].id   = CORBA::string_dup(m_project.c_str());
  (*name)[0].kind = CORBA::string_dup("");

  (*name)[1].id   = CORBA::string_dup(program_stream.str().c_str());
  (*name)[1].kind = CORBA::string_dup("Program");

  (*name)[2].id   = CORBA::string_dup(object);
  (*name)[2].kind = CORBA::string_dup("Object");

  return name;
}
  
CosNaming::NamingContext_ptr 
VOmniORBHelper::nsGetContext(const char* program, const char* object,
			     int telescopenumber)
  throw(CORBA::SystemException, 
	CosNaming::NamingContext::NotFound,
	CosNaming::NamingContext::CannotProceed,
	CosNaming::NamingContext::InvalidName)
{
  CosNaming::NamingContext_var root = nsRootContext();
  CosNaming::Name_var name = 
    nsPathToContextName(program,object,telescopenumber);
  CORBA::Object_var context = root->resolve(name);
  return CosNaming::NamingContext::_narrow(context);
}
  
CORBA::Object_ptr 
VOmniORBHelper::nsGetObject(const char* program, const char* object,
			    int telescopenumber)
  throw(CORBA::SystemException, 
	CosNaming::NamingContext::NotFound,
	CosNaming::NamingContext::CannotProceed,
	CosNaming::NamingContext::InvalidName)
{
  CosNaming::NamingContext_var root = nsRootContext();
  CosNaming::Name_var name = 
    nsPathToObjectName(program,object,telescopenumber);
  CORBA::Object_ptr obj = root->resolve(name);
  return obj;
}

CosNaming::NamingContext_ptr 
VOmniORBHelper::nsGetOrCreateContext(const char* program, const char* object,
				     int telescopenumber)
  throw(CORBA::SystemException, 
	CosNaming::NamingContext::NotFound,
	CosNaming::NamingContext::CannotProceed,
	CosNaming::NamingContext::InvalidName,
	CosNaming::NamingContext::AlreadyBound)
{
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mutex);
  CosNaming::NamingContext_ptr context;

  try
    {
      context = nsGetContext(program, object, telescopenumber);
    }
  catch(CosNaming::NamingContext::NotFound)
    {
      CosNaming::NamingContext_var root = nsRootContext();
      CosNaming::Name_var name = 
	nsPathToContextName(program, object, telescopenumber);

      for(unsigned int n=0; n<name->length(); n++)
	{
	  CosNaming::Name_var child_name = name;
	  child_name->length(n+1);

	  try
	    {
	      CORBA::Object_var object = root->resolve(name);
	    }
	  catch(CosNaming::NamingContext::NotFound)
	    {
	      CosNaming::NamingContext_var 
		child = root->bind_new_context(child_name);
	    }
	}

      CORBA::Object_var object = root->resolve(name);
      context = CosNaming::NamingContext::_narrow(object);
    }
  
  return context;
}
  
void
VOmniORBHelper::nsRegisterObject(CORBA::Object_ptr obj,
				 const char* program, const char* object,
				 int telescopenumber)
  throw(CORBA::SystemException, 
	CosNaming::NamingContext::NotFound,
	CosNaming::NamingContext::CannotProceed,
	CosNaming::NamingContext::InvalidName,
	CosNaming::NamingContext::AlreadyBound)
{
  ZThread::Guard<ZThread::RecursiveMutex> guard(m_mutex);
  CosNaming::NamingContext_var root = nsRootContext();
  CosNaming::Name_var name = 
    nsPathToObjectName(program, object, telescopenumber);

  for(unsigned int n=0; n<name->length()-1; n++)
    {
      CosNaming::Name_var child_name = name;
      child_name->length(n+1);
      
      try
	{
	  CORBA::Object_var object = root->resolve(child_name);
	}
      catch(CosNaming::NamingContext::NotFound)
	{
	  CosNaming::NamingContext_var 
	    child = root->bind_new_context(child_name);
	}
    }

  root->rebind(name,obj);
}

#ifndef NOCOSEVENT

CosEventChannelAdmin::ProxyPushConsumer_ptr VOmniORBHelper::
eventGetProxyPushConsumer(const char* program, const char* object,
			  int telescopenumber)
{
  CosEventChannelAdmin::EventChannel_var channel =
    nsGetNarrowedObject<CosEventChannelAdmin::EventChannel>(program,object,
							    telescopenumber);
  CosEventChannelAdmin::SupplierAdmin_var supplier_admin =
    channel->for_suppliers();
  return supplier_admin->obtain_push_consumer();
}
    
CosEventChannelAdmin::ProxyPushSupplier_ptr VOmniORBHelper::
eventGetProxyPushSupplier(const char* program, const char* object,
			  int telescopenumber)
{
  CosEventChannelAdmin::EventChannel_var channel =
    nsGetNarrowedObject<CosEventChannelAdmin::EventChannel>(program,object,
							    telescopenumber);
  CosEventChannelAdmin::ConsumerAdmin_var consumer_admin =
    channel->for_consumers();
  return consumer_admin->obtain_push_supplier();
}

CosEventChannelAdmin::ProxyPullConsumer_ptr VOmniORBHelper::
eventGetProxyPullConsumer(const char* program, const char* object,
			  int telescopenumber)
{
  CosEventChannelAdmin::EventChannel_var channel =
    nsGetNarrowedObject<CosEventChannelAdmin::EventChannel>(program,object,
							    telescopenumber);
  CosEventChannelAdmin::SupplierAdmin_var supplier_admin =
    channel->for_suppliers();
  return supplier_admin->obtain_pull_consumer();
}
    
CosEventChannelAdmin::ProxyPullSupplier_ptr VOmniORBHelper::
eventGetProxyPullSupplier(const char* program, const char* object,
			  int telescopenumber)
{
  CosEventChannelAdmin::EventChannel_var channel =
    nsGetNarrowedObject<CosEventChannelAdmin::EventChannel>(program,object,
							    telescopenumber);
  CosEventChannelAdmin::ConsumerAdmin_var consumer_admin =
    channel->for_consumers();
  return consumer_admin->obtain_pull_supplier();
}

void VOmniORBHelper::
setGlobalClientTimeout(unsigned timeout)
{
  omniORB::setClientCallTimeout(timeout);
}

void VOmniORBHelper::
setObjectClientTimeout(CORBA::Object_ptr obj, unsigned timeout)
{
  omniORB::setClientCallTimeout(obj,timeout);
}

#endif

//-*-mode:c++; mode:font-lock;-*-

/**
 * \file MainTerminate.cpp
 * \ingroup VTracking
 * \brief This is a one-line description of this cpp file.
 *
 * Here is a tedious verbose multi-line description of all the details
 * of the code, more than you would ever want to read. Generally, all
 * the important documentation goes in the .cpp files.
 *
 * Original Author: Stephen Fegan
 * $Author: sfegan $
 * $Date: 2006/04/10 18:01:11 $
 * $Revision: 2.1 $
 * $Tag$
 *
 **/

#include<Exception.h>
#include<Debug.h>

#include"MainTerminate.h"
#include"NET_TCInterfaceServant.h"

using namespace VERITAS;
using namespace VTracking;
using namespace VCorba;
using namespace VMessaging;

// ============================================================================
// MAIN TERMINATE FUNCTIONS
// ============================================================================

void MainTerminate::configure(VSOptions& options)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  // nothing to see here
}

MainTerminate::MainTerminate():
  Main(),
  m_scope_id(s_default_scope_id),
  m_corba_args(s_default_corba_args),
  m_corba_nameserver(s_default_corba_nameserver), 
  m_remote_ior(s_default_remote_ior)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  // nothing to see here
}

MainTerminate::~MainTerminate()
{
  // nothing to see here
}

int MainTerminate::main(int argc, char** argv)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  
  try
    {
      // ----------------------------------------------------------------------
      // Initialize the ORB if appropriate
      // ----------------------------------------------------------------------

      VOmniORBHelper* orb = 
	CorbaHelper::initOrb(m_corba_args, m_corba_nameserver, 0);

      NET_TCInterface_var tc_interface;
      if(m_remote_ior.empty())
	{
	  tc_interface = 
	    orb->nsGetNarrowedObject<NET_TCInterface>
	    (NET_TCInterface::program_name,NET_TCInterface::object_name,
	     m_scope_id);
	}
      else
	{
	  CORBA::ORB_var the_orb = orb->orb();
	  CORBA::Object_var obj = 
	    the_orb->string_to_object(m_remote_ior.c_str());
	  tc_interface = NET_TCInterface::_narrow(obj);
	}

      tc_interface->netTerminate();

      if(orb)delete orb;
    }
  catch(const CosNaming::NamingContext::NotFound)
    {
      std::cerr	<< "EXCEPTION: CORBA Naming::NotFound" << std::endl;
      exit(EXIT_FAILURE);
    }
  catch(const CORBA::TRANSIENT& x)
    {
      std::cerr	<< "EXCEPTION: CORBA TRANSIENT" << std::endl;
      exit(EXIT_FAILURE);
    }
  catch(const CORBA::SystemException& x)
    {
      std::cerr	<< "EXCEPTION: CORBA system" << std::endl
		<< x._name() << ' ' << x.NP_minorString() << std::endl;
      exit(EXIT_FAILURE);
    }
  catch(const CORBA::Exception& x)
    {
      std::cerr	<< "EXCEPTION: CORBA" << std::endl
		<< x._name() << std::endl;
      exit(EXIT_FAILURE);
    }

  return EXIT_SUCCESS;
}

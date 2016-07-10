//-*-mode:c++; mode:font-lock;-*-

/**
 * \file MainFailsafe.cpp
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

#include<Messenger.h>
#include<StreamMessenger.h>

#include"ScopeAPI.h"
#include"EIA422.h"
#include"PIUScopeAPI.h"
#include"DataStream.h"
#include"FailsafeUI.h"

#include"MainFailsafe.h"

using namespace ZThread;
using namespace VERITAS;
using namespace VTracking;
using namespace VMessaging;
using namespace VTaskNotification;
using namespace VCorba;

// ============================================================================
// FAILSAFE MEMBER VARIBALES AND FUNCTIONS
// ============================================================================

void MainFailsafe::configure(VSOptions& options)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  // nothing to see here
}

MainFailsafe::MainFailsafe():
  Main(),
  m_verbose(s_default_verbose), 
  m_scope_id(s_default_scope_id),
  m_protocol(P_PIU), 
  m_datastream(s_default_datastream),
  m_earth_pos(), m_stow_pos(), m_az_key()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  TelescopeMotionLimits* limits;
  QColor color;
  setTelescopeInfo(m_scope_id, s_default_protocol, s_default_mainmode,
		   m_protocol, m_datastream, m_earth_pos, m_stow_pos,
		   limits, color);
  delete limits;
}

MainFailsafe::~MainFailsafe()
{
  // nothing to see here
}

int MainFailsafe::main(int argc, char** argv)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  
  StreamMessenger cout_messenger;
  Messenger::relay()->registerMessenger(&cout_messenger);
  BackTrace::catchSignalPrintBacktraceAndAbort(SIGSEGV);

  try
    {
      DataStream* datastream(0);
      EIA422Stub* stub(0);
      ScopeAPI* scopeapi(0);
      
      datastream = DataStream::makeDataStream(m_datastream,m_verbose);

      switch(m_protocol)
	{
	case P_PIU:
	  scopeapi = new PIUScopeAPI(PIUScopeAPI::PV_ARRAY_050901,
				     datastream);
	  break;
	case P_PIU_PROTO:
	  scopeapi = new PIUScopeAPI(PIUScopeAPI::PV_PROTOTYPE,
				     datastream);
	  break;
	case P_EIA:
	  stub = new EIA422Stub(datastream);
	  scopeapi = new ScopeAPIToEIA422Adaptor(stub);
	case P_10M:
	  Debug::stream()
	    << "10m protocol unsupported at this time" << std::endl;
	  assert(0);
	}

      FailsafeUI ui(m_scope_id, datastream, scopeapi, m_earth_pos, 
		    m_stow_pos[0].coords(),
		    m_stow_pos[0].coords().longitudeDeg());
      ui.run();

      delete scopeapi;
      delete stub;
      delete datastream;
    }
  catch(const Exception& x)
    {
      x.print(Debug::stream());
    }
  catch(const Throwable& x)
    {
      Debug::stream() << "Caught Throwable" << std::endl;
    }

  return EXIT_SUCCESS;
}

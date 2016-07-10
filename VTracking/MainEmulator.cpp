//-*-mode:c++; mode:font-lock;-*-

/**
 * \file MainEmulator.cpp
 * \ingroup VTracking
 * \brief This is a one-line description of this cpp file.
 *
 * Here is a tedious verbose multi-line description of all the details
 * of the code, more than you would ever want to read. Generally, all
 * the important documentation goes in the .cpp files.
 *
 * Original Author: Stephen Fegan
 * $Author: sfegan $
 * $Date: 2006/04/27 17:51:55 $
 * $Revision: 2.2 $
 * $Tag$
 *
 **/

#include <zthread/Thread.h>

#include<StreamMessenger.h>

#include"DataStream.h"
#include"ScopeAPI.h"
#include"EIA422.h"
#include"PIUScopeAPI.h"
#include"ScopeProtocolServer.h"
#include"TelescopeEmulator.h"
#include"TelescopeMotionLimits.h"
#include"MainEmulator.h"

using namespace ZThread;
using namespace VERITAS;
using namespace VTracking;
using namespace VMessaging;
using namespace VTaskNotification;
using namespace VCorba;

// ============================================================================
// EMULATOR MEMBER VARIBALES AND FUNCTIONS
// ============================================================================

void MainEmulator::configure(VSOptions& options)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  // nothing to see here
}

MainEmulator::MainEmulator():
  Main(),
  m_verbose(s_default_verbose), 
  m_scope_id(s_default_scope_id),
  m_protocol(P_PIU), 
  m_datastream(s_default_datastream)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  SEphem::SphericalCoords earth_pos;
  StowObjectVector stow_pos;
  TelescopeMotionLimits* limits;
  QColor color;
  setTelescopeInfo(m_scope_id, s_default_protocol, s_default_mainmode,
		   m_protocol, m_datastream, earth_pos, stow_pos, limits,
		   color);
  delete limits;
}

MainEmulator::~MainEmulator()
{
  // nothing to see here
}

int MainEmulator::main(int argc, char** argv)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  StreamMessenger cout_messenger;
  Messenger::relay()->registerMessenger(&cout_messenger);
  BackTrace::catchSignalPrintBacktraceAndAbort(SIGSEGV);

  try
    {
      DataStream* datastream(0);
      datastream = DataStream::makeDataStream(m_datastream,m_verbose,
					      DataStream::OM_SERVER);

      TelescopeEmulator* emulator = new TelescopeEmulator(m_scope_id);
      ScopeProtocolServer* sps = 0;

      switch(m_protocol)
	{
	case P_PIU:
	  sps = new PIUScopeAPI(PIUScopeAPI::PV_ARRAY_050901,
				datastream, 3, emulator);
	  break;
	case P_PIU_PROTO:
	  sps = new PIUScopeAPI(PIUScopeAPI::PV_PROTOTYPE,
				datastream, 3, emulator);
	  break;
	case P_EIA:
	  sps = new EIA422ToScopeAPIAdaptor(emulator,datastream);
	  break;
	case P_10M:
	  Debug::stream()
	    << "10m protocol unsupported at this time" << std::endl;
	  assert(0);
	}

      ScopeProtocolServerLoop* loop = new ScopeProtocolServerLoop(sps);

      {
	ZThread::Thread emulator_thread(emulator);
	ZThread::Thread protocol_thread(loop);
      
	emulator->menu();

	loop->terminate();
	protocol_thread.wait();
	delete sps;
	
	emulator->terminate();
	emulator_thread.wait();
      }

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

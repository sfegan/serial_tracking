// This file is generated by omniidl (C++ backend) - omniORB_4_1. Do not edit.

#include "NET_VGRBMonitor.h"

OMNI_USING_NAMESPACE(omni)

static const char* _0RL_dyn_library_version = omniORB_4_1_dyn;

static ::CORBA::TypeCode::_Tracker _0RL_tcTrack(__FILE__);

#if defined(HAS_Cplusplus_Namespace) && defined(_MSC_VER)
// MSVC++ does not give the constant external linkage otherwise.
namespace VGRBMonitor { 
  const ::CORBA::TypeCode_ptr _tc_Command = CORBA::TypeCode::PR_interface_tc("IDL:VGRBMonitor/Command:1.0", "Command", &_0RL_tcTrack);
} 
#else
const ::CORBA::TypeCode_ptr VGRBMonitor::_tc_Command = CORBA::TypeCode::PR_interface_tc("IDL:VGRBMonitor/Command:1.0", "Command", &_0RL_tcTrack);
#endif

static void _0RL_VGRBMonitor_mCommand_marshal_fn(cdrStream& _s, void* _v)
{
  omniObjRef* _o = (omniObjRef*)_v;
  omniObjRef::_marshal(_o, _s);
}
static void _0RL_VGRBMonitor_mCommand_unmarshal_fn(cdrStream& _s, void*& _v)
{
  omniObjRef* _o = omniObjRef::_unMarshal(VGRBMonitor::Command::_PD_repoId, _s);
  _v = _o;
}
static void _0RL_VGRBMonitor_mCommand_destructor_fn(void* _v)
{
  omniObjRef* _o = (omniObjRef*)_v;
  if (_o)
    omni::releaseObjRef(_o);
}

void operator<<=(::CORBA::Any& _a, VGRBMonitor::Command_ptr _o)
{
  VGRBMonitor::Command_ptr _no = VGRBMonitor::Command::_duplicate(_o);
  _a.PR_insert(VGRBMonitor::_tc_Command,
               _0RL_VGRBMonitor_mCommand_marshal_fn,
               _0RL_VGRBMonitor_mCommand_destructor_fn,
               _no->_PR_getobj());
}
void operator<<=(::CORBA::Any& _a, VGRBMonitor::Command_ptr* _op)
{
  _a.PR_insert(VGRBMonitor::_tc_Command,
               _0RL_VGRBMonitor_mCommand_marshal_fn,
               _0RL_VGRBMonitor_mCommand_destructor_fn,
               (*_op)->_PR_getobj());
  *_op = VGRBMonitor::Command::_nil();
}

::CORBA::Boolean operator>>=(const ::CORBA::Any& _a, VGRBMonitor::Command_ptr& _o)
{
  void* _v;
  if (_a.PR_extract(VGRBMonitor::_tc_Command,
                    _0RL_VGRBMonitor_mCommand_unmarshal_fn,
                    _0RL_VGRBMonitor_mCommand_marshal_fn,
                    _0RL_VGRBMonitor_mCommand_destructor_fn,
                    _v)) {
    omniObjRef* _r = (omniObjRef*)_v;
    if (_r)
      _o = (VGRBMonitor::Command_ptr)_r->_ptrToObjRef(VGRBMonitor::Command::_PD_repoId);
    else
      _o = VGRBMonitor::Command::_nil();
    return 1;
  }
  return 0;
}


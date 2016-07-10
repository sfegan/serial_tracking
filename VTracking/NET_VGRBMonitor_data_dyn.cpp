// This file is generated by omniidl (C++ backend) - omniORB_4_1. Do not edit.

#include "NET_VGRBMonitor_data.h"

OMNI_USING_NAMESPACE(omni)

static const char* _0RL_dyn_library_version = omniORB_4_1_dyn;

static ::CORBA::TypeCode::_Tracker _0RL_tcTrack(__FILE__);


static CORBA::TypeCode_ptr _0RL_tc_GRBNoSuchTrigger = CORBA::TypeCode::PR_exception_tc("IDL:GRBNoSuchTrigger:1.0", "GRBNoSuchTrigger", (CORBA::PR_structMember*) 0, 0, &_0RL_tcTrack);
const CORBA::TypeCode_ptr _tc_GRBNoSuchTrigger = _0RL_tc_GRBNoSuchTrigger;


static CORBA::PR_structMember _0RL_structmember_GRBTrigger[] = {
  {"veritas_unique_sequence_number", CORBA::TypeCode::PR_ulong_tc()},
  {"veritas_receipt_mjd_int", CORBA::TypeCode::PR_ulong_tc()},
  {"veritas_receipt_msec_of_day_int", CORBA::TypeCode::PR_ulong_tc()},
  {"trigger_gcn_sequence_number", CORBA::TypeCode::PR_long_tc()},
  {"trigger_instrument", CORBA::TypeCode::PR_string_tc(0, &_0RL_tcTrack)},
  {"trigger_type", CORBA::TypeCode::PR_string_tc(0, &_0RL_tcTrack)},
  {"trigger_time_mjd_int", CORBA::TypeCode::PR_ulong_tc()},
  {"trigger_msec_of_day_int", CORBA::TypeCode::PR_ulong_tc()},
  {"coord_ra_deg", CORBA::TypeCode::PR_double_tc()},
  {"coord_dec_deg", CORBA::TypeCode::PR_double_tc()},
  {"coord_epoch_J", CORBA::TypeCode::PR_double_tc()},
  {"coord_error_circle_deg", CORBA::TypeCode::PR_double_tc()},
  {"most_likely_source_obj", CORBA::TypeCode::PR_short_tc()},
  {"source_obj_confidence_level", CORBA::TypeCode::PR_short_tc()},
  {"veritas_should_observe", CORBA::TypeCode::PR_boolean_tc()},
  {"veritas_observation_window_hours", CORBA::TypeCode::PR_double_tc()}
};

#ifdef _0RL_tc_GRBTrigger
#  undef _0RL_tc_GRBTrigger
#endif
static CORBA::TypeCode_ptr _0RL_tc_GRBTrigger = CORBA::TypeCode::PR_struct_tc("IDL:GRBTrigger:1.0", "GRBTrigger", _0RL_structmember_GRBTrigger, 16, &_0RL_tcTrack);

const CORBA::TypeCode_ptr _tc_GRBTrigger = _0RL_tc_GRBTrigger;






static CORBA::TypeCode_ptr _0RL_tc_GRBTriggerSeq = CORBA::TypeCode::PR_alias_tc("IDL:GRBTriggerSeq:1.0", "GRBTriggerSeq", CORBA::TypeCode::PR_sequence_tc(0, _0RL_tc_GRBTrigger, &_0RL_tcTrack), &_0RL_tcTrack);


const CORBA::TypeCode_ptr _tc_GRBTriggerSeq = _0RL_tc_GRBTriggerSeq;


static void _0RL_GRBNoSuchTrigger_marshal_fn(cdrStream& _s, void* _v)
{
  const GRBNoSuchTrigger* _p = (const GRBNoSuchTrigger*)_v;
  *_p >>= _s;
}
static void _0RL_GRBNoSuchTrigger_unmarshal_fn(cdrStream& _s, void*& _v)
{
  GRBNoSuchTrigger* _p = new GRBNoSuchTrigger;
  *_p <<= _s;
  _v = _p;
}
static void _0RL_GRBNoSuchTrigger_destructor_fn(void* _v)
{
  GRBNoSuchTrigger* _p = (GRBNoSuchTrigger*)_v;
  delete _p;
}

void operator<<=(::CORBA::Any& _a, const GRBNoSuchTrigger& _s)
{
  GRBNoSuchTrigger* _p = new GRBNoSuchTrigger(_s);
  _a.PR_insert(_0RL_tc_GRBNoSuchTrigger,
               _0RL_GRBNoSuchTrigger_marshal_fn,
               _0RL_GRBNoSuchTrigger_destructor_fn,
               _p);
}
void operator<<=(::CORBA::Any& _a, const GRBNoSuchTrigger* _sp)
{
  _a.PR_insert(_0RL_tc_GRBNoSuchTrigger,
               _0RL_GRBNoSuchTrigger_marshal_fn,
               _0RL_GRBNoSuchTrigger_destructor_fn,
               (GRBNoSuchTrigger*)_sp);
}

::CORBA::Boolean operator>>=(const ::CORBA::Any& _a, const GRBNoSuchTrigger*& _sp)
{
  void* _v;
  if (_a.PR_extract(_0RL_tc_GRBNoSuchTrigger,
                    _0RL_GRBNoSuchTrigger_unmarshal_fn,
                    _0RL_GRBNoSuchTrigger_marshal_fn,
                    _0RL_GRBNoSuchTrigger_destructor_fn,
                    _v)) {
    _sp = (const GRBNoSuchTrigger*)_v;
    return 1;
  }
  return 0;
}

static void _0RL_insertToAny__cGRBNoSuchTrigger(::CORBA::Any& _a, const ::CORBA::Exception& _e) {
  const GRBNoSuchTrigger & _ex = (const GRBNoSuchTrigger &) _e;
  operator<<=(_a,_ex);
}

static void _0RL_insertToAnyNCP__cGRBNoSuchTrigger (::CORBA::Any& _a, const ::CORBA::Exception* _e) {
  const GRBNoSuchTrigger* _ex = (const GRBNoSuchTrigger*) _e;
  operator<<=(_a,_ex);
}

class _0RL_insertToAny_Singleton__cGRBNoSuchTrigger {
public:
  _0RL_insertToAny_Singleton__cGRBNoSuchTrigger() {
    GRBNoSuchTrigger::insertToAnyFn = _0RL_insertToAny__cGRBNoSuchTrigger;
    GRBNoSuchTrigger::insertToAnyFnNCP = _0RL_insertToAnyNCP__cGRBNoSuchTrigger;
  }
};
static _0RL_insertToAny_Singleton__cGRBNoSuchTrigger _0RL_insertToAny_Singleton__cGRBNoSuchTrigger_;

static void _0RL_GRBTrigger_marshal_fn(cdrStream& _s, void* _v)
{
  GRBTrigger* _p = (GRBTrigger*)_v;
  *_p >>= _s;
}
static void _0RL_GRBTrigger_unmarshal_fn(cdrStream& _s, void*& _v)
{
  GRBTrigger* _p = new GRBTrigger;
  *_p <<= _s;
  _v = _p;
}
static void _0RL_GRBTrigger_destructor_fn(void* _v)
{
  GRBTrigger* _p = (GRBTrigger*)_v;
  delete _p;
}

void operator<<=(::CORBA::Any& _a, const GRBTrigger& _s)
{
  GRBTrigger* _p = new GRBTrigger(_s);
  _a.PR_insert(_0RL_tc_GRBTrigger,
               _0RL_GRBTrigger_marshal_fn,
               _0RL_GRBTrigger_destructor_fn,
               _p);
}
void operator<<=(::CORBA::Any& _a, GRBTrigger* _sp)
{
  _a.PR_insert(_0RL_tc_GRBTrigger,
               _0RL_GRBTrigger_marshal_fn,
               _0RL_GRBTrigger_destructor_fn,
               _sp);
}

::CORBA::Boolean operator>>=(const ::CORBA::Any& _a, GRBTrigger*& _sp)
{
  return _a >>= (const GRBTrigger*&) _sp;
}
::CORBA::Boolean operator>>=(const ::CORBA::Any& _a, const GRBTrigger*& _sp)
{
  void* _v;
  if (_a.PR_extract(_0RL_tc_GRBTrigger,
                    _0RL_GRBTrigger_unmarshal_fn,
                    _0RL_GRBTrigger_marshal_fn,
                    _0RL_GRBTrigger_destructor_fn,
                    _v)) {
    _sp = (const GRBTrigger*)_v;
    return 1;
  }
  return 0;
}

static void _0RL_GRBTriggerSeq_marshal_fn(cdrStream& _s, void* _v)
{
  GRBTriggerSeq* _p = (GRBTriggerSeq*)_v;
  *_p >>= _s;
}
static void _0RL_GRBTriggerSeq_unmarshal_fn(cdrStream& _s, void*& _v)
{
  GRBTriggerSeq* _p = new GRBTriggerSeq;
  *_p <<= _s;
  _v = _p;
}
static void _0RL_GRBTriggerSeq_destructor_fn(void* _v)
{
  GRBTriggerSeq* _p = (GRBTriggerSeq*)_v;
  delete _p;
}

void operator<<=(::CORBA::Any& _a, const GRBTriggerSeq& _s)
{
  GRBTriggerSeq* _p = new GRBTriggerSeq(_s);
  _a.PR_insert(_0RL_tc_GRBTriggerSeq,
               _0RL_GRBTriggerSeq_marshal_fn,
               _0RL_GRBTriggerSeq_destructor_fn,
               _p);
}
void operator<<=(::CORBA::Any& _a, GRBTriggerSeq* _sp)
{
  _a.PR_insert(_0RL_tc_GRBTriggerSeq,
               _0RL_GRBTriggerSeq_marshal_fn,
               _0RL_GRBTriggerSeq_destructor_fn,
               _sp);
}

::CORBA::Boolean operator>>=(const ::CORBA::Any& _a, GRBTriggerSeq*& _sp)
{
  return _a >>= (const GRBTriggerSeq*&) _sp;
}
::CORBA::Boolean operator>>=(const ::CORBA::Any& _a, const GRBTriggerSeq*& _sp)
{
  void* _v;
  if (_a.PR_extract(_0RL_tc_GRBTriggerSeq,
                    _0RL_GRBTriggerSeq_unmarshal_fn,
                    _0RL_GRBTriggerSeq_marshal_fn,
                    _0RL_GRBTriggerSeq_destructor_fn,
                    _v)) {
    _sp = (const GRBTriggerSeq*)_v;
    return 1;
  }
  return 0;
}


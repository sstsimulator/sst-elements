// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef TLM_ADAPTERFUNCTIONS_H
#define TLM_ADAPTERFUNCTIONS_H
#include <sst/core/sst_types.h>
#include <sst/core/output.h>
namespace SST{
namespace SysC{
class Adapter_base;
template<typename EVENT>
EVENT* makeEvent(Output& out,
                 tlm::tlm_generic_payload& _trans,
                 Adapter_base* _ad,
                 bool _copy_data)
{out.fatal(CALL_INFO,-1,"Unimplemented makeEvent\n");return NULL;}

template<typename EVENT>
tlm::tlm_generic_payload* makePayload(Output& out,
                                      EVENT& _event,
                                      Adapter_base* _ad,
                                      bool _copy_data)
{out.fatal(CALL_INFO,-1,"Unimplmemnted makePayload\n");return NULL;}

template<typename EVENT>
void updatePayload(Output& out,
                   tlm::tlm_generic_payload& _p,
                   EVENT& _e,
                   Adapter_base* _ad,
                   bool _copy_data)
{out.fatal(CALL_INFO,-1,"Unimplemented updatePayload\n");}

template<typename EVENT>
void updateEvent(Output& out,
                 EVENT& _e,
                 tlm::tlm_generic_payload& _p,
                 Adapter_base* _ad,
                 bool _copy_data)
{out.fatal(CALL_INFO,-1,"Unimplemented updateEvent\n");}

template<typename EVENT>
EVENT* makeResponse(Output& out,
                    tlm::tlm_generic_payload& _p,
                    Adapter_base* _ad)
{out.fatal(CALL_INFO,-1,"Unimplmemented makeResponse\n");return NULL;}

template<typename EVENT>
tlm::tlm_generic_payload* makeResponse(Output& out,
                                       EVENT& _e,
                                       Adapter_base* _ad)
{out.fatal(CALL_INFO,-1,"Unimplemented makeResponse\n");return NULL;}

template<typename EVENT>
uint64_t getAddress(Output& out,
                    EVENT& _e)
{out.fatal(CALL_INFO,-1,"Uniplemented getAddress\n");return 0;}

template<typename EVENT>
void loadPayload(Output& out,
                 EVENT& _e,
                 void *_src,
                 uint32_t _length)
{out.fatal(CALL_INFO,-1,"Unimplmented loadPayload\n");}

template<typename EVENT>
void storePayload(Output& out,
                  EVENT& _e,
                  void *_dest,
                  uint32_t _length)
{out.fatal(CALL_INFO,-1,"Unimplemented storePayload\n");}

template<typename EVENT>
bool isRead(Output& out,
            EVENT& _e)
{out.fatal(CALL_INFO,-1,"Unimplemented isRead\n");return false;}

template<typename EVENT>
bool isWrite(Output& out,
             EVENT& _e)
{out.fatal(CALL_INFO,-1,"Unimplemeneted isWrite\n");return false;}

template<typename EVENT,typename CONNECT>
tlm::tlm_sync_enum sendResponse(Output& out,
                                tlm::tlm_generic_payload& _p,
                                EVENT& _e,
                                Adapter_base* _adapter,
                                CONNECT* _con)
{out.fatal(CALL_INFO,-1,"Unimplemented sendResponse");return tlm::TLM_COMPLETED;}
}//namespace SysC
}//namespace SST
#endif //TLM_ADAPTERFUNCTIONS_H

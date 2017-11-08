// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef __SST_MEMH_EXTMEMBACKENDCONVERTOR__
#define __SST_MEMH_EXTMEMBACKENDCONVERTOR__

#include "sst/elements/memHierarchy/membackend/memBackendConvertor.h"

namespace SST {
namespace MemHierarchy {

class ExtMemBackendConvertor : public MemBackendConvertor {
public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT(ExtMemBackendConvertor, "memHierarchy", "ExtMemBackendConvertor", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Converts MemEventBase* for an ExtMemBackend - passes additional opcode information", "SST::MemHierarchy::MemBackendConvertor")
    
    SST_ELI_DOCUMENT_PARAMS(
            {"debug_level",     "(uint) Debugging level: 0 (no output) to 10 (all output). Output also requires that SST Core be compiled with '--enable-debug'", "0"},
            {"debug_mask",      "(uint) Mask on debug_level", "0"},
            {"debug_location",  "(uint) 0: No debugging, 1: STDOUT, 2: STDERR, 3: FILE", "0"},
            {"request_width",   "(uint) Max size of a request that can be accepted by the memory controller", "64"},
            {"backend",         "Backend memory model to use for timing. Defaults to 'simpleMem'", "memHierarchy.simpleMem"} )

    SST_ELI_DOCUMENT_STATISTICS(
            { "cycles_with_issue",                  "Total cycles with successful issue to back end",   "cycles",   1 },
            { "cycles_attempted_issue_but_rejected","Total cycles where an attempt to issue to backend was rejected (indicates backend full)", "cycles", 1 },
            { "total_cycles",                       "Total cycles called at the memory controller",     "cycles",   1 },
            { "requests_received_GetS",             "Number of GetS (read) requests received",          "requests", 1 },
            { "requests_received_GetSX",            "Number of GetSX (read) requests received",         "requests", 1 },
            { "requests_received_GetX",             "Number of GetX (read) requests received",          "requests", 1 },
            { "requests_received_PutM",             "Number of PutM (write) requests received",         "requests", 1 },
            { "outstanding_requests",               "Total number of outstanding requests each cycle",  "requests", 1 },
            { "latency_GetS",                       "Total latency of handled GetS requests",           "cycles",   1 },
            { "latency_GetSX",                      "Total latency of handled GetSX requests",          "cycles",   1 },
            { "latency_GetX",                       "Total latency of handled GetX requests",           "cycles",   1 },
            { "latency_PutM",                       "Total latency of handled PutM requests",           "cycles",   1 })


/* Class definition */
    ExtMemBackendConvertor(Component *comp, Params &params);

    virtual bool issue( BaseReq* req );
    virtual void handleMemResponse( ReqId reqId, uint32_t flags  ) {
        doResponse( reqId, flags );
    }
};

}
}
#endif

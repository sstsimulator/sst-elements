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


#ifndef __SST_MEMH_SIMPLEMEMSCRATCHBACKENDCONVERTOR__
#define __SST_MEMH_SIMPLEMEMSCRATCHBACKENDCONVERTOR__

#include <sst/core/elementinfo.h>

#include "sst/elements/memHierarchy/membackend/scratchBackendConvertor.h"

namespace SST {
namespace MemHierarchy {

class SimpleMemScratchBackendConvertor : public ScratchBackendConvertor {
public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT(SimpleMemScratchBackendConvertor, "memHierarchy", "simpleMemScratchBackendConvertor", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Convert a MemEventBase to a base MemBacked but uses a different interface than MemBackendConvertor", "SST::MemHierarchy::ScratchBackendConvertor")

    SST_ELI_DOCUMENT_PARAMS(
            {"debug_level",     "(uint) Debugging level: 0 (no output) to 10 (all output). Output also requires that SST Core be compiled with '--enable-debug'", "0"},
            {"debug_mask",      "(uint) Mask on debug_level", "0"},
            {"debug_location",  "(uint) 0: No debugging, 1: STDOUT, 2: STDERR, 3: FILE", "0"},
            {"request_width",   "(uint) Max size of a request that can be accepted by the memory controller", "64"},
            {"backend",         "(string) Backend memory model to use for timing. Defaults to 'simpleMem'", "memHierarchy.simpleMem"} )

    SST_ELI_DOCUMENT_STATISTICS(
            { "cycles_with_issue",                      "Total cycles with successful issue to backend",    "cycles",   1 },
            { "cycles_attempted_issue_but_rejected",    "Total cycles where an issue was attempted but backed rejected it", "cycles", 1},
            { "total_cycles",                           "Total cycles executed at the scratchpad",          "cycles",   1 },
            { "requests_received_GetS",                 "Number of GetS (read) requests received",          "requests", 1 },
            { "requests_received_GetSX",                "Number of GetSX (read) requests received",         "requests", 1 },
            { "requests_received_GetX",                 "Number of GetX (read) requests received",          "requests", 1 },
            { "requests_received_PutM",                 "Number of PutM (write) requests received",         "requests", 1 },
            { "latency_GetS",                           "Total latency of handled GetS requests",           "cycles",   1 },
            { "latency_GetSX",                          "Total latency of handled GetSX requests",          "cycles",   1 },
            { "latency_GetX",                           "Total latency of handled GetX requests",           "cycles",   1 },
            { "latency_PutM",                           "Total latency of handled PutM requests",           "cycles",   1 } )

/* Begin class definition */
    SimpleMemScratchBackendConvertor(Component *comp, Params &params);

    virtual bool issue( MemReq* req );

    virtual void handleMemResponse( ReqId reqId ) {
        SST::Event::id_type respId;
        if ( doResponse( reqId, respId ) ) {
            notifyResponse( respId );
        }
    }
};

}
}
#endif

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


#ifndef _H_SST_MEMH_SIMPLE_DRAM_BACKEND
#define _H_SST_MEMH_SIMPLE_DRAM_BACKEND

#include "membackend/memBackend.h"

namespace SST {
namespace MemHierarchy {

class SimpleDRAM : public SimpleMemBackend {
public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT(SimpleDRAM, "memHierarchy", "simpleDRAM", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Simplified timing model for DRAM", "SST::MemHierarchy::MemBackend")
    
    SST_ELI_DOCUMENT_PARAMS(
            /* Inherited from MemBackend */
            {"debug_level",     "(uint) Debugging level: 0 (no output) to 10 (all output). Output also requires that SST Core be compiled with '--enable-debug'", "0"},
            {"debug_mask",      "(uint) Mask on debug_level", "0"},
            {"debug_location",  "(uint) 0: No debugging, 1: STDOUT, 2: STDERR, 3: FILE", "0"},
            {"clock", "(string) Clock frequency - inherited from MemController", NULL},
            {"max_requests_per_cycle", "(int) Maximum number of requests to accept each cycle. Use 0 or -1 for unlimited.", "-1"},
            {"request_width", "(int) Maximum size, in bytes, for a request", "64"},
            {"mem_size", "(string) Size of memory with units (SI ok). E.g., '2GiB'.", NULL},
            /* Own parameters */
            {"verbose",     "(uint) Sets the verbosity of the backend output", "0" },
            {"cycle_time",  "(string) Latency of a cycle or clock frequency (e.g., '4ns' and '250MHz' are both accepted)", "4ns"},
            {"tCAS",        "(uint) Column access latency in cycles (i.e., access time if correct row is already open)", "9"},
            {"tRCD",        "(uint) Row access latency in cycles (i.e., time to open a row)", "9"},
            {"tRP",         "(uint) Precharge delay in cycles (i.e., time to close a row)", "9"},
            {"banks",       "(uint) Number of banks", "8"},
            {"bank_interleave_granularity", "(string) Granularity of interleaving in bytes (B), generally a cache line. Must be a power of 2.", "64B"},
            {"row_size",    "(string) Size of a row in bytes (B). Must be a power of 2.", "8KiB"},
            {"row_policy",  "(string) Policy for managing the row buffer - open or closed.", "closed"} )

    SST_ELI_DOCUMENT_STATISTICS(
            {"row_already_open","Number of times a request arrived and the correct row was open", "count", 1},
            {"no_row_open",     "Number of times a request arrived and no row was open", "count", 1},
            {"wrong_row_open",  "Number of times a request arrived and the wrong row was open", "count", 1} )


/* Begin class definition */
    SimpleDRAM();
    SimpleDRAM(Component *comp, Params &params);
    bool issueRequest( ReqId, Addr, bool, unsigned );
    bool isClocked() { return false; }

    typedef enum {OPEN, CLOSED, DYNAMIC, TIMEOUT } RowPolicy;

private:
    void handleSelfEvent(SST::Event *event);

    Link *self_link;

    int * openRow;
    bool * busy;
   
    // Mapping parameters
    unsigned int lineOffset;
    unsigned int rowOffset;
    unsigned int bankMask;

    // Time parameters
    unsigned int tCAS;
    unsigned int tRCD;
    unsigned int tRP;

    RowPolicy policy;

    Statistic<uint64_t> * statRowHit;
    Statistic<uint64_t> * statRowMissNoRP;
    Statistic<uint64_t> * statRowMissRP;

public:
    class MemCtrlEvent : public SST::Event {
	public:
        MemCtrlEvent(int bank, ReqId reqId ) : SST::Event(), bank(bank), close(false), reqId(reqId) { }
        MemCtrlEvent(int bank ) : SST::Event(), bank(bank), close(true) { }
        
        int bank;
		bool close;
        ReqId reqId;
    private:
        MemCtrlEvent() {} // For Serialization only
    
    public:
        void serialize_order(SST::Core::Serialization::serializer &ser)  override {
            Event::serialize_order(ser);
            ser & reqId;  // Cannot serialize pointers unless they are a serializable object
            ser & bank;
			ser & close;
        }
        ImplementSerializable(SST::MemHierarchy::SimpleDRAM::MemCtrlEvent);
    };


};

}
}

#endif

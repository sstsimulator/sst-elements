// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _SIMPLE_VECTORSHIFTREG_H
#define _SIMPLE_VECTORSHIFTREG_H

#include <sst/core/component.h>
#include <sst/core/interfaces/simpleMem.h>
#include <sst/core/timeConverter.h>

#include "VecShiftRegister_O1.h"
#include <sst/core/link.h>
#include <sst/core/clock.h>
#include <sst/core/eli/elementinfo.h>
#include "rtlevent.h"
#include "arielrtlev.h"
#include "rtlreadev.h"
#include "rtlwriteev.h"
#include "rtlmemmgr.h"

namespace SST {
    namespace RtlComponent {

class vecShiftReg : public SST::Component {

public:
	vecShiftReg( SST::ComponentId_t id, SST::Params& params );
    ~vecShiftReg();

	void setup();
    void init(unsigned int);
	void finish();

	bool clockTick( SST::Cycle_t currentCycle );

	SST_ELI_REGISTER_COMPONENT(
		vecShiftReg,
		"vecshiftreg",
		"vecShiftReg",
		SST_ELI_ELEMENT_VERSION( 1, 0, 0 ),
		"Demonstration of an External Element for SST",
		COMPONENT_CATEGORY_PROCESSOR
	)
    //Stats needs to be added. Now, stats will be added based on the outputs as mentioned by the user based on the RTL login provided. 
    SST_ELI_DOCUMENT_STATISTICS(
        { "read_requests",        "Statistic counts number of read requests", "requests", 1},   // Name, Desc, Enable Level
        { "write_requests",       "Statistic counts number of write requests", "requests", 1},
        { "read_request_sizes",   "Statistic for size of read requests", "bytes", 1},   // Name, Desc, Enable Level
        { "write_request_sizes",  "Statistic for size of write requests", "bytes", 1},
        { "split_read_requests",  "Statistic counts number of split read requests (requests which come from multiple lines)", "requests", 1},
        { "split_write_requests", "Statistic counts number of split write requests (requests which are split over multiple lines)", "requests", 1},
	    { "flush_requests",       "Statistic counts instructions which perform flushes", "requests", 1},
        { "fence_requests",       "Statistic counts instructions which perform fences", "requests", 1}
    )
    //Parameters will mostly be just frequency/clock in the design. User will mention specifically if there could be other parameters for the RTL design which needs to be configured before runtime.  Don't mix RTL input/control signals with SST parameters. SST parameters of RTL design will make the RTL design/C++ model synchronous with rest of the SST full system.   
	SST_ELI_DOCUMENT_PARAMS(
		{ "ExecFreq", "Clock frequency of RTL design in GHz", "1GHz" },
		{ "maxCycles", "Number of Clock ticks the simulation must atleast execute before halting", "1000" },
        {"memoryinterface", "Interface to memory", "memHierarchy.memInterface"}
	)

    //Default will be single port for communicating with Ariel CPU. Need to see the requirement/use-case of multi-port design and how to incorporate it in our parser tool.  
    SST_ELI_DOCUMENT_PORTS(
        {"ArielRtllink", "Link to the vecShiftReg", { /*"vecShiftReg.RTLEvent", "" */} },
        {"RtlCacheLink", "Link to Cache", {"memHierarchy.memInterface" , ""} }
    )
    
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
            {"memmgr", "Memory manager to translate virtual addresses to physical, handle malloc/free, etc.", "SST::RtlComponent::RtlMemoryManager"},
            {"memory", "Interface to the memoryHierarchy (e.g., caches)", "SST::Interfaces::SimpleMem" }
    )

    void generateReadRequest(RtlReadEvent* rEv);
    void generateWriteRequest(RtlWriteEvent* wEv);
    void setDataAddress(uint8_t*);
    uint8_t* getDataAddress();
    void setBaseDataAddress(uint8_t*);
    uint8_t* getBaseDataAddress();

private:
	SST::Output output;
    
    //RTL Clock frequency of execution and maximum Cycles/clockTicks for which RTL simulation will run.
    std::string RTLClk;
	SST::Cycle_t maxCycles;

    void handleArielEvent(SST::Event *ev);
    void handleMemEvent(Interfaces::SimpleMem::Request* event);
    SST::Link* ArielRtlLink;
    Interfaces::SimpleMem* cacheLink;
    void commitReadEvent(const uint64_t address, const uint64_t virtAddr, const uint32_t length);
    void commitWriteEvent(const uint64_t address, const uint64_t virtAddr, const uint32_t length, const uint8_t* payload);
    void sendArielEvent();
    
    TimeConverter* timeConverter;
    Clock::HandlerBase* clock_handler;
    bool writePayloads;
    RTLEvent ev;
    VecShiftRegister *cmodel;
    SST::ArielComponent::ArielRtlEvent* RtlAckEv;
    size_t inp_size, ctrl_size, updated_rtl_params_size;
    void* inp_ptr = nullptr;
    void* updated_rtl_params = nullptr;
    RtlMemoryManager* memmgr;
    bool mem_allocated = false;
    uint64_t sim_cycle;

    std::unordered_map<Interfaces::SimpleMem::Request::id_t, Interfaces::SimpleMem::Request*>* pendingTransactions;
    std::unordered_map<uint64_t, uint64_t> VA_VA_map;
    uint32_t pending_transaction_count;

    bool isStalled;
    uint64_t cacheLineSize;
    uint8_t *dataAddress, *baseDataAddress;
    
    Statistic<uint64_t>* statReadRequests;
    Statistic<uint64_t>* statWriteRequests;
    Statistic<uint64_t>* statFlushRequests;
    Statistic<uint64_t>* statFenceRequests;
    Statistic<uint64_t>* statReadRequestSizes;
    Statistic<uint64_t>* statWriteRequestSizes;
    Statistic<uint64_t>* statSplitReadRequests;
    Statistic<uint64_t>* statSplitWriteRequests;

    uint64_t tickCount;
    uint64_t dynCycles; 
};

 } //namespace RtlComponent 
} //namespace SST
#endif //_SIMPLE_VECTORSHIFTREG_H

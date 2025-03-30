// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef MEMHIERARCHY_MEMORYCONTROLLER_H
#define MEMHIERARCHY_MEMORYCONTROLLER_H

#include <sst/core/sst_types.h>

#include <sst/core/component.h>
#include <sst/core/event.h>

#include "sst/elements/memHierarchy/memEvent.h"
#include "sst/elements/memHierarchy/cacheListener.h"
#include "sst/elements/memHierarchy/memLinkBase.h"
#include "sst/elements/memHierarchy/membackend/backing.h"
#include "sst/elements/memHierarchy/customcmd/customCmdMemory.h"

namespace SST {
namespace MemHierarchy {

class MemBackendConvertor;

class MemController : public SST::Component {
public:
/* Element Library Info */
    SST_ELI_REGISTER_COMPONENT(MemController, "memHierarchy", "MemController", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Memory controller, interfaces to a main memory model for timing", COMPONENT_CATEGORY_MEMORY)

#define MEMCONTROLLER_ELI_PARAMS {"backend.mem_size",    "(string) Size of physical memory. Must include units in 'B' (SI prefixes ok).", NULL},\
            {"clock",               "(string) Clock frequency of controller", NULL},\
            {"addr_range_start",    "(uint) Lowest address handled by this memory.", "0"},\
            {"addr_range_end",      "(uint) Highest address handled by this memory.", "uint64_t-1"},\
            {"interleave_size",     "(string) Size of interleaved chunks. E.g., to interleave 8B chunks among 3 memories, set size=8B, step=24B", "0B"},\
            {"interleave_step",     "(string) Distance between starting addresses of interleaved chunks. E.g., to interleave 8B chunks among 3 memories, set size=8B, step=24B", "0B"},\
            {"backendConvertor",    "(string) Backend convertor to load. In most cases, you will not need to change this parameter.", "memHierarchy.simpleMembackendConvertor"},\
            {"backend",             "(string) Backend memory model to use for timing.  Defaults to simpleMem", "memHierarchy.simpleMem"},\
            {"request_width",       "(uint) Max request width to the backend in bytes", "64"},\
            {"verbose",             "(uint) Output verbosity for warnings/errors. 0[fatal error only], 1[warnings], 2[full state dump on fatal error]","1"},\
            {"debug_level",         "(uint) Debugging level: 0 to 10. Must configure sst-core with '--enable-debug'. 1=info, 2-10=debug output", "0"},\
            {"debug",               "(uint) 0: No debugging, 1: STDOUT, 2: STDERR, 3: FILE.", "0"},\
            {"debug_addr",          "(comma separated uint) Address(es) to be debugged. Leave empty for all, otherwise specify one or more, comma-separated values. Start and end string with brackets",""},\
            {"listenercount",       "(uint) Counts the number of listeners attached to this controller, these are modules for tracing or components like prefetchers", "0"},\
            {"listener%(listenercount)d", "(string) Loads a listener module into the controller", ""},\
            {"backing",             "(string) Type of backing store to use. Options: 'none' - no backing store (only use if simulation does not require correct memory values), 'malloc', or 'mmap'", "mmap"},\
            {"backing_size_unit",   "(string) For 'malloc' backing stores, malloc granularity", "1MiB"},\
            {"backing_init_zero",   "(string) For 'malloc' backing stores, whether to initialize memory values to 0", "false"},\
            {"memory_file",         "(string) DEPRECATED: Use 'backing_in_file' and/or 'backing_out_file' instead. Optional backing-store file to pre-load memory and/or store resulting state. If file does not exist, the backing-store will create it.", "N/A"},\
            {"backing_in_file",     "(string) An optional file to pre-load memory contents from.", ""},\
            {"backing_out_file",    "(string) An optional file to write out memory contents to. Setting this will also trigger a flush of cache contents prior to writing the file. May be the same as 'backing_in_file'.", ""},\
            {"backing_out_screen",  "(bool) Write out memory contents to screen at end of simulation. Setting this will also trigger a flush of cache contents prior to writing to screen.", "false"},\
            {"customCmdMemHandler", "(string) Name of the custom command handler to load", ""}

    SST_ELI_DOCUMENT_PARAMS( MEMCONTROLLER_ELI_PARAMS )

#define MEMCONTROLLER_ELI_PORTS {"highlink", "Direct connection to another memHierarchy component or subcomponent. If a network port is needed, fill the 'highlink' subcomponent slot instead.", {"memHierarchy.MemEventBase"} },\
            {"direct_link", "DEPRECATED: Use 'highlink' subcomponent or port instead. Direct connection to a cache/directory controller", {"memHierarchy.MemEventBase"} },\
            {"network",     "DEPRECATED: Set 'highlink' subcomponent slot to memHierarchy.MemNIC or memHierarchy.MemNICFour instead. Network connection to a cache/directory controller; also request network for split networks", {"memHierarchy.MemRtrEvent"} },\
            {"network_ack", "DEPRECATED: Set 'highlink' subcomponent slot to memHierarchy.MemNICFour instead. For split networks, ack/response network connection to a cache/directory controller", {"memHierarchy.MemRtrEvent"} },\
            {"network_fwd", "DEPRECATED: Set 'highlink' subcomponent slot to memHierarchy.MemNICFour instead. For split networks, forward request network connection to a cache/directory controller", {"memHierarchy.MemRtrEvent"} },\
            {"network_data","DEPRECATED: Set 'highlink' subcomponent slot to memHierarchy.MemNICFour instead. For split networks, data network connection to a cache/directory controller", {"memHierarchy.MemRtrEvent"} },\
            {"cube_link",   "DEPRECATED. Use named subcomponents and their links instead.", {"sst.Event"} }

    SST_ELI_DOCUMENT_PORTS( MEMCONTROLLER_ELI_PORTS )


#define MEMCONTROLLER_ELI_SUBCOMPONENTSLOTS {"backend", "Backend memory model to use for timing. Defaults to simpleMem", "SST::MemHierarchy::MemBackend"},\
            {"customCmdHandler", "Optional handler for custom command types", "SST::MemHierarchy::CustomCmdMemHandler"}, \
            {"listener", "Optional listeners to gather statistics, create traces, etc. Multiple listeners supported.", "SST::MemHierarchy::CacheListener"}, \
            {"highlink", "CPU-side port manager (e.g., link to caches/cpu). If used, do not connect the 'highlink' port and connect the subcomponent's port(s) instead. Defaults to 'memHierarchy.MemLink' if the 'highlink' port is used instead.", "SST::MemHierarchy.MemLinkBase"},\
            {"cpulink", "DEPRECATED: Renamed to 'highlink' for naming consistency. CPU-side link manager (e.g., towards caches/cpu). A common setting for network connections is 'memHierarchy.MemNIC'.", "SST::MemHierarchy::MemLinkBase"}

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS( MEMCONTROLLER_ELI_SUBCOMPONENTSLOTS )

/* Begin class definition */
    typedef uint64_t ReqId;

    MemController(ComponentId_t id, Params &params);
    virtual void init(unsigned int phase) override;
    virtual void setup() override;
    virtual void complete(unsigned int phase) override;
    void finish() override;

    virtual void handleMemResponse( SST::Event::id_type id, uint32_t flags );

    SST::Cycle_t turnClockOn();

    /* For updating memory values. CustomMemoryCommand should call this */
    void writeData(Addr addr, std::vector<uint8_t>* data);
    void readData(Addr addr, size_t size, std::vector<uint8_t>& data);

protected:
    MemController();  // for serialization only
    virtual ~MemController() {
        if (backing_)
            delete backing_;
    }

    void notifyListeners( MemEvent* ev ) {
        if (  ! listeners_.empty()) {
            // AFR: should this pass the base Addr?
            CacheListenerNotification notify(ev->getAddr(), ev->getAddr(), ev->getVirtualAddress(),
                        ev->getInstructionPointer(), ev->getSize(), READ, HIT);

            for (unsigned long int i = 0; i < listeners_.size(); ++i) {
                listeners_[i]->notifyAccess(notify);
            }
        }
    }

    virtual void handleEvent( SST::Event* );
    virtual void processInitEvent( MemEventInit* );

    virtual bool clock( SST::Cycle_t );

    void adjustRegionToMemSize();

    Output out;
    Output dbg;
    std::set<Addr> DEBUG_ADDR;
    int dlevel;

    MemBackendConvertor*    memBackendConvertor_;
    
    Backend::Backing*       backing_;
    std::string backing_outfile_;

    MemLinkBase* link_;         // Link to the rest of memHierarchy
    bool clockLink_;            // Flag - should we call clock() on this link or not

    std::vector<CacheListener*> listeners_;

    bool isRequestAddressValid(Addr addr){
        return region_.contains(addr);
    }

    void writeData( MemEvent* );
    void readData( MemEvent* );

    std::string checkpointDir_;
    enum { NO_CHECKPOINT, CHECKPOINT_LOAD, CHECKPOINT_SAVE }  checkpoint_;

    size_t memSize_;

    bool clockOn_;

    MemRegion region_; // Which address region we are, for translating to local addresses
    Addr privateMemOffset_; // If we reserve any memory locations for ourselves/directories/etc. and they are NOT part of the physical address space, shift regular addresses by this much
    Addr translateToLocal(Addr addr);
    Addr translateToGlobal(Addr addr);

    Clock::Handler<MemController>* clockHandler_;
    TimeConverter* clockTimeBase_;

    CustomCmdMemHandler * customCommandHandler_;

    /* Debug -triggered by output.fatal() and/or SIGUSR2 */
    virtual void printStatus(Output &out) override;
    virtual void emergencyShutdown() override;

    void printDataValue(Addr addr, std::vector<uint8_t>* data, bool set);

private:

    std::map<SST::Event::id_type, MemEventBase*> outstandingEvents_; // For sending responses. Expect backend to respond to ALL requests so that we know the execution order

    void handleCustomEvent(MemEventBase* ev);

    bool backing_outscreen_;
};

}}

#endif /* _MEMORYCONTROLLER_H */

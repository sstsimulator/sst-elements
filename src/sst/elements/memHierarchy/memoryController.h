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

#ifndef _MEMORYCONTROLLER_H
#define _MEMORYCONTROLLER_H

#include <sst/core/sst_types.h>

#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/elementinfo.h>

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

#define MEMCONTROLLER_ELI_PARAMS {"backend.mem_size",    "(string) Size of physical memory. NEW REQUIREMENT: must include units in 'B' (SI ok). Simple fix: add 'MiB' to old value.", NULL},\
            {"clock",               "(string) Clock frequency of controller", NULL},\
            {"backendConvertor",    "(string) Backend convertor to load", "memHierarchy.simpleMembackendConvertor"},\
            {"backend",             "(string) Backend memory model to use for timing.  Defaults to simpleMem", "memHierarchy.simpleMem"},\
            {"max_requests_per_cycle",  "(int) Maximum number of requests to accept per cycle. 0 or negative is unlimited. Default is 1 for simpleMem backend, unlimited otherwise.", "1"},\
            {"trace_file",          "(string) File name (optional) of a trace-file to generate.", ""},\
            {"verbose",             "(uint) Output verbosity for warnings/errors. 0[fatal error only], 1[warnings], 2[full state dump on fatal error]","1"},\
            {"debug_level",         "(uint) Debugging level: 0 to 10. Must configure sst-core with '--enable-debug'. 1=info, 2-10=debug output", "0"},\
            {"debug",               "(uint) 0: No debugging, 1: STDOUT, 2: STDERR, 3: FILE.", "0"},\
            {"debug_addr",          "(comma separated uint) Address(es) to be debugged. Leave empty for all, otherwise specify one or more, comma-separated values. Start and end string with brackets",""},\
            {"listenercount",       "(uint) Counts the number of listeners attached to this controller, these are modules for tracing or components like prefetchers", "0"},\
            {"listener%(listenercount)d", "(string) Loads a listener module into the controller", ""},\
            {"backing",             "(string) Type of backing store to use. Options: 'none' - no backing store (only use if simulation does not require correct memory values), 'malloc', or 'mmap'", "mmap"},\
            {"backing_size_unit",   "(string) For 'malloc' backing stores, malloc granularity", "1MiB"},\
            {"memory_file",         "(string) Optional backing-store file to pre-load memory, or store resulting state", "N/A"},\
            {"addr_range_start",    "(uint) Lowest address handled by this memory.", "0"},\
            {"addr_range_end",      "(uint) Highest address handled by this memory.", "uint64_t-1"},\
            {"interleave_size",     "(string) Size of interleaved chunks. E.g., to interleave 8B chunks among 3 memories, set size=8B, step=24B", "0B"},\
            {"interleave_step",     "(string) Distance between interleaved chunks. E.g., to interleave 8B chunks among 3 memories, set size=8B, step=24B", "0B"},\
            {"customCmdMemHandler", "(string) Name of the custom command handler to load", ""},\
            {"node",					"Node number in multinode environment"},\
            /* Old parameters - deprecated or moved */\
            {"do_not_back",         "DEPRECATED. Use parameter 'backing' instead.", "0"}, /* Remove 9.0 */\
            {"network_num_vc",      "DEPRECATED. Number of virtual channels (VCs) on the on-chip network. memHierarchy only uses one VC.", "1"}, /* Remove 9.0 */\
            {"coherence_protocol",  "DEPRECATED. No longer needed. Coherence protocol.  Supported: MESI (default), MSI. Only used when a directory controller is not present.", "MESI"}, /* Remove 9.0 */\
            {"network_address",     "DEPRECATED - Now auto-detected by link control."}, /* Remove 9.0 */\
            {"network_bw",          "MOVED. Now a member of the MemNIC subcomponent.", NULL}, /* Remove 9.0 */\
            {"network_input_buffer_size",   "MOVED. Now a member of the MemNIC subcomponent.", "1KiB"}, /* Remove 9.0 */\
            {"network_output_buffer_size",  "MOVED. Now a member of the MemNIC subcomponent.", "1KiB"}, /* Remove 9.0 */\
            {"direct_link_latency", "MOVED. Now a member of the MemLink subcomponent.", "10 ns"}, /* Remove 9.0 */\
            {"request_width",       "MOVED. Now a member of the backendConvertor subcomponent", "64"} /* Remove 9.0 */

    SST_ELI_DOCUMENT_PARAMS( MEMCONTROLLER_ELI_PARAMS )

#define MEMCONTROLLER_ELI_PORTS {"direct_link", "Direct connection to a cache/directory controller", {"memHierarchy.MemEventBase"} },\
            {"network",     "Network connection to a cache/directory controller; also request network for split networks", {"memHierarchy.MemRtrEvent"} },\
            {"network_ack", "For split networks, ack/response network connection to a cache/directory controller", {"memHierarchy.MemRtrEvent"} },\
            {"network_fwd", "For split networks, forward request network connection to a cache/directory controller", {"memHierarchy.MemRtrEvent"} },\
            {"network_data","For split networks, data network connection to a cache/directory controller", {"memHierarchy.MemRtrEvent"} },\
            {"cube_link",   "DEPRECATED. Use named subcomponents and their links instead.", {"sst.Event"} }

    SST_ELI_DOCUMENT_PORTS( MEMCONTROLLER_ELI_PORTS )
    

#define MEMCONTROLLER_ELI_SUBCOMPONENTSLOTS {"backendConvertor", "Convertor to translate incoming memory events for the backend", "SST::MemHierarchy::MemBackendConvertor"},\
            {"customCmdHandler", "Optional handler for custom command types", "SST::MemHierarchy::CustomCmdMemHandler"} 

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS( MEMCONTROLLER_ELI_SUBCOMPONENTSLOTS )

/* Begin class definition */
    typedef uint64_t ReqId;

    MemController(ComponentId_t id, Params &params);
    virtual void init(unsigned int);
    virtual void setup();
    void finish();

    virtual void handleMemResponse( SST::Event::id_type id, uint32_t flags );
    
    SST::Cycle_t turnClockOn();
    
    /* For updating memory values. CustomMemoryCommand should call this */
    void writeData(Addr addr, std::vector<uint8_t>* data);
    void readData(Addr addr, size_t size, std::vector<uint8_t>& data);

protected:
    MemController();  // for serialization only
    ~MemController() {}

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

    Output out;
    Output dbg;
    std::set<Addr> DEBUG_ADDR;

    MemBackendConvertor*    memBackendConvertor_;
    Backend::Backing*       backing_; 

    MemLinkBase* link_;         // Link to the rest of memHierarchy 
    bool clockLink_;            // Flag - should we call clock() on this link or not

    std::vector<CacheListener*> listeners_;
    
    bool isRequestAddressValid(Addr addr){
        return region_.contains(addr);
    }
    
    void writeData( MemEvent* );
    void readData( MemEvent* );

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
    virtual void printStatus(Output &out);
    virtual void emergencyShutdown();

private:
    
    std::map<SST::Event::id_type, MemEventBase*> outstandingEvents_; // For sending responses. Expect backend to respond to ALL requests so that we know the execution order

    void handleCustomEvent(MemEventBase* ev);
};

}}

#endif /* _MEMORYCONTROLLER_H */

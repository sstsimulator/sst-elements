// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2019, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef MEMHIERARCHY_MEMORYCACHECONTROLLER_H
#define MEMHIERARCHY_MEMORYCACHECONTROLLER_H

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

class MemCacheController : public SST::Component {
public:
/* Element Library Info */
    SST_ELI_REGISTER_COMPONENT(MemCacheController, "memHierarchy", "MemCacheController", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Memory controller, interfaces to a main memory model for timing", COMPONENT_CATEGORY_MEMORY)

#define MEMCACHECONTROLLER_ELI_PARAMS {"clock",               "(string) Clock frequency of controller, with units (e.g., 1GHz)", NULL},\
            {"num_caches",          "(uint) Total number of memory caches", "1"},\
            {"cache_num",           "(uint) Index of this cache between 0 and num_caches-1", "0"}, \
            {"cache_line_size",     "(uint) Cache line size in bytes", "64"}, \
            {"backing",             "(string) Type of backing store to use. Options: 'none' - no backing store (only use if simulation does not require correct memory values), 'malloc', or 'mmap'", "mmap"},\
            {"backing_size_unit",   "(string) For 'malloc' backing stores, malloc granularity", "1MiB"},\
            {"memory_file",         "(string) Optional backing-store file to pre-load memory, or store resulting state", "N/A"},\
            {"verbose",             "(uint) Output verbosity for warnings/errors. 0[fatal error only], 1[warnings], 2[full state dump on fatal error]","1"},\
            {"debug",               "(uint) 0: No debugging, 1: STDOUT, 2: STDERR, 3: FILE.", "0"},\
            {"debug_level",         "(uint) Debugging level: 0 to 10. Must configure sst-core with '--enable-debug'. 1=info, 2-10=debug output", "0"},\
            {"debug_addr",          "(comma separated uint) Address(es) to be debugged. Leave empty for all, otherwise specify one or more, comma-separated values. Start and end string with brackets",""}

    SST_ELI_DOCUMENT_PARAMS( MEMCACHECONTROLLER_ELI_PARAMS )

    SST_ELI_DOCUMENT_STATISTICS(
            {"CacheHits_Read",  "Number of read hits", "count", 1},
            {"CacheHits_Write",  "Number of write hits", "count", 1},
            {"CacheMisses_Read",  "Number of read misses", "count", 1},
            {"CacheMisses_Write",  "Number of write misses", "count", 1},
            )

#define MEMCACHE_ELI_SUBCOMPONENTSLOTS {"backend", "Memory controller and/or memory timing model.", "SST::MemHierarchy::MemBackend"},\
            {"backendConvertor", "Convertor to translate incoming memory events for the backend. Loaded automatically based on backend type.", "SST::MemHierarchy::MemBackendConvertor"},\
            {"listener", "Optional listeners to gather statistics, create traces, etc. Multiple listeners supported.", "SST::MemHierarchy::CacheListener"}, \
            {"cpulink", "CPU-side link manager (e.g., to caches/cpu)", "SST::MemHierarchy::MemLinkBase"}

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS( MEMCACHE_ELI_SUBCOMPONENTSLOTS )

/* Begin class definition */
    typedef uint64_t ReqId;

    MemCacheController(ComponentId_t id, Params &params);
    virtual void init(unsigned int);
    virtual void setup();
    void finish();

    virtual void handleLocalMemResponse(SST::Event::id_type id, uint32_t flags);
    
    SST::Cycle_t turnClockOn();
    
    /* For updating memory values. CustomMemoryCommand should call this */
    void writeData(Addr addr, std::vector<uint8_t>* data);
    void readData(Addr addr, size_t size, std::vector<uint8_t>& data);

protected:
    MemCacheController();  // for serialization only
    ~MemCacheController() {}

    /*
     *  HIT: When the response comes back we're done (tag + data or just data)
     *  HIT_TAG: The lookup will be a hit but we need to check the tag first (needed for a write)
     *  MISS: The lookup will be a miss; the current access is to check the tag
     *  MISS_WB: The lookup will be a miss and require a writeback; the current access is to check the tag
     *  STALL: Another access for the same line is outstanding, stall until it finishes -> may not actually be how MCDRAM works...
     *  DATA: Sent a request for data to the remote memroy
     */
    enum class AccessStatus { HIT, HIT_TAG, MISS, MISS_WB, DATA, STALL, FIN };

    struct MemAccessRecord {
        MemEvent* event;
        AccessStatus status;
        MemEvent* reqev;
        
        MemAccessRecord() : event(nullptr), status(AccessStatus::MISS), reqev(nullptr) { }
        MemAccessRecord(MemEvent* ev, AccessStatus stat) : event(ev), status(stat), reqev(nullptr) { }
    };

    std::map<SST::Event::id_type, MemAccessRecord> outstandingEvents_;

    std::map<uint64_t, std::queue<SST::Event::id_type> > mshr_;

    struct CacheState {
        Addr addr;
        State state;
        CacheState(Addr a, State s) : addr(a), state(s) { }
    };

    std::vector<CacheState> cache_;
    Addr lineSize_;
    Addr lineOffset_;

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

    void handleRead(MemEvent* ev, bool replay);
    void handleWrite(MemEvent* ev, bool replay);
    void handleFlush(MemEvent* ev);
    void handleDataResponse(MemEvent* ev);
    void retry(Addr cacheIndex);

    void sendResponse(MemEvent* ev, uint32_t flags);

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
    Addr toLocalAddr(Addr addr);
    
    Clock::Handler<MemCacheController>* clockHandler_;
    TimeConverter* clockTimeBase_;
    
    CustomCmdMemHandler * customCommandHandler_;

    /* Debug -triggered by output.fatal() and/or SIGUSR2 */
    virtual void printStatus(Output &out);
    virtual void emergencyShutdown();

    Statistic<uint64_t>* statReadHit;
    Statistic<uint64_t>* statReadMiss;
    Statistic<uint64_t>* statWriteHit;
    Statistic<uint64_t>* statWriteMiss;

private:
    void handleCustomEvent(MemEventBase* ev);
};

}}

#endif /* _MEMORYCONTROLLER_H */

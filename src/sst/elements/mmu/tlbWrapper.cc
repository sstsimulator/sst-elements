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


#include <sst_config.h>
#include "tlbWrapper.h"

using namespace SST;
using namespace SST::MMU_Lib;

TLB_Wrapper::TLB_Wrapper(SST::ComponentId_t id, SST::Params& params): Component(id)
{
    char buffer[100];
    snprintf(buffer,100,"@t:%s:TLB_Wrapper::@p():@l ",getName().c_str());
    dbg_.init( buffer,
        params.find<int>("debug_level", 0),
        0, // Mask
        Output::STDOUT );

    exe_ = params.find<bool>("exe", 0 );

    cpu_if_ = configureLink("cpu_if", new Event::Handler2<TLB_Wrapper,&TLB_Wrapper::handleCpuEvent>(this));
    if ( nullptr == cpu_if_ ) {
        dbg_.fatal(CALL_INFO, -1, "Error: was unable to configure link `cpu_if`\n");
    }

    cache_if_ = configureLink("cache_if", new Event::Handler2<TLB_Wrapper,&TLB_Wrapper::handleCacheEvent>(this));
    if ( nullptr == cache_if_ ) {
        dbg_.fatal(CALL_INFO, -1, "Error: was unable to configure link `cache_if`\n");
    }

    tlb_ = loadUserSubComponent<SST::MMU_Lib::TLB>("tlb");
    if ( nullptr == tlb_ ) {
        dbg_.fatal(CALL_INFO, -1, "Error: was unable to load subComponent `tlb`\n");
    }

    TLB::Callback callback = std::bind(&TLB_Wrapper::tlbCallback, this, std::placeholders::_1, std::placeholders::_2);
    tlb_->registerCallback( callback );
}

void TLB_Wrapper::init(unsigned int phase)
{
    SST::Event * ev;

    while ((ev = cpu_if_->recvUntimedData())) { //incoming from CPU forward to mem/caches
        cache_if_->sendUntimedData(ev);
    }

    while ((ev = cache_if_->recvUntimedData())) { //incoming from mem/caches forward to the CPU
        auto mem_event_init = dynamic_cast<MemHierarchy::MemEventInit*>(ev);
        if (mem_event_init) {

            //
            if (mem_event_init->getCmd() == MemHierarchy::Command::NULLCMD) {
                if (mem_event_init->getInitCmd() == MemHierarchy::MemEventInit::InitCommand::Coherence) {
                    // Read cache line size
                    MemHierarchy::MemEventInitCoherence * mem_event_init_coherence = static_cast<MemHierarchy::MemEventInitCoherence*>(mem_event_init);
                    if (mem_event_init_coherence->getType() == MemHierarchy::Endpoint::Cache || mem_event_init_coherence->getType() == MemHierarchy::Endpoint::Directory) {
                        line_size_ = mem_event_init_coherence->getLineSize();
                    }
                    // Forward event to standard interface
                    cpu_if_->sendUntimedData(ev);
                } else if (mem_event_init->getInitCmd() == MemHierarchy::MemEventInit::InitCommand::Endpoint) {
                    auto mem_event_init_endpoint = static_cast<MemHierarchy::MemEventInitEndpoint*>(mem_event_init);
                    dbg_.debug(CALL_INFO_LONG,1,0,"Received initEndpoint message: %s\n", mem_event_init_endpoint->getVerboseString(11).c_str());
                    std::vector<std::pair<MemHierarchy::MemRegion,bool>> regions = mem_event_init_endpoint->getRegions();
                    for (auto it = regions.begin(); it != regions.end(); it++) {
                        if (!it->second) {
                            // we don't want to pass this event to the CPU
                            noncacheable_regions_.insert(std::make_pair(it->first.start, it->first));
                        } else {
                            cpu_if_->sendUntimedData(ev);
                        }
                    }
                } else {
                    cpu_if_->sendUntimedData(ev);
                }
            }
        }
    }

    tlb_->init( phase );
}

void TLB_Wrapper::setup()
{
    if ( line_size_ == 0 ) {
        dbg_.fatal(CALL_INFO, -1, "Error: No line size was received during the init() phase\n");
    }
}

void TLB_Wrapper::tlbCallback( RequestID req_id, uint64_t phys_addr )
{
    auto mem_event = reinterpret_cast<MemHierarchy::MemEvent*>( req_id );
    uint64_t addr = mem_event->getAddr();
#ifdef __SST_DEBUG_OUTPUT__
    dbg_.debug(CALL_INFO_LONG,1,0,"reqId=%#" PRIx64 " virtAddr=%#" PRIx64 "  physAddr=%#" PRIx64 "\n", req_id, addr, phys_addr);
#endif

    if( phys_addr == std::numeric_limits<uint64_t>::max() ) {
        auto resp = mem_event->makeResponse();
        static_cast<MemHierarchy::MemEvent*>(resp)->setFail();
        cpu_if_->send( resp );
        delete mem_event;
    } else {

        mem_event->setVirtualAddress( mem_event->getAddr()  );
        mem_event->setBaseAddr(roundDown(phys_addr, line_size_));
        mem_event->setAddr(phys_addr);

        if (!((noncacheable_regions_).empty())) {
            // Check if addr lies in noncacheable regions.
            // For simplicity we are not dealing with the case where the address range splits a noncacheable + cacheable region
            std::multimap<MemHierarchy::Addr, MemHierarchy::MemRegion>::iterator ep = (noncacheable_regions_).upper_bound(phys_addr);
            for (std::multimap<MemHierarchy::Addr, MemHierarchy::MemRegion>::iterator it = (noncacheable_regions_).begin(); it != ep; it++) {
                if (it->second.contains(phys_addr)) {
                    mem_event->setFlag(MemHierarchy::MemEvent::F_NONCACHEABLE);
                    break;
                }
            }
        }

#ifdef __SST_DEBUG_OUTPUT__
        dbg_.debug(CALL_INFO_LONG,1,0,"memEvent thread=%" PRIu32 "  addr=%#" PRIx64 " -> %#" PRIx64 "\n",
                mem_event->getThreadID(), addr, mem_event->getAddr() );
#endif
        cache_if_->send( mem_event );
    }
    --pending_;
}

void TLB_Wrapper::handleCpuEvent( Event* ev )
{
    ++pending_;
    MemHierarchy::MemEvent* mem_event = static_cast<MemHierarchy::MemEvent*>(ev);
#ifdef __SST_DEBUG_OUTPUT__
    dbg_.debug(CALL_INFO_LONG,1,0,"memEvent thread=%" PRIu32 " baseAddr=%#" PRIx64  " addr=%#"  PRIx64  "\n",  mem_event->getThreadID(), mem_event->getBaseAddr(), mem_event->getAddr() );
    dbg_.debug(CALL_INFO_LONG,1,0," %s \n", mem_event->getVerboseString(16).c_str() );
#endif
    auto req_id = reinterpret_cast<RequestID>(mem_event);
    uint32_t hw_thread = mem_event->getThreadID();
    uint64_t virt_addr = mem_event->getAddr(); // getVirtualAddress()
    uint64_t inst_ptr = mem_event->getInstructionPointer();
    uint32_t perms = getPerms( mem_event );

    tlb_->getVirtToPhys( req_id, hw_thread, virt_addr, perms, inst_ptr );
}

void TLB_Wrapper::handleCacheEvent( Event* ev ) {
#ifdef __SST_DEBUG_OUTPUT__
    MemHierarchy::MemEvent* mem_event = dynamic_cast<MemHierarchy::MemEvent*>(ev);
    assert( mem_event );

    dbg_.debug(CALL_INFO_LONG,1,0," %s \n", mem_event->getVerboseString(16).c_str() );
#endif
    cpu_if_->send(ev);
}

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
#include <math.h>
#include "simpleTLB.h"
#include "mmu.h"
#include "utils.h"

using namespace SST;
using namespace SST::MMU_Lib;

SimpleTLB::SimpleTLB(SST::ComponentId_t id, SST::Params& params) : TLB(id,params), page_shift_(0), rng_(72727)
{
    char buffer[100];
    snprintf(buffer,100,"@t:%s:SimpleTLB::@p():@l ",getName().c_str());
    dbg_.init( buffer,
        params.find<int>("debug_level", 0),
        0, // Mask
        Output::STDOUT );

    bool found;
    hit_latency_ = params.find<uint32_t>("hit_latency", 0, found);
    if (!found) {
        hit_latency_ = params.find<uint32_t>("hitLatency", 0, found);
        if (found) {
            dbg_.output("Warning: %s, The parameter 'hitLatency' has been renamed to 'hit_latency' and the old parameter name is deprecated. Update your input file.\n", getName().c_str());
        }
    }

    UnitAlgebra clock;
    try {
        clock = params.find<UnitAlgebra>("clock", "1GHz");
    } catch (UnitAlgebra::UnitAlgebraException& exc) {
        dbg_.fatal(CALL_INFO, -1, "%s, Invalid param: Exception occurred while parsing 'clock'. '%s'\n", getName().c_str(), exc.what());
    }

    if (!clock.hasUnits("s") && !clock.hasUnits("Hz")) {
        dbg_.fatal(CALL_INFO, -1, "%s, Invalid param: 'clock' requires units of seconds ('s') or hertz ('Hz'). Parameter was '%s'.\n",
            getName().c_str(), clock.toStringBestSI().c_str());
    }

    self_link_ = configureSelfLink("selfLink", clock, new Event::Handler2<SimpleTLB,&SimpleTLB::callback>(this));

    uint32_t num_hw_threads = params.find<uint32_t>("num_hardware_threads", 1 );
    if ( 0 == num_hw_threads) {
        dbg_.fatal(CALL_INFO, -1, "Error: num_hardware threads not set\n");
    }

    tlb_size_ = params.find<uint32_t>("num_tlb_entries_per_thread", 32 );
    if ( 0 == tlb_size_ || !isPowerOfTwo(tlb_size_)) {
        dbg_.fatal(CALL_INFO, -1, "Error: num_tlb_entries_per_thread must be non-zero and a power of 2. Got '%" PRIu32 "\n", tlb_size_);
    }

    tlb_set_size_ = params.find<uint32_t>("tlb_set_size", 4 );
    if ( 0 == tlb_set_size_ ) {
        dbg_.fatal(CALL_INFO, -1, "Error: tlb_set_size is not set\n");
    }

    min_virt_addr_ = params.find<uint64_t>("min_virt_addr", 4096, found);
    if (!found) {
        min_virt_addr_ = params.find<uint64_t>("minVirtAddr", 4096, found);
        if (found) {
            dbg_.output("Warning: %s, The parameter 'minVirtAddr' has been renamed to 'min_virt_addr' and the old parameter name is deprecated. Update your input file.\n", getName().c_str());
        }
    }
    max_virt_addr_ = params.find<uint64_t>("max_virt_addr", 0x80000000, found);
    if (!found) {
        max_virt_addr_ = params.find<uint64_t>("maxVirtAddr", 0x80000000, found);
        if (found) {
            dbg_.output("Warning: %s, The parameter 'maxVirtAddr' has been renamed to 'max_virt_addr' and the old parameter name is deprecated. Update your input file.\n", getName().c_str());
        }
    }

    mmu_link_ = configureLink( "mmu", new Event::Handler2<SimpleTLB,&SimpleTLB::handleMMUEvent>(this) );
    if ( nullptr == mmu_link_ ) {
        dbg_.fatal(CALL_INFO, -1, "Error: was unable to configure mmu link\n");
    }

    waiting_miss_.resize( num_hw_threads );
    tlb_data_.resize( num_hw_threads );
    for ( int i=0; i < tlb_data_.size(); i++ ) {
        tlb_data_[i].resize( tlb_size_ );
        for ( int j=0; j < tlb_data_[i].size(); j++ ) {
            tlb_data_[i][j].resize( tlb_set_size_ );
        }
    }
    dbg_.debug(CALL_INFO,1,0,"num_hw_threads=%" PRIu32 " tlb_size=%" PRIu32 " tlb_set_size=%" PRIu32 "\n",num_hw_threads,tlb_size_,tlb_set_size_);
    tlb_index_shift_ = log2( tlb_size_ );
}

void SimpleTLB::init(unsigned int phase)
{
    Event* ev;
    while ((ev = mmu_link_->recvUntimedData())) {
        auto init_event = dynamic_cast<TlbInitEvent*>(ev);
        if ( nullptr == init_event ) {
            dbg_.fatal(CALL_INFO, -1, "Error: received unexpected event in init()\n");
        }
        page_shift_ = init_event->getPageShift();
        page_size_ = 1 << page_shift_;
        dbg_.debug(CALL_INFO,1,0,"page_shift=%" PRIu32 " page_size=%" PRIu32 "\n",page_shift_, page_size_);
        delete ev;
    }
}

void SimpleTLB::handleMMUEvent( Event* ev ) {
    auto req = dynamic_cast<TlbFillEvent*>(ev);
    if ( nullptr == req ) {
        auto req = dynamic_cast<TlbFlushReqEvent*>(ev);
        if ( nullptr != req ) {
            flushThread(req->getHwThread());
            mmu_link_->send( new TlbFlushRespEvent( req->getHwThread() ) );
            return;
        } else {
            assert(0);
        }
    }

    dbg_.debug(CALL_INFO,1,0,"req_id=%#" PRIx64 " ppn=%" PRIu32 " perms=%#" PRIx32 "\n", req->getReqId(), req->getPPN(), req->getPerms() );

    auto record = reinterpret_cast<TlbRecord*>(req->getReqId());
    uint32_t vpn = record->virt_addr >> page_shift_;

    uint64_t phys_addr;
    if( req->isSuccess() ) {
        phys_addr = ((uint64_t) req->getPPN() << page_shift_) | blockOffset( record->virt_addr );
        fillTlbEntry( record->hw_thread_id, vpn, req->getPPN(), req->getPerms() );
    } else {
        phys_addr = std::numeric_limits<uint64_t>::max();
    }

    dbg_.debug(CALL_INFO,1,0,"virt_addr=%#" PRIx64 " phys_addr=%#" PRIx64 " ppn=%" PRIu32 " perms=%#" PRIx32 "\n", record->virt_addr, phys_addr,  req->getPPN(), req->getPerms() );

    // send the first fill response
    self_link_->send( new SelfEvent( record->req_id, phys_addr ));
    auto& waiting = waiting_miss_[record->hw_thread_id];
    waiting[vpn].pop();
    delete record;

    // while there are other misses for this page send them
    while ( ! waiting[vpn].empty() ) {
        auto record = reinterpret_cast<TlbRecord*>(waiting[vpn].front());

        uint64_t phys_addr = ((uint64_t) req->getPPN() << page_shift_) | blockOffset( record->virt_addr );
        if( ! req->isSuccess() ) {
            phys_addr = std::numeric_limits<uint64_t>::max();
        } else {
            TlbEntry* entry = findTlbEntry( record->hw_thread_id, vpn );
            assert(entry);
            if ( ! checkPerms( record->perms, entry->perms() ) ) {
                dbg_.debug(CALL_INFO,1,0,"miss vpn=%" PRIu32 " want=%#" PRIx32 " have=%#" PRIx32 "\n",vpn, record->perms, entry->perms());

                auto id = reinterpret_cast<RequestID>( record );
                mmu_link_->send( new TlbMissEvent( id, record->hw_thread_id, vpn, record->perms, record->inst_ptr, record->virt_addr) );
                delete ev;
                return;
            }
        }
        dbg_.debug(CALL_INFO,1,0,"virt_addr=%#" PRIx64 " phys_addr=%#" PRIx64 "\n", record->virt_addr, phys_addr );

        self_link_->send( new SelfEvent( record->req_id, phys_addr ));
        delete record;
        waiting[vpn].pop();
    }
    waiting.erase(vpn);

    delete ev;
}

void SimpleTLB::getVirtToPhys( RequestID req_id, uint32_t hw_thread_id, uint64_t virt_addr, uint32_t perms, uint64_t inst_ptr ) {
    uint32_t vpn = virt_addr >> page_shift_;
    dbg_.debug(CALL_INFO,1,0,"req_id=%#" PRIx64 ", hw_thread_id=%" PRIu32 " virt_addr=%#" PRIx64 " vpn=%" PRIu32" perms=%#" PRIx32 "\n", req_id, hw_thread_id, virt_addr, vpn, perms);

    if ( virt_addr < min_virt_addr_ || virt_addr > max_virt_addr_ ) {
        dbg_.debug(CALL_INFO,1,0,"virt_addr=%#" PRIx64 " is out of virtual memory range, flag error\n", virt_addr);
        self_link_->send( hit_latency_, new SelfEvent( req_id, std::numeric_limits<uint64_t>::max() ));
        return;
    }

    auto& waiting = waiting_miss_[hw_thread_id];

    TlbEntry* entry = findTlbEntry( hw_thread_id, vpn );

    if ( nullptr != entry && checkPerms( perms, entry->perms() ) && waiting.find( vpn ) == waiting.end()) {

        dbg_.debug(CALL_INFO,1,0,"hit ppn=%zu\n", entry->ppn() );
        uint64_t phys_addr = entry->ppn() << page_shift_ | blockOffset( virt_addr );
        self_link_->send( hit_latency_, new SelfEvent( req_id, phys_addr ));

    } else {
        auto record = new TlbRecord( req_id, hw_thread_id, virt_addr, perms, inst_ptr );
        auto id = reinterpret_cast<RequestID>( record );

        dbg_.debug(CALL_INFO,1,0,"miss id=%#" PRIx64 "\n", id );

        if ( waiting.find( vpn ) == waiting.end() ) {
            dbg_.debug(CALL_INFO,1,0,"miss id=%#" PRIx64 " send to MMU\n", id );
            // we are passing the virt_addr as well as the vpn because we use it for debug with inst_ptr
            // this addition happened after the initial design and it makes VPN uneeded becuse VPN can be deduced at the MMU with virt_addr
            mmu_link_->send( new TlbMissEvent( id, hw_thread_id, vpn, perms, inst_ptr, virt_addr) );
        }
        waiting[vpn].push( id );
    }
}

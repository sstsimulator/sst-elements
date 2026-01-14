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

#ifndef SIMPLE_TLB_H
#define SIMPLE_TLB_H

#include <sst/core/link.h>
#include <sst/core/rng/xorshift.h>

#include "mmuEvents.h"
#include "tlb.h"
#include <queue>

namespace SST {

namespace MMU_Lib {

class SimpleTLB : public TLB {

    class TlbEntry {
      public:
        TlbEntry() : valid_(false) {}
        ~TlbEntry() {}
        void setInvalid() { valid_ = false; }
        bool isValid() { return valid_; }
        bool isDirty() { return dirty_; }
        uint32_t perms() { return perms_; }
        uint32_t tag() { return tag_; }
        uint32_t ppn() { return ppn_; }
        void init( uint32_t tag, uint32_t ppn, uint32_t perms ) {
            tag_ = tag;
            ppn_ = ppn;
            perms_ = perms;
            dirty_ = false;
            valid_ = true;
        }
      private:
        int valid_ : 1;
        int dirty_ : 1;
        uint32_t perms_: 3;
        uint32_t tag_;
        uint32_t ppn_;
    };

    class TlbRecord {
      public:
        TlbRecord( RequestID req_id, uint32_t hw_thread_id, uint64_t virt_addr, uint32_t perms, uint64_t inst_ptr )
            : req_id(req_id), hw_thread_id(hw_thread_id), virt_addr(virt_addr),perms(perms), inst_ptr(inst_ptr) {}
        RequestID req_id;
        uint32_t hw_thread_id;
        uint64_t virt_addr;
        uint32_t perms;
        uint64_t inst_ptr;
    };

    class SelfEvent  : public SST::Event {
      public:

        SelfEvent( RequestID req_id, uint64_t addr ) : Event(), req_id_( req_id ), addr_( addr ) { }
        RequestID getReqId() { return req_id_; }
        uint64_t getAddr() { return addr_; }
      private:
        RequestID req_id_;
        uint64_t addr_;

        NotSerializable(SelfEvent)
    };

  public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        SimpleTLB,
        "mmu",
        "simpleTLB",
        SST_ELI_ELEMENT_VERSION(1, 0, 0),
        "Simple TLB,",
        SST::MMU_Lib::SimpleTLB
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"hit_latency", "Total latency of TLB hit in cycles", "1"},
        {"clock", "Clock period (or cycle time). Determines TLB access latency. Units of 'Hz' or 's' required, SI prefixes OK.", "1GHz"},
        {"debug_level", "Debug verbose level", "0"},
        {"num_hardware_threads", "Number of hardware threads using this TLB", "1"},
        {"num_tlb_entries_per_thread", "Number of TLB sets for each thread. Total entries per thread will be 'num_tlb_entries_per_thread*tlb_set_size'. Must be a power of 2.", "32"},
        {"tlb_set_size", "Number of entries in each TLB set", "4"},
        {"min_virt_addr", "Minimum virtual address to be handled", "4096"},
        {"max_virt_addr", "Maximum virtual address to be handled", "0x80000000"},
        {"minVirtAddr", "DEPRECATED. Use 'min_virt_addr' instead.", "4096"},
        {"maxVirtAddr", "DEPRECATED. Use 'max_virt_addr' instead.", "0x80000000"},
        {"hitLatency", "DEPRECATED. Use 'hit_latency' instead. To update parameter without modifying timing, add one cycle to the value for 'hit_latency'. Clock used to default to 1ns but is now parameterizable and 'hitLatency' was treated as an additional latency beyond a minimum 1ns.", "0"},
    )

    SST_ELI_DOCUMENT_PORTS(
        { "mmu", "", {} },
    )

    SimpleTLB(SST::ComponentId_t id, SST::Params& params);

    virtual void init(unsigned int phase) override;

    void registerCallback( Callback& callback  ) override {
        callback_ = callback;
    }

    void getVirtToPhys( RequestID req_id, uint32_t hw_thread_id, uint64_t virt_addr, uint32_t perms, uint64_t inst_ptr ) override;

  private:
    void callback( Event* ev ) {
        auto self_event = static_cast<SelfEvent*>(ev);
        callback_( self_event->getReqId(), self_event->getAddr() );
        delete ev;
    }

    void handleMMUEvent( Event * );

    uint64_t blockOffset( uint64_t addr ) {
        return addr & ( page_size_ - 1 );
    }

    uint32_t pickVictim() {
        return rng_.generateNextUInt32() % tlb_set_size_;
    }

    void fillTlbEntry( uint32_t hw_thread_id, uint32_t vpn, uint32_t ppn, uint32_t perms ) {
        uint32_t tag = vpn >> tlb_index_shift_;
        uint32_t index = vpn & ( tlb_size_ - 1 );
        auto& vec = tlb_data_[ hw_thread_id ][ index ];

        for ( int i = 0; i<vec.size(); i++ ) {
            if ( vec[i].isValid() ) {
#ifdef __SST_DEBUG_OUTPUT__
                dbg_.debug(CALL_INFO,1,0,"vpn=%" PRIu32 ", tag=%#" PRIx64 " ppn %#lx -> %" PRIu32 ", perms %#" PRIx32 " -> %#" PRIx32 " \n",
                        vpn, (uint64_t) vec[i].tag(), vec[i].ppn(), ppn, vec[i].perms(), perms  );
#endif

                if ( tag == vec[i].tag() ) {
                    vec[ i ].init( tag, ppn, perms );
                    return;
                }
            }
        }

        assert(vpn);
        int slot = pickVictim();
#ifdef __SST_DEBUG_OUTPUT__
        dbg_.debug(CALL_INFO,1,0,"hw_thread=%" PRIu32 " vpn=%" PRIu32 " ppn=%" PRIu32 " tag%#" PRIx64 " index=%#x slot=%" PRIu32 "\n",hw_thread_id,
            vpn, ppn, (uint64_t) tag, index, slot );
#endif
        vec[ slot ].init( tag, ppn, perms );
    }

    TlbEntry* findTlbEntry( uint32_t hw_thread_id, uint32_t vpn ) {
        uint32_t tag = vpn >> tlb_index_shift_;
        uint32_t index = vpn & ( tlb_size_ - 1 );

#ifdef __SST_DEBUG_OUTPUT__
        dbg_.debug(CALL_INFO,1,0,"hw_thread=%" PRIu32 " vpn=%" PRIu32" tag=%#" PRIx64 " index=%#" PRIx32 "\n",
            hw_thread_id, vpn, (uint64_t) tag, index );
#endif

        auto& vec = tlb_data_[ hw_thread_id ][ index ];
        for ( int i = 0; i < vec.size(); i++ ) {

#ifdef __SST_DEBUG_OUTPUT__
            dbg_.debug(CALL_INFO,2,0,"check valid=%d\n",vec[i].isValid());
#endif
            if ( vec[i].isValid() && tag == vec[i].tag() ) {
#ifdef __SST_DEBUG_OUTPUT__
                dbg_.debug(CALL_INFO,1,0,"found tag=%#" PRIx32 " index=%#" PRIx32 " slot=%d\n", tag, index, i );
#endif
                return& vec[i];
            }
        }
        return nullptr;
    }

    void flushThread( uint32_t hw_thread ) {

        auto& slice = tlb_data_[ hw_thread ];
#ifdef __SST_DEBUG_OUTPUT__
        dbg_.debug(CALL_INFO,1,0,"hw_thread=%" PRIu32 " size=%zu\n",hw_thread, slice.size() );
#endif
        for ( int i = 0; i < slice.size(); i++ ) {
            auto& set = slice[i];
            for ( int j = 0; j < set.size(); j++ ) {
                if ( set[j].isValid() ) {
#ifdef __SST_DEBUG_OUTPUT__
                    dbg_.debug(CALL_INFO,1,0,"hw_thread=%" PRIu32 " index=%d set=%d  vpn=%" PRIu32 "\n",
                            hw_thread,i,j, (uint32_t) ( set[j].tag() << tlb_index_shift_ | i ));
#endif
                    set[j].setInvalid();
                }
            }
        }
    }

    Link* self_link_;
    Link* mmu_link_;
    uint32_t hit_latency_;
    uint32_t tlb_size_;     // Use as bitmask, must be power of 2

    uint32_t tlb_set_size_;
    uint32_t page_size_;
    uint32_t page_shift_;
    uint32_t tlb_index_shift_;
    std::vector< std::vector< std::vector< TlbEntry > > > tlb_data_;
    RNG::XORShiftRNG rng_;

    uint64_t min_virt_addr_;
    uint64_t max_virt_addr_;

    std::vector< std::map<uint32_t,std::queue<RequestID> > > waiting_miss_; /* List of pending (vpn, request_queue) for each hw thread*/
};

} //namespace MMU_Lib
} //namespace SST

#endif /* SIMPLE_TLB_H */

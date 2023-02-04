// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
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
        TlbEntry() : m_valid(false) {} 
        ~TlbEntry() {}
        void setInvalid() { m_valid = false; }
        bool isValid() { return m_valid; }
        bool isDirty() { return m_dirty; }
        uint32_t perms() { return m_perms; }
        size_t tag() { return m_tag; }
        size_t ppn() { return m_ppn; }
        void init( size_t tag, size_t ppn, uint32_t perms ) { 
            m_tag = tag;
            m_ppn = ppn;
            m_perms = perms;
            m_dirty = false;
            m_valid = true;
        }
      private:
        int m_valid : 1;
        int m_dirty : 1;
        uint32_t m_perms: 3;
        size_t m_tag : 52; 
        size_t m_ppn : 52;
    };

    class TlbRecord { 
      public:
        TlbRecord( RequestID reqId, int hwThreadId, uint64_t virtAddr, uint32_t perms ) : reqId(reqId), hwThreadId(hwThreadId), virtAddr(virtAddr),perms(perms) {}
        RequestID reqId;
        int hwThreadId;
        uint64_t virtAddr;
        uint32_t perms;
    };

    class SelfEvent  : public SST::Event {
      public:

        SelfEvent( RequestID reqId, uint64_t addr ) : Event(), reqId( reqId ), addr( addr ) { }
        RequestID getReqId() { return reqId; }
        uint64_t getAddr() { return addr; }
      private:
        RequestID reqId;
        uint64_t addr;

        NotSerializable(SelfEvent)
    }; 

  public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(SimpleTLB, "mmu", "simpleTLB",
                                          SST_ELI_ELEMENT_VERSION(1, 0, 0),
                                          "Simple TLB,",
                                          SST::MMU_Lib::SimpleTLB)
    SST_ELI_DOCUMENT_PARAMS(
        {"hitLatency", "latency of TLB hit in ns","0"},
    )

    SST_ELI_DOCUMENT_PORTS(
        { "mmu", "", {} },
    )

    SimpleTLB(SST::ComponentId_t id, SST::Params& params);

    virtual void init(unsigned int phase);

    void registerCallback( Callback& callback  ) {
        m_callback = callback; 
    }

    void getVirtToPhys( RequestID reqId, int hwThreadId, uint64_t virtAddr, uint32_t perms, uint64_t instPtr );

  private:
    void callback( Event* ev ) {
        auto selfEvent = dynamic_cast<SelfEvent*>(ev);
        m_callback( selfEvent->getReqId(), selfEvent->getAddr() ); 
    }

    void handleMMUEvent( Event * );

    uint64_t blockOffset( uint64_t addr ) {
        return addr & ( m_pageSize - 1 );
    }

    int pickVictim() {
        return rng.generateNextUInt32() % m_tlbSetSize;
    }

    void fillTlbEntry( int hwThreadId, size_t vpn, size_t ppn, uint32_t perms ) {
        size_t tag = vpn >> m_tlbIndexShift;
        int index = vpn & ( m_tlbSize - 1 );
        auto& vec = m_tlbData[ hwThreadId ][ index ];
        
        for ( int i = 0; i<vec.size(); i++ ) {
            auto entry = vec[i];
            if ( entry.isValid() ) {
                m_dbg.debug(CALL_INFO,1,0,"vpn=%#lx, tag=%#lx ppn %#lx -> %#lx perms %#x -> %#x \n",entry.tag(), entry.ppn(), ppn, entry.perms(), perms );

                if ( tag == entry.tag() ) {
                    vec[ i ].init( tag, ppn, perms );
                    return;
                }
            }
        } 

        assert(vpn);
        int slot = pickVictim();
        m_dbg.debug(CALL_INFO,1,0,"hwThread=%d vpn=%zu ppn=%zu tag%#x index=%#x slot=%d\n",hwThreadId, vpn, ppn, tag, index, slot );
        vec[ slot ].init( tag, ppn, perms );
    }  

    TlbEntry* findTlbEntry( int hwThreadId, size_t vpn ) {
        size_t tag = vpn >> m_tlbIndexShift;
        int index = vpn & ( m_tlbSize - 1 );

        m_dbg.debug(CALL_INFO,1,0,"hwThread=%d vpn=%zu tag=%#x index=%#x\n",hwThreadId, vpn, tag, index );

        auto& vec = m_tlbData[ hwThreadId ][ index ];
        for ( int i = 0; i < vec.size(); i++ ) {

            m_dbg.debug(CALL_INFO,2,0,"check valid=%d wantTag=%#x foundTag=%#x\n",vec[i].isValid(), tag, vec[i].tag() );
            if ( vec[i].isValid() && tag == vec[i].tag() ) {
                m_dbg.debug(CALL_INFO,1,0,"found tag=%#x index=%#x slot=%d\n",tag, index, i );
                return& vec[i];
            }
        }
        return nullptr;
    }

    void flushThread( int hwThread ) {
    
        auto& slice = m_tlbData[ hwThread ];
        m_dbg.debug(CALL_INFO,1,0,"hwThread=%d size=%zu\n",hwThread,slice.size() );

        for ( int i = 0; i < slice.size(); i++ ) {
            auto& set = slice[i]; 
            //m_dbg.debug(CALL_INFO,1,0,"size=%zu\n",set.size() );
            for ( int j = 0; j < set.size(); j++ ) {  
                if ( set[j].isValid() ) {
                    m_dbg.debug(CALL_INFO,1,0,"hwThread=%d index=%d set=%d vpn=%#x\n",hwThread,i,j, set[j].tag() << m_tlbIndexShift | i );
                    set[j].setInvalid();
                }
            }
        }
    }

    Link* m_selfLink;
    Link* m_mmuLink;
    uint64_t m_hitLatency;
    size_t m_tlbSize; 

    int m_tlbSetSize;
    int m_pageSize;
    int m_pageShift;
    int m_tlbIndexShift;
    std::vector< std::vector< std::vector< TlbEntry > > > m_tlbData;
    RNG::XORShiftRNG rng;

    std::vector< std::map<size_t,std::queue<RequestID> > > m_waitingMiss;
};

} //namespace MMU_Lib
} //namespace SST

#endif /* SIMPLE_TLB_H */

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

#include <sst_config.h>
#include <math.h>
#include "simpleTLB.h"
#include "mmu.h"
#include "utils.h"

using namespace SST;
using namespace SST::MMU_Lib;

SimpleTLB::SimpleTLB(SST::ComponentId_t id, SST::Params& params) : TLB(id,params), m_pageShift(0)
{
    char buffer[100];
    snprintf(buffer,100,"@t:%s:SimpleTLB::@p():@l ",getName().c_str());
    m_dbg.init( buffer,
        params.find<int>("debug_level", 0),
        0, // Mask
        Output::STDOUT );

    m_selfLink = configureSelfLink("selfLink", "1 ns", new Event::Handler<SimpleTLB>(this,&SimpleTLB::callback));
    m_hitLatency = params.find<int>("hitLatency", 0);

    int numHwThreads = params.find<int>("num_hardware_threads", 0 );
    if ( 0 == numHwThreads) {
        m_dbg.fatal(CALL_INFO, -1, "Error: num_hardware threads not set\n");
    }

    m_tlbSize = params.find<int>("num_tlb_entries_per_thread", 0 );
    if ( 0 == m_tlbSize ) {
        m_dbg.fatal(CALL_INFO, -1, "Error: num_tlb_entreis_per_thread is not set\n");
    } 

    m_tlbSetSize = params.find<int>("tlb_set_size", 0 );
    if ( 0 == m_tlbSetSize ) {
        m_dbg.fatal(CALL_INFO, -1, "Error: tlb_set_size is not set\n");
    } 

    m_mmuLink = configureLink( "mmu", new Event::Handler<SimpleTLB>(this, &SimpleTLB::handleMMUEvent) );
    if ( nullptr == m_mmuLink ) {
        m_dbg.fatal(CALL_INFO, -1, "Error: was unable to configure mmu link\n");
    }
    m_waitingMiss.resize( numHwThreads );
    m_tlbData.resize( numHwThreads );
    for ( int i=0; i < m_tlbData.size(); i++ ) {
        m_tlbData.at(i).resize( m_tlbSize );
        for ( int j=0; j < m_tlbData.at(i).size(); j++ ) {
            m_tlbData.at(i).at(j).resize( m_tlbSetSize );
        }
    }
    m_dbg.debug(CALL_INFO,1,0,"numHwTHreads=%d tlbSize=%d tlbSetSize=%d\n",numHwThreads,m_tlbSize,m_tlbSetSize);
    m_tlbIndexShift = log2( m_tlbSize );
}

void SimpleTLB::init(unsigned int phase) 
{
    m_dbg.debug(CALL_INFO,2,0,"phase=%d\n",phase);
    Event* ev;
    while ((ev = m_mmuLink->recvInitData())) { 
        auto initEvent = dynamic_cast<TlbInitEvent*>(ev);
        if ( nullptr == initEvent ) {
            m_dbg.fatal(CALL_INFO, -1, "Error: received unexpected event in init()\n");
        } 
        m_pageShift = initEvent->getPageShift();
        m_pageSize = 1 << m_pageShift;
        m_dbg.debug(CALL_INFO,1,0,"pageShift=%d pageSize=%d\n",m_pageShift, 1 << m_pageShift);
        delete ev;
    }
}

void SimpleTLB::handleMMUEvent( Event* ev ) {
    auto req = dynamic_cast<TlbFillEvent*>(ev);
    if ( nullptr == req ) {
        auto req = dynamic_cast<TlbFlushEvent*>(ev);
        if ( nullptr != req ) {
            flushThread(req->getHwThread());
            return;
        } else {
            assert(0);
        } 
    }

    m_dbg.debug(CALL_INFO,1,0,"reqId=%#" PRIx64 " ppn=%zu perms=%#x\n", req->getReqId(), req->getPPN(), req->getPerms() );

    auto record = reinterpret_cast<TlbRecord*>(req->getReqId());
    size_t vpn = record->virtAddr >> m_pageShift;

    uint64_t physAddr;
    if( req->isSuccess() ) {
        physAddr = req->getPPN() << m_pageShift | blockOffset( record->virtAddr );
        fillTlbEntry( record->hwThreadId, vpn, req->getPPN(), req->getPerms() );  
    } else {
        physAddr = -1;
    } 

    m_dbg.debug(CALL_INFO,1,0,"virtAddr=%#" PRIx64 " physAddr=%#" PRIx64 " ppn=%d perms=%#x\n", record->virtAddr, physAddr,  req->getPPN(), req->getPerms() );

    // send the first fill response 
    m_selfLink->send( 0, new SelfEvent( record->reqId, physAddr ));
    auto& waiting = m_waitingMiss.at(record->hwThreadId);
    waiting.at(vpn).pop();
    delete record;

    // while there are other misses for this page send them 
    while ( ! waiting.at(vpn).empty() ) {
        auto record = reinterpret_cast<TlbRecord*>(waiting.at(vpn).front());

        uint64_t physAddr = req->getPPN() << m_pageShift | blockOffset( record->virtAddr );
        if( ! req->isSuccess() ) {
            physAddr = -1;
        } else {
            TlbEntry* entry = findTlbEntry( record->hwThreadId, vpn );
            assert(entry);
            if ( record->perms & 0x2 != entry->perms() & 0x2 ) {
                printf("%s() %#lx %#x %#x\n",__func__,vpn, record->perms, entry->perms());
                fflush(stdout);
                assert( 0);
            }
        }
        m_dbg.debug(CALL_INFO,1,0,"virtAddr=%#" PRIx64 " physAddr=%#" PRIx64 "\n", record->virtAddr, physAddr );

        m_selfLink->send( 0, new SelfEvent( record->reqId, physAddr ));
        delete record;
        waiting.at(vpn).pop();
    }
    waiting.erase(vpn);

    delete ev;
}

void SimpleTLB::getVirtToPhys( RequestID reqId, int hwThreadId, uint64_t virtAddr, uint32_t perms, uint64_t instPtr ) {
    size_t vpn = virtAddr >> m_pageShift;
    m_dbg.debug(CALL_INFO,1,0,"reqId=%p, hwThreadId=%d virtAddr=%#" PRIx64 " vpn=%d perms=%#x\n", reqId, hwThreadId, virtAddr, vpn, perms);

    auto& waiting = m_waitingMiss.at(hwThreadId); 

    TlbEntry* entry = findTlbEntry( hwThreadId, vpn );

    if ( nullptr != entry && checkPerms( perms, entry->perms() ) && waiting.find( vpn ) == waiting.end()) {

        m_dbg.debug(CALL_INFO,1,0,"hit ppn=%zu\n", entry->ppn() );
        uint64_t physAddr = entry->ppn() << m_pageShift | blockOffset( virtAddr );
        m_selfLink->send( m_hitLatency, new SelfEvent( reqId, physAddr ));

    } else {
        auto record = new TlbRecord( reqId, hwThreadId, virtAddr, perms );
        auto id = reinterpret_cast<RequestID>( record );

        m_dbg.debug(CALL_INFO,1,0,"miss id=%#" PRIx64 "\n", id );

        if ( waiting.find( vpn ) == waiting.end() ) {
            m_dbg.debug(CALL_INFO,1,0,"miss id=%#" PRIx64 " send to MMU\n", id );
            // we are passing the virtAddr as well as the vpn because we use it for debug with instPtr
            // this addition happened after the initial design and it makes VPN uneeded becuse VPN can be deduced at the MMU with virtAddr
            m_mmuLink->send( 0, new TlbMissEvent( id, hwThreadId, vpn, perms, instPtr, virtAddr) );
        }
        waiting[vpn].push( id );
    }
}

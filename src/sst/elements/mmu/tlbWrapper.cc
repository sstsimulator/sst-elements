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
#include "tlbWrapper.h"

using namespace SST;
using namespace SST::MMU_Lib;

TLB_Wrapper::TLB_Wrapper(SST::ComponentId_t id, SST::Params& params): Component(id), m_pending(0) 
{
    char buffer[100];
    snprintf(buffer,100,"@t:%s:TLB_Wrapper::@p():@l ",getName().c_str());
    m_dbg.init( buffer,
        params.find<int>("debug_level", 0),
        0, // Mask
        Output::STDOUT );

    m_exe = params.find<bool>("exe", 0 );

    m_cpu_if = configureLink("cpu_if", new Event::Handler<TLB_Wrapper>(this, &TLB_Wrapper::handleCpuEvent));
    if ( nullptr == m_cpu_if ) {
        m_dbg.fatal(CALL_INFO, -1, "Error: was unable to configure link `cpu_if`\n");
    }

    m_cache_if = configureLink("cache_if", new Event::Handler<TLB_Wrapper>(this, &TLB_Wrapper::handleCacheEvent));
    if ( nullptr == m_cache_if ) {
        m_dbg.fatal(CALL_INFO, -1, "Error: was unable to configure link `cache_if`\n");
    }

    m_tlb = loadUserSubComponent<SST::MMU_Lib::TLB>("tlb");
    if ( nullptr == m_tlb ) {
        m_dbg.fatal(CALL_INFO, -1, "Error: was unable to load subComponent `tlb`\n");
    }

    TLB::Callback callback = std::bind(&TLB_Wrapper::tlbCallback, this, std::placeholders::_1, std::placeholders::_2);
    m_tlb->registerCallback( callback );
}

void TLB_Wrapper::init(unsigned int phase) 
{
    SST::Event * ev;
    while ((ev = m_cpu_if->recvInitData())) { //incoming from CPU, forward down
        m_cache_if->sendInitData(ev);
    }
    while ((ev = m_cache_if->recvInitData())) { //incoming from mem/caches, forward up
        m_cpu_if->sendInitData(ev);
    }

    m_tlb->init( phase );
}

void TLB_Wrapper::tlbCallback( RequestID reqId, uint64_t physAddr ) 
{
    auto memEv = reinterpret_cast<MemHierarchy::MemEvent*>( reqId );
    uint64_t addr = memEv->getAddr();
    m_dbg.debug(CALL_INFO_LONG,1,0,"reqId=%#" PRIx64 " virtAddr=%#" PRIx64 "  physAddr=%#" PRIx64 "\n",reqId,addr,physAddr);

    if( physAddr == -1 ) {
        auto resp = memEv->makeResponse();
        static_cast<MemHierarchy::MemEvent*>(resp)->setFail();
        m_cpu_if->send( resp );
        delete memEv;
    } else {

        memEv->setVirtualAddress( memEv->getAddr()  );
        memEv->setBaseAddr(roundDown(physAddr,64));
        memEv->setAddr(physAddr);

        m_dbg.debug(CALL_INFO_LONG,1,0,"memEvent thread=%d  addr=%#" PRIx64 " -> %#" PRIx64 "\n",
                memEv->getThreadId(), addr, memEv->getAddr() );

        m_cache_if->send( memEv );
    }
    --m_pending;
}

void TLB_Wrapper::handleCpuEvent( Event* ev )
{
    ++m_pending;
    MemHierarchy::MemEvent* memEv = dynamic_cast<MemHierarchy::MemEvent*>(ev);
    m_dbg.debug(CALL_INFO_LONG,1,0,"memEvent thread=%d baseAddr=%#" PRIx64  " addr=%#"  PRIx64  "\n",  memEv->getThreadId(), memEv->getBaseAddr(), memEv->getAddr() );
    m_dbg.debug(CALL_INFO_LONG,1,0," %s \n", memEv->getVerboseString(16).c_str() );

    auto reqId = reinterpret_cast<RequestID>(memEv);
    int hwThread = memEv->getThreadId();
    uint64_t virtAddr = memEv->getAddr(); // getVirtualAddress() 
    uint64_t instPtr = memEv->getInstructionPointer();
    uint32_t perms = getPerms( memEv );

    m_tlb->getVirtToPhys( reqId, hwThread, virtAddr, perms, instPtr );
}

void TLB_Wrapper::handleCacheEvent( Event* ev ) {
    MemHierarchy::MemEvent* memEv = dynamic_cast<MemHierarchy::MemEvent*>(ev);
    assert( memEv );

    m_dbg.debug(CALL_INFO_LONG,1,0," %s \n", memEv->getVerboseString(16).c_str() );
    m_cpu_if->send(ev);
}

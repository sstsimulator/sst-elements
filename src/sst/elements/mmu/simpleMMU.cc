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
#include "simpleMMU.h"
#include "mmuEvents.h"
#include "utils.h"

using namespace SST;
using namespace SST::MMU_Lib;

SimpleMMU::SimpleMMU(SST::ComponentId_t id, SST::Params& params) : MMU(id,params)
{
    char buffer[100];
    snprintf(buffer,100,"@t:SimpleMMU::@p():@l ");
    m_dbg.init( buffer,
        params.find<int>("debug_level", 0),
        0, // Mask
        Output::STDOUT );

    m_dbg.debug(CALL_INFO_LONG,1,0,"num_cores=%d num_hw_threads=%d\n",m_numCores,m_numHwThreads);
    m_coreToPid.resize( m_numCores );
    for ( unsigned i = 0; i < m_coreToPid.size(); i++ ) {
        m_coreToPid.at(i).resize( m_numHwThreads, -1 );
    }
}

void SimpleMMU::handleTlbEvent( Event* ev, int link ) 
{
    int core = getTlbCore( link );
    auto req = dynamic_cast<TlbMissEvent*>(ev);
    assert( req );

    unsigned hwThread = req->getHardwareThread();

    unsigned pid = getPid( core, hwThread );

    m_dbg.debug(CALL_INFO_LONG,1,0,"event on link=%d name=%s core=%d pid=%d vpn=%d perms=%#x\n",
        link,getTlbName(link).c_str(),core,pid,req->getVPN(),req->getPerms());
    
    m_dbg.debug(CALL_INFO_LONG,1,0,"reqId=%d hwTHread=%d vpn=%zu %#" PRIx64 "\n", req->getReqId(), req->getHardwareThread(), req->getVPN(), req->getVPN() << 12  );
    m_permissionsCallback( req->getReqId(), link, core, hwThread, pid, req->getVPN(), req->getPerms(), req->getInstPtr(), req->getMemAddr() );
    delete ev;
}

void SimpleMMU::map( unsigned pid, uint32_t vpn, uint32_t ppn, int pageSize, uint64_t flags ) 
{
    m_dbg.debug(CALL_INFO_LONG,1,0,"pid=%d vpn=%d ppn=%d pageSize=%d flags=%#" PRIx64 "\n", pid, vpn, ppn, pageSize, flags );
    auto pageTable = getPageTable(pid);
    assert( pageTable );

    pageTable->add( vpn, PTE( ppn, flags ) );
}

void SimpleMMU::map( unsigned pid, uint32_t vpn, std::vector<uint32_t>& ppns, int pageSize, uint64_t flags ) {

    m_dbg.debug(CALL_INFO_LONG,1,0,"pid=%d vpn=%d numPages=%zu pageSize=%d flags=%#" PRIx64 "\n", pid, vpn, ppns.size(), pageSize, flags );
    assert(0);
}


void SimpleMMU::removeWrite( unsigned pid ) {
    m_dbg.debug(CALL_INFO_LONG,1,0,"pid=%d\n",pid);
    auto table = getPageTable(pid);
    assert( table );
    table->removeWrite();
}

void SimpleMMU::dup( unsigned fromPid, unsigned toPid ) {
    m_dbg.debug(CALL_INFO_LONG,1,0,"fromPid=%d toPid=%d\n",fromPid,toPid);
    auto fromTable = getPageTable(fromPid);
    //fromTable->print("from");
    //printf("%s() %p\n",__func__,fromTable);
    assert( fromTable );

    auto newTable = new PageTable( *fromTable );
    //newTable->print("new");
    initPageTable( toPid, newTable );
} 

void SimpleMMU::flushTlb( unsigned core, unsigned hwThread ) {
    m_dbg.debug(CALL_INFO_LONG,1,0,"core=%d hwThread=%d\n",core,hwThread);
//    sendEvent( getLink(core,"itlb"), new TlbFlushEvent( hwThread ) );
    sendEvent( getLink(core,"dtlb"), new TlbFlushEvent( hwThread ) );
} 

void SimpleMMU::faultHandled( RequestID requestId, unsigned link, unsigned pid, unsigned vpn, bool success ) {

    if ( success ) {
        auto pageTable = getPageTable(pid);
        assert( pageTable );
        m_dbg.debug(CALL_INFO_LONG,1,0,"link=%d vpn=%#x virtAddr=%#lx ppn=%#x\n",link, vpn, vpn<<12, pageTable->find( vpn )->ppn );
        sendEvent( link, new TlbFillEvent( requestId, *pageTable->find( vpn ) ) );
    } else {
        m_dbg.debug(CALL_INFO_LONG,1,0,"link=%d vpn=%#x failed\n",link,vpn);
        sendEvent( link, new TlbFillEvent( requestId ) );
    }
} 

int SimpleMMU::getPerms( unsigned pid, uint32_t vpn ) {
    auto pageTable = getPageTable(pid);
    assert( pageTable );
    PTE* pte = pageTable->find( vpn );
    if ( nullptr == pte ) { 
        return -1;
    } 
    return  pte->perms; 
}


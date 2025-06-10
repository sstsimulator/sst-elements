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
        m_coreToPid[i].resize( m_numHwThreads, -1 );
    }
}

void SimpleMMU::handleNicTlbEvent( Event* ev )
{
    if( dynamic_cast<TlbFlushRespEvent*>(ev) ) {
        // we currently don't do anything with the response because there are no race conditions?
        delete ev;
        return;
    }

    auto req = dynamic_cast<TlbMissEvent*>(ev);
    assert(req);
    auto link = -1;
    auto core = 0;
    auto hwThread = 0;
    unsigned pid = getPid( 0, 0 );

    m_dbg.debug(CALL_INFO_LONG,1,0,"event on link=%d name=%s core=%d pid=%d vpn=%zu perms=%#x\n",
        link,"nicTlb",core,pid,req->getVPN(),req->getPerms());

    m_dbg.debug(CALL_INFO_LONG,1,0,"reqId=%" PRIu64 " hwTHread=%d vpn=%zu %#" PRIx64 "\n", req->getReqId(), req->getHardwareThread(), req->getVPN(), (uint64_t) req->getVPN() << 12  );
    m_permissionsCallback( req->getReqId(), link, core, hwThread, pid, req->getVPN(), req->getPerms(), req->getInstPtr(), req->getMemAddr() );
    delete ev;
}

void SimpleMMU::handleTlbEvent( Event* ev, int link )
{
    int core = getTlbCore( link );

    if( dynamic_cast<TlbFlushRespEvent*>(ev) ) {
        // we currently don't do anything with the response because there are no race conditions?
        delete ev;
        return;
    }

    auto req = dynamic_cast<TlbMissEvent*>(ev);
    assert( req );

    unsigned hwThread = req->getHardwareThread();

    unsigned pid = getPid( core, hwThread );

    m_dbg.debug(CALL_INFO_LONG,1,0,"event on link=%d name=%s core=%d hwThread=%d pid=%d vpn=%zu perms=%#x\n",
        link,getTlbName(link).c_str(),core,hwThread,pid,req->getVPN(),req->getPerms());

    m_dbg.debug(CALL_INFO_LONG,1,0,"reqId=%" PRIu64 " hwTHread=%d vpn=%zu %#" PRIx64 "\n", req->getReqId(), req->getHardwareThread(), req->getVPN(), (uint64_t) req->getVPN() << 12  );
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

void SimpleMMU::unmap( unsigned pid, uint32_t vpn, size_t numPages ) {
    m_dbg.debug(CALL_INFO_LONG,1,0,"pid=%d vpn=%d numPages=%zu\n", pid, vpn, numPages );
    auto pageTable = getPageTable(pid);
    assert( pageTable );
    for ( auto i = 0; i < numPages; i++ ) {
        pageTable->remove( vpn + i );
    }
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
    sendEvent( getLink(core,"dtlb"), new TlbFlushReqEvent( hwThread ) );
    if ( m_nicTlbLink ) {
        m_nicTlbLink->send(0,new TlbFlushReqEvent( hwThread ) );
    }
}

void SimpleMMU::faultHandled( RequestID requestId, unsigned link, unsigned pid, unsigned vpn, bool success ) {

    if ( success ) {
        auto pageTable = getPageTable(pid);
        assert( pageTable );
        m_dbg.debug(CALL_INFO_LONG,1,0,"link=%d vpn=%#x virtAddr=%#" PRIx64 " ppn=%#x\n",
            link, vpn, (uint64_t) vpn<<12, pageTable->find( vpn )->ppn );
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

void SimpleMMU::checkpoint( std::string dir ) {

    std::stringstream filename;
    filename << dir << "/" << getName();
    auto fp = fopen(filename.str().c_str(),"w+");

    m_dbg.debug(CALL_INFO_LONG,1,MMU_DBG_CHECKPOINT,"Checkpoint component `%s` %s\n",getName().c_str(), filename.str().c_str());

    fprintf(fp,"m_pageTableMap.size() %zu\n",m_pageTableMap.size());
    for ( auto & x : m_pageTableMap ) {
        auto pageTable = x.second;
        fprintf(fp,"pid: %i\n",x.first);
        pageTable->checkpoint( fp );
    }

    fprintf(fp,"m_coreToPid.size() %zu\n", m_coreToPid.size());
    for ( auto core = 0; core < m_coreToPid.size(); core++ ) {
        auto& x = m_coreToPid[core];
        fprintf(fp,"core: %d, numPids: %zu\n",core,x.size());
        for ( auto j = 0; j < x.size(); j++ ) {
            fprintf(fp,"%d ",x[j]);
        }
        fprintf(fp,"\n");
    }
}

void SimpleMMU::checkpointLoad( std::string dir ) {
    std::stringstream filename;
    filename << dir << "/" << getName();
    auto fp = fopen(filename.str().c_str(),"r");
    assert(fp);

    m_dbg.debug(CALL_INFO_LONG,1,MMU_DBG_CHECKPOINT,"Checkpoint load component `%s` %s\n",getName().c_str(), filename.str().c_str());

    int size;
    assert( 1 == fscanf( fp, "m_pageTableMap.size() %d\n",&size) );
    m_dbg.debug(CALL_INFO_LONG,1,MMU_DBG_CHECKPOINT,"m_pageTableMap.size() %d\n",size);
    for ( auto i = 0; i < size; i++ ) {
        int pid;
        assert( 1 == fscanf( fp, "pid: %d\n",&pid) );
        m_dbg.debug(CALL_INFO_LONG,1,MMU_DBG_CHECKPOINT,"pid: %d\n",pid);
        m_pageTableMap[pid] = new PageTable( &m_dbg, fp );
    }

    assert( 1 == fscanf( fp, "m_coreToPid.size() %d\n", &size) );
    m_dbg.debug(CALL_INFO_LONG,1,MMU_DBG_CHECKPOINT,"m_coreToPid.size() %d\n",size );

    m_coreToPid.resize( size );
    for ( auto core = 0; core < m_coreToPid.size(); core++ ) {
        int x, numPids;
        assert( 2 == fscanf( fp, "core: %d, numPids: %d\n", &x, &numPids) );
        m_dbg.debug(CALL_INFO_LONG,1,MMU_DBG_CHECKPOINT, "core: %d, numPids: %d\n", x, numPids);
        m_coreToPid[core].resize( numPids );
        for ( auto i = 0; i < numPids; i++ ) {
            int pid;
            assert( 1 == fscanf(fp,"%d ",&pid) );
            m_dbg.debug(CALL_INFO_LONG,1,MMU_DBG_CHECKPOINT,"%d ",pid);
            m_coreToPid[core][i] =  pid;
        }
        m_dbg.debug(CALL_INFO_LONG,1,MMU_DBG_CHECKPOINT,"\n");
    }
}

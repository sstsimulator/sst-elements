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
    dbg_.init( buffer,
        params.find<int>("debug_level", 0),
        0, // Mask
        Output::STDOUT );

    dbg_.debug(CALL_INFO_LONG,1,0,"num_cores=%" PRIu32 " num_hw_threads=%" PRIu32 "\n",num_cores_,num_hw_threads_);
    core_to_pid_.resize( num_cores_ );
    for ( size_t i = 0; i < core_to_pid_.size(); i++ ) {
        core_to_pid_[i].resize( num_hw_threads_, std::numeric_limits<uint32_t>::max() );
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
    auto link = std::numeric_limits<uint32_t>::max();
    auto core = 0;
    auto hw_thread = 0;
    uint32_t pid = getPid( 0, 0 );

#ifdef __SST_DEBUG_OUTPUT__
    dbg_.debug(CALL_INFO_LONG,1,0,"event on link=%" PRIu32 " name=%s core=%" PRIu32 " pid=%" PRIu32 " vpn=%" PRIu32 " perms=%#" PRIx32 "\n",
        link,"nicTlb",core,pid,req->getVPN(),req->getPerms());

    dbg_.debug(CALL_INFO_LONG,1,0,"reqId=%" PRIu64 " hw_thread=%" PRIu32 " vpn=%" PRIu32 " %#" PRIx64 "\n", req->getReqId(), req->getHardwareThread(), req->getVPN(), (uint64_t) req->getVPN() << 12  );
#endif
    permissions_callback_( req->getReqId(), link, core, hw_thread, pid, req->getVPN(), req->getPerms(), req->getInstPtr(), req->getMemAddr() );
    delete ev;
}

void SimpleMMU::handleTlbEvent( Event* ev, int link )
{
    uint32_t core = getTlbCore( link );

    if( dynamic_cast<TlbFlushRespEvent*>(ev) ) {
        // we currently don't do anything with the response because there are no race conditions?
        delete ev;
        return;
    }

    auto req = dynamic_cast<TlbMissEvent*>(ev);
    assert( req );

    uint32_t hw_thread = req->getHardwareThread();

    uint32_t pid = getPid( core, hw_thread );

#ifdef __SST_DEBUG_OUTPUT__
    dbg_.debug(CALL_INFO_LONG,1,0,"event on link=%" PRIu32 " name=%s core=%" PRIu32 " hw_thread=%" PRIu32 " pid=%" PRIu32 " vpn=%" PRIu32 " perms=%#" PRIx32 "\n",
        link,getTlbName(link).c_str(),core,hw_thread,pid,req->getVPN(),req->getPerms());

    dbg_.debug(CALL_INFO_LONG,1,0,"reqId=%" PRIu64 " hw_thread=%" PRIu32 " vpn=%" PRIu32 " %#" PRIx64 "\n", req->getReqId(), req->getHardwareThread(), req->getVPN(), (uint64_t) req->getVPN() << 12  );
#endif
    permissions_callback_( req->getReqId(), link, core, hw_thread, pid, req->getVPN(), req->getPerms(), req->getInstPtr(), req->getMemAddr() );
    delete ev;
}

void SimpleMMU::map( uint32_t pid, uint32_t vpn, uint32_t ppn, uint32_t page_size, uint64_t flags )
{
#ifdef __SST_DEBUG_OUTPUT__
    dbg_.debug(CALL_INFO_LONG,1,0,"pid=%" PRIu32 " vpn=%" PRIu32 " ppn=%" PRIu32 " page_size=%" PRIu32 " flags=%#" PRIx64 "\n", pid, vpn, ppn, page_size, flags );
#endif
    auto page_table = getPageTable(pid);
    assert( page_table );

    page_table->add( vpn, PTE( ppn, flags ) );
}

void SimpleMMU::map( uint32_t pid, uint32_t vpn, std::vector<uint32_t>& ppns, uint32_t page_size, uint64_t flags ) {

    dbg_.fatal(CALL_INFO_LONG,-1,"pid=%" PRIu32 " vpn=%" PRIu32 " num_pages=%zu page_size=%" PRIu32 " flags=%#" PRIx64 "\n", pid, vpn, ppns.size(), page_size, flags );
}

void SimpleMMU::unmap( uint32_t pid, uint32_t vpn, size_t num_pages ) {
#ifdef __SST_DEBUG_OUTPUT__
    dbg_.debug(CALL_INFO_LONG,1,0,"pid=%" PRIu32 " vpn=%" PRIu32 " num_pages=%zu\n", pid, vpn, num_pages );
#endif
    auto page_table = getPageTable(pid);
    assert( page_table );
    for ( auto i = 0; i < num_pages; i++ ) {
        page_table->remove( vpn + i );
    }
}

void SimpleMMU::removeWrite( uint32_t pid ) {
#ifdef __SST_DEBUG_OUTPUT__
    dbg_.debug(CALL_INFO_LONG,1,0,"pid=%" PRIu32 "\n",pid);
#endif
    auto table = getPageTable(pid);
    assert( table );
    table->removeWrite();
}

void SimpleMMU::dup( uint32_t from_pid, uint32_t to_pid ) {
#ifdef __SST_DEBUG_OUTPUT__
    dbg_.debug(CALL_INFO_LONG,1,0,"from_pid=%" PRIu32 " to_pid=%" PRIu32 "\n",from_pid,to_pid);
#endif
    auto from_table = getPageTable(from_pid);
    assert( from_table );

    auto new_table = new PageTable( *from_table );
    initPageTable( to_pid, new_table );
}

void SimpleMMU::flushTlb( uint32_t core, uint32_t hw_thread ) {
#ifdef __SST_DEBUG_OUTPUT__
    dbg_.debug(CALL_INFO_LONG,1,0,"core=%" PRIu32 " hw_thread=%" PRIu32 "\n",core,hw_thread);
#endif
    sendEvent( getLink(core,"dtlb"), new TlbFlushReqEvent( hw_thread ) );
    if ( nic_tlb_link_ ) {
        nic_tlb_link_->send(new TlbFlushReqEvent( hw_thread ) );
    }
}

void SimpleMMU::faultHandled( RequestID request_id, uint32_t link, uint32_t pid, uint32_t vpn, bool success ) {

    if ( success ) {
        auto page_table = getPageTable(pid);
        assert( page_table );
#ifdef __SST_DEBUG_OUTPUT__
        dbg_.debug(CALL_INFO_LONG,1,0,"link=%" PRIu32 " vpn=%#" PRIu32 " virtAddr=%#" PRIx64 " ppn=%#" PRIx32 "\n",
            link, vpn, (uint64_t) vpn<<12, page_table->find( vpn )->ppn );
#endif
        sendEvent( link, new TlbFillEvent( request_id, *page_table->find( vpn ) ) );
    } else {
#ifdef __SST_DEBUG_OUTPUT__
        dbg_.debug(CALL_INFO_LONG,1,0,"link=%" PRIu32 " vpn=%#" PRIx32 " failed\n",link,vpn);
#endif
        sendEvent( link, new TlbFillEvent( request_id ) );
    }
}

void SimpleMMU::snapshot( std::string dir ) {

    std::stringstream filename;
    filename << dir << "/" << getName();
    auto fp = fopen(filename.str().c_str(),"w+");

    dbg_.debug(CALL_INFO_LONG,1,MMU_DBG_SNAPSHOT,"Snapshot component `%s` %s\n",getName().c_str(), filename.str().c_str());

    fprintf(fp,"page_table_map_.size() %zu\n",page_table_map_.size());
    for ( auto & x : page_table_map_ ) {
        auto page_table = x.second;
        fprintf(fp,"pid: %" PRIu32 "\n",x.first);
        page_table->snapshot( fp );
    }

    fprintf(fp,"core_to_pid_.size() %zu\n", core_to_pid_.size());
    for ( auto core = 0; core < core_to_pid_.size(); core++ ) {
        auto& x = core_to_pid_[core];
        fprintf(fp,"core: %" PRIu32 ", numPids: %zu\n",core,x.size());
        for ( auto j = 0; j < x.size(); j++ ) {
            fprintf(fp,"%" PRIu32 " ",x[j]);
        }
        fprintf(fp,"\n");
    }
}

void SimpleMMU::snapshotLoad( std::string dir ) {
    std::stringstream filename;
    filename << dir << "/" << getName();
    auto fp = fopen(filename.str().c_str(),"r");
    assert(fp);

    dbg_.debug(CALL_INFO_LONG,1,MMU_DBG_SNAPSHOT,"Snapshot load component `%s` %s\n",getName().c_str(), filename.str().c_str());

    size_t size;
    assert( 1 == fscanf( fp, "page_table_map_.size() %zu\n",&size) );
    dbg_.debug(CALL_INFO_LONG,1,MMU_DBG_SNAPSHOT,"page_table_map_.size() %zu\n",size);
    for ( auto i = 0; i < size; i++ ) {
        uint32_t pid;
        assert( 1 == fscanf( fp, "pid: %" PRIu32 "\n",&pid) );
        dbg_.debug(CALL_INFO_LONG,1,MMU_DBG_SNAPSHOT,"pid: %" PRIu32 "\n",pid);
        page_table_map_[pid] = new PageTable( &dbg_, fp );
    }

    assert( 1 == fscanf( fp, "core_to_pid_.size() %zu\n", &size) );
    dbg_.debug(CALL_INFO_LONG,1,MMU_DBG_SNAPSHOT,"core_to_pid_.size() %zu\n",size );

    core_to_pid_.resize( size );
    for ( auto core = 0; core < core_to_pid_.size(); core++ ) {
        uint32_t x;
        size_t numPids;
        assert( 2 == fscanf( fp, "core: %" PRIu32 ", numPids: %zu\n", &x, &numPids) );
        dbg_.debug(CALL_INFO_LONG,1,MMU_DBG_SNAPSHOT, "core: %" PRIu32 ", numPids: %zu\n", x, numPids);
        core_to_pid_[core].resize( numPids );
        for ( auto i = 0; i < numPids; i++ ) {
            uint32_t pid;
            assert( 1 == fscanf(fp,"%" PRIu32 " ",&pid) );
            dbg_.debug(CALL_INFO_LONG,1,MMU_DBG_SNAPSHOT,"%" PRIu32 " ",pid);
            core_to_pid_[core][i] =  pid;
        }
        dbg_.debug(CALL_INFO_LONG,1,MMU_DBG_SNAPSHOT,"\n");
    }
}

// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include <sst/core/serialization.h>

#include <DRAMSimWrap2.h>
#include <m5.h>
#include <paramHelp.h>

using namespace SST::M5;

extern "C" {
    SimObject* create_DRAMSimWrap2( SST::Component*, std::string, SST::Params& );
}

SimObject* create_DRAMSimWrap2( SST::Component* comp, string name,
                                                    SST::Params& sstParams )
{
    DRAMSimWrap2::Params& params   = *new DRAMSimWrap2::Params;

    params.name = name;
    
    INIT_HEX( params, sstParams, backdoorRange.start );
    INIT_HEX( params, sstParams, backdoorRange.end );
    INIT_HEX( params, sstParams, rank );
    INIT_HEX( params, sstParams, range.start );
    INIT_HEX( params, sstParams, range.end );
    INIT_STR( params, sstParams, info );
    INIT_STR( params, sstParams, debug );
    INIT_STR( params, sstParams, frequency );
    INIT_STR( params, sstParams, pwd );
    INIT_STR( params, sstParams, printStats);
    INIT_STR( params, sstParams, deviceIniFilename );
    INIT_STR( params, sstParams, systemIniFilename );

    params.m5Comp = static_cast< M5* >( static_cast< void* >( comp ) );
    params.exeParams = sstParams.find_prefix_params("exe.");

    return new DRAMSimWrap2( &params );
}

DRAMSimWrap2::DRAMSimWrap2( const Params* p ) :
    DRAMSimWrap( p ),
    m_rank( p->rank ),
    m_sharedRange( p->backdoorRange ),
    m_backdoor( NULL )
{
    int m_numRanks = 2;
    Addr size = m_sharedRange.end - m_sharedRange.start;
    DBGX(2,"my rank %d num ranks %d\n", m_rank, m_numRanks ); 
    DBGX(2,"start %#lx end %#lx %#lx\n", m_sharedRange.start, 
                                    m_sharedRange.end, size );

    m_myRange.start = m_sharedRange.start + (size / m_numRanks) * m_rank; 
    m_myRange.end = m_myRange.start + size / m_numRanks; 

    DBGX(2,"my_start %#lx my_end %#lx %#lx\n", m_myRange.start, 
                                    m_myRange.end, size / m_numRanks );
}

Port* DRAMSimWrap2::getPort(const std::string &if_name, int idx )
{
    PRINTFN("%s: if_name=`%s` idx=%d\n", __func__, if_name.c_str(), idx );

    if (if_name == "backdoor") {
        assert( ! m_backdoor);
        return m_backdoor = new backdoorPort( name() + ".backdoor", this);
    }

    if (if_name == "functional") {
        return new MemPort(name() + "." + if_name, this);
    }
    
    assert( ! m_port);

    if (if_name != "port") {
        panic("DRAMSimWrap::getPort: unknown port %s requested", if_name);
    }

    return m_port = new MemPort( name() + "." + if_name, this );
}

bool DRAMSimWrap2::recvTiming(PacketPtr pkt )
{
    if ( pkt->getAddr() >= m_sharedRange.start && 
                        pkt->getAddr() < m_sharedRange.end )
    {
        if ( !( pkt->getAddr() >= m_myRange.start &&
                        pkt->getAddr() < m_myRange.end ) ) {

            PRINTFN("%s: backdoor send %#lx\n",__func__,pkt->getAddr());
            DBGX(4,"backdoor %s needsResp=%s %#lx\n",pkt->cmdString().c_str(),
               pkt->needsResponse() ? "yes" : "no", pkt->getAddr());
            return m_backdoor->sendTiming( pkt );
        }
    } 
    PRINTFN("%s: %s %#lx\n",__func__,pkt->cmdString().c_str(),pkt->getAddr());
    //DBGX(4,"%s %#lx\n",__func__,pkt->cmdString().c_str(),pkt->getAddr());
    return DRAMSimWrap::recvTiming( pkt );
}

bool DRAMSimWrap2::recvTimingBackdoor( PacketPtr pkt )
{
    DBGX(4,"%s needsResp=%s %#lx\n",pkt->cmdString().c_str(),
                pkt->needsResponse() ? "yes" : "no", pkt->getAddr());

    if ( pkt->isResponse() ) {
        PRINTFN("%s: %s %#lx\n",__func__, pkt->cmdString().c_str(),
                                                        pkt->getAddr());
        DBGX(4,"%s %#lx\n", pkt->cmdString().c_str(), pkt->getAddr());
        return m_port->sendTiming(pkt);
    }

    PRINTFN("%s: %s %#lx do access\n",__func__,
                        pkt->cmdString().c_str(),pkt->getAddr());

    DBGX(4,"%s %#lx do access\n",pkt->cmdString().c_str(),pkt->getAddr());

    doFunctionalAccess( pkt );

    return m_backdoor->sendTiming( pkt ); 
} 

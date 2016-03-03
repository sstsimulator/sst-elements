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

#include <DRAMSimWrap.h>
#include <m5.h>
#include <paramHelp.h>
#include <loadMemory.h>

// DEBUG might be defined for M5, include DRAMSim/MemorySystem.h last
// and undefined DEBUG because DRAMSim defines it
#undef DEBUG
#include	<MultiChannelMemorySystem.h>

#define MS_CAST( x ) static_cast<DRAMSim::MultiChannelMemorySystem*>(x)


using namespace SST::M5;

extern "C" {
    SimObject* create_DRAMSimWrap( SST::Component*, std::string, SST::Params& );
}

SimObject* create_DRAMSimWrap( SST::Component* comp, string name,
                                                    SST::Params& sstParams )
{
    DRAMSimWrap::Params& params   = *new DRAMSimWrap::Params;

    params.name = name;

    INIT_HEX( params, sstParams, range.start );
    INIT_HEX( params, sstParams, range.end );
    INIT_STR( params, sstParams, info );
    INIT_STR( params, sstParams, debug );
    INIT_STR( params, sstParams, frequency );
    INIT_STR( params, sstParams, pwd );
    INIT_STR( params, sstParams, printStats);
    INIT_STR( params, sstParams, deviceIniFilename );
    INIT_STR( params, sstParams, systemIniFilename );
    INIT_STR( params, sstParams, megsOfMemory );

    params.m5Comp = static_cast< M5* >( static_cast< void* >( comp ) );
    params.exeParams = sstParams.find_prefix_params("exe");

    return new DRAMSimWrap( &params );
}

DRAMSimWrap::DRAMSimWrap( const Params* p ) :
    PhysicalMemory( p ),
    m_port( NULL ),
    m_waitRetry( false ),
    m_comp( p->m5Comp ),
    m_dbg( *new Log< DRAMSIMC_DBG >( cerr, "DRAMSimC::", false ) ),
    m_log( *new Log< >( cout, "INFO DRAMSimC: ", false ) )
{
    unsigned megsOfMemory = 4096;
    DBGX( 2, "name=`%s`\n", name().c_str() );

    if ( p->info.compare( "yes" ) == 0 ) {
        m_log.enable();
    }

    if ( p->debug.compare( "yes" ) == 0 ) {
        m_dbg.enable();
    }

    loadMemory( name(), this, p->exeParams );

    string deviceIniFilename = p->pwd + "/" + p->deviceIniFilename;
    string systemIniFilename = p->pwd + "/" + p->systemIniFilename;

    stringstream(p->megsOfMemory) >> megsOfMemory;

    m_log.write("device ini %s\n",deviceIniFilename.c_str());
    m_log.write("system ini %s\n",systemIniFilename.c_str());

    m_log.write("freq %s\n",p->frequency.c_str());
    m_log.write("megsOfMemory %lu\n",megsOfMemory);

    m_memorySystem = new DRAMSim::MultiChannelMemorySystem(/* 0, */ 
            deviceIniFilename, systemIniFilename, "", "", megsOfMemory );

    DRAMSim::Callback<DRAMSimWrap, void, uint, uint64_t,uint64_t >* readDataCB;
    DRAMSim::Callback<DRAMSimWrap, void, uint, uint64_t,uint64_t >* writeDataCB;

    readDataCB = new DRAMSim::Callback
        < DRAMSimWrap, void, uint, uint64_t,uint64_t > 
                        (this, &DRAMSimWrap::readData);

    writeDataCB = new DRAMSim::Callback
        < DRAMSimWrap, void, uint, uint64_t,uint64_t >
                        (this, &DRAMSimWrap::writeData);

    MS_CAST( m_memorySystem )->RegisterCallbacks(readDataCB, writeDataCB, NULL);

    SST::SimTime_t delay = SST::Simulation::getSimulation()->
                            getTimeLord()->getSimCycles( p->frequency,"");
    
    m_tickEvent = new TickEvent( this, delay );

    unsigned long freqHz = ( ( 1.0 / delay ) * 1000000000000 ); 
    printf("DRAMSimWrap clock %lu hz\n", freqHz );
    MS_CAST( m_memorySystem )->setCPUClockSpeed(freqHz);
}

DRAMSimWrap::~DRAMSimWrap()
{
    delete &m_log;
    delete &m_dbg;
}

Port * DRAMSimWrap::getPort(const std::string &if_name, int idx )
{
    PRINTFN("%s: if_name=`%s` idx=%d\n", __func__, if_name.c_str(), idx );

    if (if_name == "functional") {
        return new MemoryPort(csprintf("%s-functional", name()), this);
    }

    assert( ! m_port );

    if (if_name != "port") {
        panic("DRAMSimWrap::getPort: unknown port %s requested", if_name);
    }

    return m_port = new MemPort( name() + "." + if_name, this );
}

void DRAMSimWrap::init()
{
    if ( ! m_port ) {
        panic("DRAMSimWrap object %s is unconnected!", name());
    }

    m_port->sendStatusChange(Port::RangeChange);
}

bool DRAMSimWrap::recvTiming(PacketPtr pkt)
{
    uint64_t addr    = pkt->getAddr();
    bool     isWrite = pkt->isWrite();
    bool     isRead = pkt->isRead();

    PRINTFN("%s: %s %#lx\n", __func__, pkt->cmdString().c_str(), (long)addr);

    if ( ! MS_CAST( m_memorySystem )->willAcceptTransaction( addr ) ) {
        return false; 
    } 

    assert( addr &= ~( JEDEC_DATA_BUS_BITS - 1 ) );

    pkt->dram_enter_time = curTick();

    if ( isRead ) { 
        assert(m_rd_pktMap.find( addr ) == m_rd_pktMap.end());
        m_rd_pktMap[ addr ] = pkt;
        MS_CAST( m_memorySystem )->addTransaction( isWrite, addr );
    } else if ( isWrite ) {
        std::multimap< uint64_t, PacketPtr >::iterator it;
        it = m_wr_pktMap.find( addr );
        assert( it == m_wr_pktMap.end());
        m_wr_pktMap.insert( pair<uint64_t, PacketPtr>(addr, pkt) );
        MS_CAST( m_memorySystem )->addTransaction( isWrite, addr );
    } else {
        if ( pkt->needsResponse() ) {
            pkt->makeTimingResponse();
            m_readyQ.push_back( pkt );
        } else {
            delete pkt;
        }
    }
    return true;
}

void DRAMSimWrap::recvRetry()
{
    PRINTFN("%s:\n",__func__);
    assert( ! m_readyQ.empty() );

    m_waitRetry = false;
    sendTry();
}

void DRAMSimWrap::sendTry()
{
    while ( ! m_readyQ.empty() && ! m_waitRetry ) {
        if ( ! m_port->sendTiming( m_readyQ.front() )  ) {
            PRINTFN("%s: sendTiming failed\n",__func__);
            m_waitRetry = true;
        } else {
            PRINTFN("%s: sendTiming succeeded\n",__func__);
            m_readyQ.pop_front();
        }
    }
}

void DRAMSimWrap::readData(uint id, uint64_t addr, uint64_t clockcycle)
{
    PRINTFN("%s: id=%d addr=%#lx clock=%lu\n",__func__,id,addr,clockcycle);

    assert( m_rd_pktMap.find( addr ) != m_rd_pktMap.end() );

    PacketPtr    pkt   = m_rd_pktMap[addr];

    m_rd_lat.sample( curTick() - pkt->dram_enter_time );

    PRINTFN("%s() `%s` addr=%#lx size=%d\n", __func__, pkt->cmdString().c_str(),
                                        (long)pkt->getAddr(), pkt->getSize());

    PhysicalMemory::doAtomicAccess( pkt );

    m_readyQ.push_back(pkt);

    m_rd_pktMap.erase( addr );
}

void DRAMSimWrap::writeData(uint id, uint64_t addr, uint64_t clockcycle)
{
    PRINTFN("%s: id=%d addr=%#lx clock=%lu\n",__func__,id,addr,clockcycle);

    std::multimap< uint64_t, PacketPtr >::iterator it =
                    m_wr_pktMap.find( addr );
    assert( it != m_wr_pktMap.end() );

    PacketPtr    pkt   = it->second;

    m_wr_lat.sample( curTick() - pkt->dram_enter_time );

    PRINTFN("%s() `%s` addr=%#lx size=%d\n", __func__, pkt->cmdString().c_str(),
                                        (long)pkt->getAddr(), pkt->getSize());

    PhysicalMemory::doAtomicAccess( pkt );

    if ( pkt->needsResponse() ) {
        m_readyQ.push_back(pkt);
    } else {
        delete pkt;
    }

    m_wr_pktMap.erase( addr );
}

void DRAMSimWrap::regStats()
{
    using namespace Stats;

    m_rd_lat
        .init(0,250000,5000)
        .name(name() + ".rt_read_lat")
        .desc("Round trip latency for a dram read req")
        .precision(2)
        ;

    m_wr_lat
        .init(0,250000,5000)
        .name(name() + ".rt_write_lat")
        .desc("Round trip latency for a dram write req")
        .precision(2)
        ;
}


void DRAMSimWrap::tick()
{
    MS_CAST( m_memorySystem )->update();

    sendTry();
}

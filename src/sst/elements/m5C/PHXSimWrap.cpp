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

#include <PHXSimWrap.h>
#include <m5.h>
#include <paramHelp.h>
#include <loadMemory.h>

using namespace SST::M5;

extern "C" {
    SimObject* create_PHXSimWrap( SST::Component*, std::string, SST::Params& );
}

SimObject* create_PHXSimWrap( SST::Component* comp, string name,
                                                    SST::Params& sstParams )
{
    PHXSimWrap::Params& params   = *new PHXSimWrap::Params;

    params.name = name;

    INIT_HEX( params, sstParams, range.start );
    INIT_HEX( params, sstParams, range.end );
    INIT_STR( params, sstParams, frequency );
    INIT_STR( params, sstParams, megsOfMemory );

    params.exeParams = sstParams.find_prefix_params("exe");

    return new PHXSimWrap( &params );
}

PHXSimWrap::PHXSimWrap( const Params* p ) :
    PhysicalMemory( p ),
    m_waitRetry( false ),
    m_port( NULL ),
    m_dbg( *new Log< PHXSIMC_DBG >( cerr, "PHXSimC::", false ) ),
    m_log( *new Log< >( cout, "INFO PHXSimC: ", false ) )
{
    loadMemory( name(), this, p->exeParams );

    PHXSim::PacketReturnCB *cb = new PHXSim::Callback< PHXSimWrap, 
                                    void, 
                                    PHXSim::Transaction *, 
                                    uint64_t >( this, &PHXSimWrap::recv );
    const char* sim_desc ="";
    
    m_memorySystem = new PHXSim::SingleCube( cb, sim_desc, true );

    SST::SimTime_t delay = SST::Simulation::getSimulation()->
                            getTimeLord()->getSimCycles( p->frequency,"");
    
    m_tickEvent = new TickEvent( this, delay );

    unsigned long freqHz = ( ( 1.0 / delay ) * 1000000000000 ); 
    printf("PHXSimWrap clock %lu hz\n", freqHz );
    m_memorySystem->SetCPUClock(freqHz);
}

void PHXSimWrap::finish() 
{
    m_memorySystem->simulationDone();
}

PHXSimWrap::~PHXSimWrap()
{
    delete &m_log;
    delete &m_dbg;
}

Port * PHXSimWrap::getPort(const std::string &if_name, int idx )
{
    PRINTFN("%s: if_name=`%s` idx=%d\n", __func__, if_name.c_str(), idx );

    if (if_name == "functional") {
        return new MemoryPort(csprintf("%s-functional", name()), this);
    }

    assert( ! m_port );

    if (if_name != "port") {
        panic("PHXSimWrap::getPort: unknown port %s requested", if_name);
    }

    return m_port = new MemPort( name() + "." + if_name, this );
}

void PHXSimWrap::init()
{
    if ( ! m_port ) {
        fatal("PHXSimWrap object %s is unconnected!", name());
    }

    m_port->sendStatusChange(Port::RangeChange);
}

bool PHXSimWrap::recvTiming(PacketPtr pkt)
{
    uint64_t addr    = pkt->getAddr();

    PRINTFN("%s: %s %#lx\n", __func__, pkt->cmdString().c_str(), (long)addr);

    pkt->dram_enter_time = curTick();

    if ( pkt->isRead() ) { 
        bool ret = m_memorySystem->AddTransaction( false, addr, 0 );
        if ( ! ret ) return false;
        assert(m_rd_pktMap.find( addr ) == m_rd_pktMap.end());
        m_rd_pktMap[ addr ] = pkt;
    } else if ( pkt->isWrite() ) {
        bool ret = m_memorySystem->AddTransaction( true, addr, 0 );
        if ( ! ret ) return false;
        std::multimap< uint64_t, PacketPtr >::iterator it;
        it = m_wr_pktMap.find( addr );
        assert( it == m_wr_pktMap.end());
        m_wr_pktMap.insert( pair<uint64_t, PacketPtr>(addr, pkt) );
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

void PHXSimWrap::recvRetry()
{
    PRINTFN("%s:\n",__func__);
    assert( ! m_readyQ.empty() );

    m_waitRetry = false;
    sendTry();
}

void PHXSimWrap::sendTry()
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

void PHXSimWrap::readData(uint id, uint64_t addr, uint64_t clockcycle)
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


void PHXSimWrap::writeData(uint id, uint64_t addr, uint64_t clockcycle)
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

void PHXSimWrap::regStats()
{
    using namespace Stats;

    m_rd_lat
        .init(0,250000,50000)
        .name(name() + ".rt_read_lat")
        .desc("Round trip latency for a phx read req")
        .precision(2)
        ;

    m_wr_lat
        .init(0,250000,50000)
        .name(name() + ".rt_write_lat")
        .desc("Round trip latency for a phx write req")
        .precision(2)
        ;
}

bool PHXSimWrap::recvSignalWrapper(void *arg) {
    PHXSim::Transaction *t = static_cast<PHXSim::Transaction *>(arg);
    recv(t, 0);
    return true;
}

void PHXSimWrap::recv(PHXSim::Transaction *t, uint64_t cycle) {
    PRINTFN("%s: id=%d addr=%#lx clock=%lu\n",__func__,
                    t->transactionID, t->address, cycle);

    if ( t->transactionType == PHXSim::RETURN_DATA ) {
        readData(0, t->address, cycle );
        delete t;
    } else {
        writeData(0, t->address, cycle );
    }
}


void PHXSimWrap::tick()
{
    m_memorySystem->Update();

    sendTry();
}

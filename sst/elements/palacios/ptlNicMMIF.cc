// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>

#include "ptlNicMMIF.h"
#include "portals4/ptlNic/ptlNicEvent.h"
#include "portals4/ptlNic/dmaEvent.h"

using namespace SST;
using namespace SST::Palacios;
using namespace SST::Portals4;

const char * PtlNicMMIF::m_cmdNames[] = CMD_NAMES;

#define INFO( fmt, args... ) \
    fprintf( stderr, "INFO: "fmt, ##args)

PtlNicMMIF::PtlNicMMIF( SST::ComponentId_t id, Params_t& params ) :
    Component( id ),
    m_barrierCallback( BarrierAction::Handler<PtlNicMMIF>(this,
                        &PtlNicMMIF::barrierLeave) ),
#if USE_THREAD 
    m_threadRun( false ),
#else
    m_simulating( false ),
#endif
    m_runCycles( 100 ),
    m_runCyclesJitter( 0 ),
    m_functor( DerivedFunctor<PtlNicMMIF>(this,&PtlNicMMIF::update) )
{
    int ret;

    DBGX(1,"\n");

    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    if( params.find_string("single","false").compare("true") )
    {
        
        DBGX(1,"\n");
        m_cmdLink = configureLink( "nic", "1ps",
            new SST::Event::Handler<PtlNicMMIF>( 
                        this, &PtlNicMMIF::ptlCmdRespHandler ) );
        assert( m_cmdLink );

        m_dmaLink = configureLink( "dma", "1ps",
            new SST::Event::Handler<PtlNicMMIF>( 
                        this, &PtlNicMMIF::dmaHandler ) );
        assert( m_dmaLink );
    }

    m_devMemory.resize(4096);
    m_cmdQueue = (cmdQueue_t*) &m_devMemory[0];
    m_cmdQueue->head = m_cmdQueue->tail = 0;
    m_barrierOffset = 4096 - sizeof(int);
    m_simulationCtrlCmd = 4096 - (sizeof(int) * 2);

    m_barrier.add( &m_barrierCallback );


    m_palaciosIF = new PalaciosIF( m_functor, params.find_string("vm-dev"), 
                                    params.find_string("dev" ), 
                                    &m_devMemory[0],
                                    m_devMemory.size(),
                                    0xfffff000 );
    assert( m_palaciosIF );

    ret = m_palaciosIF->vm_launch();
    assert( ret == 0 );

    m_guestHz            = m_palaciosIF->getCpuFreq();
    m_runCycles          = params.find_integer( "minCycles", m_runCycles );
    float runPeriod          = (1.0/m_guestHz) * m_runCycles;

    INFO("minCycles=%lu, guest CPU freq %lu hz, runPeriod %.15f\n",
                     m_runCycles, m_guestHz, runPeriod );
#if USE_THREAD 
    INFO("Using a thread to run palacios\n");
#endif

    char buf[64];
    snprintf( buf, 64, "%.15f s", runPeriod );
    TimeConverter* tmp = registerClock( buf,
        new SST::Clock::Handler< PtlNicMMIF >( this, &PtlNicMMIF::clock ) );
    assert( tmp );

    DBGX(1,"%lu\n",tmp->getFactor());

#if USE_THREAD 
    ret = pthread_barrier_init( &m_threadBarrier, NULL, 2 );
    assert( ret == 0 ); 
#endif
}    

PtlNicMMIF::~PtlNicMMIF()
{
    DBGX(1,"\n");
}

bool PtlNicMMIF::Status() {
    
    return false;
}

void PtlNicMMIF::update( int offset )
{
    DBGX(1,"offet %#x\n",offset);
    ptlCmd(offset);
    barrierCmd(offset);
}

bool PtlNicMMIF::clock( Cycle_t cycle )
{
    simCtrlCmd();

#if USE_THREAD 
    if ( m_threadRun ) {
        int ret;
        ret = pthread_barrier_wait( &m_threadBarrier );
        assert (ret == 0 || ret == PTHREAD_BARRIER_SERIAL_THREAD );
    }
#else
    if ( m_simulating ) {
        int64_t reqRunCycles = m_runCycles + m_runCyclesJitter;
        if ( reqRunCycles <= 0 ) {
                m_runCyclesJitter += m_runCycles;
        } else {
                int64_t start = m_palaciosIF->rdtsc();
                int ret =  m_palaciosIF->vm_run_cycles( reqRunCycles );
                assert( ret == 0 );
                int64_t actual = m_palaciosIF->rdtsc() - start;
                m_runCyclesJitter = reqRunCycles - actual;
        }
    }
#endif
    
    return false;
}

void PtlNicMMIF::doSimCtrlCmd( int cmd )
{
    DBGX(5,"cmd=%d\n",cmd);
    int ret;

    switch ( cmd ) {
    case VM_SIM_START:
        m_simStartWallTime = time(NULL);
        fprintf(stderr,"Starting Palacios simulate\n");
#if USE_THREAD 
        if ( ! sim_mode_start() ) {
            fprintf(stderr,"PtlNicMMIF::%s() already in simulation mode\n",
                                                        __func__);
        } 
#else
        ret = m_palaciosIF->vm_pause();
        m_simulating = true;
        assert( ret == 0 );
#endif
        // record the starting times for both Palacios and SST so 
        // we can compare results with simulation stops
        m_guestStart = m_palaciosIF->rdtsc();
        m_sstStart = Simulation::getSimulation()->getCurrentSimCycle();
 
        break;

    case VM_SIM_STOP:

        fprintf(stderr, "Stopping Palacios simulate, wall %d sec, "
                    "guest %.5f sec, SST %.5f sec\n",
            time(NULL) - m_simStartWallTime,
            (m_palaciosIF->rdtsc() - m_guestStart)* (1.0/(float) m_guestHz),
            (Simulation::getSimulation()->getCurrentSimCycle() - m_sstStart) *
                    (1.0/(float)1000000000000));

#if USE_THREAD 
        if ( ! sim_mode_stop()  ) {
            fprintf(stderr,"PtlNicMMIF::%s() not in simulation mode \n",
                                                        __func__);
        } 
#else
        m_simulating = false;
        ret = m_palaciosIF->vm_continue();
        assert( ret == 0 );
#endif
        break;

    case VM_SIM_TERM:
#if USE_THREAD 
        sim_mode_stop();
#else
        if ( m_simulating ) {
            ret = m_palaciosIF->vm_continue();
            assert( ret == 0 );
        }
#endif
        m_palaciosIF->vm_stop();
        delete m_palaciosIF;
        primaryComponentOKToEndSim();
        break;

    default:
        fprintf(stderr,"PtlNicMMIF::%s() invalid cmd %d \n", __func__, cmd );
        abort();
    }
    DBGX(5,"return\n");
}

void PtlNicMMIF::setup()
{
}

#if USE_THREAD 
void* PtlNicMMIF::thread1( void* ptr )
{
    return ((PtlNicMMIF*)ptr)->thread2();
}

void* PtlNicMMIF::thread2( )
{
    do {
        int ret =  m_palaciosIF->vm_run_cycles( m_runCycles );
        assert( ret == 0 );

        ret = pthread_barrier_wait( &m_threadBarrier );
        assert (ret == 0 || ret == PTHREAD_BARRIER_SERIAL_THREAD );

    } while ( m_threadRun );
    return NULL;
}
#endif

void PtlNicMMIF::ptlCmd( int offset )
{
    if ( m_cmdQueue->head != m_cmdQueue->tail ) {
        DBGX(2,"new command head=%d tail=%d\n",
                                m_cmdQueue->head, m_cmdQueue->tail );
        m_cmdLink->send(  
                    new PtlNicEvent( &m_cmdQueue->queue[ m_cmdQueue->head ] ) );  
        m_cmdQueue->head = ( m_cmdQueue->head + 1 ) % CMD_QUEUE_SIZE;
    } 
}

void PtlNicMMIF::simCtrlCmd()
{
    uint32_t* tmp = (uint32_t*) &m_devMemory[m_simulationCtrlCmd];
    if ( *tmp ) {
        DBGX(2,"simulation cmd %d\n", *tmp );
        doSimCtrlCmd( *tmp );
        *tmp = 0;
    }
}    

void PtlNicMMIF::barrierCmd( int offset )
{
    if ( offset != m_barrierOffset ) return;
    uint32_t* tmp = (uint32_t*) &m_devMemory[m_barrierOffset];   
    if ( *tmp == 1 ) {
        DBGX(2,"\n");
        m_barrier.enter();
        ++(*tmp);
    }
}

void PtlNicMMIF::barrierLeave()
{
    DBGX(2,"\n");
    uint32_t* tmp = (uint32_t*) &m_devMemory[m_barrierOffset];   
    *tmp = 0;
}

void PtlNicMMIF::dmaHandler( SST::Event* e )
{
    DmaEvent* event = static_cast< DmaEvent* >(e);
    DBGX( 2, "addr=%#lx size=%lu type=%s\n", event->addr, event->size,
                event->type == DmaEvent::Write ? "Write" : "Read" );
    if ( event->type == DmaEvent::Write ) {
        m_palaciosIF->writeGuestMemVirt( event->addr, 
                    (uint8_t*)event->buf, event->size );
    } else {
        m_palaciosIF->readGuestMemVirt( event->addr, 
                    (uint8_t*)event->buf, event->size );
    }

    m_dmaLink->send( event ); 
}

void PtlNicMMIF::ptlCmdRespHandler( SST::Event* e )
{
    DBGX(2,"\n");
    assert(0);
#if 0 
    PtlNicRespEvent* event = static_cast<PtlNicRespEvent*>(e);
    DBGX(2,"retval %#x\n",event->retval);
    m_cmdQueue->queue[ m_cmdQueue->head ].retval = event->retval;
    m_cmdQueue->queue[ m_cmdQueue->head ].cmd = 0;
    m_cmdQueue->head = ( m_cmdQueue->head + 1 ) % CMD_QUEUE_SIZE;
    delete e;
#endif
}

BOOST_CLASS_EXPORT(PtlNicEvent);
BOOST_CLASS_EXPORT(PtlNicRespEvent);


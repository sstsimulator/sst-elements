#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>

#include "ptlNicMMIF.h"
#include "portals4/ptlNic/ptlNicEvent.h"
#include "portals4/ptlNic/dmaEvent.h"

BOOST_CLASS_EXPORT(PtlNicEvent);
BOOST_CLASS_EXPORT(PtlNicRespEvent);

const char * PtlNicMMIF::m_cmdNames[] = CMD_NAMES;

using namespace SST;

PtlNicMMIF::PtlNicMMIF( SST::ComponentId_t id, Params_t& params ) :
    Component( id ),
    m_barrierCallback( BarrierAction::Handler<PtlNicMMIF>(this,
                        &PtlNicMMIF::barrierLeave) ),
    m_threadRun( false ),
    m_count( 0 ),
    m_runCycles( 100 )
{
    int ret;

    DBGX(1,"\n");

    registerExit();

    m_cmdLink = configureLink( "nic", "1ps",
        new SST::Event::Handler<PtlNicMMIF>( 
                        this, &PtlNicMMIF::ptlCmdRespHandler ) );
    assert( m_cmdLink );

    m_dmaLink = configureLink( "dma", "1ps",
        new SST::Event::Handler<PtlNicMMIF>( this, &PtlNicMMIF::dmaHandler ) );
    assert( m_dmaLink );

    m_devMemory.resize(4096);
    m_cmdQueue = (cmdQueue_t*) &m_devMemory[0];
    m_cmdQueue->head = m_cmdQueue->tail = 0;
    m_barrierOffset = 4096 - sizeof(int);
    m_simulationCtrlCmd = 4096 - (sizeof(int) * 2);

    m_barrier.add( &m_barrierCallback );

    m_palaciosIF = new PalaciosIF( params.find_string("vm-dev"), 
                                    params.find_string("dev" ), 
                                    &m_devMemory[0],
                                    m_devMemory.size(),
                                    0xfffff000 );
    assert( m_palaciosIF );

    ret = m_palaciosIF->vm_launch();
    assert( ret == 0 );

    TimeConverter *minPartTC = Simulation::getSimulation()->getMinPartTC();
    assert ( minPartTC );

    // getFactor returns number of ps between Sync's
    float clockPeriod_ns = minPartTC->getFactor() / 1000.0;
    m_guestHz            = params.find_integer( "cpuhz"); 
    m_runCycles          = params.find_integer( "minCycles" );

    printf("requested minCycles=%lu, guest CPU freq %lu hz\n",
                            m_runCycles, m_guestHz);

    float minSimPeriod_ns = (1.0 / (float) m_guestHz) * m_runCycles * 1000000000.0;

    float adjustedPeriod_ns = adjustPeriod( clockPeriod_ns, minSimPeriod_ns );

    printf("SST clock() period %.3f ns.\n", clockPeriod_ns );

    m_mult = (int) round(adjustedPeriod_ns / clockPeriod_ns); 
    m_runCycles = (uint64_t) round( (adjustedPeriod_ns / 1000000000.0 ) / 
                        (1.0/(float)m_guestHz) );
    printf("mult=%d runCycles=%d\n", m_mult, m_runCycles );
    printf("requested simPeriod %.3f ns. adjusted to %.3f ns.\n", 
                minSimPeriod_ns, m_mult * clockPeriod_ns );

    std::string strBuf;
    strBuf.resize(32);
    snprintf( &strBuf[0], 32, "%.0f ns", clockPeriod_ns );

    TimeConverter *tmp = registerClock( strBuf,
        new SST::Clock::Handler< PtlNicMMIF >( this, &PtlNicMMIF::clock ) );
    assert( tmp );

    ret = pthread_mutex_init( &m_threadMutex, NULL );
    assert( ret == 0 ); 

    ret = pthread_cond_init( &m_threadCond, NULL );
    assert( ret == 0 ); 
}    

PtlNicMMIF::~PtlNicMMIF()
{
    DBGX(1,"\n");
}

bool PtlNicMMIF::Status() {
    
    return false;
}

bool PtlNicMMIF::clock( Cycle_t cycle )
{
    simCtrlCmd();
    ptlCmd();
    barrierCmd();

    if ( m_threadRun ) {
        
        if ( m_count == 0 ) {
            pthread_mutex_lock( &m_threadMutex );
            pthread_cond_signal( &m_threadCond );
            pthread_mutex_unlock( &m_threadMutex );
        }
        ++m_count;
        if ( m_count == m_mult ) {
            m_count = 0;
        }
    }
    return false;
}

void PtlNicMMIF::doSimCtrlCmd( int cmd )
{
    DBGX(5,"cmd=%d\n",cmd);

    switch ( cmd ) {
    case VM_SIM_START:
        system("/bin/date");
        if ( ! sim_mode_start() ) {
            fprintf(stderr,"PtlNicMMIF::%s() already in simulation mode\n",
                                                        __func__);
        } 
        // record the starting times for both Palacios and SST so 
        // we can compare results with simulation stops
        m_guestStart = m_palaciosIF->rdtsc();
        m_sstStart = Simulation::getSimulation()->getCurrentSimCycle();
 
        break;

    case VM_SIM_STOP:
        system("/bin/date");

        printf("guest time %.6f, sst time %.6f\n",
            (m_palaciosIF->rdtsc() - m_guestStart)* (1.0/(float) m_guestHz),
            (Simulation::getSimulation()->getCurrentSimCycle() - m_sstStart) *
                    (1.0/(float)1000000000000));

        if ( ! sim_mode_stop()  ) {
            fprintf(stderr,"PtlNicMMIF::%s() not in simulation mode \n",
                                                        __func__);
        } 
        break;

    case VM_SIM_TERM:
        sim_mode_stop();
        m_palaciosIF->vm_stop();
        delete m_palaciosIF;
        unregisterExit();
        break;

    default:
        fprintf(stderr,"PtlNicMMIF::%s() invalid cmd %d \n", __func__, cmd );
        abort();
    }
    DBGX(5,"return\n");
}

int PtlNicMMIF::Setup()
{
    return 0;
}

void* PtlNicMMIF::thread1( void* ptr )
{
    return ((PtlNicMMIF*)ptr)->thread2();
}

void* PtlNicMMIF::thread2( )
{
    while ( m_threadRun ) {

        pthread_mutex_lock( &m_threadMutex );
        pthread_cond_wait( &m_threadCond, &m_threadMutex );

        int ret =  m_palaciosIF->vm_run_cycles( m_runCycles );
        assert( ret == 0 );

        pthread_mutex_unlock( &m_threadMutex );
    }
    return NULL;
}

void PtlNicMMIF::ptlCmd()
{
    if ( m_cmdQueue->head != m_cmdQueue->tail ) {
        DBGX(2,"new command head=%d tail=%d\n",
                                m_cmdQueue->head, m_cmdQueue->tail );
        m_cmdLink->Send( 
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

void PtlNicMMIF::barrierCmd()
{
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
        m_palaciosIF->writeGuestMemVirt( event->addr, event->buf, event->size );
    } else {
        m_palaciosIF->readGuestMemVirt( event->addr, event->buf, event->size );
    }

    m_dmaLink->Send( event );
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

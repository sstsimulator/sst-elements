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
    m_writeFunc( this, &PtlNicMMIF::writeFunc ),
    m_barrierCallback( BarrierAction::Handler<PtlNicMMIF>(this,
                        &PtlNicMMIF::barrierLeave) ),
    m_fooTicks( 0 ),
    m_threadRun( false )
{
    DBGX(1,"\n");

    registerExit();
#if 1
    registerTimeBase( "1ns" );

    m_cmdLink = configureLink( "nic",
        new SST::Event::Handler<PtlNicMMIF>( 
                        this, &PtlNicMMIF::ptlCmdRespHandler ) );
    
    assert( m_cmdLink );

    m_dmaLink = configureLink( "dma",
        new SST::Event::Handler<PtlNicMMIF>( this, &PtlNicMMIF::dmaHandler ) );

    assert( m_dmaLink );
#endif

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
                                    0xfffff000,
                                    m_writeFunc );
    assert( m_palaciosIF );

    int ret = m_palaciosIF->vm_launch();
    assert( ret == 0 );

    Cycle_t ticksPerSync = 0;
    TimeConverter *minPartTC = Simulation::getSimulation()->getMinPartTC();
    if ( minPartTC ) {
        ticksPerSync = minPartTC->getFactor();
    }

    float clockFreqKhz = (1000000000000 / ticksPerSync) / 1000;

    std::string strBuf;
    strBuf.resize(32);
    snprintf( &strBuf[0], 32, "%.0f khz", clockFreqKhz );

    DBGX( 3, "SST sync factor %i, `%s`\n",
                        ticksPerSync, strBuf.c_str() );

    registerClock( strBuf,
        new SST::Clock::Handler< PtlNicMMIF >( this, &PtlNicMMIF::clock ) );

    m_fooTicksPer = (int) clockFreqKhz;  
    DBGX(3,"m_fooTicksPer=%d\n", m_fooTicksPer );

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
    checkForSimCtrlCmd();

    if ( m_threadRun ) {
        ++ m_fooTicks;
        if ( m_fooTicks == m_fooTicksPer ) {
    
            pthread_mutex_lock( &m_threadMutex );
            pthread_cond_signal( &m_threadCond );
            pthread_mutex_unlock( &m_threadMutex );
            m_fooTicks = 0;
        }
    }
    return false;
}

void PtlNicMMIF::doSimCtrlCmd( int cmd )
{
    DBGX(5,"cmd=%d\n",cmd);

    switch ( cmd ) {
    case VM_SIM_START:
        if ( ! sim_mode_start() ) {
            fprintf(stderr,"PtlNicMMIF::%s() already in simulation mode\n",
                                                        __func__);
        } 
        break;

    case VM_SIM_STOP:
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

        int ret =  m_palaciosIF->vm_run_msecs(1);
        assert( ret == 0 );

        pthread_mutex_unlock( &m_threadMutex );
    }
    return NULL;
}

void PtlNicMMIF::writeFunc( unsigned long offset )
{
    DBGX(5,"offset=%#lx\n", offset );

    if ( offset == offsetof( cmdQueue_t,tail) ) {
        DBGX(2,"new command head=%d tail=%d\n",
                                m_cmdQueue->head, m_cmdQueue->tail );
        m_cmdLink->Send( 
                    new PtlNicEvent( &m_cmdQueue->queue[ m_cmdQueue->head ] ) );  
        m_cmdQueue->head = ( m_cmdQueue->head + 1 ) % CMD_QUEUE_SIZE;
    } 
    if ( offset == m_barrierOffset ) {
        DBGX(2,"barrier\n");
        m_barrier.enter();
    }
}

void PtlNicMMIF::checkForSimCtrlCmd()
{
    uint32_t* tmp = (uint32_t*) &m_devMemory[m_simulationCtrlCmd];
    if ( *tmp ) {
        DBGX(2,"simulation cmd %d\n", *tmp );
        doSimCtrlCmd( *tmp );
        *tmp = 0;
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

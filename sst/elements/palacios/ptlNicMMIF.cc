#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>

#include "ptlNicMMIF.h"
#include "portals4/ptlNic/ptlNicEvent.h"
#include "portals4/ptlNic/dmaEvent.h"

#include <cxxabi.h>

#define DBGX( x, fmt, args... ) \
{\
     char* realname = abi::__cxa_demangle(typeid(*this).name(),0,0,NULL);\
    fprintf( stderr, "%s::%s():%d: "fmt, realname ? realname : "?????????", \
                        __func__, __LINE__, ##args);\
    fflush(stderr);\
    if ( realname ) free(realname);\
}

BOOST_CLASS_EXPORT(PtlNicEvent);
BOOST_CLASS_EXPORT(PtlNicRespEvent);

const char * PtlNicMMIF::m_cmdNames[] = CMD_NAMES;

PtlNicMMIF::PtlNicMMIF( SST::ComponentId_t id, Params_t& params ) :
    Component( id ),
    m_writeFunc( this, &PtlNicMMIF::writeFunc ),
    m_barrierCallback( BarrierAction::Handler<PtlNicMMIF>(this,
                        &PtlNicMMIF::barrierLeave) )
{
    DBGX(1,"\n");


    registerTimeBase( "1ns" );

//    m_cmdLink = configureLink( "nic", "1ps",
    m_cmdLink = configureLink( "nic",
        new SST::Event::Handler<PtlNicMMIF>( 
                        this, &PtlNicMMIF::ptlCmdRespHandler ) );
    
    assert( m_cmdLink );

//    m_dmaLink = configureLink( "dma", "1ps",
    m_dmaLink = configureLink( "dma",
        new SST::Event::Handler<PtlNicMMIF>( this, &PtlNicMMIF::dmaHandler ) );

    assert( m_dmaLink );

    m_devMemory.resize(4096);
    m_cmdQueue = (cmdQueue_t*) &m_devMemory[0];
    m_cmdQueue->head = m_cmdQueue->tail = 0;
    m_barrierOffset = 4096 - sizeof(int);
    
    m_barrier.add( &m_barrierCallback );
    m_palaciosIF = new PalaciosIF( params.find_string("vm-dev"), 
                                    params.find_string("dev" ), 
                                    &m_devMemory[0],
                                    m_devMemory.size(),
                                    0xfffff000,
                                    m_writeFunc );
    assert( m_palaciosIF );
}    

PtlNicMMIF::~PtlNicMMIF()
{
    DBGX(1,"\n");
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
    //m_cmdQueue.queue[ m_cmdQueue.head ].retval = event->retval;
    //m_cmdQueue.queue[ m_cmdQueue.head ].cmd = 0;
    m_cmdQueue.head = ( m_cmdQueue.head + 1 ) % CMD_QUEUE_SIZE;
    delete e;
#endif
}


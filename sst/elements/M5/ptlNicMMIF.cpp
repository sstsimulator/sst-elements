#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>

#include <debug.h>
#include <paramHelp.h>
#include <m5.h>
#include "ptlNicMMIF.h"
#include <system.h>
#include "portals4/ptlNic/ptlNicEvent.h"
#include "portals4/ptlNic/dmaEvent.h"

extern "C" {
SimObject* create_PtlNicMMIF( SST::Component*, string name,
                                                SST::Params& sstParams );
}

SimObject* create_PtlNicMMIF( SST::Component* comp, string name, 
                                                SST::Params& sstParams )
{
    PtlNicMMIF::Params& memP   = *new PtlNicMMIF::Params;

    memP.name = name;
    INIT_HEX( memP, sstParams, startAddr );
    INIT_INT( memP, sstParams, clockFreq );
    INIT_INT( memP, sstParams, id );
    memP.system = create_System( "syscall", NULL, Enums::timing ); 
    memP.m5Comp = static_cast< M5* >( static_cast< void* >( comp ) );

    return new PtlNicMMIF( &memP );
}

PtlNicMMIF::PtlNicMMIF( const Params* p ) :
    DmaDevice( p ),
    m_startAddr( p->startAddr ),
    m_comp( p->m5Comp ),
    m_blocked( false )
{
    m_endAddr = m_startAddr + sizeof(cmdQueue_t);
    DBGX(2,"startAddr=%#lx endAddr=%#lx\n", m_startAddr, m_endAddr);
    m_cmdQueue.head = m_cmdQueue.tail = 0;

    std::stringstream numStr;
    numStr << p->id;
    m_cmdLink = m_comp->configureLink( "nic" + numStr.str(), "1ps",
        new SST::Event::Handler<PtlNicMMIF>( this, &PtlNicMMIF::ptlCmdRespHandler ) );
    
    assert( m_cmdLink );

    m_dmaLink = m_comp->configureLink( "dma" + numStr.str(), "1ps",
        new SST::Event::Handler<PtlNicMMIF>( this, &PtlNicMMIF::dmaHandler ) );

    assert( m_dmaLink );

#define chunk_size (128) 

    assert( p->clockFreq > 0 );  
    int clockPeriod = 1.0/(double)p->clockFreq * 1000000000000; 
    INFO("%s: clockFreq=%d hz, clockPeriod=%d ps, B/W=%lu b/s\n",__func__,
                      p->clockFreq, clockPeriod, p->clockFreq * chunk_size );
    m_clockEvent = new ClockEvent( this, clockPeriod );
}    

PtlNicMMIF::~PtlNicMMIF()
{
}

void PtlNicMMIF::dmaHandler( SST::Event* e )
{
//
//  FIX ME!! this is a memory leak because no one delete the allocate DmaEvent
//
    ::DmaEvent* event = static_cast< ::DmaEvent* >(e);
    DBGX( 2, "addr=%#lx size=%lu type=%s\n", event->addr, event->size,
                event->type == ::DmaEvent::Write ? "Write" : "Read" );
    event->_size = event->size;
    if ( event->type == ::DmaEvent::Write ) {
        m_dmaWriteEventQ.push_back( e );
    } else {
        m_dmaReadEventQ.push_back( e );
    }
}


void PtlNicMMIF::ptlCmdRespHandler( SST::Event* e )
{
    PtlNicRespEvent* event = static_cast<PtlNicRespEvent*>(e);
    DBGX(2,"retval %#x\n",event->retval);
    //m_cmdQueue.queue[ m_cmdQueue.head ].retval = event->retval;
    //m_cmdQueue.queue[ m_cmdQueue.head ].cmd = 0;
    m_cmdQueue.head = ( m_cmdQueue.head + 1 ) % CMD_QUEUE_SIZE;
    delete e;
}

void PtlNicMMIF::clock(  )
{
    if ( ! m_dmaReadEventQ.empty() ) {
        ::DmaEvent* event = 
            static_cast< ::DmaEvent* >( m_dmaReadEventQ.front() );

        DmaEvent* foo;
        size_t size;
        if ( (int) event->_size - chunk_size > 0 ) {
            size = chunk_size;
            foo = new DmaEvent( this, NULL );
        } else {
            size = event->_size;
            foo = new DmaEvent( this, event );
            m_dmaReadEventQ.pop_front();
        }

        dmaRead( event->addr, size, foo, event->buf, 0 );
        event->addr += size;
        event->_size -= size;
        event->buf += size;
    }
    
    if ( ! m_dmaWriteEventQ.empty() ) {
        ::DmaEvent* event = 
            static_cast< ::DmaEvent* >( m_dmaWriteEventQ.front() );

        size_t size;
        DmaEvent* foo;
        if ( (int) event->_size - chunk_size > 0 ) {
            size = chunk_size;
            foo = new DmaEvent( this, NULL);
        } else {
            size = event->_size;
            foo = new DmaEvent( this, event);
            m_dmaWriteEventQ.pop_front();
        }

        dmaWrite( event->addr, size, foo, event->buf, 0 );

        event->addr += size;
        event->_size -= size;
        event->buf += size;
    }
}

void PtlNicMMIF::process( void* key )
{
    if ( ! key ) return;

    ::DmaEvent* event = static_cast< ::DmaEvent* >(key);
    DBGX( 2, "addr=%#lx size=%lu type=%s\n", event->addr, event->size,
                event->type == ::DmaEvent::Write ? "Write" : "Read" );
    m_dmaLink->Send( event );
}

void PtlNicMMIF::addressRanges(AddrRangeList& resp)
{
    DBGX(2,"\n");
    resp.clear();
    resp.push_back( RangeSize( m_startAddr, m_endAddr ));
}

Tick PtlNicMMIF::write(Packet* pkt)
{
    DBGX(2,"paddr=%#lx size=%d\n", (unsigned long) pkt->getAddr(),
                                        pkt->getSize());

    assert( pkt->getAddr() + pkt->getSize() <= m_endAddr );

    pkt->writeData((uint8_t*) &m_cmdQueue + ( pkt->getAddr() - m_startAddr ));
    pkt->makeTimingResponse();

    if ( ( pkt->getAddr() - m_startAddr ) == offsetof( cmdQueue_t, tail ) ) {
        if ( ! m_blocked ) {
            m_cmdLink->Send( 
                    new PtlNicEvent( &m_cmdQueue.queue[ m_cmdQueue.head ] ) );  
            m_cmdQueue.head = ( m_cmdQueue.head + 1 ) % CMD_QUEUE_SIZE;
        }
    }

    return 1;
}

Tick PtlNicMMIF::read(Packet* pkt)
{
    DBGX(2,"paddr=%#lx size=%d\n", (unsigned long) pkt->getAddr(),
                                        pkt->getSize());

    assert( pkt->getAddr() + pkt->getSize() <= m_endAddr );

    pkt->setData( ( uint8_t*) &m_cmdQueue + ( pkt->getAddr() - m_startAddr ) );
    pkt->makeTimingResponse();
    return 1;
}

BOOST_CLASS_EXPORT(PtlNicEvent);
BOOST_CLASS_EXPORT(PtlNicRespEvent);

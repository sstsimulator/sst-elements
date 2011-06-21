#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>

#include "ptlNicMMIF.h"
#include <debug.h>
#include <paramHelp.h>
#include <m5.h>
#include <system.h>
#include "ptlNicEvent.h"
#include "dmaEvent.h"

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

    m_cmdLink = m_comp->configureLink( "nic", "1ps",
        new SST::Event::Handler<PtlNicMMIF>( this, &PtlNicMMIF::ptlCmdRespHandler ) );
    
    assert( m_cmdLink );

    m_dmaLink = m_comp->configureLink( "dma", "1ps",
        new SST::Event::Handler<PtlNicMMIF>( this, &PtlNicMMIF::dmaHandler ) );
    
    assert( m_dmaLink );
    
    //m_tc = p->m5Comp->registerTimeBase("1ns");
    //assert( m_tc ); 
    //m_cmdLink->setDefaultTimeBase(m_tc);
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
    if ( event->type == ::DmaEvent::Write ) {
        dmaWrite( event->addr, 
                event->size, 
                new DmaEvent( this, event), 
                event->buf, 0 );
    } else {
        dmaRead( event->addr, 
                event->size, 
                new DmaEvent( this, event), 
                event->buf, 0 );
    }
}


void PtlNicMMIF::ptlCmdRespHandler( SST::Event* e )
{
    PtlNicRespEvent* event = static_cast<PtlNicRespEvent*>(e);
    DBGX(2,"retval %#x\n",event->retval);
    m_cmdQueue.queue[ m_cmdQueue.head ].retval = event->retval;
    m_cmdQueue.queue[ m_cmdQueue.head ].cmd = 0;
    m_cmdQueue.head = ( m_cmdQueue.head + 1 ) % CMD_QUEUE_SIZE;
    delete e;
}

void PtlNicMMIF::process( void* key )
{
    ::DmaEvent* event = static_cast< ::DmaEvent* >(key);
    DBGX( 2, "addr=%#lx size=%lu type=\n", event->addr, event->size,
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
    DBGX(3,"paddr=%#lx size=%d\n", (unsigned long) pkt->getAddr(),
                                        pkt->getSize());

    assert( pkt->getAddr() + pkt->getSize() <= m_endAddr );

    pkt->writeData((uint8_t*) &m_cmdQueue + ( pkt->getAddr() - m_startAddr ));
    pkt->makeTimingResponse();

    if ( ( pkt->getAddr() - m_startAddr ) == offsetof( cmdQueue_t, tail ) ) {
        if ( ! m_blocked ) {
            m_cmdLink->Send( 
                    new PtlNicEvent( &m_cmdQueue.queue[ m_cmdQueue.head ] ) );  
        }
    }

    return 1;
}

Tick PtlNicMMIF::read(Packet* pkt)
{
    DBGX(3,"paddr=%#lx size=%d\n", (unsigned long) pkt->getAddr(),
                                        pkt->getSize());

    assert( pkt->getAddr() + pkt->getSize() <= m_endAddr );

    pkt->setData( ( uint8_t*) &m_cmdQueue + ( pkt->getAddr() - m_startAddr ) );
    pkt->makeTimingResponse();
    return 1;
}

BOOST_CLASS_EXPORT(PtlNicEvent);
BOOST_CLASS_EXPORT(PtlNicRespEvent);

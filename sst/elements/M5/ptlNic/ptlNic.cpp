#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>

#include "ptlNic.h"
#include <debug.h>
#include <paramHelp.h>
#include <m5.h>
#include <system.h>

extern "C" {
SimObject* create_PtlNic( SST::Component*, string name,
                                                SST::Params& sstParams );
}

SimObject* create_PtlNic( SST::Component* comp, string name, 
                                                SST::Params& sstParams )
{
    PtlNic::Params& memP   = *new PtlNic::Params;

    memP.name = name;
    INIT_HEX( memP, sstParams, startAddr );
    memP.system = create_System( "syscall", NULL, Enums::timing ); 
    memP.m5Comp = static_cast< M5* >( static_cast< void* >( comp ) );

    return new PtlNic( &memP );
}

PtlNic::PtlNic( const Params* p ) :
    DmaDevice( p ),
    m_startAddr( p->startAddr ),
    m_comp( p->m5Comp )
{
    m_endAddr = m_startAddr + sizeof(cmdQueue_t);
    DBGX(2,"startAddr=%#lx endAddr=%#lx\n", m_startAddr, m_endAddr);
    m_cmdQueue.head = m_cmdQueue.tail = 0;
}    

PtlNic::~PtlNic()
{
}

void PtlNic::process(void)
{
    DBGX(2,"\n");
//    finishPtlNic();
}

void PtlNic::addressRanges(AddrRangeList& resp)
{
    DBGX(2,"\n");
    resp.clear();
    resp.push_back( RangeSize( m_startAddr, m_endAddr ));
}

Tick PtlNic::write(Packet* pkt)
{
    DBGX(2,"paddr=%#lx size=%d\n", (unsigned long) pkt->getAddr(),
                                        pkt->getSize());

    assert( pkt->getAddr() + pkt->getSize() <= m_endAddr );

    pkt->writeData((uint8_t*) &m_cmdQueue + ( pkt->getAddr() - m_startAddr ));
    pkt->makeTimingResponse();

    if ( ( pkt->getAddr() - m_startAddr ) == offsetof( cmdQueue_t, tail ) ) {
        foo( m_cmdQueue.tail );
    }

    return 1;
}

Tick PtlNic::read(Packet* pkt)
{
    DBGX(2,"paddr=%#lx size=%d\n", (unsigned long) pkt->getAddr(),
                                        pkt->getSize());

    assert( pkt->getAddr() + pkt->getSize() <= m_endAddr );

    pkt->setData( ( uint8_t*) &m_cmdQueue + ( pkt->getAddr() - m_startAddr ) );
    pkt->makeTimingResponse();
    return 1;
}


void PtlNic::foo( int queuePos )
{
    DBGX(2,"tail moved to %d\n", queuePos );
    cmdQueueEntry_t *entry = &m_cmdQueue.queue[ m_cmdQueue.head ];
    DBGX(2,"cmd %s\n", m_cmdNames[entry->cmd] ); 
    switch( m_cmdQueue.queue[ m_cmdQueue.head ].cmd ) {
      case PtlNIInit:
        for ( int i = 0; i < 7; i++ ) {
            DBGX(2,"arg%d %#lx\n",i,entry->arg[i]);
        }
        break;
      case PtlNIFini:
        break;
      case PtlPTAlloc:
        break;
      case PtlPTFree:
        break;
    }
    m_cmdQueue.queue[ m_cmdQueue.head ].retval = 0;
    m_cmdQueue.queue[ m_cmdQueue.head ].cmd = 0;
    m_cmdQueue.head = ( m_cmdQueue.head + 1 ) % CMD_QUEUE_SIZE;
}

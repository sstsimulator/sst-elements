#include <sst_config.h>
#include <sst/core/serialization/element.h>

#include "dmaEngine.h"
#include "dmaEvent.h"
#include "nicMmu.h"

#include "trace.h"

DmaEngine::DmaEngine( SST::Component& comp, int nid ) :
    m_comp( comp ),
    m_nicMmu( NULL ),
    m_nid( nid )
{
    TRACE_ADD( DmaEngine );
    PRINT_AT(DmaEngine,"\n");

    m_link = comp.configureLink( "dma", "1ps",
           new SST::Event::Handler<DmaEngine>(this, &DmaEngine::eventHandler));
    assert( m_link );

    std::stringstream tmp;
    tmp << "/pTable." << m_nid;

    m_nicMmu = new NicMmu( tmp.str(), true );
}    

bool DmaEngine::write( Addr vaddr, uint8_t* buf, size_t size, 
            CallbackBase* callback )
{
    Addr paddr = lookup( vaddr );
    PRINT_AT(DmaEngine,"vaddr=%#lx paddr=%#lx buf=%p size=%lu\n", 
                                                vaddr, paddr, buf, size );
    m_link->Send( new DmaEvent( DmaEvent::Write, paddr, buf, size, callback ) );
    return false;
}

bool DmaEngine::read( Addr vaddr, uint8_t* buf, size_t size, 
                            CallbackBase* callback )
{
    Addr paddr = lookup( vaddr );
    PRINT_AT(DmaEngine,"vaddr=%#lx paddr=%#lx buf=%p size=%lu\n",
                                                vaddr, paddr, buf, size );
    m_link->Send( new DmaEvent( DmaEvent::Read, paddr, buf, size, callback ) );
    return false;
}

void DmaEngine::eventHandler( SST::Event* e )
{
    DmaEvent* event = static_cast<DmaEvent*>(e);

    PRINT_AT(DmaEngine,"addr=%#lx buf=%p size=%lu\n", 
                    event->addr,event->buf,event->size);
    
    CallbackBase* callback = (CallbackBase*)event->key;
    if ( (*callback)() ) {
        PRINT_AT(DmaEngine,"delete callback\n");
        delete  callback;
    }  
    delete e;
}

Addr DmaEngine::lookup( Addr vaddr ) 
{
    Addr paddr;
    assert( m_nicMmu->lookup( vaddr, paddr ) );
    return paddr;
}

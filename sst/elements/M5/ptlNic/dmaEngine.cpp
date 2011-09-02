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
    PRINT_AT(DmaEngine,"nid=%d\n",nid);
    assert( nid != -1 );

    m_link = comp.configureLink( "dma", "1ps",
           new SST::Event::Handler<DmaEngine>(this, &DmaEngine::eventHandler));
    assert( m_link );

    std::stringstream tmp;
    tmp << "/pTable." << m_nid << "." << getpid();

    PRINT_AT(DmaEngine,"%s\n",__func__,tmp.str().c_str());
    m_nicMmu = new NicMmu( tmp.str(), true );
}    

bool DmaEngine::write( Addr vaddr, uint8_t* buf, size_t size, 
            CallbackBase* callback )
{
    return xfer( DmaEvent::Write, vaddr, buf, size, callback );
}

bool DmaEngine::read( Addr vaddr, uint8_t* buf, size_t size, 
                            CallbackBase* callback )
{
    return xfer( DmaEvent::Read, vaddr, buf, size, callback );
}

bool DmaEngine::xfer( DmaEvent::Type type, Addr vaddr, 
                uint8_t* buf, size_t size, CallbackBase* callback )
{ 
    xyzList_t list;
    lookup( vaddr, size, list );

    DmaEntry *entry = new DmaEntry;
    entry->length       = size;
    entry->doneLength   = 0;
    entry->callback     = callback;

    xyzList_t::iterator iter = list.begin();

    while( iter != list.end() ) {
        XYZ& item = *iter;
        PRINT_AT(DmaEngine," %s vaddr=%#lx paddr=%#lx buf=%p size=%lu\n",
                    type == DmaEvent::Read ? "Read" : "Write", 
                                        vaddr, item.addr, buf, item.length );
        m_link->Send( new DmaEvent( type, item.addr, buf,
                                            item.length, entry ) );
        vaddr += item.length;
        buf += item.length;
        ++iter;
    }
    return false;
}

void DmaEngine::eventHandler( SST::Event* e )
{
    DmaEvent* event = static_cast<DmaEvent*>(e);
    DmaEntry* entry = (DmaEntry*)event->key;

    entry->doneLength += event->size;

    PRINT_AT(DmaEngine,"addr=%#lx buf=%p size=%lu %s\n", 
                    event->addr,event->buf,event->size,
                    entry->doneLength == entry->length ? "done" : "" );
    
    if ( entry->doneLength == entry->length ) {
        
        if ( entry->callback && (*entry->callback)() ) {
            PRINT_AT(DmaEngine,"delete callback\n");
            delete  entry->callback;
        }  
        delete entry;
    }
    delete e;
}

static size_t roundUp( size_t value, size_t align )
{
    size_t mask = align - 1;
    size_t tmp = (value + mask) & ~mask;
    return tmp == value ? tmp + align : tmp; 
}

void DmaEngine::lookup( Addr vaddr, size_t length, xyzList_t& list ) 
{
    size_t left = length;

    while ( left ) {
        struct XYZ item;

        item.length = vaddr + left > roundUp( vaddr, m_nicMmu->pageSize() ) ? 
                       roundUp( vaddr, m_nicMmu->pageSize() ) - vaddr : left;

        PRINT_AT( DmaEngine, "vaddr=%#lx length=%lu\n", vaddr, item.length );

        bool ret = m_nicMmu->lookup( vaddr, item.addr );
        assert( ret );

        list.push_back(item);

        left -= item.length;
        vaddr += item.length;
    }
}

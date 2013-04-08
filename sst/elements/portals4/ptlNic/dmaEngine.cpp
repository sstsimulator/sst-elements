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

#include "dmaEngine.h"
#include "dmaEvent.h"
#include "nicMmu.h"
#include "debug.h"

using namespace SST::Portals4;

DmaEngine::DmaEngine( SST::Component& comp, SST::Params& params) :
    m_comp( comp ),
#if USE_DMA_LIMIT_BW
    m_pendingBytes( 0.0 ),
#endif
    m_nicMmu( NULL )
{

    m_nid = params.find_integer( "nid" );

    DmaEngine_DBG("nid=%d\n",m_nid);
    if ( params.find_string( "dma-virt2phys" ).compare("false") ) {
        std::string file = params.find_string( "deviceFile" );
        DmaEngine_DBG("%s\n", __func__, file.c_str() );
        m_nicMmu = new NicMmu( file, true );
    } else {
        fprintf( stderr, "don't translate virt 2 phys address\n");
    }

#if USE_DMA_LIMIT_BW
    m_bytesPerClock = params.find_floating( "dmaEngine.bytesPerClock", -1.0 );
    assert( m_bytesPerClock > 0 ); 
#endif

    assert( m_nid != -1 );

    m_link = comp.configureLink( "dma", "1ps",
           new SST::Event::Handler<DmaEngine>(this, &DmaEngine::eventHandler));
    assert( m_link );
}    

#if USE_DMA_LIMIT_BW
void DmaEngine::clock()
{
    if ( m_pendingBytes > 0 ) {
        m_pendingBytes -= m_bytesPerClock; 
    } else if ( ! m_dmaQ.empty() ) {
        m_link->Send( m_dmaQ.front() );
        m_pendingBytes = m_dmaQ.front()->size;
        m_dmaQ.pop_front();
    }
}
#endif

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
        DmaEngine_DBG(" %s vaddr=%#lx paddr=%#lx buf=%p size=%lu\n",
                    type == DmaEvent::Read ? "Read" : "Write", 
                                        vaddr, item.addr, buf, item.length );
#if USE_DMA_LIMIT_BW
        m_dmaQ.push_back( new DmaEvent( type, item.addr, buf,
#else
        m_link->Send( new DmaEvent( type, item.addr, buf,
                                            item.length, entry ) );
#endif

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

    DmaEngine_DBG("addr=%#lx buf=%p size=%lu %s\n", 
                    event->addr,event->buf,event->size,
                    entry->doneLength == entry->length ? "done" : "" );
    
    if ( entry->doneLength == entry->length ) {
        
        if ( entry->callback && (*entry->callback)() ) {
            DmaEngine_DBG("delete callback\n");
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

        DmaEngine_DBG(  "vaddr=%#lx length=%lu\n", vaddr, item.length );

        if ( m_nicMmu ) {
            bool ret = m_nicMmu->lookup( vaddr, item.addr );
            assert( ret );
        } else {
            // Palacios converts the vaddr to physaddr in the DMA request
            item.addr = vaddr;
        }

        list.push_back(item);

        left -= item.length;
        vaddr += item.length;
    }
}

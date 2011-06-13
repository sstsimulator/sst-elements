#include <sst_config.h>
#include <sst/core/serialization/element.h>

#include "recvEntry.h"
#include "dmaEngine.h"
#include "callback.h"
#include "trace.h"

RecvEntry::RecvEntry( DmaEngine const& dma, Addr vaddr,
                        size_t length, CallbackBase* callback ) :
    m_dma( const_cast<DmaEngine&>(dma) ),
    m_vaddr( vaddr ),
    m_length( length ),
    m_callback( callback ),
    m_curDma( NULL ),
    m_offset( 0 ),
    m_dmaed( 0 )
{
    TRACE_ADD( RecvEntry );
}

bool RecvEntry::pushPkt( unsigned char const* data, int pktSize ) 
{
    PRINT_AT( RecvEntry, "\n" );

    do {
        if ( m_curDma == NULL ) {
            m_curDma = newDmaBuf();
        }

        size_t numPushed = m_curDma->push( data, pktSize );

        data += numPushed;
        pktSize -= numPushed;

        if ( m_curDma->full() ) {
            PRINT_AT(RecvEntry,"new dma\n");
            m_dma.write( m_curDma->vaddr(), m_curDma->bufPtr(),
                                m_curDma->size(), m_curDma->callback() );
            m_curDma = NULL;
        } 

        m_offset += numPushed; 

    } while ( m_offset < m_length && pktSize );

    return ( m_offset == m_length );
}

RecvEntry::DmaBuf* RecvEntry::newDmaBuf()
{
    size_t size = ( m_length - m_offset < m_dma.maxXfer() ) ? 
                                m_length - m_offset : m_dma.maxXfer();

    PRINT_AT(RecvEntry,"new DmaBuf size=%d\n",size);

    return new DmaBuf( m_vaddr + m_offset, size, this );
}

bool RecvEntry::dmaCallback( DmaBuf* dma )
{
    PRINT_AT( RecvEntry, "\n");
    m_dmaed += dma->size();

    delete dma;

    if ( m_dmaed == m_length ) {
        if( (*m_callback)() ) {
            delete m_callback;
        } 
    }
    return true;
}

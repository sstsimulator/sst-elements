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

#ifndef _recvEntry_h
#define _recvEntry_h

#include <vector>
#include <string.h>
#include "ptlNicTypes.h"
#include "callback.h"
#include "./debug.h"

namespace SST {
namespace Portals4 {

class DmaEngine;
class CallbackBase;

class RecvEntry {

  public:
    RecvEntry( DmaEngine const& dma, Addr vaddr, size_t length,
                                        CallbackBase* callback ); 
    bool pushPkt( unsigned char const* data, int nbytes );

  private:

    class DmaBuf {
      public:
        DmaBuf( Addr, size_t, RecvEntry* );
        size_t push( void const*, size_t );
        Addr vaddr();
        bool  full();
        size_t size();
        unsigned char* bufPtr(); 
        Callback< RecvEntry, DmaBuf >* callback();

      private:
        size_t availBytes();
        Addr                            m_vaddr;
        size_t                          m_offset;
        std::vector< unsigned char >    m_buf;
        Callback< RecvEntry, DmaBuf >*  m_callback;
    };

    typedef Callback< RecvEntry, DmaBuf > DmaCallback;
    bool dmaCallback( DmaBuf* );

    DmaBuf* newDmaBuf();
    
    Addr                m_vaddr;
    size_t              m_length;
    size_t              m_offset;
    size_t              m_dmaed;
    CallbackBase*       m_callback;
    DmaEngine&          m_dma;
    DmaBuf*             m_curDma;
};

inline RecvEntry::DmaBuf::DmaBuf( Addr vaddr, size_t size, RecvEntry* obj ) :
    m_vaddr( vaddr ),
    m_offset( 0 ),
    m_buf( size ),
    m_callback( new DmaCallback( obj, &RecvEntry::dmaCallback, this) )
{
    PRINT_AT(DmaBuf,"vaddr=%#lx size=%lu\n", vaddr, size );
}

inline size_t RecvEntry::DmaBuf::push( void const* ptr, size_t nbytes )
{
    PRINT_AT(DmaBuf,"\n");
    nbytes = nbytes > availBytes() ? availBytes() : nbytes;
    memcpy( &m_buf.at( m_offset ), ptr, nbytes );
    m_offset += nbytes;

    return nbytes;
}

inline Addr RecvEntry::DmaBuf::vaddr() {
    return m_vaddr;
}

inline RecvEntry::DmaCallback* RecvEntry::DmaBuf::callback() {
    return m_callback;
}

inline size_t RecvEntry::DmaBuf::availBytes() {
    return m_buf.size() - m_offset;
}

inline size_t RecvEntry::DmaBuf::size() {
    return m_buf.size();
}

inline bool RecvEntry::DmaBuf::full() {
    return ( m_offset == m_buf.size() );
}

inline unsigned char* RecvEntry::DmaBuf::bufPtr()
{
    return &m_buf.at( 0 );
}

}
}

#endif

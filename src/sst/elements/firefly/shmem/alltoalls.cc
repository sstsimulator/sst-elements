// Copyright 2013-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
#include <sst_config.h>

#include "shmem/alltoalls.h"
#include "hadesSHMEM.h"

using namespace SST;
using namespace Firefly;

void ShmemAlltoalls::start( Vaddr dest, Vaddr source, int dst, int sst, size_t nelems, int elsize, int PE_start,
    int logPE_stride, int PE_size, Hermes::Vaddr pSync, Hermes::Shmem::Callback callback )
{
    printf(":%d:%s():%d nelems=%lu PE_start=%d stride=%d\n",
            my_pe(),__func__,__LINE__,nelems,PE_start,logPE_stride);

    assert( PE_size > 1 );
    if ( 0 == nelems ) {
        m_api.delay(callback,0,0);
        return;
    }

    m_pSync = pSync;
    m_returnCallback = callback;
    m_logPE_stride = logPE_stride;
    m_PE_start = PE_start;
    m_PE_size = PE_size;
    m_nelems = nelems;
    m_elem_size = elsize;

    m_dst = dst;
    m_sst = sst;

    m_dest_base = dest + my_as_rank() * m_nelems * dst * elsize;
    m_src_base = source;

    m_start_pe = m_peer = m_common.circular_iter_next(m_common.my_pe(), PE_start, logPE_stride, PE_size);

    do_peer_1( 0 );
}

void ShmemAlltoalls::do_peer_0( int )
{
    printf(":%d:%s():%d peer=%d \n",my_pe(),__func__,__LINE__,m_peer);
    m_peer = m_common.circular_iter_next(m_peer, m_PE_start, m_logPE_stride, m_PE_size);

    if ( m_peer != m_start_pe ) {
        do_peer_1(0);
    } else {
        barrier(0);
    }
}

void ShmemAlltoalls::do_peer_1( int )
{
    int peer_as_rank = (m_peer - m_PE_start) / stride();
    m_dest =  m_dest_base;
    m_src =  m_src_base + peer_as_rank * m_nelems * m_sst * m_elem_size;

    m_iteration = m_nelems;

    put(0);
}

void ShmemAlltoalls::put( int )
{
    printf(":%d:%s():%d src=%#" PRIx64" dest=%#" PRIx64 " peer=%d\n",my_pe(),__func__,__LINE__,
            m_src, m_dest, m_peer );

    Shmem::Callback callback;
    if ( m_iteration - 1 > 0 ) {
        callback = std::bind( &ShmemAlltoalls::put, this, std::placeholders::_1 );
    } else {
        callback = std::bind( &ShmemAlltoalls::do_peer_0, this, std::placeholders::_1 );
    }

    m_api.put( m_dest, m_src, m_elem_size, m_peer, true, callback );
    //shmem_internal_put_nbi((void *) dest_ptr, (uint8_t *) source + peer_as_rank * len, len, peer);
    --m_iteration;

    m_src  += m_sst * m_elem_size;
    m_dest += m_dst * m_elem_size;
}


void ShmemAlltoalls::barrier( int )
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    m_api.barrier( m_PE_start, m_logPE_stride, m_PE_size, m_pSync,
            std::bind( &ShmemAlltoalls::fini, this, std::placeholders::_1 )
        );
    //shmem_internal_barrier(PE_start, logPE_stride, PE_size, pSync);
}

void ShmemAlltoalls::fini( int )
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    m_api.putv( m_pSync, m_zero, my_pe(), m_returnCallback );
#if 0
    for (i = 0; i < SHMEM_BARRIER_SYNC_SIZE; i++)
        pSync[i] = SHMEM_SYNC_VALUE;
#endif
}


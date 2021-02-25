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

#include "shmem/alltoall.h"
#include "hadesSHMEM.h"

using namespace SST;
using namespace Firefly;

void ShmemAlltoall::start( Vaddr dest, Vaddr source, size_t nelems, int PE_start,
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

    m_dest = dest  + ( ( m_common.my_pe() - PE_start ) / (1 << logPE_stride) )  * nelems;
    m_src = source;

    m_start_pe = m_common.circular_iter_next(m_common.my_pe(), PE_start, logPE_stride, PE_size);
    m_peer = m_start_pe;

    put( 0 );
}

void ShmemAlltoall::put( int )
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    int next_peer = m_common.circular_iter_next(m_peer, m_PE_start, m_logPE_stride, m_PE_size);

    Shmem::Callback callback;
    if ( next_peer != m_start_pe ) {
        callback = std::bind( &ShmemAlltoall::put, this, std::placeholders::_1 );
    } else {
        callback = std::bind( &ShmemAlltoall::barrier, this, std::placeholders::_1 );
    }
    int peer_as_rank = (m_peer - m_PE_start) / (1 << m_logPE_stride); /* Peer's index in active set */

    printf(":%d:%s():%d src=%#" PRIx64 " dest=%#" PRIx64 " peer=%d\n",my_pe(),__func__,__LINE__,
            m_src + peer_as_rank * m_nelems, m_dest,m_peer);

    m_api.put( m_dest, m_src + peer_as_rank * m_nelems, m_nelems, m_peer, true, callback );
    //shmem_internal_put_nbi((void *) dest_ptr, (uint8_t *) source + peer_as_rank * len, len, peer);

    m_peer = next_peer;
}

void ShmemAlltoall::barrier( int )
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    m_api.barrier( m_PE_start, m_logPE_stride, m_PE_size, m_pSync,
            std::bind( &ShmemAlltoall::fini, this, std::placeholders::_1 )
        );
    //shmem_internal_barrier(PE_start, logPE_stride, PE_size, pSync);
}

void ShmemAlltoall::fini( int )
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    m_api.putv( m_pSync, m_zero, my_pe(), m_returnCallback );
#if 0
    for (i = 0; i < SHMEM_BARRIER_SYNC_SIZE; i++)
        pSync[i] = SHMEM_SYNC_VALUE;
#endif
}


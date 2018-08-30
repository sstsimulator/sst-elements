// Copyright 2013-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "shmem/fcollect.h"
#include "hadesSHMEM.h"

using namespace SST;
using namespace Firefly;

void ShmemFcollect::start( Vaddr dest, Vaddr source, size_t nelems, int PE_start, int logPE_stride,
        int PE_size, Hermes::Vaddr pSync, Hermes::Shmem::Callback returnCallback )
{
    printf(":%d:%s():%d dest=%#" PRIx64 " src=%#" PRIx64 " nelems=%lu PE_start=%d stride=%d\n",
            my_pe(),__func__,__LINE__,dest,source,nelems,PE_start,logPE_stride);

    m_dest = dest;
    m_src = source;
    m_nelems = nelems;
    m_PE_start = PE_start;
    m_logPE_stride = logPE_stride;
    m_PE_size = PE_size;
    m_pSync = pSync;
    m_returnCallback = returnCallback;

    Shmem::Callback callback;
    if ( m_PE_size == 1 ) {
        callback = std::bind( &ShmemFcollect::state_5, this, std::placeholders::_1 );
    } else {
        callback = std::bind( &ShmemFcollect::state_0, this, std::placeholders::_1 );
    }

    printf(":%d:%s():%d my_id=%d\n",my_pe(),__func__,__LINE__,my_id());
    m_api.memcpy( dest + (my_id()*nelems), source, nelems, callback );
    m_iteration = 1;
}

void ShmemFcollect::state_0( int )
{
    size_t iter_offset = ((my_id() + 1 - m_iteration + m_PE_size) % m_PE_size) * m_nelems;
    printf(":%d:%s():%d iter_offset=%lu dest=%#" PRIx64 "\n",my_pe(),__func__,__LINE__,iter_offset,(uint64_t)m_dest + iter_offset);

    m_api.put( m_dest + iter_offset, m_dest + iter_offset, m_nelems, next_proc(), 
        std::bind( &ShmemFcollect::state_1, this, std::placeholders::_1 ) );
}

void ShmemFcollect::state_1( int )
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    m_api.fence( std::bind( &ShmemFcollect::state_2, this, std::placeholders::_1 ) ) ;
    //shmem_internal_fence();
}

void ShmemFcollect::state_2( int )
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
   /* send completion for this round to next proc.  Note that we
       only ever sent to next_proc and there's a shmem_fence
       between successive calls to the put above.  So a rolling
       counter is safe here. */
    m_api.add( m_pSync, m_one, next_proc(), 
                  std::bind( &ShmemFcollect::state_3, this, std::placeholders::_1 ));  
    //shmem_internal_atomic_small(pSync, &one, sizeof(long), next_proc, SHM_INTERNAL_SUM, SHM_INTERNAL_LONG);
}

void ShmemFcollect::state_3( int )
{
    printf(":%d:%s():%d iteration=%d\n",my_pe(),__func__,__LINE__,m_iteration);
    m_value.set( (long) m_iteration );

    Shmem::Callback callback;
    if ( m_iteration < m_PE_size ) {
        callback = std::bind( &ShmemFcollect::state_0, this, std::placeholders::_1 );
    } else {
        callback = std::bind( &ShmemFcollect::state_4, this, std::placeholders::_1 );
    }
	++m_iteration;
    /* wait for completion for this round */
    m_api.wait_until( m_pSync, Shmem::GTE, m_value, callback );
    //SHMEM_WAIT_UNTIL(pSync, SHMEM_CMP_GE, i);
}


void ShmemFcollect::state_4( int )
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    /* zero out psync */
    m_api.putv( m_pSync, m_zero, m_common.my_pe(), 
           std::bind( &ShmemFcollect::state_5, this, std::placeholders::_1 ) );
    //shmem_internal_put_small(pSync, &zero, sizeof(long), shmem_internal_my_pe);
} 
  
void ShmemFcollect::state_5( int )
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    m_api.wait_until( m_pSync, Shmem::EQ, m_zero, m_returnCallback );
    //SHMEM_WAIT_UNTIL(pSync, SHMEM_CMP_EQ, 0);
}

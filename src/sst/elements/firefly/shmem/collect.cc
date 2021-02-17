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

#include "shmem/collect.h"
#include "hadesSHMEM.h"

using namespace SST;
using namespace Firefly;

void ShmemCollect::start( Vaddr dest, Vaddr source, size_t nelems, int PE_start, int logPE_stride,
        int PE_size, Hermes::Vaddr pSync, Hermes::MemAddr* scratch, Hermes::Shmem::Callback callback )
{
    printf(":%d:%s():%d collect nelems=%lu PE_start=%d stride=%d\n",
            my_pe(),__func__,__LINE__,nelems,PE_start,logPE_stride);

    m_dest = dest;
    m_src = source;
    m_nelems = nelems;
    m_PE_start = PE_start;
    m_logPE_stride = logPE_stride;
    m_PE_size = PE_size;
    m_pSync = pSync;
    m_scratch = scratch;
    m_returnCallback = callback;

    if ( PE_size == 1 ) {
        m_api.memcpy( dest, source, nelems, callback);
        return;
    }

    m_my_offset = 0;
    if ( PE_start == m_common.my_pe() ) {

        m_scratch->at<pSync_t>(0) = m_nelems;
        m_scratch->at<pSync_t>(1) = 1;
        //tmp[0] = (long) len; /* FIXME: Potential truncation of size_t into long */
        //tmp[1] = 1; /* FIXME: Packing flag with data relies on byte ordering */
        printf(":%d:%s():%d send offset to %d\n",my_pe(),__func__,__LINE__, PE_start + stride());
        m_api.put( m_pSync, m_scratch->getSimVAddr(), sizeof(pSync_t) * 2, PE_start + stride(), true,
           std::bind( &ShmemCollect::do_data_0, this, std::placeholders::_1 ) );

        //shmem_internal_put_small(pSync, tmp, 2 * sizeof(long), PE_start + stride);

    } else {

        m_api.wait_until( m_pSync + sizeof(pSync_t), Shmem::EQ, m_one,
           std::bind( &ShmemCollect::do_offset_0, this, std::placeholders::_1 ) );
        /* wait for send data */
        //SHMEM_WAIT_UNTIL(&pSync[1], SHMEM_CMP_EQ, 1);
    }
}

void ShmemCollect::do_offset_0(int )
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    m_my_offset = *(pSync_t*)m_api.getBacking( m_pSync );
    //my_offset = pSync[0];

    /* Not the last guy, so send offset to next PE */
    if ( m_common.my_pe() < m_PE_start + stride() * (m_PE_size - 1)) {
        m_scratch->at<pSync_t>(0) = m_my_offset + m_nelems;
        m_scratch->at<pSync_t>(1) = 1;
        //tmp[0] = (long) (my_offset + len);
        //tmp[1] = 1;

        printf(":%d:%s():%d send offset to %d\n",my_pe(),__func__,__LINE__, m_common.my_pe() + stride());
        m_api.put( m_pSync, m_scratch->getSimVAddr(), sizeof(pSync_t) * 2, m_common.my_pe() + stride(), true,
           std::bind( &ShmemCollect::do_data_0, this, std::placeholders::_1 ) );
        //shmem_internal_put_small(pSync, tmp, 2 * sizeof(long), shmem_internal_my_pe + stride);
        //
    } else {
        do_data_0(0);
    }
}


void ShmemCollect::do_data_0(int )
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    /* Send data round-robin, ending with my PE */
    m_start_pe = m_peer = m_common.circular_iter_next(my_pe(), m_PE_start, m_logPE_stride, m_PE_size);

    do_data_1(0);
}

void ShmemCollect::do_data_1(int )
{
    printf(":%d:%s():%d m_peer=%d,m_my_offset=%#lx\n",my_pe(),__func__,__LINE__,m_peer,m_my_offset);
    if (m_nelems > 0) {
        m_api.put( m_dest + m_my_offset, m_src, m_nelems, m_peer, true,
           std::bind( &ShmemCollect::do_data_2, this, std::placeholders::_1 ) );
        //shmem_internal_put_nbi(((uint8_t *) target) + my_offset, source, len, peer);
    } else {
        do_data_2( 0 );
    }
}

void ShmemCollect::do_data_2(int )
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    m_peer = m_common.circular_iter_next(m_peer, m_PE_start, m_logPE_stride, m_PE_size);
    if ( m_peer != m_start_pe ) {
        do_data_1(0);
    } else {
        m_api.barrier(m_PE_start, m_logPE_stride, m_PE_size, m_pSync + sizeof( pSync_t) * 2,
            std::bind( &ShmemCollect::fini, this, std::placeholders::_1 ));
        //shmem_internal_barrier(PE_start, logPE_stride, PE_size, &pSync[2]);
    }
}

void ShmemCollect::fini(int )
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    pSync_t* ptr = (pSync_t*) m_api.getBacking( m_pSync );
    ptr[0] = 0;
    ptr[1] = 0;
    ptr[2] = 0;

#if 0

    pSync[0] = SHMEM_SYNC_VALUE;
    pSync[1] = SHMEM_SYNC_VALUE;

    for (i = 0; i < SHMEM_BARRIER_SYNC_SIZE; i++)
        pSync[2+i] = SHMEM_SYNC_VALUE;
#endif

    m_returnCallback( 0 );
//    ShmemCollective::fini( 0 );
}

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

#include "shmem/reduction.h"
#include "hadesSHMEM.h"

using namespace SST;
using namespace Firefly;

void ShmemReduction::start( Hermes::Vaddr dest, Hermes::Vaddr source, size_t nelems, int PE_start,
    int logPE_stride, int PE_size, Hermes::Vaddr pSync, 
    Hermes::Shmem::ReduOp op, Hermes::Value::Type dataType, Hermes::Shmem::Callback callback )
{
    printf(":%d:%s():%d nelems=%lu PE_start=%d stride=%d op=%d dataType=%d\n",
            my_pe(),__func__,__LINE__,nelems,PE_start,logPE_stride, op, dataType);

    assert( PE_size > 1 );

    m_returnCallback = callback;
    m_pSync = pSync;
    m_src = source;
    m_dest = dest;
    m_nelems = nelems;
    m_op = op;
    m_dataType = dataType;

    m_PE_start = PE_start;
    m_PE_stride = logPE_stride;
    m_PE_size = PE_size;

    if (PE_size == num_pes() ) {
        /* we're the full tree, use the binomial tree */
        m_parent = full_tree_parent();
        m_num_children = full_tree_num_children();
        m_children = &full_tree_children()[0];
    } else {
        printf(":%d:%s():%d part full tree\n",my_pe(),__func__,__LINE__);
        m_part_tree_children.resize( tree_radix() );
        m_children = &m_part_tree_children[0];
        m_common.build_kary_tree(tree_radix(), PE_start, 1 << logPE_stride,
                        PE_size, 0, &m_parent, &m_num_children, m_children);
    }

    m_iteration = 0;

    if ( 0 != m_num_children ) {
        have_children_0(0);
    } else {
        not_root_0(0);
    }
}

void ShmemReduction::have_children_0(int) 
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    /* update our target buffer with our contribution.  The put
       will flush any atomic cache value that may currently
       exist. */
    int dataSize = Hermes::Value::getLength( m_dataType );
   
    m_api.put( m_dest, m_src, m_nelems * dataSize , my_pe(), 
           std::bind( &ShmemReduction::have_children_1, this, std::placeholders::_1 ) );
    // shmem_internal_put_nb(target, source, count * type_size, shmem_internal_my_pe, &completion);
}   

void ShmemReduction::have_children_1(int) 
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    m_api.quiet( std::bind( &ShmemReduction::have_children_2, this, std::placeholders::_1 ) );
    //shmem_internal_quiet();
    m_iteration = 0;
}

void ShmemReduction::have_children_2(int) 
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    /* let everyone know that it's safe to send to us */
    Shmem::Callback callback;
    if ( m_iteration < m_num_children -1 ) {
        callback =  std::bind( &ShmemReduction::have_children_2, this, std::placeholders::_1 );
    } else {
        callback =  std::bind( &ShmemReduction::have_children_3, this, std::placeholders::_1 );
    }

    m_api.putv( m_pSync + sizeof(pSync_t), m_one, m_children[m_iteration], callback );
    ++m_iteration;
    //for (i = 0 ; i < num_children ; ++i) {
    //    shmem_internal_put_small(pSync + 1, &one, sizeof(one), children[i]);
    //}
}

void ShmemReduction::have_children_3(int) 
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    /* Wait for others to acknowledge sending data */
    m_value.set( (long) m_num_children );
    m_api.wait_until( m_pSync, Shmem::EQ, m_value,
            std::bind( &ShmemReduction::have_children_4, this, std::placeholders::_1 ) ); 
    //SHMEM_WAIT_UNTIL(pSync, SHMEM_CMP_EQ, num_children);
} 

void ShmemReduction::have_children_4(int) 
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    /* reset pSync */
    m_api.putv( m_pSync, m_zero, my_pe(), 
            std::bind( &ShmemReduction::have_children_5, this, std::placeholders::_1 ) );
    //shmem_internal_put_small(pSync, &zero, sizeof(zero), shmem_internal_my_pe);
}

void ShmemReduction::have_children_5(int) 
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    Shmem::Callback callback;
    if ( m_parent != my_pe() ) {
        callback = std::bind( &ShmemReduction::not_root_0, this, std::placeholders::_1 ); 
    } else {
        callback = std::bind( &ShmemReduction::bcast, this, std::placeholders::_1 ); 
    }
    m_api.wait_until( m_pSync, Shmem::EQ, m_zero, callback );
    //SHMEM_WAIT_UNTIL(pSync, SHMEM_CMP_EQ, 0);
}

void ShmemReduction::not_root_0(int) 
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    /* wait for clear to send */
    m_api.wait_until( m_pSync + sizeof(pSync_t) , Shmem::NE, m_zero,
                std::bind( &ShmemReduction::not_root_1, this, std::placeholders::_1 ) );
    //SHMEM_WAIT(pSync + 1, 0);
}

void ShmemReduction::not_root_1(int) 
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    /* reset pSync */
    m_api.putv( m_pSync + sizeof(pSync_t), m_zero, my_pe(),
                std::bind( &ShmemReduction::not_root_2, this, std::placeholders::_1 ) );
    //shmem_internal_put_small(pSync + 1, &zero, sizeof(zero), shmem_internal_my_pe);
}
void ShmemReduction::not_root_2(int) 
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    m_api.wait_until( m_pSync + sizeof(pSync_t), Shmem::EQ, m_zero,
                std::bind( &ShmemReduction::not_root_3, this, std::placeholders::_1 ) );
    //SHMEM_WAIT_UNTIL(pSync + 1, SHMEM_CMP_EQ, 0);
}

void ShmemReduction::not_root_3(int) 
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    /* send data, ack, and wait for completion */
    int dataSize  = Hermes::Value::getLength( m_dataType );
    
    m_api.putOp( m_dest, ( m_num_children == 0) ? m_src : m_dest, 
            m_nelems * dataSize, m_parent, m_op, m_dataType,
            std::bind( &ShmemReduction::not_root_4, this, std::placeholders::_1 ) );

    //shmem_internal_atomic_nb(target, (num_children == 0) ? source : target,
    //                             count * type_size, parent,
    //                             op, datatype, &completion);
}
void ShmemReduction::not_root_4(int) 
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    m_api.fence( std::bind( &ShmemReduction::not_root_5, this, std::placeholders::_1 ) );
    //shmem_internal_fence();
}

void ShmemReduction::not_root_5(int) 
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    m_api.add( m_pSync, m_one, m_parent,
                std::bind( &ShmemReduction::bcast, this, std::placeholders::_1 ) );
    //shmem_internal_atomic_small(pSync, &one, sizeof(one), parent, SHM_INTERNAL_SUM, SHM_INTERNAL_LONG);
}

void ShmemReduction::bcast(int) 
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);

    /* broadcast out */
    int dataSize  = Hermes::Value::getLength( m_dataType );
    m_api.broadcast( m_dest, m_dest, m_nelems * dataSize, 0, m_PE_start, m_PE_stride, m_PE_size, 
            m_pSync + sizeof(pSync_t) * 2, m_returnCallback );
    //shmem_internal_bcast(target, target, count * type_size, 0, PE_start, logPE_stride, PE_size, pSync + 2, 0);
}

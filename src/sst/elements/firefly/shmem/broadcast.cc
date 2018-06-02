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

#include "shmem/broadcast.h"
#include "hadesSHMEM.h"

using namespace SST;
using namespace Firefly;

void ShmemBroadcast::start( Vaddr dest, Vaddr source, size_t nelems, int root, int PE_start,
    int logPE_stride, int PE_size, Hermes::Vaddr pSync, Hermes::Shmem::Callback callback, bool complete )
{
    printf(":%d:%s():%d nelems=%lu PE_root=%d PE_start=%d stride=%d\n",
            my_pe(),__func__,__LINE__,nelems,root,PE_start,logPE_stride);

    m_returnCallback = callback;
    m_pSync = pSync;
    m_complete = complete;
    m_src = source;
    m_dest = dest;
    m_nelems = nelems;
    assert( root < num_pes() );
    assert( PE_size > 1 );

    if (PE_size == num_pes() && 0 == root) {
        /* we're the full tree, use the binomial tree */
        m_parent = full_tree_parent();
        m_num_children = full_tree_num_children();
        m_children = &full_tree_children()[0];
    } else {
        printf(":%d:%s():%d part tree\n",my_pe(),__func__,__LINE__);
        m_part_tree_children.resize( tree_radix() );
        m_children = &m_part_tree_children[0];
        m_common.build_kary_tree(tree_radix(), PE_start, 1 << logPE_stride,
                        PE_size, root, &m_parent, &m_num_children, m_children);
    }

    for ( int i = 0; i < m_num_children; i++ ) {
        printf(":%d:%s():%d %d \n",my_pe(),__func__,__LINE__,m_children[i]);
    } 
    m_iteration = 0;

    if ( m_num_children != 0 ) {
        if (m_parent != my_pe()) {
            node_0(0);
        } else {
            node_2(0);
        }
    } else {
        leaf_0(0);
    }
}

void ShmemBroadcast::node_0(int)
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    m_src = m_dest;
//    send_buf = target;

    /* wait for data arrival message if not the root */
    m_api.wait_until( m_pSync, Shmem::NE, m_zero,
                       std::bind( &ShmemBroadcast::node_1, this, std::placeholders::_1 ) );
    //SHMEM_WAIT(pSync, 0);
} 

void ShmemBroadcast::node_1(int)
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    /* if complete, send ack */
    if ( m_complete) {
        m_api.add( m_pSync, m_one, m_parent,
                std::bind( &ShmemBroadcast::node_2, this, std::placeholders::_1 ) );
        //shmem_internal_atomic_small(pSync, &one, sizeof(one), parent,
        //                                    SHM_INTERNAL_SUM, SHM_INTERNAL_LONG);
     } else {
         node_2(0);
     }
}

void ShmemBroadcast::node_2(int)
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    /* send data to all leaves */
    /* send completion ack to all peers */
    Shmem::Callback callback;
    if ( m_iteration < m_num_children - 1) {
        callback =  std::bind( &ShmemBroadcast::node_2, this, std::placeholders::_1 );
    } else {
        callback =  std::bind( &ShmemBroadcast::node_3, this, std::placeholders::_1 );
    }

    assert( m_children[m_iteration] != my_pe() );
    m_api.put( m_dest, m_src, m_nelems, m_children[m_iteration], callback );
    ++m_iteration;

    //for (i = 0 ; i < num_children ; ++i) {
    //   shmem_internal_put_nb(target, send_buf, len, children[i], &completion);
    //}
}

void ShmemBroadcast::node_3(int)
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);

    m_iteration = 0;

    m_api.fence( std::bind( &ShmemBroadcast::node_4, this, std::placeholders::_1 ) );
    //shmem_internal_fence();
}

void ShmemBroadcast::node_4(int)
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    /* send completion ack to all peers */
    Shmem::Callback callback;
    if ( m_iteration < m_num_children - 1 ) {
        callback =  std::bind( &ShmemBroadcast::node_4, this, std::placeholders::_1 );
    } else {
        callback =  std::bind( &ShmemBroadcast::node_5, this, std::placeholders::_1 );
    }

    m_api.putv( m_pSync, m_one, m_children[m_iteration], callback );
    ++m_iteration;
    //for (i = 0 ; i < num_children ; ++i) {
    //   shmem_internal_put_small(pSync, &one, sizeof(long), children[i]);
    //}
}

void ShmemBroadcast::node_5(int)
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    if (m_complete) {
        /* wait for acks from everyone */
        m_value.set( (long) m_num_children + ((m_parent == my_pe()) ?  0 : 1) );
        m_api.wait_until( m_pSync, Shmem::EQ, m_value,
                std::bind( &ShmemBroadcast::node_6, this, std::placeholders::_1 ) );
        //SHMEM_WAIT_UNTIL(pSync, SHMEM_CMP_EQ, num_children  +
        //                          ((parent == shmem_internal_my_pe) ?  0 : 1));
    } else {
        node_7(0);
    }
}

void ShmemBroadcast::node_6(int)
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    /* Clear pSync */
    m_api.putv( m_pSync, m_zero, my_pe(),
                std::bind( &ShmemBroadcast::node_7, this, std::placeholders::_1 ) );
    //shmem_internal_put_small(pSync, &zero, sizeof(zero), shmem_internal_my_pe);
}

void ShmemBroadcast::node_7(int)
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    m_api.wait_until( m_pSync, Shmem::EQ, m_zero,
                m_returnCallback );
                //std::bind( &ShmemBroadcast::fini, this, std::placeholders::_1 ) );
    //SHMEM_WAIT_UNTIL(pSync, SHMEM_CMP_EQ, 0);
} 


void ShmemBroadcast::leaf_0(int)
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    /* wait for data arrival message */
    m_api.wait_until( m_pSync, Shmem::NE, m_zero,
                       std::bind( &ShmemBroadcast::leaf_1, this, std::placeholders::_1 ) );
    //SHMEM_WAIT(pSync, 0);
}

void ShmemBroadcast::leaf_1(int v)
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    /* if complete, send ack */
    if (m_complete) {
        m_api.add( m_pSync, m_one, m_parent,
                std::bind( &ShmemBroadcast::leaf_2, this, std::placeholders::_1 ) );
        //shmem_internal_atomic_small(pSync, &one, sizeof(one), parent, SHM_INTERNAL_SUM, SHM_INTERNAL_LONG);
    } else {
        leaf_2( v );
    }
}

void ShmemBroadcast::leaf_2(int)
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    /* Clear pSync */
    m_api.putv( m_pSync, m_zero, my_pe(),
                std::bind( &ShmemBroadcast::leaf_3, this, std::placeholders::_1 ) );
    //shmem_internal_put_small(pSync, &zero, sizeof(zero), shmem_internal_my_pe);
} 

void ShmemBroadcast::leaf_3(int)
{
    printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
    m_api.wait_until( m_pSync, Shmem::EQ, m_zero, m_returnCallback );
//                std::bind( &ShmemBroadcast::fini, this, std::placeholders::_1 ) );
    //SHMEM_WAIT_UNTIL(pSync, SHMEM_CMP_EQ, 0);
}

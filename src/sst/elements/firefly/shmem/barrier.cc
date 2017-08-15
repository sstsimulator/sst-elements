// Copyright 2013-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "shmem/barrier.h"
#include "hadesSHMEM.h"

using namespace SST;
using namespace Firefly;


void ShmemBarrier::start( int PE_start, int logPE_stride, int PE_size, 
        Hermes::Vaddr pSync, Hermes::Shmem::Callback returnCallback )
{
    
    printf(":%d:%s():%d\n",m_my_pe,__func__,__LINE__);
    m_returnCallback = returnCallback;
    m_pSync = pSync;

    if ( PE_size == m_num_pes ) {
        printf(":%d:%s():%d full tree\n",m_my_pe,__func__,__LINE__);
       /* we're the full tree, use the binomial tree */
        m_parent = m_full_tree_parent;
        m_num_children = m_full_tree_num_children;
        m_children = &m_full_tree_children[0];    
    } else {
        printf(":%d:%s():%d part full tree\n",m_my_pe,__func__,__LINE__);
        m_part_tree_children.resize( m_tree_radix );
        m_children = &m_part_tree_children[0];
        build_kary_tree(m_tree_radix, PE_start, 1 << logPE_stride , PE_size, 0, &m_parent, &m_num_children, m_children);
    } 
    printf(":%d:%s():%d parent=%d\n",m_my_pe,__func__,__LINE__,m_parent);

    if ( m_num_children != 0 ) {
        m_api.quiet( std::bind( &ShmemBarrier::not_leaf_0, this, std::placeholders::_1 ) );
    } else {
        m_api.quiet( std::bind( &ShmemBarrier::leaf_0, this, std::placeholders::_1 ) );
    }
}

void ShmemBarrier::not_leaf_0( int )
{
    printf(":%d:%s():%d num_children=%d\n",m_my_pe,__func__,__LINE__,m_num_children);
    m_value.set( (long) m_num_children ); 
    /* wait for num_children callins up the tree */
    if (m_parent == m_my_pe) {
        m_api.wait_until( m_pSync, Shmem::EQ, m_value, 
           std::bind( &ShmemBarrier::root_0, this, std::placeholders::_1 ) );
    } else {
        m_api.wait_until( m_pSync, Shmem::EQ, m_value, 
           std::bind( &ShmemBarrier::node_0, this, std::placeholders::_1 ) );
    }

    // SHMEM_WAIT_UNTIL(pSync, SHMEM_CMP_EQ, num_children);
}

void ShmemBarrier::root_0( int )
{
    printf(":%d:%s():%d\n",m_my_pe,__func__,__LINE__);

    /* Clear pSync */
    m_api.putv( m_pSync, m_zero, m_my_pe, 
           std::bind( &ShmemBarrier::root_1, this, std::placeholders::_1 ) );

    //shmem_internal_put_small(pSync, &zero, sizeof(zero), shmem_internal_my_pe); 
} 
void ShmemBarrier::root_1( int )
{
    printf(":%d:%s():%d\n",m_my_pe,__func__,__LINE__);

    m_api.wait_until( m_pSync, Shmem::EQ, m_zero, 
           std::bind( &ShmemBarrier::root_2, this, std::placeholders::_1 ) );
    //SHMEM_WAIT_UNTIL(pSync, SHMEM_CMP_EQ, 0);

    m_iteration = 0;
}

void ShmemBarrier::root_2( int )
{
    printf(":%d:%s():%d child=%d\n",m_my_pe,__func__,__LINE__,m_children[m_iteration]);

    Shmem::Callback callback;
    if ( m_iteration < m_num_children - 1 ) {
        callback =  std::bind( &ShmemBarrier::root_2, this, std::placeholders::_1 );
    } else {
        callback =  std::bind( &ShmemBarrier::fini, this, std::placeholders::_1 );
    }

    m_api.fadd( m_retval, m_pSync, m_one, m_children[m_iteration], callback );
    ++m_iteration;
#if 0
    /* Send acks down to children */
    for (i = 0 ; i < num_children ; ++i) {
        shmem_internal_atomic_small(pSync, &one, sizeof(one),
                                            children[i],
                                            SHM_INTERNAL_SUM, SHM_INTERNAL_LONG);
    }
#endif
}


void ShmemBarrier::node_0( int )
{
    printf(":%d:%s():%d\n",m_my_pe,__func__,__LINE__);
    /* send ack to parent */
    m_api.fadd( m_retval, m_pSync, m_one, m_parent, 
           std::bind( &ShmemBarrier::node_1, this, std::placeholders::_1 ) );
    //shmem_internal_atomic_small(pSync, &one, sizeof(one),
                // parent, SHM_INTERNAL_SUM, SHM_INTERNAL_LONG);
}

void ShmemBarrier::node_1( int )
{
    printf(":%d:%s():%d\n",m_my_pe,__func__,__LINE__);
    /* wait for ack from parent */
    m_value.set( (long) m_num_children + 1 ); 
    m_api.wait_until( m_pSync, Shmem::EQ, m_value, 
           std::bind( &ShmemBarrier::node_2, this, std::placeholders::_1 ) );
    //SHMEM_WAIT_UNTIL(pSync, SHMEM_CMP_EQ, num_children  + 1);
} 

void ShmemBarrier::node_2( int )
{
    printf(":%d:%s():%d\n",m_my_pe,__func__,__LINE__);
    /* Clear pSync */
    m_api.putv( m_pSync, m_zero, m_my_pe, 
           std::bind( &ShmemBarrier::node_3, this, std::placeholders::_1 ) );
    //shmem_internal_put_small(pSync, &zero, sizeof(zero), shmem_internal_my_pe);
}
    
void ShmemBarrier::node_3( int )
{
    printf(":%d:%s():%d\n",m_my_pe,__func__,__LINE__);
    m_api.wait_until( m_pSync, Shmem::EQ, m_zero, 
           std::bind( &ShmemBarrier::node_4, this, std::placeholders::_1 ) );
    //SHMEM_WAIT_UNTIL(pSync, SHMEM_CMP_EQ, 0);
    m_iteration = 0; 
}

void ShmemBarrier::node_4( int )
{
    printf(":%d:%s():%d\n",m_my_pe,__func__,__LINE__);
    Shmem::Callback callback;
    if ( m_iteration < m_num_children - 1 ) {
        callback =  std::bind( &ShmemBarrier::node_4, this, std::placeholders::_1 );
    } else {
        callback =  std::bind( &ShmemBarrier::fini, this, std::placeholders::_1 );
    }

    m_api.fadd( m_retval, m_pSync, m_one, m_children[m_iteration], callback ); 
    ++m_iteration;

#if 0
    /* Send acks down to children */
    for (i = 0 ; i < num_children ; ++i) {
        shmem_internal_atomic_small(pSync, &one, sizeof(one), children[i], 
                SHM_INTERNAL_SUM, SHM_INTERNAL_LONG);
    }
#endif
}

void ShmemBarrier::leaf_0( int )
{
    printf(":%d:%s():%d\n",m_my_pe,__func__,__LINE__);
   /* send message up psync tree */
    m_api.fadd( m_retval, m_pSync, m_one, m_parent, 
           std::bind( &ShmemBarrier::leaf_1, this, std::placeholders::_1 ) );

#if 0
    shmem_internal_atomic_small(pSync, &one, sizeof(one), parent,
                                    SHM_INTERNAL_SUM, SHM_INTERNAL_LONG);
#endif
}

void ShmemBarrier::leaf_1( int )
{
    printf(":%d:%s():%d\n",m_my_pe,__func__,__LINE__);
    /* wait for ack down psync tree */
    m_api.wait_until( m_pSync, Shmem::NE, m_zero, 
           std::bind( &ShmemBarrier::leaf_2, this, std::placeholders::_1 ) );

   //   SHMEM_WAIT(pSync, 0);
}

void ShmemBarrier::leaf_2( int )
{
    printf(":%d:%s():%d\n",m_my_pe,__func__,__LINE__);
    /* Clear pSync */
    m_api.putv( m_pSync, m_zero, m_my_pe, 
           std::bind( &ShmemBarrier::leaf_3, this, std::placeholders::_1 ) );
#if 0
        shmem_internal_put_small(pSync, &zero, sizeof(zero),
                                 shmem_internal_my_pe);
#endif
}

void ShmemBarrier::leaf_3( int )
{
    printf(":%d:%s():%d\n",m_my_pe,__func__,__LINE__);
    m_api.wait_until( m_pSync, Shmem::EQ, m_zero, 
           std::bind( &ShmemBarrier::fini, this, std::placeholders::_1 ) );
#if 0
    SHMEM_WAIT_UNTIL(pSync, SHMEM_CMP_EQ, 0);
#endif
}
    
void ShmemBarrier::init(int requested_crossover, int requested_radix)
{
    int i, j, k;
    int tmp_radix;
    int my_root = 0;
    char *type;

    m_tree_radix = requested_radix;
    m_tree_crossover = requested_crossover;

#if 0
    /* initialize barrier_all psync array */
    m_barrier_all_psync =
        m_shmalloc(sizeof(long) * SHMEM_BARRIER_SYNC_SIZE);
    if (NULL == m_barrier_all_psync) return -1;

    for (i = 0; i < SHMEM_BARRIER_SYNC_SIZE; i++)
        m_barrier_all_psync[i] = SHMEM_SYNC_VALUE;
#endif

    /* initialize the binomial tree for collective operations over
       entire tree */
    m_full_tree_num_children = 0;
    for (i = 1 ; i <= m_num_pes ; i *= m_tree_radix) {
        tmp_radix = (m_num_pes / i < m_tree_radix) ?
            (m_num_pes / i) + 1 : m_tree_radix;
        my_root = (m_my_pe / (tmp_radix * i)) * (tmp_radix * i);
        if (my_root != m_my_pe) break;
        for (j = 1 ; j < tmp_radix ; ++j) {
            if (m_my_pe + i * j < m_num_pes) {
                m_full_tree_num_children++;
            }
        }
    }

    m_full_tree_children.resize( m_full_tree_num_children );

    k = m_full_tree_num_children - 1;
    for (i = 1 ; i <= m_num_pes ; i *= m_tree_radix) {
        tmp_radix = (m_num_pes / i < m_tree_radix) ?
            (m_num_pes / i) + 1 : m_tree_radix;
        my_root = (m_my_pe / (tmp_radix * i)) * (tmp_radix * i);
        if (my_root != m_my_pe) break;
        for (j = 1 ; j < tmp_radix ; ++j) {
            if (m_my_pe + i * j < m_num_pes) {
                m_full_tree_children[k--] = m_my_pe + i * j;
            }
        }
    }

    m_full_tree_parent = my_root;
}

void ShmemBarrier::build_kary_tree(int radix, int PE_start, int stride,
                               int PE_size, int PE_root, int *parent,
                               int *num_children, int *children)
{
    int i;

    /* my_id is the index in a theoretical 0...N-1 array of
       participating tasks. where the 0th entry is the root */
    int my_id = (((m_my_pe - PE_start) / stride) + PE_size - PE_root) % PE_size;

    /* We shift PE_root to index 0, resulting in a PE active set layout of (for
       example radix 2): 0 [ 1 2 ] [ 3 4 ] [ 5 6 ] ...  The first group [ 1 2 ]
       are chilren of 0, second group [ 3 4 ] are chilren of 1, and so on */
    *parent = PE_start + (((my_id - 1) / radix + PE_root) % PE_size) * stride;

    *num_children = 0;
    for (i = 1 ; i <= radix ; ++i) {
        int tmp = radix * my_id + i;
        if (tmp < PE_size) {
            const int child_idx = (PE_root + tmp) % PE_size;
            children[(*num_children)++] = PE_start + child_idx * stride;
        }
    }

#define DEBUG_STR(x)
    if (m_debug) {
        size_t len;
        char debug_str[256];
        len = snprintf(debug_str, sizeof(debug_str), "Building k-ary tree:"
                       "\n\t\tradix=%d, PE_start=%d, stride=%d, PE_size=%d, PE_root=%d\n",
                       radix, PE_start, stride, PE_size, PE_root);

        len += snprintf(debug_str+len, sizeof(debug_str) - len, "\t\tid=%d, parent=%d, children[%d] = { ",
                        my_id, *parent, *num_children);

        for (i = 0; i < *num_children && len < sizeof(debug_str); i++)
            len += snprintf(debug_str+len, sizeof(debug_str) - len, "%d ",
                            children[i]);

        if (len < sizeof(debug_str))
            len += snprintf(debug_str+len, sizeof(debug_str) - len, "}");

        DEBUG_STR(debug_str);
    }
}

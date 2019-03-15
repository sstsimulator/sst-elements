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

#include "shmem/common.h"

using namespace SST;
using namespace Firefly;

ShmemCommon::ShmemCommon( int my_pe, int num_pes, int requested_crossover, int requested_radix):
    m_my_pe( my_pe ), m_num_pes(num_pes), m_debug(false)
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

void ShmemCommon::build_kary_tree(int radix, int PE_start, int stride,
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

int ShmemCommon::circular_iter_next(int curr, int PE_start, int logPE_stride, int PE_size)
{
    const int stride = 1 << logPE_stride;
    const int last = PE_start + (stride * (PE_size - 1));
    int next;

    next = curr + stride;
    if (next > last)
        next = PE_start;

    return next;
}


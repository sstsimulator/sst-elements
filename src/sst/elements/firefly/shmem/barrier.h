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


#ifndef COMPONENTS_FIREFLY_SHMEM_BARRIER_H
#define COMPONENTS_FIREFLY_SHMEM_BARRIER_H

#include <vector>
#include <sst/core/params.h>
#include "sst/elements/hermes/shmemapi.h"


#if 1 
#undef printf
#define printf(x,...) 
#endif

namespace SST {
namespace Firefly {

class HadesSHMEM;

class ShmemBarrier {
  public:
    ShmemBarrier( HadesSHMEM& api, int my_pe, int num_pes ) : m_api(api), m_my_pe(my_pe), m_num_pes( num_pes ), 
        m_value( Hermes::Value::Long), m_retval( Hermes::Value::Long ), m_one((long)1), m_zero((long)0), m_debug(false)
    { 
        init( 10, 2 );
    }
    void start( int PE_start, int logPE_stride, int PE_size, Hermes::Vaddr pSync, Hermes::Shmem::Callback );
  private:
    void init( int requested_crossover, int requested_radix );
    void build_kary_tree(int radix, int PE_start, int stride,
        int PE_size, int PE_root, int *parent, int *num_children, int *children);
    void not_leaf_0(int);
    void root_0(int);
    void root_1(int);
    void root_2(int);
    void node_0(int);
    void node_1(int);
    void node_2(int);
    void node_3(int);
    void node_4(int);
    void leaf_0(int);
    void leaf_1(int);
    void leaf_2(int);
    void leaf_3(int);
    void fini(int) {
        printf(":%d:%s():%d\n",m_my_pe,__func__,__LINE__);
        m_returnCallback( 0 );
    }
    HadesSHMEM& m_api;
    Hermes::Shmem::Callback m_returnCallback;
    Hermes::Vaddr   m_pSync;

    Hermes::Value   m_value;
    Hermes::Value   m_retval;
    Hermes::Value   m_one;
    Hermes::Value   m_zero;

    int m_iteration;
    bool m_debug;

    int m_full_tree_parent;
    int m_tree_radix;
    int m_tree_crossover;
    int m_full_tree_num_children;
    std::vector<int> m_full_tree_children;
    std::vector<int> m_part_tree_children;


    int m_parent;
    int m_num_children;
    int*  m_children;
    int m_my_pe;
    int m_num_pes;
};

}
}

#endif

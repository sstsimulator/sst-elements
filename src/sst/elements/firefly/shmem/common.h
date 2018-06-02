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


#ifndef COMPONENTS_FIREFLY_SHMEM_COMMON_H
#define COMPONENTS_FIREFLY_SHMEM_COMMON_H

#include <vector>
#include "sst/elements/hermes/shmemapi.h"

#if 1
#undef printf
#define printf(x,...)
#endif

namespace SST {
namespace Firefly {

class HadesSHMEM;

class ShmemCommon {
  public:
    ShmemCommon( int my_pe, int num_pes, int requested_crossover, int requested_radix );
    void build_kary_tree(int radix, int PE_start, int stride, int PE_size, int PE_root, int *parent,
                                                                      int *num_children, int *children);
    int num_pes() { return m_num_pes; }
    int my_pe() { return m_my_pe; }
    int full_tree_parent() { return m_full_tree_parent; }
    int full_tree_num_children() { return m_full_tree_num_children; }
    int tree_radix() { return m_tree_radix; }
    std::vector<int>& full_tree_children() { return m_full_tree_children; }
    int circular_iter_next(int curr, int PE_start, int logPE_stride, int PE_size);

  private:
    bool m_debug;
    int m_my_pe;
    int m_num_pes;
    int m_full_tree_parent;
    int m_tree_radix;
    int m_tree_crossover;
    int m_full_tree_num_children;
    std::vector<int> m_full_tree_children;
};

class ShmemCollective {
  public:
    ShmemCollective( HadesSHMEM& api, ShmemCommon& common ) : m_api(api), m_common( common ),
        m_value( Hermes::Value::Long), m_one((long)1), m_zero((long)0)
    {}

    int num_pes() { return m_common.num_pes(); }
    int my_pe() { return m_common.my_pe(); }
    int tree_radix() { return m_common.tree_radix(); }
    int full_tree_parent() { return m_common.full_tree_parent(); }
    int full_tree_num_children() { return m_common.full_tree_num_children(); }
    std::vector<int>& full_tree_children() { return m_common.full_tree_children(); }

    typedef long pSync_t;

  protected:

#if 0
    void fini(int) {
        printf(":%d:%s():%d\n",my_pe(),__func__,__LINE__);
        m_returnCallback( 0 );
    }    
#endif

    std::string m_prefix;
    const char* prefix() { return m_prefix.c_str(); }
    HadesSHMEM&     m_api;
    ShmemCommon&    m_common;

    int     m_parent;
    int     m_num_children;
    int*    m_children;

    std::vector<int> m_part_tree_children;

    Hermes::Vaddr   m_pSync;
    Hermes::Value   m_value;
    Hermes::Value   m_one;
    Hermes::Value   m_zero;

    Hermes::Shmem::Callback m_returnCallback;    
};

}
}

#endif


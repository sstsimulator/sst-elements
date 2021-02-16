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


#ifndef COMPONENTS_FIREFLY_SHMEM_ALLTOALLS_H
#define COMPONENTS_FIREFLY_SHMEM_ALLTOALLS_H

#include "shmem/common.h"

namespace SST {
namespace Firefly {

class HadesSHMEM;

class ShmemAlltoalls : public ShmemCollective {
  public:
    ShmemAlltoalls( HadesSHMEM& api, ShmemCommon& common ) : ShmemCollective(api, common )
    { }
    void start( Hermes::Vaddr dest, Hermes::Vaddr source, int dst, int sst, size_t nelems, int elsize,
        int PE_start, int logPE_stride, int PE_size, Hermes::Vaddr pSync, Hermes::Shmem::Callback );
  private:

    void do_peer_0(int);
    void do_peer_1(int);
    void put(int);
    void barrier( int);
    void fini(int);

    bool done() { return true; }
    int stride() { return 1 << m_logPE_stride; }
    int my_as_rank() { return ( m_common.my_pe() - m_PE_start ) / stride();  }

    Hermes::Vaddr m_dest_base;
    Hermes::Vaddr m_src_base;
    Hermes::Vaddr m_src;
    Hermes::Vaddr m_dest;

    int m_dst;
    int m_sst;
    size_t m_nelems;
    int m_elem_size;

    int m_logPE_stride;
    int m_PE_size;
    int m_PE_start;

    int m_iteration;
    int m_start_pe;
    int m_peer;
};

}
}

#endif

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


#ifndef COMPONENTS_FIREFLY_SHMEM_ALLTOALL_H
#define COMPONENTS_FIREFLY_SHMEM_ALLTOALL_H

#include "shmem/common.h"

namespace SST {
namespace Firefly {

class HadesSHMEM;

class ShmemAlltoall : public ShmemCollective {
  public:
    ShmemAlltoall( HadesSHMEM& api, ShmemCommon& common ) : ShmemCollective(api, common )
    { }
    void start( Hermes::Vaddr dest, Hermes::Vaddr source, size_t nelems, int PE_start,
        int logPE_stride, int PE_size, Hermes::Vaddr pSync, Hermes::Shmem::Callback );
  private:

    void put(int);
    void barrier( int);
    void fini(int);

    Hermes::Vaddr m_dest;
    Hermes::Vaddr m_src;

    size_t m_nelems;
    int m_logPE_stride;
    int m_PE_size;
    int m_PE_start;

    int m_start_pe;
    int m_peer;
};

}
}

#endif

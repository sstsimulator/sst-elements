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


#ifndef COMPONENTS_FIREFLY_SHMEM_COLLECT_H
#define COMPONENTS_FIREFLY_SHMEM_COLLECT_H

#include "shmem/common.h"

namespace SST {
namespace Firefly {

class HadesSHMEM;

class ShmemCollect : public ShmemCollective {
  public:
    ShmemCollect( HadesSHMEM& api, ShmemCommon& common ) : 
       ShmemCollective(api, common )
    { }
    void start( Hermes::Vaddr dest, Hermes::Vaddr source, size_t nelems, int PE_start, 
        int logPE_stride, int PE_size, Hermes::Vaddr pSync, Hermes::MemAddr* scratch, 
        Hermes::Shmem::Callback );
  private:

    void do_offset_0(int);
    void do_data_0(int);
    void do_data_1(int);
    void do_data_2(int);
    void fini(int);
    int stride() { return 1 << m_logPE_stride; }

    Hermes::Vaddr m_dest;
    Hermes::Vaddr m_src;
    Hermes::MemAddr* m_scratch;

    size_t  m_my_offset;
    size_t  m_nelems;
    int     m_logPE_stride;
    int     m_PE_size;
    int     m_PE_start;
    int     m_start_pe;
    int     m_peer;
};

}
}

#endif

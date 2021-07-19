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


#ifndef COMPONENTS_FIREFLY_SHMEM_FCOLLECT_H
#define COMPONENTS_FIREFLY_SHMEM_FCOLLECT_H

#include "shmem/common.h"

namespace SST {
namespace Firefly {

class HadesSHMEM;

class ShmemFcollect : public ShmemCollective {
  public:
    ShmemFcollect( HadesSHMEM& api, ShmemCommon& common ) :
       ShmemCollective(api, common )
    { }
    void start( Hermes::Vaddr dest, Hermes::Vaddr source, size_t nelems, int PE_start,
        int logPE_stride, int PE_size, Hermes::Vaddr pSync,
        Hermes::Shmem::Callback );
  private:

    int stride() { return 1 << m_logPE_stride; }
    int my_id() { return (m_common.my_pe() - m_PE_start) / stride(); }
    int next_proc() { return m_PE_start + ((my_id() + 1) % m_PE_size) * stride(); }
    void state_0(int);
    void state_1(int);
    void state_2(int);
    void state_3(int);
    void state_4(int);
    void state_5(int);

    int m_iteration;
    Hermes::Vaddr m_dest;
    Hermes::Vaddr m_src;

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

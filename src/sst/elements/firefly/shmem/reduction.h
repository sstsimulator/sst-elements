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


#ifndef COMPONENTS_FIREFLY_SHMEM_REDUCTION_H
#define COMPONENTS_FIREFLY_SHMEM_REDUCTION_H

#include "shmem/common.h"

namespace SST {
namespace Firefly {

class HadesSHMEM;

class ShmemReduction : public ShmemCollective {
  public:
    ShmemReduction( HadesSHMEM& api, ShmemCommon& common ) : ShmemCollective(api, common )
    { }
    void start( Hermes::Vaddr dest, Hermes::Vaddr source, size_t nelems, int PE_start, 
        int logPE_stride, int PE_size, Hermes::Vaddr pSync, 
        Hermes::Shmem::ReduOp op, Hermes::Value::Type dataType, Hermes::Shmem::Callback );
  private:

//    void fini( int x ) { ShmemCollective::fini( x ); }
    void have_children_0(int);
    void have_children_1(int);
    void have_children_2(int);
    void have_children_3(int);
    void have_children_4(int);
    void have_children_5(int);
    void not_root_0(int);
    void not_root_1(int);
    void not_root_2(int);
    void not_root_3(int);
    void not_root_4(int);
    void not_root_5(int);
    void bcast(int);

    int m_PE_start;
    int m_PE_stride;
    int m_PE_size;

    Hermes::Value::Type m_dataType;
    Hermes::Shmem::ReduOp m_op;
    Hermes::Vaddr m_dest;
    Hermes::Vaddr m_src;
    size_t m_nelems;
    int m_iteration;
};

}
}

#endif

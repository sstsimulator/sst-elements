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


#ifndef COMPONENTS_FIREFLY_SHMEM_BROADCAST_H
#define COMPONENTS_FIREFLY_SHMEM_BROADCAST_H

#include "shmem/common.h"

namespace SST {
namespace Firefly {

class HadesSHMEM;

class ShmemBroadcast : public ShmemCollective {
  public:
    ShmemBroadcast( HadesSHMEM& api, ShmemCommon& common ) : ShmemCollective(api, common )
    { }
    void start( Hermes::Vaddr dest, Hermes::Vaddr source, size_t nelems, int root, int PE_start, 
        int logPE_stride, int PE_size, Hermes::Vaddr pSync, Hermes::Shmem::Callback, bool complete );
  private:

//    void fini( int x ) { ShmemCollective::fini( x ); }
    void node_0(int);
    void node_1(int);
    void node_2(int);
    void node_3(int);
    void node_4(int);
    void node_5(int);
    void node_6(int);
    void node_7(int);
    void leaf_0(int);
    void leaf_1(int);
    void leaf_2(int);
    void leaf_3(int);

    Hermes::Vaddr m_dest;
    Hermes::Vaddr m_src;
    size_t m_nelems;
    int m_iteration;
    bool m_complete;
};

}
}

#endif

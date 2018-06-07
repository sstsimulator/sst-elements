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


#ifndef COMPONENTS_FIREFLY_SHMEM_BARRIER_H
#define COMPONENTS_FIREFLY_SHMEM_BARRIER_H

#include "shmem/common.h"

namespace SST {
namespace Firefly {

class HadesSHMEM;

class ShmemBarrier : protected ShmemCollective {
  public:
    ShmemBarrier( HadesSHMEM& api, ShmemCommon& common ) : ShmemCollective( api, common )
    { 
		m_prefix = "@t:" + std::to_string(common.my_pe()) + ":ShmemBarrier::@p():@l ";
	}
    void start( int PE_start, int logPE_stride, int PE_size, Hermes::Vaddr pSync, Hermes::Shmem::Callback );
  private:

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

    int     m_iteration;
};

}
}

#endif

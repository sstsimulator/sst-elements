// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_SHMEM_BROADCAST
#define _H_EMBER_SHMEM_BROADCAST

#include <strings.h>
#include "shmem/emberShmemGen.h"

namespace SST {
namespace Ember {

class EmberShmemBroadcastGenerator : public EmberShmemGenerator {

public:
	EmberShmemBroadcastGenerator(SST::Component* owner, Params& params) :
		EmberShmemGenerator(owner, params, "ShmemBroadcast" ), m_phase(0) 
	{ 
        m_root = 0;
    }

    bool generate( std::queue<EmberEvent*>& evQ) 
	{
        bool ret = false;
        switch ( m_phase ) {
          case 0: 
            enQ_init( evQ );
            enQ_my_pe( evQ, &m_my_pe );
            enQ_n_pes( evQ, &m_num_pes );
            enQ_malloc( evQ, &m_memory, 24 );
            break;
          case 1:
            m_pSync = m_memory;
            m_src = m_memory.offset<long>(1);
            m_dest = m_memory.offset<long>(2);
            if ( m_root == m_my_pe ) {
                m_src.at<long>(0) = 0xdeadbeef12345678;
            } else {
                m_src.at<long>(0) = 0xf00d1234dead4567;
            }
            m_pSync.at<long>(0) = 0;
            m_dest.at<long>(0) = 0;
            enQ_barrier_all( evQ );
            break;

          case 2:
            enQ_broadcast64( evQ, m_dest, m_src, 1, m_root, 0, 0, m_num_pes, m_pSync );
            break;

          case 3:
            printf("%d:%s: %#lx\n",m_my_pe, getMotifName().c_str(), m_dest.at<long>(0));
            ret = true;

        }
        ++m_phase;

        return ret;
	}

  private:
    Hermes::MemAddr m_memory;
    Hermes::MemAddr m_pSync;
    Hermes::MemAddr m_src;
    Hermes::MemAddr m_dest;
    int m_root;
    int m_my_pe;
    int m_num_pes;
    int m_phase;
};

}
}

#endif

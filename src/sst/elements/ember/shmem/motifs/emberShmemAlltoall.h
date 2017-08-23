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


#ifndef _H_EMBER_SHMEM_ALLTOALL
#define _H_EMBER_SHMEM_ALLTOALL

#include <strings.h>
#include "shmem/emberShmemGen.h"

namespace SST {
namespace Ember {

class EmberShmemAlltoallGenerator : public EmberShmemGenerator {

public:
	EmberShmemAlltoallGenerator(SST::Component* owner, Params& params) :
		EmberShmemGenerator(owner, params, "ShmemAlltoall" ), m_phase(0) 
	{ }

    bool generate( std::queue<EmberEvent*>& evQ) 
	{
        bool ret = false;
        switch ( m_phase ) {
          case 0: 
            enQ_init( evQ );
            enQ_my_pe( evQ, &m_my_pe );
            enQ_n_pes( evQ, &m_num_pes );
            break;

          case 1:
#if 0
            printf("%d:%s: num_pes=%d\n",m_my_pe, getMotifName().c_str(), m_num_pes);
#endif
            // need 3 longs for psync
            enQ_malloc( evQ, &m_memory, 24 + 8 * m_num_pes * 2 );
            break;

          case 2:
            m_pSync = m_memory;
            m_pSync.at<long>(0) = 0;

            m_src = m_memory.offset<long>(3);
            for ( int i = 0; i < m_num_pes; i++ ) {
                m_src.at<long>(i) = ((long) (m_my_pe + 1) << 32) | i;
            }

            m_dest = m_src.offset<long>(m_num_pes);
            bzero( &m_dest.at<long>(0), sizeof(long) * m_num_pes);

#if 0
            printf("%d:%s: pSync=%#" PRIx64 " src=%#" PRIx64 " dest=%#" PRIx64 "\n",m_my_pe, getMotifName().c_str(), 
                    m_pSync.getSimVAddr(), m_src.getSimVAddr(), m_dest.getSimVAddr() );
#endif
            enQ_barrier_all( evQ );
            break;

          case 3:
#if 0
            printf("%d:%s: do alltoall\n",m_my_pe, getMotifName().c_str());
#endif
            enQ_alltoall64( evQ, m_dest, m_src, 1, 0, 0, m_num_pes, m_pSync );
            break;

          case 4:
            for ( int i = 0; i < m_num_pes; i++ ) {
                printf("%d:%s: %#lx\n",m_my_pe, getMotifName().c_str(), m_dest.at<long>(i));
            }
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
    int m_my_pe;
    int m_num_pes;
    int m_phase;
};

}
}

#endif

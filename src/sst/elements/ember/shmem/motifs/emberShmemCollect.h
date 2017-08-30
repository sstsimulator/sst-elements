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


#ifndef _H_EMBER_SHMEM_COLLECT
#define _H_EMBER_SHMEM_COLLECT

#include <strings.h>
#include "shmem/emberShmemGen.h"

namespace SST {
namespace Ember {

class EmberShmemCollectGenerator : public EmberShmemGenerator {

public:
	EmberShmemCollectGenerator(SST::Component* owner, Params& params) :
		EmberShmemGenerator(owner, params, "ShmemCollect" ), m_phase(0) 
	{ 
        m_nelems = params.find<int>("arg.nelems", 1 );
    }

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
            { 
                size_t buffer_size = 3 * sizeof(long);                   // for pSync
                buffer_size += m_nelems * sizeof(uint64_t);              // for source  
                buffer_size += m_nelems * sizeof(uint64_t) * m_num_pes;  // for dest
                enQ_malloc( evQ, &m_memory, buffer_size );
            }
            break;

          case 2:
            m_pSync = m_memory;
            m_pSync.at<long>(0) = 0;
            m_pSync.at<long>(1) = 0;
            m_pSync.at<long>(2) = 0;

            m_src = m_pSync.offset<long>(3);

            for ( int i = 0; i < m_nelems; i++ ) { 
                m_src.at<uint64_t>( i ) = ((uint64_t) (m_my_pe + 1) << 32) | i + 1;
            }

            m_dest = m_src.offset<uint64_t>( m_nelems );
            printf("%d:%s: buffer=%#" PRIx64 " src=%#" PRIx64 " dest=%#" PRIx64 "\n",m_my_pe, 
                        getMotifName().c_str(), m_memory.getSimVAddr(), 
                        m_src.getSimVAddr(), m_dest.getSimVAddr());
            bzero( &m_dest.at<uint64_t>(0), sizeof(uint64_t) * m_num_pes * m_nelems);

            enQ_barrier_all( evQ );
            break;

          case 3:
            enQ_collect64( evQ, m_dest, m_src, m_nelems, 0, 0, m_num_pes, m_pSync );
            break;

          case 4:
            for ( int pe = 0; pe < m_num_pes; pe++ ) {
                for ( int i = 0; i < m_nelems; i++ ) {
                    printf("%d:%s: pe=%d i=%d %#" PRIx64 "\n",m_my_pe, getMotifName().c_str(), 
                            pe, i, m_dest.at<uint64_t>( pe * m_nelems + i));
                    assert( m_dest.at<uint64_t>( pe * m_nelems + i) == ( ((uint64_t) (pe + 1) << 32) | i + 1  )  );
                }
            }
            printf("%d:%s exit\n",m_my_pe, getMotifName().c_str());
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
    int m_nelems;
    int m_my_pe;
    int m_num_pes;
    int m_phase;
};

}
}

#endif

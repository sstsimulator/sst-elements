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


#ifndef _H_EMBER_SHMEM_BARRIER
#define _H_EMBER_SHMEM_BARRIER

#include <strings.h>
#include "shmem/emberShmemGen.h"

namespace SST {
namespace Ember {

class EmberShmemBarrierGenerator : public EmberShmemGenerator {

public:
	EmberShmemBarrierGenerator(SST::Component* owner, Params& params) :
		EmberShmemGenerator(owner, params, "ShmemBarrier" ), m_phase(-2) 
	{ 
        m_count = (uint32_t) params.find("arg.iterations", 1);
    }

    bool generate( std::queue<EmberEvent*>& evQ) 
	{
        if ( m_phase == -2 ) {
            enQ_init( evQ );
            enQ_my_pe( evQ, &m_my_pe );
            enQ_malloc( evQ, &m_addr, 8 );
        } else if ( m_phase == -1 ) {
            m_addr.at<long>(0) = 0;
            enQ_barrier_all( evQ );
        } else {
            if ( m_my_pe == 0 ) {
                printf("%d:%s: m_phase=%d\n",m_my_pe,getMotifName().c_str(),m_phase);
            }

            if ( m_my_pe % 2 == 1 ) {
                enQ_barrier( evQ, 1, 1, 4, m_addr );
            }
        }
        ++m_phase;

        return m_phase == m_count;
	}

  private:
    Hermes::MemAddr m_addr;
    int m_my_pe;
    int m_phase;
    int m_count;
};

}
}

#endif

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


#ifndef _H_EMBER_SHMEM_BARRIER_ALL
#define _H_EMBER_SHMEM_BARRIER_ALL

#include <strings.h>
#include "shmem/emberShmemGen.h"

namespace SST {
namespace Ember {

class EmberShmemBarrierAllGenerator : public EmberShmemGenerator {

public:
	EmberShmemBarrierAllGenerator(SST::Component* owner, Params& params) :
		EmberShmemGenerator(owner, params, "ShmemBarrierAll" ), m_phase(-1) 
	{ 
        m_count = (uint32_t) params.find("arg.iterations", 1);
    }

    bool generate( std::queue<EmberEvent*>& evQ) 
	{
        if ( m_phase == -1 ) {
            enQ_init( evQ );
            enQ_my_pe( evQ, &m_my_pe );
        } else {
            if ( m_my_pe == 0 ) {
                printf("%d:%s: m_phase=%d\n",m_my_pe,getMotifName().c_str(),m_phase);
            }
            enQ_barrier_all( evQ );
        }
        ++m_phase;

        return m_phase == m_count;
	}

  private:
    int m_my_pe;
    int m_phase;
    int m_count;
};

}
}

#endif

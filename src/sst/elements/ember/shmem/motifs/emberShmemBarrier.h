// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
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
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberShmemBarrierGenerator,
        "ember",
        "ShmemBarrierMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM barrier",
        "SST::Ember::EmberGenerator"
    )

    SST_ELI_DOCUMENT_PARAMS(
    )

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
			enQ_n_pes( evQ, &m_num_pes );
            enQ_malloc( evQ, &m_addr, sizeof(long) );
        } else if ( m_phase == -1 ) {
            if ( 1 == m_my_pe ) {
                printf("%d:%s: m_count=%d\n",m_my_pe,getMotifName().c_str(),m_count);
            }
            m_addr.at<long>(0) = 0;
            enQ_barrier_all( evQ );
        } else {

            if ( m_my_pe % 2 == 1 ) {
                enQ_barrier( evQ, 1, 1, m_num_pes/2, m_addr );
            }
        }
        ++m_phase;
        if ( m_phase == m_count && 1 == m_my_pe ) {
            printf("%d:%s: exit\n",m_my_pe, getMotifName().c_str());
        }

        return m_phase == m_count;
	}

  private:
    Hermes::MemAddr m_addr;
	int m_num_pes;
    int m_my_pe;
    int m_phase;
    int m_count;
};

}
}

#endif

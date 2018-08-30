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


#ifndef _H_EMBER_SHMEM_BARRIER_ALL
#define _H_EMBER_SHMEM_BARRIER_ALL

#include <strings.h>
#include "shmem/emberShmemGen.h"

namespace SST {
namespace Ember {

class EmberShmemBarrierAllGenerator : public EmberShmemGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberShmemBarrierAllGenerator,
        "ember",
        "ShmemBarrierAllMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM barrier_all",
        "SST::Ember::EmberGenerator"
    )

    SST_ELI_DOCUMENT_PARAMS(
    )

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
            enQ_n_pes( evQ, &m_num_pes );
			enQ_getTime( evQ, &m_startTime );
        } else if ( m_phase < m_count ) {
            if ( m_phase==0 && m_my_pe == 0 ) {
                printf("%d:%s: m_count=%d\n",m_my_pe,getMotifName().c_str(),m_count);
            }
            enQ_barrier_all( evQ );
        	if ( m_phase + 1 == m_count ) {
				enQ_getTime( evQ, &m_stopTime );
			}
        } else {
			if ( 0 == m_my_pe ) {
				double totalTime = (double)(m_stopTime - m_startTime)/1000000000.0;
            	double latency = totalTime/m_count;
            	printf("%d:%s: iterations=%d time-per=%lf us\n",m_my_pe, 
							getMotifName().c_str(), m_count, latency * 1000000.0 );
			}
        }
		++m_phase;

        return m_phase == m_count + 1;
	}

  private:
	uint64_t m_startTime;
	uint64_t m_stopTime;
    int m_my_pe;
	int m_num_pes;
    int m_phase;
    int m_count;
};

}
}

#endif

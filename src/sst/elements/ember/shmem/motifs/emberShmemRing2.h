// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_SHMEM_RING2
#define _H_EMBER_SHMEM_RING2

#include <strings.h>
#include "shmem/emberShmemGen.h"
#include <cxxabi.h>

namespace SST {
namespace Ember {

template< class TYPE >
class EmberShmemRing2Generator : public EmberShmemGenerator {

public:
	EmberShmemRing2Generator(SST::ComponentId_t id, Params& params) :
		EmberShmemGenerator(id, params, "ShmemRing2" ), m_phase(-2)
	{
		m_iterations = (uint32_t) params.find("arg.iterations", 1);
		m_putv = params.find<bool>("arg.putv", true);
	}

    bool generate( std::queue<EmberEvent*>& evQ)
	{
        if ( -2 == m_phase ) {
            enQ_init( evQ );
            enQ_n_pes( evQ, &m_num_pes );
            enQ_my_pe( evQ, &m_my_pe );
            enQ_malloc( evQ, &m_dest, sizeof(TYPE) * 2 );
		} else if ( -1 == m_phase ) {

            if ( 0 == m_my_pe ) {
                printf("%d:%s: num_pes=%d \n",m_my_pe, getMotifName().c_str(), m_num_pes);
            }

            m_src = m_dest.offset<TYPE>(1);
            m_dest.at<TYPE>(0) = 0;
            enQ_barrier_all( evQ );
			enQ_getTime( evQ, &m_startTime );

		} else if ( m_phase < m_iterations ) {

            m_src.at<TYPE>(0) = m_phase+1;
			if ( 0 == m_my_pe ) {
                if ( m_putv ) {
				    enQ_putv( evQ, m_dest, m_phase+1, (m_my_pe + 1) % m_num_pes );
                } else {
				    enQ_put( evQ, m_dest, m_src, sizeof(TYPE), (m_my_pe + 1) % m_num_pes );
                }
				enQ_wait_until( evQ, m_dest, Hermes::Shmem::GT, m_phase );

			} else {
				enQ_wait_until( evQ, m_dest, Hermes::Shmem::GT, m_phase );
                if ( m_putv ) {
				    enQ_putv( evQ, m_dest, m_phase+1, (m_my_pe + 1) % m_num_pes );
                } else {
				    enQ_put( evQ, m_dest, m_src, sizeof(TYPE), (m_my_pe + 1) % m_num_pes );
                }
			}
            if ( m_phase + 1 == m_iterations ) {
                enQ_getTime( evQ, &m_stopTime );
            	enQ_barrier_all( evQ );
            }

		} else {
            if ( 0 == m_my_pe ) {
                double totalTime = (double)(m_stopTime - m_startTime)/1000000000.0;
                double latency = ((totalTime/m_iterations)/m_num_pes);
                printf("%d:%s: message-size %d, iterations %d, total-time %.3lf us, time-per %.3lf us, %.3f GB/s\n",m_my_pe,
                            getMotifName().c_str(),
							(int)sizeof(TYPE),
							m_iterations,
							totalTime * 1000000.0,
							latency * 1000000.0,
                            (sizeof(TYPE) / latency )/1000000000.0);
            }

        }
        ++m_phase;

		return m_phase == m_iterations + 1;
	}
  private:

    bool m_putv;
   	uint64_t m_startTime;
    uint64_t m_stopTime;
    Hermes::MemAddr m_dest;
    Hermes::MemAddr m_src;
	int m_iterations;
    int m_phase;
    int m_my_pe;
    int m_num_pes;
};

class EmberShmemRing2IntGenerator : public EmberShmemRing2Generator<int> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberShmemRing2IntGenerator,
        "ember",
        "ShmemRing2IntMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM ring2 int",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
		{"arg.iterations","Sets the number of iterations","1"},
        {"arg.putv","Sets what operation to use true=putv false =put","true"},
	)

public:
    EmberShmemRing2IntGenerator( SST::ComponentId_t id, Params& params ) :
        EmberShmemRing2Generator(id,  params) { }
};

}
}

#endif

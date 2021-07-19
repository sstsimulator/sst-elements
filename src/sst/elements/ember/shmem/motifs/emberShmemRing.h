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


#ifndef _H_EMBER_SHMEM_RING
#define _H_EMBER_SHMEM_RING

#include <strings.h>
#include "shmem/emberShmemGen.h"
#include <cxxabi.h>

namespace SST {
namespace Ember {

template< class TYPE >
class EmberShmemRingGenerator : public EmberShmemGenerator {

public:
	EmberShmemRingGenerator(SST::ComponentId_t id, Params& params) :
		EmberShmemGenerator(id, params, "ShmemRing" ), m_phase(-2)
	{
		m_iterations = (uint32_t) params.find("arg.iterations", 1);
		m_count = (uint32_t) params.find("arg.count", 1) - 1;
	}

    bool generate( std::queue<EmberEvent*>& evQ)
	{
        if ( -2 == m_phase ) {
            enQ_init( evQ );
            enQ_n_pes( evQ, &m_num_pes );
            enQ_my_pe( evQ, &m_my_pe );
            enQ_malloc( evQ, &m_src, sizeof(TYPE) * ( m_count * 2 + 1 ) );
		} else if ( -1 == m_phase ) {

            if ( 0 == m_my_pe ) {
                printf("%d:%s: num_pes=%d \n",m_my_pe, getMotifName().c_str(), m_num_pes);
            }

			m_dest = m_src.offset<TYPE>(m_count);
			m_flag = m_dest.offset<TYPE>(m_count);
			m_flag.at<TYPE>(0) = 0;

            enQ_barrier_all( evQ );
			enQ_getTime( evQ, &m_startTime );

		} else if ( m_phase < m_iterations ) {

			if ( 0 == m_my_pe ) {
            	enQ_put( evQ, m_dest, m_src, m_count * sizeof(TYPE), (m_my_pe + 1) % m_num_pes );
				enQ_putv( evQ, m_flag, m_phase+1, (m_my_pe + 1) % m_num_pes );
				enQ_wait_until( evQ, m_flag, Hermes::Shmem::EQ, m_phase + 1 );

			} else {
				enQ_wait_until( evQ, m_flag, Hermes::Shmem::EQ, m_phase + 1 );
            	enQ_put( evQ, m_dest, m_src, m_count * sizeof(TYPE), (m_my_pe + 1) % m_num_pes );
				enQ_putv( evQ, m_flag, m_phase+1, (m_my_pe + 1) % m_num_pes );
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
							(int)(m_count * sizeof(TYPE)),
							m_iterations,
							totalTime * 1000000.0,
							latency * 1000000.0,
                            (m_count*sizeof(TYPE) / latency )/1000000000.0);
            }

        }
        ++m_phase;

		return m_phase == m_iterations + 1;
	}
  private:

   	uint64_t m_startTime;
    uint64_t m_stopTime;
    Hermes::MemAddr m_src;
    Hermes::MemAddr m_dest;
    Hermes::MemAddr m_flag;
//	TYPE m_value;
	TYPE m_waitValue;
	int m_count;
	int m_iterations;
    int m_phase;
    int m_my_pe;
    int m_num_pes;
};
class EmberShmemRingIntGenerator : public EmberShmemRingGenerator<int> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberShmemRingIntGenerator,
        "ember",
        "ShmemRingIntMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM ring2 int",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
		{"arg.iterations","Sets the number of iterations","1"},
		{"arg.count","Sets the number of data items","1"},
	)

public:
    EmberShmemRingIntGenerator( SST::ComponentId_t id, Params& params ) :
        EmberShmemRingGenerator(id,  params) { }
};

}
}

#endif

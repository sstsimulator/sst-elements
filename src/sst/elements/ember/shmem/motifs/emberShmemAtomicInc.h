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


#ifndef _H_EMBER_SHMEM_ATOMIC_INC
#define _H_EMBER_SHMEM_ATOMIC_INC

#include <strings.h>
#include "shmem/emberShmemGen.h"
#include "rng/xorshift.h"
#include <cxxabi.h>

namespace SST {
namespace Ember {

class EmberShmemAtomicIncGenerator : public EmberShmemGenerator {

public:
	EmberShmemAtomicIncGenerator(SST::Component* owner, Params& params) :
		EmberShmemGenerator(owner, params, "ShmemAtomicInc" ), m_phase(-3), m_one(1)
	{ 
        m_dataSize = params.find<int>("arg.dataSize", 32*1024*1024 );
		m_updates = params.find<int>("arg.updates", 4096);
		m_iterations = params.find<int>("arg.iterations", 1);
		m_useFadd = params.find<bool>("arg.useFadd", false);
		m_printTotals = params.find<bool>("arg.printTotals", false);
		m_backed = params.find<bool>("arg.backed", false);
		m_outLoop = params.find<int>("arg.outLoop", 1);
		m_ranksPerNode = params.find<int>("arg.ranksPerNode", 0);
		m_times.resize(m_outLoop);
        assert( m_ranksPerNode );
        rng = new SST::RNG::XORShiftRNG(); 
	}

    bool generate( std::queue<EmberEvent*>& evQ) 
	{
        bool ret = false;
		if ( -3 == m_phase ) {
            enQ_init( evQ );
            enQ_n_pes( evQ, &m_num_pes );
            enQ_my_pe( evQ, &m_my_pe );
            enQ_malloc( evQ, &m_dest, sizeof(long) * m_dataSize, m_backed );
		} else if ( -2 == m_phase ) {

            m_myNode = m_my_pe/m_ranksPerNode;

            if ( 0 == m_my_pe ) {
                printf("%d:%s: myNode=%d num_pes=%d dataSize=%d updates=%d iterations=%d outerLoop=%d %s\n",m_my_pe,
                        getMotifName().c_str(), m_myNode, m_num_pes, m_dataSize, m_updates, m_iterations, m_outLoop,
						m_useFadd ? "fadd":"add" );
            }
            
			if ( m_backed ) {
				bzero( &m_dest.at<long>(0), sizeof(long) * m_dataSize);
			}

            enQ_barrier_all( evQ );

		} else if ( -1 == m_phase ) {

    		struct timeval start;
    		gettimeofday( &start, NULL );
#if 0
			m_randSeed = (unsigned int) (start.tv_usec);
#else
			rng->seed( (unsigned int) (start.tv_usec));
#endif

			enQ_getTime( evQ, &m_startTime );

		} else if ( m_phase < m_iterations * m_updates ) {

#if 0
			int dest = rand_r(&m_randSeed) % m_num_pes;
#else
			int dest = rng->generateNextUInt32()  % m_num_pes;
#endif

            while( dest / m_ranksPerNode == m_myNode ) {
			//while( dest == m_my_pe ) {
#if 0
				dest = rand_r(&m_randSeed) % m_num_pes;
#else
			    dest = rng->generateNextUInt32()  % m_num_pes;
#endif
			}
#if 0
			Hermes::MemAddr addr = m_dest.offset<long>( rand_r(&m_randSeed) % m_dataSize );
#else
			Hermes::MemAddr addr = m_dest.offset<long>( rng->generateNextUInt32() % m_dataSize );
#endif
	
			if ( m_useFadd ) { 
            	enQ_fadd( evQ, &m_result, addr, &m_one, dest );
			} else {
            	enQ_add( evQ, addr, &m_one, dest );
			}
            if ( m_phase + 1 == m_iterations * m_updates ) {
                enQ_getTime( evQ, &m_stopTime );
			}

		} else if ( m_phase == m_iterations * m_updates ) {

			if ( 0 == m_my_pe ) {
				//printf("outerLoop done %d\n",m_outLoop);
				printf(".");
                fflush(stdout);
			}
			--m_outLoop;

            m_times[m_outLoop] = (double)(m_stopTime - m_startTime) * 1.0e-9;;
			if ( m_outLoop > 0 ) {
				m_phase = -1;
            } else {
				++m_phase;
			}
			generate( evQ );

		} else {

		    ret = true;

            if ( 0 == m_my_pe ) {
				printf("\n");
				double updateTotal = ( (double) m_iterations * (double) m_updates * (double) m_num_pes );
				double Gupdates = updateTotal * 1.0e-9;
				double maxTime = 0;
				for ( int i = 0; i < m_times.size(); i++ ) {
					if ( m_times[i] > maxTime ) {
						maxTime = m_times[i];
					}
				} 
				double minTime = maxTime;

				for ( int i = 0; i < m_times.size(); i++ ) {
					if ( m_times[i] < minTime ) {
						minTime = m_times[i];
					}
				} 

                printf("%s: GUpdates  = %.9lf\n", getMotifName().c_str(), Gupdates ); 
                printf("%s: Min Time      = %.9lf\n", getMotifName().c_str(), minTime );
                printf("%s: Max Time      = %.9lf\n", getMotifName().c_str(), maxTime );
                printf("%s: Min GUP/s     = %.9lf\n", getMotifName().c_str(), Gupdates / maxTime);
                printf("%s: Max GUP/s     = %.9lf\n", getMotifName().c_str(), Gupdates / minTime );

            }

			if ( m_backed && m_printTotals ) {
				long mytotal = 0;
				for ( int i = 0; i < m_dataSize; ++i ) {
					mytotal +=  m_dest.at<long>(i) ;
				}
            	printf("%s: PE: %d total is: %lld\n", getMotifName().c_str(), m_my_pe, mytotal );
			}
			
        }
        ++m_phase;
        return ret;
	}
  private:
    int m_myNode;
    int m_ranksPerNode;
	int m_outLoop;
	std::vector<double> m_times;
#if 0
    unsigned int m_randSeed;
#else
    RNG::XORShiftRNG* rng;
#endif
	bool m_backed;
	bool m_printTotals;
	bool m_useFadd;
	long m_one;
	int m_dataSize;
	int m_updates;
	int m_iterations;
    uint64_t m_startTime;
    uint64_t m_stopTime;
	Hermes::MemAddr m_dest;
	std::string  m_type_name;
	long m_result;
    int m_phase;
    int m_my_pe;
    int m_num_pes;
};

}
}

#endif

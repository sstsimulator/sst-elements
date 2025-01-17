// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_SHMEM_FAM_CSWAP_INC
#define _H_EMBER_SHMEM_FAM_CSWAP_INC

#include <strings.h>
#include "shmem/emberShmemGen.h"
#include <cxxabi.h>

#define USE_SST_RNG 1
#ifdef USE_SST_RNG
#include "rng/xorshift.h"
#endif
#include "libs/misc.h"

namespace SST {
namespace Ember {


template < class TYPE, int VAL >
class EmberShmemFAM_CswapBaseGenerator : public EmberShmemGenerator {

public:
	EmberShmemFAM_CswapBaseGenerator(SST::ComponentId_t id, Params& params, std::string name) :
		EmberShmemGenerator(id, params, name ), m_phase(-3), m_numFamNodes(0), m_oldValue(0), m_newValue(1)
	{
        m_computeTime = params.find<int>("arg.computeTime", 0 );
		m_totalBytes = (uint64_t) params.find<SST::UnitAlgebra>("arg.totalBytes").getRoundedValue();

		m_updates = params.find<int>("arg.updates", 4096);
		m_iterations = params.find<int>("arg.iterations", 1);
		m_hotMult = params.find<int>("arg.hotMult", 1);

		m_printTotals = params.find<bool>("arg.printTotals", false);
		m_outLoop = params.find<int>("arg.outLoop", 1);
		m_times.resize(m_outLoop);

        m_numFamNodes = (int) params.find<int>("arg.numFamNodes",0);

#if USE_SST_RNG
        m_rng = new SST::RNG::XORShiftRNG();
#endif
	}


    bool generate( std::queue<EmberEvent*>& evQ)
	{
        bool ret = false;
		if ( -3 == m_phase ) {
            enQ_fam_initialize( evQ, m_groupName );
            enQ_n_pes( evQ, &m_num_pes );
            enQ_my_pe( evQ, &m_my_pe );
            m_miscLib->getNodeNum( evQ, &m_node_num );

		} else if ( -2 == m_phase ) {

			setVerbosePrefix( m_my_pe );
            if ( 0 == m_my_pe ) {
                printf("motif: %s\n", getMotifName().c_str() );
                printf("\tnum_pes: %d\n", m_num_pes );
                printf("\tnode_num: %d\n", m_node_num );
                printf("\ttotal Bytes: %" PRIu64 "\n", m_totalBytes );
                printf("\tupdates: %d\n", m_updates );
                printf("\titerations: %d\n", m_iterations );
                printf("\touterLoop: %d\n", m_outLoop );
                printf("\thotMult: %d\n", m_hotMult );
                printf("\tnumFamNodes: %d\n", m_numFamNodes );
                printf("\tcomputeTime: %d\n", m_computeTime );
            }
			++m_phase;
			initRngSeed( getSeed() + m_my_pe );
			enQ_getTime( evQ, &m_startTime );

		} else if ( -1 == m_phase ) {

			initRngSeed( getSeed() );
			enQ_getTime( evQ, &m_startTime );

		} else if ( m_phase < m_iterations * m_updates ) {

			uint64_t offset = genAddr();

			if ( m_computeTime ) {
            	enQ_compute( evQ, m_computeTime );
			}

            enQ_fam_compare_swap( evQ, &m_result, m_fd, offset, &m_oldValue, &m_newValue );

            if ( m_phase + 1 == m_iterations * m_updates ) {
                enQ_barrier_all( evQ );
                enQ_getTime( evQ, &m_stopTime );
			}

		} else if ( m_phase == m_iterations * m_updates ) {

            --m_outLoop;
            m_times[m_outLoop] = (double)(m_stopTime - m_startTime) * 1.0e-9;;
			if ( 0 == m_my_pe ) {
				printf("outerLoop done %d %f\n",m_outLoop,
                        ((double) m_iterations * (double) m_updates * (double) m_num_pes * 1.0e-9 ) /m_times[m_outLoop] );
			}

			if ( m_outLoop > 0 ) {
				m_phase = -1;
            } else {
				++m_phase;
			}
			generate( evQ );

		} else {

		    ret = true;

            if ( 0 == m_my_pe ) {
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

                printf("%s:GUpdates  = %.9lf\n", getMotifName().c_str(), Gupdates );
                printf("%s:MinTime      = %.9lf\n", getMotifName().c_str(), minTime );
                printf("%s:MaxTime      = %.9lf\n", getMotifName().c_str(), maxTime );
                printf("%s:MinGUP/s     = %.9lf\n", getMotifName().c_str(), Gupdates / maxTime);
                printf("%s:MaxGUP/s     = %.9lf\n", getMotifName().c_str(), Gupdates / minTime );

            }
        }
        ++m_phase;
        return ret;
	}
  private:
    unsigned int getSeed() {
        struct timeval start;
        gettimeofday( &start, NULL );
        return start.tv_usec;
    }

 protected:

	uint64_t genAddr() {
		uint64_t ret = 0;
		if ( m_hotMult > 0 ) {
			assert( m_numFamNodes );
			int node = genRand() % ( m_numFamNodes + m_hotMult );

			if ( node >= m_numFamNodes ) {
				node = m_numFamNodes - 1;
			}

			// get a offset in a FAMs address space
			ret = genRand() % ( (m_totalBytes / m_numFamNodes ) / sizeof(TYPE) );
			ret *= sizeof(TYPE);
			if ( ret >= m_totalBytes / m_numFamNodes ) {
				printf("%" PRIu64 " %" PRIx64 "\n",ret, m_totalBytes / m_numFamNodes);
				exit( 0);
			}

			// add the global offset to the local offset
			ret += node * (m_totalBytes / m_numFamNodes);

		} else {
			ret = genRand() % ( m_totalBytes / sizeof(TYPE) );
			ret *= sizeof(TYPE);
		}
		return ret;
	}

    uint64_t genRand() {
        uint64_t retval;
#if USE_SST_RNG
        retval = ((uint64_t) m_rng->generateNextUInt32()) << 32 | m_rng->generateNextUInt32();
#else
        retval = ((uint64_t) rand_r(&m_randSeed)) << 32 | rand_r(&m_randSeed);
#endif
        return retval;
    }
    void initRngSeed( unsigned int seed ) {
#if USE_SST_RNG
        m_rng->seed( seed );
#else
	    m_randSeed = seed;
#endif
    }

	int m_numFamNodes;
	uint64_t m_totalBytes;
	std::string m_groupName;

	int m_outLoop;
	std::vector<double> m_times;

#if USE_SST_RNG
    SST::RNG::XORShiftRNG* m_rng;
#else
    unsigned int m_randSeed;
#endif

	Shmem::Fam_Descriptor m_fd;
    int m_computeTime;
	bool m_printTotals;
	TYPE m_result;
	TYPE m_oldValue;
	TYPE m_newValue;
	int m_updates;
	int m_iterations;
    uint64_t m_startTime;
    uint64_t m_stopTime;
    int m_phase;
    int m_my_pe;
    int m_num_pes;
    int m_node_num;
    int m_hotMult;
};

template < class TYPE, int VAL >
class EmberShmemFAM_CswapGenerator : public EmberShmemFAM_CswapBaseGenerator<TYPE,VAL> {
public:

    EmberShmemFAM_CswapGenerator(SST::ComponentId_t id, Params& params, std::string name ) :
	    EmberShmemFAM_CswapBaseGenerator<TYPE,VAL>(id, params, name) { }
};

template < class TYPE >
class EmberShmemFAM_CswapGenerator<TYPE,1> : public EmberShmemFAM_CswapBaseGenerator<TYPE,1> {
public:

	EmberShmemFAM_CswapGenerator(SST::ComponentId_t id, Params& params, std::string name ) :
	    EmberShmemFAM_CswapBaseGenerator<TYPE,1>(id, params, name) { }
};

template < class TYPE >
class EmberShmemFAM_CswapGenerator<TYPE,2> : public EmberShmemFAM_CswapBaseGenerator<TYPE,2> {
public:

	EmberShmemFAM_CswapGenerator(SST::ComponentId_t id, Params& params, std::string name ) :
	    EmberShmemFAM_CswapBaseGenerator<TYPE,2>(id, params, name ) { }
};

#define ELI_params \
 { "arg.computeTime","Sets the compute time between SHMEM ops", "0" },\
 { "arg.totalBytes","Sets the total size of the FAM memory space", "" },\
 { "arg.updates","Sets the number of updates", "4096" },\
 { "arg.iterations","Sets the number of iterations", "1" },\
 { "arg.hotMult","Sets the multiplier for the hot node", "1" },\
 { "arg.printTotals","Sets if totals should be printed", "false" },\
 { "arg.outLoop","Sets the number of outer loops", "1" },\
 { "arg.numFamNodes","Sets the number of FAM nodes", "0" },\

class EmberShmemFAM_CswapIntGenerator : public EmberShmemFAM_CswapGenerator<int, 0> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberShmemFAM_CswapIntGenerator,
        "ember",
        "ShmemFAM_CswapIntMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM atomic inc int",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
		ELI_params
	)
public:
	EmberShmemFAM_CswapIntGenerator(SST::ComponentId_t id, Params& params) :
	    EmberShmemFAM_CswapGenerator(id, params, "ShmemFAM_CswapInt" ) { }
};

class EmberShmemFAM_CswapLongGenerator : public EmberShmemFAM_CswapGenerator<long, 0 > {
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberShmemFAM_CswapLongGenerator,
        "ember",
        "ShmemFAM_CswapLongMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM atomic inc long",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
		ELI_params
    )
public:
	EmberShmemFAM_CswapLongGenerator(SST::ComponentId_t id, Params& params) :
	    EmberShmemFAM_CswapGenerator(id, params, "ShmemFAM_CswapLong") {}
};

#undef ELI_params

}
}

#endif

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


#ifndef _H_EMBER_SHMEM_FAM_ATOMIC_INC
#define _H_EMBER_SHMEM_FAM_ATOMIC_INC

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
class EmberShmemFAM_AtomicIncBaseGenerator : public EmberShmemGenerator {

public:
	EmberShmemFAM_AtomicIncBaseGenerator(SST::Component* owner, Params& params, std::string name) :
		EmberShmemGenerator(owner, params, name ), m_phase(-3), m_one(1)
	{ 
        m_computeTime = params.find<int>("arg.computeTime", 50 );
		m_dataSize = (size_t) params.find<SST::UnitAlgebra>("arg.totalBytes").getRoundedValue() / sizeof(TYPE);
		
		m_updates = params.find<int>("arg.updates", 4096);
		m_iterations = params.find<int>("arg.iterations", 1);
		m_hotMult = params.find<int>("arg.hotMult", 1);
		m_pageSize = params.find<int>("arg.pageSize", 4096);
        
		m_numPages = m_dataSize/m_pageSize;
		m_printTotals = params.find<bool>("arg.printTotals", false);
		m_outLoop = params.find<int>("arg.outLoop", 1);
		m_times.resize(m_outLoop);
        
        m_miscLib = static_cast<EmberMiscLib*>(getLib("HadesMisc"));
        assert(m_miscLib);
#if USE_SST_RNG
        m_rng = new SST::RNG::XORShiftRNG();
#endif
	}

	size_t m_dataSize;
	std::string m_groupName;

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
                printf("\ttotal Bytes: %zu\n", m_dataSize * sizeof(TYPE) );
                printf("\tupdates: %d\n", m_updates );
                printf("\titerations: %d\n", m_iterations );
                printf("\touterLoop: %d\n", m_outLoop );
                printf("\thotMult: %d\n", m_hotMult );
            }
			++m_phase;
			initRngSeed( getSeed() );
			enQ_getTime( evQ, &m_startTime );

		} else if ( -1 == m_phase ) {

			initRngSeed( getSeed() );
			enQ_getTime( evQ, &m_startTime );

		} else if ( m_phase < m_iterations * m_updates ) {

			size_t offset = genAddr();

            enQ_compute( evQ, m_computeTime );
	
            enQ_fam_add( evQ, offset, &m_one );

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
		if ( m_hotMult > 0 ) {
			uint64_t page = genRand() % (m_numPages + m_hotMult);
			if ( page >= m_numPages) {
				page = m_numPages - 1;
			}
			return page * m_pageSize + ( genRand( ) & (m_pageSize - 1) );
		} else {
			return genRand() % m_dataSize;
		}
	}

    uint32_t genRand() {
        uint32_t retval;
#if USE_SST_RNG 
        retval = m_rng->generateNextUInt32();
#else
        retval = rand_r(&m_randSeed);
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

    EmberMiscLib* m_miscLib;
	int m_outLoop;
	std::vector<double> m_times;

#if USE_SST_RNG 
    SST::RNG::XORShiftRNG* m_rng;
#else
    unsigned int m_randSeed;
#endif

	uint64_t m_numPages;
	int m_pageSize;
    int m_computeTime;
	bool m_printTotals;
	TYPE m_one;
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
class EmberShmemFAM_AtomicIncGenerator : public EmberShmemFAM_AtomicIncBaseGenerator<TYPE,VAL> {
public:
    EmberShmemFAM_AtomicIncGenerator(SST::Component* owner, Params& params, std::string name ) :
	    EmberShmemFAM_AtomicIncBaseGenerator<TYPE,VAL>(owner, params, name) {
        } 
};

template < class TYPE >
class EmberShmemFAM_AtomicIncGenerator<TYPE,1> : public EmberShmemFAM_AtomicIncBaseGenerator<TYPE,1> {
public:
	EmberShmemFAM_AtomicIncGenerator(SST::Component* owner, Params& params, std::string name ) :
	    EmberShmemFAM_AtomicIncBaseGenerator<TYPE,1>(owner, params, name) {
        } 
};

template < class TYPE >
class EmberShmemFAM_AtomicIncGenerator<TYPE,2> : public EmberShmemFAM_AtomicIncBaseGenerator<TYPE,2> {
public:
	EmberShmemFAM_AtomicIncGenerator(SST::Component* owner, Params& params, std::string name ) :
	    EmberShmemFAM_AtomicIncBaseGenerator<TYPE,2>(owner, params, name ) {
        } 
};

class EmberShmemFAM_AtomicIncIntGenerator : public EmberShmemFAM_AtomicIncGenerator<int, 0> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberShmemFAM_AtomicIncIntGenerator,
        "ember",
        "ShmemFAM_AtomicIncIntMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM atomic inc int",
        "SST::Ember::EmberGenerator"
    )

    SST_ELI_DOCUMENT_PARAMS(
    )
public:
	EmberShmemFAM_AtomicIncIntGenerator(SST::Component* owner, Params& params) :
	    EmberShmemFAM_AtomicIncGenerator(owner, params, "ShmemFAM_AtomicIncInt" ) { } 
};

class EmberShmemFAM_AtomicIncLongGenerator : public EmberShmemFAM_AtomicIncGenerator<long, 0 > {
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberShmemFAM_AtomicIncLongGenerator,
        "ember",
        "ShmemFAM_AtomicIncLongMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM atomic inc long",
        "SST::Ember::EmberGenerator"
    )

    SST_ELI_DOCUMENT_PARAMS(
    )
public:
	EmberShmemFAM_AtomicIncLongGenerator(SST::Component* owner, Params& params) :
	    EmberShmemFAM_AtomicIncGenerator(owner, params, "ShmemFAM_AtomicIncLong") {} 
};

}
}

#endif

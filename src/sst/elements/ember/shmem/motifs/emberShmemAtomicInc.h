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


#ifndef _H_EMBER_SHMEM_ATOMIC_INC
#define _H_EMBER_SHMEM_ATOMIC_INC

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
class EmberShmemAtomicIncBaseGenerator : public EmberShmemGenerator {

    enum { Add, Fadd, Putv, Getv } m_op;
    std::string m_opStr;
public:
	EmberShmemAtomicIncBaseGenerator(SST::ComponentId_t id, Params& params, std::string name) :
		EmberShmemGenerator(id, params, name ), m_phase(-3), m_one(1)
	{
        m_computeTime = params.find<int>("arg.computeTime", 50 );
        m_dataSize = params.find<int>("arg.dataSize", 32*1024*1024 );
		m_updates = params.find<int>("arg.updates", 4096);
		m_iterations = params.find<int>("arg.iterations", 1);
		m_opStr = params.find<std::string>("arg.op", "add");
		m_hotMult = params.find<int>("arg.hotMult", 4);
        if ( m_opStr.compare("add") == 0 ) {
            m_op = Add;
        } else if ( m_opStr.compare("fadd") == 0 ) {
            m_op = Fadd;
        } else if ( m_opStr.compare("putv") == 0 ) {
            m_op = Putv;
        } else if ( m_opStr.compare("getv") == 0 ) {
            m_op = Getv;
        } else {
            assert(0);
        }

		m_printTotals = params.find<bool>("arg.printTotals", false);
		m_backed = params.find<bool>("arg.backed", false);
		m_outLoop = params.find<int>("arg.outLoop", 1);
		m_num_nodes = params.find<int>("arg.numNodes", -1);
		m_randAddr = params.find<int>("arg.randAddr", 1);
		m_times.resize(m_outLoop);

#if USE_SST_RNG
        m_rng = new SST::RNG::XORShiftRNG();
#endif
	}

    bool generate( std::queue<EmberEvent*>& evQ)
	{
        bool ret = false;
		if ( -3 == m_phase ) {
            enQ_init( evQ );
            enQ_n_pes( evQ, &m_num_pes );
            enQ_my_pe( evQ, &m_my_pe );
            enQ_malloc( evQ, &m_dest, sizeof(TYPE) * m_dataSize, m_backed );
            m_miscLib->getNodeNum( evQ, &m_node_num );
            if ( -1 == m_num_nodes ) {
                m_miscLib->getNumNodes( evQ, &m_num_nodes );
            }
		} else if ( -2 == m_phase ) {

            if ( 0 == m_my_pe ) {
                printf("motif: %s\n", getMotifName().c_str() );
                printf("\tnum_pes: %d\n", m_num_pes );
                printf("\tnum_nodes: %d\n", m_num_nodes );
                printf("\tnode_num: %d\n", m_node_num );
                printf("\tdataSize: %d\n", m_dataSize );
                printf("\tupdates: %d\n", m_updates );
                printf("\titerations: %d\n", m_iterations );
                printf("\touterLoop: %d\n", m_outLoop );
                printf("\toperation: %s\n", m_opStr.c_str() );
            }

			if ( m_backed ) {
				bzero( &m_dest.at<TYPE>(0), sizeof(TYPE) * m_dataSize);
			}

            enQ_barrier_all( evQ );

		} else if ( -1 == m_phase ) {

			initRngSeed( getSeed() );
			enQ_getTime( evQ, &m_startTime );

		} else if ( m_phase < m_iterations * m_updates ) {

            int dest = calcDestPe();

			Hermes::MemAddr addr;
            if ( m_randAddr ) {
			    addr = m_dest.offset<TYPE>( genRand() % m_dataSize );
            } else {
                addr = m_dest.offset<TYPE>( 0 );
            }
            enQ_compute( evQ, m_computeTime );

			switch ( m_op ) {
              case Fadd:
            	enQ_fadd( evQ, &m_result, addr, &m_one, dest );
                break;
              case Add:
                enQ_add( evQ, addr, &m_one, dest );
                break;
              case Putv:
                enQ_putv( evQ, addr, &m_one, dest );
                break;
              case Getv:
                enQ_getv( evQ, &m_one, addr, dest );
                break;
			}
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
			    if ( m_backed ) {
				    bzero( &m_dest.at<TYPE>(0), sizeof(TYPE) * m_dataSize);
			    }
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

			if ( m_backed && m_printTotals ) {
				uint32_t mytotal = 0;
				for ( int i = 0; i < m_dataSize; ++i ) {
					mytotal +=  m_dest.at<TYPE>(i) ;
				}
            	printf("%s: PE: %d total is: %" PRIu32 "\n", getMotifName().c_str(), m_my_pe, mytotal );
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

    virtual int calcDestPe() {
	    int dest = genRand() % m_num_pes;

		while( dest == m_my_pe ) {
			dest = genRand() % m_num_pes;
		}
        return dest;
    }
 protected:

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

	int m_outLoop;
	std::vector<double> m_times;

#if USE_SST_RNG
    SST::RNG::XORShiftRNG* m_rng;
#else
    unsigned int m_randSeed;
#endif

    int m_computeTime;
    bool m_randAddr;
	bool m_backed;
	bool m_printTotals;
	TYPE m_one;
	int m_dataSize;
	int m_updates;
	int m_iterations;
    uint64_t m_startTime;
    uint64_t m_stopTime;
	Hermes::MemAddr m_dest;
	std::string  m_type_name;
	TYPE m_result;
    int m_phase;
    int m_my_pe;
    int m_num_pes;
    int m_node_num;
    int m_num_nodes;
    int m_hotMult;
};

template < class TYPE, int VAL >
class EmberShmemAtomicIncGenerator : public EmberShmemAtomicIncBaseGenerator<TYPE,VAL> {
public:

    EmberShmemAtomicIncGenerator(SST::ComponentId_t id, Params& params, std::string name ) :
	    EmberShmemAtomicIncBaseGenerator<TYPE,VAL>(id, params, name) { }
};

template < class TYPE >
class EmberShmemAtomicIncGenerator<TYPE,1> : public EmberShmemAtomicIncBaseGenerator<TYPE,1> {
public:

	EmberShmemAtomicIncGenerator(SST::ComponentId_t id, Params& params, std::string name ) :
	    EmberShmemAtomicIncBaseGenerator<TYPE,1>(id, params, name) { }

private:
    int calcDestPe() {
	    int dest = this->genRand() % this->m_num_pes;

		while( calcNode(dest) == this->m_node_num ) {
			dest = this->genRand() % this->m_num_pes;
		}
        return dest;
    }

    int calcNode(int pe ) {
        return pe/(this->m_num_pes/this->m_num_nodes);
    }
};

template < class TYPE >
class EmberShmemAtomicIncGenerator<TYPE,2> : public EmberShmemAtomicIncBaseGenerator<TYPE,2> {
public:

	EmberShmemAtomicIncGenerator(SST::ComponentId_t id, Params& params, std::string name ) :
	    EmberShmemAtomicIncBaseGenerator<TYPE,2>(id, params, name ) { }

private:
    int calcDestPe() {
        int pe = -1;
        if( this->m_my_pe == this->m_num_pes - 1 ) {
            pe = this->genRand() % (this->m_num_pes - 1);
        } else {

            int pecountHS = this->m_num_pes + this->m_hotMult;
            pe = this->genRand() % pecountHS;

            while( pe == this->m_my_pe ) {
                pe = this->genRand() % pecountHS;
            }

            // If we generate a PE higher than we have
            // clamp ourselves to the highest PE
            if( pe >= this->m_num_pes) {
                pe = this->m_num_pes - 1;
            }
        }
        return pe;
    }
};

class EmberShmemAtomicIncIntGenerator : public EmberShmemAtomicIncGenerator<int, 0> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberShmemAtomicIncIntGenerator,
        "ember",
        "ShmemAtomicIncIntMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM atomic inc int",
       SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
        { "arg.computeTime","Sets the compute time between SHMEM operations","50"},
        { "arg.dataSize","Sets the size of the SHMEM data region","33554432"},
        { "arg.updates","Sets the number of SHMEM updates","4096"},
        { "arg.iterations","Sets the number of inner loops","1"},
        { "arg.op","Set the SHMEM op","add"},
        { "arg.hotMult","Sets the Hot spot multiplier","4"},
        { "arg.printTotals","Print out the totals","flase"},
        { "arg.backed","Configures backing of SHMEM memory","false"},
        { "arg.outLoop","Sets the number of outer loops","1"},
        { "arg.numNodes","Sets the number of nodes","-1"},
        { "arg.randAddr","Use a random address for the SHMEM op","1"},
    )
public:
	EmberShmemAtomicIncIntGenerator(SST::ComponentId_t id, Params& params) :
	    EmberShmemAtomicIncGenerator(id, params, "ShmemAtomicIncInt" ) { }
};

class EmberShmemNSAtomicIncIntGenerator : public EmberShmemAtomicIncGenerator<int, 1> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberShmemNSAtomicIncIntGenerator,
        "ember",
        "ShmemNSAtomicIncIntMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM not same nmode atomic inc int",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
    )
public:
	EmberShmemNSAtomicIncIntGenerator(SST::ComponentId_t id, Params& params) :
	    EmberShmemAtomicIncGenerator(id, params, "ShmemNSAtomicIncInt") { }
};

class EmberShmemHotAtomicIncIntGenerator : public EmberShmemAtomicIncGenerator<int, 2> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberShmemHotAtomicIncIntGenerator,
        "ember",
        "ShmemHotAtomicIncIntMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM hot spot atomic inc int",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
    )
public:
	EmberShmemHotAtomicIncIntGenerator(SST::ComponentId_t id, Params& params) :
	    EmberShmemAtomicIncGenerator(id, params, "ShmemHotAtomicIncInt") { }
};

class EmberShmemAtomicIncLongGenerator : public EmberShmemAtomicIncGenerator<long, 0 > {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberShmemAtomicIncLongGenerator,
        "ember",
        "ShmemAtomicIncLongMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM atomic inc long",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
    )
public:
	EmberShmemAtomicIncLongGenerator(SST::ComponentId_t id, Params& params) :
	    EmberShmemAtomicIncGenerator(id, params, "ShmemAtomicIncLong") {}
};

class EmberShmemHotAtomicIncLongGenerator : public EmberShmemAtomicIncGenerator<long, 1 > {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberShmemHotAtomicIncLongGenerator,
        "ember",
        "ShmemHotAtomicIncLongMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM hot spot atomic inc long",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
    )
public:
	EmberShmemHotAtomicIncLongGenerator(SST::ComponentId_t id, Params& params) :
	    EmberShmemAtomicIncGenerator(id, params, "ShmemHotAtomicIncLong") {}
};

}
}

#endif

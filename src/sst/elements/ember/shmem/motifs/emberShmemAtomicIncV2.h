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


#ifndef _H_EMBER_SHMEM_ATOMIC_INC_V2
#define _H_EMBER_SHMEM_ATOMIC_INC_V2

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
class EmberShmemAtomicIncV2BaseGenerator : public EmberShmemGenerator {

    enum { Add, Fadd, Putv, Getv } m_op;
    std::string m_opStr;
public:
	EmberShmemAtomicIncV2BaseGenerator(SST::ComponentId_t id, Params& params, std::string name) :
		EmberShmemGenerator(id, params, name ), m_phase(Init1), m_one(1), m_curUpdate(0), m_loopCnt(0), m_detailedDone(false)
	{
        m_computeTime = params.find<int>("arg.computeTime", 0 );
        m_dataSize = params.find<int>("arg.dataSize", 32*1024*1024 );
		m_updates = params.find<int>("arg.updates", 4096);
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

		m_detailedComputeList = params.find<std::string>("arg.detailedCompute","");
		m_printTotals = params.find<bool>("arg.printTotals", false);
		m_backed = params.find<bool>("arg.backed", false);
		m_times.resize( params.find<int>("arg.outLoop", 1) );
		m_randAddr = params.find<int>("arg.randAddr", 1);
		m_run4ever = params.find<bool>("arg.run4ever", false);

#if USE_SST_RNG
        m_rng = new SST::RNG::XORShiftRNG();
#endif
	}
	//virtual bool primary( ) {
	//	return m_detailed;
	//}

    bool generate( std::queue<EmberEvent*>& evQ)
	{
        bool ret = false;
		switch( m_phase ) {
		case Init1:
			verbose(CALL_INFO, 1,0, "Init1\n");
            enQ_init( evQ );
            enQ_n_pes( evQ, &m_num_pes );
            enQ_my_pe( evQ, &m_my_pe );
            enQ_malloc( evQ, &m_dest, sizeof(TYPE) * m_dataSize, m_backed );
            m_miscLib->getNodeNum( evQ, &m_node_num );
            m_miscLib->getNumNodes( evQ, &m_num_nodes );
			m_phase=Init2;
			break;

		case Init2:
			verbose(CALL_INFO, 1,0, "Init2\n");
			m_detailedCompute = findNid( m_node_num, m_detailedComputeList );
            if ( 0 == m_my_pe ) {
                printf("motif: %s\n", getMotifName().c_str() );
                printf("\tnum_pes: %d\n", m_num_pes );
                printf("\tnum_nodes: %d\n", m_num_nodes );
                printf("\tnode_num: %d\n", m_node_num );
                printf("\tdataSize: %d\n", m_dataSize );
                printf("\tupdates: %d\n", m_updates );
                printf("\touterLoop: %zu\n", m_times.size() );
                printf("\toperation: %s\n", m_opStr.c_str() );
            }

			if ( m_backed ) {
				bzero( &m_dest.at<TYPE>(0), sizeof(TYPE) * m_dataSize);
			}

            enQ_barrier_all( evQ );
			m_phase=InitLoop;
			break;

		case InitLoop:
			verbose(CALL_INFO, 1,0, "InitLoop\n");

			initRngSeed( getSeed() );
			enQ_getTime( evQ, &m_startTime );
			m_phase=Work;
#if 1
			if ( m_detailedCompute ) {
				detailedLocalPE( evQ );
			}
#endif
			break;

		case Work:

			{
				bool done;
#if  0
				if ( m_detailedCompute ) {
					done = detailedLocalPE( evQ );
				} else {
					done = work( evQ );
				}
#else
					done = work( evQ );
#endif

				if ( done ) {
        			enQ_getTime( evQ, &m_stopTime );
					m_phase=FiniLoop;
				}
			}
			break;

		case FiniLoop:

			verbose(CALL_INFO, 1,0, "FiniLoop\n");
       		m_times[m_loopCnt] = (double)(m_stopTime - m_startTime) * 1.0e-9;
			if ( 0 == m_my_pe ) {
				printf("outerLoop done loopCnt=%d %f\n",m_loopCnt,
                        ((double) m_updates * (double) m_num_pes * 1.0e-9 ) /m_times[m_loopCnt] );
			}

			++m_loopCnt;

			if ( m_loopCnt < m_times.size() ) {
			    if ( m_backed ) {
				    bzero( &m_dest.at<TYPE>(0), sizeof(TYPE) * m_dataSize);
			    }
				m_phase = InitLoop;
				generate( evQ );
				break;
            } else {
				m_phase = Fini;
			}

		case Fini:
			verbose(CALL_INFO, 1,0, "Fini node_num=%d pe=%d\n",m_node_num,m_my_pe);

		    ret = true;

            if ( m_detailedCompute ) {
				double updateTotal = ( (double) m_updates * (double) m_num_pes );
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
			break;

        }
        return ret;
	}
  private:

	//bool detailedLocalPE( std::queue<EmberEvent*>& evQ ) {
	void detailedLocalPE( std::queue<EmberEvent*>& evQ ) {
		//printf("%s()\n",__func__);
       	verbose( CALL_INFO, 1, 0, "\n");

       	Params params;

       	std::string motif;

       	std::stringstream tmp;

   		motif = "miranda.GUPSGenerator";

       	tmp.str( std::string() ); tmp.clear();
       	tmp << m_updates;
       	params.insert("count", tmp.str() );

		uint64_t minAddr = m_dest.getSimVAddr();;
       	tmp.str( std::string() ); tmp.clear();
       	tmp << minAddr;
       	params.insert("min_address", tmp.str() );

       	tmp.str( std::string() ); tmp.clear();
       	tmp << minAddr + sizeof(TYPE) * m_dataSize;
       	params.insert("max_address", tmp.str() );

       	params.insert("length", "8",true);

       	params.insert( "generatorParams.verbose", "1" );
       	params.insert( "verbose", "1" );

	    std::function<int()> foo = [=](){
			printf("done foo\n");
			m_detailedDone = true;
            return 0;
        };

       	enQ_detailedCompute( evQ, motif, params, foo );
       	//enQ_detailedCompute( evQ, motif, params );
		//return true;
	}

	bool work( std::queue<EmberEvent*>& evQ ) {
		//printf("%s()\n",__func__);
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

		if ( m_run4ever ) {
			return m_detailedDone;
		}
		++m_curUpdate;
		if ( m_curUpdate == m_updates ) {
			m_curUpdate = 0;
			return true;
		} else {
			return false;
		}
	}

    bool findNid( int nid, std::string nidList ) {

        if ( 0 == nidList.compare( "all" ) ) {
            return true;
        }

        if ( nidList.empty() ) {
            return false;
        }

        size_t pos = 0;
        size_t end = 0;
        do {
            end = nidList.find( ",", pos );
            if ( end == std::string::npos ) {
                end = nidList.length();
            }
            std::string tmp = nidList.substr(pos,end-pos);

            if ( tmp.length() == 1 ) {
                int val = atoi( tmp.c_str() );
                if ( nid == val ) {
                    return true;
                }
            } else {
                size_t dash = tmp.find( "-" );
                int first = atoi(tmp.substr(0,dash).c_str()) ;
                int last = atoi(tmp.substr(dash+1).c_str());
                if ( nid >= first && nid <= last ) {
                    return true;
                }
            }

            pos = end + 1;
        } while ( end < nidList.length() );

        return false;
    }

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

	int m_loopCnt;
	std::vector<double> m_times;

#if USE_SST_RNG
    SST::RNG::XORShiftRNG* m_rng;
#else
    unsigned int m_randSeed;
#endif

	int m_curUpdate;
    bool m_detailedCompute;
    std::string m_detailedComputeList;
    int m_computeTime;
	bool m_detailedDone;
    bool m_randAddr;
	bool m_backed;
	bool m_printTotals;
	bool m_run4ever;
	TYPE m_one;
	int m_dataSize;
	int m_updates;
    uint64_t m_startTime;
    uint64_t m_stopTime;
	Hermes::MemAddr m_dest;
	std::string  m_type_name;
	TYPE m_result;
    enum { Init1, Init2, InitLoop, Work, FiniLoop, Fini } m_phase;
    int m_my_pe;
    int m_num_pes;
    int m_node_num;
    int m_num_nodes;
    int m_hotMult;
};

template < class TYPE, int VAL >
class EmberShmemAtomicIncV2Generator : public EmberShmemAtomicIncV2BaseGenerator<TYPE,VAL> {
public:

    EmberShmemAtomicIncV2Generator(SST::ComponentId_t id, Params& params, std::string name ) :
	    EmberShmemAtomicIncV2BaseGenerator<TYPE,VAL>(id, params, name) { }
};

template < class TYPE >
class EmberShmemAtomicIncV2Generator<TYPE,1> : public EmberShmemAtomicIncV2BaseGenerator<TYPE,1> {
public:

	EmberShmemAtomicIncV2Generator(SST::ComponentId_t id, Params& params, std::string name ) :
	    EmberShmemAtomicIncV2BaseGenerator<TYPE,1>(id, params, name) { }

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
class EmberShmemAtomicIncV2Generator<TYPE,2> : public EmberShmemAtomicIncV2BaseGenerator<TYPE,2> {
public:

	EmberShmemAtomicIncV2Generator(SST::ComponentId_t id, Params& params, std::string name ) :
	    EmberShmemAtomicIncV2BaseGenerator<TYPE,2>(id, params, name ) { }

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


#define ELI_params \
		{ "arg.computeTime","Configure the compute time between SHMEM ops","0"},\
		{ "arg.dataSize","Configure the size of the SHMEM region","33554432"},\
		{ "arg.updates","Configure the number of SHMEM op","4096"},\
		{ "arg.op","Configure the SHMEM op","add"},\
		{ "arg.hotMult","Configure hot spot node multiplier","4"},\
		{ "arg.detailedCompute","Configure list of nodes to use detailed compute model",""},\
		{ "arg.printTotals","Configure to print totals","false"},\
		{ "arg.backed","Configure if SHMEM memory is back","false"},\
		{ "arg.outLoop","Configure the number of outer loops","1"},\
		{ "arg.randAddr","Configure to use random address for SHMEM op","1"},\
		{ "arg.run4ever","Configure the detailed compute model to run forever","false"},\

class EmberShmemAtomicIncV2IntGenerator : public EmberShmemAtomicIncV2Generator<int, 0> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberShmemAtomicIncV2IntGenerator,
        "ember",
        "ShmemAtomicIncV2IntMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM atomic inc int",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
		ELI_params
	)

public:
	EmberShmemAtomicIncV2IntGenerator(SST::ComponentId_t id, Params& params) :
	    EmberShmemAtomicIncV2Generator(id, params, "ShmemAtomicIncV2Int" ) { }
};

class EmberShmemNSAtomicIncV2IntGenerator : public EmberShmemAtomicIncV2Generator<int, 1> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberShmemNSAtomicIncV2IntGenerator,
        "ember",
        "ShmemNSAtomicIncV2IntMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM not same nmode atomic inc int",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
		ELI_params
	)

public:
	EmberShmemNSAtomicIncV2IntGenerator(SST::ComponentId_t id, Params& params) :
	    EmberShmemAtomicIncV2Generator(id, params, "ShmemNSAtomicIncV2Int") { }
};

class EmberShmemHotAtomicIncV2IntGenerator : public EmberShmemAtomicIncV2Generator<int, 2> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberShmemHotAtomicIncV2IntGenerator,
        "ember",
        "ShmemHotAtomicIncV2IntMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM hot spot atomic inc int",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
		ELI_params
	)

public:
	EmberShmemHotAtomicIncV2IntGenerator(SST::ComponentId_t id, Params& params) :
	    EmberShmemAtomicIncV2Generator(id, params, "ShmemHotAtomicIncV2Int") { }
};

class EmberShmemAtomicIncV2LongGenerator : public EmberShmemAtomicIncV2Generator<long, 0 > {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberShmemAtomicIncV2LongGenerator,
        "ember",
        "ShmemAtomicIncV2LongMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM atomic inc long",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
		ELI_params
	)

public:
	EmberShmemAtomicIncV2LongGenerator(SST::ComponentId_t id, Params& params) :
	    EmberShmemAtomicIncV2Generator(id, params, "ShmemAtomicIncV2Long") {}
};

class EmberShmemHotAtomicIncV2LongGenerator : public EmberShmemAtomicIncV2Generator<long, 1 > {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberShmemHotAtomicIncV2LongGenerator,
        "ember",
        "ShmemHotAtomicIncV2LongMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM hot spot atomic inc long",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
		ELI_params
    )
public:
	EmberShmemHotAtomicIncV2LongGenerator(SST::ComponentId_t id, Params& params) :
	    EmberShmemAtomicIncV2Generator(id, params, "ShmemHotAtomicIncV2Long") {}
};

#undef ELI_params

}
}

#endif

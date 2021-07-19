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


#ifndef _H_EMBER_SHMEM_PUTV
#define _H_EMBER_SHMEM_PUTV

#include <strings.h>
#include "shmem/emberShmemGen.h"
#include <cxxabi.h>

namespace SST {
namespace Ember {

template< class TYPE >
class EmberShmemPutvGenerator : public EmberShmemGenerator {

    template<class T>
    typename std::enable_if<std::is_floating_point<T>::value,T>::type
    genSeed( ) {
		return 489173890.047781;
    }

    template<class T>
    typename std::enable_if<std::is_integral<T>::value,T>::type
    genSeed( ) {
		return  (T)0xf00d000deadbee0;
    }

public:
	EmberShmemPutvGenerator(SST::ComponentId_t id, Params& params) :
		EmberShmemGenerator(id, params, "ShmemPutv" ), m_phase(-2)
	{
        m_printResults = params.find<bool>("arg.printResults", false );
		m_iterations = (uint32_t) params.find("arg.iterations", 1);
        int status;
        std::string tname = typeid(TYPE).name();
		char* tmp = abi::__cxa_demangle(tname.c_str(), NULL, NULL, &status);
        m_type_name = tmp;
		free(tmp);
	}

    bool generate( std::queue<EmberEvent*>& evQ)
	{
        bool ret = false;
		if ( -2 == m_phase ) {
            enQ_init( evQ );
            enQ_n_pes( evQ, &m_num_pes );
            enQ_my_pe( evQ, &m_my_pe );
            enQ_malloc( evQ, &m_dest, sizeof(TYPE) );
		} else if ( -1 == m_phase ) {

            if ( 0 == m_my_pe ) {
                printf("%d:%s: num_pes=%d type=\"%s\"\n",m_my_pe,
                        getMotifName().c_str(), m_num_pes, m_type_name.c_str());
                assert( 2 == m_num_pes );
            }

			m_dest.at<TYPE>(0) = 0;
            enQ_barrier_all( evQ );
			enQ_getTime( evQ, &m_startTime );

			m_value = genSeed<TYPE>() + m_my_pe;
		} else if ( m_phase < m_iterations ) {

            enQ_putv( evQ, m_dest, m_value, (m_my_pe + 1) % m_num_pes );

            if ( m_phase + 1 == m_iterations ) {
                enQ_getTime( evQ, &m_stopTime );
                enQ_barrier_all( evQ );
            }

		} else {

			if ( m_printResults ) {
				std::stringstream tmp;
               	tmp << " got="<< m_dest.at<TYPE>(0) << " want=" <<  genSeed<TYPE>() + ((m_my_pe + 1) % 2);

            	printf("%d:%s: PUT %s\n",m_my_pe, getMotifName().c_str(), tmp.str().c_str());
			}
			assert( m_dest.at<TYPE>(0) == genSeed<TYPE>() + ((m_my_pe + 1) % 2) );

		    ret = true;

            if ( 0 == m_my_pe ) {
                double totalTime = (double)(m_stopTime - m_startTime)/1000000000.0;
                double latency = (totalTime/m_iterations);
                printf("%d:%s: iterations %d, total-time %.3lf us, time-per %.3lf us\n",m_my_pe,
                            getMotifName().c_str(),
                            m_iterations,
                            totalTime * 1000000.0,
                            latency * 1000000.0 );
            }

        }
        ++m_phase;
        return ret;
	}
  private:
	int m_iterations;
    uint64_t m_startTime;
    uint64_t m_stopTime;
	bool m_printResults;
	std::string  m_type_name;
    Hermes::MemAddr m_dest;
	TYPE m_value;
    int m_phase;
    int m_my_pe;
    int m_num_pes;
};

#define ELI_params \
	{"arg.printResults","Sets is results are printed","false"},\
	{"arg.iterations","Sets the number of iterations","1"},\

class EmberShmemPutvIntGenerator : public EmberShmemPutvGenerator<int> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberShmemPutvIntGenerator,
        "ember",
        "ShmemPutvIntMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM putv int",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
		ELI_params
	)

public:
    EmberShmemPutvIntGenerator( SST::ComponentId_t id, Params& params ) :
        EmberShmemPutvGenerator(id,  params) { }
};

class EmberShmemPutvLongGenerator : public EmberShmemPutvGenerator<long> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberShmemPutvLongGenerator,
        "ember",
        "ShmemPutvLongMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM putv long",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
		ELI_params
	)

public:
    EmberShmemPutvLongGenerator( SST::ComponentId_t id, Params& params ) :
        EmberShmemPutvGenerator(id,  params) { }
};

class EmberShmemPutvDoubleGenerator : public EmberShmemPutvGenerator<double> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberShmemPutvDoubleGenerator,
        "ember",
        "ShmemPutvDoubleMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM putv double",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
		ELI_params
	)

public:
    EmberShmemPutvDoubleGenerator( SST::ComponentId_t id, Params& params ) :
        EmberShmemPutvGenerator(id,  params) { }
};

class EmberShmemPutvFloatGenerator : public EmberShmemPutvGenerator<float> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberShmemPutvFloatGenerator,
        "ember",
        "ShmemPutvFloatMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM putv float",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
		ELI_params
	)

public:
    EmberShmemPutvFloatGenerator( SST::ComponentId_t id, Params& params ) :
        EmberShmemPutvGenerator(id,  params) { }
};

#undef ELI_params

}
}

#endif

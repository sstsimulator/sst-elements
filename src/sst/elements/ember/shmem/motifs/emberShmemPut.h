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


#ifndef _H_EMBER_SHMEM_PUT
#define _H_EMBER_SHMEM_PUT

#include <strings.h>
#include "shmem/emberShmemGen.h"
#include <cxxabi.h>

namespace SST {
namespace Ember {

template< class TYPE >
class EmberShmemPutGenerator : public EmberShmemGenerator {

public:
	EmberShmemPutGenerator(SST::ComponentId_t id, Params& params) :
		EmberShmemGenerator(id, params, "ShmemPut" ), m_phase(-2)
	{
        m_biDir = params.find<bool>("arg.biDir", 0);
        m_nelems = params.find<int>("arg.nelems", 1);
        m_printResults = params.find<bool>("arg.printResults", false );
		m_iterations = (uint32_t) params.find("arg.iterations", 1);
		m_blocking = (bool) params.find("arg.blocking", true);
        int status;
        std::string tname = typeid(TYPE).name();
		char* tmp = abi::__cxa_demangle(tname.c_str(), NULL, NULL, &status);
        m_type_name = tmp;
		free( tmp );
    }

    bool generate( std::queue<EmberEvent*>& evQ)
	{
        bool ret = false;
		if ( -2 == m_phase ) {
            enQ_init( evQ );
            enQ_n_pes( evQ, &m_num_pes );
            enQ_my_pe( evQ, &m_my_pe );
            enQ_malloc( evQ, &m_src, m_nelems * sizeof(TYPE) * 2);
		} else if ( -1 == m_phase ) {

            if ( 0 == m_my_pe ) {
                printf("%d:%s: num_pes=%d nelems=%d type=\"%s\"\n",m_my_pe,
                        getMotifName().c_str(), m_num_pes, m_nelems, m_type_name.c_str());
            }
			assert( 2 == m_num_pes );

            m_other_pe = (m_my_pe + 1) % m_num_pes;

            m_dest = m_src.offset<TYPE>(m_nelems );

            for ( int i = 0; i < m_nelems; i++ ) {
                m_src.at<TYPE>(i) = m_my_pe + i;
            }

			bzero( &m_dest.at<TYPE>(0), sizeof(TYPE) * m_nelems);
            enQ_barrier_all( evQ );

			enQ_getTime( evQ, &m_startTime );

		} else if ( m_phase < m_iterations ) {

			if ( 0 == m_my_pe || m_biDir ) {
                if ( m_blocking ) {
            	    enQ_put( evQ,
					    m_dest,
					    m_src,
                        m_nelems*sizeof(TYPE),
                        m_other_pe );
                } else {
            	    enQ_put_nbi( evQ,
					    m_dest,
					    m_src,
                        m_nelems*sizeof(TYPE),
                        m_other_pe );
                    enQ_quiet( evQ );
                }
			} else {
                m_phase = m_iterations - 1;
            }

			if (  m_phase + 1 == m_iterations ) {
                enQ_barrier_all( evQ );
                enQ_getTime( evQ, &m_stopTime );
			}

		} else {

            if ( m_biDir ) {
            	for ( int i = 0; i < m_nelems; i++ ) {
					TYPE want = ((m_my_pe + 1) % 2 )  + i;
					if ( m_printResults ) {
                		std::stringstream tmp;
                		tmp << " got="<< m_dest.at<TYPE>(i) << " want=" <<  want;
                		printf("%d:%s: PUT %s\n",m_my_pe, getMotifName().c_str(), tmp.str().c_str());
					}
                	assert( m_dest.at<TYPE>(i) == want );
            	}
			}
		    ret = true;
            if ( 0 == m_my_pe ) {
                double totalTime = (double)(m_stopTime - m_startTime)/1000000000.0;
                double latency = (totalTime/m_iterations);
                printf("%d:%s: message-size %d, iterations %d, total-time %.3lf us, time-per %.3lf us, %.3f GB/s\n",m_my_pe,
                            getMotifName().c_str(),
                            (int) (m_nelems * sizeof(TYPE)),
                            m_iterations,
                            totalTime * 1000000.0,
                            latency * 1000000.0,
                            (m_nelems*sizeof(TYPE) / latency )/1000000000.0 * ( m_biDir ? 2 : 1 )  );
            }
        }
        ++m_phase;
        return ret;
	}
  private:

    bool m_blocking;
	bool m_biDir;
	uint64_t m_startTime;
	uint64_t m_stopTime;
    bool m_printResults;
    std::string m_type_name;
    Hermes::MemAddr m_src;
    Hermes::MemAddr m_dest;
	int m_iterations;
    int m_nelems;
    int* m_from;
    int* m_to;
    int m_phase;
    int m_my_pe;
    int m_other_pe;
    int m_num_pes;
};

#define ELI_params \
	{"arg.biDir","Sets if test is bidirectional","0"},\
	{"arg.nelems","Sets the number of data elements","1"},\
	{"arg.printResults","Sets if results are printed","false"},\
	{"arg.iterations","Sets the number of iterations","1"},\
	{"arg.blocking","Sets if memory is backed","true"},\


class EmberShmemPutIntGenerator : public EmberShmemPutGenerator<int> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberShmemPutIntGenerator,
        "ember",
        "ShmemPutIntMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM put int",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
		ELI_params
	)

public:
    EmberShmemPutIntGenerator( SST::ComponentId_t id, Params& params ) :
        EmberShmemPutGenerator(id,  params) { }
};

class EmberShmemPutLongGenerator : public EmberShmemPutGenerator<long> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberShmemPutLongGenerator,
        "ember",
        "ShmemPutLongMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM put long",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
		ELI_params
	)

public:
    EmberShmemPutLongGenerator( SST::ComponentId_t id, Params& params ) :
        EmberShmemPutGenerator(id,  params) { }
};

class EmberShmemPutDoubleGenerator : public EmberShmemPutGenerator<double> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberShmemPutDoubleGenerator,
        "ember",
        "ShmemPutDoubleMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM put double",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
		ELI_params
	)

public:
    EmberShmemPutDoubleGenerator( SST::ComponentId_t id, Params& params ) :
        EmberShmemPutGenerator(id,  params) { }
};

class EmberShmemPutFloatGenerator : public EmberShmemPutGenerator<float> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberShmemPutFloatGenerator,
        "ember",
        "ShmemPutFloatMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM put float",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
		ELI_params
	)

public:
    EmberShmemPutFloatGenerator( SST::ComponentId_t id, Params& params ) :
        EmberShmemPutGenerator(id,  params) { }
};
#undef ELI_params
}
}

#endif

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


#ifndef _H_EMBER_SHMEM_GETNBI
#define _H_EMBER_SHMEM_GETNBI

#include <strings.h>
#include "shmem/emberShmemGen.h"
#include <cxxabi.h>

namespace SST {
namespace Ember {

template< class TYPE >
class EmberShmemGetNBIGenerator : public EmberShmemGenerator {

public:
	EmberShmemGetNBIGenerator(SST::Component* owner, Params& params) :
		EmberShmemGenerator(owner, params, "ShmemGetNBI" ), m_phase(0)
	{ 
        m_nelems = params.find<int>("arg.nelems", 1);
        m_i = m_count = params.find<int>("arg.count", 1);
        int status;
        std::string tname = typeid(TYPE).name();
		char* tmp = abi::__cxa_demangle(tname.c_str(), NULL, NULL, &status);
        m_type_name = tmp;
		free(tmp);
	}

    bool generate( std::queue<EmberEvent*>& evQ) 
	{
        bool ret = false;
        switch ( m_phase ) {
        case 0:
            enQ_init( evQ );
            enQ_n_pes( evQ, &m_num_pes );
            enQ_my_pe( evQ, &m_my_pe );
            break;

        case 1:

            if ( 0 == m_my_pe ) {
                printf("%d:%s: num_pes=%d nelems=%d type=\"%s\"\n",m_my_pe,
                        getMotifName().c_str(), m_num_pes, m_nelems, m_type_name.c_str());
            }
			assert( 2 == m_num_pes );
            m_other_pe = (m_my_pe + 1) % m_num_pes;
            enQ_malloc( evQ, &m_src, m_nelems * sizeof(TYPE));
            enQ_malloc( evQ, &m_dest, m_nelems * sizeof(TYPE));
            break;

        case 2:

            for ( int i = 0; i < m_nelems; i++ ) {
                m_src.at<TYPE>(i) = m_my_pe + i;
            }

            bzero( &m_dest.at<TYPE>(0), sizeof(TYPE) * m_nelems);

            enQ_barrier_all( evQ );
            enQ_getTime( evQ, &m_startTime );
	    	if ( m_my_pe == 0 ) { 
				++m_phase;
                enQ_barrier_all( evQ );
			}
            break;

        case 3:
            enQ_get_nbi( evQ, m_dest, m_src, m_nelems*sizeof(TYPE), m_other_pe );

            if ( --m_i ) {
        	    --m_phase;
	        } else {
                enQ_quiet( evQ );
                enQ_getTime( evQ, &m_stopTime );
                enQ_barrier_all( evQ );
			}
            break;

        case 4:
            ret = true;
            if ( m_my_pe != 0 ) {
				double time = m_stopTime-m_startTime;
				size_t bytes = m_count * m_nelems * sizeof(TYPE);
                printf("%d:%s: count=%d, %.3lf ns, %zu bytes, %.3lf GB/s \n",m_my_pe, getMotifName().c_str(), 
						m_count,time/(double)m_count, bytes, (double) bytes/ time );
            }
        }
        ++m_phase;
        return ret;
	}
  private:
    bool m_printResults;
    std::string m_type_name;
    uint64_t m_startTime;
    uint64_t m_stopTime;
    Hermes::MemAddr m_src;
    Hermes::MemAddr m_dest;
    int m_count;
    int m_i;
    int m_nelems;
    int m_phase;
    int m_my_pe;
    int m_other_pe;
    int m_num_pes;
};

class EmberShmemGetNBIIntGenerator : public EmberShmemGetNBIGenerator<int> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberShmemGetNBIIntGenerator,
        "ember",
        "ShmemGetNBIIntMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM get int",
        "SST::Ember::EmberGenerator"

    )

    SST_ELI_DOCUMENT_PARAMS(
    )

public:
    EmberShmemGetNBIIntGenerator( SST::Component* owner, Params& params ) :
        EmberShmemGetNBIGenerator(owner,  params) { }
};

class EmberShmemGetNBILongGenerator : public EmberShmemGetNBIGenerator<long> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberShmemGetNBILongGenerator,
        "ember",
        "ShmemGetNBILongMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM get long",
        "SST::Ember::EmberGenerator"

    )

    SST_ELI_DOCUMENT_PARAMS(
    )

public:
    EmberShmemGetNBILongGenerator( SST::Component* owner, Params& params ) :
        EmberShmemGetNBIGenerator(owner,  params) { }
};

class EmberShmemGetNBIDoubleGenerator : public EmberShmemGetNBIGenerator<double> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberShmemGetNBIDoubleGenerator,
        "ember",
        "ShmemGetNBIDoubleMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM get double",
        "SST::Ember::EmberGenerator"

    )

    SST_ELI_DOCUMENT_PARAMS(
    )

public:
    EmberShmemGetNBIDoubleGenerator( SST::Component* owner, Params& params ) :
        EmberShmemGetNBIGenerator(owner,  params) { }
};

class EmberShmemGetNBIFloatGenerator : public EmberShmemGetNBIGenerator<float> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberShmemGetNBIFloatGenerator,
        "ember",
        "ShmemGetNBIFloatMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM get float",
        "SST::Ember::EmberGenerator"

    )

    SST_ELI_DOCUMENT_PARAMS(
    )

public:
    EmberShmemGetNBIFloatGenerator( SST::Component* owner, Params& params ) :
        EmberShmemGetNBIGenerator(owner,  params) { }
};
}
}

#endif

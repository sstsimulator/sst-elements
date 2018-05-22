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


#ifndef _H_EMBER_SHMEM_CSWAP
#define _H_EMBER_SHMEM_CSWAP

#include <strings.h>
#include "shmem/emberShmemGen.h"
#include <cxxabi.h>

namespace SST {
namespace Ember {

template< class TYPE >
class EmberShmemCswapGenerator : public EmberShmemGenerator {

public:
    EmberShmemCswapGenerator(SST::Component* owner, Params& params) :
		EmberShmemGenerator(owner, params, "ShmemCswap" ), m_phase(0) 
	{ 
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
                printf("%d:%s: num_pes=%d type=\"%s\"\n",m_my_pe,
                        getMotifName().c_str(), m_num_pes, m_type_name.c_str());
            }
			assert( 2 == m_num_pes );
            enQ_malloc( evQ, &m_addr, sizeof(TYPE) );
            break;

        case 2:
            
            if ( m_my_pe == 0 ) {
                m_addr.at<TYPE>(0) = 10;
            }

            enQ_barrier_all( evQ );

            if ( m_my_pe == 1 ) {
                m_value = 19; 
				m_cond = 10; 
				enQ_cswap( evQ, &m_result, m_addr, &m_cond, &m_value, 0 );
            }
            enQ_barrier_all( evQ );
            break;

        case 3:
            if ( 0 == m_my_pe ) {
                std::stringstream tmp;
                tmp << " got="<< m_addr.at<TYPE>(0) << " want=" << 19;
                printf("%d:%s: Fadd %s\n",m_my_pe, getMotifName().c_str(), tmp.str().c_str());

                assert( m_addr.at<TYPE>(0) == 19 );
            } else {
                std::stringstream tmp;
                tmp << " got="<< m_result << " want=" << 10;
                printf("%d:%s: Fadd %s\n",m_my_pe, getMotifName().c_str(), tmp.str().c_str());

                assert ( m_result == 10 ); 
            }
		    ret = true;
        }
        ++m_phase;
        return ret;
	}
  private:
	std::string m_type_name;
    Hermes::MemAddr m_addr;
    TYPE m_value;
    TYPE m_result;
    TYPE m_cond;
    int m_phase;
    int m_my_pe;
    int m_num_pes;
};

class EmberShmemCswapIntGenerator : public EmberShmemCswapGenerator<int> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberShmemCswapIntGenerator,
        "ember",
        "ShmemCswapIntMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM cswap int",
        "SST::Ember::EmberGenerator"

    )

    SST_ELI_DOCUMENT_PARAMS(
    )

public:
    EmberShmemCswapIntGenerator( SST::Component* owner, Params& params ) :
        EmberShmemCswapGenerator(owner,  params) { }
};

class EmberShmemCswapLongGenerator : public EmberShmemCswapGenerator<long> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberShmemCswapLongGenerator,
        "ember",
        "ShmemCswapLongMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM cswap long",
        "SST::Ember::EmberGenerator"

    )

    SST_ELI_DOCUMENT_PARAMS(
    )

public:
    EmberShmemCswapLongGenerator( SST::Component* owner, Params& params ) :
        EmberShmemCswapGenerator(owner,  params) { }
};

class EmberShmemCswapDoubleGenerator : public EmberShmemCswapGenerator<double> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberShmemCswapDoubleGenerator,
        "ember",
        "ShmemCswapDoubleMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM cswap double",
        "SST::Ember::EmberGenerator"

    )

    SST_ELI_DOCUMENT_PARAMS(
    )

public:
    EmberShmemCswapDoubleGenerator( SST::Component* owner, Params& params ) :
        EmberShmemCswapGenerator(owner,  params) { }
};

class EmberShmemCswapFloatGenerator : public EmberShmemCswapGenerator<float> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberShmemCswapFloatGenerator,
        "ember",
        "ShmemCswapFloatMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM cswap float",
        "SST::Ember::EmberGenerator"

    )

    SST_ELI_DOCUMENT_PARAMS(
    )

public:
    EmberShmemCswapFloatGenerator( SST::Component* owner, Params& params ) :
        EmberShmemCswapGenerator(owner,  params) { }
};

}
}

#endif

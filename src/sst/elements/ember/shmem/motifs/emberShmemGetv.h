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


#ifndef _H_EMBER_SHMEM_GETV
#define _H_EMBER_SHMEM_GETV

#include <strings.h>
#include "shmem/emberShmemGen.h"
#include <cxxabi.h>

namespace SST {
namespace Ember {

template< class TYPE >
class EmberShmemGetvGenerator : public EmberShmemGenerator {

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
	EmberShmemGetvGenerator(SST::ComponentId_t id, Params& params) :
		EmberShmemGenerator(id, params, "ShmemGetv" ), m_phase(0)
	{
        m_printResults = params.find<bool>("arg.printResults", false );
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
                assert( m_num_pes == 2 );
            }
            enQ_malloc( evQ, &m_src, sizeof(TYPE) );
            break;

        case 2:

			m_src.at<TYPE>( 0 ) = genSeed<TYPE>() + m_my_pe;
            enQ_barrier_all( evQ );

            enQ_getv( evQ, &m_result, m_src, (m_my_pe + 1) % m_num_pes );
            break;

        case 3:
            if ( m_printResults )
			{
                std::stringstream tmp;
                tmp << " got="<< m_result << " want=" <<  genSeed<TYPE>() + ((m_my_pe + 1) % 2);
                printf("%d:%s: PUT %s\n",m_my_pe, getMotifName().c_str(), tmp.str().c_str());
            }
            assert( m_result == genSeed<TYPE>() + ((m_my_pe + 1) % 2) );
		    ret = true;
            if ( 0 == m_my_pe ) {
                printf("%d:%s: exit\n",m_my_pe, getMotifName().c_str());
            }

        }
        ++m_phase;
        return ret;
	}
  private:
    bool m_printResults;
    std::string  m_type_name;
    Hermes::MemAddr m_src;
	TYPE m_result;
    int m_phase;
    int m_my_pe;
    int m_num_pes;
};

class EmberShmemGetvIntGenerator : public EmberShmemGetvGenerator<int> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberShmemGetvIntGenerator,
        "ember",
        "ShmemGetvIntMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM getv int",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS()

public:
    EmberShmemGetvIntGenerator( SST::ComponentId_t id, Params& params ) :
        EmberShmemGetvGenerator(id,  params) { }
};

class EmberShmemGetvLongGenerator : public EmberShmemGetvGenerator<long> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberShmemGetvLongGenerator,
        "ember",
        "ShmemGetvLongMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM getv long",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS()

public:
    EmberShmemGetvLongGenerator( SST::ComponentId_t id, Params& params ) :
        EmberShmemGetvGenerator(id,  params) { }
};

class EmberShmemGetvDoubleGenerator : public EmberShmemGetvGenerator<double> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberShmemGetvDoubleGenerator,
        "ember",
        "ShmemGetvDoubleMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM getv double",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS()

public:
    EmberShmemGetvDoubleGenerator( SST::ComponentId_t id, Params& params ) :
        EmberShmemGetvGenerator(id,  params) { }
};

class EmberShmemGetvFloatGenerator : public EmberShmemGetvGenerator<float> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberShmemGetvFloatGenerator,
        "ember",
        "ShmemGetvFloatMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM getv float",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS()

public:
    EmberShmemGetvFloatGenerator( SST::ComponentId_t id, Params& params ) :
        EmberShmemGetvGenerator(id,  params) { }
};

}
}

#endif

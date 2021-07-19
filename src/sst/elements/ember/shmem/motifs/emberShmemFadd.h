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


#ifndef _H_EMBER_SHMEM_FADD
#define _H_EMBER_SHMEM_FADD

#include <strings.h>
#include "shmem/emberShmemGen.h"
#include <cxxabi.h>

namespace SST {
namespace Ember {

template< class TYPE >
class EmberShmemFaddGenerator : public EmberShmemGenerator {

public:
	EmberShmemFaddGenerator(SST::ComponentId_t id, Params& params) :
		EmberShmemGenerator(id, params, "ShmemFadd" ), m_phase(0)
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
                enQ_fadd( evQ, &m_result, m_addr, &m_value, 0 );
            }
            enQ_barrier_all( evQ );
            break;

        case 3:
            if ( 0 == m_my_pe ) {

                std::stringstream tmp;
                tmp << " got="<< m_addr.at<TYPE>(0) << " want=" << 29;
                printf("%d:%s: Fadd %s\n",m_my_pe, getMotifName().c_str(), tmp.str().c_str());

                assert( m_addr.at<TYPE>(0) == 29 );
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
    std::string  m_type_name;
    Hermes::MemAddr m_addr;
    TYPE m_value;
    TYPE m_result;
    int m_phase;
    int m_my_pe;
    int m_num_pes;
};

class EmberShmemFaddIntGenerator : public EmberShmemFaddGenerator<int> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberShmemFaddIntGenerator,
        "ember",
        "ShmemFaddIntMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM fadd int",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS()

public:
    EmberShmemFaddIntGenerator( SST::ComponentId_t id, Params& params ) :
        EmberShmemFaddGenerator(id,  params) { }
};

class EmberShmemFaddLongGenerator : public EmberShmemFaddGenerator<long> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberShmemFaddLongGenerator,
        "ember",
        "ShmemFaddLongMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM fadd long",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS()

public:
    EmberShmemFaddLongGenerator( SST::ComponentId_t id, Params& params ) :
        EmberShmemFaddGenerator(id,  params) { }
};

class EmberShmemFaddDoubleGenerator : public EmberShmemFaddGenerator<double> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberShmemFaddDoubleGenerator,
        "ember",
        "ShmemFaddDoubleMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM fadd double",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS()

public:
    EmberShmemFaddDoubleGenerator( SST::ComponentId_t id, Params& params ) :
        EmberShmemFaddGenerator(id,  params) { }
};

class EmberShmemFaddFloatGenerator : public EmberShmemFaddGenerator<float> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberShmemFaddFloatGenerator,
        "ember",
        "ShmemFaddFloatMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM fadd float",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS()

public:
    EmberShmemFaddFloatGenerator( SST::ComponentId_t id, Params& params ) :
        EmberShmemFaddGenerator(id,  params) { }
};

}
}

#endif

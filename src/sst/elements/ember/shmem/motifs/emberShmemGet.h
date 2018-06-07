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


#ifndef _H_EMBER_SHMEM_GET
#define _H_EMBER_SHMEM_GET

#include <strings.h>
#include "shmem/emberShmemGen.h"
#include <cxxabi.h>

namespace SST {
namespace Ember {

template< class TYPE >
class EmberShmemGetGenerator : public EmberShmemGenerator {

public:
	EmberShmemGetGenerator(SST::Component* owner, Params& params) :
		EmberShmemGenerator(owner, params, "ShmemGet" ), m_phase(0)
	{ 
        m_nelems = params.find<int>("arg.nelems", 1);
        m_printResults = params.find<bool>("arg.printResults", false );
        m_blocking = params.find<bool>("arg.blocking", true );
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
            enQ_malloc( evQ, &m_src, m_nelems * sizeof(TYPE) * 2);
            break;

        case 2:

            m_dest = m_src.offset<TYPE>(m_nelems );

            for ( int i = 0; i < m_nelems; i++ ) {
                m_src.at<TYPE>(i) = m_my_pe + i;
            }

            bzero( &m_dest.at<TYPE>(0), sizeof(TYPE) * m_nelems);

            enQ_barrier_all( evQ );
            break;

        case 3:
            if ( m_blocking ) {
                enQ_get( evQ, 
                    m_dest,
                    m_src, 
                    m_nelems*sizeof(TYPE),
                    m_other_pe );
                enQ_barrier_all( evQ );
            } else {
                enQ_get_nbi( evQ, 
                    m_dest,
                    m_src, 
                    m_nelems*sizeof(TYPE),
                    m_other_pe );
                enQ_quiet( evQ );
            }
            break;

        case 4: 
            for ( int i = 0; i < m_nelems; i++ ) {
                TYPE want = ((m_my_pe + 1) % 2 )  + i;
                if ( m_printResults ) {
                    std::stringstream tmp;
                    tmp << " got="<< m_dest.at<TYPE>(i) << " want=" <<  want;
                    printf("%d:%s: PUT %s\n",m_my_pe, getMotifName().c_str(), tmp.str().c_str());
                }
                assert( m_dest.at<TYPE>(i) == want );
            }
		    ret = true;
            if ( 0 == m_my_pe ) {
                printf("%d:%s: exit\n",m_my_pe, getMotifName().c_str());
            }
        }
        ++m_phase;
        return ret;
	}
  private:
    bool m_blocking;
    bool m_printResults;
    std::string m_type_name;
    Hermes::MemAddr m_src;
    Hermes::MemAddr m_dest;
    int m_nelems;
    int m_phase;
    int m_my_pe;
    int m_other_pe;
    int m_num_pes;
};

class EmberShmemGetIntGenerator : public EmberShmemGetGenerator<int> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberShmemGetIntGenerator,
        "ember",
        "ShmemGetIntMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM get int",
        "SST::Ember::EmberGenerator"

    )

    SST_ELI_DOCUMENT_PARAMS(
    )

public:
    EmberShmemGetIntGenerator( SST::Component* owner, Params& params ) :
        EmberShmemGetGenerator(owner,  params) { }
};

class EmberShmemGetLongGenerator : public EmberShmemGetGenerator<long> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberShmemGetLongGenerator,
        "ember",
        "ShmemGetLongMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM get long",
        "SST::Ember::EmberGenerator"

    )

    SST_ELI_DOCUMENT_PARAMS(
    )

public:
    EmberShmemGetLongGenerator( SST::Component* owner, Params& params ) :
        EmberShmemGetGenerator(owner,  params) { }
};

class EmberShmemGetDoubleGenerator : public EmberShmemGetGenerator<double> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberShmemGetDoubleGenerator,
        "ember",
        "ShmemGetDoubleMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM get double",
        "SST::Ember::EmberGenerator"

    )

    SST_ELI_DOCUMENT_PARAMS(
    )

public:
    EmberShmemGetDoubleGenerator( SST::Component* owner, Params& params ) :
        EmberShmemGetGenerator(owner,  params) { }
};

class EmberShmemGetFloatGenerator : public EmberShmemGetGenerator<float> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberShmemGetFloatGenerator,
        "ember",
        "ShmemGetFloatMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM get float",
        "SST::Ember::EmberGenerator"

    )

    SST_ELI_DOCUMENT_PARAMS(
    )

public:
    EmberShmemGetFloatGenerator( SST::Component* owner, Params& params ) :
        EmberShmemGetGenerator(owner,  params) { }
};
}
}

#endif

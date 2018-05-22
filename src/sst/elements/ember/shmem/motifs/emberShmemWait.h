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


#ifndef _H_EMBER_SHMEM_WAIT
#define _H_EMBER_SHMEM_WAIT

#include <strings.h>
#include "shmem/emberShmemGen.h"
#include <cxxabi.h>

namespace SST {
namespace Ember {

template <class TYPE>
class EmberShmemWaitGenerator : public EmberShmemGenerator {

public:
	EmberShmemWaitGenerator(SST::Component* owner, Params& params) :
		EmberShmemGenerator(owner, params, "ShmemWait" ), m_phase(0) 
	{ 
        int status;
        std::string tname = typeid(TYPE).name();
		char* tmp = abi::__cxa_demangle(tname.c_str(), NULL, NULL, &status);
        m_type_name = tmp;
		free(tmp); 

        assert( 4 == sizeof(TYPE) || 8 == sizeof(TYPE) );
	}

    bool generate( std::queue<EmberEvent*>& evQ) 
	{
        bool ret = false;
        switch ( m_phase ) {
        case 0:
            enQ_init( evQ );
            enQ_n_pes( evQ, &m_n_pes );
            enQ_my_pe( evQ, &m_my_pe );
            break;

        case 1:

            if ( 0 == m_my_pe ) {
                printf("%d:%s: type=\"%s\"\n",m_my_pe, getMotifName().c_str(), m_type_name.c_str());
            }
			assert( 2 == m_n_pes );
            enQ_malloc( evQ, &m_addr, sizeof(TYPE) );
            break;

        case 2:

			m_addr.at<TYPE>(0) = 0;
            enQ_barrier_all( evQ );
            
            if ( m_my_pe == 0 ) {
                enQ_wait( evQ, m_addr, (TYPE) 0 );
            } else {
                enQ_putv( evQ, m_addr, (TYPE) (0xdead0000 + m_my_pe), 0 );
            }

            break;

        case 3:
            if ( m_my_pe == 0 ) {
                printf("%d:%s: got %#" PRIx64 "\n",m_my_pe, getMotifName().c_str(), (uint64_t) m_addr.at<TYPE>(0));
				assert( m_addr.at<TYPE>(0) == 0xdead0000 + ((  m_my_pe + 1 ) % m_n_pes ) );
            }

		    ret = true;
            break;
        }
        ++m_phase;
        return ret;
	}
  private:
	std::string m_type_name;
    Hermes::MemAddr m_addr;
    int m_phase;
    int m_my_pe;
    int m_n_pes;
};

class EmberShmemWaitIntGenerator : public EmberShmemWaitGenerator<int> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberShmemWaitIntGenerator,
        "ember",
        "ShmemWaitIntMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM wait int",
        "SST::Ember::EmberGenerator"

    )

    SST_ELI_DOCUMENT_PARAMS(
    )

public:
    EmberShmemWaitIntGenerator( SST::Component* owner, Params& params ) :
        EmberShmemWaitGenerator(owner,  params) { }
};

class EmberShmemWaitLongGenerator : public EmberShmemWaitGenerator<long> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberShmemWaitLongGenerator,
        "ember",
        "ShmemWaitLongMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM wait long",
        "SST::Ember::EmberGenerator"

    )

    SST_ELI_DOCUMENT_PARAMS(
    )

public:
    EmberShmemWaitLongGenerator( SST::Component* owner, Params& params ) :
        EmberShmemWaitGenerator(owner,  params) { }
};

}
}

#endif

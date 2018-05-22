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


#ifndef _H_EMBER_SHMEM_WAITUNTIL
#define _H_EMBER_SHMEM_WAITUNTIL

#include <strings.h>
#include "shmem/emberShmemGen.h"
#include <cxxabi.h>

namespace SST {
namespace Ember {

template <class TYPE>
class EmberShmemWaitUntilGenerator : public EmberShmemGenerator {

public:
	EmberShmemWaitUntilGenerator(SST::Component* owner, Params& params) :
		EmberShmemGenerator(owner, params, "ShmemWaitUntil" ), m_phase(0) 
	{ 
        m_opName = params.find<std::string>("arg.op", "NE");

        m_initValue = 0; 
        m_waitValue = m_initValue;
        if ( 0 == m_opName.compare("LTE") ) {
            m_waitValue = -1;
            m_op = Hermes::Shmem::LTE;
            m_putValue = -1;
        } else if ( 0 == m_opName.compare("LT") ) {
            m_op = Hermes::Shmem::LT;
            m_putValue = -1;
        } else if ( 0 == m_opName.compare("EQ") ) {
            m_op = Hermes::Shmem::EQ;
            m_waitValue = 1;
            m_putValue = 1;
        } else if ( 0 == m_opName.compare("NE") ) {
            m_op = Hermes::Shmem::NE;
            m_putValue = 1;
        } else if ( 0 == m_opName.compare("GT") ) {
            m_op = Hermes::Shmem::GT;
            m_putValue = 1;
        } else if ( 0 == m_opName.compare("GTE") ) {
            m_op = Hermes::Shmem::GTE;
            m_waitValue = 1;
            m_putValue = 1;
        } else {
            assert(0);
        }
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
            enQ_barrier_all( evQ );
            break;

        case 2:
            m_addr.at<TYPE>(0) = m_initValue;
            enQ_barrier_all( evQ );
            
            if ( m_my_pe == 0 ) {
                enQ_wait_until( evQ, m_addr, m_op, m_waitValue );
            } else {
                enQ_putv( evQ, m_addr, m_putValue, 0 );
            }

            break;

        case 3:
            if ( m_my_pe == 0 ) {
                std::stringstream tmp;
				tmp << "init=" << m_initValue << " wait=" << m_waitValue << " put=" << m_putValue << " got=" << m_addr.at<TYPE>(0);

                printf("%d:%s: op=%s %s\n",m_my_pe, getMotifName().c_str(),
                        m_opName.c_str(), tmp.str().c_str());
            }
		    ret = true;
            break;
        }
        ++m_phase;
        return ret;
	}
  private:
	std::string m_type_name;
    Hermes::Shmem::WaitOp m_op;
    Hermes::MemAddr       m_addr;
    std::string m_opName;
    TYPE m_initValue;
    TYPE m_putValue;
    TYPE m_waitValue;
    int m_phase;
    int m_my_pe;
    int m_n_pes;
};

class EmberShmemWaitUntilIntGenerator : public EmberShmemWaitUntilGenerator<int> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberShmemWaitUntilIntGenerator,
        "ember",
        "ShmemWaitUntilIntMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM wait_until int",
        "SST::Ember::EmberGenerator"

    )

    SST_ELI_DOCUMENT_PARAMS(
    )

public:
    EmberShmemWaitUntilIntGenerator( SST::Component* owner, Params& params ) :
        EmberShmemWaitUntilGenerator(owner,  params) { }
};

class EmberShmemWaitUntilLongGenerator : public EmberShmemWaitUntilGenerator<long> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberShmemWaitUntilLongGenerator,
        "ember",
        "ShmemWaitUntilLongMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM wait_until long",
        "SST::Ember::EmberGenerator"

    )

    SST_ELI_DOCUMENT_PARAMS(
    )

public:
    EmberShmemWaitUntilLongGenerator( SST::Component* owner, Params& params ) :
        EmberShmemWaitUntilGenerator(owner,  params) { }
};
}
}

#endif

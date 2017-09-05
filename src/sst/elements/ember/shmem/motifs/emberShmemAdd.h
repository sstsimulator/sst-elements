// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_SHMEM_ADD
#define _H_EMBER_SHMEM_ADD

#include <strings.h>
#include "shmem/emberShmemGen.h"
#include <cxxabi.h>

namespace SST {
namespace Ember {

template< class TYPE >
class EmberShmemAddGenerator : public EmberShmemGenerator {

public:
	EmberShmemAddGenerator(SST::Component* owner, Params& params) :
		EmberShmemGenerator(owner, params, "ShmemAdd" ), m_phase(0) 
	{ 
        int status;
        std::string tname = typeid(TYPE).name();
		char* tmp = abi::__cxa_demangle(tname.c_str(), NULL, NULL, &status);
        m_type_name = tmp;

		free( tmp );
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
            
            m_addr.at<TYPE>(0) = 10;

            enQ_barrier_all( evQ );

			m_value = m_my_pe + 11; 
            enQ_add( evQ, m_addr, &m_value, (m_my_pe + 1 ) % 2 );

            enQ_barrier_all( evQ );
            break;

        case 3:
			{
                std::stringstream tmp;
                tmp << " got="<< m_addr.at<TYPE>(0) << " want=" <<  11 + 10 + (m_my_pe + 1 ) % 2;
                printf("%d:%s: Add %s\n",m_my_pe, getMotifName().c_str(), tmp.str().c_str());

                assert( m_addr.at<TYPE>(0) == 11 + 10 + (m_my_pe + 1 ) % 2 );
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
    int m_phase;
    int m_my_pe;
    int m_num_pes;
};

}
}

#endif

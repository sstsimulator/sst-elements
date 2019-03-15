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


#ifndef _H_EMBER_SHMEM_BROADCAST
#define _H_EMBER_SHMEM_BROADCAST

#include <strings.h>
#include "shmem/emberShmemGen.h"
#include <cxxabi.h>

namespace SST {
namespace Ember {

template< class TYPE >
class EmberShmemBroadcastGenerator : public EmberShmemGenerator {

public:
	EmberShmemBroadcastGenerator(SST::Component* owner, Params& params) :
		EmberShmemGenerator(owner, params, "ShmemBroadcast" ), m_phase(0) 
	{ 
		m_printResults = params.find<bool>("arg.printResults", false );
		m_nelems = params.find<int>("arg.nelems", 1 );
		m_root = params.find<int>("arg.root", 0 );
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
            enQ_my_pe( evQ, &m_my_pe );
            enQ_n_pes( evQ, &m_num_pes );
            break;

          case 1:

            if ( 0 == m_my_pe ) {
                printf("%d:%s: num_pes=%d nelems=%d type=\"%s\" root=%d\n",m_my_pe, 
						getMotifName().c_str(), m_num_pes, m_nelems, m_type_name.c_str(),m_root);
				assert( m_root < m_num_pes ); 
            }
            {
                size_t buffer_size = 3 * sizeof(long);    // for pSync
                buffer_size += m_nelems * sizeof(TYPE);   // for source
                buffer_size += m_nelems * sizeof(TYPE);   // for dest
                enQ_malloc( evQ, &m_pSync, buffer_size );
            }
            break;	


		  case 2:
			bzero( &m_pSync.at<long>(0), sizeof(long) * 3);

            m_src = m_pSync.offset<long>(3);

            if ( m_root == m_my_pe ) {
				int shift = (sizeof(TYPE) * 8 )/ 2;
				for ( int i = 0; i < m_nelems; i++ ) {
                	m_src.at<TYPE>(i) = ((TYPE)(m_my_pe + 1) << shift) | i + 1;
				} 
            } 

            m_dest = m_src.offset<TYPE>( m_nelems );
			bzero( &m_dest.at<TYPE>(0), sizeof(TYPE) * m_nelems);

            enQ_barrier_all( evQ );
            break;

          case 3:
			if ( 8 == sizeof(TYPE) ) {
            	enQ_broadcast64( evQ, m_dest, m_src, m_nelems, m_root, 0, 0, m_num_pes, m_pSync );
			} else {
            	enQ_broadcast32( evQ, m_dest, m_src, m_nelems, m_root, 0, 0, m_num_pes, m_pSync );
			}
            break;

          case 4:
			for ( int i = 0; i < m_nelems; i++ ) { 
				int shift = (sizeof(TYPE) * 8 )/ 2;
				if ( m_my_pe != m_root ) {
					if ( m_printResults ) {
            			printf("%d:%s: %#" PRIx64 "\n",m_my_pe, getMotifName().c_str(), (uint64_t) m_dest.at<TYPE>(i));
					}

					assert( (((TYPE)(m_root + 1) << shift) | i + 1 ) == m_dest.at<TYPE>(i) );
				}
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
	bool m_printResults;
	std::string m_type_name;
    Hermes::MemAddr m_pSync;
    Hermes::MemAddr m_src;
    Hermes::MemAddr m_dest;
	int m_nelems;
    int m_root;
    int m_my_pe;
    int m_num_pes;
    int m_phase;
};

class EmberShmemBroadcast32Generator : public EmberShmemBroadcastGenerator<uint32_t> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberShmemBroadcast32Generator,
        "ember",
        "ShmemBroadcast32Motif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM broadcasts32",
        "SST::Ember::EmberGenerator"
    )

    SST_ELI_DOCUMENT_PARAMS(
    )

public:
    EmberShmemBroadcast32Generator(SST::Component* owner, Params& params) :
    EmberShmemBroadcastGenerator( owner, params) {}
};

class EmberShmemBroadcast64Generator : public EmberShmemBroadcastGenerator<uint64_t> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberShmemBroadcast64Generator,
        "ember",
        "ShmemBroadcast64Motif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM broadcasts64",
        "SST::Ember::EmberGenerator"
    )

    SST_ELI_DOCUMENT_PARAMS(
    )

public:
    EmberShmemBroadcast64Generator(SST::Component* owner, Params& params) :
    EmberShmemBroadcastGenerator( owner, params) {}
};
}
}

#endif

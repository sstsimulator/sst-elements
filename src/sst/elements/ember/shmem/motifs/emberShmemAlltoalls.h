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


#ifndef _H_EMBER_SHMEM_ALLTOALLS
#define _H_EMBER_SHMEM_ALLTOALLS

#include <strings.h>
#include "shmem/emberShmemGen.h"
#include <cxxabi.h>

namespace SST {
namespace Ember {

template <class TYPE>
class EmberShmemAlltoallsGenerator : public EmberShmemGenerator {

public:
	EmberShmemAlltoallsGenerator(SST::Component* owner, Params& params) :
		EmberShmemGenerator(owner, params, "ShmemAlltoalls" ), m_phase(0) 
	{ 
        m_nelems = params.find<int>("arg.nelems", 1 );
        m_dst = params.find<int>("arg.dst", 1 );
        m_sst = params.find<int>("arg.sst", 1 );
        m_printResults = params.find<bool>("arg.printResults", false );

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
            	printf("%d:%s: num_pes=%d nelems=%d type=\"%s\" dst=%d sst=%d\n",m_my_pe, getMotifName().c_str(), 
							m_num_pes, m_nelems, m_type_name.c_str(), m_dst, m_sst);
			}
            { 
                size_t buffer_size = 3 * sizeof(long);                   // for pSync
                buffer_size += m_nelems * sizeof(TYPE) * m_num_pes * m_sst;  // for source  
                buffer_size += m_nelems * sizeof(TYPE) * m_num_pes * m_dst;  // for dest
                enQ_malloc( evQ, &m_pSync, buffer_size );
            }
            break;

          case 2:
            m_pSync.at<long>(0) = 0;
            m_pSync.at<long>(1) = 0;
            m_pSync.at<long>(2) = 0;

            m_src = m_pSync.offset<long>(3);

            bzero( &m_src.at<TYPE>(0), sizeof(TYPE) * m_num_pes * m_nelems * m_dst);

            for ( int pe = 0; pe < m_num_pes; pe++ ) {
				int shift = (sizeof(TYPE) * 8 )/ 2;
                for ( int i = 0; i < m_nelems; i++ ) { 

                    m_src.at<TYPE>( pe * m_nelems *  m_sst + i * m_sst ) = ((TYPE) (m_my_pe + 1) << shift) | i + 1;
                } 
            }

            m_dest = m_src.offset<TYPE>( m_nelems * m_num_pes * m_sst );
			if ( m_printResults &&  0 == m_my_pe ) { 
            	printf("%d:%s: pSync=%#" PRIx64 " src=%#" PRIx64 " dest=%#" PRIx64 "\n",m_my_pe, 
                        getMotifName().c_str(), m_pSync.getSimVAddr(), 
                        m_src.getSimVAddr(), m_dest.getSimVAddr());
			}
            bzero( &m_dest.at<TYPE>(0), sizeof(TYPE) * m_num_pes * m_nelems * m_dst);

            enQ_barrier_all( evQ );
            break;

          case 3:
			if ( 4 == sizeof(TYPE) ) {
            	enQ_alltoalls32( evQ, m_dest, m_src, m_dst, m_sst, m_nelems, 0, 0, m_num_pes, m_pSync );
			} else {
            	enQ_alltoalls64( evQ, m_dest, m_src, m_dst, m_sst, m_nelems, 0, 0, m_num_pes, m_pSync );
			}
            break;

          case 4:
            for ( int pe = 0; pe < m_num_pes; pe++ ) {
				int shift = (sizeof(TYPE) * 8 )/ 2;
                for ( int i = 0; i < m_nelems; i++ ) {

					if ( m_printResults ) {
                    	printf("%d:%s: addr=%#" PRIx64 " pe=%d i=%d %#" PRIx64 "\n",m_my_pe, getMotifName().c_str(), 
                            (uint64_t) m_dest.getSimVAddr<TYPE>( pe * m_nelems * m_dst + i * m_dst  ), 
                            pe, i, (uint64_t) m_dest.at<TYPE>( pe * m_nelems * m_dst + i * m_dst));
					}

                    assert( m_dest.at<TYPE>( pe * m_nelems * m_dst + i * m_dst) == ( ((TYPE) (pe + 1) << shift) | i + 1  )  );
                }
            }
            if ( 0 == m_my_pe ) {
                printf("%d:%s: exit\n",m_my_pe, getMotifName().c_str());
            }
            ret = true;

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
    int m_dst;
    int m_sst;
    int m_nelems;
    int m_my_pe;
    int m_num_pes;
    int m_phase;
};

class EmberShmemAlltoalls32Generator : public EmberShmemAlltoallsGenerator<uint32_t> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberShmemAlltoalls32Generator,
        "ember",
        "ShmemAlltoalls32Motif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM alltoalls32",
        "SST::Ember::EmberGenerator"
    )

    SST_ELI_DOCUMENT_PARAMS(
    )

public:
	EmberShmemAlltoalls32Generator(SST::Component* owner, Params& params) :
	EmberShmemAlltoallsGenerator( owner, params) {} 
};

class EmberShmemAlltoalls64Generator : public EmberShmemAlltoallsGenerator<uint64_t> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberShmemAlltoalls64Generator,
        "ember",
        "ShmemAlltoalls64Motif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM alltoalls64",
        "SST::Ember::EmberGenerator"
    )

    SST_ELI_DOCUMENT_PARAMS(
    )

public:
	EmberShmemAlltoalls64Generator(SST::Component* owner, Params& params) :
	EmberShmemAlltoallsGenerator( owner, params) {} 
};

}
}

#endif

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


#ifndef _H_EMBER_SHMEM_ALLTOALL
#define _H_EMBER_SHMEM_ALLTOALL

#include <strings.h>
#include "shmem/emberShmemGen.h"
#include <cxxabi.h>

namespace SST {
namespace Ember {

template < class TYPE >
class EmberShmemAlltoallGenerator : public EmberShmemGenerator {

public:
	EmberShmemAlltoallGenerator(SST::ComponentId_t id, Params& params) :
		EmberShmemGenerator(id, params, "ShmemAlltoall" ), m_phase(0)
	{
        m_nelems = params.find<int>("arg.nelems", 1 );
        m_printResults = params.find<bool>("arg.printResults", false );
        int status;
        std::string tname = typeid(TYPE).name();
		char * tmp = abi::__cxa_demangle(tname.c_str(), NULL, NULL, &status);
        m_type_name = tmp;
		free(tmp);
		m_PE_start = params.find<int>("arg.PE_start", 0 );
		m_logPE_stride = params.find<int>("arg.logPE_stride", 0 );

		assert( 4 == sizeof(TYPE) || 8 == sizeof(TYPE) );
    }
	int stride() {
		return 1 << m_logPE_stride;
	}
	bool imIn( int my_pe ) {
		return my_pe >= m_PE_start && ( my_pe - m_PE_start) % stride() == 0;
	}
	int calcPE_size() {
		return (m_num_pes-m_PE_start)/stride() + (((m_num_pes-m_PE_start) % stride() ) ? 1:0);
	}
	int calcPE( int j ) {
		int tmp = j * stride() + m_PE_start;
		return tmp;
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
			m_PE_size = calcPE_size();
			if ( 0 == m_my_pe ) {
            	printf("%d:%s: num_pes=%d nelems=%d type=\"%s\"\n",m_my_pe, getMotifName().c_str(), m_num_pes, m_nelems, m_type_name.c_str());
				printf("%d:%s: PE_start=%d logPE_stride=%d PE_size=%d\n", m_my_pe, getMotifName().c_str(), m_PE_start, m_logPE_stride, m_PE_size );
			}
            {
                size_t buffer_size = 3 * sizeof(long);                   // for pSync
                buffer_size += m_nelems * sizeof(TYPE) * m_PE_size;  // for source
                buffer_size += m_nelems * sizeof(TYPE) * m_PE_size;  // for dest
                enQ_malloc( evQ, &m_pSync, buffer_size );
            }
            break;

          case 2:
			bzero( &m_pSync.at<long>(0), sizeof(long) * 3);

            m_src = m_pSync.offset<long>(3);

            for ( int h = 0; h < m_PE_size; h++ ) {
				int shift = (sizeof(TYPE) * 8 )/ 2;
                for ( int i = 0; i < m_nelems; i++ ) {
                    m_src.at<TYPE>( h * m_nelems + i ) = ((TYPE) (m_my_pe + 1) << shift) | i + 1;
                }
            }

            m_dest = m_src.offset<TYPE>( m_nelems * m_PE_size );
			if ( m_printResults && 0 == m_my_pe ) {
            	printf("%d:%s: pSync=%#" PRIx64 " src=%#" PRIx64 " dest=%#" PRIx64 "\n",m_my_pe,
                        getMotifName().c_str(), m_pSync.getSimVAddr(),
                        m_src.getSimVAddr(), m_dest.getSimVAddr());
			}
            bzero( &m_dest.at<TYPE>(0), sizeof(TYPE) * m_PE_size * m_nelems);

            enQ_barrier_all( evQ );
            break;

          case 3:
			if ( imIn( m_my_pe ) ) {
				if ( m_printResults ) {
					printf("%d:%s: ImIn\n",m_my_pe, getMotifName().c_str());
				}
				if ( 4 == sizeof( TYPE ) ) {
            		enQ_alltoall32( evQ, m_dest, m_src, m_nelems, m_PE_start, m_logPE_stride, m_PE_size, m_pSync );
				} else {
            		enQ_alltoall64( evQ, m_dest, m_src, m_nelems, m_PE_start, m_logPE_stride,  m_PE_size, m_pSync );
				}
			} else {
				ret = true;
			}
            break;

          case 4:
            for ( int h = 0; h < m_PE_size; h++ ) {
				int pe = calcPE(h);
				int shift = (sizeof(TYPE) * 8 )/ 2;
                for ( int i = 0; i < m_nelems; i++ ) {
					if ( m_printResults ) {
                    	printf("%d:%s: pe=%d i=%d %#" PRIx64 "\n",m_my_pe, getMotifName().c_str(),
                            pe, i, (uint64_t) m_dest.at<TYPE>( h * m_nelems + i) );
					}

                    assert( m_dest.at<TYPE>( h * m_nelems + i) == ( ((TYPE) (pe + 1) << shift) | i + 1  )  );
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
	int m_PE_start;
	int m_logPE_stride;
	int m_PE_size;;
	bool m_printResults;
	std::string m_type_name;
    Hermes::MemAddr m_pSync;
    Hermes::MemAddr m_src;
    Hermes::MemAddr m_dest;
    int m_nelems;
    int m_my_pe;
    int m_num_pes;
    int m_phase;
};
class EmberShmemAlltoall32Generator : public EmberShmemAlltoallGenerator<uint32_t> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberShmemAlltoall32Generator,
        "ember",
        "ShmemAlltoall32Motif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM alltoall32",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
		{ "arg.nelems","Sets number of data elements","1"},
		{ "arg.printResults","Should we print results","false"},
		{ "arg.PE_start","Sets the start PE","0"},
		{ "arg.logPE_stride","Sets the log PE stride","0"},
	)

public:
    EmberShmemAlltoall32Generator(SST::ComponentId_t id, Params& params) :
    	EmberShmemAlltoallGenerator( id, params) {}
};

class EmberShmemAlltoall64Generator : public EmberShmemAlltoallGenerator<uint64_t> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberShmemAlltoall64Generator,
        "ember",
        "ShmemAlltoall64Motif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM alltoall64",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
    )

public:
    EmberShmemAlltoall64Generator(SST::ComponentId_t id, Params& params) :
    	EmberShmemAlltoallGenerator( id, params) {}
};

}
}

#endif

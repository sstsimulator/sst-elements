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


#ifndef _H_EMBER_SHMEM_TEST1
#define _H_EMBER_SHMEM_TEST1

#include <strings.h>
#include "shmem/emberShmemGen.h"
#include <cxxabi.h>

namespace SST {
namespace Ember {

class EmberShmemFAM_GetGenerator : public EmberShmemGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberShmemFAM_GetGenerator,
        "ember",
        "ShmemFAM_GetMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM FAM_Get",
        "SST::Ember::EmberGenerator"

    )

    SST_ELI_DOCUMENT_PARAMS(
    )

public:
	EmberShmemFAM_GetGenerator(SST::Component* owner, Params& params) :
		EmberShmemGenerator(owner, params, "ShmemFAM_Get" ), m_phase(Init),m_curBlock(0)
	{ 
        m_nComputeNodes = params.find<int>("arg.nComputeNodes", 0);
        m_nFAMnodes     = params.find<int>("arg.nFAMnodes", 0);
		m_nFAMnodeBytes = (size_t) params.find<SST::UnitAlgebra>("arg.nFAMnodeBytes").getRoundedValue(); 
		m_blockSize	    = params.find<int>("arg.blockSize", 4096);
		m_vectorSize    = params.find<int>("arg.vectorSize", (m_nFAMnodes * m_nFAMnodeBytes) / sizeof(int)); 
		m_getLoop       = params.find<int>("arg.getLoop", 1);
		
		assert( m_vectorSize * sizeof(int) <= m_nFAMnodes * m_nFAMnodeBytes );
		assert( m_nFAMnodes );
		assert( m_nFAMnodeBytes );

		m_numBlocks = (m_vectorSize*sizeof(int))/m_blockSize;
		m_numBlocksPerFAMnode = m_numBlocks/m_nFAMnodes;

        m_miscLib = static_cast<EmberMiscLib*>(getLib("HadesMisc"));
        assert(m_miscLib);
	}

	EmberMiscLib* m_miscLib;
	int m_numBlocksPerFAMnode;
	int m_vectorSize;
	size_t m_nFAMnodeBytes;
	int m_nFAMnodes;
	int m_nComputeNodes;
	int m_num_nodes;
	int m_node_num;
	int m_blockSize;
	int m_curBlock;
	int m_numBlocks;
	int m_getLoop;

    bool generate( std::queue<EmberEvent*>& evQ) 
	{
        switch ( m_phase ) {
        case Init:

            enQ_init( evQ );
            enQ_n_pes( evQ, &m_num_pes );
            enQ_my_pe( evQ, &m_my_pe );
			m_miscLib->getNodeNum( evQ, &m_node_num );
            m_miscLib->getNumNodes( evQ, &m_num_nodes );
			m_phase = Alloc;
            break;

        case Alloc:

			assert( m_nFAMnodes < m_num_nodes );
			if ( m_nComputeNodes == 0 ) {
				m_nComputeNodes = m_num_nodes - m_nFAMnodes;
			}
			assert( m_nFAMnodes + m_nComputeNodes <= m_num_nodes );

			if ( m_node_num == 0 ) {
				printf("number of FAM nodes:     %d\n", m_nFAMnodes);
				printf("number of Compute nodes: %d\n", m_nComputeNodes );
				printf("number of pes:           %d\n",	m_num_pes );
				printf("block size:              %d\n",	m_blockSize );
				printf("number of blocks:        %d\n",	m_numBlocks );
				printf("vector num elements:     %d\n",	m_vectorSize );
			}

			{
				size_t bufSize; 
				std::string tmp;
				if ( m_node_num < m_nFAMnodes ) {
					bufSize = m_nFAMnodeBytes;
					tmp = "FAM";
					m_phase = Wait;
				} else {
					tmp = "compute";
				   	bufSize = m_vectorSize * sizeof(int);
					m_phase = Work;
				}

				if ( m_node_num == 0 || m_node_num == m_nFAMnodes ) {
					printf("%s node buffer size: %lu\n", tmp.c_str(), bufSize );
				}

            	enQ_malloc( evQ, &m_mem, bufSize );
			}

            enQ_barrier_all( evQ );
            break;

        case Work:
			
			//printf("pe=%d call work curBlock=%d\n",m_my_pe,m_curBlock);
			if ( ! work( evQ ) ) {
				m_phase = Wait;
			}
			break;

        case Wait:
            enQ_barrier_all( evQ );
			m_phase = Fini;
            break;

        case Fini:
			break;
        }
        return m_phase == Fini;
	}

	bool work(  std::queue<EmberEvent*>& evQ ) {

		for ( int i = 0; i < m_getLoop && m_curBlock < m_numBlocks; i++ ) {

			int srcPe = calcBlockOwner( m_curBlock );
			int srcBlock = calcSrcBlock( m_curBlock );

			//printf("pe=%d curBlock=%d srcPe=%d srcBLock=%d\n",m_my_pe,m_curBlock,srcPe,srcBlock);

    		Hermes::MemAddr m_src = m_mem.offset<unsigned char>( 4096 * srcBlock );
    		Hermes::MemAddr m_dest = m_mem.offset<unsigned char>( 4096 * m_curBlock );

        	enQ_get_nbi( evQ, 
                    m_dest,
                    m_src, 
					m_blockSize,
					srcPe );
			++m_curBlock;
		}

        enQ_quiet( evQ );
		return m_curBlock < m_numBlocks;
	}

	int calcSrcBlock( int block ) {
		return block / m_numBlocksPerFAMnode;
		//return block % m_numBlocksPerFAMnode;
	}

	int calcBlockOwner( int block ) {
		//return block / m_numBlocksPerFAMnode;
		return block % m_nFAMnodes;
	}

  private:
    Hermes::MemAddr m_mem;
    enum { Init, Alloc, Work, Wait, Fini } m_phase;
    int m_my_pe;
    int m_num_pes;
};

}
}

#endif

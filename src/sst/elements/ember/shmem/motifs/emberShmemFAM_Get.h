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


#ifndef _H_EMBER_SHMEM_FAM_GET
#define _H_EMBER_SHMEM_FAM_GET

#include <strings.h>
#include "shmem/emberShmemGen.h"
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include "rng/xorshift.h"

#include <unistd.h>

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
		EmberShmemGenerator(owner, params, "ShmemFAM_Get" ), m_phase(Init),m_curBlock(0),m_rng(NULL)
	{ 
        m_nComputeNodes = params.find<int>("arg.nComputeNodes", 0);
        m_nFAMnodes     = params.find<int>("arg.nFAMnodes", 0);
        m_stride        = params.find<int>("arg.stride", 1);
		m_nFAMnodeBytes = (size_t) params.find<SST::UnitAlgebra>("arg.nFAMnodeBytes").getRoundedValue(); 
		m_blockSize	    = params.find<int>("arg.blockSize", 4096);
		m_vectorSize    = params.find<size_t>("arg.vectorSize", (m_nFAMnodes * m_nFAMnodeBytes) / sizeof(int)); 
		m_getLoop       = params.find<int>("arg.getLoop", 1);
		m_partitionSize = (size_t) params.find<SST::UnitAlgebra>("arg.partitionSize","16MiB").getRoundedValue(); 
		m_maxDelay      = params.find<int>("arg.maxDelay",20);
		bool useRand    = params.find<int>("arg.useRand",false);
		
		assert( m_vectorSize * sizeof(int) <= m_nFAMnodes * m_nFAMnodeBytes );
		assert( m_nFAMnodes );
		assert( m_nFAMnodeBytes );

		m_numBlocks = (m_vectorSize*sizeof(int))/m_blockSize;
		m_numBlocksPerFAMnode = m_numBlocks/m_nFAMnodes;
		m_numBlocksPerPartition = m_partitionSize/m_blockSize;

        m_miscLib = static_cast<EmberMiscLib*>(getLib("HadesMisc"));
        assert(m_miscLib);

		if ( useRand ) {
			m_rng = new SST::RNG::XORShiftRNG();
        	struct timeval start;
        	gettimeofday( &start, NULL );
			m_rng->seed( start.tv_usec );
		}
	}

	EmberMiscLib* m_miscLib;
	int m_numBlocksPerPartition;
	int m_numBlocksPerFAMnode;
	size_t m_partitionSize;
	size_t m_vectorSize;
	size_t m_nFAMnodeBytes;
	int m_nFAMnodes;
	int m_nComputeNodes;
	int m_num_nodes;
	int m_node_num;
	int m_blockSize;
	int m_curBlock;
	int m_numBlocks;
	int m_getLoop;
	int m_stride;
	int m_maxDelay;
	SST::RNG::XORShiftRNG* m_rng;

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
				printf("vector num elements:     %zu\n",	m_vectorSize );
				if ( m_rng ) {
					printf("using random:        %d\n",	m_maxDelay );
				}
			}

			{
				size_t bufSize; 
				std::string tmp;
				if ( iAmCompute( m_node_num ) ) {
					tmp = "compute";
				   	bufSize = m_partitionSize;
					m_phase = Work;
				} else if ( iAmFAM( m_node_num ) ) {
					bufSize = m_nFAMnodeBytes;
					tmp = "FAM";
					m_phase = Wait;
				} else {
					bufSize = 0;
					tmp = "idle";
					m_phase = Wait;
				}

				if ( bufSize ) {
					printf("physNode=%d is %s node %d\n",m_node_num, tmp.c_str(), calcVirtNum(m_node_num) );
            		enQ_malloc( evQ, &m_mem, bufSize );
				}
			}

            enQ_barrier_all( evQ );
            break;

        case Work:
			
			if ( ! work( evQ ) ) {
				m_phase = Wait;
			}
			break;

        case Wait:
            enQ_barrier_all( evQ );
            enQ_barrier_all( evQ );
			m_phase = Fini;
            break;

        case Fini:
			break;
        }
        return m_phase == Fini;
	}

	bool isStride( int node_num, int which ) {
		bool ret = (node_num / m_stride) % 2 == which;
		//printf("%s() node=%d which=%d %s \n",__func__, node_num, which, ret ? "yes":"no" );
		return  ret;
	}
	int calcFAMnum( int node_num ) {
		int ret = calcVirtNum(node_num);
		//printf("%s() node=%d -> %d \n",__func__, node_num, ret );
		return ret; 
	}

	int calcComputeNum( int node_num ) {
		int ret = calcVirtNum(node_num);
		//printf("%s() node=%d -> %d \n",__func__, node_num, ret );
		return ret;
	}

	int calcVirtNum( int node_num ) {
		if ( m_stride == 0 ) return node_num % (m_num_nodes/2);

		int ret = (node_num/m_stride) / 2 * m_stride + node_num % m_stride;
		return ret;
	}

	bool iAmFAM( int node_num ) {
		//printf("%s() node=%d\n",__func__, node_num);
		if ( m_stride == 0 ) return node_num < m_nFAMnodes;

		bool ret = isStride(node_num,0) && calcFAMnum(node_num) < m_nFAMnodes;
		//printf("%s() node=%d %d\n",__func__, node_num, ret );
		return ret;
	}

	bool iAmCompute( int node_num ) {
		//printf("%s() node=%d\n",__func__, node_num);

		if ( m_stride == 0 ) return node_num >= m_nFAMnodes && node_num < m_nFAMnodes + m_nComputeNodes;

		bool ret = isStride(node_num,1) && calcComputeNum(node_num) < m_nComputeNodes;
		//printf("%s() node=%d %d\n",__func__, node_num, ret );
		return ret; 
	}

	int calcFAMphysNode( int virt ) {
		if ( m_stride == 0 ) return virt;
		int rem = virt % m_stride; 
		return (virt - rem) * 2  +  rem;
	}

	int mapPeToFAM( int linePe ) {
		
		int phys;
		if ( m_stride == 0 ) {
			phys = linePe;
		} else if (  m_stride == 1 )  {
			phys = linePe * 2;
		} else {
			phys = calcFAMphysNode(linePe);
		}
		//printf( "%d:%s() linePe %d -> %d\n", m_my_pe, __func__, linePe, phys );
		return phys;
	}

	bool work(  std::queue<EmberEvent*>& evQ ) {

		for ( int i = 0; i < m_getLoop && m_curBlock < m_numBlocks; i++ ) {

			int srcPe = calcBlockOwner( m_curBlock );
			int srcBlock = calcSrcBlock( m_curBlock );

			if ( m_rng ) {
				int delay = m_rng->generateNextUInt32();
				enQ_compute( evQ, delay % m_maxDelay );
			}

			//printf("pe=%d curBlock=%d srcPe=%d srcBLock=%d\n",m_my_pe,m_curBlock,srcPe,srcBlock);

			if ( srcBlock >= 512 ) {
				printf("src=%d cur=%d\n",srcBlock,m_curBlock);
				exit(-1);
			}
			if ( (m_curBlock % m_numBlocksPerPartition) >= 4096 ) {
				printf("dest=%d cur=%d\n",(m_curBlock % m_numBlocksPerPartition),m_curBlock);
				exit(-1);
			}
    		Hermes::MemAddr m_src = m_mem.offset<unsigned char>( 4096 * srcBlock );
    		Hermes::MemAddr m_dest = m_mem.offset<unsigned char>( 4096 * (m_curBlock % m_numBlocksPerPartition)  );

        	enQ_get_nbi( evQ, 
                    m_dest,
                    m_src, 
					m_blockSize,
					mapPeToFAM( srcPe ) );

			++m_curBlock;
		}

        enQ_quiet( evQ );
		return m_curBlock < m_numBlocks;
	}

	int calcSrcBlock( int block ) {
		return block / m_nFAMnodes;
	}

	int calcBlockOwner( int block ) {
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

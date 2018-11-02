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


#ifndef _H_EMBER_SHMEM_FAM_GET2
#define _H_EMBER_SHMEM_FAM_GET2

#include <strings.h>
#include "shmem/emberShmemGen.h"
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include "rng/xorshift.h"

#include <unistd.h>

namespace SST {
namespace Ember {

class EmberShmemFAM_Get2Generator : public EmberShmemGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        EmberShmemFAM_Get2Generator,
        "ember",
        "ShmemFAM_Get2Motif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM FAM_Get2",
        "SST::Ember::EmberGenerator"

    )

    SST_ELI_DOCUMENT_PARAMS(
    )

public:
	EmberShmemFAM_Get2Generator(SST::Component* owner, Params& params) :
		EmberShmemGenerator(owner, params, "ShmemFAM_Get2" ), m_phase(Init), m_groupName("MyApplication"),m_curBlock(0),m_rng(NULL)
	{ 
		m_totalBytes = (size_t) params.find<SST::UnitAlgebra>("arg.totalBytes").getRoundedValue(); 
		m_getLoop       = params.find<int>("arg.getLoop", 1);
		m_maxDelay      = params.find<int>("arg.maxDelay",20);
		m_blockSize	    = params.find<int>("arg.blockSize", 4096);
		m_partitionSize = (size_t) params.find<SST::UnitAlgebra>("arg.partitionSize","16MiB").getRoundedValue();
		m_backed	    = (bool) ( 0 == params.find<std::string>("arg.backed", "yes").compare("yes") );

		m_numBlocks = m_totalBytes/m_blockSize;
		m_numBlocksPerPartition = m_partitionSize/m_blockSize;

        m_miscLib = static_cast<EmberMiscLib*>(getLib("HadesMisc"));
        assert(m_miscLib);

		if ( params.find<int>("arg.useRand",false) ) {
			m_rng = new SST::RNG::XORShiftRNG();
        	struct timeval start;
        	gettimeofday( &start, NULL );
			m_rng->seed( start.tv_usec );
		}
	}

	uint64_t m_regionSize;
	std::string m_groupName;
	Fam_Region_Descriptor m_rd;
	EmberMiscLib* m_miscLib;

	bool m_backed;
	int m_numBlocksPerPartition;
	int m_partitionSize;
	int m_blockSize;
	int m_maxDelay;
	int m_blockOffset;
	int m_getLoop;
	size_t m_totalBytes;
	int m_numBlocks;
	int m_curBlock;
	int m_node_num;
	SST::RNG::XORShiftRNG* m_rng;

    bool generate( std::queue<EmberEvent*>& evQ) 
	{
        switch ( m_phase ) {
        case Init:

            enQ_fam_initialize( evQ, m_groupName );
            enQ_n_pes( evQ, &m_num_pes );
            enQ_my_pe( evQ, &m_my_pe );
			m_miscLib->getNodeNum( evQ, &m_node_num );
			m_phase = Alloc;
            break;

        case Alloc:

			if ( m_my_pe == 0 ) {
				printf("number of pes:           %d\n",	m_num_pes );
				printf("block size:              %d\n",	m_blockSize );
				printf("number of blocks:        %d\n",	m_numBlocks );
				printf("loop:                    %d\n",	m_getLoop );
				if ( m_rng ) {
					printf("using random:        %d\n",	m_maxDelay );
				}
			}
			m_blockOffset = m_my_pe;
			enQ_malloc( evQ, &m_mem, m_numBlocksPerPartition * m_blockSize, m_backed );
			m_phase = Work;
            break;

        case Work:
			
			if ( ! work( evQ ) ) {
				enQ_quiet( evQ );
				m_phase = Wait;
			}
			break;

        case Wait:

            enQ_barrier_all( evQ );
			m_phase = Fini;
            break;

        case Fini:

			if ( m_my_pe == 0 ) {
				printf("Finished\n");
			}
			return true;
        }
        return false;
	}

	bool work(  std::queue<EmberEvent*>& evQ ) {

		for ( int i = 0; i < m_getLoop && m_curBlock < m_numBlocks; i++ ) {

			int foo = (m_curBlock + m_blockOffset) % m_numBlocks; 
			uint64_t offset = foo * m_blockSize;

			if ( m_rng ) {
				int delay = m_rng->generateNextUInt32();
				enQ_compute( evQ, delay % m_maxDelay );
			}

			verbose(CALL_INFO,2,0,"0x%" PRIx64" %p\n", m_mem.getSimVAddr(), m_mem.getBacking() );
    		Hermes::MemAddr m_dest = m_mem.offset<unsigned char>( 4096 * (m_curBlock % m_numBlocksPerPartition) );

        	enQ_fam_get_nonblocking( evQ, m_dest,
                    m_rd,
                    offset, 
					m_blockSize );

			++m_curBlock;
		}

		return m_curBlock < m_numBlocks;
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

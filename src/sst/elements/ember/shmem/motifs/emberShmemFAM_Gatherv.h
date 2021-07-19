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


#ifndef _H_EMBER_SHMEM_FAM_GATHERV
#define _H_EMBER_SHMEM_FAM_GATHERV

#include <strings.h>
#include "shmem/emberShmemGen.h"
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include "rng/xorshift.h"

#include <unistd.h>

namespace SST {
namespace Ember {

class EmberShmemFAM_GathervGenerator : public EmberShmemGenerator {

public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberShmemFAM_GathervGenerator,
        "ember",
        "ShmemFAM_GathervMotif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM FAM_Gatherv",
        SST::Ember::EmberGenerator
    )

    SST_ELI_DOCUMENT_PARAMS(
		{"arg.blocking","Sets if FAM ops are blocking","false"},
		{"arg.blockSize","Sets size of each block of data transfers","4096"},
		{"arg.numBlocks","Sets number of blacks","0"},
		{"arg.firstBlock","Sets first block","0"},
		{"arg.blockStride","Sets stride between blocks","0"},
		{"arg.backed","Sets SHMEM memory backing","no"},
	)

public:
	EmberShmemFAM_GathervGenerator(SST::ComponentId_t id, Params& params) :
		EmberShmemGenerator(id, params, "ShmemFAM_Gatherv" ), m_phase(Init), m_groupName("MyApplication")
	{
		m_blocking		= params.find<bool>("arg.blocking", false);
		m_blockSize	    = params.find<int>("arg.blockSize", 4096);
		m_numBlocks	    = params.find<int>("arg.numBlocks", 0);
		m_firstBlock	= params.find<int>("arg.firstBlock", 0);
		m_blockStride	= params.find<int>("arg.blockStride", 0);
		assert( m_numBlocks > 0 );
		m_backed	    = (bool) ( 0 == params.find<std::string>("arg.backed", "no").compare("yes") );
		m_indexes.resize( m_numBlocks );
		for ( int i = 0; i < m_numBlocks; i++ ) {
			m_indexes[i] = i;
		}
	}

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

			setVerbosePrefix( m_my_pe );
			if ( m_my_pe == 0 ) {
				printf("number of pes:           %d\n",	m_num_pes );
				printf("block size:              %" PRIu64 "\n",	m_blockSize );
				printf("number of blocks:        %" PRIu64 "\n",	m_numBlocks );
				printf("firstBlock:              %" PRIu64 "\n",	m_firstBlock );
				printf("blockStride              %" PRIu64 "\n",	m_blockStride );
			}
			enQ_malloc( evQ, &m_src, m_blockSize * m_numBlocks, m_backed );
			enQ_getTime( evQ, &m_startTime );
			m_phase = Work;
            break;

        case Work:

			if ( m_blockStride ) {
				if ( m_blocking ) {
        			enQ_fam_gather_blocking( evQ, m_src, m_fd, m_numBlocks, m_firstBlock, m_blockStride,
						m_blockSize );
				} else {
        			enQ_fam_gather_nonblocking( evQ, m_src, m_fd, m_numBlocks, m_firstBlock, m_blockStride,
						m_blockSize );
				}
			} else {
				if ( m_blocking ) {
        			enQ_fam_gatherv_blocking( evQ, m_src, m_fd, m_numBlocks, m_indexes, m_blockSize );
				} else {
        			enQ_fam_gatherv_nonblocking( evQ, m_src, m_fd, m_numBlocks, m_indexes, m_blockSize );
				}
			}

			if ( ! m_blocking ) {
				enQ_getTime( evQ, &m_postTime );
			}
			enQ_quiet( evQ );
			enQ_getTime( evQ, &m_stopTime );
			m_phase = Fini;
            break;

        case Fini:

            if ( m_my_pe == 0 ) {
                double time = m_stopTime-m_startTime;
                size_t bytes = m_numBlocks * m_blockSize;
				uint64_t postTime;
				if ( ! m_blocking ) {
					postTime = m_postTime - m_startTime;
				} else {
					postTime = m_stopTime - m_startTime;
				}
                printf("%d:%s:  %zu bytes, %.3lf GB/s, postTime %" PRIu64 " ns\n",m_my_pe, getMotifName().c_str(), bytes,
					(double) bytes/ time, postTime );
            }
            return true;
        }
        return false;
	}


  private:

	bool m_backed;
	uint64_t m_firstBlock;
	uint64_t m_blockStride;
	uint64_t m_numBlocks;
	uint64_t m_blockSize;
	std::vector<uint64_t> m_indexes;

    Hermes::MemAddr m_src;

	std::string m_groupName;
	Shmem::Fam_Descriptor m_fd;

    enum { Init, Alloc, Work, Fini } m_phase;

	int m_node_num;
    int m_my_pe;
    int m_num_pes;
    uint64_t m_startTime;
    uint64_t m_stopTime;
    uint64_t m_postTime;
	bool m_blocking;
};

}
}

#endif

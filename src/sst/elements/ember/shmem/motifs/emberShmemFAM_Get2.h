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
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        EmberShmemFAM_Get2Generator,
        "ember",
        "ShmemFAM_Get2Motif",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SHMEM FAM_Get2",
        SST::Ember::EmberGenerator

    )

    SST_ELI_DOCUMENT_PARAMS(
		{"arg.totalBytes","Sets total size of FAM memory space",""},\
		{"arg.getLoop","Sets the number of gets to queue each call to work()","1"},\
		{"arg.maxDelay","Sets the max delay when using random compute time ","20"},\
		{"arg.blockSize","Sets the size of transfer","4096"},\
		{"arg.partitionSize","Sets the size of local memory used for incoming get data",""},\
		{"arg.backed","Sets if FAM memory is backed","false"},\
		{"arg.randomGet","Sets if address of each get is random","false"},\
		{"arg.stream_n","Sets the size of the stream for the detailed model","1000"},\
		{"arg.useRand","Sets if random compute time is used","false"},\
		{"arg.blocking","Sets if blocking FAM calls are used","false"},\
		{"arg.detailedCompute","Sets the list of PEs that use detailed model for compute delay",""},\
    )

public:
	EmberShmemFAM_Get2Generator(SST::ComponentId_t id, Params& params) :
		EmberShmemGenerator(id, params, "ShmemFAM_Get2" ), m_phase(Init), m_groupName("MyApplication"),m_curBlock(0),m_rng(NULL)
	{
		m_totalBytes = (size_t) params.find<SST::UnitAlgebra>("arg.totalBytes").getRoundedValue();
		m_getLoop       = params.find<int>("arg.getLoop", 1);
		m_maxDelay      = params.find<int>("arg.maxDelay",20);
		m_blockSize	    = params.find<int>("arg.blockSize", 4096);
		m_partitionSize = (size_t) params.find<SST::UnitAlgebra>("arg.partitionSize","16MiB").getRoundedValue();
		m_backed	    = (bool) ( 0 == params.find<std::string>("arg.backed", "yes").compare("yes") );
		m_randomGet     = params.find<bool>("arg.randomGet",false);
		m_stream_n = params.find<int32_t>("arg.stream_n", 1000);
		m_randCompute = params.find<int>("arg.useRand",false);
		m_blocking = params.find<bool>("arg.blocking",false);

		m_detailedComputeList =	params.find<std::string>("arg.detailedCompute","");

		assert ( ! ( m_randCompute && m_detailedCompute ) );

		m_numBlocks = m_totalBytes/m_blockSize;
		m_numBlocksPerPartition = m_partitionSize/m_blockSize;

		if ( m_randCompute || m_randomGet ) {
			m_rng = new SST::RNG::XORShiftRNG();
        	struct timeval start;
        	gettimeofday( &start, NULL );
			m_rng->seed( start.tv_usec );
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

			m_detailedCompute = findNid( m_node_num, m_detailedComputeList );
			if ( m_my_pe == 0 ) {
				printf("number of pes:           %d\n",	m_num_pes );
				printf("block size:              %d\n",	m_blockSize );
				printf("number of blocks:        %d\n",	m_numBlocks );
				printf("loop:                    %d\n",	m_getLoop );
				if ( m_randomGet ) {
					printf("randomGet                yes\n" );
				}
				if ( m_randCompute ) {
					printf("random compute, maxDelay: %d\n", m_maxDelay );
				}
				if ( ! m_detailedComputeList.empty() ) {
					printf("detailedComputeList:     %s\n", m_detailedComputeList.c_str() );
				}
			}

			if ( m_detailedCompute ) {
			    printf("node %d, pe %d using detailed compute\n", m_node_num, m_my_pe  );
			}

			m_blockOffset = m_my_pe;
			enQ_malloc( evQ, &m_mem, m_numBlocksPerPartition * m_blockSize, m_backed );
			enQ_malloc( evQ, &m_streamBuf, m_stream_n * 8 * 3);
			enQ_getTime( evQ, &m_startTime );
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
			enQ_getTime( evQ, &m_stopTime );
			m_phase = Fini;
            break;

        case Fini:

            if ( m_my_pe == 0 ) {
                double time = m_stopTime-m_startTime;
                size_t bytes = m_numBlocks * m_blockSize;
                printf("%d:%s:  %zu bytes, %.3lf GB/s \n",m_my_pe, getMotifName().c_str(), bytes, (double) bytes/ time );
            }
            return true;
        }
        return false;
	}

  private:

	bool work(  std::queue<EmberEvent*>& evQ ) {

		for ( int i = 0; i < m_getLoop && m_curBlock < m_numBlocks; i++ ) {

			uint32_t block;
			if ( m_randomGet ) {
				block = m_rng->generateNextUInt32();
			} else {
				block = (m_curBlock + m_blockOffset);
			}

			block %= m_numBlocks;
			uint64_t offset = block * m_blockSize;

			if (m_randCompute ) {
				int delay = m_rng->generateNextUInt32();
				enQ_compute( evQ, delay % m_maxDelay );
			} else if ( m_detailedCompute ) {
				computeDetailed( evQ );
			}

			verbose(CALL_INFO,2,0,"0x%" PRIx64" %p\n", m_mem.getSimVAddr(), m_mem.getBacking() );
    		Hermes::MemAddr m_dest = m_mem.offset<unsigned char>( m_blockSize * (m_curBlock % m_numBlocksPerPartition) );

			if ( m_blocking ) {
        		enQ_fam_get_blocking( evQ, m_dest,
                    m_fd,
                    offset,
					m_blockSize );
			} else {
        		enQ_fam_get_nonblocking( evQ, m_dest,
                    m_fd,
                    offset,
					m_blockSize );
			}

			++m_curBlock;
		}

		return m_curBlock < m_numBlocks;
	}


    void computeDetailed( std::queue<EmberEvent*>& evQ)
    {
        verbose( CALL_INFO, 1, 0, "\n");

        Params params;

        std::string motif;

        std::stringstream tmp;

        motif = "miranda.STREAMBenchGenerator";

        tmp.str( std::string() ); tmp.clear();
        tmp << m_stream_n;
        params.insert("n", tmp.str() );

        tmp.str( std::string() ); tmp.clear();
        tmp << m_streamBuf.getSimVAddr();
        params.insert("start_a", tmp.str() );

        params.insert("operandwidth", "8",true);

        params.insert( "generatorParams.verbose", "0" );
        params.insert( "verbose", "0" );

        enQ_detailedCompute( evQ, motif, params );
    }

    bool findNid( int nid, std::string nidList ) {

        if ( 0 == nidList.compare( "all" ) ) {
            return true;
        }

        if ( nidList.empty() ) {
            return false;
        }

        size_t pos = 0;
        size_t end = 0;
        do {
            end = nidList.find( ",", pos );
            if ( end == std::string::npos ) {
                end = nidList.length();
            }
            std::string tmp = nidList.substr(pos,end-pos);

            if ( tmp.length() == 1 ) {
                int val = atoi( tmp.c_str() );
                if ( nid == val ) {
                    return true;
                }
            } else {
                size_t dash = tmp.find( "-" );
                int first = atoi(tmp.substr(0,dash).c_str()) ;
                int last = atoi(tmp.substr(dash+1).c_str());
                if ( nid >= first && nid <= last ) {
                    return true;
                }
            }

            pos = end + 1;
        } while ( end < nidList.length() );

        return false;
    }

	bool m_randCompute;
	bool m_detailedCompute;
	std::string m_detailedComputeList;

	Hermes::MemAddr m_streamBuf;
    Hermes::MemAddr m_mem;

	uint64_t m_regionSize;
	std::string m_groupName;
	Shmem::Fam_Descriptor m_fd;

	int m_stream_n;
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
    enum { Init, Alloc, Work, Wait, Fini } m_phase;
    int m_my_pe;
    int m_num_pes;
    uint64_t m_startTime;
    uint64_t m_stopTime;
    bool m_randomGet;
	bool m_blocking;
};

}
}

#endif

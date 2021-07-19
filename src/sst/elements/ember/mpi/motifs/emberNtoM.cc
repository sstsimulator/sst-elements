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


#include <sst_config.h>
#include "emberNtoM.h"

using namespace SST::Ember;

#define TAG 0xDEADBEEF

EmberNtoMGenerator::EmberNtoMGenerator(SST::ComponentId_t id, Params& params ) :
	EmberMessagePassingGenerator(id, params, "NtoM"), m_phase(Init), m_currentBuf(0), m_currentMsg(0), m_currentTarget(0),
    m_loopIndex(0)
{
	m_messageSize = params.find<uint32_t>("arg.messageSize", 1024);
	m_iterations =  params.find<uint32_t>("arg.iterations", 1);
	m_numRecvBufs = params.find<uint32_t>("arg.numRecvBufs", 1);
	std::string targetRankList = params.find<std::string>("arg.targetRankList","0" );
	m_target = findNum( rank(), targetRankList );
	getTargetRanks( targetRankList, m_targetRanks );
	int numSrc = size() - m_targetRanks.size(); 
	m_totalMsgs = numSrc * m_iterations;

    m_sendBuf = memAlloc(m_messageSize);
    m_recvBuf = memAlloc(m_messageSize * m_numRecvBufs );

	m_req.resize( m_numRecvBufs );
	if ( rank() == 0 ) {
		printf("targetRankList: %s\n",targetRankList.c_str());
		printf("messageSize:    %d\n",m_messageSize);
		printf("iterations:     %d\n",m_iterations);
		printf("numRecvBufs:    %d\n",m_numRecvBufs);
		printf("numSrc:         %d\n",numSrc);
		printf("totalMsgs:      %d\n",m_totalMsgs);
	}
}

bool EmberNtoMGenerator::generate( std::queue<EmberEvent*>& evQ)
{
	if ( m_target ) {
		return target( evQ );
	} else {
		return source( evQ );
	}
}

bool EmberNtoMGenerator::source( std::queue<EmberEvent*>& evQ) {
	if ( m_phase == Init) { 
		enQ_barrier( evQ, GroupWorld );
		m_phase = Run;
	} else {
    	if ( m_loopIndex < m_iterations * m_targetRanks.size() ) {
			int i = m_loopIndex % m_targetRanks.size(); 
			verbose(CALL_INFO, 2, MOTIF_MASK,"%d send to %d\n",rank(),m_targetRanks[i]);
    		enQ_send( evQ, m_sendBuf, m_messageSize, CHAR, m_targetRanks[i], TAG, GroupWorld );
			++m_loopIndex;
		} else {
			return true;
		}
	}

	return false;
}
bool EmberNtoMGenerator::target( std::queue<EmberEvent*>& evQ) {
	switch ( m_phase ) { 
	  case Init:
		for ( int i = 0; i < m_numRecvBufs; i++ ) {
           	enQ_irecv( evQ, (unsigned char*) m_recvBuf + m_messageSize * i, m_messageSize, CHAR, AnySrc,
                                                TAG, GroupWorld, &m_req[i] );
			verbose( CALL_INFO, 2, MOTIF_MASK,"post receive\n");
		}
		m_phase = Run;
		enQ_barrier( evQ, GroupWorld );

		verbose(CALL_INFO, 2, MOTIF_MASK,"wait on receive\n");
        enQ_wait( evQ, &m_req[m_currentBuf], &m_resp );

        enQ_getTime( evQ, &m_startTime );
		break;

	  case Run:
		
		++m_currentMsg;
		
		if ( m_currentMsg < m_totalMsgs ) {

			verbose(CALL_INFO, 2, MOTIF_MASK,"%d got message from %d\n",rank(),m_resp.src);
			verbose(CALL_INFO, 2, MOTIF_MASK,"post receive\n");
			// note that we will end up with posted buffers that are not used
       		enQ_irecv( evQ, (unsigned char*) m_recvBuf + m_messageSize * m_currentBuf, m_messageSize, CHAR, AnySrc,
                                                TAG, GroupWorld, &m_req[m_currentBuf] );
		
			// we just reposted buffer N, check to see if N+1 is ready
			m_currentBuf = (m_currentBuf + 1 ) % m_numRecvBufs;
			verbose(CALL_INFO, 2, MOTIF_MASK,"wait on receive\n");
        	enQ_wait( evQ, &m_req[m_currentBuf], &m_resp );

		} else {
			verbose(CALL_INFO, 2, MOTIF_MASK,"%d got message from %d\n",rank(),m_resp.src);
        	enQ_getTime( evQ, &m_stopTime );
			m_phase = Fini;
		} 
		break;

	  case Fini:
		if ( 0 == rank() ) {
            double totalTime = (double)(m_stopTime - m_startTime)/1000000000.0;
			uint64_t totalBytes = m_totalMsgs * m_messageSize; 
			printf("%s: time=%.3f us totalBytes=%" PRIu64 " bandwidth=%.3f MB/s\n", getMotifName().c_str(),
				totalTime * 1000000.0, totalBytes, (totalBytes/totalTime)/1000000.0);
		}
		return true;
	}
	return false;
}

bool EmberNtoMGenerator::findNum( int num, std::string& numList ) {

    if ( numList.empty() ) {
        return false;
    }

    size_t pos = 0;
    size_t end = 0;
    do {
        end = numList.find( ",", pos );
        if ( end == std::string::npos ) {
            end = numList.length();
        }
        std::string tmp = numList.substr(pos,end-pos);

        if ( tmp.length() == 1 ) {
            int val = atoi( tmp.c_str() );
            if ( num == val ) {
				//printf("%d: is target\n",rank());
                return true;
            }
        } else {
            size_t dash = tmp.find( "-" );
            int first = atoi(tmp.substr(0,dash).c_str()) ;
            int last = atoi(tmp.substr(dash+1).c_str());
            if ( num >= first && num <= last ) {
				//printf("%d: is target\n",rank());
                return true;
            }
        }

        pos = end + 1;
    } while ( end < numList.length() );

    return false;
}

void EmberNtoMGenerator::getTargetRanks( std::string& numList, std::vector<uint32_t>& buf ) {
    if ( numList.empty() ) {
        return;
    }

    size_t pos = 0;
    size_t end = 0;
    do {
        end = numList.find( ",", pos );
        if ( end == std::string::npos ) {
            end = numList.length();
        }
        std::string tmp = numList.substr(pos,end-pos);

        if ( tmp.length() == 1 ) {
            int val = atoi( tmp.c_str() );
			//printf("%d: add %d\n",rank(),val);
			buf.push_back( val ); 
        } else {
            size_t dash = tmp.find( "-" );
            int first = atoi(tmp.substr(0,dash).c_str()) ;
            int last = atoi(tmp.substr(dash+1).c_str());
			for ( auto i = first; i <= last; i++ ) {
				//printf("%d: add %d\n",rank(),i);
				buf.push_back( i ); 
            }
        }

        pos = end + 1;
    } while ( end < numList.length() );

}

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
#include "emberdetailedring.h"

using namespace SST::Ember;

#define TAG 0xDEADBEEF

EmberDetailedRingGenerator::EmberDetailedRingGenerator(SST::ComponentId_t id, Params& params) :
	EmberMessagePassingGenerator(id, params, "DetailedRing"),
    m_loopIndex(-1), m_computeFunc( NULL ), m_computeTime(0)
{
	m_messageSize = params.find<uint32_t>("arg.messagesize", 1024);
	m_iterations = params.find<int32_t>("arg.iterations", 1);
	m_stream_n = params.find<int32_t>("arg.stream_n", 1000);
	m_printRank = params.find<int32_t>("arg.printRank", 0);

	m_rankList = params.find<std::string>("arg.detailedCompute","");
	m_computeTime = params.find<int32_t>("arg.computeTime", 0);
	m_computeWindow = params.find<int32_t>("arg.computeWindow", m_computeTime);
}

std::string EmberDetailedRingGenerator::getComputeModelName()
{
	return "thornhill.SingleThread";
}

inline long mod( long a, long b )
{
    long tmp = ((a % b) + b) % b;
    return tmp;
}

bool EmberDetailedRingGenerator::generate( std::queue<EmberEvent*>& evQ)
{
   if ( m_loopIndex == m_iterations ) {
        if ( m_printRank == rank() || -1 == m_printRank ) {
            double totalTime = (double)(m_stopTime - m_startTime)/1000000000.0;

            double latency = ((totalTime/m_iterations)/size());
            double bandwidth = (double) m_messageSize / latency;

            output("%s rank %d, total time %.3f us, loop %d, bufLen %d"
                    ", latency %.3f us. bandwidth %f GB/s\n",
                                getMotifName().c_str(), rank(),
                                totalTime * 1000000.0, m_iterations,
                                m_messageSize,
                                latency * 1000000.0,
                                bandwidth / 1000000000.0 );

            if (m_computeFunc) {
                double computeTime = (double)(m_stopCompute - m_startCompute)/1000000000.0;

                output("%s rank %d, `%s` compute %.3f us time per iteration\n", getMotifName().c_str(), rank(),
                    m_computeFunc == &EmberDetailedRingGenerator::computeSimple  ? "Simple": "Detailed" ,
                    computeTime * 1000000.0 );
            }
        }
        return true;
    }

    if ( -1 == m_loopIndex ) {
		++m_loopIndex;
		enQ_memAlloc( evQ, &m_sendBuf, m_messageSize );
		enQ_memAlloc( evQ, &m_recvBuf, m_messageSize );
		if ( ! m_rankList.empty() && findNum( rank(), m_rankList ) ) {
			printf("Rank %d using detailed compute\n",rank());
			m_computeFunc = &EmberDetailedRingGenerator::computeDetailed;
			enQ_memAlloc( evQ, &m_streamBuf, m_stream_n * 8 * 3);
		} else {
			m_computeFunc = &EmberDetailedRingGenerator::computeSimple;
		}
		return false;
	}

    int to = mod( rank() + 1, size());
    int from = mod( (signed int) rank() - 1, size() );
    verbose( CALL_INFO, 2, 0, "to=%d from=%d\n",to,from);

    if ( 0 == m_loopIndex ) {
        verbose( CALL_INFO, 1, 0, "rank=%d size=%d\n", rank(), size());

		verbose( CALL_INFO, 2, 0, "sendbuff=%" PRIx64 "\n",m_sendBuf.getSimVAddr());
		verbose( CALL_INFO, 2, 0, "recvbuff=%" PRIx64 "\n",m_recvBuf.getSimVAddr());
		verbose( CALL_INFO, 2, 0, "streambuff=%" PRIx64 "\n",m_streamBuf.getSimVAddr());

        if ( m_printRank == rank() || -1 == m_printRank ) {
            if ( m_computeTime ) {
                output("%s rank %d, 'Simple' computeTime=%" PRIu64" computeWindow=%" PRIu64 "\n",
                                getMotifName().c_str(),rank(),m_computeTime,m_computeWindow);
            }
        }
        enQ_getTime( evQ, &m_startTime );
    }

    if ( 0 == rank() ) {
        enQ_isend( evQ, m_sendBuf, m_messageSize, CHAR, to, TAG,
                                                GroupWorld, &m_req[0] );
	    enQ_irecv( evQ, m_recvBuf, m_messageSize, CHAR, from, TAG,
                                                GroupWorld, &m_req[1] );
    } else {
	    enQ_irecv( evQ, m_recvBuf, m_messageSize, CHAR, from, TAG,
                                                GroupWorld, &m_req[0] );
	    enQ_isend( evQ, m_sendBuf, m_messageSize, CHAR, to, TAG,
                                                GroupWorld, &m_req[1] );
    }

    if (m_computeFunc ) {
        enQ_getTime( evQ, &m_startCompute );
	    (this->*m_computeFunc)(evQ);
        enQ_getTime( evQ, &m_stopCompute );
    }

	enQ_wait( evQ, &m_req[0] );
	enQ_wait( evQ, &m_req[1] );

    if ( ++m_loopIndex == m_iterations ) {
        enQ_getTime( evQ, &m_stopTime );
    }
    return false;
}

void EmberDetailedRingGenerator::computeSimple( std::queue<EmberEvent*>& evQ)
{
    verbose( CALL_INFO, 1, 0, "\n");
    while ( m_computeTime ) {
        int64_t x = m_computeTime > m_computeWindow ? m_computeWindow : m_computeTime;
    	enQ_compute( evQ, x );
		enQ_makeProgress(evQ);
        m_computeTime -= x;
    }
}

void EmberDetailedRingGenerator::computeDetailed( std::queue<EmberEvent*>& evQ)
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

	for ( int i = 0; i < 10; i++ ) {
    	enQ_detailedCompute( evQ, motif, params );
		enQ_makeProgress(evQ);
	}
}

bool EmberDetailedRingGenerator::findNum( int num, std::string list ) {

    if ( 0 == list.compare( "all" ) ) {
        return true;
    }

    if ( list.empty() ) {
        return false;
    }

    size_t pos = 0;
    size_t end = 0;
    do {
        end = list.find( ",", pos );
        if ( end == std::string::npos ) {
            end = list.length();
        }
        std::string tmp = list.substr(pos,end-pos);

        if ( tmp.length() == 1 ) {
            int val = atoi( tmp.c_str() );
            if ( num == val ) {
                return true;
            }
        } else {
            size_t dash = tmp.find( "-" );
            int first = atoi(tmp.substr(0,dash).c_str()) ;
            int last = atoi(tmp.substr(dash+1).c_str());
            if ( num >= first && num <= last ) {
                return true;
            }
        }

        pos = end + 1;
    } while ( end < list.length() );

    return false;
}

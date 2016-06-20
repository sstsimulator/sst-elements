// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include "emberdetailedring.h"

using namespace SST::Ember;

#define TAG 0xDEADBEEF

EmberDetailedRingGenerator::EmberDetailedRingGenerator(SST::Component* owner, Params& params) :
	EmberMessagePassingGenerator(owner, params, "DetailedRing"),
    m_loopIndex(-1)
{
	m_messageSize = params.find<uint32_t>("arg.messagesize", 1024);
	m_iterations = params.find<int32_t>("arg.iterations", 1);
	m_stream_n = params.find<int32_t>("arg.stream_n", 1000);
	m_printRank = params.find<int32_t>("arg.printRank", 0);
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

            output("%s total time %.3f us, loop %d, bufLen %d"
                    ", latency %.3f us. bandwidth %f GB/s\n",
                                getMotifName().c_str(),
                                totalTime * 1000000.0, m_iterations,
                                m_messageSize,
                                latency * 1000000.0,
                                bandwidth / 1000000000.0 );

            double computeTime = (double)(m_stopCompute - m_startCompute)/1000000000.0;

            output("%s total compute %.3f us\n", getMotifName().c_str(),
                                computeTime * 1000000.0 );
        }
        return true;
    }

    if ( -1 == m_loopIndex ) {
        verbose( CALL_INFO, 1, 0, "rank=%d size=%d\n", rank(), size());
		enQ_memAlloc( evQ, &m_sendBuf, m_messageSize );
		enQ_memAlloc( evQ, &m_recvBuf, m_messageSize );
		enQ_memAlloc( evQ, &m_streamBuf, m_stream_n * 8 * 3);
		++m_loopIndex;
		return false;
	}


	verbose( CALL_INFO, 2, 1, "sendbuff=%" PRIx64 "\n",m_sendBuf.simVAddr);
	verbose( CALL_INFO, 2, 1, "recvbuff=%" PRIx64 "\n",m_recvBuf.simVAddr);
	verbose( CALL_INFO, 2, 1, "streambuff=%" PRIx64 "\n",m_streamBuf.simVAddr);

    int to = mod( rank() + 1, size());
    int from = mod( (signed int) rank() - 1, size() );
    verbose( CALL_INFO, 2, 0, "to=%d from=%d\n",to,from);

    if ( 0 == m_loopIndex ) {
        verbose( CALL_INFO, 1, 0, "rank=%d size=%d\n", rank(), size());

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

    if ( haveDetailed() ) {
        verbose( CALL_INFO, 1, 0, "\n");
        Params params;

        std::string motif;

#if 0
        motif = "miranda.CopyGenerator";
        params.insert("read_start_address", "0",true);
        params.insert("request_width", "16",true);
        params.insert("request_count", "65536",true);
        enQ_detailedCompute( evQ, motif, params );
#endif

#if 0
        motif = "miranda.SingleStreamGenerator";
        params.insert("startat", "3",true);
        params.insert("count", "500000",true);
        params.insert("max_address", "512000",true);
#endif
		std::stringstream tmp;	

        motif = "miranda.STREAMBenchGenerator";

		tmp.str( std::string() ); tmp.clear();
		tmp << m_stream_n;
        params.insert("n", tmp.str() );

		tmp.str( std::string() ); tmp.clear();
		tmp << m_streamBuf.simVAddr;
        params.insert("start_a", tmp.str() );

        params.insert("operandwidth", "8",true);

        params.insert( "generatorParams.verbose", "1" );
        params.insert( "verbose", "1" );

        enQ_getTime( evQ, &m_startCompute );
        enQ_detailedCompute( evQ, motif, params );
        enQ_getTime( evQ, &m_stopCompute );
    }

	enQ_wait( evQ, &m_req[0] );
	enQ_wait( evQ, &m_req[1] );

    if ( ++m_loopIndex == m_iterations ) {
        enQ_getTime( evQ, &m_stopTime );
    }
    return false;
}

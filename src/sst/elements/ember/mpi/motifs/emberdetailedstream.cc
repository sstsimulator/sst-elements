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


#include <sst_config.h>
#include "emberdetailedstream.h"

using namespace SST::Ember;

EmberDetailedStreamGenerator::EmberDetailedStreamGenerator(SST::Component* owner, Params& params) :
	EmberMessagePassingGenerator(owner, params, "DetailedStream"),
    m_loopIndex(-1), m_numLoops(2), m_startTime(2), m_stopTime(2)
{
	m_benchSize = {  2*8, 3*8 };
	m_benchName = { "Copy", "Triad" };
	m_stream_n = params.find<int32_t>("arg.stream_n", 1024);
	m_n_per_call_copy = params.find<int32_t>("arg.n_per_call_copy", 1000);
	m_n_per_call_triad = params.find<int32_t>("arg.n_per_call_triad", 1000);
	m_operandwidth = params.find<int32_t>("arg.operandwidth", 8);
	m_numLoops = params.find<int32_t>("arg.n_loops", 2);

	assert( ! ( m_stream_n % ( m_operandwidth / 8 ) ) );
}

std::string EmberDetailedStreamGenerator::getComputeModelName()
{
	return "thornhill.SingleThread";
}

void EmberDetailedStreamGenerator::print( ) 
{
	output("%s, Array Size %d, Total Memory Required %.3f MB\n", 
					getMotifName().c_str(), 
					m_stream_n,
					(m_stream_n * 3 * 8) / 1048576.0
					);	
	for ( unsigned i = 0; i < m_benchName.size(); i++ ) {
		double computeTime = (double)(m_stopTime[i] - m_startTime[i]);

 		output("%s, %d, %s %f MB/s, Time=%.3f us\n",
					getMotifName().c_str(), 
					m_stream_n,
					m_benchName[i].c_str(), 
					(double) (m_benchSize[i]* m_stream_n)/computeTime * 1000.0 ,
					computeTime / 1000.0 );
	}
}

bool EmberDetailedStreamGenerator::generate( std::queue<EmberEvent*>& evQ) 
{
	if ( m_loopIndex == m_numLoops ) {
		print( );
        return true;
    }

    if ( -1 == m_loopIndex ) {
		enQ_memAlloc( evQ, &m_streamBuf, m_stream_n * 8 * 3);
		++m_loopIndex;
		return false;
	}

	verbose( CALL_INFO, 2, 1, "streambuff=%" PRIx64 "\n",m_streamBuf.getSimVAddr());

    enQ_getTime( evQ, &m_startTime[0] );
    Simulation::getSimulation()->getStatisticsProcessingEngine()->performGlobalStatisticOutput();
    computeDetailedCopy( evQ ); 
    enQ_getTime( evQ, &m_stopTime[0] );

    enQ_getTime( evQ, &m_startTime[1] );
    Simulation::getSimulation()->getStatisticsProcessingEngine()->performGlobalStatisticOutput();
  	computeDetailedTriad( evQ ); 
    enQ_getTime( evQ, &m_stopTime[1] );

	++m_loopIndex;

    return false;
}

void EmberDetailedStreamGenerator::computeDetailedCopy( std::queue<EmberEvent*>& evQ) 
{
    verbose( CALL_INFO, 1, 0, "\n");

	Params params;

    std::string motif;

	std::stringstream tmp;	

    motif = "miranda.CopyGenerator";

	tmp.str( std::string() ); tmp.clear();
	tmp << m_stream_n/(m_operandwidth/8);
    params.insert("request_count", tmp.str() );

	tmp.str( std::string() ); tmp.clear();
	tmp << m_streamBuf.getSimVAddr();
    params.insert("read_start_address", tmp.str() );

	tmp.str( std::string() ); tmp.clear();
	tmp << m_operandwidth;
    params.insert("operandwidth", tmp.str(),true);

	tmp.str( std::string() ); tmp.clear();
	tmp << m_n_per_call_copy;
    params.insert("n_per_call", tmp.str());

    params.insert( "generatorParams.verbose", "1" );
    params.insert( "verbose", "1" );

  	enQ_detailedCompute( evQ, motif, params );
}
void EmberDetailedStreamGenerator::computeDetailedTriad( std::queue<EmberEvent*>& evQ) 
{
    verbose( CALL_INFO, 1, 0, "\n");

	Params params;

    std::string motif;

	std::stringstream tmp;	

    motif = "miranda.STREAMBenchGenerator";

	tmp.str( std::string() ); tmp.clear();
	tmp << m_stream_n/(m_operandwidth/8);
    params.insert("n", tmp.str() );

	tmp.str( std::string() ); tmp.clear();
	tmp << m_streamBuf.getSimVAddr();
    params.insert("start_a", tmp.str() );

	tmp.str( std::string() ); tmp.clear();
	tmp << m_operandwidth;
    params.insert("operandwidth", tmp.str(),true);

	tmp.str( std::string() ); tmp.clear();
	tmp << m_n_per_call_triad;
    params.insert("n_per_call", tmp.str());

    params.insert( "generatorParams.verbose", "1" );
    params.insert( "verbose", "1" );

  	enQ_detailedCompute( evQ, motif, params );
}


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
#include "emberalltoall.h"

using namespace SST::Ember;

EmberAlltoallGenerator::EmberAlltoallGenerator(SST::ComponentId_t id,
                                            Params& params) :
	EmberMessagePassingGenerator(id, params, "Alltoall"),
    m_loopIndex(0)
{
	m_iterations = (uint32_t) params.find("arg.iterations", 1);
	m_compute    = (uint32_t) params.find("arg.compute", 0);
	m_bytes      = (uint32_t) params.find("arg.bytes", 1);
    jobId        = (int) params.find<int>("_jobId"); //NetworkSim
    m_sendBuf = NULL;
    m_recvBuf = NULL;
}

bool EmberAlltoallGenerator::generate( std::queue<EmberEvent*>& evQ) {

    if ( m_loopIndex == m_iterations ) {
        if ( 0 == rank() ) {
			double latency = (double)(m_stopTime-m_startTime)/(double)m_iterations;
			latency /= 1000000000.0;
            output( "%s: ranks %d, loop %d, bytes %d, latency %.3f us\n",
					getMotifName().c_str(), size(), m_iterations, m_bytes, latency * 1000000.0  );
            /*
            //NetworkSim: report total running time
            double latency_us = (double)(m_stopTime-m_startTime)/1000.0;
            double latency_per_iter = latency_us/(double)m_iterations;
            output("Motif Latency: JobNum:%d Total latency:%.3f us\n", jobId, latency_us );
            output("Motif Latency: JobNum:%d Per iteration latency:%.3f us\n", jobId, latency_per_iter );
            output("Job Finished: JobNum:%d Time:%" PRIu64 " us\n", jobId,  getCurrentSimTimeMicro());
            */
        }
        return true;
    }
    if ( 0 == m_loopIndex ) {
        enQ_getTime( evQ, &m_startTime );
    }

    enQ_compute( evQ, m_compute );
    enQ_alltoall( evQ, m_sendBuf, m_bytes, CHAR, m_recvBuf, m_bytes, CHAR, GroupWorld );

    if ( ++m_loopIndex == m_iterations ) {
        enQ_getTime( evQ, &m_stopTime );
    }
    return false;
}

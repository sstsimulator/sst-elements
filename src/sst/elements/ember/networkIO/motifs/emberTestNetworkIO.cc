// SPDX-FileCopyrightText: Copyright Hewlett Packard Enterprise Development LP
// SPDX-License-Identifier: BSD-3-Clause

#include <sst_config.h>
#include "emberTestNetworkIO.h"
#include <iostream>

using namespace SST::Ember;

EmberTestNetworkIOGenerator::EmberTestNetworkIOGenerator(SST::ComponentId_t id, Params& params) 
    : EmberNetworkIOGenerator(id, params, "TestNetworkIO"), m_phase(0)
{
        m_messageSize = params.find<uint32_t>("arg.messageSize", 1024);
        m_iterations = params.find<uint32_t>("arg.iterations", 5);
        m_opType = params.find<std::string>("arg.op", "write");
        m_fileSize = params.find<uint64_t>("arg.fileSize", 10485760);  // 10MB default
        
        m_rng = new SST::RNG::MarsagliaRNG();
        m_startTime = 0;
        m_stopTime = 0;
}

bool EmberTestNetworkIOGenerator::generate( std::queue<EmberEvent*>& evQ) 
{
    bool ret = false;
    switch(m_phase) 
    {
        case 0:
            memSetNotBacked();
            m_localBuffer = memAlloc(m_messageSize);
            enQ_getTime(evQ, &m_startTime);
            for (uint32_t i = 0; i < m_iterations; i++) 
            {
                uint64_t offset = m_rng->generateNextUInt64() % m_fileSize;
                
                if (m_opType == "read") 
                    networkIO().networkIORead(evQ, m_localBuffer, offset, m_messageSize);
                else 
                    networkIO().networkIOWrite(evQ, offset, m_localBuffer, m_messageSize);
            }
            enQ_getTime(evQ, &m_stopTime);
            break;
            
        case 1:
            double totalTime = (double)(m_stopTime - m_startTime)/1000000000.0;
            double latency = (totalTime/m_iterations);
            output("message-size %u, iterations %u, total-time %.3lf us\n",
                        m_messageSize, m_iterations, totalTime * 1000000.0) ;
            ret = true;
            break;
    }
    
    ++m_phase;
    return ret;
}


// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include "emberTrafficGen.h"
#include <limits>
#include <cstdlib>

using namespace SST;
using namespace SST::Ember;

#define TAG 0xDEADBEEF

EmberTrafficGenGenerator::EmberTrafficGenGenerator(SST::ComponentId_t id,
                                                    Params& params) :
    EmberMessagePassingGenerator(id, params, "TrafficGen"),
    m_generateLoopIndex(0), m_needToWait(false), m_currentTime(0), m_stopped(false), m_numStopped(0),
    m_rankBytes(0), m_totalBytes(0), m_requestIndex(-1), m_allRequests(nullptr),
    m_dataSendActive(false), m_dataRecvActive(false), m_dataRecvRequest(nullptr), m_dataSendRequest(nullptr)
{
    m_pattern = params.find<std::string>("arg.pattern", "plusOne");

    m_messageSize = (uint32_t) params.find("arg.messageSize", 1024);
    m_maxMessageSize = (uint32_t) params.find("arg.maxMessageSize", pow(2,20));

    if (m_pattern == "plusOne") {
        m_sendBuf = memAlloc(m_messageSize);
        m_recvBuf = memAlloc(m_messageSize);
    }
    else {
        m_sendBuf = memAlloc(m_maxMessageSize);
        m_recvBuf = memAlloc(m_maxMessageSize);
    }

    if (m_pattern == "plusOne") {
        m_mean = params.find("arg.mean", 5000.0);
        m_stddev = params.find("arg.stddev", 300.0 );
        m_startDelay = params.find("arg.startDelay", .0 );
    }
    else {
        m_debug = params.find<int>("arg.debugLevel",0);
        m_meanMessageSize = params.find("arg.messageSizeMean", 1024);
        m_stddevMessageSize = params.find("arg.messageSizeStdDev", 1024);
        m_computeDelay = params.find("arg.computeDelay", 100);
        m_hotSpots = params.find<unsigned int>("arg.hotSpots",0);
        m_hotSpotsRatio = params.find<unsigned int>("arg.hotSpotsRatio",99);
        m_iterations = params.find<unsigned int>("arg.stopIterations", std::numeric_limits<unsigned int>::max());
        m_stopTime = params.find<uint64_t>("arg.stopTimeUs", std::numeric_limits<uint64_t>::max());
        if (m_iterations != std::numeric_limits<unsigned int>::max() && m_stopTime != std::numeric_limits<uint64_t>::max()) {
            std::cerr << "Please set either stopIterations or stopTimeUs, not both\n";
            abort();
        }
        if (m_stopTime != std::numeric_limits<uint64_t>::max()) m_stopTime *= 1000;
        if (m_iterations == std::numeric_limits<unsigned int>::max() && m_stopTime == std::numeric_limits<uint64_t>::max()) {
            m_iterations = 1000;
        }
    }

    configure();
}

void EmberTrafficGenGenerator::configure()
{
    if (m_pattern == "plusOne") {
        configure_plusOne();
        return;
    }

    m_rank = rank();

    m_distMessageSize = new SSTGaussianDistribution(
                m_meanMessageSize, m_stddevMessageSize, new RNG::MarsagliaRNG( 11 + rank(), RAND_MAX / (rank() + 1) ) );

    std::srand(1); // want same hotRanks on every node
    if (m_hotSpots) {
        while (m_hotRanks_set.size() < m_hotSpots) {
            uint32_t rank = std::rand() % size();
            m_hotRanks_set.insert( rank );
        }
        std::copy(m_hotRanks_set.begin(), m_hotRanks_set.end(), std::back_inserter(m_hotRanks));

        m_hotCounterInitial = m_hotSpotsRatio;
        m_hotCounter = m_hotCounterInitial;
    }

    memSetBacked();
    m_rankBytes = memAlloc(sizeofDataType(UINT64_T));
    uint64_t* value_ptr = (uint64_t*) m_rankBytes.getBacking();
    *value_ptr = 0;
    m_totalBytes = memAlloc(sizeofDataType(UINT64_T));
    value_ptr = (uint64_t*) m_totalBytes.getBacking();
    *value_ptr = 0;
}

void EmberTrafficGenGenerator::configure_plusOne()
{
    assert( 2 == size() );

    m_random = new SSTGaussianDistribution( m_mean, m_stddev,
                        //new RNG::MarsagliaRNG( 11 + rank(), 79  ) );
                        new RNG::MarsagliaRNG( 11 + rank(), getJobId()  ) );

    if ( 0 == rank() ) {
        verbose(CALL_INFO, 1, 0, "startDelay %.3f ns\n",m_startDelay);
        verbose(CALL_INFO, 1, 0, "compute time: mean %.3f ns,"
        " stdDev %.3f ns\n", m_random->getMean(), m_random->getStandardDev());
        verbose(CALL_INFO, 1, 0, "messageSize %d\n", m_messageSize);
    }
}

bool EmberTrafficGenGenerator::generate( std::queue<EmberEvent*>& evQ)
{
    if (m_pattern == "plusOne") return generate_plusOne(evQ);
    return generate_random(evQ);
}

bool EmberTrafficGenGenerator::generate_plusOne( std::queue<EmberEvent*>& evQ)
{
    double computeTime = m_random->getNextDouble();

    if ( computeTime < 0 ) {
        computeTime = 0.0;
    }
    verbose(CALL_INFO, 1, 0, "computeTime=%.3f ns\n", computeTime );
    enQ_compute( evQ, (computeTime + m_startDelay) * 1000 );
    m_startDelay = 0;

    int other = (rank() + 1) % 2;
    enQ_irecv( evQ, m_recvBuf, m_messageSize, CHAR, other, TAG,
                                                GroupWorld, &m_req );
    enQ_send( evQ, m_sendBuf, m_messageSize, CHAR, other, TAG, GroupWorld );
    enQ_wait( evQ, &m_req );

    return false;
}

bool EmberTrafficGenGenerator::generate_random( std::queue<EmberEvent*>& evQ)
{
    evQ_ = &evQ;
    m_currentTime = getCurrentSimTimeNano();

    if(m_stopped) {
        if (m_rank == 0) {
            if (m_iterations != std::numeric_limits<unsigned int>::max())
                m_stopTime = m_currentTime;
            uint64_t bytes = m_totalBytes.at<uint64_t>(0);
            std::cerr << "Total observed bandwidth: " << (double) bytes / (double) m_stopTime  << " GB/s\n";
        }
        enQ_cancel( evQ, *m_dataRecvRequest);
        return true;
    }

    if (m_debug > 2) std::cerr << "rank " << m_rank << " entering loop " << m_generateLoopIndex
                               << " (t=" << m_currentTime / 1000.0 <<"us)" << std::endl;

    if (m_generateLoopIndex == 0) {
        enQ_getTime( evQ, &m_startTime );
        //post receives to handle termination
        if (m_rank != 0) recv_allstopped();
        else recv_stopping();
        // post receive for data notifies
        recv_data();
        send_data();
        m_needToWait = true;
        ++m_generateLoopIndex;
        return false;
    }

    if (m_needToWait) {
        if (m_debug > 2) std::cerr << "rank " << m_rank << " waiting for any request\n";
        wait_for_any();
        m_needToWait = false;
        ++m_generateLoopIndex;
        return false;
    }

    // setup request indexes
    int data_send_index = -1;
    int data_recv_index = -1;
    int stopping_index = 0;
    std::string reqType[3];
    if (m_dataRecvActive) { data_recv_index = 0; reqType[data_recv_index] = "data receive"; }
    if (m_dataSendActive) { data_send_index = data_recv_index + 1; reqType[data_send_index] = "data send"; }
    if (data_send_index >= 0) stopping_index = data_send_index + 1;
    else if (data_recv_index >= 0) stopping_index = data_recv_index + 1;
    reqType[stopping_index] = "stopping";
    if (m_debug > 2) std::cerr << "rank " << m_rank << " got completed request: " << reqType[m_requestIndex] << std::endl;

    // Time to stop?
    // All nonzero ranks send STOPPING messages to rank zero when they meet their stopping criteria
    // Once zero has received all expected STOPPING messages and stopped itself, it sends ALLSTOPPED messages to all ranks
    // Upon ALLSTOPPED we reduce the total bytes sent/received, report results, and end the motif.
    if (m_requestIndex == stopping_index) {
        m_requestIndex = -1;
        if (m_rank == 0) {
            if (m_numStopped < size() - 1) ++m_numStopped;
            if (m_debug > 0) std::cerr << "rank " << m_rank << " received stop message " << m_numStopped
                                       << " from " << m_anyResponse.src << std::endl;
            if (!check_stop()) {
                recv_stopping();
                m_needToWait = true;
            }
        }
        else {
            if (m_debug > 1) std::cerr << "rank " << m_rank << " stopping with bytes " << m_rankBytes.at<uint64_t>(0) << std::endl;
            m_stopped = true;
            enQ_reduce( evQ, m_rankBytes, m_totalBytes, 1, UINT64_T, Hermes::MP::SUM, 0, GroupWorld );
        }
        return false;
    }

    if (m_requestIndex == data_recv_index) {
        m_requestIndex = -1;
        m_dataRecvActive = false;
        if (m_debug > 0) std::cerr << "rank " << m_rank << " received " << m_anyResponse.count << " from " << m_anyResponse.src << std::endl;
        m_currentTime = getCurrentSimTimeNano();
        if (m_currentTime < m_stopTime) {
            m_dataSize = m_anyResponse.count;
            if (m_debug > 2) std::cerr << "rank " << m_rank << " accumulating " << m_dataSize << " bytes at time " << m_currentTime << std::endl;
            uint64_t* bytes = (uint64_t*) m_rankBytes.getBacking();
            *bytes += m_dataSize;
            uint64_t delay = m_dataSize * m_computeDelay;
            if (m_debug > 0) std::cerr << "rank " << m_rank <<  " computing for " << delay << std::endl;
            if (m_currentIteration <= m_iterations) {
                enQ_compute( evQ, delay);
                if (m_debug > 0) std::cerr << "rank " << m_rank <<  " computing for " << delay << std::endl;
            }
        }
        recv_data();
    }
    else if (m_requestIndex == data_send_index) {
        m_requestIndex = -1;
        m_dataSendActive = false;
        if (m_currentTime < m_stopTime && m_currentIteration <= m_iterations) {
            ++m_currentIteration;
            send_data();
        }
        else if (m_rank != 0) {
            if (m_debug > 0) std::cerr << "rank " << m_rank << " stopping\n";
            enQ_send(evQ, nullptr, 1, CHAR, 0, STOPPING, GroupWorld);
        }
        else {
            ++m_generateLoopIndex;
            if (!check_stop())
                wait_for_any();
            return false;
        }
    }

    m_needToWait = true;
    ++m_generateLoopIndex;
    return false;
}

void EmberTrafficGenGenerator::recv_data() {
    std::queue<EmberEvent*>& evQ = *evQ_;
    if (m_debug > 2) std::cerr << "rank " << m_rank << " start a datareq recv\n";
    if (m_dataRecvRequest) delete m_dataRecvRequest;
    m_dataRecvRequest = new MessageRequest;
    enQ_irecv( evQ, m_recvBuf, m_maxMessageSize, UINT64_T, Hermes::MP::AnySrc, DATA, GroupWorld, m_dataRecvRequest );
    m_dataRecvActive = true;
}

void EmberTrafficGenGenerator::recv_stopping() {
    std::queue<EmberEvent*>& evQ = *evQ_;
    enQ_irecv( evQ, nullptr, 1, CHAR, Hermes::MP::AnySrc, STOPPING, GroupWorld, &m_stopRequest);
}

void EmberTrafficGenGenerator::recv_allstopped() {
    std::queue<EmberEvent*>& evQ = *evQ_;
    enQ_irecv( evQ, nullptr, 1, CHAR, 0, ALLSTOPPED, GroupWorld, &m_stopRequest);
}

void EmberTrafficGenGenerator::send_data() {
    std::queue<EmberEvent*>& evQ = *evQ_;

    // determine rank to send data to
    uint32_t partner = (uint32_t) m_rank;
    if (m_hotSpots && !m_hotCounter && !m_hotRanks_set.count(m_rank) ) {
        partner = m_hotRanks[std::rand() % m_hotSpots];
    }
    else {
        while (partner == m_rank)
            partner = std::rand() % size();
    }
    if (m_hotSpots) {
        if (m_hotCounter) --m_hotCounter;
        else m_hotCounter = m_hotCounterInitial;
    }

    // determine size of data
    m_dataSize = int( abs( m_distMessageSize->getNextDouble() ) );
    if (m_dataSize < 1) m_dataSize = 1;
    if (m_dataSize > m_maxMessageSize) m_dataSize = m_maxMessageSize;

    if (m_debug > 1) std::cerr << "rank " << m_rank << " sending data (size=" << m_dataSize << ") to rank " << partner << std::endl;
    if (m_dataSendRequest) delete m_dataSendRequest;
    m_dataSendRequest = new MessageRequest;
    enQ_isend( evQ, m_sendBuf, m_dataSize, UINT64_T, partner, DATA, GroupWorld, m_dataSendRequest);
    m_dataSendActive = true;
}

void EmberTrafficGenGenerator::wait_for_any() {
    std::queue<EmberEvent*>& evQ = *evQ_;
    uint64_t size = m_dataSendActive + m_dataRecvActive + 1;
    if (m_debug > 2) std::cerr << "rank " << m_rank <<  " enqueing waitany with size " << size << std::endl;
    if (m_allRequests) delete m_allRequests;
    m_allRequests = new MessageRequest[size];
    uint64_t index=0;
    if (m_dataRecvActive) {
        if (m_debug > 3) std::cerr << "copy dataRecvRequest " << m_dataRecvRequest << " to " << index << std::endl;
        m_allRequests[index] = *m_dataRecvRequest;
        ++index;
    }
    if (m_dataSendActive) {
        if (m_debug > 3) std::cerr << "copy dataSendRequest " << m_dataSendRequest << " to " << index << std::endl;
        m_allRequests[index] = *m_dataSendRequest;
        ++index;
    }
    m_allRequests[index] = m_stopRequest;
    enQ_waitany(evQ, size, m_allRequests, &m_requestIndex, &m_anyResponse);
}

bool EmberTrafficGenGenerator::check_stop() {
    std::queue<EmberEvent*>& evQ = *evQ_;
    if (m_numStopped == size() - 1 && (m_currentTime >= m_stopTime || m_currentIteration > m_iterations)){
        if (m_debug > 1) std::cerr << "rank " << m_rank << " all ranks complete, stopping with bytes " << m_rankBytes.at<uint64_t>(0) << std::endl;
        m_stopped = true;
        m_needToWait = false;
        m_stopTimeActual = getCurrentSimTimeNano();
        for (int i=1; i < size(); ++i) {
            enQ_send(evQ, m_allStopped, 1, CHAR, i, ALLSTOPPED, GroupWorld);
        }
        enQ_reduce( evQ, m_rankBytes, m_totalBytes, 1, UINT64_T, Hermes::MP::SUM, 0, GroupWorld );
        return true;
    }
    else {
        if (m_debug > 1) std::cerr << "rank " << m_rank << " not stopping yet\n";
    }
    return false;
}

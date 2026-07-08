// Copyright 2013-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2026, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include <cinttypes>

#include "simpleSSD.h"

using namespace SST;
using namespace Firefly;

SimpleSSD::SimpleSSD(ComponentId_t id, Params &params)
    : SimpleSSDAPI(id), m_pendingRequests(0) 
{
    int nSSDsPerNode = params.find<int>("nSSDsPerNode", 1);
    int nQueuesPerSSD = params.find<int>("queuesCountPerSSD", 4);
    int totalLanes = nSSDsPerNode * nQueuesPerSSD;
    m_bus.lanes.resize(totalLanes);
    m_laneFreeAtCycle.assign(totalLanes, 0);

    m_readOverheadLatency_ns = params.find<int64_t>("readOverheadLatency_ns", 500);
    m_writeOverheadLatency_ns = params.find<int64_t>("writeOverheadLatency_ns", 500);

    double readBandwidthPerSSD = params.find<double>("readBandwidthPerSSD_GBps", 6.25);
    m_readBandwidthPerQueue_GBps = readBandwidthPerSSD / nQueuesPerSSD;

    double writeBandwidthPerSSD = params.find<double>("writeBandwidthPerSSD_GBps", 6.25);
    m_writeBandwidthPerQueue_GBps = writeBandwidthPerSSD / nQueuesPerSSD;

    int verboseLevel = params.find<int>("verboseLevel", 0);
    int verboseMask = params.find<int>("verboseMask", -1);
    m_out.init("[SimpleSSD] ", verboseLevel, verboseMask, Output::STDOUT);
    registerClock("1GHz", new Clock::Handler2<SimpleSSD, &SimpleSSD::clockTick>(this));
    m_selfLink = configureSelfLink("ReadWriteLatency", "1 ns", new Event::Handler2<SimpleSSD, &SimpleSSD::handleEvent>(this));

    m_statReadRequests        = registerStatistic<uint64_t>("readRequests");
    m_statWriteRequests       = registerStatistic<uint64_t>("writeRequests");
    m_statReadBytes           = registerStatistic<uint64_t>("readBytes");
    m_statWriteBytes          = registerStatistic<uint64_t>("writeBytes");
    m_statReadLatency         = registerStatistic<uint64_t>("readLatency_ns");
    m_statWriteLatency        = registerStatistic<uint64_t>("writeLatency_ns");
    m_statQueueDepthOnEnqueue = registerStatistic<uint64_t>("queueDepthOnEnqueue");
    m_statPendingOnDispatch   = registerStatistic<uint64_t>("pendingOnDispatch");
}

void SimpleSSD::finish()
{
    m_out.verbose(CALL_INFO, 0, 0,
        "summary: reads=%" PRIu64 " (%" PRIu64 " B) writes=%" PRIu64 " (%" PRIu64 " B) inflightAtFinish=%" PRId64 "\n",
        m_totalReadRequests, m_totalReadBytes, m_totalWriteRequests, m_totalWriteBytes, m_pendingRequests);
}

void SimpleSSD::read(int64_t offset, size_t bytes, const SsdReqCallback &callback)
{
    m_statReadRequests->addData(1);
    m_statReadBytes->addData(static_cast<uint64_t>(bytes));
    ++m_totalReadRequests;
    m_totalReadBytes += bytes;
    int64_t delay_ns = calcDelay_ns(bytes, m_readBandwidthPerQueue_GBps, m_readOverheadLatency_ns);
    m_statReadLatency->addData(static_cast<uint64_t>(delay_ns));
    enqueueRequest(offset, bytes, m_readBandwidthPerQueue_GBps, m_readOverheadLatency_ns, callback);
}

void SimpleSSD::write(int64_t offset, size_t bytes, const SsdReqCallback &callback)
{
    m_statWriteRequests->addData(1);
    m_statWriteBytes->addData(static_cast<uint64_t>(bytes));
    ++m_totalWriteRequests;
    m_totalWriteBytes += bytes;
    int64_t delay_ns = calcDelay_ns(bytes, m_writeBandwidthPerQueue_GBps, m_writeOverheadLatency_ns);
    m_statWriteLatency->addData(static_cast<uint64_t>(delay_ns));
    enqueueRequest(offset, bytes, m_writeBandwidthPerQueue_GBps, m_writeOverheadLatency_ns, callback);
}

void SimpleSSD::handleEvent(SST::Event *ev)
{
    DelayEvent *event = dynamic_cast<DelayEvent *>(ev);
    assert(event);
    invokeSsdCb(event->m_cb_id);
    delete ev;
    --m_pendingRequests;
}

bool SimpleSSD::clockTick(SST::Cycle_t n)
{
    int64_t now = static_cast<int64_t>(n);
    for (size_t i = 0; i < m_bus.lanes.size(); ++i)
    {
        if (m_laneFreeAtCycle[i] > now) continue;
        if (m_bus.lanes[i].empty()) continue;

        Request request = m_bus.lanes[i].front();
        m_bus.lanes[i].pop();

        m_statPendingOnDispatch->addData(static_cast<uint64_t>(m_pendingRequests));

        uint32_t cb_id = registerSsdCb(request.callback);
        DelayEvent *ev = new DelayEvent(cb_id);
        m_selfLink->send(request.delay_ns, ev);

        m_laneFreeAtCycle[i] = now + request.delay_ns;
        ++m_pendingRequests;
    }
    return false;
}

int64_t SimpleSSD::calcDelay_ns(const size_t bytes, const double bandwidth_GBps, const int64_t latency_ns)
{
    double delay_ns = latency_ns + ((bytes / bandwidth_GBps));
    return int64_t(delay_ns);
}

void SimpleSSD::enqueueRequest(const int64_t offset, const size_t bytes, const double bandwidth_GBps, const int64_t overheadLatency_ns, const SsdReqCallback& callback)
{
    Request request;
    request.bytes = bytes;
    request.delay_ns = this->calcDelay_ns(bytes, bandwidth_GBps, overheadLatency_ns);
    request.offset = offset;
    request.callback = callback;
    m_bus.currentLane %= m_bus.lanes.size();
    auto &lane = m_bus.lanes.at(m_bus.currentLane);
    lane.push(request);
    m_statQueueDepthOnEnqueue->addData(static_cast<uint64_t>(lane.size()));
    m_bus.currentLane++;
}

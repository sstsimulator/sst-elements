// SPDX-FileCopyrightText: Copyright Hewlett Packard Enterprise Development LP
// SPDX-License-Identifier: BSD-3-Clause

#include "sst_config.h"

#include "simpleSSD.h"

using namespace SST;
using namespace Firefly;

SimpleSSD::SimpleSSD(ComponentId_t id, Params &params)
    : SimpleSSDAPI(id), m_pendingRequests(0) 
{
    int nSSDsPerNode = params.find<int>("nSSDsPerNode", 1);
    int nQueuesPerSSD = params.find<int>("queuesCountPerSSD", 4);
    m_bus.lanes.resize(nQueuesPerSSD);

    m_readOverheadLatency_ns = params.find<int64_t>("readOverheadLatency_ns", 500);
    m_writeOverheadLatency_ns = params.find<int64_t>("writeOverheadLatency_ns", 500);

    double readBandwidthPerSSD = params.find<double>("readBandwidthPerSSD_GBps", 6.25);
    m_readBandwidthPerQueue_GBps = readBandwidthPerSSD*nSSDsPerNode/nQueuesPerSSD;

    double writeBandwidthPerSSD = params.find<double>("writeBandwidthPerSSD_GBps", 6.25);
    m_writeBandwidthPerQueue_GBps = writeBandwidthPerSSD*nSSDsPerNode/nQueuesPerSSD;

    int verboseLevel = params.find<int>("verboseLevel", 0);
    int verboseMask = params.find<int>("verboseMask", -1);
    m_out.init("[SimpleSSD] ", verboseLevel, verboseMask, Output::STDOUT);
    registerClock("1GHz", new Clock::Handler2<SimpleSSD, &SimpleSSD::clockTick>(this));
    m_selfLink = configureSelfLink("ReadWriteLatency", "1 ns", new Event::Handler2<SimpleSSD, &SimpleSSD::handleEvent>(this));
}

void SimpleSSD::read(int64_t offset, size_t bytes, const SsdReqCallback &callback)
{
    enqueueRequest(offset, bytes, m_readBandwidthPerQueue_GBps, m_readOverheadLatency_ns, callback);
}

void SimpleSSD::write(int64_t offset, size_t bytes, const SsdReqCallback &callback)
{
    enqueueRequest(offset, bytes, m_writeBandwidthPerQueue_GBps, m_writeOverheadLatency_ns, callback);
}

void SimpleSSD::handleEvent(SST::Event *ev)
{
    DelayEvent *event = dynamic_cast<DelayEvent *>(ev);
    assert(event);
    event->m_callback();
    delete ev;
    --m_pendingRequests;
}

bool SimpleSSD::clockTick(SST::Cycle_t n)
{
    if (m_pendingRequests == 0)
    {
        for (int i = 0; i < m_bus.lanes.size(); ++i)
        {
            if (!m_bus.lanes.at(i).empty())
            {
                Request request = m_bus.lanes.at(i).front();
                DelayEvent *ev = new DelayEvent(request.callback);
                m_selfLink->send(request.delay_ns, ev);
                m_bus.lanes.at(i).pop();
                ++m_pendingRequests;
            }
        }
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
    m_bus.lanes.at(m_bus.currentLane).push(request);
    m_bus.currentLane++;
}

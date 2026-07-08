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

#pragma once

#include <sst/core/subcomponent.h>
#include <sst/core/link.h>

#include <queue>
#include <vector>
#include <functional>
#include <unordered_map>
#include <cstdint>

using SsdReqCallback = std::function<void(void)>;

namespace SST
{
    namespace Firefly
    {
        struct Request
        {
            int64_t offset;
            size_t bytes;
            int64_t delay_ns;
            SsdReqCallback callback;
        };
        
        struct Bus
        {
            std::vector<std::queue<Request>> lanes;
            int currentLane = 0;
        };
        
        class SimpleSSDAPI : public SST::SubComponent
        {
        public:
            SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Firefly::SimpleSSDAPI)
            
            /// Constructor
            SimpleSSDAPI(ComponentId_t id) : SubComponent(id) {};
            
            /// Destructor            
            virtual ~SimpleSSDAPI() {};
            
            /// SSD read API to read bytes from offset
            virtual void read(int64_t offset, size_t bytes, const SsdReqCallback &callback) = 0;

            /// SSD write API to write bytes to offset
            virtual void write(int64_t offset, size_t bytes, const SsdReqCallback &callback) = 0;

            /// API that models the variation of delay with message size
            virtual int64_t calcDelay_ns(size_t bytes, double bandwidth, int64_t latency) = 0;

        protected:
            /// An instance of SSD bus abstracting connection to NIC
            Bus m_bus;

            /// Overhead latency used to tune SSD read calls simulations with hardware
            int64_t m_readOverheadLatency_ns;

            /// Overhead latency used to tune SSD write calls simulations with hardware
            int64_t m_writeOverheadLatency_ns;

            /// SSD's read bandwidth per queue
            double m_readBandwidthPerQueue_GBps;
            
            /// SSD's read bandwidth per queue
            double m_writeBandwidthPerQueue_GBps;
        };

        class SimpleSSD : public SimpleSSDAPI
        {
        public:
            SST_ELI_REGISTER_SUBCOMPONENT(
                SimpleSSD,
                "firefly",
                "SimpleSSD",
                SST_ELI_ELEMENT_VERSION(1, 0, 0),
                "A simple server model to mimic SSD",
                SST::Firefly::SimpleSSDAPI)

            SST_ELI_DOCUMENT_PARAMS({"nSSDsPerNode","The number of SSDs to simulate", "1"},
                                    {"queuesCountPerSSD", "The number of parallel paths per SSD", "4"},
                                    {"readBandwidthPerSSD_GBps", "", "6.25"}, 
                                    {"writeBandwidthPerSSD_GBps", "", "6.25"}, 
                                    {"readOverheadLatency_ns", "Latency(ns) used for tuning simulations to hardware experiments", "500"},
                                    {"writeOverheadLatency_ns", "Latency(ns) used for tuning simulations to hardware experiments", "500"})

            SST_ELI_DOCUMENT_STATISTICS(
                {"readRequests",       "Number of read requests dispatched to the SSD bus", "count", 1},
                {"writeRequests",      "Number of write requests dispatched to the SSD bus", "count", 1},
                {"readBytes",          "Total bytes read from the SSD", "bytes", 1},
                {"writeBytes",         "Total bytes written to the SSD", "bytes", 1},
                {"readLatency_ns",     "Per-request read latency (ns), sum of modelled SSD delays", "ns", 1},
                {"writeLatency_ns",    "Per-request write latency (ns), sum of modelled SSD delays", "ns", 1},
                {"queueDepthOnEnqueue","Lane queue depth observed at the moment a request was enqueued", "depth", 1},
                {"pendingOnDispatch",  "In-flight request count observed when clockTick issued a request", "depth", 1},
            )

            /// Constructor
            SimpleSSD(ComponentId_t id, Params &params);
            
            /// Copy constructor deleted
            SimpleSSD(const SimpleSSD &) = delete;

            /// Assignment operator deleted
            SimpleSSD operator=(const SimpleSSD &) = delete;

            /// Destructor
            ~SimpleSSD() = default;

            /// SSD read API to read bytes from offset
            void read(int64_t offset, size_t bytes, const std::function<void(void)> &callback) override;
            
            /// SSD write API to write bytes to offset
            void write(int64_t offset, size_t bytes, const std::function<void(void)> &callback) override;

            /// Caculating the delay of a particular message size
            int64_t calcDelay_ns(size_t bytes, double bandwidth, int64_t latency) override;

        private:
            /// Logging system
            SST::Output m_out;

            /// Self link for delay
            SST::Link *m_selfLink;

            /// Counter for the total number of pending requests
            int64_t m_pendingRequests;

            /// Per-lane busy-until cycle (1 GHz clock => 1 cycle == 1 ns)
            std::vector<int64_t> m_laneFreeAtCycle;

            /// Counters for finish() summary lines
            uint64_t m_totalReadRequests = 0;
            uint64_t m_totalWriteRequests = 0;
            uint64_t m_totalReadBytes = 0;
            uint64_t m_totalWriteBytes = 0;

            /// SST statistics
            Statistic<uint64_t>* m_statReadRequests = nullptr;
            Statistic<uint64_t>* m_statWriteRequests = nullptr;
            Statistic<uint64_t>* m_statReadBytes = nullptr;
            Statistic<uint64_t>* m_statWriteBytes = nullptr;
            Statistic<uint64_t>* m_statReadLatency = nullptr;
            Statistic<uint64_t>* m_statWriteLatency = nullptr;
            Statistic<uint64_t>* m_statQueueDepthOnEnqueue = nullptr;
            Statistic<uint64_t>* m_statPendingOnDispatch = nullptr;

            /// Handle event function for the self-link
            void handleEvent(SST::Event *ev);

            /// Clock tick function
            virtual bool clockTick(SST::Cycle_t);

            /// API to queue the requests
            void enqueueRequest(const int64_t offset, const size_t bytes, const double bandwidth, const int64_t latency, const SsdReqCallback& callback);

            /// finish() emits a single summary line per SimpleSSD instance
            void finish() override;

            std::unordered_map<uint32_t, SsdReqCallback> m_ssdCbMap;
            uint32_t m_nextSsdCbId = 1;
            uint32_t registerSsdCb(const SsdReqCallback& cb) {
                uint32_t id = m_nextSsdCbId++;
                if ( 0 == id ) { id = m_nextSsdCbId++; }
                m_ssdCbMap[id] = cb;
                return id;
            }
            void invokeSsdCb(uint32_t id) {
                auto it = m_ssdCbMap.find(id);
                assert( it != m_ssdCbMap.end() );
                auto cb = std::move(it->second);
                m_ssdCbMap.erase(it);
                cb();
            }

            /// Delay class for the delay events
            class DelayEvent : public SST::Event
            {
            public:
                uint32_t m_cb_id;
                DelayEvent() : Event(), m_cb_id(0) {}
                DelayEvent(uint32_t cb_id) : Event(), m_cb_id(cb_id) {}

                void serialize_order(SST::Core::Serialization::serializer& ser) override {
                    Event::serialize_order(ser);
                    SST_SER(m_cb_id);
                }
                ImplementSerializable(SST::Firefly::SimpleSSD::DelayEvent);
            };
        };
    }
}

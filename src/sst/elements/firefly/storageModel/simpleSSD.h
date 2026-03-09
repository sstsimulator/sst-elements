#pragma once

#include <sst/core/subcomponent.h>
#include <sst/core/link.h>

#include <queue>
#include <vector>
#include <functional>

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

            /// Handle event function for the self-link
            void handleEvent(SST::Event *ev);

            /// Clock tick function
            virtual bool clockTick(SST::Cycle_t);

            /// API to queue the requests
            void enqueueRequest(const int64_t offset, const size_t bytes, const double bandwidth, const int64_t latency, const SsdReqCallback& callback);

            /// Delay class for the delay events
            class DelayEvent : public SST::Event
            {
            public:
                SsdReqCallback m_callback;
                DelayEvent(SsdReqCallback callback) : Event(), m_callback(callback) {}
                NotSerializable(DelayEvent)
            };
        };
    }
}

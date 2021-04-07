// Copyright 2013-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _MEMHIERARCHY_MEMNICFOUR_SUBCOMPONENT_H_
#define _MEMHIERARCHY_MEMNICFOUR_SUBCOMPONENT_H_

#include <string>
#include <map>
#include <queue>

#include <sst/core/event.h>
#include <sst/core/output.h>
#include <sst/core/subcomponent.h>
#include <sst/core/interfaces/simpleNetwork.h>

#include "sst/elements/memHierarchy/memEventBase.h"
#include "sst/elements/memHierarchy/util.h"
#include "sst/elements/memHierarchy/memNIC.h"

namespace SST {
namespace MemHierarchy {

class MemNICFour : public MemNICBase {

public:
/* Element Library Info */
#define MEMNICFOUR_ELI_PARAMS MEMNICBASE_ELI_PARAMS, \
        { "data.network_bw",                "(string) Data network. Network bandwidth", "80GiB/s" },\
        { "data.network_input_buffer_size", "(string) Data network. Size of input buffer", "1KiB"},\
        { "data.network_output_buffer_size","(string) data network. Size of output buffer", "1KiB"},\
        { "data.min_packet_size",           "(string) Data network. Size of a packet without a payload (e.g., control message size)", "8B"},\
        { "data.port",                      "(string) Data network. Set by parent component. Name of port this NIC sits on.", ""}, \
        { "req.network_bw",                 "(string) Req network. Network bandwidth", "80GiB/s" },\
        { "req.network_input_buffer_size",  "(string) Req network. Size of input buffer", "1KiB"},\
        { "req.network_output_buffer_size", "(string) Req network. Size of output buffer", "1KiB"},\
        { "req.min_packet_size",            "(string) Req network. Size of a packet without a payload (e.g., control message size)", "8B"},\
        { "req.port",                       "(string) Req network. Set by parent component. Name of port this NIC sits on.", ""}, \
        { "ack.network_bw",                 "(string) Ack network. Network bandwidth", "80GiB/s" },\
        { "ack.network_input_buffer_size",  "(string) Ack network. Size of input buffer", "1KiB"},\
        { "ack.network_output_buffer_size", "(string) Ack network. Size of output buffer", "1KiB"},\
        { "ack.min_packet_size",            "(string) Ack network. Size of a packet without a payload (e.g., control message size)", "8B"},\
        { "ack.port",                       "(string) Ack network. Set by parent component. Name of port this NIC sits on.", ""}, \
        { "fwd.network_bw",                 "(string) Fwd network. Network bandwidth", "80GiB/s" },\
        { "fwd.network_input_buffer_size",  "(string) Fwd network. Size of input buffer", "1KiB"},\
        { "fwd.network_output_buffer_size", "(string) Fwd network. Size of output buffer", "1KiB"},\
        { "fwd.min_packet_size",            "(string) Fwd network. Size of a packet without a payload (e.g., control message size)", "8B"},\
        { "fwd.port",                       "(string) Fwd network. Set by parent component. Name of port this NIC sits on.", ""},\
        { "clock",                          "(string) Units for latency statistics", "1GHz"}


    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(MemNICFour, "memHierarchy", "MemNICFour", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Memory-oriented network interface for split networks", SST::MemHierarchy::MemLinkBase)

    SST_ELI_DOCUMENT_PARAMS( MEMNICFOUR_ELI_PARAMS )

    SST_ELI_DOCUMENT_STATISTICS(
            { "data_events", "Number of events received on data network", "count", 1},
            { "req_events", "Number of events received on request network", "count", 1},
            { "ack_events", "Number of events received on acknowledgement network", "count", 1},
            { "fwd_events", "Number of events received on forward request network", "count", 1},
            { "outoforder_data_events", "Number of out of order events on data network", "count", 1},
            { "outoforder_req_events", "Number of out of order events on request network", "count", 1},
            { "outoforder_ack_events", "Number of out of order events on acknowledgement network", "count", 1},
            { "outoforder_fwd_events", "Number of out of order events on forward request network", "count", 1},
            { "outoforder_depth_at_event_receive", "Depth of re-order buffer at an event receive", "count", 1},
            { "outoforder_depth_at_event_receive_src", "Depth of re-order buffer for the sender of an event at event receive", "count", 1},
            { "ordering_latency", "For events that arrived out of order, cycles spent in buffer. Cycles in units determined by 'clock' parameter (default 1GHz)", "cycles", 1})

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
            {"data", "Link control subcomponent to data network", "SST::Interfaces::SimpleNetwork"},
            {"req", "Link control subcomponent to request network", "SST::Interfaces::SimpleNetwork"},
            {"ack", "Link control subcomponent to acknowledgement network", "SST::Interfaces::SimpleNetwork"},
            {"fwd", "Link control subcomponent to forwarded request network", "SST::Interfaces::SimpleNetwork"})

/* Begin class definition */

    enum NetType { REQ, ACK, FWD, DATA };
    /* Constructor */
    MemNICFour(ComponentId_t id, Params &params);

    /* Destructor */
    ~MemNICFour() { }

    /* Functions called by parent for handling events */
    bool clock();
    void send(MemEventBase * ev);
    bool recvNotifyReq(int);
    bool recvNotifyAck(int);
    bool recvNotifyFwd(int);
    bool recvNotifyData(int);
    void doRecv(SST::Interfaces::SimpleNetwork::Request* req, NetType net);

    /* Helper functions */
    size_t getSizeInBits(MemEventBase * ev, NetType net);

    /* Initialization and finish */
    void init(unsigned int phase);
    void finish() {
        for (int i = 0; i < 4; i++)
            link_control[i]->finish();
    }
    void setup();

    // Router events
    class OrderedMemRtrEvent : public MemNICBase::MemRtrEvent {
        public:
            unsigned int tag;

            OrderedMemRtrEvent() : MemRtrEvent() { }
            OrderedMemRtrEvent(MemEventBase * ev, unsigned int t) : MemRtrEvent(ev), tag(t) { }

            virtual Event* clone(void) override {
                OrderedMemRtrEvent * omre = new OrderedMemRtrEvent(*this);
                if (this->event != nullptr)
                    omre->event = this->event->clone();
                else
                    omre->event = nullptr;
                return omre;
            }

            virtual bool hasClientData() const override { return true; }

            void serialize_order(SST::Core::Serialization::serializer &ser) override {
                MemRtrEvent::serialize_order(ser);
                ser & tag;
            }

            ImplementSerializable(SST::MemHierarchy::MemNICFour::OrderedMemRtrEvent);
    };

    /* Debug support */
    void printStatus(Output& out);

private:

    void recvNotify(MemNICFour::OrderedMemRtrEvent* mre);
    MemNICFour::OrderedMemRtrEvent* processRecv(SST::Interfaces::SimpleNetwork::Request* req);

    // Other parameters
    size_t packetHeaderBytes[4];

    // Handlers and network
    std::array<SST::Interfaces::SimpleNetwork*, 4> link_control;

    // Event queues
    std::queue<SST::Interfaces::SimpleNetwork::Request*> sendQueue[4];
    std::queue<MemNICFour::OrderedMemRtrEvent*> recvQueue;

    // Order tag tracking
    std::map<uint64_t, unsigned int> sendTags;
    std::map<uint64_t, unsigned int> recvTags;
    std::map<uint64_t, std::map<unsigned int, std::pair<OrderedMemRtrEvent*,SimTime_t> > > orderBuffer;

    // Statistics
    Statistic<uint64_t>* stat_oooEvent[4];
    Statistic<uint64_t>* stat_oooDepth;
    Statistic<uint64_t>* stat_oooDepthSrc;
    Statistic<uint64_t>* stat_orderLatency;
    uint64_t totalOOO; // Since there's no simple count of orderBuffer size
};

} //namespace memHierarchy
} //namespace SST

#endif

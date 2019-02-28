// Copyright 2013-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _MEMHIERARCHY_MULTINETMEMNIC_SUBCOMPONENT_H_
#define _MEMHIERARCHY_MULTINETMEMNIC_SUBCOMPONENT_H_

#include <string>
#include <map>
#include <queue>

#include <sst/core/event.h>
#include <sst/core/output.h>
#include <sst/core/subcomponent.h>
#include <sst/core/elementinfo.h>
#include <sst/core/interfaces/simpleNetwork.h>

#include "sst/elements/memHierarchy/memEventBase.h"
#include "sst/elements/memHierarchy/util.h"
#include "sst/elements/memHierarchy/memNIC.h"

namespace SST {
namespace MemHierarchy {

/*
 *  MemNIC provides a simpleNetwork (from SST core) network interface for memory components
 *  and overlays memory functions on top
 *
 *  The multinet memNIC sends data on different networks depending on traffic class
 *
 *  The memNIC assumes each network endpoint is associated with a set of memory addresses and that
 *  each endpoint communicates with a subset of endpoints on the network as defined by "sources"
 *  and "destinations".
 *
 */
class MemNICMulti : public MemNICBase {

public:
/* Element Library Info */
#define MULTINETMEMNIC_ELI_PARAMS MEMNIC_ELI_PARAMS, \
        { "data.network_bw",                "(string) Data network. Network bandwidth", "80GiB/s" },\
        { "data.network_input_buffer_size", "(string) Data network. Size of input buffer", "1KiB"},\
        { "data.network_output_buffer_size","(string) data network. Size of output buffer", "1KiB"},\
        { "data.min_packet_size",           "(string) Data network. Size of a packet without a payload (e.g., control message size)", "8B"},\
        { "data.port",                      "(string) Data network. Set by parent component. Name of port this NIC sits on.", ""}


    SST_ELI_REGISTER_SUBCOMPONENT(MemNICMulti, "memHierarchy", "MemNICMulti", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Memory-oriented network interface for split control and data networks", "SST::MemLinkBase")

    SST_ELI_DOCUMENT_PARAMS( MULTINETMEMNIC_ELI_PARAMS )

/* Begin class definition */
    /* Constructor */
    MemNICMulti(Component * comp, Params &params);

    /* Destructor */
    ~MemNICMulti() { }

    /* Functions called by parent for handling events */
    bool clock();
    void send(MemEventBase * ev);
    bool recvNotifyCtrl(int);
    bool recvNotifyData(int);
    MemEventBase* recv(bool ctrlLink);

    /* Helper functions */
    size_t getSizeInBits(MemEventBase * ev);

    /* Initialization and finish */
    void init(unsigned int phase);
    void finish() { link_control->finish(); data_link_control->finish(); }
    void setup() {
        link_control->setup();
        data_link_control->setup();
        MemLinkBase::setup();
    }

    // Router events
    class SplitMemRtrEvent : public MemNICBase::MemRtrEvent {
        public:
            unsigned int tag;   // Tag -> matches from control & data (unique = <src,tag>). 0 for no match needed

            SplitMemRtrEvent() : MemRtrEvent() { }
            SplitMemRtrEvent(MemEventBase * ev, unsigned int t) : MemRtrEvent(ev), tag(t) { }
            SplitMemRtrEvent(unsigned int t) : MemRtrEvent(nullptr), tag(t) { }
            SplitMemRtrEvent(MemEventBase * ev) : MemRtrEvent(ev), tag(0) { }

            virtual Event* clone(void) override {
                SplitMemRtrEvent *mre = new SplitMemRtrEvent(*this);
                if (this->event != nullptr) {
                    mre->event = this->event->clone();
                } else {
                    mre->event = nullptr;
                }
                return mre;
            }

            virtual bool hasClientData() const { return true; }

            void serialize_order(SST::Core::Serialization::serializer &ser) override {
                MemRtrEvent::serialize_order(ser);
                ser & tag;
            }

            ImplementSerializable(SST::MemHierarchy::MemNICMulti::SplitMemRtrEvent);
    };

private:

    void recvNotify(MemNICMulti::SplitMemRtrEvent* mre);
    MemNICMulti::SplitMemRtrEvent* processRecv(SST::Interfaces::SimpleNetwork::Request* req);

    bool matchCtrlTags(uint64_t src, unsigned int tag);
    bool matchDataTags(uint64_t src, unsigned int tag);

    // Other parameters
    size_t packetHeaderBytes;
    size_t dataPacketHeaderBytes;

    // Handlers and network
    SST::Interfaces::SimpleNetwork *link_control;
    SST::Interfaces::SimpleNetwork *data_link_control;

    // Event queues
    std::queue<SST::Interfaces::SimpleNetwork::Request*> dataSendQueue; // Queue of events waiting to be sent (sent on clock)

    // Receive queue for forcing ordering
    // A queue for each source
    std::queue<SST::Interfaces::SimpleNetwork::Request*> sendQueue; // Queue of events waiting to be sent (sent on clock)
    std::map<uint64_t, std::queue< std::pair<bool,SplitMemRtrEvent*> > > recvMsgQueue;

    // Unmatched tag tracking, set for each source
    std::map<uint64_t, std::set<unsigned int> > dataTags;
    std::map<uint64_t, std::set<unsigned int> > ctrlTags;

    unsigned int nextTag;
    bool dupCtrl;
};

} //namespace memHierarchy
} //namespace SST

#endif

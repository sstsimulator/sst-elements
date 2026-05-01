// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_MERLIN_REORDERLINKCONTROL_H
#define COMPONENTS_MERLIN_REORDERLINKCONTROL_H

#include <sst/core/subcomponent.h>
#include <sst/core/unitAlgebra.h>

#include <sst/core/interfaces/simpleNetwork.h>

#include <sst/core/statapi/statbase.h>

#include "sst/elements/merlin/router.h"

#include <queue>
#include <unordered_map>

namespace SST {

class Component;

namespace Merlin {

// Need our own version of Request to add a sequence number
class ReorderRequest : public SST::Interfaces::SimpleNetwork::Request {

public:
    uint32_t seq = 0;

    ReorderRequest() = default;


    ReorderRequest(SST::Interfaces::SimpleNetwork::Request* req, uint32_t seq = 0) :
        Request(req->dest, req->src, req->size_in_bits, req->head, req->tail),
        seq(seq)
        {
            givePayload(req->takePayload());
            trace = req->getTraceType();
            traceID = req->getTraceID();
        }

    ~ReorderRequest() {}


    // This is here just for the priority_queue insertion, so is
    // sorting based on what comes out of queue first (i.e. lowest
    // number, which makes this look backwards)
    class Priority {
    public:
        bool operator()(ReorderRequest* const& lhs, ReorderRequest* const& rhs) {
            return lhs->seq > rhs->seq;
        }
    };

    typedef std::priority_queue<ReorderRequest*, std::vector<ReorderRequest*>, Priority> PriorityQueue;

    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        SST::Interfaces::SimpleNetwork::Request::serialize_order(ser);
        SST_SER(seq);
    }

private:
    ImplementSerializable(SST::Merlin::ReorderRequest)
};



struct ReorderInfo {
    uint32_t send;
    uint32_t recv;
    ReorderRequest::PriorityQueue queue;

    ReorderInfo() :
        send(0),
        recv(0)
    {
        // Put a dummy entry into queue to avoid checks for NULL later
        // on when looking for fragments to deliver.  This does mean
        // we can't handle more than 4 billion fragments to each host
        // with overflow.
        ReorderRequest* req = new ReorderRequest();
        req->seq = 0xffffffff;
        queue.push(req);
    }

    void serialize_order(SST::Core::Serialization::serializer& ser)
    {
        SST_SER(send);
        SST_SER(recv);
        SST_SER(queue);
    }

};

// Version of LinkControl that will allow out of order receive, but
// will make things appear in order to NIC.  The current version will
// have essentially infinite resources and is here just to get
// functionality working.
class ReorderLinkControl : public SST::Interfaces::SimpleNetwork {
public:

    SST_ELI_REGISTER_SUBCOMPONENT(
        ReorderLinkControl,
        "merlin",
        "reorderlinkcontrol",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Link Control module that can handle out of order packet arrival. Events are sequenced and order is reconstructed on receive.",
        SST::Interfaces::SimpleNetwork
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"rlc.networkIF","SimpleNetwork subcomponent to be used for connecting to network", "merlin.linkcontrol"},
        {"networkIF","SimpleNetwork subcomponent to be used for connecting to network", "merlin.linkcontrol"}
    )

    SST_ELI_DOCUMENT_PORTS(
        {"rtr_port", "Port that connects to router", { "merlin.RtrEvent", "merlin.credit_event", "" } },
    )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        {"networkIF", "Network interface", "SST::Interfaces::SimpleNetwork" }
    )

    typedef std::queue<SST::Interfaces::SimpleNetwork::Request*> request_queue_t;

private:
    int vns = 0;
    SST::Interfaces::SimpleNetwork* link_control = nullptr;


    UnitAlgebra link_bw;
    int id = 0;

    std::unordered_map<SST::Interfaces::SimpleNetwork::nid_t, ReorderInfo*> reorder_info;

    // One buffer for each virtual network.  At the NIC level, we just
    // provide a virtual channel abstraction.  Don't need output
    // buffers, sends will go directly to LinkControl.  Do need input
    // buffers.
    request_queue_t* input_buf = nullptr;

    // Functors for notifying the parent when there is more space in
    // output queue or when a new packet arrives
    HandlerBase* receiveFunctor = nullptr;
//    HandlerBase* sendFunctor;

public:
    ReorderLinkControl(ComponentId_t cid, Params &params, int vns);
    ReorderLinkControl() = default;

    ~ReorderLinkControl();

    void setup() override;
    void init(unsigned int phase) override;
    void complete(unsigned int phase) override;
    void finish() override;

    // Returns true if there is space in the output buffer and false
    // otherwise.
    bool send(SST::Interfaces::SimpleNetwork::Request* req, int vn) override;

    // Returns true if there is space in the output buffer and false
    // otherwise.
    bool spaceToSend(int vn, int flits) override;

    // Returns NULL if no event in input_buf[vn]. Otherwise, returns
    // the next event.
    SST::Interfaces::SimpleNetwork::Request* recv(int vn) override;

    // Returns true if there is an event in the input buffer and false
    // otherwise.
    bool requestToReceive( int vn ) override;

    void sendUntimedData(SST::Interfaces::SimpleNetwork::Request* ev) override;
    SST::Interfaces::SimpleNetwork::Request* recvUntimedData() override;

    // const PacketStats& getPacketStats(void) const { return stats; }

    void setNotifyOnReceive(HandlerBase* functor) override;
    void setNotifyOnSend(HandlerBase* functor) override;

    bool isNetworkInitialized() const override;
    nid_t getEndpointID() const override;
    const UnitAlgebra& getLinkBW() const override;


    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(SST::Merlin::ReorderLinkControl)

private:

    bool handle_event(int vn);
};

}
}

#endif // COMPONENTS_MERLIN_REORDERLINKCONTROL_H

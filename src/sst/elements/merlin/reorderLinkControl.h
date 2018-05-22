// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_MERLIN_REORDERLINKCONTROL_H
#define COMPONENTS_MERLIN_REORDERLINKCONTROL_H

#include <sst/core/elementinfo.h>
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
    uint32_t seq;

    ReorderRequest() :
        Request(),
        seq(0)
        {}

    // ReorderRequest(SST::Interfaces::SimpleNetwork::nid_t dest, SST::Interfaces::SimpleNetwork::nid_t src,
    //                size_t size_in_bits, bool head, bool tail, uint32_t seq, Event* payload = NULL) :
    //     Request(dest, src, size_in_bits, head, tail, payload ),
    //     seq(seq)
    //     {
    //     }

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
        ser & seq;
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
        "SST::Interfaces::SimpleNetwork")
    
    SST_ELI_DOCUMENT_PARAMS(
        {"rlc:networkIF","SimpleNetwork subcomponent to be used for connecting to network", "merlin.linkcontrol"}
    )

    
    typedef std::queue<SST::Interfaces::SimpleNetwork::Request*> request_queue_t;
    
private:
    int vns;
    SST::Interfaces::SimpleNetwork* link_control;


    UnitAlgebra link_bw;
    int id;

    std::unordered_map<SST::Interfaces::SimpleNetwork::nid_t, ReorderInfo*> reorder_info;
    
    // One buffer for each virtual network.  At the NIC level, we just
    // provide a virtual channel abstraction.  Don't need output
    // buffers, sends will go directly to LinkControl.  Do need input
    // buffers.
    request_queue_t* input_buf;

    // Functors for notifying the parent when there is more space in
    // output queue or when a new packet arrives
    HandlerBase* receiveFunctor;
//    HandlerBase* sendFunctor;
    
public:
    ReorderLinkControl(Component* parent, Params &params);

    ~ReorderLinkControl();

    // Must be called before any other functions to configure the link.
    // Preferably during the owning component's constructor
    // time_base is a frequency which represents the bandwidth of the link in flits/second.
    bool initialize(const std::string& port_name, const UnitAlgebra& link_bw_in,
                    int vns, const UnitAlgebra& in_buf_size,
                    const UnitAlgebra& out_buf_size);
    void setup();
    void init(unsigned int phase);
    void finish();

    // Returns true if there is space in the output buffer and false
    // otherwise.
    bool send(SST::Interfaces::SimpleNetwork::Request* req, int vn);

    // Returns true if there is space in the output buffer and false
    // otherwise.
    bool spaceToSend(int vn, int flits);

    // Returns NULL if no event in input_buf[vn]. Otherwise, returns
    // the next event.
    SST::Interfaces::SimpleNetwork::Request* recv(int vn);

    // Returns true if there is an event in the input buffer and false 
    // otherwise.
    bool requestToReceive( int vn );

    void sendInitData(SST::Interfaces::SimpleNetwork::Request* ev);
    SST::Interfaces::SimpleNetwork::Request* recvInitData();

    // const PacketStats& getPacketStats(void) const { return stats; }

    void setNotifyOnReceive(HandlerBase* functor);
    void setNotifyOnSend(HandlerBase* functor);

    bool isNetworkInitialized() const;
    nid_t getEndpointID() const;
    const UnitAlgebra& getLinkBW() const;

    
private:

    bool handle_event(int vn);
};

}
}

#endif // COMPONENTS_MERLIN_REORDERLINKCONTROL_H

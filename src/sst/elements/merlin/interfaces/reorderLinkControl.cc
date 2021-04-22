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


#include <sst_config.h>

#include "reorderLinkControl.h"

#include <sst/core/simulation.h>

#include "merlin.h"

namespace SST {
using namespace Interfaces;

namespace Merlin {

ReorderLinkControl::ReorderLinkControl(ComponentId_t cid, Params &params, int vns) :
    SimpleNetwork(cid),
    receiveFunctor(NULL),
    vns(vns)
{
    if ( isUser() ) {
        // Need to see if the network_if was loaded as a user subcomponent
        link_control = loadUserSubComponent<SimpleNetwork>("networkIF", ComponentInfo::SHARE_NONE, vns);
        // If the load was successful, we can return
        if ( link_control ) {
            this->vns = vns;

            // Don't need output buffers, sends will go directly to
            // LinkControl.  Do need input buffers.
            input_buf = new request_queue_t[vns];
            return;
        }
    }

    // NetworkIF not loaded as user subcomponent, try anonymous
    std::string networkIF = params.find<std::string>("networkIF", "merlin.linkcontrol");

    // Get the params for the child
    Params childParams = params.get_scoped_params("networkIF");
    if ( childParams.size() == 0 ) {
        // Not using new method of passing through parameters, just
        // send all params to child
        childParams = params;
    }

    // If this was loaded as a user subcomponent, need to tell the
    // child what port to connect to
    if ( isUser() ) childParams.insert("port_name","rtr_port");

    input_buf = new request_queue_t[vns];

    link_control =
        loadAnonymousSubComponent<SimpleNetwork>(networkIF, "networkIF", 0, ComponentInfo::INSERT_STATS | ComponentInfo::SHARE_PORTS, childParams, vns);
}


ReorderLinkControl::~ReorderLinkControl() {
    delete [] input_buf;
}

void
ReorderLinkControl::setup()
{
    link_control->setup();
    // while ( init_events.size() ) {
    //     delete init_events.front();
    //     init_events.pop_front();
    // }
    link_control->setNotifyOnReceive(new SimpleNetwork::Handler<ReorderLinkControl>(this,&ReorderLinkControl::handle_event));
}

void ReorderLinkControl::init(unsigned int phase)
{
    if ( phase == 0 ) {
        link_control->setNotifyOnReceive(new SimpleNetwork::Handler<ReorderLinkControl>(this,&ReorderLinkControl::handle_event));
    }
    link_control->init(phase);
    if (link_control->isNetworkInitialized()) {
        id = link_control->getEndpointID();
    }
}

void ReorderLinkControl::complete(unsigned int phase)
{
    link_control->complete(phase);
}


void ReorderLinkControl::finish(void)
{
    // Clean up all the events left in the queues.  This will help
    // track down real memory leaks as all this events won't be in the
    // way.
    // for ( int i = 0; i < req_vns; i++ ) {
    //     while ( !input_buf[i].empty() ) {
    //         delete input_buf[i].front();
    //         input_buf[i].pop();
    //     }
    // }

    // TODO: Need to delete reordered data as well

    link_control->finish();
}


// Returns true if there is space in the output buffer and false
// otherwise.
bool ReorderLinkControl::send(SimpleNetwork::Request* req, int vn) {
    if ( vn >= vns ) return false;
    if ( !link_control->spaceToSend(vn, req->size_in_bits) ) return false;
    ReorderRequest* my_req = new ReorderRequest(req);
    delete req;

    // Need to put in the sequence number

    // See if we already have reorder_info for this dest
    if ( reorder_info.find(my_req->dest) == reorder_info.end() ) {
        ReorderInfo* info = new ReorderInfo();
        reorder_info[my_req->dest] = info;
    }
    ReorderInfo* info = reorder_info[my_req->dest];
    my_req->seq = info->send++;

    // // To test, just going to switch order
    // uint32_t my_seq = info->send % 2 == 0 ? info->send + 1 : info->send - 1;
    // my_req->seq = my_seq;
    // info->send++;

    // std::cout << id << ": sending packet with sequence number " << my_req->seq << std::endl;

    return link_control->send(my_req, vn);
}


// Returns true if there is space in the output buffer and false
// otherwise.
bool ReorderLinkControl::spaceToSend(int vn, int bits) {
    return link_control->spaceToSend(vn, bits);
}


// Returns NULL if no event in input_buf[vn]. Otherwise, returns
// the next event.
SST::Interfaces::SimpleNetwork::Request* ReorderLinkControl::recv(int vn) {
    if ( input_buf[vn].size() == 0 ) return NULL;

    SimpleNetwork::Request* req = input_buf[vn].front();
    input_buf[vn].pop();
    return req;
}

bool ReorderLinkControl::requestToReceive( int vn ) {
//    return link_control->requestToReceive(vn);
    return !input_buf[vn].empty();
}


void ReorderLinkControl::sendUntimedData(SST::Interfaces::SimpleNetwork::Request* req)
{
    link_control->sendUntimedData(req);
}

SST::Interfaces::SimpleNetwork::Request* ReorderLinkControl::recvUntimedData()
{
    return link_control->recvUntimedData();
}

void ReorderLinkControl::sendInitData(SST::Interfaces::SimpleNetwork::Request* req)
{
    link_control->sendInitData(req);
}

SST::Interfaces::SimpleNetwork::Request* ReorderLinkControl::recvInitData()
{
    return link_control->recvInitData();
}


void ReorderLinkControl::setNotifyOnReceive(HandlerBase* functor) {
    receiveFunctor = functor;
}

void ReorderLinkControl::setNotifyOnSend(HandlerBase* functor) {
    // The notifyOnSend can just be handled directly by the
    // link_control block
    link_control->setNotifyOnSend(functor);
}


bool ReorderLinkControl::isNetworkInitialized() const {
    return link_control->isNetworkInitialized();
}

SST::Interfaces::SimpleNetwork::nid_t ReorderLinkControl::getEndpointID() const {
    return link_control->getEndpointID();
}

const UnitAlgebra& ReorderLinkControl::getLinkBW() const {
    return link_control->getLinkBW();
}

bool ReorderLinkControl::handle_event(int vn) {
    ReorderRequest* my_req = static_cast<ReorderRequest*>(link_control->recv(vn));

    // std::cout << id << ": recieved packet with sequence number " << my_req->seq << std::endl;

    if ( reorder_info.find(my_req->src) == reorder_info.end() ) {
        ReorderInfo* info = new ReorderInfo();
        reorder_info[my_req->src] = info;
    }

    ReorderInfo* info = reorder_info[my_req->src];

    // See if this is the expected sequence number, if not, put it
    // into the ReorderInfo.queue.
    if ( my_req->seq == info->recv ) {
        input_buf[vn].push(my_req);
        info->recv++;
        // Need to also see if we have any other fragments which are
        // now ready to be delivered
        my_req = info->queue.top();
        while ( my_req->seq == info->recv ) {
            input_buf[vn].push(my_req);
            info->queue.pop();
            my_req = info->queue.top();
            info->recv++;
        }

        // If there is a recv functor, need to notify parent
        if ( receiveFunctor != NULL ) {
            bool keep = (*receiveFunctor)(vn);
            if (!keep) receiveFunctor = NULL;
            // while ( keep && input_buf[vn].size() != 0 ) {
            //     keep = (*receiveFunctor)(vn);
            //     if (!keep) receiveFunctor = NULL;
            // }
        }

    }
    else {
        info->queue.push(my_req);
    }

    return true;
}

// bool ReorderLinkControl::handle_send(int vn) {
//     if ( sendFunctor != NULL ) {
//         bool keep = (*sendFunctor)(vn);
//         if ( !keep ) {
//             sendFunctor = NULL;
//             return false;
//         }
//         else {
//             return true;
//         }
//     }
//     else {
//         return false;
//     }
// }


} // namespace Merlin
} // namespace SST


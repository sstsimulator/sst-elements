// Copyright 2013-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2018, NTESS
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

#include "linkControl.h"

#include <sst/core/simulation.h>

namespace SST {
using namespace Interfaces;

namespace Kingsley {

LinkControl::LinkControl(Component* parent, Params &params) :
    SST::Interfaces::SimpleNetwork(parent),
    init_state(0),
    rtr_link(NULL), output_timing(NULL),
    req_vns(0), id(-1),
    input_buf(NULL), output_buf(NULL),
    rtr_credits(NULL), in_ret_credits(NULL),
    waiting(true), have_packets(false),
    receiveFunctor(NULL), sendFunctor(NULL),
    network_initialized(false),
    output(Simulation::getSimulation()->getSimulationOutput())
{
}
    
bool
LinkControl::initialize(const std::string& port_name, const UnitAlgebra& link_bw_in,
                        int vns, const UnitAlgebra& in_buf_size,
                        const UnitAlgebra& out_buf_size)
{    
    req_vns = vns;
    link_bw = link_bw_in;
    if ( link_bw.hasUnits("B/s") ) {
        link_bw *= UnitAlgebra("8b/B");
    }
    
    // Input and output buffers
    input_buf = new network_queue_t[req_vns];
    output_buf = new network_queue_t[req_vns];

    // Initialize credit arrays.  Credits are in flits, and we don't
    // yet know the flit size, so can't initialize in_ret_credits and
    // outbuf_credits yet.  Will initialize them after we get the
    // flit_size
    rtr_credits = new int[req_vns];
    in_ret_credits = new int[req_vns];
    outbuf_credits = new int[req_vns];

    inbuf_size = in_buf_size;
    if ( !inbuf_size.hasUnits("b") && !inbuf_size.hasUnits("B") ) {
        output.fatal(CALL_INFO,-1,"in_buf_size must be specified in either "
                           "bits or bytes: %s\n",inbuf_size.toStringBestSI().c_str());
    }
    if ( inbuf_size.hasUnits("B") ) inbuf_size *= UnitAlgebra("8b/B");


    outbuf_size = out_buf_size;
    if ( !outbuf_size.hasUnits("b") && !outbuf_size.hasUnits("B") ) {
        output.fatal(CALL_INFO,-1,"out_buf_size must be specified in either "
                           "bits or bytes: %s\n",outbuf_size.toStringBestSI().c_str());
    }
    if ( outbuf_size.hasUnits("B") ) outbuf_size *= UnitAlgebra("8b/B");
    
    // The output credits are set to zero and the other side of the
    // link will send the number of tokens.
    for ( int i = 0; i < req_vns; i++ ) rtr_credits[i] = 0;

    // Configure the links
    // For now give it a fake timebase.  Will give it the real timebase during init
    // rtr_link = rif->configureLink(port_name, time_base, new Event::Handler<LinkControl>(this,&LinkControl::handle_input));
    rtr_link = configureLink(port_name, std::string("1GHz"), new Event::Handler<LinkControl>(this,&LinkControl::handle_input));
    // output_timing = rif->configureSelfLink(port_name + "_output_timing", time_base,
    //         new Event::Handler<LinkControl>(this,&LinkControl::handle_output));
    output_timing = configureSelfLink(port_name + "_output_timing", "1GHz",
            new Event::Handler<LinkControl>(this,&LinkControl::handle_output));

    // Register statistics
    packet_latency = registerStatistic<uint64_t>("packet_latency");
    // send_bit_count = registerStatistic<uint64_t>("send_bit_count");
    // output_port_stalls = registerStatistic<uint64_t>("output_port_stalls");
    // idle_time = registerStatistic<uint64_t>("idle_time");
    
    return true;
}


LinkControl::~LinkControl()
{
    delete [] input_buf;
    delete [] output_buf;
    delete [] rtr_credits;
    delete [] in_ret_credits;
    delete [] outbuf_credits;
}

void LinkControl::setup()
{
    while ( init_events.size() ) {
        delete init_events.front();
        init_events.pop_front();
    }
}


void LinkControl::init(unsigned int phase)
{
    Event* ev;
    NocInitEvent* init_ev;
    switch ( init_state ) {
    case 0:
    {
        // output.output("LinkControl::init()\n");
        // Tell the other side of the link that I'm an endpoint
        init_ev = new NocInitEvent();
        init_ev->command = NocInitEvent::REPORT_ENDPOINT;
        init_ev->int_value = req_vns;
        rtr_link->sendInitData(init_ev);

        init_state = 1;
        break;
    }
    case 1:
    {
        ev = rtr_link->recvInitData();
        if ( NULL == ev ) break;
        init_ev = static_cast<NocInitEvent*>(ev);
        UnitAlgebra flit_size_ua = init_ev->ua_value;
        flit_size = flit_size_ua.getRoundedValue();

        UnitAlgebra link_clock = link_bw / flit_size_ua;
        
        TimeConverter* tc = getTimeConverter(link_clock);
        output_timing->setDefaultTimeBase(tc);

        for ( int i = 0; i < req_vns; ++i ) {
            outbuf_credits[i] = outbuf_size.getRoundedValue() / flit_size;
            in_ret_credits[i] = inbuf_size.getRoundedValue() /flit_size;
        }
       
        init_state = 2;
        break;
    }
    case 2:
    {
        ev = rtr_link->recvInitData();
        if ( NULL == ev ) break;
        init_ev = static_cast<NocInitEvent*>(ev);
        id = init_ev->int_value;

        // Send credit event to router
        credit_event* cr_ev = new credit_event(0,inbuf_size.getRoundedValue() / flit_size);
        rtr_link->sendInitData(cr_ev);
        
        // network_initialized = true;
        init_state = 3;
        break;
    }
    case 3:
        network_initialized = true;
        init_state = 4;
        // Falls through on purpose
    default:
        // For all other phases, look for credit events, any other
        // events get passed up to containing component by adding them
        // to init_events queue
        while ( ( ev = rtr_link->recvInitData() ) != NULL ) {
            BaseNocEvent* bev = static_cast<BaseNocEvent*>(ev);
            switch (bev->getType()) {
            case BaseNocEvent::CREDIT:
            {
                credit_event* ce = static_cast<credit_event*>(bev);
                // output.output("%d: Got a credit event for VN %d with %d credits\n",id,ce->vn,ce->credits);
                if ( ce->vn < req_vns ) {  // Ignore credit events for VNs I don't have
                    rtr_credits[ce->vn] += ce->credits;
                }
                delete ev;
                // if ( waiting && have_packets ) {
                //     output_timing->send(1,NULL);
                //     waiting = false;
                // }
            }
                break;
            case BaseNocEvent::PACKET:
                init_events.push_back(static_cast<NocPacket*>(ev));
                break;
            default:
                // This shouldn't happen.  Only NocPackets (PACKET
                // types) should not be handled in the LinkControl
                // object.
                // output.fatal(CALL_INFO, 1, "Reached state where a non-NocPacket was not handled.");
                break;
            }
        }
        break;
    }
}

void LinkControl::complete(unsigned int phase)
{
    Event* ev;
    NocInitEvent* init_ev;
    // For all other phases, look for credit events, any other
    // events get passed up to containing component by adding them
    // to init_events queue
    while ( ( ev = rtr_link->recvInitData() ) != NULL ) {
        BaseNocEvent* bev = static_cast<BaseNocEvent*>(ev);
        switch (bev->getType()) {
        case BaseNocEvent::CREDIT:
        {
            credit_event* ce = static_cast<credit_event*>(bev);
            // output.output("%d: Got a credit event for VN %d with %d credits\n",id,ce->vn,ce->credits);
            if ( ce->vn < req_vns ) {  // Ignore credit events for VNs I don't have
                rtr_credits[ce->vn] += ce->credits;
            }
            delete ev;
            // if ( waiting && have_packets ) {
            //     output_timing->send(1,NULL);
            //     waiting = false;
            // }
        }
        break;
        case BaseNocEvent::PACKET:
            init_events.push_back(static_cast<NocPacket*>(ev));
            break;
        default:
            // This shouldn't happen.  Only NocPackets (PACKET
            // types) should not be handled in the LinkControl
            // object.
            // output.fatal(CALL_INFO, 1, "Reached state where a non-NocPacket was not handled.");
            break;
        }
    }
    return;
}


void LinkControl::finish(void)
{
    // Clean up all the events left in the queues.  This will help
    // track down real memory leaks as all this events won't be in the
    // way.
/*    for ( int i = 0; i < req_vns; i++ ) {
        while ( !input_buf[i].empty() ) {
            delete input_buf[i].front();
            input_buf[i].pop();
        }
        while ( !output_buf[i].empty() ) {
            delete output_buf[i].front();
            output_buf[i].pop();
        }
    }*/
}


// Returns true if there is space in the output buffer and false
// otherwise.
bool LinkControl::send(SimpleNetwork::Request* req, int vn) {
    if ( vn >= req_vns ) return false;
    req->vn = vn;
    NocPacket* ev = new NocPacket(req);
    int flits = (ev->request->size_in_bits + (flit_size - 1)) / flit_size;
    ev->setSizeInFlits(flits);

    if ( outbuf_credits[vn] < flits ) return false;

    outbuf_credits[vn] -= flits;

    // printf("%d: Send message to %llu on VN: %d, which is actually VN:%d --> %llu",id,req->dest,vn,req->vn,req->dest+req->src);
    // std::cout << std::endl;
    
    output_buf[ev->request->vn].push(ev);
    if ( waiting && !have_packets ) {
        output_timing->send(1,NULL);
        waiting = false;
    }
    
    if ( ev->getTraceType() != SimpleNetwork::Request::NONE ) {
        output.output("TRACE(%d): %" PRIu64 " ns: Send on LinkControl in NIC: %s\n",ev->getTraceID(),
                      parent->getCurrentSimTimeNano(), parent->getName().c_str());
    }
    
    return true;
}


// Returns true if there is space in the output buffer and false
// otherwise.
bool LinkControl::spaceToSend(int vn, int bits) {
    if ( (outbuf_credits[vn] * flit_size) < bits) return false;
    return true;
}


// Returns NULL if no event in input_buf[vn]. Otherwise, returns
// the next event.
SST::Interfaces::SimpleNetwork::Request* LinkControl::recv(int vn) {
    if ( input_buf[vn].size() == 0 ) return NULL;

    NocPacket* event = input_buf[vn].front();
    input_buf[vn].pop();

    // Figure out how many credits to return
    int flits = event->getSizeInFlits();
    in_ret_credits[event->request->vn] += flits;

    // For now, we're just going to send the credits back to the
    // other side.  The required BW to do this will not be taken
    // into account.
    rtr_link->send(new credit_event(event->request->vn,in_ret_credits[event->request->vn]));
    in_ret_credits[event->request->vn] = 0;

    // printf("%d: Returning credits on VN: %d for packet from %llu",id, event->request->vn, event->request->src);
    // std::cout << std::endl;
    
    // if ( event->getTraceType() != SimpleNetwork::Request::NONE ) {
    //     output.output("TRACE(%d): %" PRIu64 " ns: recv called on LinkControl in NIC: %s\n",event->getTraceID(),
    //                   parent->getCurrentSimTimeNano(), parent->getName().c_str());
    //     // std::cout << "TRACE(" << event->getTraceID() << "): " << parent->getCurrentSimTimeNano()
    //     //           << " ns: recv called on LinkControl in NIC: "
    //     //           << parent->getName() << std::endl;
    // }

    SST::Interfaces::SimpleNetwork::Request* ret = event->request;
    event->request = NULL;
    delete event;
    return ret;
}

void LinkControl::sendInitData(SST::Interfaces::SimpleNetwork::Request* req)
{
    rtr_link->sendInitData(new NocPacket(req));
}

SST::Interfaces::SimpleNetwork::Request* LinkControl::recvInitData()
{
    if ( init_events.size() ) {
        NocPacket *ev = init_events.front();
        init_events.pop_front();
        SST::Interfaces::SimpleNetwork::Request* ret = ev->request;
        ev->request = NULL;
        delete ev;
        return ret;
    } else {
        return NULL;
    }
}



void LinkControl::handle_input(Event* ev)
{
    // TraceFunction trace(CALL_INFO_LONG);
    // Check to see if this is a credit or data packet
    // credit_event* ce = dynamic_cast<credit_event*>(ev);
    // if ( ce != NULL ) {
    BaseNocEvent* bev = static_cast<BaseNocEvent*>(ev);
    switch (bev->getType()) {
    case BaseNocEvent::CREDIT:
    {
    	credit_event* ce = static_cast<credit_event*>(ev);
        rtr_credits[ce->vn] += ce->credits;
        delete ev;

        // If we're waiting, we need to send a wakeup event to the
        // output queues
        if ( waiting ) {
            output_timing->send(0,NULL);
            waiting = false;
        }
        break;
    }

    case BaseNocEvent::PACKET:
    {
        NocPacket* event = static_cast<NocPacket*>(ev);
        input_buf[event->vn].push(event);

        if ( event->request->getTraceType() == SimpleNetwork::Request::FULL ) {
            output.output("TRACE(%d): %" PRIu64 " ns: Received an event on LinkControl in NIC: %s"
                          " on VN %d from src %" PRIu64 "\n",
                          event->request->getTraceID(),
                          parent->getCurrentSimTimeNano(),
                          parent->getName().c_str(),
                          event->request->vn,
                          event->request->src);
        }
        SimTime_t lat = parent->getCurrentSimTimeNano() - event->getInjectionTime();
        packet_latency->addData(lat);

        if ( receiveFunctor != NULL ) {
            bool keep = (*receiveFunctor)(0 /*event->vn*/);
            if ( !keep) receiveFunctor = NULL;
        }

        break;
    }
    default:
        break;
    }
}


void LinkControl::handle_output(Event* ev)
{
    // The event is an empty event used just for timing.

    // ***** Need to add in logic for when to return credits *****
    // For now just done automatically when events are pulled out
    // of the block

    // Only supporting one VN for now
    if ( !output_buf[0].empty() ) {
        have_packets = true;
        int vn = 0;
        NocPacket* send_event = output_buf[vn].front();

        // output.output("rtr_credits[%d] = %d\n",vn, rtr_credits[vn]);
        if ( rtr_credits[vn] < send_event->getSizeInFlits() ) {
            waiting = true;
            return;
        }
        // output.output("found enough credits\n");
        output_buf[vn].pop();
        
        // Send the output to the network.
        // First set the virtual channel.
        send_event->request->vn = vn;
        send_event->vn = vn;

        // Need to return credits to the output buffer
        int size = send_event->getSizeInFlits();
        // outbuf_credits[vn_to_send] += size;
        outbuf_credits[vn] += size;

        // Send an event to wake up again after this packet is sent.
        output_timing->send(size,NULL);
        
        // // Add in inject time so we can track latencies
        send_event->setInjectionTime(parent->getCurrentSimTimeNano());
        
        // Subtract credits
        rtr_credits[vn] -= size;

        // printf("%d: Sending packet to %llu on VN: %d",id, send_event->request->dest, send_event->request->vn);
        // std::cout << std::endl;

        rtr_link->send(send_event);
        // std::cout << "Sent packet on vn " << vn_to_send << ", credits remaining: " << rtr_credits[vn_to_send] << std::endl;
        
        if ( send_event->getTraceType() == SimpleNetwork::Request::FULL ) {
            output.output("TRACE(%d): %" PRIu64 " ns: Sent an event to router from LinkControl"
                          " in NIC: %s on VN %d to dest %" PRIu64 ".\n",
                          send_event->getTraceID(),
                          parent->getCurrentSimTimeNano(),
                          parent->getName().c_str(),
                          send_event->request->vn,
                          send_event->request->dest);
        }

        if (sendFunctor != NULL ) {
            bool keep = (*sendFunctor)(vn);
            if ( !keep ) sendFunctor = NULL;
        }
    }
    else {
        waiting = true;
        have_packets = false;
    }

    // send_bit_count->addData(send_event->request->size_in_bits);
}

void
LinkControl::printStatus(Output& out)
{
    out.output("Start LinkControl for Component %s:\n", parent->getName().c_str());

    out.output("  Router credits = %d\n",rtr_credits[0]);
    out.output("  Output Buffer credits = %d\n",outbuf_credits[0]);
    out.output("  Input queue total packets = %lu\n",input_buf[0].size());
    out.output("  Input queue total packets = %lu, head packet info:\n",output_buf[0].size());
    if ( output_buf[0].empty() ) {
        out.output("    <empty>\n");
    }
    else {
        NocPacket* event = output_buf[0].front();
        out.output("      src = %lld, dest = %lld, flits = %d\n",
                   event->request->src, event->request->dest,
                   event->getSizeInFlits());
    }
    
    out.output("End LinkControl for Component %s\n", parent->getName().c_str());
}

} // namespace Merlin
} // namespace SST

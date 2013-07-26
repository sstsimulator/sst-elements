// Copyright 2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst/core/serialization.h>

#include "linkControl.h"


using namespace SST;
using namespace Merlin;


LinkControl::LinkControl(Params &params) :
    rtr_link(NULL), output_timing(NULL),
    num_vcs(0),
    input_buf(NULL), output_buf(NULL),
    rtr_credits(NULL), in_ret_credits(NULL),
    curr_out_vc(0), waiting(true),
    parent(NULL)
{
}

// time_base is a frequency which represents the bandwidth of the link in flits/second.
void LinkControl::configureLink(Component* rif, std::string port_name, TimeConverter* time_base, int vcs, int* in_buf_size, int* out_buf_size)
{
    num_vcs = vcs;
    parent = rif;


    // Input and output buffers
    input_buf = new network_queue_t[vcs];
    output_buf = new network_queue_t[vcs];

    // Initialize credit arrays
    rtr_credits = new int[vcs];
    in_ret_credits = new int[vcs];
    outbuf_credits = new int[vcs];

    // Copy the starting return tokens for the input buffers (this
    // essentially sets the size of the buffer)
    memcpy(in_ret_credits,in_buf_size,vcs*sizeof(int));
    memcpy(outbuf_credits,out_buf_size,vcs*sizeof(int));

    // The output credits are set to zero and the other side of the
    // link will send the number of tokens.
    for ( int i = 0; i < vcs; i++ ) rtr_credits[i] = 0;

    // Configure the links
    rtr_link = rif->configureLink(port_name, time_base, new Event::Handler<LinkControl>(this,&LinkControl::handle_input));
    output_timing = rif->configureSelfLink(port_name + "_output_timing", time_base,
            new Event::Handler<LinkControl>(this,&LinkControl::handle_output));

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
    if ( phase == 0 ) {
        // Need to send the available credits to the other side
        for ( int i = 0; i < num_vcs; i++ ) {
            rtr_link->sendInitData(new credit_event(i,in_ret_credits[i]));
            in_ret_credits[i] = 0;
        }
    }
    // Need to recv the credits send from the other side
    Event* ev;
    while ( ( ev = rtr_link->recvInitData() ) != NULL ) {
        credit_event* ce = dynamic_cast<credit_event*>(ev);
        if ( ce != NULL ) {
            rtr_credits[ce->vc] += ce->credits;
            delete ev;
        } else {
            init_events.push_back(ev);
        }
    }
}


void LinkControl::finish(void)
{

}


// Returns true if there is space in the output buffer and false
// otherwise.
bool LinkControl::send(RtrEvent* ev, int vc) {
    if ( outbuf_credits[vc] < ev->size_in_flits ) return false;

    outbuf_credits[vc] -= ev->size_in_flits;
    ev->vc = vc;

    output_buf[vc].push(ev);
    if ( waiting ) {
        output_timing->send(1,NULL);
        waiting = false;
    }

    ev->setInjectionTime(parent->getCurrentSimTimeNano());

    if ( ev->getTraceType() != RtrEvent::NONE ) {
        std::cout << "TRACE(" << ev->getTraceID() << "): " << parent->getCurrentSimTimeNano()
                  << " ns: Sent on LinkControl in NIC: "
                  << parent->getName() << std::endl;
    }
    return true;
}


// Returns true if there is space in the output buffer and false
// otherwise.
bool LinkControl::spaceToSend(int vc, int flits) {
    if (outbuf_credits[vc] < flits) return false;
    return true;
}


// Returns NULL if no event in input_buf[vc]. Otherwise, returns
// the next event.
RtrEvent* LinkControl::recv(int vc) {
    if ( input_buf[vc].size() == 0 ) return NULL;

    RtrEvent* event = input_buf[vc].front();
    input_buf[vc].pop();

    // Figure out how many credits to return
    in_ret_credits[vc] += event->size_in_flits;

    // For now, we're just going to send the credits back to the
    // other side.  The required BW to do this will not be taken
    // into account.
    rtr_link->send(1,new credit_event(vc,in_ret_credits[vc]));
    in_ret_credits[vc] = 0;

    if ( event->getTraceType() != RtrEvent::NONE ) {
        std::cout << "TRACE(" << event->getTraceID() << "): " << parent->getCurrentSimTimeNano()
                  << " ns: recv called on LinkControl in NIC: "
                  << parent->getName() << std::endl;
    }

    return event;
}

void LinkControl::sendInitData(RtrEvent *ev)
{
    rtr_link->sendInitData(ev);
}

Event* LinkControl::recvInitData()
{
    if ( init_events.size() ) {
        Event *ev = init_events.front();
        init_events.pop_front();
        return ev;
    } else {
        return NULL;
    }
}



void LinkControl::handle_input(Event* ev)
{
    // Check to see if this is a credit or data packet
    credit_event* ce = dynamic_cast<credit_event*>(ev);
    if ( ce != NULL ) {
        rtr_credits[ce->vc] += ce->credits;
        // std::cout << "Got " << ce->credits << " credits.  Current credits: " << rtr_credits[ce->vc] << std::endl;
        delete ev;

        // If we're waiting, we need to send a wakeup event to the
        // output queues
        if ( waiting ) {
            output_timing->send(1,NULL);
            waiting = false;
        }
    }
    else {
        // std::cout << "LinkControl received an event" << std::endl;
        RtrEvent* event = static_cast<RtrEvent*>(ev);
        // Simply put the event into the right virtual network queue
        input_buf[event->vc].push(event);
        if ( event->getTraceType() == RtrEvent::FULL ) {
            std::cout << "TRACE(" << event->getTraceID() << "): " << parent->getCurrentSimTimeNano()
                                                                     << " ns: Received an event on LinkControl in NIC: "
                                                                     << parent->getName() << " on VC " << event->vc << " from src " << event->src
                                                                     << "." << std::endl;
        }
        SimTime_t lat = parent->getCurrentSimTimeNano() - event->getInjectionTime();
        stats.insertPacketLatency(lat);
    }
}


void LinkControl::handle_output(Event* ev)
{
    // The event is an empty event used just for timing.

    // ***** Need to add in logic for when to return credits *****
    // For now just done automatically when events are pulled out
    // of the block

    // We do a round robin scheduling.  If the current vc has no
    // data, find one that does.
    int vc_to_send = -1;
    bool found = false;
    RtrEvent* send_event = NULL;

    for ( int i = curr_out_vc; i < num_vcs; i++ ) {
        if ( output_buf[i].empty() ) continue;
        send_event = output_buf[i].front();
        // Check to see if the needed VC has enough space
        if ( rtr_credits[i] < send_event->size_in_flits ) continue;
        vc_to_send = i;
        output_buf[i].pop();
        found = true;
        break;
    }

    if (!found)  {
        for ( int i = 0; i < curr_out_vc; i++ ) {
            if ( output_buf[i].empty() ) continue;
            send_event = output_buf[i].front();
            // Check to see if the needed VC has enough space
            if ( rtr_credits[i] < send_event->size_in_flits ) continue;
            vc_to_send = i;
            output_buf[i].pop();
            found = true;
            break;
        }
    }


    // If we found an event to send, go ahead and send it
    if ( found ) {
        // Send the output to the network.
        // First set the virtual channel.
        send_event->vc = vc_to_send;

        // Need to return credits to the output buffer
        int size = send_event->size_in_flits;
        outbuf_credits[vc_to_send] += size;

        // Send an event to wake up again after this packet is sent.
        output_timing->send(size,NULL);

        curr_out_vc = vc_to_send + 1;
        if ( curr_out_vc == num_vcs ) curr_out_vc = 0;

        // Subtract credits
        rtr_credits[vc_to_send] -= size;
        rtr_link->send(send_event);
        // std::cout << "Sent packet on vc " << vc_to_send << std::endl;

        if ( send_event->getTraceType() == RtrEvent::FULL ) {
            std::cout << "TRACE(" << send_event->getTraceID() << "): " << parent->getCurrentSimTimeNano()
                                                                          << " ns: Sent an event to router from LinkControl in NIC: "
                                                                          << parent->getName() << " on VC " << send_event->vc << " to dest " << send_event->dest
                                                                          << "." << std::endl;
        }
    }
    else {
        // What do we do if there's nothing to send??  It could be
        // because everything is empty or because there's not
        // enough room in the router buffers.  Either way, we
        // don't send a wakeup event.  We will send a wake up when
        // we either get something new in the output buffers or
        // receive credits back from the router.  However, we need
        // to know that we got to this state.
        // std::cout << "Waiting ..." << std::endl;
        waiting = true;
    }
}


void LinkControl::PacketStats::insertPacketLatency(SimTime_t lat)
{
    numPkts++;
    if ( 1 == numPkts ) {
        minLat = lat;
        maxLat = lat;
        m_n = m_old = lat;
        s_old = 0.0;
    } else {
        minLat = std::min(minLat, lat);
        maxLat = std::max(maxLat, lat);
        m_n = m_old + (lat - m_old) / numPkts;
        s_n = s_old + (lat - m_old) * (lat - m_n);

        m_old = m_n;
        s_old = s_n;
    }
}



/*
 * =====================================================================================
// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
 * =====================================================================================
 */

/*
 * =====================================================================================
 *       Filename:  router.cc
 * =====================================================================================
 */

#ifndef  _NINTERFACE_CC_INC
#define  _NINTERFACE_CC_INC

#include "sst_config.h"
#include "sst/core/serialization.h"
#include	"ninterface.h"

#include	<sstream>

#include "sst/core/element.h"
#include "sst/core/link.h"
#include "sst/core/params.h"

namespace SST {
namespace Iris {
const char* NINTERFACE_FREQ = "500MHz";
}
}


using namespace SST::Iris;

/* *********** Arbiter Functions ************ */
SimpleArbiter::SimpleArbiter()
{
}

SimpleArbiter::~SimpleArbiter()
{
}

void
SimpleArbiter::init ( void )
{
    requests.resize(no_channels);
    for ( uint i=0; i<no_channels; i++)
        requests[i] = false;

    return ;
}		/* -----  end of method SimpleArbiter::init  ----- */

bool
SimpleArbiter::is_requested ( uint ch )
{
//    assert( ch < requests.size() );
    return requests[ch];
}

bool
SimpleArbiter::is_empty ( void )
{
    for ( uint i=0 ; i<no_channels; i++ )
        if ( requests[i] )
            return false;

    return true;
}

void
SimpleArbiter::request ( uint ch )
{
    requests[ch] = true;
    return ;
}	

uint
SimpleArbiter::pick_winner ( void )
{
    for ( uint i=last_winner+1; i<requests.size() ; i++ )
        if ( requests[i] )
        {
            last_winner = i;
            requests[i] = false;
            return last_winner;
        }

    for ( uint i=0; i<=last_winner ; i++ )
        if ( requests[i] )
        {
            last_winner = i;
            requests[i] = false;
            return last_winner;
        }

    std::cout << " ERROR: Interface Arbiter was either empty or called and dint have a winner" << std::endl;
    exit (1);
}

/* *********** Network Interface Functions ************ */
NInterface::NInterface (SST::ComponentId_t id, Params& params): DES_Component(id), no_vcs(2)
{

    // get configuration parameters
    if ( params.find("id") == params.end() ) {
        _abort(event_test,"Specify node_id for the router\n");
    }
    node_id = strtol( params[ "id" ].c_str(), NULL, 0 );

    if ( params.find("vcs") != params.end() ) {
        no_vcs = strtol( params[ "vcs" ].c_str(), NULL, 0 );
    }
    if ( params.find("buffer_size") != params.end() ) {
        buffer_size= strtol( params[ "buffer_size" ].c_str(), NULL, 0 );
    }
    if ( params.find("terminal_credits") != params.end() ) {
        tcredits = strtol( params[ "terminal_credits" ].c_str(), NULL, 0 );
    }
    if ( params.find("rtr_credits") != params.end() ) {
        rcredits = strtol( params[ "rtr_credits" ].c_str(), NULL, 0 );
    }
    std::string nic_frequency = "500MHz";
    nic_frequency = params["clock"];
    /* ************ End of update configuration *************** */
    arbiter = *new SimpleArbiter();
    arbiter.no_channels = no_vcs;
    arbiter.last_winner = 0;
    arbiter.init();

    // Need to configure all links with the handler functions 
    terminal_link = configureLink( "cpu", 
					    new SST::Event::Handler<NInterface,int>
					    (this, &NInterface::handle_terminal_link_arrival,1)); 
    
    router_link = configureLink( "rtr", 
					    new SST::Event::Handler<NInterface,int>
					    (this, &NInterface::handle_router_link_arrival,0)); 

    //set our clock
    DES_Clock::Handler<NInterface>* clock_handler
        = new DES_Clock::Handler<NInterface>(this, &NInterface::tock);
    registerClock(nic_frequency, clock_handler);

    resize();

    //    r_param->dump_router_config();
}  /* -----  end of method NInterface::NInterface  (constructor)  ----- */

NInterface::~NInterface ()
{
    terminal_credits.clear();
//    terminal_inBuffer.clear();
    terminal_inPkt_time.clear();
    terminal_pktcomplete.clear();

    router_credits.clear();
//    router_inBuffer.clear();
    router_inPkt_time.clear();
    router_pktcomplete.clear();

}		/* -----  end of method NInterface::~NInterface  ----- */

void
NInterface::resize( void )
{

    /* Clear out the previous setting 
    terminal_credits.clear();
    terminal_inBuffer.clear();
    terminal_inPkt_time.clear();
    terminal_pktcomplete.clear();

    router_credits.clear();
    router_inBuffer.clear();
    router_inPkt_time.clear();
    router_pktcomplete.clear();
     * */

    /* ************* END of clean up *************/

    router_inBuffer = new GenericBuffer(no_vcs);
    terminal_inBuffer = new GenericBuffer(no_vcs);

    // TODO want to bound buffers using buffer size
    //  router_inBuffer->resize ( no_vcs, buffer_size);     
    //  terminal_inBuffer->resize ( no_vcs, buffer_size);

    last_inpkt_winner = 0;

    for ( int i=0; i<no_vcs; i++)
    {
        terminal_inPkt_time.push_back(0);
        router_inPkt_time.push_back(0);

        terminal_pktcomplete.push_back(false);
        router_pktcomplete.push_back(false);

        router_credits.push_back(rcredits);
        terminal_credits.push_back(tcredits);
    }

    /*  Stats */
    total_pkts_out = 0;
    avg_pkt_lat = 0;

    return;
}

void
NInterface::reset_stats ( void )
{

    return ;
}		/* -----  end of method NInterface::reset_stats  ----- */

void
NInterface::print_stats ( std::string& stat_str ) const
{
    std::stringstream ss;
    ss << 
        "NInt ["<<node_id<<"] total_pkts_out: "<< total_pkts_out << "\n"
        "NInt ["<<node_id<<"] avg_pkt_lat: "<< avg_pkt_lat *1.0 / total_pkts_out << "\n"
        ;
    stat_str.assign(ss.str());
    return ;
}		/* -----  end of method NInterface::print_stats  ----- */

void
NInterface::handle_router_link_arrival ( DES_Event* ev, int port_id)
{
    irisRtrEvent* event = dynamic_cast<irisRtrEvent*>(ev);

    switch ( event->type )
    {
        case irisRtrEvent::Credit:
            {
                /* Router channel is emptied update state to 
                   indicate one more pkt can be sent out */
                if( event->credit.vc > no_vcs )
                {
                    iris_panic(" ninterface got credit from router for non existing vc!!");
                }
                else
                    router_credits.at(event->credit.vc)++;
                break;
            }
        case irisRtrEvent::Packet:
            {

                // Get the port from the link name or pass a parameter
                router_inBuffer->push(event->packet);
                router_inPkt_time.at(event->packet->vc) = _TICK_NOW;

                // add some fixed latency for accepting all the flits in the packet

                break;
            }
        default:
            break;
    }


    return ;
}		/* -----  end of method NInterface::handle_router_link_arrival  ----- */

void
NInterface::handle_terminal_link_arrival ( DES_Event* ev ,int port_id )
{
    irisRtrEvent* event = dynamic_cast<irisRtrEvent*>(ev);

    switch ( event->type )
    {
        case irisRtrEvent::Credit:
            {
                if( event->credit.vc > no_vcs )
                {
                    iris_panic(" ninterface got credit from terminal for non existing vc!!");
                }
                else
                    terminal_credits.at(event->credit.vc)++;

                break;
            }
        case irisRtrEvent::Packet:
            {
                assert ( event->packet != NULL && "Pkt event has null packet\n" );
                terminal_inBuffer->push(event->packet);
                terminal_inPkt_time.at(event->packet->vc) = _TICK_NOW;

                break;
            }
    }
}		/* -----  end of method NInterface::handle_terminal_link_arrival  ----- */

bool
NInterface::tock (SST::Cycle_t cycle )
{ 
    /******************* Handle events TERMINAL->NETWORK ********************/
    {

        //Request the arbiter for all completed packets. Do not request if
        //router credits are empty.
        for ( int i=0; i<no_vcs ; i++ )
        {
            // check all the packets that can be sent out and request the arbiter
            // to pick the winning packet
            if ( terminal_pktcomplete.at(i) && router_credits.at(i) )
            {
                if ( !arbiter.is_requested(i) )
                    arbiter.request(i);
            }
        }

        // Pick a winner for the arbiter
        if ( !arbiter.is_empty() )
        {
            int winner = arbiter.pick_winner();
            assert ( winner < no_vcs );

            // send out the pkt to router as we already ensured there
            // are credits earlier this cycle

            /*create des event with Npkt to send to router */
            irisNPkt* pkt = terminal_inBuffer->pull(winner);
            pkt->vc = winner;
            irisRtrEvent* pkt_event = new irisRtrEvent;
            pkt_event->type = irisRtrEvent::Packet;
            pkt_event->packet = pkt;
            router_link->send(pkt_event); 
            router_credits.at(winner)--;
            total_pkts_out++;
            avg_pkt_lat += (_TICK_NOW - terminal_inPkt_time.at(winner)); 

            terminal_pktcomplete.at(winner) = false;

            // update your credits counter to indicate router has one
            // less empty buffer

            /* create des event for terminal signalling pkt out */
            irisRtrEvent* credit_event = new irisRtrEvent; 
            credit_event->type = irisRtrEvent::Credit;
            credit_event->credit.vc = winner;
            credit_event->credit.num = 1;      // if we do use flits then update this
            terminal_link->send(credit_event);  

        }

        // not simulating flits for now
        // check if the time at which the pkt was recieved at this 
        // interface+no of flits is the current time
        // Move flits from the pkt buffer to the router_out_buffer
        for ( int i=0 ; i<no_vcs ; i++ )
            if( terminal_inBuffer->get_occupancy(i) && !terminal_pktcomplete[i])
            {
                uint64_t send_pkt_time = terminal_inPkt_time.at(i) + 
                    terminal_inBuffer->get_pktLength(i)+1;
                if (send_pkt_time <= _TICK_NOW )
                {
                    // push the flit into the router out buffer and clear the
                    // terminal buffer to send out the next packet
                    terminal_pktcomplete.at(i) = true;
                }
            }
    }
    /************** Handle events on the NETWORK->TERMINAL *********************/

    // Move completed packets to the terminal
    for ( int i=0; i<no_vcs ; i++ )
        if( router_inBuffer->get_occupancy(i) && terminal_credits[i] && router_pktcomplete[i] )
        {
            terminal_credits.at(i)--;
            router_pktcomplete.at(i) = false;
            //            static_cast<IrisTerminal*>(terminal)->terminal_recv = true;

            //send the new pkt to the terminal
            irisNPkt* pkt = router_inBuffer->pull(i);
            pkt->vc = i;
            irisRtrEvent* pkt_event = new irisRtrEvent;
            pkt_event->type = irisRtrEvent::Packet;
            pkt_event->packet = pkt;
            terminal_link->send(pkt_event); 

            //send the credit back to the router
            irisRtrEvent* credit_event = new irisRtrEvent; 
            credit_event->type = irisRtrEvent::Credit;
            credit_event->credit.vc = i;
            credit_event->credit.num = 1;      // if we do use flits then update this
            //credit_ev->src = this->GetComponentId();
            router_link->send(credit_event);  

        }


    // mock the interface streaming flits in on the router link
    for ( int i=0; i<no_vcs ; i++ )
    {
        if( router_inBuffer->get_occupancy(i) )
        {
            uint64_t pkt_complete_time = router_inBuffer->get_pktLength(i)
                + router_inPkt_time.at(i)+1;
            if( pkt_complete_time <= _TICK_NOW )
                router_pktcomplete.at(i) = true;
        }
    }

    return false;
}		/* -----  end of method NInterface::tock  ----- */

#endif   /* ----- #ifndef _NINTERFACE_CC_INC  ----- */

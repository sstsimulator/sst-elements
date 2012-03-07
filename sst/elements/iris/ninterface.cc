
/*
 * =====================================================================================
 *       Filename:  router.cc
 * =====================================================================================
 */

#ifndef  _NINTERFACE_CC_INC
#define  _NINTERFACE_CC_INC

#include "sst_config.h"
#include "sst/core/serialization/element.h"

#include "sst/core/element.h"

#include	"ninterface.h"
const char* NINTERFACE_FREQ = "500MHz";
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
NInterface::NInterface (SST::ComponentId_t id, Params_t& params): DES_Component(id) 
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
    if ( params.find("credits") != params.end() ) {
        buffer_size= strtol( params[ "credits" ].c_str(), NULL, 0 );
    }
    /* ************ End of update configuration *************** */
    arbiter = *new SimpleArbiter();
    arbiter.no_channels = no_vcs;
    arbiter.last_winner = 0;
    arbiter.init();

    // Need to configure all links with the handler functions 
    terminal_link = configureLink( "terminal_link", 
					    new SST::Event::Handler<NInterface,int>
					    (this, &NInterface::handle_terminal_link_arrival,1)); 
    
    router_link = configureLink( "router_link", 
					    new SST::Event::Handler<NInterface,int>
					    (this, &NInterface::handle_router_link_arrival,0)); 

    //set our clock
    new DES_Clock::Handler<NInterface>(this, &NInterface::tock);

    resize();

    //    r_param->dump_router_config();
}  /* -----  end of method NInterface::NInterface  (constructor)  ----- */

NInterface::~NInterface ()
{
  terminal_credits.clear();
//  terminal_inBuffer.clear();
  terminal_inPkt_time.clear();
  terminal_pktcomplete.clear();

  router_credits.clear();
//  router_inBuffer.clear();
  router_inPkt_time.clear();
  router_pktcomplete.clear();

}		/* -----  end of method NInterface::~NInterface  ----- */

void
NInterface::resize( void )
{

    /* Clear out the previous setting */
  terminal_credits.clear();
  //terminal_inBuffer.clear();
  terminal_inPkt_time.clear();
  terminal_pktcomplete.clear();

  router_credits.clear();
  //router_inBuffer.clear();
  router_inPkt_time.clear();
  router_pktcomplete.clear();

  /* ************* END of clean up *************/

  router_inBuffer = new GenericBuffer(no_vcs);
  terminal_inBuffer = new GenericBuffer(no_vcs);

// TODO want to bound buffers using buffer size
//  router_inBuffer->resize ( no_vcs, buffer_size);     
//  terminal_inBuffer->resize ( no_vcs, buffer_size);

  router_credits.resize( no_vcs );
  terminal_credits.resize ( no_vcs );

  terminal_inPkt_time.resize ( no_vcs );
  router_inPkt_time.resize ( no_vcs );

  terminal_pktcomplete.resize(no_vcs);
  router_pktcomplete.resize(no_vcs);

  last_inpkt_winner = 0;

  for ( int i=0; i<no_vcs; i++)
  {
      terminal_inPkt_time[i] = 0;
      router_inPkt_time[i] = 0;

      terminal_pktcomplete[i] = false;
      router_pktcomplete[i] = false;

      router_credits[i] = credits;
      terminal_credits[i]=true;
  }

  return;
}

void
NInterface::reset_stats ( void )
{

    return ;
}		/* -----  end of method NInterface::reset_stats  ----- */

const char* 
NInterface::print_stats ( void ) const
{
    return "add stats for interface";
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
                router_inBuffer->push(&event->packet);
                router_inPkt_time.at(event->packet.vc) = _TICK_NOW;

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
                    terminal_credits.at(event->credit.vc) = true;

                break;
            }
        case irisRtrEvent::Packet:
            {
                terminal_inBuffer->push(&event->packet);
                terminal_inPkt_time.at(event->packet.vc) = _TICK_NOW;
                break;
            }
    }
}		/* -----  end of method NInterface::handle_terminal_link_arrival  ----- */

bool
NInterface::tock ( SST::Cycle_t now )
{ 
    /******************* Handle events on the ROUTER side ********************/
    {

        //Request the arbiter for all completed packets. Do not request if
        //router credits are empty.
        for ( int i=0; i<no_vcs ; i++ )
        {
            // check all the packets that can be sent out and request the arbiter
            // to pick the winning packet
            if ( terminal_pktcomplete.at(i) &&
                 router_credits.at(i) >= terminal_inBuffer->get_pktLength(i) )
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
            pkt_event->packet = *pkt;
            router_link->Send(pkt_event);

            terminal_pktcomplete.at(winner) = false;

            // update your credits counter to indicate router has one
            // less empty buffer
            router_credits.at(winner)--;

            /* create des event for terminal signalling pkt out */
            irisRtrEvent* credit_event = new irisRtrEvent; 
            credit_event->type = irisRtrEvent::Credit;
            credit_event->credit.vc = winner;
            credit_event->credit.num = 1;      // if we do use flits then update this
            terminal_link->Send(credit_event);

        }

        // not simulating flits for now
        // check if the time at which the pkt was recieved at this 
        // interface+no of flits is the current time
        // Move flits from the pkt buffer to the router_out_buffer
        for ( int i=0 ; i<no_vcs ; i++ )
            if( terminal_inBuffer->get_occupancy(i) )
            {
                uint64_t send_pkt_time = terminal_inPkt_time.at(i) + 
                    terminal_inBuffer->get_pktLength(i);
                if (send_pkt_time >= _TICK_NOW )
                {
                    // push the flit into the router out buffer and clear the
                    // terminal buffer to send out the next packet
                    terminal_pktcomplete.at(i) = true;
                }
            }
    }
    /************** Handle events on the PROCESSOR side *********************/

    // Move completed packets to the terminal
    bool found = false;
    // send a new packet to the terminal.
    // In order to make sure it gets one packet in a cycle you check the
    // terminal_recv flag
    // for parallel simulation make sure the interface and terminal are on the
    // same node
    if ( !static_cast<IrisTerminal*>(terminal)->terminal_recv )
    {
        for ( int i=last_inpkt_winner+1; i<no_vcs ; i++)
            if ( terminal_credits.at(i) && router_pktcomplete.at(i) ) 
            {
                last_inpkt_winner= i;
                found = true;
                break;
            }
        if ( !found )
            for ( int i=0; i<=last_inpkt_winner; i++)
                if ( terminal_credits.at(i) && router_pktcomplete.at(i) ) 
                {
                    last_inpkt_winner= i;
                    found = true;
                    break;
                }

        if ( found )
        {
            terminal_credits.at(last_inpkt_winner) = false;
            router_pktcomplete.at(last_inpkt_winner) = false;
            static_cast<IrisTerminal*>(terminal)->terminal_recv = true;

            //send the new pkt to the terminal
            irisNPkt* pkt = router_inBuffer->pull(last_inpkt_winner);
            pkt->vc = last_inpkt_winner;
            irisRtrEvent* pkt_event = new irisRtrEvent;
            pkt_event->type = irisRtrEvent::Packet;
            pkt_event->packet = *pkt;
            terminal_link->Send(pkt_event);

            //send the credit back to the router
            irisRtrEvent* credit_event = new irisRtrEvent; 
            credit_event->type = irisRtrEvent::Credit;
            credit_event->credit.vc = last_inpkt_winner;
            credit_event->credit.num = 1;      // if we do use flits then update this
            //credit_ev->src = this->GetComponentId();
            router_link->Send(credit_event);

        }
    }

    // mock the interface streaming flits in on the router link
    for ( int i=0; i<no_vcs ; i++ )
    {
        if( router_inBuffer->get_occupancy(i) )
        {
            uint64_t pkt_complete_time = router_inBuffer->get_pktLength(i)
                + router_inPkt_time.at(i);
            if( pkt_complete_time >= _TICK_NOW )
                terminal_pktcomplete.at(i) = true;
        }
    }

    return false;
}		/* -----  end of method NInterface::tock  ----- */

#endif   /* ----- #ifndef _NINTERFACE_CC_INC  ----- */

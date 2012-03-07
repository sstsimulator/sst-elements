
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
NInterface::NInterface (SST::ComponentId_t id, Params_t& params): DES_Component(id) ,
                /*  Init stats */
{

    currently_clocking = true;
    // get configuration parameters
    if ( params.find("id") == params.end() ) {
        _abort(event_test,"Specify node_id for the router\n");
    }
    node_id = strtol( params[ "id" ].c_str(), NULL, 0 );

    if ( params.find("ports") != params.end() ) {
        r_param->ports = strtol( params[ "ports" ].c_str(), NULL, 0 );
    }
    if ( params.find("vcs") != params.end() ) {
        r_param->vcs = strtol( params[ "vcs" ].c_str(), NULL, 0 );
    }
    if ( params.find("credits") != params.end() ) {
        r_param->credits= strtol( params[ "credits" ].c_str(), NULL, 0 );
    }
    if ( params.find("rc_scheme") != params.end() ) {
        r_param->rc_scheme = (routing_scheme_t)strtol( params[ "rc_scheme" ].c_str(), NULL, 0 );
    }
    if ( params.find("no_nodes") != params.end() ) {
        r_param->no_nodes= strtol( params[ "no_nodes" ].c_str(), NULL, 0 );
    }
    if ( params.find("grid_size") != params.end() ) {
        r_param->grid_size= strtol( params[ "grid_size" ].c_str(), NULL, 0 );
    }
    if ( params.find("buffer_size") != params.end() ) {
        r_param->buffer_size= strtol( params[ "buffer_size" ].c_str(), NULL, 0 );
    }
    /* ************ End of update configuration *************** */


    // Need to configure all links with the handler functions 
    // TODO: Check the 7 here
    for (int dir = 0; dir < 7; ++dir) 
        links.push_back( configureLink( LinkNames[dir], 
                                        new SST::Event::Handler<NInterface,int>
                                        (this, &NInterface::handle_link_arrival, dir)));

    //set our clock
    clock_handler = new SST::Clock::Handler<NInterface>(this, &NInterface::tock);
    SST::TimeConverter* tc = registerClock(NINTERFACE_FREQ,clock_handler);
//    printf(" NInterfaces time conversion factor is %lu \n", tc->getFactor());

    resize();

    //    r_param->dump_router_config();
}  /* -----  end of method NInterface::NInterface  (constructor)  ----- */

NInterface::~NInterface ()
{
    // clean up some vectors maybe
    total_buff_occ.clear();

}		/* -----  end of method NInterface::~NInterface  ----- */

void
NInterface::resize( void )
{

    /* Clear out the previous setting */
    for ( uint i=0; i<downstream_credits.size(); ++i)
        downstream_credits.at(i).clear();
    downstream_credits.clear();
    ib_state.clear();
    in_buffer.clear();
    rc.clear();
    /* ************* END of clean up *************/

    ib_state.resize(r_param->ports*r_param->vcs);

    rc.insert(rc.begin(), r_param->ports, GenericRC());
    for ( uint i=0; i<r_param->ports; ++i)
        rc.at(i).node_id = node_id;

    in_buffer.insert(in_buffer.begin(), r_param->ports, GenericBuffer());

    downstream_credits.resize(r_param->ports);
    for ( uint i=0; i<r_param->ports; ++i)
        downstream_credits.at(i).insert(downstream_credits.at(i).begin(), r_param->vcs, r_param->credits);

    vca.resize();
    swa.resize();
    return;
}

void
NInterface::reset_stats ( void )
{
    stat_flits_in = 0;
    stat_flits_out = 0;
    stat_last_flit_time_nano = 0;
    stat_last_flit_cycle = 0;
    stat_packets_out = 0;
    stat_total_pkt_latency_nano = 0;
    heartbeat_interval = 0;
    total_buff_occ.clear();
    stat_avg_buffer_occ=0;

    return ;
}		/* -----  end of method NInterface::reset_stats  ----- */

const char* 
NInterface::print_stats ( void ) const
{
    std::stringstream str;
    str 
        << "\n SimpleNInterface[" << node_id << "] stat_flits_in: " << stat_flits_in
        << "\n SimpleNInterface[" << node_id << "] stat_flits_out: " << stat_flits_out
        << "\n SimpleNInterface[" << node_id << "] avg_router_pkt_latency: " << stat_total_pkt_latency_nano+0.0/stat_packets_out
        << "\n SimpleNInterface[" << node_id << "] stat_last_flit_cycle: " << stat_last_flit_cycle
        << "\n";

    return str.str().c_str();
}		/* -----  end of method NInterface::print_stats  ----- */

void
NInterface::parse_config(std::map<std::string, std::string>& p)
{
    /* Update the parameters if they exist in the map */

    /* Resize the router and all sub components */
    resize();   //TODO: check that clear is doing the right thing
}

void
NInterface::handle_link_arrival ( DES_Event* ev, int dir)
{
    irisRtrEvent* event = dynamic_cast<irisRtrEvent*>(ev);

    switch ( event->type )
    {
        case irisRtrEvent::Credit:
            {
                /* Processor channel is emptied */
                terminal_credits.at(dir)=true;
                break;
            }
        case irisRtrEvent::Packet:
            {
                //                printf("%d: NInterface got pkt dst:%d @ %lu\n", node_id, event->packet.destNum, _TICK_NOW );
                /* Stats update */
                stat_flits_in++;

                // Get the port from the link name or pass a parameter
                in_buffer.at(dir).push(&event->packet);

                break;
            }
        default:
            break;
    }

    if ( !currently_clocking ) {
        //        printf(" Restart the clock@%lu \n", _TICK_NOW);
        currently_clocking = true;
        clock_handler = new SST::Clock::Handler<NInterface>(this, &NInterface::tock);
        SST::TimeConverter* tc = registerClock(NINTERFACE_FREQ,clock_handler, true);
        if ( !tc)
            _abort(event_test,"Failed to restart the clock\n");
    }

    return ;
}		/* -----  end of method NInterface::handle_link_arrival  ----- */

//extern void dump_state_at_deadlock(void);
bool
NInterface::tock ( SST::Cycle_t t )
{ /* Handle events on the ROUTER side */
    // Move flits from the pkt buffer to the router_out_buffer
    for ( uint i=0 ; i<no_vcs ; i++ )
        if ( proc_out_buffer[i].size()>0 && proc_ob_flit_index[i] < proc_out_buffer[i].pkt_length )
        {
            router_out_buffer.change_push_channel(i);
            Flit* f = proc_out_buffer[i].get_next_flit();
            f->virtual_channel = i;
            router_out_buffer.push(f);
            proc_ob_flit_index[i]++;

            if ( proc_ob_flit_index[i] == proc_out_buffer[i].pkt_length )
            {
                proc_ob_flit_index[i] = 0;
                router_ob_packet_complete[i] = true;
            }
        }

    //Request the arbiter for all completed packets. Do not request if
    //downstream credits are empty.
    for ( uint i=0; i<no_vcs ; i++ )
        if( downstream_credits[i]>0 && router_out_buffer.get_occupancy(i)>0
            && router_ob_packet_complete[i] )
        {
            router_out_buffer.change_pull_channel(i);
            Flit* f= router_out_buffer.peek();
            if((f->type != HEAD)||( f->type == HEAD  && downstream_credits[i] == credits))
            {
              
                if ( !arbiter.is_requested(i) )
                    arbiter.request(i);
            }
        }

    // Pick a winner for the arbiter
    if ( !arbiter.is_empty() )
    {
        uint winner = arbiter.pick_winner();
//        assert ( winner < no_vcs );

        // send out a flit as credits were already checked earlier this cycle
        router_out_buffer.change_pull_channel(winner);
        Flit* f = router_out_buffer.pull();

        downstream_credits[winner]--;

        //if this is the last flit signal the terminal to update credit
        if ( f->type == TAIL || f->pkt_length == 1 )
        {
            if( f->type == TAIL )
                static_cast<TailFlit*>(f)->enter_network_time = manifold::kernel::Manifold::NowTicks();
            router_ob_packet_complete[winner] = false;
            handle_send_credit_event( (int) SEND_SIG, winner);
            
        }

        LinkData* ld =  new LinkData();
        ld->type = FLIT;
        ld->src = this->GetComponentId();
        ld->f = f;
        ld->vc = winner;

        Send(0, ld);

#ifdef _DEBUG
        _DBG(" SEND FLIT %d Flit is %s", winner, f->toString().c_str() );std::cout.flush();
#endif        
        //Use manifold send here to push the ld obj out
        //manifold::kernel::Manifold::Schedule(1, &IrisRouter::handle_link_arrival, static_cast<IrisRouter*>(router), (uint)SEND_DATA, ld );

        // Give every vc a fair chance to use the link
        //        arbiter.clear_winner();
    }

    /* Handle events on the PROCESSOR side */
    // Move completed packets to the terminal
    bool found = false;
    // send a new packet to the terminal.
    // In order to make sure it gets one packet in a cycle you check the
    // proc_recv flag
    if ( !static_cast<ManifoldProcessor*>(terminal)->proc_recv )
    {
        for ( uint i=last_inpkt_winner+1; i<no_vcs ; i++)
            if ( proc_credits[i] && proc_ib_flit_index[i]!=0 && 
                 proc_ib_flit_index[i] == proc_in_buffer[i].pkt_length)
            {
                last_inpkt_winner= i;
                found = true;
                break;
            }
        if ( !found )
            for ( uint i=0; i<=last_inpkt_winner; i++)
                if ( proc_credits[i] && proc_ib_flit_index[i]!=0 && 
                     proc_ib_flit_index[i] == proc_in_buffer[i].pkt_length)
                {
                    last_inpkt_winner= i;
                    found = true;
                    break;
                }

        if ( found )
        {
            proc_credits[last_inpkt_winner] = false;
            NetworkPacket* np = new NetworkPacket();
            np->from_flit_level_packet(&proc_in_buffer[last_inpkt_winner]);
            proc_ib_flit_index[last_inpkt_winner] = 0;
            static_cast<ManifoldProcessor*>(terminal)->proc_recv = true;

            //            _DBG(" Int sending pkt to proc %d", last_inpkt_winner); 
            manifold::kernel::Manifold::Schedule(1, &ManifoldProcessor::handle_new_packet_event, static_cast<ManifoldProcessor*>(terminal),(uint)SEND_DATA, np);

            /*  Send the tail credit back. All other flit credits were sent
             *  as soon as flit was got in link-arrival */
            LinkData* ld =  new LinkData();
            ld->type = CREDIT;
            ld->src = this->GetComponentId();
            ld->vc = last_inpkt_winner;

            //_DBG(" SEND CREDIT vc%d", ld->vc);
            Send(1,ld);
            //Use manifold send here to push the ld obj out
            //manifold::kernel::Manifold::Schedule(1, &IrisRouter::handle_link_arrival, static_cast<IrisRouter*>(router), (uint)SEND_DATA, ld );
        }
    }

    // push flits coming in to the in buffer
    for ( uint i=0; i<no_vcs ; i++ )
    {
        if( router_in_buffer.get_occupancy(i) > 0 && (proc_ib_flit_index.at(i) < (proc_in_buffer.at(i)).pkt_length || proc_ib_flit_index.at(i) == 0) )
        {
            router_in_buffer.change_pull_channel(i);
            Flit* ptr = router_in_buffer.pull();
            proc_in_buffer[i].add(ptr); 
            proc_ib_flit_index[i]++;
        }
    }

    return false;
}		/* -----  end of method NInterface::tock  ----- */

#endif   /* ----- #ifndef _NINTERFACE_CC_INC  ----- */

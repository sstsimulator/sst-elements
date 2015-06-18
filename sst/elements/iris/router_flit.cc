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

#ifndef  _ROUTER_CC_INC
#define  _ROUTER_CC_INC

#define DEBUG_NODE 4

#include "sst_config.h"
#include "sst/core/serialization.h"

#include "sst/core/element.h"

#include	"router.h"
namespace SST {
namespace Iris {
const char* SST::Isis::ROUTER_FREQ = "500MHz";

// Move this include into irisComponent
//#include	"genericHeader.cc"

const char* SST::Isis::LinkNames[] = {
    "nic",
    "xPos",
    "xNeg",
    "yPos",
    "yNeg",
    "zPos",
    "zNeg",
};
}
}

using namespace SST::Iris;

Router::Router (SST::ComponentId_t id, Params& params): DES_Component(id) ,
                /*  Init stats */
                stat_flits_in(0), stat_flits_out(0), stat_last_flit_time_nano(0), 
                stat_last_flit_cycle(0), stat_packets_in(0),stat_packets_out(0),
                stat_total_pkt_latency_nano(0),
                heartbeat_interval(500)/* in cycles */, stat_avg_buffer_occ(0.0),
                router_fastfwd(0), empty_cycles(0),
                output(Simulation::getSimulation()->getSimulationOutput())
{

    currently_clocking = true;
    // get configuration parameters
    if ( params.find("id") == params.end() ) {
        output.fatal(CALL_INFO, -1,"Specify node_id for the router\n");
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
                                        new SST::Event::Handler<Router,int>
                                        (this, &Router::handle_link_arrival, dir)));

    //set our clock
    clock_handler = new SST::Clock::Handler<Router>(this, &Router::tock);
    SST::TimeConverter* tc = registerClock(ROUTER_FREQ,clock_handler);

    resize();

    //    r_param->dump_router_config();
}  /* -----  end of method Router::Router  (constructor)  ----- */

Router::~Router ()
{
    // clean up some vectors maybe
    total_buff_occ.clear();

}		/* -----  end of method Router::~Router  ----- */

void
Router::resize( void )
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
Router::reset_stats ( void )
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
}		/* -----  end of method Router::reset_stats  ----- */

const char* 
Router::print_stats ( void ) const
{
    std::stringstream str;
    str 
        << "\n SimpleRouter[" << node_id << "] stat_flits_in: " << stat_flits_in
        << "\n SimpleRouter[" << node_id << "] stat_flits_out: " << stat_flits_out
        << "\n SimpleRouter[" << node_id << "] stat_packets_out: " << stat_packets_out
        << "\n SimpleRouter[" << node_id << "] avg_router_pkt_latency: " << stat_total_pkt_latency_nano+0.0/stat_packets_out
        << "\n SimpleRouter[" << node_id << "] stat_last_flit_cycle: " << stat_last_flit_cycle
        << "\n";

    return str.str().c_str();
}		/* -----  end of method Router::print_stats  ----- */

void
Router::parse_config(std::map<std::string, std::string>& p)
{
    /* Update the parameters if they exist in the map */

    /* Resize the router and all sub components */
    resize();   //TODO: check that clear is doing the right thing
}

void
Router::do_ib (const Flit* hf, uint16_t p )
{
    uint16_t v = hf->pktinfo.vc;
    IB_state& msg = ib_state.at(p*r_param->vcs+v);
    msg.in_port = p;
    msg.in_channel = v;
    msg.packet_length = hf->pktinfo.sizeInFlits;
    msg.pipe_stage = IB;
    msg.mclass = (message_class_t)hf->pktinfo.mclass;
    msg.pkt_arrival_time = _TICK_NOW;

    msg.oport_list.clear();
    msg.ovc_list.clear();

    msg.src_node_id = hf->pktinfo.srcNum;
    msg.dst_node_id = hf->pktinfo.destNum;
    msg.address = p;

    // The router model assumes that RC happens in the same cycle as IB
    // Move to a different cycle is needed
    rc.at(p).push(hf);
    std::vector<uint16_t> tmp_op = rc.at(p).entries.at(v).possible_out_ports;
    std::vector<uint16_t> tmp_ovc = rc.at(p).entries.at(v).possible_out_vcs;

    msg.oport_list.assign(tmp_op.begin(), tmp_op.end());
    msg.ovc_list.assign(tmp_ovc.begin(), tmp_ovc.end());

    // check rc was successful
    assert( msg.oport_list.size() > 0
            && " RC for out r_param->ports failed!! ");
    assert( msg.ovc_list.size() > 0
            && " RC for out channel failed!! ");

    return ;
}		/* -----  end of method Router::do_ib  ----- */

void
Router::do_vca ( void )
{
    if ( !vca.is_empty() )
    {
        vca.pick_winner();
        // update vca access stats
    }

    for( uint16_t i=0; i<r_param->ports; ++i)
        for ( uint16_t ai=0; ai<vca.current_winners.at(i).size(); ++ai)
        {
            VCA_unit winner = vca.current_winners.at(i).at(ai);
            uint16_t ip = winner.in_port;
            uint16_t ic = winner.in_vc;
            uint16_t op = winner.out_port;
            uint16_t oc = winner.out_vc;

            IB_state& curr_msg= ib_state.at(ip*r_param->vcs+ic);
//            printf(" winner ps:%s occ:%d ", Router_PS_string[curr_msg.pipe_stage], in_buffer[ip].get_occupancy(ic));
            if ( curr_msg.pipe_stage == VCA_REQUESTED ) 
                // &&                  downstream_credits[op)[oc)==credits)
            {
                curr_msg.out_port = op;
                curr_msg.out_channel= oc;
                curr_msg.pipe_stage = VCA;
                // if requesting multiple outr_param->ports make sure to cancel them as
                // pkt will no longer be VCA_REQUESTED
                if(in_buffer.at(ip).get_occupancy(ic))
                {
                   swa.request(op, oc, ip, ic, _TICK_NOW);
                    curr_msg.pipe_stage = SWA_REQUESTED;
                }
            }

        }

    // If swa request failed and your stuck in vca you can request here
    for( uint16_t i=0; i<(r_param->ports*r_param->vcs); ++i)
        if( ib_state.at(i).pipe_stage == VCA)
        {
            IB_state& curr_msg = ib_state.at(i);
            uint16_t ip = curr_msg.in_port;
            uint16_t ic = curr_msg.in_channel;
            uint16_t op = curr_msg.out_port;
            uint16_t oc = curr_msg.out_channel;

            if(in_buffer.at(ip).get_occupancy(ic))
            {
                swa.request(op, oc, ip, ic, _TICK_NOW);
                curr_msg.pipe_stage = SWA_REQUESTED;
            }
        }

    for( uint16_t i=0; i<(r_param->ports*r_param->vcs); ++i)
    {
        uint16_t ip = (int)i/r_param->vcs;
        uint16_t ic = (int)i%r_param->vcs;
        if ( in_buffer.at(ip).get_occupancy(ic) && ib_state.at(i).pipe_stage == EMPTY )
        {
            Flit* data = in_buffer.at(ip).peek(ic);
            if ( data->type == Flit::HEAD )
            {
                do_ib(data, ip);
            }
            else
                rc.at(ip).push(data);
        }
    }

    /* Request VCA */
    for( uint16_t i=0; i<(r_param->ports*r_param->vcs); ++i)
        if( ib_state.at(i).pipe_stage == IB )
        {
            IB_state& tmp = ib_state.at(i);
            uint16_t ip = tmp.in_port;
            uint16_t ic = tmp.in_channel;
            uint16_t op = tmp.oport_list.at(0);
            uint16_t oc = tmp.ovc_list.at(0);
            //                if( downstream_credits[op)[oc) == credits &&
            if ( !vca.is_requested(op, oc, ip, ic) 
                 && downstream_credits.at(op).at(oc)>0 )
            {
                //                printf(" requesting vca %d-%d|%d-%d", ip, ic, op, oc);
                tmp.pipe_stage = VCA_REQUESTED;
                vca.request(op,oc,ip,ic);
                //break;      /* allow just one */
            }
        }


    return ;
}		/* -----  end of method Router::do_vca  ----- */

void
Router::do_swa ( void )
{
    /* Switch Allocation */
    for( uint16_t i=0; i<r_param->ports*r_param->vcs; ++i)
        if( ib_state.at(i).pipe_stage == SWA_REQUESTED)
            if ( !swa.is_empty())
            {
                IB_state& sa_msg = ib_state.at(i);
                uint16_t op = -1, oc = -1;
                SA_unit sa_winner;
                uint16_t ip = sa_msg.in_port;
                uint16_t ic = sa_msg.in_channel;
                op = sa_msg.out_port;
                oc= sa_msg.out_channel;
                sa_winner = swa.pick_winner(op, _TICK_NOW);

                if( sa_winner.port == ip && sa_winner.channel == ic
                    && in_buffer.at(ip).get_occupancy(ic) > 0
                    && downstream_credits.at(op).at(oc)>0 )
                {
                    //                    printf(" swa won %d-%d|%d-%d \n",ip,ic,op,oc);
                    sa_msg.pipe_stage = ST;
                }
                else
                {
                    sa_msg.pipe_stage = VCA;
                    swa.clear_requestor(op, ip,oc);
                }

            }


    return ;
}

void
Router::do_st ( void )
{
    for ( uint i=0; i<ib_state.size(); ++i)
        if( ib_state.at(i).pipe_stage==ST) 
            //            && ib_state.at(i).pkt_arrival_time+12 <= _TICK_NOW)
        {
            IB_state& curr_pkt = ib_state.at(i);
            uint16_t op = curr_pkt.out_port;
            uint16_t oc = curr_pkt.out_channel;
            uint16_t ip = curr_pkt.in_port;
            uint16_t ic = curr_pkt.in_channel;
            if ( downstream_credits.at(op).at(oc) && in_buffer.at(ip).get_occupancy(ic) )
            {

                Flit* f = in_buffer.at(ip).pull(ic);

                irisFlitEvent* ev = new irisFlitEvent;
                f->vc = oc;
                ev->type = irisRtrEvent::Flit;
                ev->flit = f;
                links.at(op)->send(ev);   

                /*  I have one slot in the next buffer less now. */
                downstream_credits.at(op).at(oc)--;

                irisFlitEvent* cr_ev = new irisFlitEvent; //(CREDIT, ic);
                cr_ev->type = irisRtrEvent::Credit;
                cr_ev->credit.vc = oc;
                cr_ev->credit.num = 1;
                links.at(ip)->send(cr_ev);

                //If the tail is going out then ib_state is cleared later
                swa.request(op, oc, ip, ic, _TICK_NOW);
                ib_state[i].pipe_stage = SWA_REQUESTED;
                stat_flits_out++;

                if ( f->type == Flit::TAIL){
                    /* Update packet stats */
                    stat_packets_out++;
                    stat_last_flit_cycle = _TICK_NOW;
                    uint16_t lat = _TICK_NOW - curr_pkt.pkt_arrival_time;
                    stat_total_pkt_latency_nano += lat;

                    curr_pkt.pipe_stage = EMPTY;
                    curr_pkt.in_port = -1;
                    curr_pkt.in_channel = -1;
                    curr_pkt.out_port = -1;
                    curr_pkt.out_channel = -1;
                    vca.clear_winner(op, oc, ip, ic);
                    swa.clear_requestor(op, ip,oc);
                    curr_pkt.oport_list.clear();
                    curr_pkt.ovc_list.clear();
                }
            }
            else
            {
                curr_pkt.pipe_stage = VCA;
                swa.clear_requestor(op, ip,oc);
            }
        }

    return ;
}		/* -----  end of method Router::do_swa  ----- */

void
Router::handle_link_arrival ( DES_Event* ev, int dir)
{
    irisFlitEvent* event = dynamic_cast<irisFlitEvent*>(ev);

    switch ( event->type )
    {
        case irisRtrEvent::Credit:
            {
                /*  Update credit information for downstream buffer
                 *  corresponding to port and vc */
                downstream_credits.at(dir).at(event->credit.vc) += event->credit.num;
                break;
            }
        case irisRtrEvent::Flit:
            {
                /* Stats update */
                stat_flits_in++;

                in_buffer.at(dir).push(event->flit);

                break;
            }
        default:
            break;
    }

    /*  Restart the clock for this router since a new flit is in */
    if ( !currently_clocking ) {
        currently_clocking = true;
        clock_handler = new SST::Clock::Handler<Router>(this, &Router::tock);
        SST::TimeConverter* tc = registerClock(ROUTER_FREQ,clock_handler, true);
        if ( !tc)
            output.fatal(CALL_INFO, -1,"Failed to restart the clock\n");
    }

    return ;
}		/* -----  end of method Router::handle_link_arrival  ----- */

//extern void dump_state_at_deadlock(void);
bool
Router::tock ( SST::Cycle_t t )
{
    /*
     * This can be used to check if a message does not move for a large
     * interval. ( eg. interval of arrival_time + 100 )
     for ( uint i=0; i<input_buffer_state.size(); ++i)
     if (input_buffer_state[i].pipe_stage != EMPTY
     && input_buffer_state[i].pipe_stage != PS_INVALID
     && input_buffer_state[i].pkt_arrival_time+100 < manifold::kernel::Manifold::NowTicks())
     {
     fprintf(stderr,"\n\nDeadlock at Router %d node %d Msg id %d Fid%d", GetComponentId(),      node_id,
     i,input_buffer_state[i].fid);
     dump_state_at_deadlock();
     }
     * */

    // calculate avg buffer occ stat every cycle
    // can make it mod some cycles for speed up
    //    for(uint i=0; i<r_param->ports; ++i)
    //        for(uint j=0; j<r_param->vcs; ++j)
    //        {
    //            stat_pp_avg_buff[i][j] += in_buffers[i].get_occupancy(j);
    //            stat_avg_buff += in_buffers[i].get_occupancy(j);
    //        }

    do_st();
    do_swa();
    do_vca();

    /* Try to unregister and register clock for simulation speedup
     * Returning true from the function registered with the clock will
     * automatically unregister the clock. 
     * Return false to stay ticked every cycle
     * */
    bool work_done = true;
    for( uint16_t i=0; i<(r_param->ports*r_param->vcs); ++i)
        if ( ib_state.at(i).pipe_stage != EMPTY )
        {
            Flit* data = in_buffer.at(ib_state[i].in_port).peek(ib_state[i].in_channel);
//            printf("%d@%lu ps:%s %d\n",i, _TICK_NOW, Router_PS_string[ib_state[i].pipe_stage], (int)data->type);
            work_done = false;
            break;
        }

    if ( work_done )
    {
        currently_clocking = false;
        empty_cycles++;
        return true;
    }

    return false;
}		/* -----  end of method Router::tock  ----- */

#endif   /* ----- #ifndef _ROUTER_CC_INC  ----- */

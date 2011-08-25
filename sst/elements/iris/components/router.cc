/*
 * =====================================================================================
//       Filename:  router.cc
 * =====================================================================================
 */

#ifndef  _ROUTER_CC_INC
#define  _ROUTER_CC_INC

#include	"router.h"
// Move this include into irisComponent
#include	"../interfaces/genericHeader.cc"

Router::Router (uint16_t nid): DES_Component() ,
                /*  Init stats */
                stat_flits_in(0), stat_flits_out(0), stat_last_flit_time_nano(0), 
                stat_last_flit_cycle(0), stat_packets_out(0), stat_total_pkt_latency_nano(0),
                heartbeat_interval(500)/* in cycles */, stat_avg_buffer_occ(0.0),
                router_fastfwd(0), node_id(nid)
{

}  /* -----  end of method Router::Router  (constructor)  ----- */

Router::~Router ()
{
    // clean up some vectors maybe
    total_buff_occ.clear();

}		/* -----  end of method Router::~Router  ----- */

inline void
Router::resize( void )
{

    /* Clear out the previous setting */
    for ( uint i=0; i<r_param.ports; i++)
        downstream_credits.at(i).clear();
    downstream_credits.clear();
    ib_state.clear();
    in_buffer.clear();
    rc.clear();
    /* ************* END of clean up *************/

    ib_state.resize(r_param.ports*r_param.vcs);

    rc.insert(rc.begin(), r_param.ports, GenericRC());
    for ( uint i=0; i<r_param.ports; i++)
        rc.at(i).node_id = node_id;

    in_buffer.insert(in_buffer.begin(), r_param.ports, GenericBuffer());

    downstream_credits.resize(r_param.ports);
    for ( uint i=0; i<r_param.ports; i++)
        downstream_credits.at(i).insert(downstream_credits.at(i).begin(), r_param.vcs, r_param.credits);
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

std::string
Router::print_stats ( void ) const
{
    std::stringstream str;
    str 
        << "\n SimpleRouter[" << node_id << "] stat_flits_in: " << stat_flits_in
        << "\n SimpleRouter[" << node_id << "] stat_flits_out: " << stat_flits_out
        << "\n";

    return str.str();
}		/* -----  end of method Router::print_stats  ----- */

void
Router::parse_config(std::map<std::string, std::string>& p)
{
    /* Update the parameters if they exist in the map */

    /* Resize the router and all sub components */
    resize();   //TODO: check that clear is doing the right thing
}

void
Router::do_ib ( HeadFlit* hf, uint16_t p, uint16_t v )
{
    ib_state.at(p*r_param.vcs+v).in_port = p;
    ib_state.at(p*r_param.vcs+v).in_channel = p;
    ib_state.at(p*r_param.vcs+v).packet_length = hf->pkt_length;
    ib_state.at(p*r_param.vcs+v).pipe_stage = IB;
    ib_state.at(p*r_param.vcs+v).mclass = hf->mclass;
    ib_state.at(p*r_param.vcs+v).pkt_arrival_time = _TICK_NOW;

    ib_state.at(p*r_param.vcs+v).oport_list.clear();
    ib_state.at(p*r_param.vcs+v).ovc_list.clear();

    ib_state.at(p*r_param.vcs+v).src_node_id = hf->src_node_id;
    ib_state.at(p*r_param.vcs+v).dst_node_id = hf->dst_node_id;
    ib_state.at(p*r_param.vcs+v).address = p;

    // The router model assumes that RC happens in the same cycle as IB
    // Move to a different cycle is needed
    rc.at(p).push(hf);
    std::vector<uint16_t> tmp_op = rc.at(p).entries.at(v).possible_out_ports;
    std::vector<uint16_t> tmp_ovc = rc.at(p).entries.at(v).possible_out_vcs;

    ib_state.at(p*r_param.vcs+v).oport_list.assign(tmp_op.begin(), tmp_op.end());
    ib_state.at(p*r_param.vcs+v).ovc_list.assign(tmp_ovc.begin(), tmp_ovc.end());

    // check rc was successful
    assert( ib_state.at(p*r_param.vcs+v).oport_list.size() > 0
            && " RC for out r_param.ports failed!! ");
    assert( ib_state.at(p*r_param.vcs+v).ovc_list.size() > 0
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

    for( uint16_t i=0; i<r_param.ports; i++)
        for ( uint16_t ai=0; ai<vca.current_winners.at(i).size(); ai++)
        {
            VCA_unit winner = vca.current_winners.at(i).at(ai);
            uint16_t ip = winner.in_port;
            uint16_t ic = winner.in_vc;
            uint16_t op = winner.out_port;
            uint16_t oc = winner.out_vc;

            uint16_t msgid = ip*r_param.vcs+ic;
            if ( ib_state.at(msgid).pipe_stage == VCA_REQUESTED ) // &&                  downstream_credits[op)[oc)==credits)
            {
                ib_state.at(msgid).out_port = op;
                ib_state.at(msgid).out_channel= oc;
                ib_state.at(msgid).pipe_stage = VCA;
                // if requesting multiple outr_param.ports make sure to cancel them as
                // pkt will no longer be VCA_REQUESTED
            }

        }

    for( uint16_t i=0; i<(r_param.ports*r_param.vcs); i++)
        if( ib_state.at(i).pipe_stage == VCA)
        {
            uint16_t ip = ib_state.at(i).in_port;
            uint16_t ic = ib_state.at(i).in_channel;
            uint16_t op = ib_state.at(i).out_port;
            uint16_t oc = ib_state.at(i).out_channel;

            if(in_buffer.at(ip).get_occupancy(ic))
            {
                swa.request(op, oc, ip, ic, _TICK_NOW);
                ib_state.at(i).pipe_stage = SWA_REQUESTED;
            }
        }

    for( uint16_t i=0; i<(r_param.ports*r_param.vcs); i++)
    {
        uint16_t ip = (int)i/r_param.vcs;
        uint16_t ic = (int)i%r_param.vcs;
        if ( in_buffer.at(ip).get_occupancy(ic) && ib_state.at(i).pipe_stage == EMPTY )
        {
            Flit* data = in_buffer.at(ip).peek(ic);
            if ( data->type == HEAD )
                do_ib(static_cast<HeadFlit*>(data), ip, ic);
            else
                rc.at(ip).push(data);
        }
    }

    /* Request VCA */
    for( uint16_t i=0; i<(r_param.ports*r_param.vcs); i++)
        if( ib_state.at(i).pipe_stage == IB )
        {
            uint16_t ip = ib_state.at(i).in_port;
            uint16_t ic = ib_state.at(i).in_channel;
            uint16_t op = ib_state.at(i).oport_list.at(0);
            uint16_t oc = -1;
            for ( uint16_t ab=0; ab<ib_state.at(i).ovc_list.size();ab++)
            {
                oc = ib_state.at(i).ovc_list.at(ab);
                //                if( downstream_credits[op)[oc) == credits &&
                if ( !vca.is_requested(op, oc, ip, ic) )
                {
                    ib_state.at(i).pipe_stage = VCA_REQUESTED;
                    vca.request(op,oc,ip,ic);
                    //                    printf("Node: %d VCA_REQUESTED for pkt %llx\n",node_id,                   ib_state.at(i).address);
                    //break;      /* allow just one */
                }
            }
        }


    return ;
}		/* -----  end of method Router::do_vca  ----- */

void
Router::do_swa ( void )
{
    /* Switch Allocation */
    for( uint16_t i=0; i<r_param.ports*r_param.vcs; i++)
        if( ib_state.at(i).pipe_stage == SWA_REQUESTED)
            if ( !swa.is_empty())
            {
                uint16_t op = -1, oc = -1;
                SA_unit sa_winner;
                uint16_t ip = ib_state.at(i).in_port;
                uint16_t ic = ib_state.at(i).in_channel;
                op = ib_state.at(i).out_port;
                oc= ib_state.at(i).out_channel;
                sa_winner = swa.pick_winner(op, _TICK_NOW);

                bool alloc_done = false;
                if(ib_state.at(i).sa_head_done)
                {
                    if( sa_winner.port == ip && sa_winner.channel == ic
                        && in_buffer.at(ip).get_occupancy(ic) > 0
                        && downstream_credits.at(op).at(oc)>0 )
                    {
                        ib_state.at(i).pipe_stage = ST;
                        alloc_done = true;
                    }
                }
                else
                {
#if MULTI_PKT
                    if( sa_winner.port == ip && sa_winner.channel == ic
                        && ((ib_state.at(i).pkt_length < r_param.credits &&
                             downstream_credits.at(op).at(oc) >= input_buffer_state.at(i).pkt_length)
                            || (downstream_credits.at(op).at(oc)==r_param.credits)))
#else
                        if( sa_winner.port == ip && sa_winner.channel == ic
                            && downstream_credits.at(op).at(oc)==r_param.credits )
#endif
                        {
                            ib_state.at(i).sa_head_done = true;
                            ib_state.at(i).pipe_stage = ST;
                            alloc_done = true;
                        }

                }
                if ( !alloc_done )
                {
                    ib_state.at(i).pipe_stage = VCA;
                    swa.clear_requestor(op, ip,oc);
                }

            }

    return ;
}		/* -----  end of method Router::do_st  ----- */

void
Router::do_st ( void )
{
     /* Switch traversal */
    for( uint16_t i=0; i<r_param.ports*r_param.vcs; i++)
        if( ib_state.at(i).pipe_stage == ST )
        {
            uint16_t op = ib_state.at(i).out_port;
            uint16_t oc = ib_state.at(i).out_channel;
            uint16_t ip = ib_state.at(i).in_port;
            uint16_t ic= ib_state.at(i).in_channel;
            if( in_buffer.at(ip).get_occupancy(ic)> 0
                && downstream_credits.at(op).at(oc)>0 )
            {
                Flit* f = in_buffer.at(ip).pull(ic);
                f->virtual_channel = oc;

                //If the tail is going out then its cleared later
                swa.request(op, oc, ip, ic, _TICK_NOW);
                ib_state.at(i).pipe_stage = SWA_REQUESTED;

                if( f->type == TAIL || f->pkt_length == 1)
                {
                    /* Update packet stats */
                    stat_packets_out++;

                    ib_state.at(i).is_valid = true;
                    ib_state.at(i).pipe_stage = EMPTY;
                    ib_state.at(i).in_port = -1;
                    ib_state.at(i).in_channel = -1;
                    ib_state.at(i).out_port = -1;
                    ib_state.at(i).out_channel = -1;
                    vca.clear_winner(op, oc, ip, ic);
                    swa.clear_requestor(op, ip,oc);
                    ib_state.at(i).oport_list.clear();
                    ib_state.at(i).ovc_list.clear();
                }

                stat_flits_out++;
                // update xbar access stats

                /* 
                IrisEvent* ev = new IrisEvent();
                ld->type = FLIT;
                ld->src = this->GetComponentId();
                f->virtual_channel = oc;
                ld->f = f;
                ld->vc = oc;
                 * */

                //Not using send then identify between router and interface
                //Use manifold send here to push the ld obj out

                // DES_Send( ev);    //schedule cannot be used here as the       component is not on the same LP

                /* Send a credit back and update buffer state for the
                 * downstream router buffer */
                downstream_credits.at(op).at(oc)--;

                /* 
                LinkData* ldc = new LinkData();
                ldc->type = CREDIT;
                ldc->src = this->GetComponentId();
                ldc->vc = ic;

                Send(signal_outr_param.ports.at(ip),ldc);
                */
                stat_last_flit_cycle = _TICK_NOW;

            }
            else
            {
                ib_state.at(i).pipe_stage = VCA;
                swa.clear_requestor(op, ip,oc);
            }
        }

    return ;
}		/* -----  end of method Router::do_swa  ----- */

void
Router::handle_link_arrival ( IrisEvent* event )
{
    switch ( event->etype )
    {
        case CREDIT:
            {
                /*  Update credit information for downstream buffer
                 *  corresponding to port and vc */
                uint16_t ip = 0;
                downstream_credits.at(ip).at(event->channel)++;
                break;
            }
        case FLIT:
            {
                // cast to a flit event f_ev
                IrisEvent* f_ev = NULL;

                /* Stats update */
                stat_flits_in++;

//                if ( f_ev->f->type == TAIL ) stat_packets_in++;

                // Get the port from the link name or pass a parameter
                uint16_t ip = 0;

                //push the flit into the buffer. Init for buffer state done
                //inside do_input_buffering
//                in_buffer.at(ip).push(f_ev->f, f_ev->f->virtual_channel);
                break;
            }
        default:
            break;
    }
    return ;
}		/* -----  end of method Router::handle_link_arrival  ----- */

//extern void dump_state_at_deadlock(void);
void
Router::tock ( void )
{
    /*
     * This can be used to check if a message does not move for a large
     * interval. ( eg. interval of arrival_time + 100 )
     for ( uint i=0; i<input_buffer_state.size(); i++)
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
//    for(uint i=0; i<r_param.ports; i++)
//        for(uint j=0; j<r_param.vcs; j++)
//        {
//            stat_pp_avg_buff[i][j] += in_buffers[i].get_occupancy(j);
//            stat_avg_buff += in_buffers[i].get_occupancy(j);
//        }

    do_st();
    do_swa();
    do_vca();

    return ;
}		/* -----  end of method Router::tock  ----- */

#endif   /* ----- #ifndef _ROUTER_CC_INC  ----- */

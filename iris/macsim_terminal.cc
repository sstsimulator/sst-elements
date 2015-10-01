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
//       Filename:  terminal.cc
 * =====================================================================================
 */

#ifndef  _MACSIM_TERMINAL_CC_INC
#define  _MACSIM_TERMINAL_CC_INC

#include <sst_config.h>
#include	"macsim_terminal.h"

//Include macsim main SST class here adding dummy for now
#include        "ex_macsim_component.h"
#include <sst/core/link.h>

using namespace SST::Iris;

mem_req_s* 
MacsimTerminal::check_queue( void )
{
    if ( recvQ.empty())
        return NULL;

    mem_req_s *ret = recvQ.front();
    recvQ.pop();

    return ret;
}

void
MacsimTerminal::handle_interface_link_arrival ( DES_Event* ev ,int port_id )
{
    irisRtrEvent* event = dynamic_cast<irisRtrEvent*>(ev);

    switch ( event->type )
    {
        case irisRtrEvent::Credit:
            {
                if( event->credit.vc > vcs )
                {
                    iris_panic(" macsim terminal got credit from interface for non existing vc!!");
                }
                else
                    interface_credits.at(event->credit.vc) = true;

                break;
            }
        case irisRtrEvent::Packet:
            {
                owner->terminal_recv = false;

                // send a credit back to the interface
                irisRtrEvent* credit_event = new irisRtrEvent; 
                credit_event->type = irisRtrEvent::Credit;
                credit_event->credit.vc = event->credit.vc;
                credit_event->credit.num = 1;      // if we do use flits then update this
                owner->interface_link->send(credit_event);   

                // update macsim
                // @ Genie FIXME: MEM_NOC_DONE and mem_req_s are types in macsim
                // need includes for them
                mem_req_s* req = static_cast<mem_req_s*>(event->packet->req);
                req->m_state = MEM_NOC_DONE; 
                recvQ.push(req);
                delete (&event->packet);
                return;

                break;
            }
    }
    delete event;
}		

bool
MacsimTerminal::send_packet(mem_req_s *req)
{
    bool is_buffer_empty = false;
    uint send_buffer_id = -1;
    
    uint i = (req->m_noc_type == PROC_REQ) ? 0 : vcs/2;
    for ( int count=0; count < vcs/2 ; count++,i++ )
    {
        if ( interface_credits.at(i) )
        {
            is_buffer_empty = true;
            send_buffer_id = i;
            break;
        }
    }
	
    if ( is_buffer_empty )
    {
        irisNPkt* np = new irisNPkt;
        np->srcNum = node_id; 
        np->destNum = req->m_msg_dst;
        np->req = req;

        //np->dst_component_id = 3;       // can use this for further splitting at the interface
        np->address = req->m_addr; 
        //FIXME: Genie fix defn for req  
        np->mclass = (message_class_t)req->m_noc_type;	

        np->sizeInFlits = 4;    // FIXME: compute number of link cycles and update
        //        np->proc_buffer_id = send_buffer_id;

        //FIXME: Genie this is for stat computation
        //just set it to the current clock either from sst or macsim
        //        np->sending_time = _TICK_NOW;

        interface_credits[send_buffer_id] = false;

        irisRtrEvent* pkt_event = new irisRtrEvent;
        pkt_event->type = irisRtrEvent::Packet;
        pkt_event->packet = np;
        owner->interface_link->send(pkt_event);  

        //FIXME: memory leaks check what np should do
        return true;

    } else
        return false;
}

#endif   /* ----- #ifndef _MACSIM_TERMINAL_CC_INC  ----- */

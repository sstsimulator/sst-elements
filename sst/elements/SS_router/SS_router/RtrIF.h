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


// Copyright 2007 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2007, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef ED_RTRIF_H
#define ED_RTRIF_H
#include <sst/core/sst_types.h>

#include <deque>
#include <sstream>

#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/params.h>


#include "SS_network.h"

namespace SST {
namespace SS_router {

class RtrIF : public Component {
public:
    RtrIF( ComponentId_t id, Params& params ) :
        Component(id),
        rtrCountP(0),
        num_vcP(2),
        out(Simulation::getSimulation()->getSimulationOutput())
    {
        int num_tokens = 512;


        // if ( params.find( "id" ) == params.end() ) {
        //     out.fatal(CALL_INFO, -1,"couldn't find routerID\n" );
        // }
        m_id = params.find_integer( "id" );
        if ( m_id == -1 ) {
            out.fatal(CALL_INFO, -1,"couldn't find routerID\n" );
        }

        if ( params.find("clock") != params.end() ) {
            frequency = params["clock"];
        }

        // if ( params.find( "num_vc" ) != params.end() ) {
        //     num_vcP = str2long( params["num_vc"] );
        // }

        if ( params.find( "num_vc" ) != params.end() ) {
            num_vcP = params.find_integer("num_vc");
        }

        if ( params.find( "Node2RouterQSize_flits" ) != params.end() ) {
            num_tokens = params.find_integer("Node2RouterQSize_flits");
        }

	m_rtrLink = configureLink( "rtr", frequency, new Event::Handler<RtrIF>(this,&RtrIF::processEvent) );
    assert(m_rtrLink);

	registerClock( frequency, new Clock::Handler<RtrIF>(this, &RtrIF::clock), false );

        for ( unsigned int i=0; i < num_vcP; i++ ) {
            toNicMapP[i] = new ToNic();
            toRtrMapP[i] = new ToRtr(num_tokens,toRtrQP);
        }
    }

    bool toNicQ_empty(unsigned int vc)
    {
        if ( vc >= num_vcP ) out.fatal(CALL_INFO, -1,"vc=%d\n",vc);
        return toNicMapP[vc]->empty();
    }

    RtrEvent *toNicQ_front(unsigned int vc)
    {
        if ( vc >= num_vcP ) out.fatal(CALL_INFO, -1,"vc=%d\n",vc);
        return toNicMapP[vc]->front();
    }

    void toNicQ_pop(unsigned int vc)
    {
        if ( vc >= num_vcP ) out.fatal(CALL_INFO, -1,"vc=%d\n",vc);
        returnTokens2Rtr( vc, toNicMapP[vc]->front()->packet.sizeInFlits );
        toNicMapP[vc]->pop_front();
    }

    bool send2Rtr( RtrEvent *event)
    {
        networkPacket* pkt = &event->packet;
        if ( pkt->vc >= (int) num_vcP ) out.fatal(CALL_INFO, -1,"vc=%d\n",pkt->vc);
//  	printf("%5d: Sending to %d @ %lu\n",m_id,pkt->destNum,getCurrentSimTimeNano()); 

// 	// Translate destination into x,y,z
// 	event->mesh_loc[0] = pkt->destNum % x;
// 	event->mesh_loc[0] = ( pkt->destNum / x ) % y;
// 	event->mesh_loc[0] = nodeNumber / ( x * y );

	bool retval = toRtrMapP[pkt->vc]->push( event );
        if ( retval ) {
            sendPktToRtr( toRtrQP.front() );
            toRtrQP.pop_front();
        }
        return retval;
    }

    void finish() { }


private:
    bool rtrWillTake( int vc, int numFlits )
    {
        if ( vc >= (int) num_vcP ) out.fatal(CALL_INFO, -1,"\n");
        return toRtrMapP[vc]->willTake( numFlits );
    }
    
    void processEvent( Event* e)
    {
        RtrEvent* event = static_cast<RtrEvent*>(e);
// 	printf("%5d:  got an event from %d @ %lu\n",m_id,event->packet.srcNum,getCurrentSimTimeNano());
	

        switch ( event->type ) {
        case RtrEvent::Credit:
            returnTokens2Nic( event->credit.vc, event->credit.num );
            delete event;
            break;

        case RtrEvent::Packet:
            send2Nic( event );
            break;

        default:
            out.fatal(CALL_INFO, -1,"unknown type %d\n",event->type);
        }
    }

    bool clock( Cycle_t cycle)
    {
        rtrCountP = (rtrCountP >= 0) ? 0 : rtrCountP + 1;

//         if ( ! toRtrQP.empty() ) {
//             sendPktToRtr( toRtrQP.front());
//             toRtrQP.pop_front();
//         }
// 	else {
// 	    // Nothing to be done, remove clock handler
// 	    return true;
// 	}
        return false;
    }

    void send2Nic( RtrEvent* event )
    {
        networkPacket *pkt = &event->packet; 

        pkt->vc = RTR_2_NIC_VC(pkt->vc);

        if ( pkt->vc >= (int) num_vcP ) {
            out.fatal(CALL_INFO, -1,"vc=%d pkt=%p\n",pkt->vc,pkt);
        }

        toNicMapP[pkt->vc]->push_back( event );
    }

    void returnTokens2Nic( int vc, uint32_t num )
    {
        if ( vc >= (int) num_vcP ) out.fatal(CALL_INFO, -1,"\n");
        toRtrMapP[vc]->returnTokens( num );
    }

    void returnTokens2Rtr( int vc, uint numFlits ) 
    {
        RtrEvent* event = new RtrEvent;

        event->type = RtrEvent::Credit;
        event->credit.num = numFlits;
        event->credit.vc = vc;
        m_rtrLink->send( event ); 
    }

    void sendPktToRtr( RtrEvent* event ) 
    {
        networkPacket* pkt = &event->packet;


        event->type = RtrEvent::Packet;
        event->packet = *pkt;
        int lat = reserveRtrLine(pkt->sizeInFlits);
        m_rtrLink->send( lat, event ); 
    }

    int reserveRtrLine (int cyc)
    {
        int ret = (rtrCountP <= 0) ? -rtrCountP : 0;
        rtrCountP -= cyc;
        return ret;
    }

    typedef std::deque<RtrEvent*> ToNic;

    class ToRtr {
    public:
        ToRtr( int num_tokens, deque<RtrEvent*> &eventQ ) :
                tokensP(num_tokens), eventQP(eventQ) {}

        bool push( RtrEvent* event) {
            networkPacket* pkt = &event->packet;
            if ( pkt->sizeInFlits > (unsigned int) tokensP ) return false;
            tokensP -= pkt->sizeInFlits;
            eventQP.push_back(event);
            return true;
        }

        int size() {
            return eventQP.size();
        }

        bool willTake( int numFlits ) {
            return (numFlits <= tokensP );
        }

        void returnTokens( int num ) {
            tokensP += num;
        }

    private:
        int tokensP;
        deque<RtrEvent*> &eventQP;
    };

    int rtrCountP;
    map<int,ToNic*>         toNicMapP;
    map<int,ToRtr*>         toRtrMapP;

    uint                    num_vcP;

    deque<RtrEvent*>        toRtrQP;

    Link*                   m_rtrLink;

protected:
    int                     m_id;
    std::string             frequency;
    Output &                out;
};

}
}

#endif

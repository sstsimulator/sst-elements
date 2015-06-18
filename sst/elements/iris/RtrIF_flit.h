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

#include <sstream>
#include <deque>

#include <sst/core/sst_types.h>
#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include	"SST_interface.h"

#define RTRIF_DBG 1 
#ifndef RTRIF_DBG
#define RTRIF_DBG 0
#endif

#define db_RtrIF(fmt,args...) \
    m_dbg.write( "%s():%d: "fmt, __FUNCTION__, __LINE__, ##args)
#define RTR_2_NIC_VC(vcc) ( vcc/2 )

namespace SST {
namespace Iris {

class Flit_conv {
    public:
        Flit_conv();
        ~Flit_conv();

        void convert_pkt(irisNPkt& pkt, std::vector<Flit*>& flitq)
        {
            assert(&pkt != NULL && "Flit_conv pkt to convert is NULL");
            int16_t noflits = pkt.sizeInFlits;
            assert(noflits >= 2);

            //Populate the head flit with the network packet
            Flit* hf = new Flit(pkt.vc, Flit::HEAD);
            hf->pktinfo= pkt;
            flitq.push_back(hf);
            --noflits;

            //populate body flits
            while ( noflits != 1 ){
                Flit* f = new Flit(pkt.vc, Flit::BODY);
                flitq.push_back(f);
                --noflits;
            }

            // populate Tail flit
            if( noflits == 1){
                Flit* f = new Flit(pkt.vc, Flit::TAIL);
                flitq.push_back(f);
            }

            return ;

        }
};

class iris_RtrIF : public Component {
public:
    iris_RtrIF( ComponentId_t id, Params& params ) :
        Component(id),
        rtrCountP(0),
        num_vcP(2),
        m_log(Simulation::getSimulation()->getSimulationOutput()),
        stat_avg_pkt_lat(0),
        stat_total_pkts_recv(0)
    {
        int num_tokens = 512;


        if ( params.find( "id" ) == params.end() ) {
            m_log.fatal(CALL_INFO, -1,"couldn't find routerID\n" );
        }
        m_id = params.find_integer("id");

        if ( params.find("clock") != params.end() ) {
            frequency = params["clock"];
        }

        if ( params.find( "num_vc" ) != params.end() ) {
            num_vcP = params.find_integer("num_vc");
        }

        if ( params.find( "Node2RouterQSize_flits" ) != params.end() ) {
            num_tokens = params.find_integer("Node2RouterQSize_flits");
        }

        for( uint16_t i=0; i<num_vcP; ++i){
            std::vector<Flit*> tmp;
            flit_outq.insert(std::make_pair(i, tmp));
        }

	m_rtrLink = configureLink( "rtr", frequency, new Event::Handler<iris_RtrIF>(this,&iris_RtrIF::processEvent) );
	registerClock( frequency, new Clock::Handler<iris_RtrIF>(this, &iris_RtrIF::clock), false );


        for ( unsigned int i=0; i < num_vcP; i++ ) {
            toNicMapP[i] = new ToNic();
            toRtrMapP[i] = new ToRtr(num_tokens,toRtrQP);
        }
    }

    bool toNicQ_empty(unsigned int vc)
    {
        if ( vc >= num_vcP ) m_log.fatal(CALL_INFO, -1,"vc=%d\n",vc);
        return toNicMapP[vc]->empty();
    }

    irisRtrEvent *toNicQ_front(unsigned int vc)
    {
        if ( vc >= num_vcP ) m_log.fatal(CALL_INFO, -1,"vc=%d\n",vc);
        return toNicMapP[vc]->front();
    }

    void toNicQ_pop(unsigned int vc)
    {
        if ( vc >= num_vcP ) m_log.fatal(CALL_INFO, -1,"vc=%d\n",vc);
        returnTokens2Rtr( vc, toNicMapP[vc]->front()->packet.sizeInFlits );
        toNicMapP[vc]->pop_front();
    }

    bool send2Rtr( irisRtrEvent *event)
    {
        irisNPkt* pkt = &event->packet;
        if ( pkt->vc >= (int) num_vcP ) m_log.fatal(CALL_INFO, -1,"vc=%d\n",pkt->vc);
        pkt->sending_time=getCurrentSimTimeNano();
        //  	printf("%5d: Sending to %d @ %lu\n",m_id,pkt->destNum,getCurrentSimTimeNano()); 

        // 	// Translate destination into x,y,z
        // 	event->mesh_loc[0] = pkt->destNum % x;
        // 	event->mesh_loc[0] = ( pkt->destNum / x ) % y;
        // 	event->mesh_loc[0] = nodeNumber / ( x * y );

        bool retval = toRtrMapP[pkt->vc]->push( event );
        if ( retval ) {
            sendPktToRtr(pkt->vc, toRtrQP.front() );
            toRtrQP.pop_front();
        }
        return retval;
    }

    void finish()
    { 
        fprintf(stderr,"\n RtrIF Node %d\n", m_id );
        fprintf(stderr," Total no of pkts recv %lu \n",stat_total_pkts_recv); 
        fprintf(stderr," Avg pkt latency %0.2f \n",stat_avg_pkt_lat+0.0/stat_total_pkts_recv); 
    }

private:
    bool rtrWillTake( int vc, int numFlits )
    {
        if ( vc >= (int) num_vcP ) m_log.fatal(CALL_INFO, -1,"\n");
        return toRtrMapP[vc]->willTake( numFlits );
    }

    void processEvent( Event* e)
    {
        irisFlitEvent* event = static_cast<irisFlitEvent*>(e);
        // 	printf("%5d:  got an event from %d @ %lu\n",m_id,event->packet.srcNum,getCurrentSimTimeNano());


        switch ( event->type ) {
            case irisRtrEvent::Credit:
                returnTokens2Nic( event->credit.vc, event->credit.num );
                delete event;
                break;

            case irisRtrEvent::Flit:
//                if ( m_id == 0)
//                    printf("%d@%lu: got flit\t type:%d\n",m_id, getCurrentSimTimeNano(), (int)event->flit->type);
                if ( event->flit->type == Flit::HEAD)
                {
                    irisRtrEvent* ev = new irisRtrEvent;
                    ev->packet = event->flit->pktinfo;
                    send2Nic( ev );
                }
                delete event;
                break;

            default:
                m_log.fatal(CALL_INFO, -1,"unknown type %d\n",event->type);
        }
    }

    bool clock( Cycle_t cycle)
    {
        rtrCountP = (rtrCountP >= 0) ? 0 : rtrCountP + 1;

        for ( uint16_t vc = 0; vc < num_vcP; ++vc ) {
            if ( !flit_outq[vc].empty())
                sendPktToRtr(vc, NULL);
        }

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

    void send2Nic( irisRtrEvent* event )
    {
        irisNPkt *pkt = &event->packet; 
        stat_avg_pkt_lat += (getCurrentSimTimeNano()-pkt->sending_time);
        stat_total_pkts_recv++;

        pkt->vc = RTR_2_NIC_VC(pkt->vc);

        if ( pkt->vc >= (int) num_vcP ) {
            m_log.fatal(CALL_INFO, -1,"vc=%d pkt=%p\n",pkt->vc,pkt);
        }

        toNicMapP[pkt->vc]->push_back( event );
    }

    void returnTokens2Nic( int vc, uint32_t num )
    {
        if ( vc >= (int) num_vcP ) m_log.fatal(CALL_INFO, -1," vc is %d and num_vcP is %d \n", vc, num_vcP);
        toRtrMapP[vc]->returnTokens( num );
    }

    void returnTokens2Rtr( int vc, uint numFlits ) 
    {
        irisFlitEvent* event = new irisFlitEvent;

        event->type = irisRtrEvent::Credit;
        event->credit.num = numFlits;
        event->credit.vc = vc;
        m_rtrLink->send( event );   
    }

    void sendPktToRtr( uint16_t vc, irisRtrEvent* event ) 
    {
        if ( event ){
            irisNPkt* pkt = &event->packet;
            flit_conv.convert_pkt(*pkt, this->flit_outq[vc]);
            delete event;
        }
        if ( !flit_outq[vc].empty() ){
            irisFlitEvent* fe = new irisFlitEvent();
            fe->type = irisRtrEvent::Flit;
            fe->flit = (flit_outq[vc].front());
//            if (m_id ==1)
//                printf(" Send flit of type:%d @%lu\n",(int)fe->flit->type, getCurrentSimTimeNano());
            flit_outq[vc].erase(flit_outq[vc].begin());
            m_rtrLink->send( fe);   
        }
        //            int lat = reserveRtrLine(pkt->sizeInFlits);
        //            m_rtrLink->send( lat, event );
    }

    int reserveRtrLine (int cyc)
    {
        int ret = (rtrCountP <= 0) ? -rtrCountP : 0;
        rtrCountP -= cyc;
        return ret;
    }

    typedef std::deque<irisRtrEvent*> ToNic;

    class ToRtr {
        public:
            ToRtr( int num_tokens, std::deque<irisRtrEvent*> &eventQ ) :
                tokensP(num_tokens), eventQP(eventQ) {}

            bool push( irisRtrEvent* event) {
                irisNPkt* pkt = &event->packet;
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
            std::deque<irisRtrEvent*> &eventQP;
    };

    int rtrCountP;
    std::map<int,ToNic*>         toNicMapP;
    std::map<int,ToRtr*>         toRtrMapP;

    uint                    num_vcP;

    std::deque<irisRtrEvent*>        toRtrQP;

    Link*                   m_rtrLink;
    static Flit_conv flit_conv;

protected:
    int                     m_id;
    std::string             frequency;
    Output                 &m_log;

private:
    uint64_t stat_avg_pkt_lat;
    uint64_t stat_total_pkts_recv;
    std::map<uint16_t, std::vector<Flit*> > flit_outq;
};

}
}
#endif

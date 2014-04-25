// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


/* Copyright 2007 Sandia Corporation. Under the terms
 of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
 Government retains certain rights in this software.

 Copyright (c) 2007, Sandia Corporation
 All rights reserved.

 This file is part of the SST software package. For license
 information, see the LICENSE file in the top level directory of the
 distribution. */

#include <sst_config.h>
#include "sst/core/serialization.h"

#include <sstream>

#include "sst/core/debug.h"

#include "SS_router.h"
#include "SS_routerInternals.cpp"
#include "SS_routerEvent.cpp"

using namespace std;
using namespace SST::SS_router;

const char* SST::SS_router::LinkNames[] = {
    "xPos",
    "xNeg",
    "yPos",
    "yNeg",
    "zPos",
    "zNeg",
    "nic",
};

//: Router function to pass tokens back
// When tokens are returned, this might cause an output queue to become ready to accept data
void SS_router::updateToken_flits( int link, int vc, int flits )
{
    m_dbg.output(CALL_INFO, "%lld: link %d return %d flit token to rtr %d, vc %d\n",
              cycle(), link, flits, routerID, vc);

    int old_tokens = outLCB[link].vcTokens[vc];

    if ( link == ROUTER_HOST_PORT ) {
        vc =  NIC_2_RTR_VC( vc );
    }

    outLCB[link].vcTokens[vc] += flits;

    if (old_tokens < MaxPacketSize) {
        if (outLCB[link].vcTokens[vc] >= MaxPacketSize) {
            outLCB[link].ready_vc_count++;
            if (!outLCB[link].internal_busy)
                ready_oLCB = true;
        }
    }
}

void SS_router::returnToken_flits (int dir, int flits, int vc) {

    m_dbg.output(CALL_INFO, "dir=%d flits=%d vc=%d\n",dir,flits,vc );
    RtrEvent* event = new RtrEvent;
    event->type  = RtrEvent::Credit;
    if ( dir == ROUTER_HOST_PORT ) {
        vc = RTR_2_NIC_VC( vc );
    }

    event->credit.num = flits;
    event->credit.vc = vc;

    linkV[dir]->send( event ); 
}

//: Constructor
SS_router::SS_router( ComponentId_t id, Params& params ) :
        Component( id ),
        overheadMultP(1.5),
        oLCB_maxSize_flits(512),
        dumpTables(false),
        clock_count(0),
        m_cycle(0)
{
    int tmp;
    //printParams(params);

    routerID = params.find_integer("id");
    if ( routerID == -1 ) {
        _abort(SS_router,"couldn't find routerID\n" );
    }

    bool do_dbg = !params.find_string("debug", "no").compare("yes");
    bool do_log = !params.find_string("info", "no").compare("yes");

    std::ostringstream idStr;
    idStr << "SS_Router: " << routerID << ":";

    m_dbg.init(idStr.str(), 0, 0, do_dbg ? Output::STDOUT : Output::NONE);
    m_log.init(idStr.str(), 0, 0, do_log ? Output::STDOUT : Output::NONE);


    m_dbg.output(CALL_INFO, "this=%p id=%lu\n",this,id);

    sprintf (LINK_DIR_STR[LINK_POS_X], "POSX");
    sprintf (LINK_DIR_STR[LINK_POS_Y], "POSY");
    sprintf (LINK_DIR_STR[LINK_POS_Z], "POSZ");
    sprintf (LINK_DIR_STR[LINK_NEG_X], "NEGX");
    sprintf (LINK_DIR_STR[LINK_NEG_Y], "NEGY");
    sprintf (LINK_DIR_STR[LINK_NEG_Z], "NEGZ");

    for (int ln = 0; ln < ROUTER_NUM_LINKS+1; ln++) {
        //No links are set up yet
        rx_netlinks[ln] = NULL;
        tx_netlinks[ln] = NULL;
    }

    iLCBLat = params.find_integer("iLCBLat");
    if ( iLCBLat == -1 ) {
        _abort(SS_router,"couldn't find iLCBLat\n" );
    }

    oLCBLat = params.find_integer("oLCBLat");
    if ( oLCBLat == -1 ) {
        _abort(SS_router,"couldn't find oLCBLat\n" );
    }

    routingLat = params.find_integer("routingLat");
    if ( routingLat == -1 ) {
        _abort(SS_router,"couldn't find routingLat\n" );
    }

    iQLat = params.find_integer("iQLat");
    if ( iQLat == -1 ) {
        _abort(SS_router,"couldn't find iQLat\n" );
    }

    tmp = params.find_integer("OutputQSize_flits");
    if ( tmp == -1 ) {
        _abort(SS_router,"couldn't find OutputQSize_flits\n" );
    }

    m_log.output("OutputQSize_flits=%d\n",tmp);

    for ( int i = 0; i < ROUTER_NUM_LINKS; i++ ) {
        rtrOutput_maxQSize_flits[i] = tmp;
    }

    tmp = params.find_integer("InputQSize_flits");
    if ( tmp == -1 ) {
        _abort(SS_router,"couldn't find InputQSize_flits\n" );
    }
    m_log.output("InputQSize_flits=%d\n",tmp);

    for ( int i = 0; i < ROUTER_NUM_LINKS; i++ ) {
        rtrInput_maxQSize_flits[i] = tmp;
    }

    tmp = params.find_integer("Router2NodeQSize_flits");
    if ( tmp == -1 ) {
        _abort(SS_router,"couldn't find Router2NodeQSize_flits\n" );
    }

    m_log.output("Router2NodeQSize_flits=%d\n",tmp);

    rtrOutput_maxQSize_flits[ROUTER_HOST_PORT] = tmp;
    rtrInput_maxQSize_flits[ROUTER_HOST_PORT] = tmp;

    debug_interval = params.find_integer( "debugInterval", 0 );

    dumpTables = params.find_integer( "dumpTables", 0 );

    overheadMultP = params.find_integer( "overheadMult", 0 );

    m_log.output("overhead mult %f\n",overheadMultP);

    int ln, ch, iln;

    for (ln = 0; ln < ROUTER_NUM_OUTQS; ln++)
        for (iln = 0; iln < ROUTER_NUM_INQS; iln++) {
            for (ch = 0; ch < ROUTER_NUM_VCS; ch++) {
                outputQ[ln][iln].size_flits[ch] = 0;
                outputQ[ln][iln].vcQ[ch].clear();
                //-KDU
                //inputQ[iln].skipQs[ln][ch] = -1;
            }
        }


    for (iln = 0; iln < ROUTER_NUM_INQS; iln++) {
        //ready_inQs[iln] = NULL;

        inputQ[iln].head_busy = false;
        inputQ[iln].link = iln;
        inputQ[iln].vc_rr = 0;
        inputQ[iln].ready_vcQs = 0;

        for (ch = 0; ch < ROUTER_NUM_VCS; ch++) {
            //all input queues are empty
            inputQ[iln].size_flits[ch] = 0;
            inputQ[iln].vcQ[ch].clear();
            //inputQ[iln].vc_ready[ch] = false;
        }
    }


    for (int link = 0; link < ROUTER_NUM_OUTQS; link++) {

        //ready_oLCB[link] = NULL;
        //ready_iLCB[link] = NULL;

        //init packet counters
        txCount[link] = 0;
        rxCount[link] = 0;

        outLCB[link].dataQ.clear();
        outLCB[link].size_flits = 0;
        outLCB[link].external_busy = false;
        outLCB[link].internal_busy = false;
        outLCB[link].link = link;
        outLCB[link].ready_vc_count = 0;
        outLCB[link].vc_rr = 0;

        inLCB[link].dataQ.clear();
        inLCB[link].size_flits = 0;
        //inLCB[link].external_busy = false;
        inLCB[link].internal_busy = false;
        inLCB[link].link = link;

        for (ch = 0; ch < ROUTER_NUM_VCS; ch++) {
            //outLCB[link].front_rp_count[ch] = 0;
            outLCB[link].ilink_rr[ch] = 0;
            //outLCB[link].ready_vc[ch] = false;
            outLCB[link].stall4tokens[ch] = 0;
            outLCB[link].vcTokens[ch] = rtrInput_maxQSize_flits[link];
            outLCB[link].ready_outQ_count[ch] = 0;

            //for (iln = 0; iln < ROUTER_NUM_INQS; iln++)
            //outLCB[link].front_rp[ch][iln] = NULL;
        }
    }

    //ready_inQs_count = 0;
    //ready_oLCB_count = 0;
    //ready_iLCB_count = 0;

    ready_inQ = false;
    ready_oLCB = false;
    ready_iLCB = false;

    Params tmp_params; 

    m_datelineV.resize( 3 );
    for ( int i = 0; i < 3; i++ ) {
        m_datelineV[i] = false;
    }
    tmp_params = params.find_prefix_params( "network." );
    network = new Network( tmp_params ); 

    tmp_params = params.find_prefix_params( "routing." );
    setupRoutingTable ( tmp_params, network->size(), network->xDimSize(),
                        network->yDimSize(), network->zDimSize() );

    if (debug_interval > 0) {
        rtrEvent_t *event = eventPool.getEvent();
        event->cycle = cycle() + debug_interval;
        event->type = Debug;
        event->rp = NULL;
        rtrEventQ.push_back(event);
    }

    linkV.resize( ROUTER_NUM_LINKS + 1 );
    for (int dir = 0; dir < ROUTER_NUM_LINKS + 1; dir++) {
        //set up this nodes router link to the neighbor router

//         EventHandler_t*   handler = new EventHandler1Arg< 
//                             SS_router, bool, Event*, int >
//                        ( this, &SS_router::handleParcel, dir );

        m_dbg.output(CALL_INFO, "adding link %s\n", LinkNames[dir] );
	linkV[dir] = configureLink( LinkNames[dir],
				    new Event::Handler<SS_router,int>(this, &SS_router::handleParcel, dir));

        txlinkTo( linkV[dir], dir );
    }


    frequency = "1GHz";
    if ( params.find("clock") != params.end() ) {
        frequency = params["clock"];
    }

    m_log.output("frequency=%s\n", frequency.c_str() );

//     ClockHandler_t*     clockHandler;
//     clockHandler = new EventHandler< SS_router, bool, Cycle_t >
//                                                 ( this, &SS_router::clock );
//     TimeConverter* tc = registerClock( frequency, clockHandler );
    clock_handler = new Clock::Handler<SS_router>(this, &SS_router::clock);
    TimeConverter* tc = registerClock( frequency, clock_handler );
    if ( ! tc ) {
        _abort(XbarV2,"couldn't register clock handler");
    }
    currently_clocking = true;
}

void SS_router::newSetup() {
}


//: Output statistics
void SS_router::finish () {
#if 0 // newFinish() dumpTables
    m_dbg.output(CALL_INFO, "\n");
    if ( m_print_info )
        dumpStats(stdout);

    if ( m_print_info && dumpTables ) {
        string filename = configuration::getStrValue (":ed:datadir");
        char buf[256];
        sprintf(buf, "%s/routing/routing_table-%d", filename.c_str(), routerID);
        FILE *fp = fopen (buf, "w+");
        dumpTable(fp);
        fclose(fp);
    }
#endif
//    return 0;
}

void SS_router::dumpStats (FILE* fp) {
    m_dbg.output(CALL_INFO, "\n");

    fprintf(fp, "Router %d\n", routerID);


#if 0 // dumpStats
    int rxt, txt, myt;
    rxt = 0;
    txt = 0;
    myt = 0;

    int clockRate = configuration::getValue(":ed:MTA:clockRate");

    for (int i = 0; i < ROUTER_NUM_OUTQS; i++) {
        double sent = (double)txCount[i] / (double)cycle() * (double)clockRate;
        double recv = (double)rxCount[i] / (double)cycle() * (double)clockRate;
        fprintf(fp, "rtr %2d: %2d link: %9.3fM per sec sent %9.3fM per sec recv\n", routerID, i, sent, recv);
    }
    for (int i = 0; i < ROUTER_NUM_OUTQS; i++) {
        fprintf(fp, "rtr %2d: %2d link: %9d flits sent %9d flits recv\n", routerID, i, txCount[i], rxCount[i]);
    }

    fprintf(fp, "\t%9d total packets recv\n", rxt);
    fprintf(fp, "\t%9d total packets sent\n", txt);
    fprintf(fp, "\t%9d total packets for me\n", myt);
#endif

}

//: create a transmit network link
// and connect this and a neighboring router together over it
void SS_router::txlinkTo (Link* neighbor, int dir) {

    m_dbg.output(CALL_INFO, "dir=%d link=%p\n",dir,neighbor);

    if (tx_netlinks[dir] != NULL) {
        printf ("Error: router %d cannot tx link to %d dir, already linked\n", routerID, dir);
        return;
    }

    //create the new link and add it to the tx links
    link_t ln = new netlink;
    tx_netlinks[dir] = ln;
    ln->link = neighbor;
    ln->dir = dir;

    m_dbg.output(CALL_INFO, "Router %d tx linked to router XXX in %d direction\n", routerID, dir);
}

//: Used to see if all the links have been initialized for this router
bool SS_router::checkLinks() {

    m_dbg.output(CALL_INFO, "\n");
    for (int ln = 0; ln < ROUTER_NUM_LINKS; ln++)
        if ( (rx_netlinks[ln] == NULL) || (tx_netlinks[ln] == NULL) )
            return false;

    return true;
}


//: Dump the routing table for debugging
void SS_router::dumpTable (FILE *fp) {

    m_dbg.output(CALL_INFO, "\n");
    pair<int, int> key, localdest;
    int nodes = network->size();

    for (int nd = 0; nd < nodes ; nd++) {
        key.first = nd;

        for (int vc = 0; vc < ROUTER_NUM_VCS; vc++) {
            key.second = vc;
            localdest = routingTable[key];
            fprintf (fp, "node %3d: key %3d:%1d, dest %1d:%1d\n", routerID, key.first, key.second,
                     localdest.first, localdest.second);
        }
    }

}


//: Build the routing table
void SS_router::setupRoutingTable( Params params, int nodes,
                                   int xDim, int yDim, int zDim) {

    m_dbg.output(CALL_INFO, "\n");

    int tmp;
    tmp = params.find_integer("xDateline");
    if ( tmp == -1 ) {
        _abort(SS_router,"couldn't find xDateline\n" );
    }
    if ( tmp == calcXPosition(routerID,xDim,yDim,zDim) ) {
        m_datelineV[dimension(LINK_POS_X)] = true; 
    }

    tmp = params.find_integer("yDateline");
    if ( tmp == -1 ) {
        _abort(SS_router,"couldn't find yDateline\n" );
    }
    if ( tmp == calcYPosition(routerID,xDim,yDim,zDim) ) {
        m_datelineV[dimension(LINK_POS_Y)] = true; 
    }

    tmp = params.find_integer("zDateline");
    if ( tmp == -1 ) {
        _abort(SS_router,"couldn't find zDateline\n" );
    }
    if ( tmp == calcZPosition(routerID,xDim,yDim,zDim) ) {
        m_datelineV[dimension(LINK_POS_Z)] = true; 
    }

    my_xPos = calcXPosition( routerID, xDim, yDim, zDim );
    my_yPos = calcYPosition( routerID, xDim, yDim, zDim );
    my_zPos = calcZPosition( routerID, xDim, yDim, zDim );

    this->xDim = xDim;
    this->yDim = yDim;
    this->zDim = zDim;
    
//     m_routingTableV.resize(network->size());
//     for ( int i = 0; i < network->size(); i++ ) {
//         int dst_xPos = calcXPosition( i, xDim, yDim, zDim );
//         int dst_yPos = calcYPosition( i, xDim, yDim, zDim );
//         int dst_zPos = calcZPosition( i, xDim, yDim, zDim );

//         if ( my_xPos != dst_xPos ) {
//             int dir = calcDirection( my_xPos, dst_xPos, xDim );
//             if ( dir > 0 ) {
//                 m_routingTableV[ i ] = LINK_POS_X; 
//             } else {
//                 m_routingTableV[ i ] = LINK_NEG_X; 
//             } 
//         } else if ( my_yPos != dst_yPos ) {
//             int dir = calcDirection( my_yPos, dst_yPos, yDim );
//             if ( dir > 0 ) {
//                 m_routingTableV[ i ] = LINK_POS_Y; 
//             } else {
//                 m_routingTableV[ i ] = LINK_NEG_Y;
//             }
//         } else if ( my_zPos != dst_zPos ) {
//             int dir = calcDirection( my_zPos, dst_zPos, zDim );
//             if ( dir > 0 ) {
//                 m_routingTableV[ i ] = LINK_POS_Z; 
//             } else {
//                 m_routingTableV[ i ] = LINK_NEG_Z; 
//             } 
//         } else {
//             m_routingTableV[ i ] = ROUTER_HOST_PORT;
//         }
//     } 

    return;

}

//: receive a parcel, which should carry a packet
void SS_router::handleParcel( Event* e, int dir )
{
    RtrEvent*  event = static_cast<RtrEvent*>(e);

    m_cycle = getCurrentSimTime();
    m_dbg.output(CALL_INFO, "got event type %d on link %s\n", event->type, LinkNames[dir] );

    if ( event->type == RtrEvent::Credit ) {
        m_dbg.output(CALL_INFO, "%s returned tokens vc=%d num=%" PRIu32 "\n", LinkNames[dir],
                 event->credit.vc, event->credit.num );
        updateToken_flits( dir, event->credit.vc, event->credit.num );
        delete event;
    } else {

        int ilink, ivc, flits;
        networkPacket *np = &event->packet;
        flits = np->sizeInFlits;
        ivc = NIC_2_RTR_VC(np->vc);
        np->vc = ivc;

        ilink = dir;

        rxCount[ilink] += flits;
        InLCB( event, ilink, ivc, flits);
    }

    if ( !currently_clocking ) {
	registerClock(frequency,clock_handler);
	currently_clocking = true;
    }
    
    return;
}

//: route a packet
bool SS_router::route(rtrP* rp)
{
    m_dbg.output(CALL_INFO, "\n");
    networkPacket *packet =  &rp->event->packet;

    return findRoute( packet->destNum, 
            rp->ivc, rp->ilink, rp->ovc, rp->olink );
}

#if 0
    pair< int, int > key, localDest;

    key.first = packet->destNum();
    key.second = rp->ivc;

    if (routingTable.count(key) <= 0)
        return false;

    localDest = routingTable[key];

    rp->olink = localDest.first;
    rp->ovc = localDest.second;

    bool switch_dim = false;
//Deleted on 01-06-2006 -KDU
//reinstated on 01-09-2006 -KDU.  We need to cross back down when
//crossing dimensions
    switch (rp->ilink) {
    case LINK_NEG_X:
        if (rp->olink != LINK_POS_X) switch_dim = true;
        break;
    case LINK_POS_X:
        if (rp->olink != LINK_NEG_X) switch_dim = true;
        break;
    case LINK_NEG_Y:
        if (rp->olink != LINK_POS_Y) switch_dim = true;
        break;
    case LINK_POS_Y:
        if (rp->olink != LINK_NEG_Y) switch_dim = true;
        break;
    case LINK_NEG_Z:
        if (rp->olink != LINK_POS_Z) switch_dim = true;
        break;
    case LINK_POS_Z:
        if (rp->olink != LINK_NEG_Z) switch_dim = true;
        break;
    }

    if (switch_dim && rp->olink != ROUTER_HOST_PORT) {
        switch (rp->ovc) {
        case LINK_VC0:
        case LINK_VC1:
            rp->ovc = LINK_VC0;
            break;
        case LINK_VC2:
        case LINK_VC3:
            rp->ovc = LINK_VC2;
            break;
        }
    }

    m_dbg.output(CALL_INFO, "%lld: router %d parcel final dest %d (inc vc %d) local dest :%d:%d:\n",
              cycle(), routerID, key.first, key.second, localDest.first,
              localDest.second);

    return true;
}
#endif

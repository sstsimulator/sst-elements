/*
 * =====================================================================================
// Copyright 2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//
// Current router is a 4 stage input buffered only router. 
// TODO: remove packet level flow control add flit convertor to interface
 * =====================================================================================
 */

#ifndef  _NINTERFACE_H_INC
#define  _NINTERFACE_H_INC

#include	"router.h"

/* Local arbiter for ninterface */
class SimpleArbiter
{
    public:
        SimpleArbiter ();
        ~SimpleArbiter();
        void init();
        bool is_requested(uint ch);
        bool is_empty();
        void request( uint ch);
        uint pick_winner();

        uint no_channels;
        uint last_winner;
    protected:

    private:
        std::vector < bool > requests;

}; 

/*
 * =====================================================================================
 *        Class:  NInterface
 *  Description:  simple send recv queues for connecting a terminal to a router
 * =====================================================================================
 */

class NInterface : public DES_Component
{
    public:
        NInterface (SST::ComponentId_t id, Params_t& params);
        ~NInterface ();  

        /* Event handlers from external components. Prioritize such that all
         * these pkts are recieved before the clock runs so the new pkt state
         * can be updated
         * */
        void handle_link_arrival (DES_Event* e, int dir);

        /* Clocked events */
        bool tock(SST::Cycle_t c);

        /*  Generic Helper functions */
        void handle_issue_pkt_event( int inputid, uint64_t data); 
        void handle_new_packet_event( int port, NetworkPacket* data); 

        void parse_config( std::map<std::string, std::string>& p); /* overwrite init config */
        void resize( void ); // Reconfigure the parameters for the router

        int Finish()
        {
            fprintf(stderr,"\nNinterface Stats for node %d %s ",node_id, print_stats());
            return 0;
        }

    private:

        // Node ID
        int16_t node_id; /*  NOTE: node_id is not unique to a component. */

        // sub components
        std::vector<uint16_t > downstream_credits;
        std::vector<bool> terminal_credits;
        std::vector<uint16_t > terminal_outbuffer_flitindex;
        std::vector<uint16_t > terminal_inbuffer_flitindex;
        std::vector<bool> router_outbuffer_pktcomplete;

        DES_Link* router_link;
        DES_Link* terminal_link;

        SimpleArbiter arbiter;
        
        //link to macsim object
//        macsim_c* m_simBase;

        bool currently_clocking;
        SST::Clock::Handler<NInterface>* clock_handler;


}; /* -----  end of class NInterface  ----- */

#endif   /* ----- #ifndef _NINTERFACE_H_INC  ----- */

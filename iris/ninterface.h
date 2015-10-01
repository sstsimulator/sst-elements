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
// Current router is a 4 stage input buffered only router. 
// TODO: remove packet level flow control add flit convertor to interface
 * =====================================================================================
 */

#ifndef  _NINTERFACE_H_INC
#define  _NINTERFACE_H_INC

#include	"router.h"
#include        "irisTerminal.h"

namespace SST {
namespace Iris {

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
        NInterface (SST::ComponentId_t id, Params& params);
        ~NInterface ();  

        //link to macsim object
        IrisTerminal* terminal;

        /* Event handlers from external components. Prioritize such that all
         * these pkts are recieved before the clock runs so the new pkt state
         * can be updated
         * */
        void handle_router_link_arrival (DES_Event* e, int port_id);
        void handle_terminal_link_arrival (DES_Event* e, int port_id);

        /* Clocked events */
        bool tock( SST::Cycle_t cycle);

        void resize( void ); // Reconfigure the parameters for the router
        void print_stats ( std::string& stat_str ) const;
        void reset_stats ( void );

        void finish()
        {
            std::string tmp;
            print_stats(tmp);
            fprintf(stderr,"\nNode%d Interface \n%s ",node_id, tmp.c_str());
        }

    private:

        // Node ID
        int16_t node_id; /*  NOTE: node_id is not unique to a component. */

        // sub components
        std::vector<uint16_t > router_credits;
        std::vector<bool> router_pktcomplete;
        std::vector<uint64_t > router_inPkt_time;

        std::vector<int> terminal_credits;
        std::vector<bool> terminal_pktcomplete;
        std::vector<uint64_t > terminal_inPkt_time;

        GenericBuffer* router_inBuffer;
        GenericBuffer* terminal_inBuffer;

        DES_Link* router_link;
        DES_Link* terminal_link;

        int no_vcs;
        int buffer_size;
        int rcredits;
        int tcredits;
        int last_inpkt_winner;
        SimpleArbiter arbiter;

        /* Stats for interface */
        uint64_t total_pkts_out;
        uint64_t avg_pkt_lat;

}; /* -----  end of class NInterface  ----- */

}
}

#endif   /* ----- #ifndef _NINTERFACE_H_INC  ----- */

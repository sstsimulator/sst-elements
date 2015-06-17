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

#ifndef  _ROUTER_H_INC
#define  _ROUTER_H_INC

#include	"genericHeader.h"
#include	"router_params.h"
#include	"genericBuffer.h"
#include	"genericRC.h"
#include	"genericSwa.h"
#include	"genericVca.h"
#include	"stat.h"

namespace SST {
namespace Iris {

enum Router_PS {
    INVALID = 0,
    EMPTY,
    IB,
    RC,
    VCA_REQUESTED,
    VCA,
    SWA_REQUESTED,
    SWA,
    ST
};

/* its in genericHeader.cc */
extern const char* Router_PS_string;

/*-----------------------------------------------------------------------------
 *  Data structure that contains state for all active packets
 *  in the system ( active->packets in IB/RC/VCA/SA/ST ). 
 *  At any point there should be a max of ports*vcs no of entries in the
 *  router.
 *-----------------------------------------------------------------------------*/
struct IB_state
{
    IB_state ():in_port(-1),in_channel(-1),out_port(-1),out_channel(-1)
                , packet_length(-1), pipe_stage(EMPTY), mclass(INVALID_PKT)
                , is_valid(false)
                , pkt_arrival_time(0), no_arbitration_failures(0) 
    {}  

    uint16_t in_port;
    uint16_t in_channel;
    uint16_t out_port;
    uint16_t out_channel;
    uint16_t packet_length;
    Router_PS pipe_stage;
    message_class_t mclass;
    bool is_valid;
    bool sa_head_done;  // try to eliminate this
    std::vector<uint16_t> oport_list;
    std::vector<uint16_t> ovc_list;

    /* Stats */
    uint64_t pkt_arrival_time;
    uint16_t no_arbitration_failures;

    /* Helpful debugging entries */
    uint16_t src_node_id;
    uint16_t dst_node_id;
    uint64_t address;

    std::string 
        toString() const
        {
            std::stringstream str;
            str 
                << in_port << "|"
                << in_channel<< "|"
                << out_port << "|"
                << out_channel << "|"
                << packet_length << "|"
                << Router_PS_string[pipe_stage]<< "|"
                <<"\n";

            return str.str();
        }

    /*TODO: Write the copy constructor for this so that it can be used to reset stats */
}; 


/*
 * =====================================================================================
 *        Class:  Router
 *  Description:  Generic router with 5 pipeline stages 
 *  IB+RC| VCA | SA | ST | LT
 *  sub components for buffers, rc, vca and sa.. see corresponding class files
 *  lt is on the sst provided link/ for manifold its a manifold link
 *  links are bidirectional dlinks for data and slinks for signalling
 * =====================================================================================
 */

class Router : public DES_Component
{
    public:
        Router (SST::ComponentId_t id, Params& params);
        ~Router ();  

        /* Event handlers from external components. Prioritize such that all
         * these complete before the clocked event starts. ( Router state is
         * updated for all incoming packets and credits this way )
         * */
        void handle_link_arrival (DES_Event* e, int dir);

        /* Clocked events */
        bool tock(SST::Cycle_t c);

        /* Router stats */
        uint64_t stat_flits_in;
        uint64_t stat_flits_out;
        uint64_t stat_last_flit_cycle;
        RunningStat stat_total_pkt_latency;
        uint64_t heartbeat_interval;
        // take the buff occupancy at heartbeat_intervals
        double stat_avg_buffer_occ;
        uint64_t router_fastfwd;            /* fastfwd interval in cycles */
        uint64_t empty_cycles;
        std::vector<uint32_t> total_buff_occ;

        /*  Generic Helper functions */
        void print_stats(std::string&) const;
        void reset_stats();    /* Useful for fast forwarding. (uses router_fastfwd) */
        void parse_config( std::map<std::string, std::string>& p); /* overwrite init config */
        void resize( void ); // Reconfigure the parameters for the router

        void finish()
        {
            std::string stat_string;
            print_stats(stat_string);
            fprintf(stdout,"\nNode %d Rtr %s \n",node_id, stat_string.c_str());
//            fprintf(stderr,"\n\nempty_cycles-no of simulated cycles saved because the router had no work: %lu\n",empty_cycles);
        }

    private:
        void do_ib( const irisNPkt* , uint16_t ip); 
        void do_vca(void);
        void do_swa(void);
        void do_st(void);

        // Node ID
        int16_t node_id; /*  NOTE: node_id is not unique to a component. 
                          *  Use getId from the kernel for a unique component id */

        // sub components
        std::vector<GenericBuffer> in_buffer;
        std::vector<GenericRC> rc;
        GenericVca vca;
        GenericSwa swa;
        std::vector<IB_state> ib_state;
        std::vector<std::vector<uint16_t> > downstream_credits;

        std::vector<SST::Link*> links;
        std::vector<uint16_t> pending_stmsg;

        bool currently_clocking;
        SST::Clock::Handler<Router>* clock_handler;

        Output& output;

}; /* -----  end of class Router  ----- */

}
}

#endif   /* ----- #ifndef _ROUTER_H_INC  ----- */

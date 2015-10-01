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

#ifndef  _GENERICRC_H_INC
#define  _GENERICRC_H_INC

#include "genericHeader.h"
#include "router_params.h"
#include "SST_interface.h"

namespace SST {
namespace Iris {


extern Router_params* r_param; 
/*
 * =====================================================================================
 *        Class:  GenericRC
 *  Description:  RC unit for computing the route of an incoming packet. One per
 *  buffer ( can handle multiple virtual channels ). 
 *  Configure to support multipe routing schemes (deterministic/adaptive) for
 *  different topologies.
 * =====================================================================================
 */
class GenericRC
{
    public:
        GenericRC (); 
        ~GenericRC();

        void push( const irisNPkt*);
        uint16_t get_output_port ( uint16_t );
        uint16_t get_virtual_channel ( uint16_t );
        uint16_t no_adaptive_ports( uint16_t );
        uint16_t no_adaptive_vcs( uint16_t );
//        std::vector<uint16_t>* get_oport_list( uint16_t );
//        std::vector<uint16_t>* get_ovcs_list( uint16_t );
        bool is_empty();
        std::string toString() const;

        /*  If you want to pre-compute and store tables at init. 
        std::vector < uint16_t > grid_xloc;
        std::vector < uint16_t > grid_yloc;
         *  */
        // RC needs to know where u are located in a grid
        uint16_t node_id;


        class RC_entry
        {
            public:
                RC_entry(): current_oport_index(0), current_ovc_index(0), route_valid(false), out_port(-1), out_vc(-1) {}
                uint16_t current_oport_index;
                uint16_t current_ovc_index;
                bool route_valid;
                uint16_t out_port;
                uint16_t out_vc;
                std::vector < uint16_t > possible_out_ports;
                std::vector < uint16_t > possible_out_vcs;

        }; /* ----- end of class Address ----- */

        std::vector<RC_entry> entries;

    private:
        void route_x_y( const irisNPkt* hf );
        void route_twonode( const irisNPkt* hf );
        void route_torus(const irisNPkt* hf );
        void route_ring( const irisNPkt* hf );
        void route_ring_unidirection( const irisNPkt* hf );


}; /* ----- end of class GenericRC ----- */

}
}
#endif   /* ----- #ifndef _GENERICRC_H_INC  ----- */

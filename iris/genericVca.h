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

#ifndef  _GENERICVCA_H_INC
#define  _GENERICVCA_H_INC

#include	"genericHeader.h"
#include	"router_params.h"

namespace SST {
namespace Iris {

extern Router_params* r_param; 
struct VCA_unit
{
    VCA_unit(): in_port(-1), in_vc(-1), out_port(-1), out_vc(-1), is_valid(false){}
    uint16_t in_port;
    uint16_t in_vc;
    uint16_t out_port;
    uint16_t out_vc;
    bool is_valid;
}; 

class GenericVca
{
    public:
        GenericVca ();
        ~GenericVca ();

        void resize ( void );
        bool request( uint16_t op, uint16_t ovc, uint16_t ip, uint16_t ivc);
        void pick_winner( void );
        void clear_winner( uint16_t op, uint16_t oc, uint16_t ip, uint16_t ic); 
        uint16_t no_requestors ( uint16_t op );

        std::string toString() const;

        // should be visible in router class
        std::vector < std::vector <VCA_unit> > current_winners;

        // useful inlines
        inline bool is_empty ( uint16_t port ) const
        {
            for ( uint16_t i=0; i<r_param->ports*r_param->vcs; ++i )
                if (requesting_inputs.at(port).at(i).is_valid)
                    return false;

            return true;
        }	

        inline bool is_empty ( void ) const
        {
            for ( uint16_t p=0; p<r_param->ports; ++p )
                if (!is_empty(p))
                    return false;

            return true;
        }

        inline bool is_requested ( uint16_t op, uint16_t ovc, uint16_t ip, uint16_t ivc ) const
        {
            return requesting_inputs.at(op).at(ip+r_param->ports*ivc).is_valid ;
        }


    private:
        //Requestor matrix 
        std::vector < std::vector <VCA_unit> > requesting_inputs;
        // pool of grants to issue from
        std::vector < std::vector <uint16_t> > ovc_tokens;
        // Winner list and index of last winner to round robin
        // out_ports x out_vc matrix
        std::vector < std::vector <uint16_t> > last_winner;

}; /* -----  end of class GenericVca  ----- */

}
}
#endif   /* ----- #ifndef _GENERICVCA_H_INC  ----- */

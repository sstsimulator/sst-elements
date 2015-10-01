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

#ifndef  _GENERICSWA_H_INC
#define  _GENERICSWA_H_INC

#include	"genericHeader.h"
#include	"router_params.h"

namespace SST {
namespace Iris {

extern Router_params* r_param; 
struct SA_unit
{
        SA_unit():port(-1),channel(-1),win_cycle(0), is_valid(false){};
        uint16_t port;
        uint16_t channel;
        uint64_t win_cycle;
        bool is_valid;
};

class GenericSwa
{
    public:
        GenericSwa();
        ~GenericSwa ();

        void resize( void );
        bool is_requested(uint16_t outp, uint16_t inp, uint16_t ovc);
        void clear_requestor(uint16_t outp, uint16_t inp, uint16_t ovc);
        void request(uint16_t op, uint16_t ovc, uint16_t inp, uint16_t ivc, uint64_t now);
        SA_unit pick_winner( uint16_t op, uint64_t now);
        SA_unit do_round_robin_arbitration( uint16_t op, uint64_t now);
        bool is_empty();
        std::string toString() const;

    protected:

    private:
        // This is also the requestor matrix op x (ip*ovc) wide that holds
        // information of the requestor ( input pot & input vc can be used to
        // record in time etc
        std::vector < std::vector<SA_unit> > requesting_inputs;

        // Current winner for each out port.
        std::vector < SA_unit > last_winner;
        // Index where you stopped on the requestor matrix so you can round
        // robin in the next cycle. 
        std::vector < uint16_t> last_port_winner;

};

}
}

#endif   /* ----- #ifndef _GENERICSWA_H_INC  ----- */

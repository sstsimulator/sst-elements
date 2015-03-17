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
//       Filename:  genericSwa.cc
 * =====================================================================================
 */
#ifndef  _GENERICSWA_CC_INC
#define  _GENERICSWA_CC_INC

#include <sst_config.h>
#include	"genericSwa.h"

using namespace SST::Iris;

GenericSwa::GenericSwa()
{
    resize();
}

GenericSwa::~GenericSwa()
{
    last_winner.clear();
    last_port_winner.clear();
}

void
GenericSwa::resize( )
{
    /* Clear out the previous setting */
    for ( uint16_t i=0; i<requesting_inputs.size(); i++)
        requesting_inputs.at(i).clear();

    requesting_inputs.clear();
    last_winner.clear();
    last_port_winner.clear();
    /* ************* END of clean up *************/

    // Use of these vectors can be confusing see header for explanation.
    // 2D array
    requesting_inputs.resize(r_param->ports);

    //1D array
    last_winner.resize(r_param->ports);
    last_port_winner.insert(last_port_winner.begin(),r_param->ports,0);

    for ( uint i=0; i<r_param->ports; i++)
        requesting_inputs.at(i).resize(r_param->ports*r_param->vcs);

    return;
}

bool
GenericSwa::is_requested ( uint16_t op, uint16_t ip, uint16_t ic )
{
    assert( op < r_param->ports && " Requested outport > configured r_param->ports ");
    assert( ip < r_param->ports && " Requested inport > configured r_param->ports ");
    assert( ic < r_param->vcs && " Requested Vc greater than configured r_param->vcs");
    return requesting_inputs.at(op).at(ip*r_param->vcs+ic).is_valid;
}		/* -----  end of function is_requested  ----- */

void
GenericSwa::clear_requestor ( uint16_t op, uint16_t ip, uint16_t ic )
{
    requesting_inputs.at(op).at(ip*r_param->vcs+ic).is_valid=false;
    return ;
}		/* -----  end of method GenericSwa::clear_requestor  ----- */

void
GenericSwa::request ( uint16_t op, uint16_t ovc, uint16_t ip, uint16_t ivc, uint64_t now )
{
    SA_unit& tmp = requesting_inputs.at(op).at(ip*r_param->vcs+ovc);
    tmp.is_valid = true;
    tmp.port = ip;
    tmp.channel = ivc;
    return ;
}		/* -----  end of method GenericSwa::request  ----- */

/* Time is taken from the router since the class does not inherit from kernel
 * component.
 * This can be avoided as well. Just make sure pick winner for the same output
 * port never gets called twice in a router.
 */
SA_unit
GenericSwa::pick_winner ( uint16_t op, uint64_t now )
{
    /* TODO: Implement more efficient algorithms for bipartite graph assignment 
     */
    return do_round_robin_arbitration(op, now);
}		/* -----  end of method GenericSwa::pick_winner  ----- */

SA_unit
GenericSwa::do_round_robin_arbitration ( uint16_t op, uint64_t now )
{
    // for a given op you can have only one winner in a cycle
    // Safety check so no two winners are picked in the same cycle
    if ( last_winner.at(op).win_cycle >= now )
        return last_winner.at(op);

    bool found_winner = false;
    for ( uint16_t i=last_port_winner.at(op); i<r_param->ports*r_param->vcs; i++) 
        if ( requesting_inputs.at(op).at(i).is_valid)
        {
            last_port_winner.at(op)=i;
            found_winner = true;
        }

    if ( !found_winner )
    {
        for ( uint16_t i=0; i < last_port_winner.at(op);i++) 
            if ( requesting_inputs.at(op).at(i).is_valid )
            {
                last_port_winner.at(op)=i;
                found_winner = true;
            }
    }

    // Update internal state
    if ( found_winner )
    {
        last_winner.at(op).port = requesting_inputs.at(op).at(last_port_winner.at(op)).port;
        last_winner.at(op).channel= requesting_inputs.at(op).at(last_port_winner.at(op)).channel;
        last_winner.at(op).win_cycle= now;
    }

    assert ( found_winner == true && " SWA dint issue a grant with mulptiple requestors");

    return last_winner.at(op);
}		/* -----  end of method GenericSwa::do_round_robin_arbitration  ----- */

bool
GenericSwa::is_empty ( void )
{
    for( uint16_t p=0; p<requesting_inputs.size(); p++)
    {
        std::vector<SA_unit> tmp = requesting_inputs.at(p);
        std::vector<SA_unit>::const_iterator it = tmp.begin();
        for ( it=tmp.begin(); it!=tmp.end(); it++){
            if ( it->is_valid )
                return false;
        }
    }
    return true ;
}		/* -----  end of method GenericSwa::is_empty  ----- */

std::string
GenericSwa::toString ( void ) const
{
    std::stringstream str;
    str << "GenericSwa: matrix size "
        << "\t: " << requesting_inputs.size();
    if( requesting_inputs.size())
        str << " x " << requesting_inputs.at(0).size();
    return str.str();
}		/* -----  end of method GenericSwa::toString  ----- */

#endif   /* ----- #ifndef _GENERICSWA_CC_INC  ----- */

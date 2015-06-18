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
//       Filename:  genericRC.cc
 * =====================================================================================
 */

#ifndef  _GENERICRC_CC_INC
#define  _GENERICRC_CC_INC

#include <sst_config.h>
#include "genericRC.h"

using namespace SST::Iris;

GenericRC::GenericRC ()
{
    entries.resize(r_param->vcs);
}

GenericRC::~GenericRC()
{
    /*  Router resize is failing. Hence commenting it */
//    for ( uint16_t i=0; i<entries.size() ; i++ ) {
//        delete &entries.at(i);
//    }
}

void
GenericRC::route_x_y(const irisNPkt* hf)
{
    ports_t oport = INV;
    uint16_t myx = (int)(node_id%r_param->grid_size);
    uint16_t destx = (int)(hf->destNum%r_param->grid_size);
    uint16_t myy = (int)(node_id/r_param->grid_size);
    uint16_t desty = (int)(hf->destNum/r_param->grid_size);
    if ( myx == destx && myy == desty )
        oport = nic;
    else if ( myx == destx )
    {
        if( desty < myy )
            oport = yNeg;
        else
            oport = yPos;
    }
    else
    {
        if( destx < myx )
            oport = xNeg;
        else
            oport = xPos;
    }

    assert ( oport != INV && " Routing for flit failed \n");
    entries.at(hf->vc).possible_out_ports.push_back(oport);
    entries.at(hf->vc).possible_out_vcs.push_back(0);

//    printf(" route pkt %d \n", oport);
    return ;
}

void
GenericRC::route_torus(const irisNPkt* hf)
{
    uint16_t myx = (int)(node_id%r_param->grid_size);
    uint16_t destx = (int)(hf->destNum%r_param->grid_size);
    uint16_t myy = (int)(node_id/r_param->grid_size);
    uint16_t desty = (int)(hf->destNum/r_param->grid_size);
    int oport = -1;
    int ovc= -1;

    if ( myx == destx && myy == desty )
    {
        oport = EJECT_PORT;
        if ( hf->mclass == MC_RESP)
            ovc = 1;
        else
            ovc = 0;
    }
    else if ( myx == destx ) /* reached row but not col */
    {
        /* Decide the port based on hops around the ring */
        if ( desty > myy )
        {
            if ((desty-myy)>r_param->grid_size/2)
                oport = 3;
            else
                oport = 4;
        }
        else
        {
            if ((myy - desty )>r_param->grid_size/2)
                oport = 4;
            else
                oport = 3;
        }

        /* Decide the vc */
        if( oport == 3)
        {
            desty = (r_param->grid_size-desty)%r_param->grid_size;
            myy= (r_param->grid_size-myy)%r_param->grid_size;
        }

        if ( desty > myy )
        {
            if ( hf->mclass == MC_RESP)
                ovc = 3;
            else
                ovc = 4;
        }
        else
        {
            if ( hf->mclass == MC_RESP)
                ovc = 1;
            else
                ovc = 0;
        }

    }
    /* both row and col dont match do x first. Y port is
     * adaptive in this case and can only be used with the adaptive vc */
    else
    {
        if ( destx > myx )
        {
            if ((destx - myx)>r_param->grid_size/2)
                oport = 1;
            else
                oport = 2;
        }
        else
        {
            if ((myx - destx )>r_param->grid_size/2)
                oport = 2;
            else
                oport = 1;
        }

        /* Decide the vc */
        if( oport == 1)
        {
            destx = (r_param->grid_size-destx)%r_param->grid_size;
            myx= (r_param->grid_size-myx)%r_param->grid_size;
        }

        if ( destx > myx )
        {
            if ( hf->mclass == MC_RESP)
                ovc = 3;
            else
                ovc = 2;
        }
        else
        {
            if ( hf->mclass == MC_RESP)
                ovc = 1;
            else
                ovc = 0;
        }

    }

    assert ( oport != -1 && ovc != -1 && " Routing for flit failed \n");
    entries.at(hf->vc).possible_out_ports.push_back(oport);
    entries.at(hf->vc).possible_out_vcs.push_back(ovc);

    return;
}

// this routes in a unidirectional ring. When comparing against the bi
// directional case remember to adjust for channel widths/no of flits in a
// packet for a fair evaluation
void
GenericRC::route_ring_unidirection(const irisNPkt* hf)
{

    r_param->grid_size = r_param->no_nodes;
    uint16_t myx = node_id;
    uint16_t destx = hf->destNum;
    int oport = -1;
    int ovc = -1;

    if ( myx == destx )
    {
        oport = 0;
        if ( hf->mclass== PROC_REQ)
            ovc = 0;
        else
        {
            ovc = 1;
            // should be able to add 0-4 vcs here but make sure vca can
            // handle multiple selections
        }

    }
    else
    {
        oport = 2; 

        /* Decide the vc */
        if ( destx > myx )
        {
            if ( hf->mclass == PROC_REQ )
                ovc = 2;
            else
                ovc = 3;
        }
        else
        {
            if ( hf->mclass == PROC_REQ)
                ovc = 0;
            else
                ovc = 1;
        }

    }

    assert ( oport != -1 && ovc != -1 && " Routing for flit failed \n");
    entries.at(hf->vc).possible_out_ports.push_back(oport);
    entries.at(hf->vc).possible_out_vcs.push_back(ovc);
    return;
}

void
GenericRC::route_ring(const irisNPkt* hf)
{

    r_param->grid_size = r_param->no_nodes;
    uint16_t myx = node_id;
    uint16_t destx = hf->destNum;
    int oport = -1, ovc = -1;

    if ( myx == destx )
    {
        oport = 0;
        if ( hf->mclass== PROC_REQ)
            ovc = 0;
        else
        {
            ovc = 1;
            // should be able to add 0-4 vcs here but make sure vca can
            // handle multiple selections
        }

    }
    else
    {
        if ( destx > myx)
        {
            if ( (destx - myx) > r_param->grid_size/2)
                oport = 1;
            else
                oport = 2;
        }
        else
        {
            if ( (myx - destx) > r_param->grid_size/2)
                oport = 2;
            else
                oport = 1;
        }

        /* Decide the vc */
        if( oport == 1)
        {
            destx = (r_param->grid_size-destx)%r_param->grid_size;
            myx= (r_param->grid_size-myx)%r_param->grid_size;
        }

        if ( destx > myx )
        {
            if ( hf->mclass == PROC_REQ )
                ovc = 2;
            else
                ovc = 3;
        }
        else
        {
            if ( hf->mclass == PROC_REQ)
                ovc = 0;
            else
                ovc = 1;
        }

    }

    assert ( oport != -1 && ovc != -1 && " Routing for flit failed \n");
    entries.at(hf->vc).possible_out_ports.push_back(oport);
    entries.at(hf->vc).possible_out_vcs.push_back(ovc);
    return;
}


void
GenericRC::route_twonode(const irisNPkt* hf)
{
    int oport = -1, ovc = -1;

    if ( node_id == 0 )
    {
        if ( hf->destNum== 1)
            oport = 1;
        else
            oport = 0;
    }

    if ( node_id == 1 )
    {
        if ( hf->destNum== 0)
            oport = 1;
        else
            oport = 0;
    }

    if ( hf->mclass == PROC_REQ )
        ovc = 0;
    else
        ovc = 1;

    assert ( oport != -1 && ovc != -1 && " Routing for flit failed \n");
    entries.at(hf->vc).possible_out_ports.push_back(oport);
    entries.at(hf->vc).possible_out_vcs.push_back(ovc);
    return;
}

void
GenericRC::push (const irisNPkt* f)
{
    uint16_t push_channel =f->vc;
    assert ( push_channel < entries.size() && " Invalid push channel RC");


    //Route the head
    //    if( f->type == HEAD )
    {
        entries.at(push_channel).possible_out_ports.clear();
        entries.at(push_channel).possible_out_vcs.clear();

        switch ( r_param->rc_scheme ) {
            case RING_ROUTING:	
                route_ring( f );
                break;

            case TWONODE_ROUTING:	
                route_twonode( f );
                break;

            case XY:	
                route_x_y(f);
                break;

            case TORUS_ROUTING:	
                route_torus( f );
                break;

            default:	
                break;
        }				/* -----  end switch  ----- */

        assert ( entries.at(push_channel).possible_out_ports.size() > 0 
                 && " RC did not return and output port " );
        assert ( entries.at(push_channel).possible_out_vcs.size() > 0 
                 && " RC did not return and output channel " );
        entries.at(push_channel).out_port = entries.at(push_channel).possible_out_ports.at(0);
        entries.at(push_channel).out_vc = entries.at(push_channel).possible_out_vcs.at(0);
        entries.at(push_channel).route_valid = true;

    }
    /* need to clear the rc unit when sending the tail flit out 
       else if(f->type == TAIL)
       {
       assert (entries.at(push_channel).route_valid
       && " Got TAIL flit but rc unit has no valid route");

       entries.at(push_channel).route_valid = false;
       entries.at(push_channel).possible_out_ports.clear();
       entries.at(push_channel).possible_out_vcs.clear();
       }
       else if (f->type == BODY)
       {
       assert (entries.at(push_channel).route_valid
       && " Got BODY flit but rc unit has no valid route");
       }
       else
       {
       Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1, InvalidFlitException fty: %d ", f->type);
       }
     * */

    return ;
} /* ----- end of method genericRC::push ----- */

uint16_t
GenericRC::get_output_port ( uint16_t channel)
{
    if ( entries.at(channel).current_oport_index == entries.at(channel).possible_out_ports.size())
        entries.at(channel).current_oport_index = 0;

    uint16_t oport = entries.at(channel).possible_out_ports.at(entries.at(channel).current_oport_index);
    entries.at(channel).current_oport_index++;

    return oport;
} /* ----- end of method genericRC::get_output_port ----- */

uint16_t
GenericRC::get_virtual_channel ( uint16_t channel )
{
    if ( entries.at(channel).current_ovc_index == entries.at(channel).possible_out_vcs.size())
        entries.at(channel).current_ovc_index = 0;

    uint16_t ovc = entries.at(channel).possible_out_vcs.at(entries.at(channel).current_ovc_index);
    entries.at(channel).current_ovc_index++;

    return ovc;
} /* ----- end of method genericRC::get_vc ----- */

uint16_t
GenericRC::no_adaptive_vcs( uint16_t channel )
{
    return entries.at(channel).possible_out_vcs.size();
}

uint16_t
GenericRC::no_adaptive_ports( uint16_t channel)
{
    return entries.at(channel).possible_out_ports.size();
}

/* 
   std::vector<uint16_t>*
   GenericRC::get_oport_list( uint16_t channel)
   {
   return &entries.at(channel).possible_out_ports;
   }

   std::vector<uint16_t>*
   GenericRC::get_ovcs_list ( uint16_t channel)
   {
   return &entries.at(channel).possible_out_vcs;
   }
 * */

/*  The RC unit is busy between cycles IB for HEAD and IB for TAIL */
bool
GenericRC::is_empty ()
{
    for ( uint16_t i=0 ; i<r_param->vcs; i++ )
        if(entries.at(i).route_valid)
            return false;

    return true;
} /* ----- end of method genericRC::is_empty ----- */

std::string
GenericRC::toString () const
{
    std::stringstream str;
    str << "GenericRC"
        << "\tchannels: " << entries.size();
    return str.str();
} /* ----- end of function GenericRC::toString ----- */

#endif   /* ----- #ifndef _GENERICRC_CC_INC  ----- */

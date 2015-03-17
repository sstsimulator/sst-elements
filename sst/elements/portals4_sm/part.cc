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

#include <sst_config.h>
#include <sst/elements/portals4_sm/part.h>

using namespace std;
using namespace SST;
using namespace SST::Portals4_sm;

Portals4Partition::Portals4Partition(int total_ranks) :
    SSTPartitioner(),
    ranks(total_ranks)
{}

void Portals4Partition::performPartition(ConfigGraph* graph) {
    int sx=0, sy=0, sz=0;

        
    // Need to look through components until we find a router so we
    // can get the dimensions of the torus
    ConfigComponentMap_t& comps = graph->getComponentMap();
    for ( ConfigComponentMap_t::iterator iter = comps.begin();
          iter != comps.end(); ++iter )
    {
        ConfigComponent* ccomp = &(*iter);
        if ( ccomp->type == "SS_router.SS_router" ) {
            sx = ccomp->params.find_integer("network.xDimSize");
            sy = ccomp->params.find_integer("network.yDimSize");
            sz = ccomp->params.find_integer("network.zDimSize");
            if ( sx == -1 || sy == -1 || sz == -1 ) {
                printf("SS_router found without properly set network dimensions, aborting...\n");
                abort();
            }
	    break;
        }
    }
    
    for ( ConfigComponentMap_t::iterator iter = comps.begin();
          iter != comps.end(); ++iter )
    {
        ConfigComponent* ccomp = &(*iter);
        int id = ccomp->params.find_integer("id");
        if ( id == -1 ) {
            printf("Couldn't find id for component %s, aborting...\n",ccomp->name.c_str());
            abort();
        }
        // Get the x, y and z dimensions for the given id.
        int x, y, z;

        z = id / (sx * sy);
        y = (id / sx) % sy;
        x = id % sx;
        
        /* Partition logic */
        int rank = id % ranks;
        if ( ranks == 2 ) {
            if ( x < sx/2 ) rank = 0;
            else rank = 1;
        }
        if ( ranks == 4 ) {
            rank = 0;
            if ( x >= sx/2 ) rank = rank | 1;
            if ( y >= sy/2 ) rank = rank | (1 << 1);
        }
        if ( ranks == 8 ) {
            rank = 0;
            if ( x >= sx/2 ) rank = rank | 1;
            if ( y >= sy/2 ) rank = rank | (1 << 1);
            if ( z >= sz/2 ) rank = rank | (1 << 2);
        }

        if ( ranks == 16 ) {
            rank = 0;
            if ( x >= 3*(sx/4) ) rank = rank | 3;
            else if ( x >= sx/2 && x < 3*(sx/4) ) rank = rank | 2;
            else if ( x >= sx/4 && x < sx/2 ) rank = rank | 1;
            if ( y >= sy/2 ) rank = rank | (1 << 2);
            if ( z >= sz/2 ) rank = rank | (1 << 3);
        }
        
        ccomp->rank = rank;
        
    }    
    
}

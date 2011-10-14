// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include "sst/core/serialization/element.h"

#include <sst/core/element.h>

#include <sst/core/configGraph.h>

#include "trig_cpu/trig_cpu.h"
#include "trig_nic/trig_nic.h"

static Component* 
create_trig_cpu(SST::ComponentId_t id, 
                SST::Component::Params_t& params)
{
    return new trig_cpu( id, params );
}

static Component* 
create_trig_nic(SST::ComponentId_t id, 
                SST::Component::Params_t& params)
{
    return new trig_nic( id, params );
}

#if 0
// Just a quick test
static void partition(ConfigGraph* graph, int ranks) {
    Params rtr_params;
    rtr_params["clock"] = "500Mhz";
    rtr_params["debug"] = "no";
    rtr_params["info"] = "no";
    rtr_params["iLCBLat"] = "13";
    rtr_params["oLCBLat"] = "7";
    rtr_params["routingLat"] = "3";
    rtr_params["iQLat"] = "2";

    rtr_params["OutputQSize"] = "flits";
    rtr_params["InputQSize"] = "flits";
    rtr_params["Router2NodeQSize"] = "flits";

    rtr_params["network.xDimSize"] = "16";
    rtr_params["network.yDimSize"] = "8";
    rtr_params["network.zDimSize"] = "8";

    rtr_params["routing.xDateline"] = "0";
    rtr_params["routing.yDateline"] = "0";
    rtr_params["routing.zDateline"] = "0";

    Params cpu_params;
    cpu_params["radix"] = "8";
    cpu_params["timing_set"] = "2";
    cpu_params["nodes"] = "1024";
    cpu_params["msgrate"] = "5MHz";
    cpu_params["xDimSize"] = "16";
    cpu_params["yDimSize"] = "8";
    cpu_params["zDimSize"] = "8";
    cpu_params["noiseRuns"] = "0";
    cpu_params["noiseInterval"] = "1kHz";
    cpu_params["noiseDuration"] = "25us";
    cpu_params["application"] = "allreduce.tree_triggered";
    cpu_params["latency"] = "500";
    cpu_params["msg_size"] = "1048576";
    cpu_params["chunk_size"] = "16384";
    cpu_params["coalesce"] = "0";
    cpu_params["enable_putv"] = "0";

    Params nic_params;
    nic_params["clock"] = "500MHz";
    nic_params["timing_set"] = "2";
    nic_params["info"] = "no";
    nic_params["debug"] = "no";
    nic_params["dummyDebug"] = "no";
    nic_params["latency"] = "500";

    string nic_link_lat = "200ns";
    string rtr_link_lat = "10ns";
    
    int x_count = 16;
    int y_count = 8;
    int z_count = 8;
    int size = x_count * y_count * z_count;

    ComponentId_t cid;
    for ( int i = 0; i < size; ++i) {
        int x, y, z;

	z = i / (x_count * y_count);
	y = (i / x_count) % y_count;
	x = i % x_count;

	
	cid = graph->addComponent("cpu."+i, "portals4_sm.trig_cpu");
	graph->addParams(cid, cpu_params);
	graph->addParameter(cid, "id", ""+i);
	graph->addLink(cid, "cpu2nic."+i, "nic", nic_link_lat);

	cid = graph->addComponent("nic."+i, "portals4_sm.trig_nic");
	graph->addParams(cid, nic_params);
	graph->addParameter(cid, "id", ""+i);
	graph->addLink(cid, "cpu2nic."+i, "cpu", nic_link_lat);
	graph->addLink(cid, "nic2rtr."+i, "rtr", nic_link_lat);

	cid = graph->addComponent("rtr."+i, "SS_router.SS_router");
	graph->addParams(cid, rtr_params);
	graph->addParameter(cid, "id", "" + i);

	// if ( x_count > 1 ) {
	//     graph->addLink(cid, string("xr2r.") + y + "." + z + "." + ((x+1)%x_count), "xPos", rtr_link_lat);
	//     graph->addLink(cid, "xr2r."+y+"."+z+"."+x, "xNeg", rtr_link_lat);
	// }
	
	// if ( y_count > 1 ) {
	//     graph->addLink(cid, "yr2r."+x+"."+z+"."+((y+1)%y_count), "yPos", rtr_link_lat);
	//     graph->addLink(cid, "yr2r."+x+"."+z+"."+y, "yNeg", rtr_link_lat);
	// }
	
	// if ( z_count > 1 ) {
	//     graph->addLink(cid, "xr2r."+x+"."+y+"."+((z+1)%z_count), "zPos", rtr_link_lat);
	//     graph->addLink(cid, "xr2r."+x+"."+y+"."+z, "zNeg", rtr_link_lat);
	// }
	
    }
    
}

#endif
static void partition(ConfigGraph* graph, int ranks) {
    int sx, sy, sz;

        
    // Need to look through components until we find a router so we
    // can get the dimensions of the torus
    ConfigComponentMap_t& comps = graph->getComponentMap();
    for ( ConfigComponentMap_t::iterator iter = comps.begin();
                            iter != comps.end(); ++iter )
    {
	ConfigComponent* ccomp = (*iter).second;
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
	ConfigComponent* ccomp = (*iter).second;
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

static const ElementInfoComponent components[] = {
    { "trig_cpu",
      "Triggered CPU for Portals 4 research",
      NULL,
      create_trig_cpu,
    },
    { "trig_nic",
      "Triggered NIC for Portals 4 research",
      NULL,
      create_trig_nic,
    },
    { NULL, NULL, NULL, NULL }
};

static const ElementInfoPartitioner partitioners[] = {
    { "partitioner",
      "Hello",
      NULL,
      partition,
    },
    { NULL, NULL, NULL, NULL }
};
      
					     

extern "C" {
    ElementLibraryInfo portals4_sm_eli = {
        "portals4_sm",
        "State-machine based processor/nic for Portals 4 research",
        components,
	NULL,
	NULL,
	partitioners,
    };
}

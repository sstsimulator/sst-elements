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
#include "sst/core/serialization.h"

#include <sst/core/element.h>

#include <sst/core/configGraph.h>

#include "trig_cpu/trig_cpu.h"
#include "trig_nic/trig_nic.h"

#include <stdio.h>
#include <stdarg.h>

#include "portals_args.h"
#include "part.h"
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>

using namespace std;
using namespace SST;
using namespace SST::Portals4_sm;

static SST::Component*
create_trig_cpu(SST::ComponentId_t id,
                SST::Params& params)
{
    return new SST::Portals4_sm::trig_cpu( id, params );
}

static SST::Component*
create_trig_nic(SST::ComponentId_t id,
                SST::Params& params)
{
    return new SST::Portals4_sm::trig_nic( id, params );
}


static string str(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start (args, format);
    vsprintf (buffer,format, args);
    va_end (args);
    string ret(buffer);
    return ret;
}


using namespace boost;

static void generate(ConfigGraph* graph, string options, int ranks) {
    // Need to break up the string options into argc, argv type format
    char_separator<char> sep(" ");
    tokenizer< char_separator<char> > tok(options, sep);
    int count = 1;
    BOOST_FOREACH(string t, tok) {
	count++;
    }
    // for (tokenizer<>::iterator it = tok.begin(); it != tok.end(); ++it){
    // 	count++;
    // }

    char** argv = new char*[count];
    int loop = 1;
    argv[0] = (char*)"portals4_sm.generate";
    
    BOOST_FOREACH(string t, tok) {
	char* new_str = (char*)malloc((t.length() + 1) * sizeof(char));
	argv[loop++] = strncpy(new_str,t.c_str(),t.length() + 1);
    }
    // for (tokenizer<>::iterator it = tok.begin(); it != tok.end(); ++it){
    // 	argv[i++] = (*it)->c_str();
    // }

    portals_args args;

    parse_partals_args(count,argv,&args);

    if (0 != args.noise_runs && (NULL == args.noise_interval || NULL == args.noise_duration)) {
        print_usage(argv[0]);
        exit(1);
    }

    if (NULL == args.application) {
        print_usage(argv[0]);
        exit(1);
    }

    /* clean up so SDL file looks nice */
    if (NULL == args.noise_interval) args.noise_interval = (char*)"1kHz";
    if (NULL == args.noise_duration) args.noise_duration = (char*)"25us";

    string rtr_link_lat = "10ns";
    string nic_link_lat;
    if ( args.timing_set == 1 ) nic_link_lat = "100ns";
    if ( args.timing_set == 2 ) nic_link_lat = "200ns";
    if ( args.timing_set == 3 ) nic_link_lat = "250ns";
    
    int x_count = args.x;
    int y_count = args.y;
    int z_count = args.z;
    int radix = args.radix;
    int size = x_count * y_count * z_count;

    Params rtr_params;
    rtr_params["clock"] = "500Mhz";
    rtr_params["debug"] = "no";
    rtr_params["info"] = "no";
    rtr_params["iLCBLat"] = "13";
    rtr_params["oLCBLat"] = "7";
    rtr_params["routingLat"] = "3";
    rtr_params["iQLat"] = "2";

    rtr_params["OutputQSize_flits"] = "16";
    rtr_params["InputQSize_flits"] = "96";
    rtr_params["Router2NodeQSize_flits"] = "512";

    rtr_params["network.xDimSize"] = str("%d",x_count);
    rtr_params["network.yDimSize"] = str("%d",y_count);
    rtr_params["network.zDimSize"] = str("%d",z_count);

    rtr_params["routing.xDateline"] = "0";
    rtr_params["routing.yDateline"] = "0";
    rtr_params["routing.zDateline"] = "0";

    // cout << "rtr_params:" << endl;
    // rtr_params.print_all_params(cout);
    
    Params cpu_params;
    cpu_params["radix"] = str("%d",radix);
    cpu_params["timing_set"] = str("%d",args.timing_set);
    cpu_params["nodes"] = str("%d",size);
    cpu_params["msgrate"] = str("%s",args.msg_rate);
    cpu_params["xDimSize"] = str("%d",x_count);
    cpu_params["yDimSize"] = str("%d",y_count);
    cpu_params["zDimSize"] = str("%d",z_count);
    cpu_params["noiseRuns"] = str("%d",args.noise_runs);
    cpu_params["noiseInterval"] = str("%s",args.noise_interval);
    cpu_params["noiseDuration"] = str("%s",args.noise_duration);
    cpu_params["application"] = str("%s",args.application);
    cpu_params["latency"] = str("%d",args.latency);
    cpu_params["msg_size"] = str("%d",args.msg_size);
    cpu_params["chunk_size"] = str("%d",args.chunk_size);
    cpu_params["coalesce"] = str("%d",args.coalesce);
    cpu_params["enable_putv"] = str("%d",args.enable_putv);

    // cout << "cpu_params:" << endl;
    // cpu_params.print_all_params(cout);

    Params nic_params;
    nic_params["clock"] = "500MHz";
    nic_params["timing_set"] = str("%d",args.timing_set);
    nic_params["info"] = "no";
    nic_params["debug"] = "no";
    nic_params["dummyDebug"] = "no";
    nic_params["latency"] = str("%d",args.latency);

    // cout << "nic_params:" << endl;
    // nic_params.print_all_params(cout);
    
    ComponentId_t cid;
    for ( int i = 0; i < size; ++i) {
	int x, y, z;

	z = i / (x_count * y_count);
	y = (i / x_count) % y_count;
	x = i % x_count;

	
	cid = graph->addComponent(str("%d.cpu",i), "portals4_sm.trig_cpu");
	graph->addParams(cid, cpu_params);
	graph->addParameter(cid, "id", str("%d",i));
	graph->addLink(cid, str("%d.cpu2nic",i), "nic", nic_link_lat);

	cid = graph->addComponent(str("%d.nic",i), "portals4_sm.trig_nic");
	graph->addParams(cid, nic_params);
	graph->addParameter(cid, "id", str("%d",i));
	graph->addLink(cid, str("%d.cpu2nic",i), "cpu", nic_link_lat);
	graph->addLink(cid, str("%d.nic2rtr",i), "rtr", nic_link_lat);

	cid = graph->addComponent(str("%d.rtr",i), "SS_router.SS_router");
	graph->addParams(cid, rtr_params);
	graph->addParameter(cid, "id", str("%d",i));
	graph->addLink(cid, str("%d.nic2rtr",i),"nic",nic_link_lat);

	if ( x_count > 1 ) {
	    graph->addLink(cid, str("xr2r.%d.%d.%d",y,z,((x+1)%x_count)), "xPos", rtr_link_lat);
	    graph->addLink(cid, str("xr2r.%d.%d.%d",y,z,x), "xNeg", rtr_link_lat);
	}
	
	if ( y_count > 1 ) {
	    graph->addLink(cid, str("yr2r.%d.%d.%d",x,z,((y+1)%y_count)), "yPos", rtr_link_lat);
	    graph->addLink(cid, str("yr2r.%d.%d.%d",x,z,y), "yNeg", rtr_link_lat);
	}
	
	if ( z_count > 1 ) {
	    graph->addLink(cid, str("zr2r.%d.%d.%d",x,y,((z+1)%z_count)), "zPos", rtr_link_lat);
	    graph->addLink(cid, str("zr2r.%d.%d.%d",x,y,z), "zNeg", rtr_link_lat);
	}
	
    }
}



static const ElementInfoPort tric_cpu_ports[] = {
    {"nic", "Network Interface port", NULL},
    {NULL, NULL, NULL}
};

static const ElementInfoPort tric_nic_ports[] = {
    {"cpu", "Connection to controlling CPU port", NULL},
    {"rtr", "Connection to Router port", NULL},
    {NULL, NULL, NULL}
};


static const ElementInfoComponent components[] = {
    { "trig_cpu",
      "Triggered CPU for Portals 4 research",
      NULL,
      create_trig_cpu,
      NULL, /* Params */
      tric_cpu_ports
    },
    { "trig_nic",
      "Triggered NIC for Portals 4 research",
      NULL,
      create_trig_nic,
      NULL, /* Params */
      tric_nic_ports
    },
    { NULL, NULL, NULL, NULL }
};

static const ElementInfoPartitioner partitioners[] = {
    { "partitioner",
      "Partitioner for portals4_sm simulations",
      NULL,
      Portals4Partition::allocate,
    },
    { NULL, NULL, NULL, NULL }
};
      
static const ElementInfoGenerator generators[] = {
    { "generator",
      "Generator for portals4_sm simulations",
      NULL,
      generate,
    },
    { NULL, NULL, NULL, NULL }
};
      
					     

extern "C" {
    ElementLibraryInfo portals4_sm_eli = {
        "portals4_sm",
        "State-machine based processor/nic for Portals 4 research",
        components,
        NULL,  // events
        NULL,  // introspectors
        NULL,  // modules
        NULL, // subcomponents
        partitioners,
        NULL,
        generators,
    };
}

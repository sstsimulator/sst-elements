//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// Rolf Riesen, October 2011
// This object extracts machine parameters from the SST params list
// and makes it available to other pieces of the pattern generators.
//

#include <sst_config.h>
#include <stdio.h>
#include <inttypes.h>		// For PRId64
#include "patterns.h"
#include "machine_info.h"



// Local functions
static bool compare_NICparams(NICparams_t first, NICparams_t second);



bool
MachineInfo::init(SST::Params& params)
{

int far_dest, far_dest_port, far_src, far_src_port;
FarLink_t fl;


    verbose= 0;
    debug= params.find_integer("debug", 0);
    _my_rank= params.find_integer("rank", -1);
    _my_core= -1;
    Net_width= params.find_integer("Net_x_dim", -1);
    Net_height= params.find_integer("Net_y_dim", -1);
    Net_depth= params.find_integer("Net_z_dim", -1);
    NoC_width= params.find_integer("NoC_x_dim", -1);
    NoC_height= params.find_integer("NoC_y_dim", -1);
    NoC_depth= params.find_integer("NoC_z_dim", -1);
    cores_per_NoC_router= params.find_integer("cores", -1);
    cores_per_Net_router= -1;
    num_router_nodes= params.find_integer("nodes", -1);

    // FIXME: Maybe some day I should make NIC_model_t an iterator
    for (int i= Net; i <= Far; i++)   {
        NICgap[i]= 0;
    }
    NICgap[Net] = params.find_integer("NetNICgap", 0);
    NICgap[NoC] = params.find_integer("NoCNICgap", 0);

    // FIXME: Make these a parameter
    NICsend_fraction[NoC]= 0.5;
    NICsend_fraction[Net]= 0.75;
    NICsend_fraction[Far]= 0.5;

    NetXwrap= params.find_integer("NetXwrap", 0);
    NetYwrap= params.find_integer("NetYwrap", 0);
    NetZwrap= params.find_integer("NetZwrap", 0);
    NoCXwrap= params.find_integer("NoCXwrap", 0);
    NoCYwrap= params.find_integer("NoCYwrap", 0);
    NoCZwrap= params.find_integer("NoCZwrap", 0);

    FarLinkPortFieldWidth= params.find_integer("FarLinkPortFieldWidth", 0);
    FarLinknum= 0;

    LinkBandwidth[Net] = params.find_integer("NetLinkBandwidth", 0);
    LinkBandwidth[NoC] = params.find_integer("NoCLinkBandwidth", 0);
    LinkLatency[Net] = params.find_integer("NetLinkLatency", 0);
    LinkLatency[NoC] = params.find_integer("NoCLinkLatency", 0);

    IOLinkBandwidth = params.find_integer("IOLinkBandwidth", 0);
    IOLinkLatency = params.find_integer("IOLinkLatency", 0);

    // Pre processing the parameter list
#define MAX_NIC 1024
	for ( int index = 0 ; index < MAX_NIC ; index++ ) {
		char param[64] = {0};
		uint64_t val = 0;
		bool pfound = false;

		sprintf(param, "NetNICinflection%d", index);
		val = params.find_integer(param, 0, pfound);
		if ( pfound ) {
			insert_inflection_point(Net, index, val);
		}

		sprintf(param, "NoCNICinflection%d", index);
		val = params.find_integer(param, 0, pfound);
		if ( pfound ) {
			insert_inflection_point(NoC, index, val);
		}

		sprintf(param, "NetNIClatnecy%d", index);
		val = params.find_integer(param, 0, pfound);
		if ( pfound ) {
			insert_inflection_latency(Net, index, val);
		}

		sprintf(param, "NoCNIClatnecy%d", index);
		val = params.find_integer(param, 0, pfound);
		if ( pfound ) {
			insert_inflection_latency(NoC, index, val);
		}


		sprintf(param, "NICstat%d", index);
		val = params.find_integer(param, 0, pfound);
		if ( pfound ) {
			NICstat_ranks.insert(val);
		}
	}




    if (_my_rank < 0)   {
	fprintf(stderr, "Need to specify my_rank in xml file!\n");
	return false;
    }


    // Now process far links.
    // I have to do this seperaty because I need to know what my node ID
    // and FarLinkPortFieldWidth is first.
    uint64_t code;
#define MAX_NODES 1024
#define MAX_PORTS 1024
	for ( far_src = 0 ; far_src < MAX_NODES ; far_src++ ) {
		for ( far_src_port = 0 ; far_src_port < MAX_PORTS ; far_src_port++ ) {
			char param[64] = {0};
			bool found = false;
			sprintf(param, "FarLink%dp%d", far_src, far_src_port);
			code = params.find_integer(param, 0, found);
			if ( found ) {
				far_dest= code >> FarLinkPortFieldWidth;
				far_dest_port= code & ((1 << FarLinkPortFieldWidth) - 1);

				if (far_src == myNode())   {
					// I can reach the destination node via a far link
					fl.dest= far_dest;
					fl.dest_port= far_dest_port;
					fl.src_port= far_src_port;
					FarLink.insert(fl);
				} else if (far_dest == myNode())   {
					// I am the destination of a far link; i.e.,
					// I can get to the source via the link.
					// We assume bidirection links
					fl.dest= far_src;
					fl.dest_port= far_src_port;
					fl.src_port= far_dest_port;
					FarLink.insert(fl);
				}
				// Count all links in system
				FarLinknum++;
			}
		}
	}

    if (!error_check())   {
	return false;
    }

    if (verbose)   {
	if (_my_rank == 0)   {
	    for (int i= Net; i <= Far; i++)   {
		if (i == Far && FarLinknum == 0)   {
		    continue;
		}
		printf("#  |||  %s NIC inflection points (sorted by inflection point)\n",
		    type_name((NIC_model_t)i));
		printf("#  |||      index inflection    latency\n");

		std::list<NICparams_t>::iterator k;
		for (k= NICparams[i].begin(); k != NICparams[i].end(); k++)   {
		    printf("#  |||      %3d %12" PRId64 " %12" PRId64 "\n", k->index,
			k->inflectionpoint, k->latency);
		}
	    }
	}
    }


    total_cores= Net_width * Net_height * Net_depth * num_router_nodes *
	         NoC_width * NoC_height * NoC_depth * cores_per_NoC_router;
    if (total_cores < 0)   {
	// That means one of the above parameters was not set in the XML file
	if (_my_rank == 0)   {
	    fprintf(stderr, "One of these params not set: Net_width, Net_height, "
		"Net_depth, num_router_nodes, NoC_width, NoC_height, NoC_depth, or cores_per_NoC_router\n");
	}
	return false;
    }

    cores_per_node= NoC_width * NoC_height * NoC_depth * cores_per_NoC_router;
    cores_per_Net_router= cores_per_node * num_router_nodes;

    // For now we have a one-to-one mapping
    _my_core= _my_rank;
    total_ranks= total_cores;

    if ((_my_rank < 0) || (_my_rank >= total_cores))   {
	fprintf(stderr, "My rank not 0 <= %d < %d (cores)\n", _my_rank, total_cores);
	return false;
    }


    // Let rank 0 display some info
    disp_banner(0);

    return true; /* success */

}  // end of init()



bool
MachineInfo::FarLinkExists(int dest_node)
{

FarLink_t fl;


    fl.dest= dest_node;
    if (FarLink.find(fl) != FarLink.end())   {
	return true;
    } else   {
	return false;
    }

}  // end of FarLinkExists()



int
MachineInfo::FarLinkExitPort(int dest_node)
{

FarLink_t fl;


    fl.dest= dest_node;
    return FarLink.find(fl)->src_port;

}  // end of FarLinkExitPort()



// This is the width of the main network and doesn't take nodes into account!
int
MachineInfo::myNetX(void)
{

int plane;


    plane= _my_rank % (Net_width * cores_per_Net_router * Net_height);
    return (plane % (Net_width * cores_per_Net_router)) / cores_per_Net_router;

}  // end of myNetX()



// This is the height of the main network and doesn't take nodes into account!
int
MachineInfo::myNetY(void)
{

int plane;


    plane= _my_rank % (Net_width * cores_per_Net_router * Net_height);
    return plane / (Net_width * cores_per_Net_router);

}  // end of myNetY()



int
MachineInfo::myNetZ(void)
{

    return _my_rank / (Net_width * cores_per_Net_router * Net_height);

}  // end of myNetY()



int
MachineInfo::myNoCX(void)
{

int node_core;
int plane;


    node_core= _my_rank % cores_per_Net_router;
    plane= node_core % (NoC_width * cores_per_NoC_router * NoC_height);
    return (plane % (NoC_width * cores_per_NoC_router)) / cores_per_NoC_router;

}  // end of myNoCX()



int
MachineInfo::myNoCY(void)
{

int node_core;
int plane;


    node_core= _my_rank % cores_per_Net_router;
    plane= node_core % (NoC_width * cores_per_NoC_router * NoC_height);
    return plane / (NoC_width * cores_per_NoC_router);

}  // end of myNoCY()



int
MachineInfo::myNoCZ(void)
{

int node_core;


    node_core= _my_rank % cores_per_Net_router;
    return node_core / (NoC_width * cores_per_NoC_router * NoC_height);

}  // end of myNoCY()



int
MachineInfo::myNode(void)
{

    return destNode(_my_rank);

}  // end of myNode()



int
MachineInfo::destNode(int dest_core)
{

    return dest_core / (NoC_width * NoC_height * NoC_depth * cores_per_NoC_router);

}  // end of destNode()



int
MachineInfo::myNetRouter(void)
{

    return destNetRouter(myNode());

}  // end of myNetRouter()



int
MachineInfo::destNetRouter(int dest_node)
{

    return dest_node / num_router_nodes;

}  // end of destNetRouter()



//
// Some private functions
//
void
MachineInfo::insert_inflection_point(NIC_model_t nic, int index, int64_t point)
{

std::list<NICparams_t>::iterator k;
bool found;


    // Is that index in the list already?
    found= false;
    for (k= NICparams[nic].begin(); k != NICparams[nic].end(); k++)   {
	if (k->index == index)   {
	    // Yes? Update the entry
	    k->inflectionpoint= point;
	    found= true;
	}
    }
    if (!found)   {
	// No? Create a new entry
	NICparams_t another;
	another.inflectionpoint= point;
	another.index= index;
	another.latency= -1;
	NICparams[nic].push_back(another);
    }

}  // end of insert_inflection_point()



void
MachineInfo::insert_inflection_latency(NIC_model_t nic, int index, int64_t latency)
{

std::list<NICparams_t>::iterator k;
bool found;


    // Is that index in the list already?
    found= false;
    for (k= NICparams[nic].begin(); k != NICparams[nic].end(); k++)   {
	if (k->index == index)   {
	    // Yes? Update the entry
	    k->latency= latency;
	    found= true;
	}
    }
    if (!found)   {
	// No? Create a new entry
	NICparams_t another;
	another.latency= latency;
	another.index= index;
	another.inflectionpoint= -1;
	NICparams[nic].push_back(another);
    }

}  // end of insert_inflection_latency()



bool
MachineInfo::error_check(void)
{

std::list<NICparams_t>::iterator k;


    for (int i= Net; i <= Far; i++)   {
	const char *name= type_name((NIC_model_t)i);

	if (i == Far && FarLinknum == 0)   {
	    continue;
	}

	// None of these should be 0 (I think)
	if (NICgap[i] == 0)   {
	    if (_my_rank == 0)   {
		fprintf(stderr, "Param not set: %sNICgap\n", name);
	    }
	    return false;
	}

	if (NICparams[i].size() < 2)   {
	    if (_my_rank == 0)   {
		fprintf(stderr, "Need at least two inflection point for the %s NIC\n",
		    name);
		fprintf(stderr, "    (%sNICinflectionX and %sNIClatencyX parameters)\n",
		    name, name);
	    }
	    return false;
	}

	// Check each entry
	for (k= NICparams[i].begin(); k != NICparams[i].end(); k++)   {
	    if (k->inflectionpoint < 0)   {
		fprintf(stderr, "%sNICinflection%d in xml file must be >= 0!\n",
		    name, (int)distance(NICparams[i].begin(), k));
		return false;
	    }
	    if (k->latency < 0)   {
		fprintf(stderr, "%sNIClatency%d in xml file must be >= 0!\n",
		    name, (int)distance(NICparams[i].begin(), k));
		return false;
	    }
	}

	// Sort the entries and do a basic check
	NICparams[i].sort(compare_NICparams);
	if (NICparams[i].front().inflectionpoint != 0)   {
	    fprintf(stderr, "Need a NICparams[%s] entry for inflection point 0\n", name);
	    return false;
	}
    }

    return true;

}  // end of error_check()



void
MachineInfo::disp_banner(int disp_rank)
{

    if (_my_rank == disp_rank)   {
	std::set<int>::iterator rank;

	printf("#  |||  Network x %d, y %d, z %d, with %d nodes each = %d nodes\n",
	    Net_width, Net_height, Net_depth, num_router_nodes,
	    Net_width * Net_height * Net_depth * num_router_nodes);
	printf("#  |||  Network: ");
	if (NetXwrap)   {
	    printf("X");
	}
	if (NetYwrap)   {
	    if (NetXwrap)   {
		printf(", Y");
	    } else   {
		printf("Y");
	    }
	}
	if (NetZwrap)   {
	    if (NetXwrap || NetYwrap)   {
		printf(", Z");
	    } else   {
		printf("Z");
	    }
	}
	if (!(NetXwrap || NetYwrap || NetZwrap))   {
	    printf("No");
	}
	printf(" dimensions wrapped\n");

	printf("#  |||  NoC x %d, y %d, z %d with %d cores each = %d cores per node\n",
	    NoC_width, NoC_height, NoC_depth, cores_per_NoC_router, cores_per_node);
	printf("#  |||  NoC: ");
	if (NoCXwrap)   {
	    printf("x");
	}
	if (NoCYwrap)   {
	    if (NoCXwrap)   {
		printf(", y");
	    } else   {
		printf("y");
	    }
	}
	if (NoCZwrap)   {
	    if (NoCXwrap || NoCYwrap)   {
		printf(", z");
	    } else   {
		printf("z");
	    }
	}
	if (!(NoCXwrap || NoCYwrap || NoCZwrap))   {
	    printf("No");
	}
	printf(" dimensions wrapped\n");

	printf("#  |||  Total %d cores in system\n", total_cores);

	printf("#  |||  NIC statistics for ranks ");
	for (rank= NICstat_ranks.begin(); rank != NICstat_ranks.end(); rank++)   {
	    printf("%d, ", *rank);
	}
	printf("\n");

	printf("#  |||  Network NIC model\n");
	printf("#  |||      gap           %0.9f s\n", NICgap[Net] / TIME_BASE_FACTOR);
	printf("#  |||      inflections   %11d\n", (int)NICparams[Net].size());
	printf("#  |||  Network link bandwidth %" PRId64 " B/s, latency %" PRId64 " %s\n",
	    LinkBandwidth[Net], LinkLatency[Net], TIME_BASE);
	printf("#  |||  NoC NIC model\n");
	printf("#  |||      gap           %0.9f s\n", NICgap[NoC] / TIME_BASE_FACTOR);
	printf("#  |||      inflections   %11d\n", (int)NICparams[NoC].size());
	printf("#  |||  NoC link bandwidth %" PRId64 " B/s, latency %" PRId64 " %s\n",
	    LinkBandwidth[NoC], LinkLatency[NoC], TIME_BASE);
	printf("#  |||  Number of far links (from node 0) = %d\n", (int)FarLink.size());
	printf("#  |||      Total in system (all nodes)   = %d\n", FarLinknum);
    }

}  // end of disp_banner()



//
// Some local functions
//


// Sort by inflectionpoint value
static bool
compare_NICparams(NICparams_t first, NICparams_t second)
{
    if (first.inflectionpoint < second.inflectionpoint)   {
	return true;
    } else   {
	return false;
    }
}  // end of compare_NICparams



#ifdef SERIALIZATION_WORKS_NOW
BOOST_CLASS_EXPORT(MachineInfo)
#endif // SERIALIZATION_WORKS_NOW

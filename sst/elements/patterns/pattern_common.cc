/*
** Copyright 2009-2011 Sandia Corporation. Under the terms
** of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
** Government retains certain rights in this software.
**
** Copyright (c) 2009-2011, Sandia Corporation
** All rights reserved.
**
** This file is part of the SST software package. For license
** information, see the LICENSE file in the top level directory of the
** distribution.
*/

/*
** This file contains common routines used by all pattern generators.
*/
#include <stdio.h>
#define __STDC_FORMAT_MACROS	(1)
#include <inttypes.h>		// For PRId64
#include <string>
#include <sst_config.h>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/set.hpp>
#include <sst/core/serialization/element.h>
#include <sst/core/cpunicEvent.h>

#include "pattern_common.h"


using namespace SST;


/* Local functions */
static void gen_route(CPUNicEvent *e, int src, int dest, int width, int height, int depth,
	int x_wrap, int y_wrap, int z_wrap);
static bool compare_NICparams(NICparams_t first, NICparams_t second);



/*
** Must be called before any other function in this file.
** Sets up some things based on stuff we learn when we read the
** SST xml file.
*/
int
Patterns::init(SST::Component::Params_t& params, Link *net_link, Link *self_link,
	SST::Link *NoC_link, SST::Link *nvram_link, SST::Link *storage_link, int verbose)
{

std::list<NICparams_t>::iterator k;
bool found;
int index;
int far_dest, far_dest_port, far_src, far_src_port;
int FarLinknum;
int my_node;
FarLink_t fl;
uint64_t code;


    my_rank= -1;
    mesh_width= -1;
    mesh_height= -1;
    mesh_depth= -1;
    NoC_width= -1;
    NoC_height= -1;
    NoC_depth= -1;
    cores_per_NoC_router= -1;
    num_router_nodes= -1;

    NetNICgap= 0;
    NoCNICgap= 0;

    NextNetNICslot= 0;
    NextNoCNICslot= 0;
    NextFarNICslot= 0;
    msg_seq= 1;
    FarLinknum= 0;
    NetXwrap= 0;
    NetYwrap= 0;
    NetZwrap= 0;
    NoCXwrap= 0;
    NoCYwrap= 0;
    NoCZwrap= 0;

    // Clear stats
    stat_NoCNICsend= 0;
    stat_NoCNICsend_bytes= 0;
    stat_NoCNICbusy= 0;
    stat_NetNICsend= 0;
    stat_NetNICsend_bytes= 0;
    stat_NetNICbusy= 0;
    stat_FarNICsend= 0;
    stat_FarNICsend_bytes= 0;
    stat_FarNICbusy= 0;
    stat_NetNICrecv= 0;
    stat_NetNICrecv_bytes= 0;
    stat_Netmsg_hops= 0;
    stat_Netmsg_congestion_cnt= 0;
    stat_Netmsg_congestion_delay= 0;
    stat_NoCNICrecv= 0;
    stat_NoCNICrecv_bytes= 0;
    stat_NoCmsg_hops= 0;
    stat_NoCmsg_congestion_cnt= 0;
    stat_NoCmsg_congestion_delay= 0;

    // Pre processing the parameter list
    // Go through the parameter list to find my_rank.
    // We need it to process far links and print debug info
    // We also need to know what FarLinkPortFieldWidth is.
    SST::Component::Params_t::iterator it= params.begin();
    while (it != params.end())   {
	if (!it->first.compare("rank"))   {
	    sscanf(it->second.c_str(), "%d", &my_rank);
	}
	if (!it->first.compare("FarLinkPortFieldWidth"))   {
	    sscanf(it->second.c_str(), "%d", &FarLinkPortFieldWidth);
	}
	it++;
    }

    if (my_rank < 0)   {
	fprintf(stderr, "Need to specify my_rank in xml file!\n");
	return FALSE;
    }


    // Now that we have my_rank, process the other parameters
    it= params.begin();
    while (it != params.end())   {
	if (!it->first.compare("Net_x_dim"))   {
	    sscanf(it->second.c_str(), "%d", &mesh_width);
	}

	if (!it->first.compare("Net_y_dim"))   {
	    sscanf(it->second.c_str(), "%d", &mesh_height);
	}

	if (!it->first.compare("Net_z_dim"))   {
	    sscanf(it->second.c_str(), "%d", &mesh_depth);
	}

	if (!it->first.compare("NoC_x_dim"))   {
	    sscanf(it->second.c_str(), "%d", &NoC_width);
	}

	if (!it->first.compare("NoC_y_dim"))   {
	    sscanf(it->second.c_str(), "%d", &NoC_height);
	}

	if (!it->first.compare("NoC_z_dim"))   {
	    sscanf(it->second.c_str(), "%d", &NoC_depth);
	}

	if (!it->first.compare("NetXwrap"))   {
	    sscanf(it->second.c_str(), "%d", &NetXwrap);
	}

	if (!it->first.compare("NetYwrap"))   {
	    sscanf(it->second.c_str(), "%d", &NetYwrap);
	}

	if (!it->first.compare("NetZwrap"))   {
	    sscanf(it->second.c_str(), "%d", &NetZwrap);
	}

	if (!it->first.compare("NoCXwrap"))   {
	    sscanf(it->second.c_str(), "%d", &NoCXwrap);
	}

	if (!it->first.compare("NoCYwrap"))   {
	    sscanf(it->second.c_str(), "%d", &NoCYwrap);
	}

	if (!it->first.compare("NoCZwrap"))   {
	    sscanf(it->second.c_str(), "%d", &NoCZwrap);
	}

	if (!it->first.compare("cores"))   {
	    sscanf(it->second.c_str(), "%d", &cores_per_NoC_router);
	}

	if (!it->first.compare("nodes"))   {
	    sscanf(it->second.c_str(), "%d", &num_router_nodes);
	}

	if (!it->first.compare("NetNICgap"))   {
	    sscanf(it->second.c_str(), "%" PRId64, &NetNICgap);
	}

	if (!it->first.compare("NoCNICgap"))   {
	    sscanf(it->second.c_str(), "%" PRId64, &NoCNICgap);
	}

	if (!it->first.compare("NetLinkBandwidth"))   {
	    sscanf(it->second.c_str(), "%" PRId64, &NetLinkBandwidth);
	}

	if (!it->first.compare("NetLinkLatency"))   {
	    sscanf(it->second.c_str(), "%" PRId64, &NetLinkLatency);
	}

	if (!it->first.compare("NoCLinkBandwidth"))   {
	    sscanf(it->second.c_str(), "%" PRId64, &NoCLinkBandwidth);
	}

	if (!it->first.compare("NoCLinkLatency"))   {
	    sscanf(it->second.c_str(), "%" PRId64, &NoCLinkLatency);
	}

	if (!it->first.compare("IOLinkBandwidth"))   {
	    sscanf(it->second.c_str(), "%" PRId64, &IOLinkBandwidth);
	}

	if (!it->first.compare("IOLinkLatency"))   {
	    sscanf(it->second.c_str(), "%" PRId64, &IOLinkLatency);
	}

	if (it->first.find("NetNICinflection") != std::string::npos)   {
	    if (sscanf(it->first.c_str(), "NetNICinflection%d", &index) != 1)   {
		it++;
		continue;
	    }
	    // Is that index in the list already?
	    found= false;
	    for (k= NetNICparams.begin(); k != NetNICparams.end(); k++)   {
		if (k->index == index)   {
		    // Yes? Update the entry
		    k->inflectionpoint= strtoll(it->second.c_str(), (char **)NULL, 0);
		    found= true;
		}
	    }
	    if (!found)   {
		// No? Create a new entry
		NICparams_t another;
		another.inflectionpoint= strtoll(it->second.c_str(), (char **)NULL, 0);
		another.index= index;
		another.latency= -1;
		NetNICparams.push_back(another);
	    }
	}

	if (it->first.find("NoCNICinflection") != std::string::npos)   {
	    if (sscanf(it->first.c_str(), "NoCNICinflection%d", &index) != 1)   {
		it++;
		continue;
	    }
	    // Is that index in the list already?
	    found= false;
	    for (k= NoCNICparams.begin(); k != NoCNICparams.end(); k++)   {
		if (k->index == index)   {
		    // Yes? Update the entry
		    k->inflectionpoint= strtoll(it->second.c_str(), (char **)NULL, 0);
		    found= true;
		}
	    }
	    if (!found)   {
		// No? Create a new entry
		NICparams_t another;
		another.inflectionpoint= strtoll(it->second.c_str(), (char **)NULL, 0);
		another.index= index;
		another.latency= -1;
		NoCNICparams.push_back(another);
	    }
	}

	if (it->first.find("NetNIClatency") != std::string::npos)   {
	    if (sscanf(it->first.c_str(), "NetNIClatency%d", &index) != 1)   {
		it++;
		continue;
	    }
	    // Is that index in the list already?
	    found= false;
	    for (k= NetNICparams.begin(); k != NetNICparams.end(); k++)   {
		if (k->index == index)   {
		    // Yes? Update the entry
		    k->latency= strtoll(it->second.c_str(), (char **)NULL, 0);
		    found= true;
		}
	    }
	    if (!found)   {
		// No? Create a new entry
		NICparams_t another;
		another.latency= strtoll(it->second.c_str(), (char **)NULL, 0);
		another.index= index;
		another.inflectionpoint= -1;
		NetNICparams.push_back(another);
	    }
	}

	if (it->first.find("NoCNIClatency") != std::string::npos)   {
	    if (sscanf(it->first.c_str(), "NoCNIClatency%d", &index) != 1)   {
		it++;
		continue;
	    }
	    // Is that index in the list already?
	    found= false;
	    for (k= NoCNICparams.begin(); k != NoCNICparams.end(); k++)   {
		if (k->index == index)   {
		    // Yes? Update the entry
		    k->latency= strtoll(it->second.c_str(), (char **)NULL, 0);
		    found= true;
		}
	    }
	    if (!found)   {
		// No? Create a new entry
		NICparams_t another;
		another.latency= strtoll(it->second.c_str(), (char **)NULL, 0);
		another.index= index;
		another.inflectionpoint= -1;
		NoCNICparams.push_back(another);
	    }
	}

	if (it->first.find("NICstat") != std::string::npos)   {
	    if (sscanf(it->first.c_str(), "NICstat%d", &index) == 1)   {
		int r;
		sscanf(it->second.c_str(), "%d", &r);
		NICstat_ranks.push_back(r);
	    }
	}

	it++;
    }


    // Now process far links.
    // I have to do this seperaty because I need NoC_width, NoC_height, cores_per_NoC_router
    // to compute my node ID
    //
    it= params.begin();
    while (it != params.end())   {
	if (it->first.find("FarLink") != std::string::npos)   {
	    if (sscanf(it->first.c_str(), "FarLink%dp%d", &far_src, &far_src_port) == 2)   {
		sscanf(it->second.c_str(), "%" PRId64, &code);
		my_node= my_rank / (NoC_width * NoC_height * NoC_depth * cores_per_NoC_router);
		far_dest= code >> FarLinkPortFieldWidth;
		far_dest_port= code & ((1 << FarLinkPortFieldWidth) - 1);

		if (my_rank == 0)   {
		}
		if (far_src == my_node)   {
		    // I can reach the destination node via a far link
		    fl.dest= far_dest;
		    fl.dest_port= far_dest_port;
		    fl.src_port= far_src_port;
		    FarLink.insert(fl);
		} else if (far_dest == my_node)   {
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

	it++;
    }

    // None of these should be 0 (I think)
    if ((NetNICgap == 0) ||
	(NoCNICgap == 0))   {

	if (my_rank == 0)   {
	    fprintf(stderr, "One of these params not set: NetNICgap, or NoCNICgap\n");
	}
	return FALSE;
    }

    if (NetNICparams.size() < 2)   {
	if (my_rank == 0)   {
	    fprintf(stderr, "Need at least two inflection point for the network NIC\n");
	    fprintf(stderr, "    (NetNICinflectionX and NetNIClatencyX parameters)\n");
	}
	return FALSE;
    }

    if (NoCNICparams.size() < 2)   {
	if (my_rank == 0)   {
	    fprintf(stderr, "Need at least two inflection point for the NoC NIC\n");
	    fprintf(stderr, "    (NoCNICinflectionX and NoCNIClatencyX parameters)\n");
	}
	return FALSE;
    }

    // Check each entry
    for (k= NetNICparams.begin(); k != NetNICparams.end(); k++)   {
	if (k->inflectionpoint < 0)   {
	    fprintf(stderr, "NetNICinflection%d in xml file must be >= 0!\n",
		(int)distance(NetNICparams.begin(), k));
	    return FALSE;
	}
	if (k->latency < 0)   {
	    fprintf(stderr, "NetNIClatency%d in xml file must be >= 0!\n",
		(int)distance(NetNICparams.begin(), k));
	    return FALSE;
	}
    }
    for (k= NoCNICparams.begin(); k != NoCNICparams.end(); k++)   {
	if (k->inflectionpoint < 0)   {
	    fprintf(stderr, "NoCNICinflection%d in xml file must be >= 0!\n",
		(int)distance(NoCNICparams.begin(), k));
	    return FALSE;
	}
	if (k->latency < 0)   {
	    fprintf(stderr, "NoCNIClatency%d in xml file must be >= 0!\n",
		(int)distance(NoCNICparams.begin(), k));
	    return FALSE;
	}
    }

    // Sort the entries and do a basic check
    NetNICparams.sort(compare_NICparams);
    if (NetNICparams.front().inflectionpoint != 0)   {
	fprintf(stderr, "Need a NetNICparams entry for inflection point 0\n");
	return FALSE;
    }

    NoCNICparams.sort(compare_NICparams);
    if (NoCNICparams.front().inflectionpoint != 0)   {
	fprintf(stderr, "Need a NoCNICparams entry for inflection point 0\n");
	return FALSE;
    }

    if (verbose)   {
	if (my_rank == 0)   {
	    printf("#  |||  Network NIC inflection points (sorted by inflection point)\n");
	    printf("#  |||      index inflection    latency\n");
	    for (k= NetNICparams.begin(); k != NetNICparams.end(); k++)   {
		printf("#  |||      %3d %12" PRId64 " %12" PRId64 "\n", k->index,
		    k->inflectionpoint, k->latency);
	    }
	}
	if (my_rank == 0)   {
	    printf("#  |||  NoC NIC inflection points (sorted by inflection point)\n");
	    printf("#  |||      index inflection    latency\n");
	    for (k= NoCNICparams.begin(); k != NoCNICparams.end(); k++)   {
		printf("#  |||      %3d %12" PRId64 " %12" PRId64 "\n", k->index,
		    k->inflectionpoint, k->latency);
	    }
	}
    }


    total_cores= mesh_width * mesh_height * mesh_depth * num_router_nodes *
	         NoC_width * NoC_height * NoC_depth * cores_per_NoC_router;
    if (total_cores < 0)   {
	// That means one of the above parameters was not set in the XML file
	if (my_rank == 0)   {
	    fprintf(stderr, "One of these params not set: mesh_width, mesh_height, "
		"mesh_depth, num_router_nodes, NoC_width, NoC_height, NoC_depth, or cores_per_NoC_router\n");
	}
	return FALSE;
    }

    cores_per_node= NoC_width * NoC_height * NoC_depth * cores_per_NoC_router;
    cores_per_Net_router= cores_per_node * num_router_nodes;

    if ((my_rank < 0) || (my_rank >= total_cores))   {
	fprintf(stderr, "My rank not 0 <= %d < %d (cores)\n", my_rank, total_cores);
	return FALSE;
    }

    my_net_link= net_link;
    my_self_link= self_link;
    my_NoC_link= NoC_link;
    my_nvram_link= nvram_link;
    my_storage_link= storage_link;


    if (my_rank == 0)   {
	std::list<int>::iterator rank;

	printf("#  |||  Network x %d, y %d, z %d, with %d nodes each = %d nodes\n",
	    mesh_width, mesh_height, mesh_depth, num_router_nodes,
	    mesh_width * mesh_height * mesh_depth * num_router_nodes);
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
	printf("#  |||      gap           %0.9f s\n", NetNICgap / TIME_BASE_FACTOR);
	printf("#  |||      inflections   %11d\n", (int)NetNICparams.size());
	printf("#  |||  Network link bandwidth %" PRId64 " B/s, latency %" PRId64 " %s\n",
	    NetLinkBandwidth, NetLinkLatency, TIME_BASE);
	printf("#  |||  NoC NIC model\n");
	printf("#  |||      gap           %0.9f s\n", NoCNICgap / TIME_BASE_FACTOR);
	printf("#  |||      inflections   %11d\n", (int)NoCNICparams.size());
	printf("#  |||  NoC link bandwidth %" PRId64 " B/s, latency %" PRId64 " %s\n",
	    NoCLinkBandwidth, NoCLinkLatency, TIME_BASE);
	printf("#  |||  Number of far links (from node 0) = %d\n", (int)FarLink.size());
	printf("#  |||      Total in system (all nodes)   = %d\n", FarLinknum);
    }

    return TRUE; /* success */

}  /* end of init() */



// Save the lower 27 bits of the event ID for the rank number
#define RANK_FIELD		(27)


// There is a lot of hardcoded information about how the network is wired.
// the program genPatterns generates XML files with routers that have
// the following port connections:
// Port 0: East/Left
// Port 1: South/Down
// Port 2: West/Right
// Port 3: North/Up
// Port 4...: local links to cores or nodes

#define EAST_PORT		(0)
#define SOUTH_PORT		(1)
#define WEST_PORT		(2)
#define NORTH_PORT		(3)
#define BACK_PORT		(4)
#define FRONT_PORT		(5)
#define FIRST_LOCAL_PORT	(6)



void
Patterns::self_event_send(int event, int32_t tag, SST::SimTime_t delay)
{

CPUNicEvent *e;


    // Create an event and fill in the event info
    e= new CPUNicEvent();
    e->SetRoutine((int)event);
    e->router_delay= 0;
    e->hops= 0;
    e->msg_len= 0;
    e->tag= tag;
    e->dest= my_rank;
    e->msg_id= (msg_seq++ << RANK_FIELD) | my_rank;

    my_self_link->Send(delay, e);

}  // end of self_event_send()



/*
** This function computes a route, based on the assumption that we
** are on a x * y torus for our NoC, and a x * y torus for the network.
** This function then sends an event along that route.
** No actual data is sent.
*/
void
Patterns::event_send(int dest, int event, SST::SimTime_t CurrentSimTime,
	int32_t tag, uint32_t msg_len, const char *payload, int payload_len, int blocking)
{

CPUNicEvent *e;
int my_node, dest_node;
FarLink_t fl;
stat_dest_t dl;
std::set<stat_dest_t>::iterator pos;


    // Create an event and fill in the event info
    e= new CPUNicEvent();
    e->SetRoutine((int)event);
    e->router_delay= 0;
    e->hops= 0;
    e->msg_len= msg_len;
    e->tag= tag;
    e->dest= dest;
    e->msg_id= (msg_seq++ << RANK_FIELD) | my_rank;

    // If there is a payload, attach it
    if (payload)   {
	e->AttachPayload(payload, payload_len);
    }

    my_node= my_rank / (NoC_width * NoC_height * NoC_depth * cores_per_NoC_router);
    dest_node= dest / (NoC_width * NoC_height * NoC_depth * cores_per_NoC_router);

    // Count number of sends to each destination
    dl.dest= dest_node;
    pos= stat_dest.find(dl);
    if (pos != stat_dest.end())   {
	// Update. FIXME: This seems overly complicated...
	dl.cnt= pos->cnt + 1;
	stat_dest.erase(dl);
	stat_dest.insert(dl);
	pos= stat_dest.find(dl);
    } else   {
	// Never sent to this node before. Create an entry.
	dl.dest= dest_node;
	dl.cnt= 0;
	stat_dest.insert(dl);
    }

    if (dest == my_rank)   {
	// No need to go through the network for this
	// FIXME: Shouldn't this involve some sort of delay?
	if (blocking >= 0)   {
	    send_msg_complete(0, blocking, tag);
	}
	my_self_link->Send(e);
	return;
    }

    /* Is dest within our NoC? */
    if (my_node == dest_node)   {
	/* Route locally */
	NoCsend(e, my_rank, dest, CurrentSimTime, blocking, tag);

    } else   {
	/* Route off chip */

	fl.dest= dest_node;
	if (FarLink.find(fl) != FarLink.end())   {
	    // We have a far link to that destination node. Use it
	    FarLinksend(e, my_node, dest_node, dest, CurrentSimTime, blocking, tag);
	} else   {
	    // Send it through the network
	    Netsend(e, my_node, dest_node, dest, CurrentSimTime, blocking, tag);
	}
    }

}  /* end of event_send() */



/*
** Send a chunk of data to our stable storage device.
*/
void
Patterns::storage_write(int data_size, int return_event)
{

CPUNicEvent *e;
int my_node;
int my_core;
uint64_t delay;


    // Create an event and fill in the event info
    e= new CPUNicEvent();
    assert(0);  // FIXME: We need to SetRoutine()
    // e->SetRoutine(BIT_BUCKET_WRITE_START);
    e->router_delay= 0;
    e->hops= 0;
    e->msg_len= data_size;
    e->dest= -1;
    e->msg_id= (msg_seq++ << RANK_FIELD) | my_rank;
    e->return_event= return_event;

    // Storage request go out on port 0 of the aggregator
    e->route.push_back(0);

    // And then again on port 0 at the node aggregator
    e->route.push_back(0);

    // Coming back: First hop is to the right node
    my_node= my_rank / (NoC_width * NoC_height * NoC_depth * cores_per_NoC_router);
    e->reverse_route.push_back(my_node + 1);	// Port 0 is for the SSD

    // From the aggregator, pick the right core
    my_core= my_rank % cores_per_node;
    e->reverse_route.push_back(my_core + 1);	// Port 0 goes off node

    // Send the write request
    delay= 0; // FIXME; Need to figure this out
    my_storage_link->Send(delay, e);

}  /* end of storage_write() */



/*
** Send a chunk of data to our local NVRAM
*/
void
Patterns::nvram_write(int data_size, int return_event)
{

CPUNicEvent *e;
int my_core;
uint64_t delay;


    // Create an event and fill in the event info
    e= new CPUNicEvent();
    // assert(0);  // FIXME: We need to SetRoutine()
    // e->SetRoutine(BIT_BUCKET_WRITE_START);
    e->router_delay= 0;
    e->hops= 0;
    e->msg_len= data_size;
    e->dest= -1;
    e->msg_id= (msg_seq++ << RANK_FIELD) | my_rank;
    e->return_event= return_event;

    // NVRAM requests go out on port 0 of the aggregator
    e->route.push_back(0);

    // Coming back: Only one hop back to us
    my_core= my_rank % cores_per_node;
    e->reverse_route.push_back(my_core + 1);	// Port 0 goes off node

    // Send the write request
    delay= 0; // FIXME; Need to figure this out
    my_nvram_link->Send(delay, e);

}  /* end of nvram_write() */



/*
** -----------------------------------------------------------------------------
** Some local functions
** -----------------------------------------------------------------------------
*/


#define ROUTE_DEBUG 1
#undef ROUTE_DEBUG
#if ROUTE_DEBUG
void
show_route(std::string net_name, CPUNicEvent *e, int src_rank, int dest_rank)    {

char route[128];
char tmp[128];
route[0]= 0;
tmp[0]= 0;
std::vector<int>::iterator itNum;

    for(itNum = e->route.begin(); itNum < e->route.end(); itNum++)   {
	sprintf(tmp, "%s.%d", route, *itNum);
	strcpy(route, tmp);
    }

    fprintf(stderr, "%s from %d --> %d, route %s\n", net_name.c_str(),
	 src_rank, dest_rank, route);

}  // end of show_route()
#endif  // ROUTE_DEBUG



/*
** Send a message into the Network on Chip with the appropriate
** delays.
*/
void
Patterns::NoCsend(CPUNicEvent *e, int my_rank, int dest_rank, SST::SimTime_t CurrentSimTime,
	int blocking, int tag)
{

int my_router, dest_router;
SimTime_t delay;
int64_t latency;
int64_t msg_duration;
int64_t link_duration;


    /*
    ** For cores that share a router we'll assume that they
    ** can communicate with each other much faster; e.g., through
    ** a shared L2 cache. We mark that traffic so the router can
    ** deal with it seperatly.
    */
    my_router= my_rank / cores_per_NoC_router;
    dest_router= dest_rank / cores_per_NoC_router;
    if (my_router != dest_router)   {
	// This message travels through the NoC
	e->local_traffic= false;
    } else   {
	// This is a message among cores of the same router
	e->local_traffic= true;
    }

    gen_route(e, my_router, dest_router, NoC_width, NoC_height, NoC_depth, NoCXwrap, NoCYwrap, NoCZwrap);

    // Exit to the local (NIC) pattern generator
    e->route.push_back(FIRST_LOCAL_PORT + (dest_rank % cores_per_NoC_router));

#if ROUTE_DEBUG
    show_route("NoC", e, my_rank, dest_rank);
#endif

    // Calculate when this message can leave the NIC
    stat_NoCNICsend++;
    stat_NoCNICsend_bytes += e->msg_len;
    get_NICparams(NoCNICparams, e->msg_len, &latency, &msg_duration);

    // How long will this message occupy the outbound link?
    link_duration= e->msg_len / NoCLinkBandwidth;

    if (CurrentSimTime > NextNoCNICslot)   {
	// NIC is not busy

	// We move a portion of the delay to the link, so SST can use it for
	// scheduling and partitioning.
	delay= latency + msg_duration - link_duration - NoCLinkLatency;
	if (delay < 0)   {
	    delay= 0;
	}
	if (blocking >= 0)   {
	    send_msg_complete(delay, blocking, tag);
	}
	my_NoC_link->Send(delay, e);
	NextNoCNICslot= CurrentSimTime + latency + msg_duration + link_duration;

    } else   {
	// NIC is busy
	delay= NextNoCNICslot - CurrentSimTime + NoCNICgap +
	    latency + msg_duration - link_duration - NoCLinkLatency;
	if (delay < 0)   {
	    delay= 0;
	}
	if (blocking >= 0)   {
	    send_msg_complete(delay, blocking, tag);
	}
	my_NoC_link->Send(delay, e);
	NextNetNICslot= CurrentSimTime + delay + latency + msg_duration + link_duration;
	stat_NoCNICbusy++;
    }


}  // end of NoCsend()



/*
** Send a message into the global Network with the
** appropriate delays.
*/
void
Patterns::Netsend(CPUNicEvent *e, int my_node, int dest_node, int dest_rank,
	SST::SimTime_t CurrentSimTime, int blocking, int tag)
{

int my_router, dest_router;
SimTime_t delay;
int64_t latency;
int64_t msg_duration;
int64_t link_duration;


    e->local_traffic= false;

    /* On the network aggregator we'll have to go out port 0 to reach the mesh */
    e->route.push_back(0);

    my_router= my_node / num_router_nodes;
    dest_router= dest_node / num_router_nodes;
    gen_route(e, my_router, dest_router, mesh_width, mesh_height, mesh_depth, NetXwrap, NetYwrap, NetZwrap);


    // Exit to the local node (aggregator on that node
    e->route.push_back(FIRST_LOCAL_PORT +
	    ((dest_rank % (num_router_nodes * cores_per_node)) / cores_per_node));

    /*
    ** We come out at the aggregator on the destination node.
    ** Figure out which port to go out to reach the destination core
    ** Port 0 is where we came in
    */
    e->route.push_back(1 + (dest_rank % cores_per_node));

#if ROUTE_DEBUG
    show_route("Net", e, my_rank, dest_rank);
#endif

    // Calculate when this message can leave the NIC
    stat_NetNICsend++;
    stat_NetNICsend_bytes += e->msg_len;
    get_NICparams(NetNICparams, e->msg_len, &latency, &msg_duration);

    // How long will this message occupy the outbound link?
    link_duration= e->msg_len / NetLinkBandwidth;

    if (CurrentSimTime > NextNetNICslot)   {
	// NIC is not busy

	// We move a portion of the delay to the link, so SST can use it for
	// scheduling and partitioning.
	delay= latency + msg_duration - link_duration - NetLinkLatency;
	if (delay < 0)   {
	    delay= 0;
	}
	if (blocking >= 0)   {
	    send_msg_complete(delay, blocking, tag);
	}
	my_net_link->Send(delay, e);
	NextNetNICslot= CurrentSimTime + latency + msg_duration + link_duration;

    } else   {
	// NIC is busy
	delay= NextNetNICslot - CurrentSimTime + NetNICgap +
	    latency + msg_duration - link_duration - NetLinkLatency;
	if (delay < 0)   {
	    delay= 0;
	}
	if (blocking >= 0)   {
	    send_msg_complete(delay, blocking, tag);
	}
	my_net_link->Send(delay, e);
	NextNetNICslot= CurrentSimTime + delay + latency + msg_duration + link_duration;
	stat_NetNICbusy++;
    }

}  // end of Netsend()



/*
** Send a message through a far link
*/
void
Patterns::FarLinksend(CPUNicEvent *e, int my_node, int dest_node, int dest_rank,
	SST::SimTime_t CurrentSimTime, int blocking, int tag)
{

SimTime_t delay;
int64_t latency;
int64_t msg_duration;
int64_t link_duration;
FarLink_t fl;


    e->local_traffic= false;

    // On the network aggregator we'll have to go out the specified port to
    // reach the appropriate far link
    fl.dest= dest_node;
    e->route.push_back(FarLink.find(fl)->src_port);


    /*
    ** We come out at the aggregator on the destination node.
    ** Figure out which port to go out to reach the destination core
    ** Port 0 is where we came in
    */
    e->route.push_back(1 + (dest_rank % cores_per_node));

#if ROUTE_DEBUG
    show_route("Far", e, my_rank, dest_rank);
#endif

    // Calculate when this message can leave the NIC
    stat_FarNICsend++;
    stat_FarNICsend_bytes += e->msg_len;

    // We assume the far link is connected to a regular NIC
    get_NICparams(NetNICparams, e->msg_len, &latency, &msg_duration);

    // How long will this message occupy the outbound link?
    link_duration= e->msg_len / NetLinkBandwidth;

    if (CurrentSimTime > NextFarNICslot)   {
	// NIC is not busy

	// We move a portion of the delay to the link, so SST can use it for
	// scheduling and partitioning.
	delay= latency + msg_duration - link_duration - NetLinkLatency;
	if (delay < 0)   {
	    delay= 0;
	}
	if (blocking >= 0)   {
	    send_msg_complete(delay, blocking, tag);
	}
	my_net_link->Send(delay, e);
	NextFarNICslot= CurrentSimTime + latency + msg_duration + link_duration;

    } else   {
	// NIC is busy
	delay= NextFarNICslot - CurrentSimTime + NetNICgap +
	    latency + msg_duration - link_duration - NetLinkLatency;
	if (delay < 0)   {
	    delay= 0;
	}
	if (blocking >= 0)   {
	    send_msg_complete(delay, blocking, tag);
	}
	my_net_link->Send(delay, e);
	NextFarNICslot= CurrentSimTime + delay + latency + msg_duration + link_duration;
	stat_FarNICbusy++;
    }

}  // end of FarLinksend()



// Send a send completion event to ourselves
void
Patterns::send_msg_complete(SimTime_t delay, int blocking, int tag)
{

CPUNicEvent *e;


    // Create an event and fill in the event info
    e= new CPUNicEvent();
    e->SetRoutine(blocking);
    e->router_delay= 0;
    e->hops= 0;
    e->msg_len= 0;
    e->tag= tag;
    e->dest= my_rank;
    e->msg_id= (msg_seq++ << RANK_FIELD) | my_rank;

    my_self_link->Send(delay, e);

}  // end of send_msg_complete



/*
** Generate a route through a X * Y torus
** We use it to traverse the mesh as well as the NoC
*/
static void
gen_route(CPUNicEvent *e, int src, int dest, int width, int height, int depth,
	int x_wrap, int y_wrap, int z_wrap)
{

int c1, c2;
int my_X, my_Y, my_Z;
int dest_X, dest_Y, dest_Z;
int x_delta, y_delta, z_delta;


// fprintf(stderr, "[%3d] to %3d gen_route(x %2d * y %2d * z %2d, wrap %d %d %d)\n", src, dest, width, height, depth, x_wrap, y_wrap, z_wrap);

    // Calculate x, y and z coordinates within the Network or NoC
    my_Y= (src % (width * height)) / width;
    my_X= (src % (width * height)) % width;
    my_Z= src / (width * height);

    dest_Y= (dest % (width * height)) / width;
    dest_X= (dest % (width * height)) % width;
    dest_Z= dest / (width * height);

    // Use the shortest distance to reach the destination
    if (x_wrap)   {
	if (my_X > dest_X)   {
	    // How much does it cost to go left?
	    c1= my_X - dest_X;
	    // How much does it cost to go right?
	    c2= (dest_X + width) - my_X;
	} else   {
	    // How much does it cost to go left?
	    c1= (my_X + width) - dest_X;
	    // How much does it cost to go right?
	    c2= dest_X - my_X;
	}
	if (c1 > c2)   {
	    // go right
	    x_delta= c2;
	} else   {
	    // go left
	    x_delta= -c1;
	}
    } else   {
	x_delta= dest_X - my_X;
    }

    if (y_wrap)   {
	if (my_Y > dest_Y)   {
	    // How much does it cost to go up?
	    c1= my_Y - dest_Y;
	    // How much does it cost to go down?
	    c2= (dest_Y + height) - my_Y;
	} else   {
	    // How much does it cost to go up?
	    c1= (my_Y + height) - dest_Y;
	    // How much does it cost to go down?
	    c2= dest_Y - my_Y;
	}
	if (c1 > c2)   {
	    // go down
	    y_delta= c2;
	} else   {
	    // go up
	    y_delta= -c1;
	}
    } else   {
	y_delta= dest_Y - my_Y;
    }

    if (z_wrap)   {
	if (my_Z > dest_Z)   {
	    // How much does it cost to go back?
	    c1= my_Z - dest_Z;
	    // How much does it cost to go forward?
	    c2= (dest_Z + depth) - my_Z;
	} else   {
	    // How much does it cost to go back?
	    c1= (my_Z + depth) - dest_Z;
	    // How much does it cost to go forward?
	    c2= dest_Z - my_Z;
	}
	if (c1 > c2)   {
	    // go back
	    z_delta= c2;
	} else   {
	    // go forward
	    z_delta= -c1;
	}
    } else   {
	z_delta= dest_Z - my_Z;
    }

    if (x_delta > 0)   {
	// Go East
	while (x_delta > 0)   {
	    e->route.push_back(EAST_PORT);
	    x_delta--;
	}
    } else if (x_delta < 0)   {
	// Go West
	while (x_delta < 0)   {
	    e->route.push_back(WEST_PORT);
	    x_delta++;
	}
    }

    if (y_delta > 0)   {
	// Go SOUTH
	while (y_delta > 0)   {
	    e->route.push_back(SOUTH_PORT);
	    y_delta--;
	}
    } else if (y_delta < 0)   {
	// Go NORTH
	while (y_delta < 0)   {
	    e->route.push_back(NORTH_PORT);
	    y_delta++;
	}
    }

    if (z_delta > 0)   {
	// Go BACK
	while (z_delta > 0)   {
	    e->route.push_back(BACK_PORT);
	    z_delta--;
	}
    } else if (z_delta < 0)   {
	// Go NORTH
	while (z_delta < 0)   {
	    e->route.push_back(FRONT_PORT);
	    z_delta++;
	}
    }

}  // end of gen_route()



void
Patterns::record_Net_msg_stat(int hops, long long congestion_cnt,
	SimTime_t congestion_delay, uint32_t msg_len)
{

    stat_NetNICrecv++;
    stat_NetNICrecv_bytes += msg_len;
    stat_Netmsg_hops += hops;
    stat_Netmsg_congestion_cnt += congestion_cnt;
    stat_Netmsg_congestion_delay += congestion_delay;

}  // End of record_Net_msg_stat()



void
Patterns::record_NoC_msg_stat(int hops, long long congestion_cnt,
	SimTime_t congestion_delay, uint32_t msg_len)
{

    stat_NoCNICrecv++;
    stat_NoCNICrecv_bytes += msg_len;
    stat_NoCmsg_hops += hops;
    stat_NoCmsg_congestion_cnt += congestion_cnt;
    stat_NoCmsg_congestion_delay += congestion_delay;

}  // End of record_NoC_msg_stat()



void
Patterns::stat_print(void)
{

std::list<int>::iterator rank;
std::set<stat_dest_t>::iterator pos;


    for (rank= NICstat_ranks.begin(); rank != NICstat_ranks.end(); rank++)   {
	if (my_rank == *rank)   {
	    printf("# [%3d] NoC NIC model statistics\n", my_rank);
	    printf("# [%3d]     Total receives           %12lld\n", my_rank, stat_NoCNICrecv);
	    printf("# [%3d]     Bytes received           %12lld\n", my_rank, stat_NoCNICrecv_bytes);
	    if (stat_NoCNICrecv > 0)   {
		printf("# [%3d]     Average hop count        %12.2f\n", my_rank,
		    (float)stat_NoCmsg_hops / (float)stat_NoCNICrecv);
	    } else   {
		printf("# [%3d]     Average hop count        %12.2f\n", my_rank, 0.0);
	    }
	    if (stat_NoCmsg_congestion_cnt > 0)   {
		printf("# [%3d]     Average congestion delay %12lld %s\n", my_rank,
		    stat_NoCmsg_congestion_delay / stat_NoCmsg_congestion_cnt, TIME_BASE);
	    } else   {
		printf("# [%3d]     Average congestion delay %12d %s\n", my_rank, 0, TIME_BASE);
	    }
	    printf("# [%3d]     Total sends              %12lld\n", my_rank, stat_NoCNICsend);
	    printf("# [%3d]     Bytes sent               %12lld\n", my_rank, stat_NoCNICsend_bytes);
	    if (stat_NoCNICsend > 0)   {
		printf("# [%3d]     NIC send busy            %12.1f %%\n", my_rank,
		    (100.0 / stat_NoCNICsend) * stat_NoCNICbusy);
	    } else   {
		printf("# [%3d]     NIC send busy                     0.0 %%\n", my_rank);
	    }

	    printf("# [%3d] Net NIC model statistics\n", my_rank);
	    printf("# [%3d]     Total receives           %12lld\n", my_rank, stat_NetNICrecv);
	    printf("# [%3d]     Bytes received           %12lld\n", my_rank, stat_NetNICrecv_bytes);
	    if (stat_NetNICrecv > 0)   {
		printf("# [%3d]     Average hop count        %12.2f\n", my_rank,
		    (float)stat_Netmsg_hops / (float)stat_NetNICrecv);
	    } else   {
		printf("# [%3d]     Average hop count        %12.2f\n", my_rank, 0.0);
	    }
	    if (stat_Netmsg_congestion_cnt > 0)   {
		printf("# [%3d]     Average congestion delay %12lld %s\n", my_rank,
		    stat_Netmsg_congestion_delay / stat_Netmsg_congestion_cnt, TIME_BASE);
	    } else   {
		printf("# [%3d]     Average congestion delay %12d %s\n", my_rank, 0, TIME_BASE);
	    }
	    printf("# [%3d]     Total sends              %12lld\n", my_rank, stat_NetNICsend);
	    printf("# [%3d]     Bytes sent               %12lld\n", my_rank, stat_NetNICsend_bytes);
	    if (stat_NetNICsend > 0)   {
		printf("# [%3d]     NIC send busy            %12.1f %%\n", my_rank,
		    (100.0 / stat_NetNICsend) * stat_NetNICbusy);
	    } else   {
		printf("# [%3d]     NIC send busy                     0.0 %%\n", my_rank);
	    }

	    printf("# [%3d] Far link model statistics\n", my_rank);
	    printf("# [%3d]     Total receives                unknown\n", my_rank);
	    printf("# [%3d]     Total sends              %12lld\n", my_rank, stat_FarNICsend);
	    printf("# [%3d]     Bytes sent               %12lld\n", my_rank, stat_FarNICsend_bytes);
	    if (stat_FarNICbusy > 0)   {
		printf("# [%3d]     NIC send busy            %12.1f %%\n", my_rank,
		    (100.0 / stat_FarNICsend) * stat_FarNICbusy);
	    } else   {
		printf("# [%3d]     NIC send busy                     0.0 %%\n", my_rank);
	    }

	    printf("# [%3d] Node destinations and message count\n", my_rank);
	    int my_node= my_rank / (NoC_width * NoC_height * NoC_depth * cores_per_NoC_router);
	    printf("# [%3d]      From node %d to ", my_rank, my_node);
	    for (pos= stat_dest.begin(); pos != stat_dest.end(); pos++)   {
		printf("%d(%" PRId64 "), ", pos->dest, pos->cnt);
	    }
	    printf("\n");
	    printf("#\n");
	}
    }

}  // end of stat_print()



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



// Based on message length and the NIC parameter list, extract the
// startup costs of a message of this length (latency) and the
// duration msg_len bytes will occupy the NIC
void
Patterns::get_NICparams(std::list<NICparams_t> params, int64_t msg_len,
	int64_t *latency, int64_t *msg_duration)
{

std::list<NICparams_t>::iterator k;
std::list<NICparams_t>::iterator previous;
double T, B;
double byte_cost;


    previous= params.begin();
    k= previous;
    k++;
    for (; k != params.end(); k++)   {
	if (k->inflectionpoint > msg_len)   {
	    T= k->latency - previous->latency;
	    B= k->inflectionpoint - previous->inflectionpoint;
	    byte_cost= T / B;
	    // We want the values from the previous point
	    *latency= previous->latency;
	    *msg_duration= (int64_t)((double)(msg_len - previous->inflectionpoint) * byte_cost);
	    return;
	}
	previous++;
    }

    // We're beyond the list. Use the last two values in the list and
    // extrapolite.
    previous--;
    T= params.back().latency - previous->latency;
    B= params.back().inflectionpoint - previous->inflectionpoint;
    byte_cost= T / B;
    *latency= params.back().latency;
    *msg_duration= (int64_t)((double)(msg_len - params.back().inflectionpoint) * byte_cost);

}  // end of getNICparams()

#ifdef SERIALIZARION_WORKS_NOW
BOOST_CLASS_EXPORT(Patterns)
#endif // SERIALIZARION_WORKS_NOW

// -*- mode: c++ -*-

// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: MIT License

// Authors: Kartik Lakhotia, Sai Prabhakar Rao Chenna


#include <sst_config.h>
#include "polarfly.h"

#include "sst/core/rng/xorshift.h"



using namespace SST::Merlin;

topo_polarfly::topo_polarfly(ComponentId_t cid, Params& params, int num_ports, int rtr_id, int num_vns) :
    Topology(cid),
    router_id(rtr_id),
    num_vns(num_vns)
{
    //Get the various parameters
    q                   = params.find<int>("q");
    hosts_per_router    = params.find<int>("hosts_per_router");
    network_radix       = params.find<int>("network_radix");
    total_radix         = params.find<int>("total_radix");
    total_routers       = params.find<int>("total_routers");
    total_endnodes      = params.find<int>("total_endnodes");
    std::string algo    = params.find<std::string>("algorithm");

    if (!algo.compare("MINIMAL") ) {
        routing_algo = MINIMAL;
        num_vcs      = 2;
    }
    else if (!algo.compare("VALIANT")) {
        routing_algo = VALIANT;
        num_vcs      = 4;
    }
    else if (!algo.compare("UGAL")) {
        routing_algo = UGAL;
        num_vcs      = 4;
    }
    else if (!algo.compare("UGAL_PF"))
    {
        routing_algo = UGAL_PF;
        num_vcs      = 4;
    }
    else
        output.fatal(CALL_INFO,-1,"Unknown routing mode specified: %s\n",algo.c_str());

    
    rng = new RNG::XORShiftRNG(router_id+1);

    if (num_ports < total_radix){
        output.fatal(CALL_INFO, -1, "Number of ports should be at least %d for this configuration\n", total_radix);
    }

    /* first generate the polar graph, so that we can get the number of
     nodes and links to set the globals */
    initPolarGraph();

    /* Initialize the routing table*/
    initRouteTable();

    /* Initialize the hopcount_map statistic
     * For now, doing it in a dumb way, should figure out an error-free way to create a vector array of statistics*/

    hopcount1 = registerStatistic<uint32_t>("hopcount1");
    hopcount2 = registerStatistic<uint32_t>("hopcount2");
    hopcount3 = registerStatistic<uint32_t>("hopcount3");
    hopcount4 = registerStatistic<uint32_t>("hopcount4");

    adaptive_bias   = 50;
}

topo_polarfly::~topo_polarfly(){
}

void
topo_polarfly::setOutputBufferCreditArray(int const* array, int vcs)
{
    output_credits      = array;
    output_buffer_size  = array[0];
    assert(vcs==num_vcs);
}

void
topo_polarfly::setOutputQueueLengthsArray(int const* array, int vcs)
{
    output_queue_lengths = array;
    assert(vcs==num_vcs);
}

bool topo_polarfly::isNeighbor(int node)
{
    for (int i=0; i<node_links; i++)
    {
        if (neighbor_list[i]==node)
            return true;
    }
    return false;
}

void topo_polarfly::route_packet(int port, int vc, internal_router_event* ev){

    if (routing_algo == MINIMAL) return routeMinimal(port,vc,ev);
    if (routing_algo == VALIANT) return routeValiant(port,vc,ev);
    if (routing_algo == UGAL) return routeUgal(port,vc,ev);
    if (routing_algo == UGAL_PF) return routeUgalpf(port,vc,ev);
    
}

void topo_polarfly::routeMinimal(int port, int vc, internal_router_event* ev){

    // Get the node ID of the destination router
    int dest_node = getRouterID(ev->getDest());

    int out_channel;

    topo_polarfly_event *tt_ev = static_cast<topo_polarfly_event*>(ev);

    if (dest_node == router_id){
	    //If we reached the destination node, dump the no of switch hops the packet took in total
        dumpHopCount(tt_ev);

        out_channel = getDestLocalPort(ev->getDest());

        tt_ev->setNextPort(out_channel);
        tt_ev->setVC(0);
    }
    else{
        out_channel = route_table[dest_node] + hosts_per_router;

        if (tt_ev->hop_count == 0)
            tt_ev->setVC(0);
        else
            tt_ev->setVC(vc+1);

        tt_ev->setNextPort(out_channel);
    }

    tt_ev->hop_count++;
}



void topo_polarfly::routeValiant(int port, int vc, internal_router_event* ev){
    // Get the node ID of the destination router
    int dest_node = getRouterID(ev->getDest());

    topo_polarfly_event *tt_ev = static_cast<topo_polarfly_event*>(ev);

    int out_channel;

    if (dest_node == router_id)
    {
    	//If we reached the destination node, dump the no of switch hops the packet took in total
	//For now, only tracked node and packets dump this info.	    
	    dumpHopCount(tt_ev);

        out_channel = getDestLocalPort(tt_ev->getDest());

        tt_ev->setNextPort(out_channel);

        tt_ev->setVC(0);
    }
    else
    {
        int source_node = getRouterID(tt_ev->getSrc());

        int minimal_channel, isneighbor;

        //First check if the packet originated here and //If yes, take the valiant path
        if ((source_node == router_id) && (tt_ev->hop_count == 0) )
        {
            minimal_channel = route_table[dest_node];

            int valiant;
            //Randomly select one neighbor from the neighborhood

            do 
            {
                valiant = rng->generateNextUInt32() % total_routers;
            } while(valiant == router_id);

            tt_ev->valiant      = valiant;
            tt_ev->non_minimal  = true;

            out_channel         = route_table[valiant] + hosts_per_router;

            tt_ev->setNextPort(out_channel);
            tt_ev->setVC(0);
            assert(tt_ev->hop_count < 1);
        }
        //if you've reached the intermediate valiant node, proceed to destination along the shortest path
        else if (tt_ev->valiant==router_id || (!tt_ev->non_minimal))
        {
            minimal_channel     = route_table[dest_node];
            tt_ev->non_minimal  = false;
            out_channel         = minimal_channel + hosts_per_router;

            tt_ev->setNextPort(out_channel); 
            tt_ev->setVC(vc + 1);

            assert(tt_ev->hop_count < 4);
        }
        //If the current router is not where the packet started, first check the minimal path
        else 
        {
            minimal_channel     = route_table[tt_ev->valiant];
            out_channel         = minimal_channel + hosts_per_router;

            tt_ev->setNextPort(out_channel);
            tt_ev->setVC(vc + 1);

            assert(tt_ev->hop_count < 3);
        }
    }
    tt_ev->hop_count++;
}


void topo_polarfly::initPolarGraph() {
    //Read polarfly topology from file
    char dir[256];
    getcwd(dir, 256);
    std::string filepath    = std::string(dir) + "/polarfly_data/PolarFly.q_" + std::to_string(this->q) + ".txt";
    
    std::fstream fp;
    fp.open(filepath.c_str(), std::ios::in);

    std::string line;
    int line_no = 0;
    int fV, fE, v;
    while(std::getline(fp, line))
    {   
        std::istringstream iss(line);
        if (line_no == 0)
        {
            iss>>fV; 
            iss>>fE;
        }
        else
        {
            this->polar.push_back(std::vector<int>());
            int u   = line_no - 1; 
            while(iss >> v)
                this->polar[u].push_back(v);
        }
        line_no += 1;
    } 
    fp.close();

    assert(fV==this->total_routers);
}


void topo_polarfly::initRouteTable() {

    int i, j, k, neighbor;
    int links, neighbor_links, NODES;

    NODES           = this->total_routers;

    /* allocate the routing tables */
    route_table.resize(NODES); // route_table[j] contains the port link from current router to router/node j (could be 1 hop or 2 hop)
    node_links      = polar[router_id].size();
  
    for( i = 0; i < NODES; i++ )
        route_table[i]          = -1;
    neighbor_list.resize(node_links);
    neighbor_list.assign(polar[router_id].begin(), polar[router_id].end());

    /* initialize the routing table */
    for( i = 0; i < node_links; i++)
    {
        // 1-hop neighbor 
        neighbor                = polar[router_id][i];
        route_table[neighbor]   = i;
        neighbor_links          = polar[neighbor].size();
    }
    for( i = 0; i < node_links; i++)
    {
        neighbor                = polar[router_id][i];
        neighbor_links          = polar[neighbor].size();
        for( j = 0; j < neighbor_links; j++)
        {
            // 2-hop neighbors, not already covered
            int neigh2          = polar[neighbor][j];

            if (route_table[neigh2]<0 && neigh2!=router_id)
                route_table[neigh2] = i;
        }
    }

    /* make sure the table is properly built */
    for( i = 0; i < NODES; i++ )
    {
	    if ( i != router_id ) 
        {
	        assert(route_table[i] >= 0);
            assert(route_table[i] < node_links);
        }
	    else
	        assert( route_table[i] == -1 );
    }       

    //Free memory associated with graph topology
    for( i = 0; i < NODES; i++)
    {
        std::vector<int> tmp;
        polar[i].swap(tmp);
    } 
    std::vector<std::vector<int>> tmp;
    polar.swap(tmp);
}



//For now, while building the polarfly topology, we assume all local ports of the switch are connected to the endpoints
Topology::PortState topo_polarfly::getPortState(int port) const
{

    if (port < hosts_per_router) return R2N;
    else if (port >= hosts_per_router && port <= hosts_per_router + network_radix) return R2R;
    else return UNCONNECTED; 

}


int topo_polarfly::getEndpointID(int port)
{
    if ( port < hosts_per_router ) return (((router_id)*hosts_per_router) + port);
    else return -1 ;
}


 //For now, there is no need to wrap any additional details to the incoming router event. So just doing the basic
internal_router_event* topo_polarfly::process_input(RtrEvent* ev)
{
    topo_polarfly_event* tt_ev = new topo_polarfly_event(0);
    tt_ev->setEncapsulatedEvent(ev);
    tt_ev->setVC(tt_ev->getVN());

    return tt_ev;
}

internal_router_event* topo_polarfly::process_InitData_input(RtrEvent* ev)
{
    topo_polarfly_init_event *tt_ev = new topo_polarfly_init_event(0, this->total_routers);
    tt_ev->setEncapsulatedEvent(ev);

    return tt_ev;
}



void topo_polarfly::routeInitData(int port, internal_router_event* ev, std::vector<int> &outPorts)
{
    topo_polarfly_init_event *tt_ev = static_cast<topo_polarfly_init_event*>(ev);

    if (ev->getDest() == INIT_BROADCAST_ADDR){
        //phase=0 -> packet injected into network from this router
        if (tt_ev->phase == 0) {
            //This is the entry router. First send to local ports.
            tt_ev->covered[router_id]   = 1;

            for (int i=0; i<hosts_per_router; i++)
            {
                if (port != i) outPorts.push_back(i);
            }

            // Also send to the adjacent neighbors
            for (int j=0; j < node_links; j++)
            {
                outPorts.push_back(j + hosts_per_router);
                tt_ev->covered[neighbor_list[j]]   = 1;
            }

            //Increment the phase value
            tt_ev->phase    = 1;
        }
        else if (tt_ev->phase < 2)
        {
            // ensure that broadcast reaches all the endpoints only once
            for (int j=0; j<node_links; j++)
            {
                int neighbor    = neighbor_list[j];
                if (tt_ev->covered[neighbor]==0)
                {
                    outPorts.push_back(j + hosts_per_router);
                    tt_ev->covered[neighbor]    = 1;
                }
            }
            tt_ev->phase    += 1;

            //Don't forget to send it to the local endpoints
            for (int i=0; i < hosts_per_router;i++) {
                if( port != i) outPorts.push_back(i) ;
            }
        }
        else if (tt_ev->phase == 2) {
            //This means that the initial BROADCAST packet has reached the final stage of routers. No need to forward further as it is a 2 hop network.
            //Just send it to the local endpoints
            for (int i=0; i < hosts_per_router;i++) {
                if( port != i) outPorts.push_back(i) ;
            }
        }
        else {
            //You shouldn't reach here
            tt_ev->phase =0;
        }
    }
    else
    {
        route_packet(port, 0, ev);
        outPorts.push_back(ev->getNextPort());
    }
}



int topo_polarfly::getRouterID(int endpoint) {

    return (int) endpoint / hosts_per_router;

}


int topo_polarfly::getDestLocalPort(int node){

    return (int) node % hosts_per_router ; 

}



void topo_polarfly::routeUgal(int port, int vc, internal_router_event* ev){

    topo_polarfly_event *tt_ev = static_cast<topo_polarfly_event*>(ev);

    // Get the node ID of the destination router
    int dest_node = getRouterID(ev->getDest());

    int out_channel;
    int out_vc    = (tt_ev->hop_count==0) ? 0 : vc + 1;


    //destination on same router
    if (dest_node == router_id)
    {
        dumpHopCount(tt_ev);
        out_channel = getDestLocalPort(ev->getDest());

        tt_ev->setNextPort(out_channel);
        tt_ev->setVC(0);
    
        return;
    }
    //injection
    else if (port < hosts_per_router)
    {
        //minpath details
        int min_channel = route_table[dest_node] + hosts_per_router;
        int min_queue   = output_queue_lengths[min_channel*num_vcs + out_vc];

        //find valiant intermediate node
        int valiant, val_channel;
        int val_queue   = std::numeric_limits<int>::max();
        int num_tries   = 4;
        for (int i=0; i<num_tries; i++)
        {
            int candidate;
            do
            {
                candidate   = rng->generateNextUInt32() % total_routers;
            } while(candidate == router_id);
            int candidate_channel   = route_table[candidate] + hosts_per_router;
            int candidate_queue     = output_queue_lengths[candidate_channel*num_vcs + out_vc];
            if (val_queue > candidate_queue)
            {
                val_queue   = candidate_queue;
                val_channel = candidate_channel;
                valiant     = candidate;
            }
        }
        
        //select between valiant and minpath 
        if ((min_queue < 2*val_queue + adaptive_bias))
        {
            out_channel         = min_channel;
            tt_ev->non_minimal  = false;
        }
        else
        {
            tt_ev->non_minimal  = true;
            tt_ev->valiant      = valiant;
            out_channel         = val_channel;
        }
        assert(tt_ev->hop_count < 4);
    }
    else if ((tt_ev->valiant == router_id && tt_ev->non_minimal) || (!tt_ev->non_minimal))
    {
        out_channel         = route_table[dest_node] + hosts_per_router;
        out_vc              = vc + 1;
        tt_ev->non_minimal  = false;
        assert(tt_ev->hop_count < 4);
    }
    else
    {
        out_channel         = route_table[tt_ev->valiant] + hosts_per_router;
        out_vc              = vc + 1;
        assert(tt_ev->hop_count < 3);
    }

    tt_ev->hop_count++;
    tt_ev->setNextPort(out_channel);
    tt_ev->setVC(out_vc);
}

//adaptive routing that reduces hop count of valiant paths
void topo_polarfly::routeUgalpf(int port, int vc, internal_router_event* ev){

    topo_polarfly_event *tt_ev = static_cast<topo_polarfly_event*>(ev);

    // Get the node ID of the destination router
    int dest_node = getRouterID(ev->getDest());

    int out_channel;
    int out_vc    = (tt_ev->hop_count==0) ? 0 : vc + 1;


    //destination on same router
    if (dest_node == router_id)
    {
        dumpHopCount(tt_ev);
        out_channel = getDestLocalPort(ev->getDest());

        tt_ev->setNextPort(out_channel);
        tt_ev->setVC(0);
    
        return;
    }
    //injection
    else if (port < hosts_per_router)
    {
        bool adj_dst    = isNeighbor(dest_node);

        //minpath details
        int min_channel = route_table[dest_node] + hosts_per_router;
        int min_queue   = output_queue_lengths[min_channel*num_vcs + out_vc];

        //find valiant intermediate node
        int valiant, val_channel;
        int val_queue   = std::numeric_limits<int>::max();
        int num_tries   = 4;
        for (int i=0; i<num_tries; i++)
        {
            int candidate;
            do
            {
                //choose a random valiant (most likely 2-hops away)
                if (adj_dst)
                    candidate   = rng->generateNextUInt32() % total_routers;
                //choose a valiant from your neighborhood
                else
                    candidate   = neighbor_list[rng->generateNextUInt32() % node_links];
            } while(candidate == router_id);
            int candidate_channel   = route_table[candidate] + hosts_per_router;
            int candidate_queue     = output_queue_lengths[candidate_channel*num_vcs + out_vc];
            if (val_queue > candidate_queue)
            {
                val_queue   = candidate_queue;
                val_channel = candidate_channel;
                valiant     = candidate;
            }
        }
        
        //select between valiant and minpath 
        if ((min_queue < (3*val_queue)/2 + adaptive_bias))
        {
            out_channel         = min_channel;
            tt_ev->non_minimal  = false;
        }
        else
        {
            tt_ev->non_minimal  = true;
            tt_ev->valiant      = valiant;
            out_channel         = val_channel;
        }
        assert(tt_ev->hop_count < 4);
    }
    else if ((tt_ev->valiant == router_id && tt_ev->non_minimal) || (!tt_ev->non_minimal))
    {
        out_channel         = route_table[dest_node] + hosts_per_router;
        out_vc              = vc + 1;
        tt_ev->non_minimal  = false;
        assert(tt_ev->hop_count < 4);
    }
    else
    {
        out_channel         = route_table[tt_ev->valiant] + hosts_per_router;
        out_vc              = vc + 1;
        assert(tt_ev->hop_count < 3);
    }

    tt_ev->hop_count++;
    tt_ev->setNextPort(out_channel);
    tt_ev->setVC(out_vc);
}


//Debug by Sai Chenna. As we reached the final router/node. Dump the no of switch hops this packet has taken to reach the destination. 
void topo_polarfly::dumpHopCount(topo_polarfly_event* tt_ev){

	int count = tt_ev->hop_count;
	switch (count) {
		case 1:
			hopcount1->addData(1);
			break;

		case 2:
			hopcount2->addData(1);
			break;

		case 3:
			hopcount3->addData(1);
			break;

		case 4:
			hopcount4->addData(1);
			break;
	}

}

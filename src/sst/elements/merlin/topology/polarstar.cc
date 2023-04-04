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

// Author: Kartik Lakhotia


#include <sst_config.h>
#include "polarstar.h"

#include "sst/core/rng/xorshift.h"


using namespace SST::Merlin;

topo_polarstar::topo_polarstar(ComponentId_t cid, Params& params, int num_ports, int rtr_id, int num_vns) :
    Topology(cid),
    router_id(rtr_id),
    num_vns(num_vns)
{

    //Get the various parameters
    d                   = params.find<int>("d");
    sn_type             = params.find<std::string>("sn_type");
    pfq                 = params.find<int>("pfq");
    snq                 = params.find<int>("snq");

    hosts_per_router    = params.find<int>("hosts_per_router");

    network_radix       = params.find<int>("network_radix");

    total_radix         = params.find<int>("total_radix");

    assert(total_radix == num_ports);

    total_routers       = params.find<int>("total_routers");

    total_endnodes      = params.find<int>("total_endnodes");

    std::string algo    = params.find<std::string>("algorithm");

    if (!algo.compare("MINIMAL") ) {
        routing_algo = MINIMAL;
        num_vcs      = 3;
    }
    else if (!algo.compare("VALIANT")) {
        routing_algo = VALIANT;
        num_vcs      = 6;
    }
    else if (!algo.compare("UGAL")) {
        routing_algo = UGAL;
        num_vcs      = 6;
    }
    else {
        output.fatal(CALL_INFO,-1,"Unknown routing mode specified: %s\n",algo.c_str());
    }

    rng = new RNG::XORShiftRNG(router_id+1);

    if (num_ports < total_radix){
        output.fatal(CALL_INFO, -1, "Number of ports should be at least %d for this configuration\n", total_radix);
    }

    /* first generate the polar graph, so that we can get the number of
     nodes and links to set the globals */
    initPolarGraph();
    
    assert(total_routers == polar.size());

    /* Initialize the routing table*/
    initRouteTable();

    /* Initialize the hopcount_map statistic
     * For now, doing it in a dumb way, should figure out an error-free way to create a vector array of statistics*/

    hopcount1 = registerStatistic<uint32_t>("hopcount1");
    hopcount2 = registerStatistic<uint32_t>("hopcount2");
    hopcount3 = registerStatistic<uint32_t>("hopcount3");
    hopcount4 = registerStatistic<uint32_t>("hopcount4");
    hopcount5 = registerStatistic<uint32_t>("hopcount5");
    hopcount6 = registerStatistic<uint32_t>("hopcount6");


    adaptive_bias   = 33;

}

topo_polarstar::~topo_polarstar(){
}

void topo_polarstar::route_packet(int port, int vc, internal_router_event* ev){

    if (routing_algo == MINIMAL) return routeMinimal(port,vc,ev);
    if (routing_algo == VALIANT) return routeValiant(port,vc,ev);
    if (routing_algo == UGAL) return routeUgal(port,vc,ev);
}

void topo_polarstar::routeMinimal(int port, int vc, internal_router_event* ev){

    //std::cout << "Using minimal routing algorithm!" << std::endl;
    // Get the node ID of the destination router
    int dest_node = getRouterID(ev->getDest());

    int out_channel;

    topo_polarstar_event *tt_ev = static_cast<topo_polarstar_event*>(ev);

    if (dest_node == router_id){
	//If we reached the destination node, dump the no of switch hops the packet took in total
        dumpHopCount(tt_ev);

        out_channel = getDestLocalPort(ev->getDest());

        tt_ev->setNextPort(out_channel);
        tt_ev->setVC(0);
    }
    else{
        out_channel = route_table[dest_node] + hosts_per_router;

        tt_ev->setNextPort(out_channel);

        if (tt_ev->hop_count == 0)
            tt_ev->setVC(0);
        else
            tt_ev->setVC(vc+1);
    }
    tt_ev->hop_count++;
}


void topo_polarstar::routeValiant(int port, int vc, internal_router_event* ev){
    // Get the node ID of the destination router
    int dest_node               = getRouterID(ev->getDest());

    topo_polarstar_event *tt_ev = static_cast<topo_polarstar_event*>(ev);

    int out_channel;


    if (dest_node == router_id){
    	//If we reached the destination node, dump the no of switch hops the packet took in total
	//For now, only tracked node and packets dump this info.	    
	    dumpHopCount(tt_ev);

        out_channel     = getDestLocalPort(tt_ev->getDest());

        tt_ev->setNextPort(out_channel);
        tt_ev->setVC(0);
    }
    else{
        int source_node = getRouterID(tt_ev->getSrc());

        int minimal_channel, isneighbor;

        //First check if the packet originated here and //If yes, take the valiant path
        if ((source_node == router_id) && (tt_ev->hop_count == 0) ){

            int valiant;

            do 
            {
                valiant = rng->generateNextUInt32() % total_routers;
            } while(valiant == router_id);

            tt_ev->valiant      = valiant;
            tt_ev->non_minimal  = true;

            out_channel = route_table[valiant] + hosts_per_router;

            tt_ev->setNextPort(out_channel);
            assert(tt_ev->hop_count < 1);
            tt_ev->setVC(0);
        }
        //if you've reached the intermediate valiant node, proceed to destination along the shortest path
        else if (tt_ev->valiant==router_id || (!tt_ev->non_minimal))
        {
            minimal_channel     = route_table[dest_node];
            tt_ev->non_minimal  = false;
            out_channel         = minimal_channel + hosts_per_router;
            tt_ev->setNextPort(out_channel); 

            assert(tt_ev->hop_count < 6);
            tt_ev->setVC(vc + 1);
        }
        //If the current router is not where the packet started, first check the minimal path
        else {

            minimal_channel     = route_table[tt_ev->valiant];
            out_channel         = minimal_channel + hosts_per_router;

            tt_ev->setNextPort(out_channel);

            assert(tt_ev->hop_count < 3);
            tt_ev->setVC(vc + 1);
         }
    }
    tt_ev->hop_count++;
}


void topo_polarstar::routeUgal(int port, int vc, internal_router_event* ev){

    topo_polarstar_event *tt_ev = static_cast<topo_polarstar_event*>(ev);

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
            int candidate, candidate_channel;
            do
            {
                candidate   = rng->generateNextUInt32() % total_routers;
                candidate_channel   = route_table[candidate] + hosts_per_router;
            } while((candidate == router_id) || (candidate_channel == min_channel));
            int candidate_queue     = output_queue_lengths[candidate_channel*num_vcs + out_vc];
            if (val_queue > candidate_queue)
            {
                val_queue   = candidate_queue;
                val_channel = candidate_channel;
                valiant     = candidate;
            }
        }
        
        //select between valiant and minpath 
        //printf("router = %d, dest = %d, min queue = %d, val_queue = %d\n", router_id, dest_node, min_queue, val_queue); 
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
        assert(tt_ev->hop_count < 6);
    }
    else if ((tt_ev->valiant == router_id && tt_ev->non_minimal) || (!tt_ev->non_minimal))
    {
        out_channel         = route_table[dest_node] + hosts_per_router;
        tt_ev->non_minimal  = false;
        assert(tt_ev->hop_count < 6);
    }
    else
    {
        out_channel         = route_table[tt_ev->valiant] + hosts_per_router;
        assert(tt_ev->hop_count < 3);
    }

    tt_ev->hop_count++;
    tt_ev->setNextPort(out_channel);
    tt_ev->setVC(out_vc);
}





void topo_polarstar::initPolarGraph() {
    char dir[256];
    getcwd(dir, 256); 
    std::string filepath    = std::string(dir) + "/polarstar_data/PolarStar.d_" + 
                            std::to_string(this->d) + "_pfq_" + std::to_string(this->pfq) + 
                            + "_sn_" + this->sn_type +
                            "_snq_" + std::to_string(this->snq) + ".txt";

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
}


void topo_polarstar::initRouteTable() {

    int i, j, k, neighbor, links, NODES;

    NODES = this->total_routers;

    /* allocate the routing tables */
    route_table.resize(NODES); // route_table[j] contains the port link from current router to router/node j (could be 1 hop or 2 hop)
    node_links      = polar[router_id].size();
  
    for( i = 0; i < NODES; i++ )
        route_table[i]  = -1;
    neighbor_list.resize(node_links);
    neighbor_list.assign(polar[router_id].begin(), polar[router_id].end());

    std::vector<int> frontier;
    std::vector<int> nxt;

    /* initialize the routing table */
    for (j = 0; j < node_links; j++)
    {
        neighbor                = polar[router_id][j];   
        route_table[neighbor]   = j;
        frontier.push_back(neighbor);
    }
    while(frontier.size()>0)
    {
        int fs      = frontier.size();
        for (i=0; i<fs; i++)
        {
            int v       = frontier[i];
            int pId     = route_table[v]; assert(pId >= 0);
            links       = polar[v].size();
           
            for (j=0; j<links; j++)
            {
                neighbor    = polar[v][j];
                if ((route_table[neighbor] < 0) && (neighbor != router_id))
                {
                    route_table[neighbor]   = pId;
                    nxt.push_back(neighbor);
                }
            }
        }
        frontier.swap(nxt);
        nxt.clear();
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

//For now, while building the polarstar topology, we assume all local ports of the switch are connected to the endpoints
Topology::PortState topo_polarstar::getPortState(int port) const
{
    if (port < hosts_per_router) return R2N;
    else if (port >= hosts_per_router && port <= hosts_per_router + node_links) return R2R;
    else return UNCONNECTED; 
}


int topo_polarstar::getEndpointID(int port)
{
    if ( port < hosts_per_router ) return (((router_id)*hosts_per_router) + port);
    else return -1 ;
}


 //For now, there is no need to wrap any additional details to the incoming router event. So just doing the basic
internal_router_event* topo_polarstar::process_input(RtrEvent* ev)
{
    topo_polarstar_event* tt_ev = new topo_polarstar_event(0);
    tt_ev->setEncapsulatedEvent(ev);
    tt_ev->setVC(tt_ev->getVN());

    return tt_ev;
}

internal_router_event* topo_polarstar::process_InitData_input(RtrEvent* ev)
{
    topo_polarstar_init_event *tt_ev = new topo_polarstar_init_event(0, this->total_routers);
    tt_ev->setEncapsulatedEvent(ev);

    return tt_ev;
}

void topo_polarstar::routeInitData(int port, internal_router_event* ev, std::vector<int> &outPorts)
{
    topo_polarstar_init_event *tt_ev = static_cast<topo_polarstar_init_event*>(ev);

    if (ev->getDest() == INIT_BROADCAST_ADDR ){
        //phase=0 -> packet injected into network from this router
        if (tt_ev->phase == 0) {
            //This is the entry router. First send to local ports.
            tt_ev->covered[router_id] = 1;

            for (int i=0; i < hosts_per_router;i++) {

                if( port != i) outPorts.push_back(i) ;
            }

            // Also send to the adjacent neighbors
            for (int j=0; j < node_links; j++) {

                outPorts.push_back(j+hosts_per_router);
                tt_ev->covered[neighbor_list[j]]    = 1; 
            }

            //Increment the phase value
            tt_ev->phase = 1;
        }

        else if (tt_ev->phase < 3) {
            //Send to all the adjacent routers that have not received
            for (int j=0; j < node_links; j++) {
                int neighbor    = neighbor_list[j];
                if (tt_ev->covered[neighbor]==0)
                {
                    outPorts.push_back(j+hosts_per_router);
                    tt_ev->covered[neighbor]    = 1;
                }
            }
            tt_ev->phase    += 1;

            //Don't forget to send it to the local endpoints
            for (int i=0; i < hosts_per_router;i++) {
                if( port != i) outPorts.push_back(i) ;
            }
        }
        else if (tt_ev->phase == 3) {
            //This means that the initial BROADCAST packet has reached the final stage of routers. No need to forward further as it is a 3 hop network.
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
    else {

        route_packet(port,0,ev);
        outPorts.push_back(ev->getNextPort());
    }
}


void
topo_polarstar::setOutputBufferCreditArray(int const* array, int vcs)
{
    output_credits      = array;
    output_buffer_size  = array[0];
    assert(vcs==num_vcs);
}

void
topo_polarstar::setOutputQueueLengthsArray(int const* array, int vcs)
{
    output_queue_lengths = array;
    assert(vcs==num_vcs);
}


int topo_polarstar::getRouterID(int endpoint) {

    return (int) endpoint / hosts_per_router;

}


int topo_polarstar::getDestLocalPort(int node){

    return (int) node % hosts_per_router ; 

}


//Debug by Sai Chenna. As we reached the final router/node. Dump the no of switch hops this packet has taken to reach the destination.
void topo_polarstar::dumpHopCount(topo_polarstar_event* tt_ev){


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

        case 5:
            hopcount5->addData(1);
            break;

        case 6:
            hopcount6->addData(1);
	}
}

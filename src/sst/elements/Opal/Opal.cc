// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//


//
/* Author: Amro Awad
 * E-mail: aawad@sandia.gov
 */
/* Author: Vamsee Reddy Kommareddy
 * E-mail: vamseereddy@knights.ucf.edu
 */


#include <sst_config.h>
#include <string>
#include<iostream>
#include "Opal.h"


using namespace SST::Interfaces;
using namespace SST;
using namespace SST::OpalComponent;


#define OPAL_VERBOSE(LEVEL, OUTPUT) if(verbosity >= (LEVEL)) OUTPUT


Opal::Opal(SST::ComponentId_t id, SST::Params& params): Component(id) {


 registerAsPrimaryComponent();
//    primaryComponentDoNotEndSim();

    int verbosity = (uint32_t) params.find<uint32_t>("verbose", 1);
	output = new SST::Output("OpalComponent[@f:@l:@p] ", verbosity, 0, SST::Output::STDOUT);

	max_inst = (uint32_t) params.find<uint32_t>("max_inst", 1);
	cores_per_node = (uint32_t) params.find<uint32_t>("cores_per_node", 1);
	num_nodes = (uint32_t) params.find<uint32_t>("num_nodes", 1);
	latency = (uint32_t) params.find<uint32_t>("latency", 1);

	Pool *pool;
	char* buffer = (char*) malloc(sizeof(char) * 256);

	/* shared memory */
	/* shared memory */
	num_shared_mempools = params.find<uint32_t>("shared_mempools", 0);
	std::cerr << "Number of Shared Memory Pools: "<< num_shared_mempools << endl;

	Params sharedMemParams = params.find_prefix_params("shared_mem.");

	for(int i = 0; i < num_shared_mempools; i++) {
		sprintf(buffer, "mempool%" PRIu32 ".", i);
		Params memPoolParams = sharedMemParams.find_prefix_params(buffer);
		pool = new Pool(memPoolParams);
		pool->set_memPool_type(SST::OpalComponent::MemType::SHARED);
		shared_mem[i] = pool;
		std::cerr << "Shared Memory pool: " << buffer;
	}

	/* local memory for each node */
	Params localMemParams = params.find_prefix_params("local_mem.");

	memset(buffer, 0 , 256);
	for(int i=0; i<num_nodes; i++) {
		sprintf(buffer, "mempool%" PRIu32 ".", i);
		Params memPoolParams = localMemParams.find_prefix_params(buffer);
		pool = new Pool(memPoolParams);
		pool->set_memPool_type(SST::OpalComponent::MemType::LOCAL);
		local_mem[i] = pool;
		allochist[i] = 0; // 0 for local memory
		std::cerr << "Local Memory Pool: " << buffer;
	}

	Handlers = new core_handler[num_nodes*cores_per_node*2];
	samba_to_opal = new SST::Link * [num_nodes*cores_per_node*2]; // Note the links can also come directly form Ariel to send hints, rather than only from Samba's Page Table Walker

	/* creating links from samba and ariel core's of each node */
	memset(buffer, 0 , 256);
	for(int i = 0; i < num_nodes*cores_per_node*2; i++) {
		sprintf(buffer, "requestLink%" PRIu32, i);
		SST::Link * link = configureLink(buffer, "1ns", new Event::Handler<core_handler>((&Handlers[i]), &core_handler::handleRequest));
		samba_to_opal[i] = link;
		Handlers[i].singLink = link;
		Handlers[i].id = i;
		Handlers[i].latency = latency;
		Handlers[i].setOwner(this);
	}

	std::string cpu_clock = params.find<std::string>("clock", "1GHz");
	registerClock( cpu_clock, new Clock::Handler<Opal>(this, &Opal::tick ) );
}



Opal::Opal() : Component(-1)
{
	// for serialization only
	//
}



bool Opal::tick(SST::Cycle_t x)
{

	int inst_served = 0;

	while(!requestQ.empty()) {
		if(inst_served < max_inst) {
			MemPoolResponse pool_resp;

			OpalEvent *ev = requestQ.front();
			int node_num = ev->getNodeId();

			switch(ev->getType()) {
			case SST::OpalComponent::EventType::MMAP:
			{
				std::cout<<"Opal has received an MMAP CALL "<<std::endl;
			}
			break;

			case SST::OpalComponent::EventType::UNMAP:
			{
				std::cout<<"Opal has received an UNMAP CALL "<<std::endl;
			}
			break;

			case SST::OpalComponent::EventType::REQUEST:
			{

				Pool*  mempool;

				if( !allochist[node_num] )
				{
					mempool = local_mem[node_num];
					pool_resp = mempool->allocate_frames(ev->getSize());

					//if the local memory allocates the required memory then its not required to allocate from the shared memory. If not the memory is allocated form the shared memory.
					if(pool_resp.pAddress != -1) {
						//output->verbose(CALL_INFO, 2, 0, "%s, Node %" PRIu32 ": Allocate physical memory %" PRIu64 " for virtual address %" PRIu64 " in the local memory with %" PRIu32 " frames\n",
						//		getName().c_str(), node_num, pool_resp.pAddress, ev->getAddress, pool_resp.num_frames);
						OpalEvent *tse = new OpalEvent(EventType::RESPONSE);
						tse->setResp(ev->getAddress(), pool_resp.pAddress, pool_resp.num_frames*pool_resp.frame_size);
						Handlers[node_num].singLink->send(latency, tse);
					}
					else{
						//output->verbose(CALL_INFO, 2, 0, "%s, Node %" PRIu32 ": Could not allocate memory for virtual address %" PRIu64 " from Local memory \n",
						//		getName().c_str(), node_num, ev->getAddress,);
						OpalEvent *tse = new OpalEvent(EventType::RESPONSE);
						tse->setResp(ev->getAddress(), -1, 0);
						Handlers[node_num].singLink->send(latency, tse);
					}
				}
				else
				{
					pool_resp.pAddress = -1;
					for (std::map<int, Pool*>::iterator it = shared_mem.begin(); it != shared_mem.end(); ++it) {
						mempool = it->second;
						pool_resp = mempool->allocate_frames(ev->getSize());

						if(pool_resp.pAddress != -1) {
							//output->verbose(CALL_INFO, 2, 0, "%s, Node %" PRIu32 ": Allocate physical memory %" PRIu64 " for virtual address %" PRIu64 " in the shared memory pool %" PRIu32 "with %" PRIu32 " frames\n",
							//		getName().c_str(), node_num, pool_resp.pAddress, ev->getAddress, *it, pool_resp.num_frames);
							OpalEvent *tse = new OpalEvent(EventType::RESPONSE);
							tse->setResp(ev->getAddress(), pool_resp.pAddress, pool_resp.num_frames*pool_resp.frame_size);
							Handlers[node_num].singLink->send(latency, tse);
							break;
						}
					}

					if(pool_resp.pAddress == -1) {
						//output->verbose(CALL_INFO, 2, 0, "%s, Node %" PRIu32 ": Could not allocate memory for virtual address %" PRIu64 " from any shared memory pool \n",
						//		getName().c_str(), node_num, ev->getAddress,);
						OpalEvent *tse = new OpalEvent(EventType::RESPONSE);
						tse->setResp(ev->getAddress(), -1, 0);
						Handlers[node_num].singLink->send(latency, tse);
					}

				}
				allochist[node_num] = (allochist[node_num] + 1) % ( num_shared_mempools + 1 );

			}
			break;

			default:
				//output->fatal(CALL_INFO, -1, "%s, Error - Unknown request\n", getName().c_str());
				break;

			}

			requestQ.pop();
			delete ev;
			inst_served++;

		}
		else {
			//output->verbose(CALL_INFO, 2, 0, "%s, number of requests served has reached maximum width in the given time slot \n", getName().c_str());
			break;
		}
	}

	return false;
}


void Opal::handleRequest( SST::Event* e )
{







}



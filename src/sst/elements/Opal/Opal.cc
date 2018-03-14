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

    verbosity = (uint32_t) params.find<uint32_t>("verbose", 1);
	output = new SST::Output("OpalComponent[@f:@l:@p] ", verbosity, 0, SST::Output::STDOUT);

	max_inst = (uint32_t) params.find<uint32_t>("max_inst", 1);
	cores_per_node = (uint32_t) params.find<uint32_t>("cores_per_node", 1);
	num_nodes = (uint32_t) params.find<uint32_t>("num_nodes", 1);
	latency = (uint32_t) params.find<uint32_t>("latency", 1);
	allocpolicy = (uint32_t) params.find<uint32_t>("allocation_policy", 0);
	std::cerr << "Allocation policy: " << allocpolicy << std::endl;
	std::cerr << "Maximum Instructions per cycle: " << max_inst << std::endl;

	Pool *pool;
	char* buffer = (char*) malloc(sizeof(char) * 256);

	/* shared memory */
	num_shared_mempools = params.find<uint32_t>("shared_mempools", 0);
	std::cerr << "Number of Shared Memory Pools: "<< num_shared_mempools << endl;

	Params sharedMemParams = params.find_prefix_params("shared_mem.");
	shared_mem_size = 0;

	uint32_t i;
	for(i = 0; i < num_shared_mempools; i++) {
		memset(buffer, 0 , 256);
		sprintf(buffer, "mempool%" PRIu32 ".", i);
		Params memPoolParams = sharedMemParams.find_prefix_params(buffer);
		std::cerr << "Configuring Shared " << buffer << std::endl;
		pool = new Pool(this, memPoolParams, SST::OpalComponent::MemType::SHARED, i);
		shared_mem[i] = pool;
		shared_mem_size += memPoolParams.find<long long int>("size", 0);
	}

	/* local memory for each node */
	Params localMemParams = params.find_prefix_params("local_mem.");

	memset(buffer, 0 , 256);
	for(i = 0; i < num_nodes; i++) {
		memset(buffer, 0 , 256);
		sprintf(buffer, "mempool%" PRIu32 ".", i);
		Params memPoolParams = localMemParams.find_prefix_params(buffer);
		std::cerr << "Configuring Local " << buffer << std::endl;
		pool = new Pool(this, memPoolParams, SST::OpalComponent::MemType::LOCAL, i);
		local_mem[i] = pool;
		nextallocmem[i] = 0; // 0 for local memory
		allocatedmempool[i] = 0;
		local_mem_size[i] = memPoolParams.find<long long int>("size", 0);
	}

	Handlers = new core_handler[num_nodes*cores_per_node*2];
	samba_to_opal = new SST::Link * [num_nodes*cores_per_node*2]; // Note the links can also come directly form Ariel to send hints, rather than only from Samba's Page Table Walker

	/* creating links from samba and ariel core's of each node */
	memset(buffer, 0 , 256);
	for(i = 0; i < num_nodes*cores_per_node*2; i++) {
		sprintf(buffer, "requestLink%" PRIu32, i);
		SST::Link * link = configureLink(buffer, "1ns", new Event::Handler<core_handler>((&Handlers[i]), &core_handler::handleRequest));
		samba_to_opal[i] = link;
		Handlers[i].singLink = link;
		Handlers[i].id = i;
		Handlers[i].nodeID = i/(cores_per_node*2);
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

//shared or local
void Opal::setNextMemPool( int node )
{
	switch(allocpolicy)
	{
	case 3:
		//proportional allocation policy
		nextallocmem[node] = ( nextallocmem[node] + 1 ) % ( shared_mem_size/local_mem_size[node] + 1 );
		allocatedmempool[node] = nextallocmem[node] ? ( allocatedmempool[node] + 1 ) % ( num_shared_mempools + 1)
					? ( allocatedmempool[node] + 1 ) % ( num_shared_mempools + 1) : 1 : 0;

		break;

	case 2:
		//round robin allocation policy
		nextallocmem[node] = ( nextallocmem[node] + 1 ) % ( num_shared_mempools + 1 );
		allocatedmempool[node] = nextallocmem[node];
		break;

	case 1:
		//alternate allocation policy
		if( allocatedmempool[node] != 0) {
			allocatedmempool[node] = 0;
		} else {
			nextallocmem[node] = ( nextallocmem[node] + 1 ) % ( num_shared_mempools + 1 )
								? ( nextallocmem[node] + 1 ) % ( num_shared_mempools + 1 ) : 1;
			allocatedmempool[node] = nextallocmem[node];
		}
		break;

	case 0:
	default:
		//local memory first
		allocatedmempool[node] = 0;
		break;

	}

}

long long int Opal::allocLocalMemPool(int node, int linkId, long long int vAddress, int size )
{
	Pool *mempool = local_mem[node];
	MemPoolResponse pool_resp = mempool->allocate_frames(size);

	if(pool_resp.pAddress != -1) {
		//output->verbose(CALL_INFO, 2, 0, "%s, Node %" PRIu32 ": Allocate physical memory %" PRIu64 " for virtual address %" PRIu64 " in the local memory with %" PRIu32 " frames\n",
		//		getName().c_str(), node, pool_resp.pAddress, ev->getAddress, pool_resp.num_frames);
		OpalEvent *tse = new OpalEvent(EventType::RESPONSE);
		tse->setResp(vAddress, pool_resp.pAddress, pool_resp.num_frames*pool_resp.frame_size*1024,node);
		Handlers[linkId].singLink->send(latency, tse);

	}

	return pool_resp.pAddress;
}

long long int Opal::allocSharedMemPool(int node, int linkId, long long int vAddress, int size )
{
	Pool *mempool = shared_mem[allocatedmempool[node] -1];
	MemPoolResponse pool_resp = mempool->allocate_frames(size);

	if(pool_resp.pAddress != -1) {
		//output->verbose(CALL_INFO, 2, 0, "%s, Node %" PRIu32 ": Allocate physical memory %" PRIu64 " for virtual address %" PRIu64 " in the shared memory pool %" PRIu32 "with %" PRIu32 " frames\n",
		//		getName().c_str(), node, pool_resp.pAddress, vAddress, *it, pool_resp.num_frames);
		OpalEvent *tse = new OpalEvent(EventType::RESPONSE);
		tse->setResp(vAddress, pool_resp.pAddress, pool_resp.num_frames*pool_resp.frame_size*1024,node);
		Handlers[linkId].singLink->send(latency, tse);

	}

	return pool_resp.pAddress;
}

bool Opal::tick(SST::Cycle_t x)
{

	int inst_served = 0;

	while(!requestQ.empty()) {
		if(inst_served < max_inst) {

			OpalEvent *ev = requestQ.front();
			int node_num = ev->getNodeId();
			int linkId = ev->getLinkId();

			switch(ev->getType()) {
			case SST::OpalComponent::EventType::MMAP:
			{
				OPAL_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Node%" PRIu32 " Opal has received an MMAP CALL\n", node_num));
			}
			break;

			case SST::OpalComponent::EventType::UNMAP:
			{
				OPAL_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Node%" PRIu32 " Opal has received an UNMAP CALL\n", node_num));
			}
			break;

			case SST::OpalComponent::EventType::REQUEST:
			{
				long long int address;

				if( !allocatedmempool[node_num] )
				{
					address = allocLocalMemPool( node_num, linkId, ev->getAddress(), ev->getSize() ); // 0 search in local memory
					//If local memory is full then allocate memory from next shared memory pool according to the allocation policy
					if(address == -1) {
						uint32_t search_count;
						OPAL_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Node%" PRIu32 " Local Memory is drained out\n", node_num));
						for(search_count = 0; search_count < num_shared_mempools && address == -1; search_count++) {
							if(allocpolicy) {
								setNextMemPool(node_num);
								if( !allocatedmempool[node_num] ) //local memory is already full. No point searching in local memory again
									setNextMemPool(node_num);
								address = allocSharedMemPool( node_num, linkId, ev->getAddress(), ev->getSize() );
							} else {
								allocatedmempool[node_num] = (rand()%num_shared_mempools) + 1;
								address = allocSharedMemPool( node_num, linkId, ev->getAddress(), ev->getSize() );
							}
						}
					}
					setNextMemPool(node_num);
				}
				else
				{
					address = allocSharedMemPool( node_num, linkId, ev->getAddress(), ev->getSize() );
					if(address == -1) {
						uint32_t search_count;
						for(search_count = 0; search_count < num_shared_mempools && address == -1; search_count++) {
							setNextMemPool(node_num);
							address = allocSharedMemPool( node_num, linkId, ev->getAddress(), ev->getSize() );

						}
					}
					setNextMemPool(node_num);
				}

				if(address == -1)
					output->fatal(CALL_INFO, -1, "Opal: Memory is drained out\n");

			}
			break;

			default:
				output->fatal(CALL_INFO, -1, "%s, Error - Unknown request\n", getName().c_str());
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


void Opal::finish()
{
	uint32_t mem;

	for(mem = 0; mem < num_nodes; mem++ )
	  local_mem[mem]->finish();


	for(mem = 0; mem < num_shared_mempools; mem++ )
	  shared_mem[mem]->finish();

}

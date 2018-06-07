// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
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
	num_nodes = (uint32_t) params.find<uint32_t>("num_nodes", 1);
	nodeInfo = new NodePrivateInfo*[num_nodes];
	std::cerr << "Maximum Instructions per cycle: " << max_inst << std::endl;


	char* buffer = (char*) malloc(sizeof(char) * 256);

	/* Configuring shared memory */
	/*----------------------------------------------------------------------------------------*/

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
		shared_mem[i] = new Pool(this, memPoolParams, SST::OpalComponent::MemType::SHARED, i);
		shared_mem_size += memPoolParams.find<long long int>("size", 0);
	}

	/* Configuring nodes */
	/*----------------------------------------------------------------------------------------*/

	int linksCount = 0;
	for(i = 0; i < num_nodes; i++) {
		memset(buffer, 0 , 256);
		sprintf(buffer, "node%" PRIu32 ".", i);
		Params nodePrivateParams = params.find_prefix_params(buffer);
		nodeInfo[i] = new NodePrivateInfo(this, i, nodePrivateParams);
		for(uint32_t j=0; j<nodeInfo[i]->cores*2; j++) {
			memset(buffer, 0 , 256);
			sprintf(buffer, "requestLink%" PRIu32, linksCount + j);
			nodeInfo[i]->Handlers[j].singLink = configureLink(buffer, "1ns", new Event::Handler<core_handler>((&nodeInfo[i]->Handlers[j]), &core_handler::handleRequest));
		}
		linksCount += nodeInfo[i]->cores*2;
	}

	free(buffer);

	/* registering clock */
	/*----------------------------------------------------------------------------------------*/
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
	switch(nodeInfo[node]->memoryAllocationPolicy)
	{
	case 4:
		//random allocation policy
		nodeInfo[node]->allocatedmempool = rand() % ( num_shared_mempools + 1 );
		break;

	case 3:
		//proportional allocation policy
		nodeInfo[node]->nextallocmem = ( nodeInfo[node]->nextallocmem + 1 ) % ( shared_mem_size/nodeInfo[node]->memory_size + 1 );
		nodeInfo[node]->allocatedmempool = nodeInfo[node]->nextallocmem ? ( nodeInfo[node]->allocatedmempool + 1 ) % ( num_shared_mempools + 1)
					? ( nodeInfo[node]->allocatedmempool + 1 ) % ( num_shared_mempools + 1) : 1 : 0;

		break;

	case 2:
		//round robin allocation policy
		nodeInfo[node]->nextallocmem = ( nodeInfo[node]->nextallocmem + 1 ) % ( num_shared_mempools + 1 );
		nodeInfo[node]->allocatedmempool = nodeInfo[node]->nextallocmem;
		break;

	case 1:
		//alternate allocation policy
		if( nodeInfo[node]->allocatedmempool != 0) {
			nodeInfo[node]->allocatedmempool = 0;
		} else {
			nodeInfo[node]->nextallocmem = ( nodeInfo[node]->nextallocmem + 1 ) % ( num_shared_mempools + 1 )
								? ( nodeInfo[node]->nextallocmem + 1 ) % ( num_shared_mempools + 1 ) : 1;
			nodeInfo[node]->allocatedmempool = nodeInfo[node]->nextallocmem;
		}
		break;

	case 0:
	default:
		//local memory first
		nodeInfo[node]->allocatedmempool = 0;
		break;

	}

}

long long int Opal::allocateMemory( SST::OpalComponent::MemType memType, int node, int requested_size, int *allocated_size)
{
	Pool *mempool;
	if( memType == SST::OpalComponent::MemType::LOCAL)
		mempool = nodeInfo[node]->memory_pool;
	else
		mempool = shared_mem[nodeInfo[node]->allocatedmempool -1];

	MemPoolResponse pool_resp = mempool->allocate_frames(requested_size);

	if(pool_resp.pAddress != -1)
		*allocated_size = pool_resp.num_frames*pool_resp.frame_size*1024;

	std::cerr << getName().c_str() << " Node: " << node << " Allocated address: " << pool_resp.pAddress << " of size: " << allocated_size << std::endl;
	return pool_resp.pAddress;
}

void Opal::registerHint(int node, int fileId, long long int vAddress, int size)
{

	std::map<int, std::pair<std::list<int>, long long int> >::iterator fileIdHint = mmapFileIdHints.find(fileId);

	//fileId is already registered by another node
	if( fileIdHint != mmapFileIdHints.end() )
	{
		//search for nodeId
		std::list<int> it = (fileIdHint->second).first;
	    auto it1 = std::find(it.begin(), it.end(), node);
		if( it1 != it.end() )
		{
			std::cerr << "Physical memory " << std::hex << (fileIdHint->second).second << " is already allocated for fileId: " << fileIdHint->first << " in the same node" << std::endl;
		}
		else
		{
			it.push_back(node);
			nodeInfo[node]->reservedSpace.insert(std::make_pair(vAddress, std::make_pair(fileId, std::make_pair(size,0))));
		}
	}
	else
	{
		std::list<int> it;
		it.push_back(node);
		(fileIdHint->second).first = it;
		mmapFileIdHints.insert(std::make_pair(fileId, std::make_pair( it, -1 )));

		nodeInfo[node]->reservedSpace.insert(std::make_pair(vAddress, std::make_pair(fileId, std::make_pair(size,0))));

	}
}

long long int Opal::isAddressReserved(int node,long long int vAddress)
{
	for (std::map<long long int, std::pair<int, std::pair<int, int> > >::iterator it= (nodeInfo[node]->reservedSpace).begin(); it!=(nodeInfo[node]->reservedSpace).end(); ++it)
	{
		long long int virtAddress = it->first;
		int reservedSize = (it->second).second.first;
		if(virtAddress <= vAddress && vAddress < virtAddress + reservedSize)
			return virtAddress;
	}

	return -1;
}

long long int Opal::allocateLocalMemory(int node, int size, int *allocated_size)
{
	int allocated_pAddress = -1;

	allocated_pAddress = allocateMemory( SST::OpalComponent::MemType::LOCAL, node, size, allocated_size ); // 0 search in local memory
	//If local memory is full then allocate memory from next shared memory pool according to the allocation policy
	if(allocated_pAddress == -1) {
		OPAL_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Node%" PRIu32 " Local Memory is drained out\n", node));
		uint32_t search_count;
		for(search_count = 0; search_count < num_shared_mempools && allocated_pAddress == -1; search_count++) {
			if(nodeInfo[node]->memoryAllocationPolicy) {
				setNextMemPool(node);
				if( !nodeInfo[node]->allocatedmempool ) //local memory is already full. No point searching in local memory again
					setNextMemPool(node);
				allocated_pAddress = allocateMemory( SST::OpalComponent::MemType::SHARED, node, size, allocated_size );
			} else {
				nodeInfo[node]->allocatedmempool = (rand()%num_shared_mempools) + 1;
				allocated_pAddress = allocateMemory( SST::OpalComponent::MemType::SHARED, node, size, allocated_size );
			}
		}
	}
	setNextMemPool(node);

	return allocated_pAddress;
}

long long int Opal::allocateSharedMemory(int node, int size, int *allocated_size)
{
	int allocated_pAddress = -1;

	allocated_pAddress = allocateMemory( SST::OpalComponent::MemType::SHARED, node, size, allocated_size );
	if(allocated_pAddress == -1) {
		uint32_t search_count;
		for(search_count = 0; search_count < num_shared_mempools && allocated_pAddress == -1; search_count++)
		{
			setNextMemPool(node);
			allocated_pAddress = allocateMemory( SST::OpalComponent::MemType::SHARED, node, size, allocated_size );
		}
	}
	setNextMemPool(node);

	return allocated_pAddress;
}

long long int Opal::allocateFromReservedMemory(int node, long long int reserved_address, long long int vAddress, int requested_size, int *allocated_size)
{
	long long int address = -1;

	int fileID = nodeInfo[node]->reservedSpace[reserved_address].first;
	int size_reserved = nodeInfo[node]->reservedSpace[reserved_address].second.first;
	int size_used = nodeInfo[node]->reservedSpace[reserved_address].second.second;

	if( size_used == 0 )
	{
		address = allocateSharedMemory(node, size_reserved, allocated_size);
		if(address == -1)
			output->fatal(CALL_INFO, -1, "Opal: Memory is drained out\n");

		mmapFileIdHints[fileID].second = address;
		nodeInfo[node]->reservedSpace[reserved_address].second.second = requested_size;
		*allocated_size = size_reserved;
	}
	else if( size_used+requested_size <= size_reserved )
	{
		address = mmapFileIdHints[fileID].second + size_used;
		nodeInfo[node]->reservedSpace[reserved_address].second.second += requested_size;
	}
	else
	{
		output->fatal(CALL_INFO, -1, "Opal: address :%lld requested with fileId:%d has no space left\n", vAddress, fileID);
	}

	return address;
}

long long int Opal::processRequest(int node, int linkId, long long int page_fault_vAddress, int size)
{
	long long int allocated_pAddress = -1;
	int allocated_size = 1;

	//what if 4 pages are reserved and 3rd page virtual address is received. Find the valid virtual address to search
	long long int reserved_address;
	reserved_address = isAddressReserved(node, page_fault_vAddress);
	if( reserved_address != -1)
		allocated_pAddress = allocateFromReservedMemory(node, reserved_address, page_fault_vAddress, size, &allocated_size);
	else {
		if( !nodeInfo[node]->allocatedmempool )
			allocated_pAddress = allocateLocalMemory(node, size, &allocated_size);
		else
			allocated_pAddress = allocateSharedMemory(node, size, &allocated_size);

	}

	if( allocated_pAddress != -1 ) {
		OpalEvent *tse = new OpalEvent(EventType::RESPONSE);
		tse->setResp(page_fault_vAddress, allocated_pAddress, allocated_size);//pool_resp.num_frames*pool_resp.frame_size*1024);
		nodeInfo[node]->Handlers[linkId].singLink->send(nodeInfo[node]->latency, tse);
	}
	else
		output->fatal(CALL_INFO, -1, "Opal: Memory is drained out\n");

	return allocated_pAddress;
}

bool Opal::tick(SST::Cycle_t x)
{

	int inst_served = 0;

	while(!requestQ.empty()) {
		if(inst_served < max_inst) {

			OpalEvent *ev = requestQ.front();
			int node_num = ev->getNodeId();

			switch(ev->getType()) {
			case SST::OpalComponent::EventType::MMAP:
			{
				OPAL_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Node%" PRIu32 " Opal has received an MMAP CALL\n", node_num));
				//size should be in the multiple of page size (4096) from ariel core
				registerHint(node_num, ev->fileID, ev->getAddress(), ev->getSize());

			}
			break;

			case SST::OpalComponent::EventType::UNMAP:
			{
				OPAL_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Node%" PRIu32 " Opal has received an UNMAP CALL\n", node_num));
			}
			break;

			case SST::OpalComponent::EventType::REQUEST:
			{
				//size is 4096 (4K pages) from PTW
				processRequest(node_num, ev->getLinkId(), ev->getAddress(), ev->getSize());

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
	uint32_t i;

	for(i = 0; i < num_nodes; i++ )
	  nodeInfo[i]->memory_pool->finish();


	for(i = 0; i < num_shared_mempools; i++ )
	  shared_mem[i]->finish();

}


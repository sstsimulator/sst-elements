// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
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
#include "Opal.h"

#include <string>
#include <iostream>

using namespace SST::Interfaces;
using namespace SST;
using namespace SST::OpalComponent;


#define OPAL_VERBOSE(LEVEL, OUTPUT) if(verbosity >= (LEVEL)) OUTPUT


Opal::Opal(SST::ComponentId_t id, SST::Params& params): Component(id) {


 registerAsPrimaryComponent();

    verbosity = (uint32_t) params.find<uint32_t>("verbose", 1);
	output = new SST::Output("OpalComponent[@f:@l:@p] ", verbosity, 0, SST::Output::STDOUT);

	max_inst = (uint32_t) params.find<uint32_t>("max_inst", 1);
	num_nodes = (uint32_t) params.find<uint32_t>("num_nodes", 1);
	nodeInfo = new NodePrivateInfo*[num_nodes];
	num_cores = 0;
	num_memCntrls = 0;

	cycles = 0;

	opalBase = new OpalBase();

	char* buffer = (char*) malloc(sizeof(char) * 256);

	/* Configuring shared memory */
	/*----------------------------------------------------------------------------------------*/
	num_shared_mempools = params.find<uint32_t>("shared_mempools", 0);
	std::cerr << getName().c_str() << "Number of Shared Memory Pools: "<< num_shared_mempools << endl;

	Params sharedMemParams = params.get_scoped_params("shared_mem");
	shared_mem_size = 0;

	sharedMemoryInfo = new MemoryPrivateInfo*[num_shared_mempools];

	for(uint32_t i = 0; i < num_shared_mempools; i++) {
		memset(buffer, 0 , 256);
		sprintf(buffer, "mempool%" PRIu32 "", i);
		Params memPoolParams = sharedMemParams.get_scoped_params(buffer);
		sharedMemoryInfo[i] = new MemoryPrivateInfo(opalBase, i, memPoolParams);
		std::cerr << getName().c_str() << "Configuring Shared " << buffer << std::endl;
		shared_mem_size += memPoolParams.find<uint64_t>("size", 0);
		memset(buffer, 0 , 256);
		sprintf(buffer, "globalMemCntrLink%" PRIu32, i);
		sharedMemoryInfo[i]->link = configureLink(buffer, "1ns", new Event::Handler<MemoryPrivateInfo>((sharedMemoryInfo[i]), &MemoryPrivateInfo::handleRequest));
	}

	/* Configuring nodes */
	/*----------------------------------------------------------------------------------------*/
	for(uint32_t i = 0; i < num_nodes; i++) {
		memset(buffer, 0 , 256);
		sprintf(buffer, "node%" PRIu32 "", i);
		Params nodePrivateParams = params.get_scoped_params(buffer);
		nodeInfo[i] = new NodePrivateInfo(opalBase, i, nodePrivateParams);
		for(uint32_t j=0; j<nodeInfo[i]->cores; j++) {
			memset(buffer, 0 , 256);
			sprintf(buffer, "coreLink%" PRIu32, num_cores);
			nodeInfo[i]->coreInfo[j].coreLink = configureLink(buffer, "1ns", new Event::Handler<CorePrivateInfo>((&nodeInfo[i]->coreInfo[j]), &CorePrivateInfo::handleRequest));
			memset(buffer, 0 , 256);
			sprintf(buffer, "mmuLink%" PRIu32, num_cores);
			nodeInfo[i]->coreInfo[j].mmuLink = configureLink(buffer, "1ns", new Event::Handler<CorePrivateInfo>((&nodeInfo[i]->coreInfo[j]), &CorePrivateInfo::handleRequest));
			num_cores++;
		}
		for(uint32_t j=0; j<nodeInfo[i]->memory_cntrls; j++) {
			memset(buffer, 0 , 256);
			sprintf(buffer, "memCntrLink%" PRIu32, num_memCntrls);
			nodeInfo[i]->memCntrlInfo[j].link = configureLink(buffer, "1ns", new Event::Handler<MemoryPrivateInfo>((&nodeInfo[i]->memCntrlInfo[j]), &MemoryPrivateInfo::handleRequest));
			num_memCntrls++;
		}

		char* subID = (char*) malloc(sizeof(char) * 32);
		sprintf(subID, "%" PRIu32, i);
		nodeInfo[i]->statLocalMemUsage = registerStatistic<uint64_t>("local_mem_usage", subID );
		nodeInfo[i]->statSharedMemUsage = registerStatistic<uint64_t>("shared_mem_usage", subID );
		free(subID);
	}

	free(buffer);

	/* registering clock */
	/*----------------------------------------------------------------------------------------*/
	std::string cpu_clock = params.find<std::string>("clock", "1GHz");
	std::cerr << "clock: "<< cpu_clock.c_str() << std::endl;
	registerClock( cpu_clock, new Clock::Handler<Opal>(this, &Opal::tick ) );
}



Opal::Opal() : Component(-1)
{
	// for serialization only
	//
}


void Opal::setNextMemPool( int node, int fault_level )
{
	switch(nodeInfo[node]->memoryAllocationPolicy)
	{
	case 8:
		//alternate allocation policy 1:16
		nodeInfo[node]->nextallocmem = ( nodeInfo[node]->nextallocmem + 1 ) % 17;
		nodeInfo[node]->allocatedmempool = nodeInfo[node]->nextallocmem;
		break;

	case 7:
		//alternate allocation policy 1:8
		nodeInfo[node]->nextallocmem = ( nodeInfo[node]->nextallocmem + 1 ) % 9;
		nodeInfo[node]->allocatedmempool = nodeInfo[node]->nextallocmem;
		break;

	case 6:
		//alternate allocation policy 1:4
		nodeInfo[node]->nextallocmem = ( nodeInfo[node]->nextallocmem + 1 ) % 5;
		nodeInfo[node]->allocatedmempool = nodeInfo[node]->nextallocmem;
		break;

	case 5:
		//alternate allocation policy 1:2
		nodeInfo[node]->nextallocmem = ( nodeInfo[node]->nextallocmem + 1 ) % 3;
		nodeInfo[node]->allocatedmempool = nodeInfo[node]->nextallocmem;
		break;

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

void Opal::processHint(int node, int fileId, uint64_t vAddress, int size)
{

	std::map<int, std::pair<std::vector<int>*, std::vector<uint64_t>* > >::iterator fileIdHint = opalBase->mmapFileIdHints.find(fileId);

	//fileId is already registered by another node
	if( fileIdHint != opalBase->mmapFileIdHints.end() )
	{
		//search for nodeId
		std::vector<int>* it = (fileIdHint->second).first;
	    auto it1 = std::find(it->begin(), it->end(), node);
		if( it1 != it->end() )
		{
			std::cerr << "Memory is already allocated for fileId: " << fileIdHint->first << " in the same node" << std::endl;
		}
		else
		{
			int owner_node = it->back();
			if( nodeInfo[owner_node]->page_size != nodeInfo[node]->page_size)
				output->fatal(CALL_INFO, -1, "Opal: Page sizes of the owner node which reserved space with fileId: %d and the requesting node are not same\n",fileId);

			it->push_back(node);
			//(fileIdHint->second).first = it;
			nodeInfo[node]->reservedSpace.insert(std::make_pair(vAddress/4096, std::make_pair(fileId, std::make_pair( ceil(size/(nodeInfo[node]->page_size)), 0))));
		}
	}
	else
	{
		std::vector<int> *it = new std::vector<int>;
		std::vector<uint64_t> *pa = new std::vector<uint64_t>;

		it->push_back(node);
		opalBase->mmapFileIdHints.insert(std::make_pair(fileId, std::make_pair( it, pa )));
		nodeInfo[node]->reservedSpace.insert(std::make_pair(vAddress/4096, std::make_pair(fileId, std::make_pair( ceil(size/(nodeInfo[node]->page_size)), 0))));

	}
}

REQRESPONSE Opal::isAddressReserved(int node, uint64_t vAddress)
{
	REQRESPONSE response;
	response.status = 0;


	for (std::map<uint64_t, std::pair<int, std::pair<int, int> > >::iterator it= (nodeInfo[node]->reservedSpace).begin(); it!=(nodeInfo[node]->reservedSpace).end(); ++it)
	{
		uint64_t reservedVAddress = it->first;
		int pages_reserved = (it->second).second.first;
		if(reservedVAddress <= vAddress && vAddress < reservedVAddress + pages_reserved*nodeInfo[node]->page_size) {
			response.status = 1;
			response.address = reservedVAddress;
		}
	}

	return response;
}

REQRESPONSE Opal::allocateSharedMemory(int node, int coreId, uint64_t vAddress, int fault_level, int pages)
{
	REQRESPONSE response;
	response.status = 0;

	int sharedMemPoolId;

	if(nodeInfo[node]->memoryAllocationPolicy) {

		sharedMemPoolId = nodeInfo[node]->allocatedmempool - 1;

	}
	else {

		for(uint32_t i = 0; i<num_shared_mempools; i++)
		{
			if( sharedMemoryInfo[i]->pool->available_frames >= pages )
			{
				Pool *pool = sharedMemoryInfo[i]->pool;
				for(int j=0; j<pages; j++) {
					response = pool->allocate_frame(1);
					if(!response.status)
						output->fatal(CALL_INFO, -1, "Opal: Allocating shared memory. This should never happen\n");

					nodeInfo[node]->profileEvent(SST::OpalComponent::MemType::SHARED);
				}

				response.pages = pages;
				response.status = 1;
				break;
			}
		}

		if(!response.status)
			output->fatal(CALL_INFO, -1, "Opal(%s): Memory is drained out\n",getName().c_str());

		return response;
	}

	if( sharedMemoryInfo[sharedMemPoolId]->pool->available_frames >= pages ) {
		Pool *pool = sharedMemoryInfo[sharedMemPoolId]->pool;
		for(int j=0; j<pages; j++) {
			response = pool->allocate_frame(1);
			if(!response.status)
				output->fatal(CALL_INFO, -1, "Opal: Allocating shared memory. This should never happen\n");

			nodeInfo[node]->profileEvent(SST::OpalComponent::MemType::SHARED);
		}

		setNextMemPool( node,fault_level );
		response.pages = pages;
		response.status = 1;
	}
	else
	{
		for(uint32_t i = 0; i < num_shared_mempools; i++)
		{
			setNextMemPool(node,fault_level);
			if( !nodeInfo[node]->allocatedmempool ) // skip local memory
				setNextMemPool( node,fault_level );

			sharedMemPoolId = nodeInfo[node]->allocatedmempool - 1;

			if( sharedMemoryInfo[sharedMemPoolId]->pool->available_frames >= pages ) {
				Pool *pool = sharedMemoryInfo[sharedMemPoolId]->pool;
				for(int j=0; j<pages; j++) {
					response = pool->allocate_frame(1);
					if(!response.status)
						output->fatal(CALL_INFO, -1, "Opal: Allocating shared memory. This should never happen\n");

					nodeInfo[node]->profileEvent(SST::OpalComponent::MemType::SHARED);
				}

				setNextMemPool( node,fault_level );
				response.pages = pages;
				response.status = 1;
				break;
			}
		}

		if(!response.status)
			output->fatal(CALL_INFO, -1, "Opal: Memory is drained out\n");

	}

	return response;
}

REQRESPONSE Opal::allocateLocalMemory(int node, int coreId, uint64_t vAddress, int fault_level, int pages)
{

	REQRESPONSE response;
	response.status = 0;


	if(nodeInfo[node]->pool->available_frames >= pages) {
		Pool *pool = nodeInfo[node]->pool;
		for(int i=0; i<pages; i++) {
			response = pool->allocate_frame(1);
			if(!response.status)
				output->fatal(CALL_INFO, -1, "Opal: Allocating local memory. This should never happen\n");

			nodeInfo[node]->profileEvent(SST::OpalComponent::MemType::LOCAL);
		}

		response.pages = pages;
		response.status = 1;
		setNextMemPool( node,fault_level );
	}
	else {
		OPAL_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Node%" PRIu32 " Local Memory is drained out\n", node));

		setNextMemPool( node,fault_level );
		response = allocateSharedMemory(node, coreId, vAddress, fault_level, pages);
	}


	return response;

}

REQRESPONSE Opal::allocateFromReservedMemory(int node, uint64_t reserved_vAddress, uint64_t vAddress, int pages)
{
	REQRESPONSE response;
	response.status = 0;

	int fileID = nodeInfo[node]->reservedSpace[reserved_vAddress].first;
	int pages_reserved = nodeInfo[node]->reservedSpace[reserved_vAddress].second.first;
	int pages_used = nodeInfo[node]->reservedSpace[reserved_vAddress].second.second;

	std::vector<uint64_t> *reserved_pAddress = opalBase->mmapFileIdHints[fileID].second;

	//Allocate all the pages. TODO: pages can be reserved on demand instead of allocating all the pages at a time. But what if the memory is drained out.
	if(reserved_pAddress->empty()) {

		for(uint32_t i = 0; i<num_shared_mempools; i++) {

			if( sharedMemoryInfo[i]->pool->available_frames >= pages ) {
				Pool *pool = sharedMemoryInfo[i]->pool;
				for(int j=0; j<pages_reserved; j++) {
					response = pool->allocate_frame(1);
					reserved_pAddress->push_back( response.address );

					if(!response.status)
						output->fatal(CALL_INFO, -1, "Opal: Allocating reserved memory. This should never happen\n");
				}

				response.pages = pages;
				response.status = 1;
				break;
			}

		}

		if(!response.status)
			output->fatal(CALL_INFO, -1, "Opal: memory not available to allocate memory for file ID: %d \n", fileID);

	}

	if( pages_used + pages <= pages_reserved )
	{

		auto it = reserved_pAddress->begin();
		std::advance(it, pages_used);
		response.address = *it;
		response.pages = pages;
		response.status = 1;
		nodeInfo[node]->reservedSpace[reserved_vAddress].second.second += pages;

	}
	else
	{
		output->fatal(CALL_INFO, -1, "Opal: address :%llu requested with fileId:%d has no space left\n", vAddress, fileID);
	}

	return response;
}

bool Opal::processRequest(int node, int coreId, uint64_t vAddress, int fault_level, int size)
{

	REQRESPONSE response;
	response.status = 0;

	int pages = ceil(size/(nodeInfo[node]->page_size));

	// If multiple pages are requested how are the physical addresses sent to the requester as in future sue to opal parallelization continuous addresses cannot be allocated
	if(pages != 1)
		output->fatal(CALL_INFO, -1, "Opal: currently opal does not support multiple page allocations\n");

	// if the page fault request is for CR3 register allocate the memory from local memory
	if(4 == fault_level)
		response = allocateLocalMemory(node, coreId, vAddress, fault_level, pages);
	else
	{

		// check if memory is to be allocated from the reserved address space
		response = isAddressReserved(node, vAddress);

		if( response.status )
			response = allocateFromReservedMemory(node, response.address, vAddress, pages);

		else {
			if( !nodeInfo[node]->allocatedmempool ) {
				response = allocateLocalMemory(node, coreId, vAddress, fault_level, pages);
				//std::cerr << getName() << " Node: " << node << " core " << coreId << " response page address: " << vAddress << " allocated local address: " << response.address << " pages: "<< pages << " level: " << fault_level  << std::endl;
			}
			else {
				response = allocateSharedMemory(node, coreId, vAddress, fault_level, pages);
				//std::cerr << getName() << " Node: " << node << " core " << coreId << " response page address: " << vAddress << " allocated shared address: " << response.address << " pages: " << " level: " << fault_level << std::endl;
			}
		}
	}

	if( response.status ) {
		OpalEvent *tse = new OpalEvent(EventType::RESPONSE);
		tse->setResp(vAddress, response.address, response.pages*nodeInfo[node]->page_size);
		tse->setCoreId(coreId);
		nodeInfo[node]->coreInfo[coreId].mmuLink->send(tse);
	}
	else
		output->fatal(CALL_INFO, -1, "Opal(%s): Memory is drained out\n",getName().c_str());

	return true;

}


bool Opal::tick(SST::Cycle_t x)
{
	cycles++;

	int inst_served = 0;
	while(!opalBase->requestQ.empty()) {
		if(inst_served < max_inst) {
			OpalEvent *ev = opalBase->requestQ.front();
			bool removeEvent = true;

			switch(ev->getType()) {
			case SST::OpalComponent::EventType::HINT:
			{
				std::cerr << getName().c_str() << " node: " << ev->getNodeId() << " core: "<< ev->getCoreId() << " request page address: " << ev->getAddress() << " hint" << std::endl;
			}
			break;

			case SST::OpalComponent::EventType::MMAP:
			{
				OPAL_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Node%" PRIu32 " Opal has received an MMAP CALL\n", ev->getNodeId()));
				std::cerr << "MLM mmap(" << ev->getFileId()<< ") : level "<< ev->getHint() << " Starting address is "<< std::hex << ev->getAddress();
				std::cerr << std::dec << " Size: "<< ev->getSize();
				std::cerr << " Ending address is " << std::hex << ev->getAddress() + ev->getSize() - 1;
				std::cerr << std::dec << std::endl;
				//size should be in the multiple of page size (4096) from ariel core
				//processHint(ev->getNodeId(), ev->getFileId(), ev->getAddress(), ev->getSize());
			}
			break;

			case SST::OpalComponent::EventType::UNMAP:
			{
				std::cerr << getName().c_str() << " node: " << ev->getNodeId() << " core: "<< ev->getCoreId() << " request page address: " << ev->getAddress() << " unmap"<< std::endl;
				OPAL_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Node%" PRIu32 " Opal has received an UNMAP CALL\n", ev->getNodeId()));
			}
			break;

			case SST::OpalComponent::EventType::REQUEST:
			{
				removeEvent = processRequest(ev->getNodeId(), ev->getCoreId(), ev->getAddress(), ev->getFaultLevel(), ev->getSize());
			}
			break;

			default:
				output->fatal(CALL_INFO, -1, "%s, Error - Unknown request\n", getName().c_str());
				break;

			}

			if(!removeEvent) {
				break;
			}

			opalBase->requestQ.pop();
			delete ev;
			inst_served++;
		}
		else {
			output->verbose(CALL_INFO, 2, 0, "%s, number of requests served has reached maximum width in the given time slot \n", getName().c_str());
			break;
		}
	}

	return false;
}


void Opal::finish()
{
	for(uint32_t i = 0; i < num_nodes; i++ )
	  nodeInfo[i]->pool->finish();

	for(uint32_t i = 0; i < num_shared_mempools; i++ )
	  sharedMemoryInfo[i]->pool->finish();

}

void Opal::deallocateSharedMemory(uint64_t page, int N)
{
	for(uint32_t sm=0; sm<num_shared_mempools; sm++)
		if(sharedMemoryInfo[sm]->contains(page)) {
			sharedMemoryInfo[sm]->pool->deallocate_frame(page, 1);
			break;
		}
}


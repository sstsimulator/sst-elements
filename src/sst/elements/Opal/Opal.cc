// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2019, NTESS
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
#include <thread>
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

	ariel_enable_count = 0;
	max_inst = (uint32_t) params.find<uint32_t>("max_inst", 1);
	num_nodes = (uint32_t) params.find<uint32_t>("num_nodes", 1); 	std::cerr << "Number of Nodes: "<< num_nodes << std::endl;
	nodeInfo = new NodePrivateInfo*[num_nodes];
	num_cores = 0;
	num_memCntrls = 0;
	uint32_t shootdown_latency = (uint32_t) params.find<uint32_t>("shootdown_latency", 1); std::cerr << "Shootdown latency: "<< shootdown_latency << std::endl;

	std::cerr << "Maximum Instructions per cycle: " << max_inst << std::endl;

	cycles = 0;

	char* buffer = (char*) malloc(sizeof(char) * 256);

	/* Configuring shared memory */
	/*----------------------------------------------------------------------------------------*/

	num_shared_mempools = params.find<uint32_t>("shared_mempools", 0);
	std::cerr << "Number of Shared Memory Pools: "<< num_shared_mempools << endl;

	Params sharedMemParams = params.find_prefix_params("shared_mem.");
	shared_mem_size = 0;

	sharedMemoryInfo = new MemoryPrivateInfo*[num_shared_mempools];//Pool*[num_shared_mempools];

	for(uint32_t i = 0; i < num_shared_mempools; i++) {
		memset(buffer, 0 , 256);
		sprintf(buffer, "mempool%" PRIu32 ".", i);
		Params memPoolParams = sharedMemParams.find_prefix_params(buffer);
		sharedMemoryInfo[i] = new MemoryPrivateInfo(this, i, memPoolParams);
		std::cerr << "Configuring Shared " << buffer << std::endl;
		shared_mem_size += memPoolParams.find<uint64_t>("size", 0);
		memset(buffer, 0 , 256);
		sprintf(buffer, "globalMemCntrLink%" PRIu32, i);
		sharedMemoryInfo[i]->link = configureLink(buffer, "1ns", new Event::Handler<MemoryPrivateInfo>((sharedMemoryInfo[i]), &MemoryPrivateInfo::handleRequest));
		//num_memCntrls++;
	}

	/* Configuring nodes */
	/*----------------------------------------------------------------------------------------*/

	for(uint32_t i = 0; i < num_nodes; i++) {
		memset(buffer, 0 , 256);
		sprintf(buffer, "node%" PRIu32 ".", i);
		Params nodePrivateParams = params.find_prefix_params(buffer);
		nodeInfo[i] = new NodePrivateInfo(this, i, nodePrivateParams);
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

		// IPI + context switching latency with respect to number of cores. 15000 cycles for 2 cores
		// nodeInfo[i]->shootdown_latency = (nodeInfo[i]->cores/2)*(15000/nodeInfo[i]->clock)*1000;
		nodeInfo[i]->shootdown_latency = shootdown_latency; // ns

	}

	free(buffer);

	self_link = configureSelfLink("opal_self_link", "1ns", new Event::Handler<Opal>(this, &Opal::handleEvent));

	ptw_aware_allocation = params.find<uint32_t>("ptw_aware_allocation", 0); std::cerr << "ptw aware allocation: "<< ptw_aware_allocation << std::endl;

	//page_placement = params.find<uint32_t>("page_placement", 0); std::cerr << "page placement: "<< page_placement << std::endl;

	//localPageReferenceQ = new std::queue<OpalEvent*>[num_nodes];
	//localPageReferenceQ = new std::vector<uint64_t>[num_nodes];
	//globalPageReferenceQ = new std::queue<OpalEvent*>[num_shared_mempools];

	//for(uint32_t n=0; n<num_nodes; n++)
	//{
		//local_page_reference_threads.push_back(std::thread(&Opal::localPageReferences, this, n));
	//	nodeInfo[n]->local_page_ref_epoch = params.find<uint32_t>("page_ref_epoch", 1000);
	//	nodeInfo[n]->migration_epoch = params.find<uint32_t>("migration_epoch", 100000);
	//	std::cerr << "migration epoch/print page access stats epoch: "<< nodeInfo[n]->migration_epoch << std::endl;
	//}
	//for(uint32_t s=0; s<num_shared_mempools; s++)
	//{
		//global_page_reference_threads.push_back(std::thread(&Opal::globalPageReferences, this, s));
	//}

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

void Opal::processHint(int node, int fileId, uint64_t vAddress, int size)
{

	std::map<int, std::pair<std::vector<int>*, std::vector<uint64_t>* > >::iterator fileIdHint = mmapFileIdHints.find(fileId);

	//fileId is already registered by another node
	if( fileIdHint != mmapFileIdHints.end() )
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
		mmapFileIdHints.insert(std::make_pair(fileId, std::make_pair( it, pa )));
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

		for(uint32_t i = 0; i<num_shared_mempools; i++) {

			/*
			 * TODO:
			 *    Opal is not parallel for now, i.e, requests are addressed serially.
			 * 	  So just checking if the memory is available or not and allocating.
			 * 	  In future lock share memory pool and allocate pages if available.
			 */
			if( sharedMemoryInfo[i]->pool->available_frames >= pages ) {

				Pool *pool = sharedMemoryInfo[i]->pool;
				for(int j=0; j<pages; j++) {
					response = pool->allocate_frame(1);
					nodeInfo[node]->insertFrame(response.address, vAddress, fault_level, SST::OpalComponent::MemType::SHARED);

					if(!response.status)
						output->fatal(CALL_INFO, -1, "Opal: Allocating shared memory. This should never happen\n");
				}

				response.pages = pages;
				response.status = 1;
				break;
			}
		}

		if(!response.status)
			output->fatal(CALL_INFO, -1, "Opal: Memory is drained out\n");

		return response;
	}

	/*
	 * TODO:
	 *    Opal is not parallel for now, i.e, requests are addressed serially.
	 * 	  So just checking if the memory is available or not and allocating.
	 * 	  In future lock share memory pool and allocate pages if available.
	 */
	if( sharedMemoryInfo[sharedMemPoolId]->pool->available_frames >= pages ) {
		Pool *pool = sharedMemoryInfo[sharedMemPoolId]->pool;
		for(int j=0; j<pages; j++) {
			response = pool->allocate_frame(1);
			nodeInfo[node]->insertFrame(response.address, vAddress, fault_level, SST::OpalComponent::MemType::SHARED);

			if(!response.status)
				output->fatal(CALL_INFO, -1, "Opal: Allocating shared memory. This should never happen\n");
		}

		setNextMemPool( node );
		response.pages = pages;
		response.status = 1;
	}
	else
	{

		// Memory in shared memory pool: " << sharedMemPoolId << " is full. Searching in other shared memory pools" << endl;

		for(uint32_t i = 0; i < num_shared_mempools; i++)
		{
			setNextMemPool(node);
			if( !nodeInfo[node]->allocatedmempool ) // skip local memory
				setNextMemPool( node );

			sharedMemPoolId = nodeInfo[node]->allocatedmempool - 1;

			/*
			 * TODO:
			 *    Opal is not parallel for now, i.e, requests are addressed serially.
			 * 	  So just checking if the memory is available or not and allocating.
			 * 	  In future lock share memory pool and allocate pages if available.
			 */
			if( sharedMemoryInfo[sharedMemPoolId]->pool->available_frames >= pages ) {
				Pool *pool = sharedMemoryInfo[sharedMemPoolId]->pool;
				for(int j=0; j<pages; j++) {
					response = pool->allocate_frame(1);
					nodeInfo[node]->insertFrame(response.address, vAddress, fault_level, SST::OpalComponent::MemType::SHARED);

					if(!response.status)
						output->fatal(CALL_INFO, -1, "Opal: Allocating shared memory. This should never happen\n");
				}

				setNextMemPool( node );
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
			if(4 == fault_level)
				nodeInfo[node]->regiserCR3Address(coreId, response.address, vAddress);
			else
				nodeInfo[node]->insertFrame(response.address, vAddress, fault_level, SST::OpalComponent::MemType::LOCAL);
			if(!response.status)
				output->fatal(CALL_INFO, -1, "Opal: Allocating local memory. This should never happen\n");
		}

		response.pages = pages;
		response.status = 1;
		setNextMemPool( node );
	}
	else {
		OPAL_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Node%" PRIu32 " Local Memory is drained out\n", node));

		//if(4 == fault_level)
		//	output->fatal(CALL_INFO, -1, "Opal: Allocating memory for CR3 failed\n");
		//else {
			setNextMemPool( node );
			response = allocateSharedMemory(node, coreId, vAddress, fault_level, pages);
		//}
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

	std::vector<uint64_t> *reserved_pAddress = mmapFileIdHints[fileID].second;

	//Allocate all the pages. TODO: pages can be reserved on demand instead of allocating all the pages at a time. But what if the memory is drained out.
	if(reserved_pAddress->empty()) {

		for(uint32_t i = 0; i<num_shared_mempools; i++) {

			// allocating all the pages from the same pool.
			/*
			 * TODO:
			 * 1. Is it a good idea to allocate pages from different memory pools?
			 * 2. Opal is not parallel for now, i.e, requests are addressed serially.
			 * 	  So just checking if the memory is available or not and allocating.
			 * 	  In future lock share memory pool and allocate pages if available.
			 *
			*/
			if( sharedMemoryInfo[i]->pool->available_frames >= pages ) {
				Pool *pool = sharedMemoryInfo[i]->pool;
				for(int j=0; j<pages_reserved; j++) {
					response = pool->allocate_frame(1);
					//nodeInfo[node]->insertFrame(response.address, vAddress, SST::OpalComponent::MemType::SHARED); // Not saving reserved frames in node information as these should not be migrated.
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
		output->fatal(CALL_INFO, -1, "Opal: address :%lu requested with fileId:%d has no space left\n", vAddress, fileID);
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
	//if(4 == fault_level)
	//	response = allocateLocalMemory(node, coreId, vAddress, fault_level, pages);
	//else
	//{

		// check if memory is to be allocated from the reserved address space
		//response = isAddressReserved(node, vAddress);

		//if( response.status )
		//	response = allocateFromReservedMemory(node, response.address, vAddress, pages);

		//else {

			if(ptw_aware_allocation) {
				if(ptw_aware_allocation == 1 && fault_level != 0) {
					//response = allocateLocalMemory(node, coreId, vAddress, fault_level, pages);
					if(nodeInfo[node]->pool->available_frames >= pages) {
						Pool *pool = nodeInfo[node]->pool;
						for(int i=0; i<pages; i++) {
							response = pool->allocate_frame(1);
							if(4 == fault_level)
								nodeInfo[node]->regiserCR3Address(coreId, response.address, vAddress);
							else
								nodeInfo[node]->insertFrame(response.address, vAddress, fault_level, SST::OpalComponent::MemType::LOCAL);
							if(!response.status)
								output->fatal(CALL_INFO, -1, "Opal: Allocating local memory. This should never happen\n");
						}

						response.pages = pages;
						response.status = 1;
					}
					else response = allocateSharedMemory(node, coreId, vAddress, fault_level, pages);
				}
				else if(ptw_aware_allocation == 2 && fault_level != 0) {
					//response = allocateLocalMemory(node, coreId, vAddress, fault_level, pages);
					if( sharedMemoryInfo[0]->pool->available_frames >= pages ) {
						Pool *pool = sharedMemoryInfo[0]->pool;
						for(int j=0; j<pages; j++) {
							response = pool->allocate_frame(1);
							nodeInfo[node]->insertFrame(response.address, vAddress, fault_level, SST::OpalComponent::MemType::SHARED);

							if(!response.status)
								output->fatal(CALL_INFO, -1, "Opal: Allocating shared memory. This should never happen\n");
						}

						response.pages = pages;
						response.status = 1;
					}
				}
			}
			if(!ptw_aware_allocation || fault_level == 0) {
				if( !nodeInfo[node]->allocatedmempool ) {
					response = allocateLocalMemory(node, coreId, vAddress, fault_level, pages);
				//std::cerr << getName() << " Node: " << node << " core " << coreId << " response page address: " << vAddress << " allocated local address: " << response.address << " pages: "<< pages << " level: " << fault_level  << std::endl;
				}
				else {
					response = allocateSharedMemory(node, coreId, vAddress, fault_level, pages);
				//std::cerr << getName() << " Node: " << node << " core " << coreId << " response page address: " << vAddress << " allocated shared address: " << response.address << " pages: " << " level: " << fault_level << std::endl;
				}
			}
		//}
	//}

	if( response.status ) {
		OpalEvent *tse = new OpalEvent(EventType::RESPONSE);
		tse->setResp(vAddress, response.address, response.pages*nodeInfo[node]->page_size);
		nodeInfo[node]->coreInfo[coreId].mmuLink->send(tse);
	}
	else
		output->fatal(CALL_INFO, -1, "Opal: Memory is drained out\n");

	return true;

}


// gathers shared memory page references
void Opal::globalPageReferences(int memPoolId) {
	/*
	while(!globalPageReferenceQ[memPoolId].empty()) {
		OpalEvent *ev = globalPageReferenceQ[memPoolId].front();

		if(ev->getType() == EventType::PAGE_REFERENCE_END)
		{
			nodeInfo[ev->getNodeId()]->global_memory_response_count++; // memory controller sends invalid node number to indicate end of sending page references
			globalPageReferenceQ[memPoolId].pop();
			delete ev;
			break;
		}
		else
		{
			nodeInfo[ev->getNodeId()]->registerGlobalPageReference(ev->getPaddress(), ev->getSize());
		}

		globalPageReferenceQ[memPoolId].pop();
		delete ev;
	}

	//std::cerr << " thread sm id: " << memPoolId << " existing" << std::endl;
	//pthread_exit(NULL);
	 */
}


// processes page placement related
void Opal::processPagePlacement()
{
	/*
	for(uint32_t n=0; n<num_nodes; n++)
	{
		if(!nodeInfo[n]->page_migration_in_progress)
		{
			// perform page migration for every epoch
			if(nodeInfo[n]->current_migration_epoch >= nodeInfo[n]->migration_epoch) {
				/* print page access stats
				//std::cerr << getName().c_str() << " node: " << n << " printing average: " << std::endl;
				OpalEvent *tse1 = new OpalEvent(EventType::PRINT_AVG_PAGE_ACCESS);
				tse1->setNodeId(0);
				nodeInfo[n]->memCntrlInfo->link->send(tse1);
				for(uint32_t m=0; m<num_shared_mempools; m++)
				{
					//std::cerr << getName().c_str() << " node: " << n << " shared memory: " << m << " printing average: " << std::endl;
					OpalEvent *tse = new OpalEvent(EventType::PRINT_AVG_PAGE_ACCESS);
					tse->setNodeId(n);
					sharedMemoryInfo[m]->link->send(tse);
				}

				if(page_placement)
				{
					/* get shared memory migration pages
					nodeInfo[n]->page_migration_in_progress = 1;
					// collect pages to be migrated information
					for(uint32_t m=0; m<num_shared_mempools; m++)
					{
						OpalEvent *tse = new OpalEvent(EventType::PAGE_REFERENCE);
						tse->setNodeId(n);
						sharedMemoryInfo[m]->link->send(tse);
					}
				}
				nodeInfo[n]->current_migration_epoch = 0;
			}
			nodeInfo[n]->current_migration_epoch++;
		}
		else
		{
			for(uint32_t s=0; s<num_shared_mempools; s++)
			{
				globalPageReferences(s);
			}

			if(nodeInfo[n]->global_memory_response_count == num_shared_mempools)	// check if received responses from all the shared memory pools
			{
				nodeInfo[n]->global_memory_response_count = 0;
				nodeInfo[n]->migratePages(); 	// perform page migration
			}

		}
	}
	*/
}



bool Opal::tick(SST::Cycle_t x)
{

	cycles++;

	//processPagePlacement(); // and print average page access count

	int inst_served = 0;
	while(!requestQ.empty()) {
		if(inst_served < max_inst) {
			OpalEvent *ev = requestQ.front();
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

			/*case SST::OpalComponent::EventType::SDACK:
			{
				OPAL_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Node%" PRIu32 " Opal has received a SDACK CALL\n", ev->getNodeId()));
				OpalEvent *tse = new OpalEvent(EventType::SDACK);
				nodeInfo[ev->getNodeId()]->coreInfo[ev->getCoreId()].coreLink->send(tse);	// resume core
				if(0 == ev->getCoreId()) {
					nodeInfo[ev->getNodeId()]->tlb_shootdown_in_progress = 0;
					nodeInfo[ev->getNodeId()]->page_migration_in_progress = 0;
					int thr_change = nodeInfo[ev->getNodeId()]->getUpdatedThreshold();
					for(uint32_t m=0; m<num_shared_mempools; m++)
					{
						if(thr_change != 0) {
							OpalEvent *tse = new OpalEvent(EventType::IPC_INFO);
							tse->setResp(0,0,thr_change); // embed threshold value in size
							tse->setNodeId(ev->getNodeId());
							sharedMemoryInfo[m]->link->send(tse);
						}
					}
				}
			}
			break;
			*/
			case SST::OpalComponent::EventType::REQUEST:
			{
				removeEvent = processRequest(ev->getNodeId(), ev->getCoreId(), ev->getAddress(), ev->getFaultLevel(), ev->getSize());
			}
			break;

			case SST::OpalComponent::EventType::ARIEL_ENABLED:
			{
				ariel_enable_count++;
				if(ariel_enable_count >= num_nodes) {
					for(uint32_t i=0; i<num_nodes; i++)
						for(uint32_t j=0; j<nodeInfo[i]->cores; j++) {
							OpalEvent *tse = new OpalEvent(EventType::ARIEL_ENABLED);
							nodeInfo[i]->coreInfo[j].coreLink->send(tse);
							std::cerr << "enabling core: " << j << std::endl;
						}
				}
				else {
					for(uint32_t j=0; j<nodeInfo[ev->getNodeId()]->cores; j++) {
						std::cerr << "disable core: " << j << std::endl;
						OpalEvent *tse = new OpalEvent(EventType::SHOOTDOWN);
						nodeInfo[ev->getNodeId()]->coreInfo[j].coreLink->send(tse);
					}
				}
			}
			break;

			default:
				output->fatal(CALL_INFO, -1, "%s, Error - Unknown request\n", getName().c_str());
				break;

			}

			if(!removeEvent) {
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


void Opal::finish()
{
	//page_placement=0;

	uint32_t i;

	for(i = 0; i < num_nodes; i++ )
	  nodeInfo[i]->pool->finish();


	for(i = 0; i < num_shared_mempools; i++ )
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


//void Opal::updateLocalPTR(uint64_t page, int ref, int node) {
	//std::cerr << getName().c_str() << " updateLocalPTR page: " << page << " ref: " << ref << std::endl;
	//nodeInfo[node]->local_PTR[page] = ref;
//}


void Opal::handleEvent(SST::Event* event)
{

}



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
	num_cores = 0;
	std::cerr << "Maximum Instructions per cycle: " << max_inst << std::endl;


	char* buffer = (char*) malloc(sizeof(char) * 256);

	/* Configuring shared memory */
	/*----------------------------------------------------------------------------------------*/

	num_shared_mempools = params.find<uint32_t>("shared_mempools", 0);
	std::cerr << "Number of Shared Memory Pools: "<< num_shared_mempools << endl;

	Params sharedMemParams = params.find_prefix_params("shared_mem.");
	shared_mem_size = 0;

	sharedMemoryInfo = new Pool*[num_shared_mempools];

	for(uint32_t i = 0; i < num_shared_mempools; i++) {
		memset(buffer, 0 , 256);
		sprintf(buffer, "mempool%" PRIu32 ".", i);
		Params memPoolParams = sharedMemParams.find_prefix_params(buffer);
		std::cerr << "Configuring Shared " << buffer << std::endl;
		sharedMemoryInfo[i] = new Pool(this, memPoolParams, SST::OpalComponent::MemType::SHARED, i);
		shared_mem_size += memPoolParams.find<uint64_t>("size", 0);
	}

	/* Configuring nodes */
	/*----------------------------------------------------------------------------------------*/

	int linksCount = 0;
	for(uint32_t i = 0; i < num_nodes; i++) {
		memset(buffer, 0 , 256);
		sprintf(buffer, "node%" PRIu32 ".", i);
		Params nodePrivateParams = params.find_prefix_params(buffer);
		nodeInfo[i] = new NodePrivateInfo(this, i, nodePrivateParams);
		for(uint32_t j=0; j<nodeInfo[i]->cores; j++) {
			memset(buffer, 0 , 256);
			sprintf(buffer, "requestLink%" PRIu32, linksCount + j*2);
			nodeInfo[i]->coreInfo[j].coreLink = configureLink(buffer, "1ns", new Event::Handler<CorePrivateInfo>((&nodeInfo[i]->coreInfo[j]), &CorePrivateInfo::handleRequest));
			memset(buffer, 0 , 256);
			sprintf(buffer, "requestLink%" PRIu32, linksCount + (j*2)+1);
			nodeInfo[i]->coreInfo[j].mmuLink = configureLink(buffer, "1ns", new Event::Handler<CorePrivateInfo>((&nodeInfo[i]->coreInfo[j]), &CorePrivateInfo::handleRequest));
			nodeInfo[i]->coreInfo[j].id = num_cores;
			num_cores++;
		}
		linksCount += nodeInfo[i]->cores*2;
	}

	free(buffer);

	statPagesMigrated = registerStatistic<uint64_t>( "num_of_pages_migrated" );

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

void Opal::processHint(int node, int fileId, uint64_t vAddress, int size)
{

	std::map<int, std::pair<std::list<int>*, std::list<uint64_t>* > >::iterator fileIdHint = mmapFileIdHints.find(fileId);

	//fileId is already registered by another node
	if( fileIdHint != mmapFileIdHints.end() )
	{
		//search for nodeId
		std::list<int>* it = (fileIdHint->second).first;
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
			nodeInfo[node]->reservedSpace.insert(std::make_pair(vAddress/4096, std::make_pair(fileId, std::make_pair( ceil(size/(nodeInfo[node]->page_size*1024)), 0))));
		}
	}
	else
	{
		std::list<int> *it = new std::list<int>;
		std::list<uint64_t> *pa = new std::list<uint64_t>;

		it->push_back(node);
		mmapFileIdHints.insert(std::make_pair(fileId, std::make_pair( it, pa )));
		nodeInfo[node]->reservedSpace.insert(std::make_pair(vAddress/4096, std::make_pair(fileId, std::make_pair( ceil(size/(nodeInfo[node]->page_size*1024)), 0))));

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
		if(reservedVAddress <= vAddress && vAddress < reservedVAddress + pages_reserved*nodeInfo[node]->page_size*1024) {
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

		/*
		 * TODO:
		 *    Opal is not parallel for now, i.e, requests are addressed serially.
		 * 	  So just checking if the memory is available or not and allocating.
		 * 	  In future lock share memory pool and allocate pages if available.
		 */
		if( sharedMemoryInfo[sharedMemPoolId]->available_frames >= pages ) {
			Pool *pool = sharedMemoryInfo[sharedMemPoolId];
			for(int j=0; j<pages; j++) {
				response = pool->allocate_frame(1);
				nodeInfo[node]->insertFrame(coreId, response.address, vAddress, fault_level, SST::OpalComponent::MemType::SHARED);

				if(!response.status)
					output->fatal(CALL_INFO, -1, "Opal: Allocating shared memory. This should never happen\n");
			}

			setNextMemPool( node );
			sharedMemoryInfo[sharedMemPoolId]->profileStats(0,pages);
			sharedMemoryInfo[sharedMemPoolId]->profileStats(1,pages);
			response.pages = pages;
			response.status = 1;
		}

		else{

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
				if( sharedMemoryInfo[sharedMemPoolId]->available_frames >= pages ) {
					Pool *pool = sharedMemoryInfo[sharedMemPoolId];
					for(int j=0; j<pages; j++) {
						response = pool->allocate_frame(1);
						nodeInfo[node]->insertFrame(coreId, response.address, vAddress, fault_level, SST::OpalComponent::MemType::SHARED);

						if(!response.status)
							output->fatal(CALL_INFO, -1, "Opal: Allocating shared memory. This should never happen\n");
					}

					setNextMemPool( node );
					sharedMemoryInfo[sharedMemPoolId]->profileStats(0,pages);
					sharedMemoryInfo[sharedMemPoolId]->profileStats(1,pages);
					response.pages = pages;
					response.status = 1;
					break;
				}
			}

			if(!response.status)
				output->fatal(CALL_INFO, -1, "Opal: Memory is drained out\n");

		}
	}

	else
	{

		for(uint32_t i = 0; i<num_shared_mempools; i++) {

			/*
			 * TODO:
			 *    Opal is not parallel for now, i.e, requests are addressed serially.
			 * 	  So just checking if the memory is available or not and allocating.
			 * 	  In future lock share memory pool and allocate pages if available.
			 */
			if( sharedMemoryInfo[i]->available_frames >= pages ) {

				Pool *pool = sharedMemoryInfo[i];
				for(int j=0; j<pages; j++) {
					response = pool->allocate_frame(1);
					nodeInfo[node]->insertFrame(coreId, response.address, vAddress, fault_level, SST::OpalComponent::MemType::SHARED);

					if(!response.status)
						output->fatal(CALL_INFO, -1, "Opal: Allocating shared memory. This should never happen\n");
				}

				sharedMemPoolId = i;
				sharedMemoryInfo[sharedMemPoolId]->profileStats(0,pages);
				sharedMemoryInfo[sharedMemPoolId]->profileStats(1,pages);
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
			nodeInfo[node]->insertFrame(coreId, response.address, vAddress, fault_level, SST::OpalComponent::MemType::LOCAL);
			if(!response.status)
				output->fatal(CALL_INFO, -1, "Opal: Allocating local memory. This should never happen\n");
		}

		pool->profileStats(0,1);
		pool->profileStats(1,1);
		response.pages = pages;
		response.status = 1;
		response.page_migration = 0;
		setNextMemPool( node );
	}
	else {
		OPAL_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Node%" PRIu32 " Local Memory is drained out\n", node));

		if((nodeInfo[node]->page_migration && !nodeInfo[node]->memoryAllocationPolicy) || 4 == fault_level) {
			if(4 == fault_level)
				std::cerr << getName().c_str() << " migrating a page to allocate memory for node " << node << " core " << coreId << " CR3" << std::endl;

			response.page_migration = 1;
		}
		else {
			setNextMemPool( node );
			response = allocateSharedMemory(node, coreId, vAddress, fault_level, pages);
			response.page_migration = 0;
		}
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

	std::list<uint64_t> *reserved_pAddress = mmapFileIdHints[fileID].second;

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
			if( sharedMemoryInfo[i]->available_frames >= pages ) {
				Pool *pool = sharedMemoryInfo[i];
				for(int j=0; j<pages_reserved; j++) {
					response = pool->allocate_frame(1);
					//nodeInfo[node]->insertFrame(coreId, response.address, vAddress, SST::OpalComponent::MemType::SHARED); // Not saving reserved frames in node information as these should not be migrated.
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
		output->fatal(CALL_INFO, -1, "Opal: address :%" PRIu64 "llu requested with fileId:%d has no space left\n", vAddress, fileID);
	}

	return response;
}

bool Opal::processRequest(int node, int coreId, uint64_t vAddress, int fault_level, int size)
{

	REQRESPONSE response;
	int pages = ceil(size/(nodeInfo[node]->page_size*1024));

	// If multiple pages are requested how are the physical addresses sent to the requester as in future sue to opal parallelization continuous addresses cannot be allocated
	if(pages != 1)
		output->fatal(CALL_INFO, -1, "Opal: currently opal does not support multiple page allocations\n");

	// if the page fault request is for CR3 register allocate the memory from local memory
	if(4 == fault_level)
	{
		response = allocateLocalMemory(node, coreId, vAddress, fault_level, pages);
		if(response.page_migration) {
			migratePages(node, coreId, 1);
			return false;
		}
	}
	else
	{

		// check if memory is to be allocated from the reserved address space
		response = isAddressReserved(node, vAddress);

		if( response.status )
			response = allocateFromReservedMemory(node, response.address, vAddress, pages);

		else {
			if( !nodeInfo[node]->allocatedmempool ) {
				response = allocateLocalMemory(node, coreId, vAddress, fault_level, pages);
				if(response.page_migration) {
					if(nodeInfo[node]->page_migration && 0==nodeInfo[node]->page_migration_policy && !nodeInfo[node]->memoryAllocationPolicy)
					{
						// Bulk page migrations
						migratePages(node, coreId, nodeInfo[node]->num_pages_to_migrate);
					}
					else if(nodeInfo[node]->page_migration && 1==nodeInfo[node]->page_migration_policy && !nodeInfo[node]->memoryAllocationPolicy)
					{
						// Only one page migration
						migratePages(node, coreId, 1);
					}
					else
						output->fatal(CALL_INFO, -1, "Opal: Page migration policy not detected. This should not happen\n");

					return false;
				}
				//std::cout << getName() << " Node: " << node << " core " << coreId << " vAddress: " << vAddress << " allocated local  address: " << response.address << " pages: " << " level: " << fault_level  << std::endl;
			}
			else {
				response = allocateSharedMemory(node, coreId, vAddress, fault_level, pages);
				//std::cout << getName() << " Node: " << node << " core " << coreId << " vAddress: " << vAddress << " allocated shared address: " << response.address << " pages: " << " level: " << fault_level << std::endl;
			}
		}
	}

	if( response.status ) {
		OpalEvent *tse = new OpalEvent(EventType::RESPONSE);
		tse->setResp(vAddress, response.address, response.pages*nodeInfo[node]->page_size*1024);
		nodeInfo[node]->coreInfo[coreId].mmuLink->send(nodeInfo[node]->latency, tse);
	}
	else
		output->fatal(CALL_INFO, -1, "Opal: Memory is drained out\n");

	return true;

}

void Opal::migratePages(int node, int coreId, int pages)
{

	REQRESPONSE response;
	response.status = 0;

	// local memory pages to migrate
	std::list<std::pair<uint64_t, std::pair<uint64_t, int> > > lm_pages = nodeInfo[node]->getPagesToMigrate(pages);

	// get shared memory pages
	int sharedMemPoolId = 0;
	std::list<uint64_t> sm_pages;
	for(uint32_t i = 0; i<num_shared_mempools; i++) {

		/*
		 * TODO:
		 *    Opal is not parallel for now, i.e, requests are addressed serially.
		 * 	  So just checking if the memory is available or not and allocating.
		 * 	  In future lock share memory pool and allocate pages if available.
		 */
		if( sharedMemoryInfo[i]->available_frames >= pages ) {
			Pool *pool = sharedMemoryInfo[i];
			for(int j = 0; j < pages; j++)
			{
				response = pool->allocate_frame(1);

				if(!response.status)
					output->fatal(CALL_INFO, -1, "Opal: Allocating shared memory. This should never happen\n");

				sm_pages.push_back(response.address);
				sharedMemoryInfo[i]->profileStats(0,1);
				sharedMemoryInfo[i]->profileStats(1,1);
			}

			sharedMemPoolId = i;
			response.pages = pages;
			response.status = 1;
			break;
		}

		if(!response.status)
			output->fatal(CALL_INFO, -1, "Opal: memory not available to migrate pages\n");

	}


	// error checking
	if( (uint32_t)lm_pages.size() != (uint32_t)pages && (uint32_t)sm_pages.size() != (uint32_t)pages)
		output->fatal(CALL_INFO, -1, "Opal: This should not happen\n");

	// swap
	uint64_t sm_address, lm_address;
	while(!lm_pages.empty())
	{
		auto it = lm_pages.front();

		sm_address = sm_pages.front();
		sm_pages.pop_front();

		lm_address = it.first;
		it.first = sm_address;

		// move the local page to shared memory
		nodeInfo[node]->insertFrame(coreId, sm_address, it.second.first, it.second.second, SST::OpalComponent::MemType::SHARED);

		// unmap local memory
		nodeInfo[node]->pool->deallocate_frame(lm_address,1);

		// store the address to invalidate
		nodeInfo[node]->coreInfo[coreId].addInvalidAddress(sm_address, it.second.first, it.second.second);

		lm_pages.pop_front();

		nodeInfo[node]->pool->profileStats(2,1);
		sharedMemoryInfo[sharedMemPoolId]->profileStats(2,1);

		statPagesMigrated->addData(1);

	}

	int shootdownId = nodeInfo[node]->coreInfo[coreId].id;
	// register tlb shootdown id
	tlbShootdownInfo[shootdownId] = std::make_pair(node, coreId);

	// initiate tlb shootdown
	tlbShootdown(node, coreId, shootdownId);

}

//TODO:: Can we send a list instead of sending one address at a time
void Opal::tlbShootdown(int node, int coreId, int shootdownId)
{

	std::list<std::pair<uint64_t, std::pair<uint64_t, int> > > *invalidAddrs = nodeInfo[node]->coreInfo[coreId].getInvalidAddresses();

	if(invalidAddrs->empty()) {
		std::cerr << getName().c_str() << " Nothing to invalidate" << std::endl;
		OpalEvent *tse = new OpalEvent(EventType::SDACK);
		nodeInfo[node]->coreInfo[coreId].coreLink->send(nodeInfo[node]->latency, tse);

	}

	// set how many cores does this shootdown should wait for
	//nodeInfo[node]->coreInfo[coreId].sdAckCount = num_nodes * num_cores;
	nodeInfo[node]->coreInfo[coreId].sdAckCount = nodeInfo[node]->cores;

	// stall cores
	//for(uint32_t n=0; n<num_nodes; n++)
		int n=node;
		for(uint32_t c=0; c<nodeInfo[n]->cores; c++) {
			OpalEvent *tse = new OpalEvent(EventType::SHOOTDOWN);
			nodeInfo[n]->coreInfo[c].coreLink->send(tse); // Stall all the cores
		}


	// send invalid addresses to core 0 1st as core 0 ptw will register the updated addresses.
	std::list<std::pair<uint64_t, std::pair<uint64_t, int> > > temp = *invalidAddrs;
	while(!temp.empty())
	{
		auto it = temp.front();
		OpalEvent *tse = new OpalEvent(EventType::INVALIDADDR);
		tse->setResp(it.second.first,it.first,0);
		tse->setFaultLevel(it.second.second);
		tse->setShootdownId(shootdownId);
		nodeInfo[n]->coreInfo[0].mmuLink->send(tse); // 1ns to update address
		temp.pop_front();
	}

	// send invalid addresses to core mmu's
	while(!invalidAddrs->empty())
	{
		auto it = invalidAddrs->front();
		//for(uint32_t n=0; n<num_nodes; n++)
			//int n=node;
			for(uint32_t c=1; c<nodeInfo[n]->cores; c++) {
				OpalEvent *tse = new OpalEvent(EventType::INVALIDADDR);
				tse->setResp(it.second.first,it.first,0);
				tse->setFaultLevel(it.second.second);
				tse->setShootdownId(shootdownId);
				nodeInfo[n]->coreInfo[c].mmuLink->send(tse); // 1ns to update address
			}

		invalidAddrs->pop_front();
	}


	// this include IPI + context switching latency with respect to number of cores
	// 15000 cycles for 2 cores
	int shootdown_latency = (nodeInfo[node]->cores/2)*(15000/nodeInfo[node]->clock)*1000; // in ns

	// initiate shootdown
	//for(uint32_t n=0; n<num_nodes; n++)
		//int n=node;
		for(uint32_t c=0; c<nodeInfo[n]->cores; c++) {
			OpalEvent *tse = new OpalEvent(EventType::SHOOTDOWN);
			tse->setShootdownId(shootdownId);
			nodeInfo[n]->coreInfo[c].mmuLink->send(shootdown_latency, tse);
		}

	nodeInfo[n]->pool->profileStats(3,1);
	nodeInfo[n]->pool->profileStats(4,(shootdown_latency/1000)); // in us

}

void Opal::processTLBShootdownAck(int node, int coreId, int shootdownId)
{
	std::map<int, std::pair<int, int> >::iterator it = tlbShootdownInfo.find(shootdownId);
	if( it == tlbShootdownInfo.end() )
		output->fatal(CALL_INFO, -1, "Opal: DANGER\n");

	int n=it->second.first;
	int c=it->second.second;
	nodeInfo[n]->coreInfo[c].sdAckCount--;
	if(nodeInfo[n]->coreInfo[c].sdAckCount < 0)
		std::cerr << getName().c_str() << " DANGER!! sdAckCount less than 0" << std::endl;

	OpalEvent *tse = new OpalEvent(EventType::SDACK);
	nodeInfo[node]->coreInfo[coreId].coreLink->send(nodeInfo[node]->latency, tse);

	/*if(nodeInfo[n]->coreInfo[c].sdAckCount != 0) {
		// should not resume initiated core
		// find out if ack is by initiated core or not
		if(nodeInfo[node]->coreInfo[coreId].id != shootdownId) {
			nodeInfo[node]->coreInfo[coreId].coreLink->send(nodeInfo[node]->latency, tse);
		}
	} else {
		// shootdown is performed by all the cores. resume all the cores
		// find out if the last acknowledged core is initiated core or not. If it is initiated core then resume only initiated core if not resume acknowledged core and initialized core
		nodeInfo[n]->coreInfo[c].coreLink->send(nodeInfo[n]->latency, tse);
		if(nodeInfo[node]->coreInfo[coreId].id != shootdownId) {
			nodeInfo[node]->coreInfo[coreId].coreLink->send(nodeInfo[node]->latency, tse);
		}
	}*/

}

void Opal::processInvalidAddrEvent(int node, int coreId, uint64_t vAddress)
{
	//store invalid address
	//nodeInfo[node]->coreInfo[coreId].invalidAddrs(vAddress);
}

void Opal::processShootdownEvent(int node, int coreId, uint64_t vaddress, uint64_t paddress, int fault_level)
{
	int shootdownId = nodeInfo[node]->coreInfo[coreId].id;

	// register tlb shootdown id
	tlbShootdownInfo[shootdownId] = std::make_pair(node, coreId);

	std::pair<uint64_t, int> temp = nodeInfo[node]->globalPageList[paddress];
	//std::cout << getName().c_str() << " Node: " << node << " core: " << coreId << " paddress: " << std::hex << paddress << " vaddress: " << temp.first << " and level: " << temp.second << " vaddress: " << vaddress << std::endl;
	if(temp.first != vaddress)
		output->fatal(CALL_INFO, -1, "%s, Error - unknown shootdown request\n", getName().c_str());

	std::list<std::pair<uint64_t, std::pair<uint64_t, int> > > lm_pages = nodeInfo[node]->getPagesToMigrate(1);

	if(lm_pages.size() > 1)
		output->fatal(CALL_INFO, -1, "%s, Error - shootdown request local memory migration pages more than 1\n", getName().c_str());


	auto it = lm_pages.front();

	// move the local page to shared memory
	nodeInfo[node]->removeFrame(paddress, SST::OpalComponent::MemType::SHARED);
	nodeInfo[node]->insertFrame(coreId, paddress, it.second.first, it.second.second, SST::OpalComponent::MemType::SHARED);

	nodeInfo[node]->insertFrame(coreId, it.first, vaddress, fault_level, SST::OpalComponent::MemType::LOCAL);

	// store the address to invalidate
	nodeInfo[node]->coreInfo[coreId].addInvalidAddress(paddress, it.second.first, it.second.second);
	nodeInfo[node]->coreInfo[coreId].addInvalidAddress(it.first, vaddress, fault_level);

	lm_pages.pop_front();

	//nodeInfo[node]->pool->profileStats(2,1);
	//sharedMemoryInfo[sharedMemPoolId]->profileStats(2,1);

	statPagesMigrated->addData(1);

	// initiate tlb shootdown
	tlbShootdown(node, coreId, shootdownId);

}

bool Opal::tick(SST::Cycle_t x)
{

	int inst_served = 0;

	while(!requestQ.empty()) {
		if(inst_served < max_inst) {

			OpalEvent *ev = requestQ.front();
			bool removeEvent = true;

			switch(ev->getType()) {
			case SST::OpalComponent::EventType::HINT:
			{
				//std::cerr << "MLM Hint(0) : level "<< ev->hint << " Starting address is "<< std::hex << ev->getAddress();
				//std::cerr << std::dec << " Size: "<< ev->getSize();
				//std::cerr << " Ending address is " << std::hex << ev->getAddress() + ev->getSize() - 1;
				//std::cerr << std::dec << std::endl;
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
				processHint(ev->getNodeId(), ev->getFileId(), ev->getAddress(), ev->getSize());
			}
			break;

			case SST::OpalComponent::EventType::UNMAP:
			{
				OPAL_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Node%" PRIu32 " Opal has received an UNMAP CALL\n", ev->getNodeId()));
			}
			break;

			case SST::OpalComponent::EventType::INVALIDADDR:
			{
				OPAL_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Node%" PRIu32 " Opal has received an INVALIDADDR CALL\n", ev->getNodeId()));
				processInvalidAddrEvent(ev->getNodeId(), ev->getCoreId(), ev->getAddress());
			}
			break;

			case SST::OpalComponent::EventType::SHOOTDOWN:
			{
				OPAL_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Node%" PRIu32 " Opal has received an SHOOTDOWN CALL\n", ev->getNodeId()));
				processShootdownEvent(ev->getNodeId(), ev->getCoreId(), ev->getAddress(), ev->getPaddress(), ev->getFaultLevel());
			}
			break;

			case SST::OpalComponent::EventType::SDACK:
			{
				OPAL_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Node%" PRIu32 " Opal has received an SDACK CALL\n", ev->getNodeId()));
				processTLBShootdownAck(ev->getNodeId(), ev->getCoreId(), ev->getShootdownId());
			}
			break;

			case SST::OpalComponent::EventType::REQUEST:
			{
				//size is 4096 (4K pages) from PTW
				removeEvent = processRequest(ev->getNodeId(), ev->getCoreId(), ev->getAddress(), ev->getFaultLevel(), ev->getSize());

			}
			break;

			default:
				output->fatal(CALL_INFO, -1, "%s, Error - Unknown request\n", getName().c_str());
				break;

			}

			if(!removeEvent) {
				return false;
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
	uint32_t i;

	for(i = 0; i < num_nodes; i++ )
	  nodeInfo[i]->pool->finish();


	for(i = 0; i < num_shared_mempools; i++ )
	  sharedMemoryInfo[i]->finish();

}


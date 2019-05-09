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


/* Author: Amro Awad
 * E-mail: amro.awad@ucf.edu
 */
/* Author: Vamsee Reddy Kommareddy
 * E-mail: vamseereddy@knights.ucf.edu
 */

#include <cstring>
#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <algorithm>

#include <stdio.h>
#include <stdint.h>
#include <poll.h>
#include <queue>

#include <sst/core/sst_types.h>
#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/interfaces/simpleMem.h>
#include <sst/core/output.h>
#include <sst/core/elementinfo.h>

#include "Opal_Event.h"
#include "mempool.h"

using namespace SST;

using namespace SST::OpalComponent;


namespace SST
{
	namespace OpalComponent
	{
		class MemoryPrivateInfo;
		class NodePrivateInfo;

		class Opal : public SST::Component
		{
			public:

				Opal( SST::ComponentId_t id, SST::Params& params); 

				void setup()  { };

				void finish();

				bool tick(SST::Cycle_t x);

				void setNextMemPool( int node );

				REQRESPONSE allocateLocalMemory(int node, int coreId, uint64_t vAddress, int fault_level, int pages);

				REQRESPONSE allocateSharedMemory(int node, int coreId, uint64_t vAddress, int fault_level, int pages);

				REQRESPONSE allocateFromReservedMemory(int node, uint64_t reserved_vAddress, uint64_t vAddress, int pages);

				REQRESPONSE isAddressReserved(int node, uint64_t vAddress);

				bool processRequest(int node, int coreId, uint64_t vAddress, int fault_level, int size);

				void processHint(int node, int fileId, uint64_t vAddress, int size);

				void processPagePlacement();

				void localPageReferences(int node);

				void globalPageReferences(int memPoolId);

				std::queue<OpalEvent*> requestQ; // stores page fault requests, hints and shootdown acknowledgement events from all the cores

				//std::queue<OpalEvent*> *localPageReferenceQ; // stores local memory page reference requests.

				std::vector<uint64_t> *localPageReferenceQ; // stores local memory page reference requests.

				std::queue<OpalEvent*> *globalPageReferenceQ; // stores global memory page reference requests.

				void deallocateSharedMemory(uint64_t page, int N);

				void updateLocalPTR(uint64_t page, int ref, int node);

				void handleEvent(SST::Event *);

				~Opal() {

/*
					for(uint32_t i=0; i<num_nodes; i++)
						delete[] nodeInfo[i];
					delete[] nodeInfo;

					for(uint32_t i=0; i<num_shared_mempools; i++)
						delete[] sharedMemoryInfo[i];
					delete[] sharedMemoryInfo;

					std::map<int, std::pair<std::vector<int>*, std::vector<uint64_t>* > >::iterator it;
					for(it=mmapFileIdHints.begin(); it!=mmapFileIdHints.end(); it++){
						delete (it->second).second;
						delete (it->second).first;
					}*/

				};

				SST_ELI_REGISTER_COMPONENT(
						Opal,
						"Opal",
						"Opal",
						SST_ELI_ELEMENT_VERSION(1,0,0),
						"Memory Allocation Manager",
						COMPONENT_CATEGORY_PROCESSOR
						)


					SST_ELI_DOCUMENT_PARAMS(
							{"clock", "Internal Controller Clock Rate.", "1.0 Ghz"},
							{"latency", "The time to be spent to service a memory request", "1000"},
							{"verbose", "debug level", "1"},
							{"max_inst", "maximum number of instructions per cycle", "1"},
							{"num_nodes", "number of disaggregated nodes in the system", "1"},
							{"cores_per_node", "total number of cores. this will be used to account for TLB shootdown latency", "1"},
							{"num_ports", "total number of request links", "2"},
							{"num_cores", "total number of core request links", "2"},
							{"num_memCntrls", "total number of loacl memory controller request links", "2"},
							{"num_globalMemCntrls", "total number of global memory controller request links", "2"},
							{"num_pools", "This determines the number of memory pools", "1"},
							{"num_domains", "The number of domains in the system, typically similar to number of sockets/SoCs", "1"},
							{"allocation_policy", "0 is private pools, then clustered pools, then public pools", "0"},
							{"shared_mempools", "This determines the number of shared memory pools", "1"},
							{"shared_mem.mempool%(shared_mempools).start", "the starting physical address of each shared memory pool in KBs", "0"},
							{"shared_mem.mempool%(shared_mempools).size", "Size of each shared memory pool in KBs", "1024"},
							{"shared_mem.mempool%(shared_mempools).frame_size", "Size of each shared memory pool in KBs", "4"},
							{"shared_mem.mempool%(shared_mempools).mem_tech", "memory technology of each shared memory pool in KBs", "0"},
							{"local_mem.mempool%(num_nodes).start", "the starting physical address of each local memory pool in KBs", "0"},
							{"local_mem.mempool%(num_nodes).size", "Size of each local memory pool in KBs", "1024"},
							{"local_mem.mempool%(num_nodes).frame_size", "frame size of each local memory pool in KBs", "4"},
							{"local_mem.mempool%(num_nodes).mem_tech", "memory technology of each local memory pool in KBs", "0"},
							{"startaddress%(num_pools)", "the starting physical address of the pool", "0"},
							{"type%(num_pools)", "0 means private for specific NUMA domain, 1 means shared among specific NUMA domains, 2 means public", "2"},
							{"cluster_size", "This determines the number of NUMA domains in each cluster, if clustering is used", "1"},
							{"memtype%(num_pools)", "0 for typical DRAM, 1 for die-stacked DRAM, 2 for NVM", "0"},
							{"typepriority%(num_pools)", "0 means die-stacked, typical DRAM, then NVM", "0"},
							)

					// Optional since there is nothing to document
					SST_ELI_DOCUMENT_STATISTICS(
							{ "local_mem_usage", "Number of pages allocated in local memory", "requests", 1},
							{ "shared_mem_usage", "Number of pages allocated in shared memory", "requests", 1},
							{ "local_mem_mapped", "Number of pages mapped in local memory", "requests", 1},
							{ "shared_mem_mapped", "Number of pages mapped in shared memory", "requests", 1},
							{ "local_mem_unmapped", "Number of pages unmapped while memory allocation in local memory", "requests", 1},
							{ "shared_mem_unmapped", "Number of pages unmapped while memory allocation in local memory", "requests", 1},
							{ "tlb_shootdowns", "Number of tlb shootdowns initiated per node", "requests", 1},
							{ "tlb_shootdown_delay_initiator", "Total tlb shootdown delays per node(initiating core)", "requests", 1},
							{ "tlb_shootdown_delay_slave", "Total tlb shootdown delays per node(slave core)", "requests", 1},
							{ "num_of_pages_migrated", "Number of pages migrated per node", "requests", 1},
							)

					SST_ELI_DOCUMENT_PORTS(
							{"coreLink%(num_cores)d", "Link to receive core requests", { "OpalComponent.OpalEvent", "" } },
							{"mmuLink%(num_cores)d", "Link to receive mmu requests", { "OpalComponent.OpalEvent", "" } },
							{"memCntrLink%(num_memCntrls)d", "Link to receive memory controller requests", { "OpalComponent.OpalEvent", "" } },
							{"globalMemCntrLink%(shared_mempools)d", "Link to receive shared memory controller requests", { "OpalComponent.OpalEvent", "" } },
							{"shootdownLink%(num_nodes*num_cores)d", "Link to send shootdown requests to Samba units", { "OpalComponent.OpalEvent", "" } },
							)

					// Optional since there is nothing to document
					SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
							)

			//public:

					Opal();  // for serialization only

					Opal(const Opal&); // do not implement

					void operator=(const Opal&); // do not implement

					uint64_t cycles;

					Output* output;

					int verbosity;

					SST::Link * self_link;

					long long int max_inst; //maximum instructions per cycle

					uint32_t ariel_enable_count;

					uint32_t num_nodes; // stores total number of nodes available in the system

					uint32_t num_cores; // stores total number of cores from all the nodes in the system

					uint32_t num_memCntrls; //

					MemoryPrivateInfo **sharedMemoryInfo; // stores private information of each shared memory pool

					uint64_t shared_mem_size; // stores total shared memory size

					uint32_t num_shared_mempools; // stores number of pools shared memory is divided into

					NodePrivateInfo **nodeInfo; // stores private information of each node

					//int page_placement; // used to enable/disable page placement

					int ptw_aware_allocation;

					std::map<int, std::pair<std::vector<int>*, std::vector<uint64_t>* > > mmapFileIdHints; // used to store reserved memory which is useful for inter-node communication


		};// END Opal

		class MemoryPrivateInfo
		{
			public:

				Opal *owner;

				int nodeId;

				int memContrlId;

				SST::Link * link;

				unsigned int latency;

				Pool* pool;

				MemoryPrivateInfo() { }

				MemoryPrivateInfo(Opal *_owner, uint32_t _id, Params params)
				{
					memContrlId = _id;
					owner = _owner;
					latency = (uint32_t) params.find<uint32_t>("latency", 1);
					pool = new Pool(owner, params, SST::OpalComponent::MemType::SHARED, _id);
				}

				~MemoryPrivateInfo() {
					delete pool;
				}

				void setOwner(Opal *owner_) { owner = owner_; }

				void handleRequest(SST::Event* e)
				{
					//std::cerr << " reposnse from memory controller: " << memContrlId << std::endl;
					OpalEvent *ev =  static_cast<OpalComponent::OpalEvent*> (e);
					ev->setMemContrlId(memContrlId);
					owner->globalPageReferenceQ[memContrlId].push(ev);
				}

				bool contains(uint64_t page)
				{
					return ((pool->start <= page) && (page < pool->start + pool->num_frames*pool->frsize)) ? true : false;
				}
		};

		class CorePrivateInfo
		{
			public:

				Opal *owner;

				int nodeId;

				int coreId;

				uint64_t cr3;

				SST::Link * coreLink;

				SST::Link * mmuLink;

				float ipc;

				CorePrivateInfo() { }

				~CorePrivateInfo() { }

				unsigned int latency;

				void setOwner(Opal *owner_) { owner = owner_; }

				void handleRequest(SST::Event* e)
				{
					OpalEvent *ev =  static_cast<OpalComponent::OpalEvent*> (e);
					//switch(ev->getType())
					//{
					//case SST::OpalComponent::EventType::PAGE_REFERENCE:
						//owner->updateLocalPTR(ev->getPaddress()/4096, ev->getSize(), nodeId);
						//std::cerr << owner->getName().c_str() << " node: " << nodeId << " core: "<< coreId << " clock request page address: " << ev->getPaddress() <<
						//		" type: " << ev->getType() << std::endl;
						//delete ev;
						//break;

					//case SST::OpalComponent::EventType::IPC_INFO:
						//ipc = (float)ev->getAddress()/ev->getPaddress(); // embedded instructions in vaddress, cycles in paddress
						//std::cerr << owner->getName().c_str() << " node: " << nodeId << " core: "<< coreId << " ipc: " << ipc << " instructions: " << ev->getAddress() << " cycles: " << ev->getPaddress() << std::endl;
						//break;

					//default:
						ev->setNodeId(nodeId);
						ev->setCoreId(coreId);
						owner->requestQ.push(ev);
					//}
				}

		};// END CorePrivateInfo

		class NodePrivateInfo
		{
			public:

				Opal *owner;

				uint32_t clock; // clock rate of Opal

				uint32_t node_num; // stores the node number of this node

				uint32_t cores; // stores number of cores in this node

				uint32_t memory_cntrls; // stores number of memory controllers available in this node

				uint32_t latency; // latency to communicate with Core, MMU and Memory controller units

				CorePrivateInfo *coreInfo; // stores core specific information of this node

				MemoryPrivateInfo *memCntrlInfo; // stores memory controller information of this node

				uint32_t memoryAllocationPolicy; // used for deciding memory allocation policies

				int nextallocmem; // stores next memory pool to allocate memory from

				int allocatedmempool; // used to store current allocated memory pool

				Pool* pool; // local memory pool which maintains memory utilization - allocated and free pages

				uint64_t page_size; // page size of the node in KB's

				uint64_t memory_size; // local memory size in terms of number of pages

				uint64_t num_pages; // number of pages in local memory

				uint32_t pages_available; // used to check number of pages free in local memory

				std::map<uint64_t, int> cr3AddressTable; // local page table with allocated frame and virtual address, fault level fields

				std::map<uint64_t, std::pair<uint64_t, int> > localPageTable; // local page table with allocated frame and virtual address, fault level fields

				std::map<uint64_t, std::pair<uint64_t, int> > globalPageTable; // global page table with allocated frame and virtual address, fault level fields

				std::map<uint64_t, std::pair<int, std::pair<int, int> > > reservedSpace; // stores pages that are reserved by nodes. these can be shared by other nodes for inter-node communication. fileds: virtual address, fileId, size

				int page_placement; // used to enable page placement

				int page_placement_policy; // used to decide page placement policy

				//int num_pages_to_migrate; // used to decide number of pages to migrate

				int eviction_policy; // to decide least recently used local pages

				int tlb_shootdown_in_progress; // when tlb shootdown is in progress do not update page references

				int page_migration_in_progress;	// when tlb shootdown is in progress do not process anything

				int global_memory_response_count;	// response count from shared memory controllers

				//std::map<uint64_t, int> local_PTR; // local memory page references used in clock replacement policy

				//uint64_t local_PTR_index;

				//std::map<uint64_t, int>::iterator local_memory_page_reference_pointer; // local memory page reference pointer for clock replacement policy

				//std::map<uint64_t, int> most_frequently_used_local_pages; // most frequently accessed local pages used for page placement

				//std::map<uint64_t, int> least_frequently_used_local_pages; // least frequently accessed local pages used for page placement

				//std::vector<uint64_t> least_frequently_used_local_pages; // most frequently accessed global pages used for page placement

				//std::vector<uint64_t> most_frequently_used_global_pages; // most frequently accessed global pages used for page placement

				//std::map<uint64_t, int> least_frequently_used_global_pages; // least frequently accessed global pages used for page placement

				//std::map<uint64_t, std::pair<uint64_t,int> > invalid_addrs; // list of addresses to be invalidated

				uint64_t shootdown_latency; // TLB shootdown latency in ns

				//uint64_t current_local_page_ref_epoch; // stores current epoch to get page references from each node

				//uint64_t local_page_ref_epoch; // stores time interval at which local memory references are collected

				//uint64_t current_migration_epoch; // stores current epoch which is used to perform get migrations

				//uint64_t migration_epoch; // stores time interval at which page migration is initiated

				//int local_migrating_pages_available;

				//std::vector<uint64_t> localPageReferenceQ;

				/* dynamic page migration parameters */
				//float ipc_previous;
				//float ipc_current;
				//int ipc_init;
				/* end of dynamic page migration parameters */

				Statistic<uint64_t>* statLocalMemUsage;
				Statistic<uint64_t>* statSharedMemUsage;
				Statistic<uint64_t>* statLocalMemUnmapped;
				Statistic<uint64_t>* statSharedMemUnmapped;
				Statistic<uint64_t>* statNumPagesMigrated;
				Statistic<uint64_t>* statNumTlbShootdowns;
				Statistic<uint64_t>* statTlbShootdownDelayInitiator;
				Statistic<uint64_t>* statTlbShootdownDelaySlave;

				NodePrivateInfo(Opal *_owner, uint32_t node, Params params)
				{
					owner = _owner;
					node_num = node;
					clock = (uint32_t) params.find<uint32_t>("clock", 2000); // in MHz
					cores = (uint32_t) params.find<uint32_t>("cores", 1);
					memory_cntrls = (uint32_t) params.find<uint32_t>("memory_cntrls", 1);
					latency = (uint32_t) params.find<uint32_t>("latency", 2000);	//2us

					memoryAllocationPolicy = (uint32_t) params.find<uint32_t>("allocation_policy", 0);
					nextallocmem = 0;
					allocatedmempool = 0;

					pool = new Pool(owner, (Params) params.find_prefix_params("memory."), SST::OpalComponent::MemType::LOCAL, node);
					memory_size = (uint32_t) params.find<uint32_t>("memory.size", 1);	// in KB's
					page_size = (uint32_t) params.find<uint32_t>("memory.frame_size", 4);
					page_size = page_size * 1024;

					page_placement = (uint32_t) params.find<uint32_t>("page_placement", 0);
					page_placement_policy = (uint32_t) params.find<uint32_t>("page_placement_policy", 0);
					//num_pages_to_migrate = (uint32_t) params.find<uint32_t>("num_pages_to_migrate", 50); // 50 pages by default
					eviction_policy = 0;
					tlb_shootdown_in_progress = 0;
					page_migration_in_progress = 0;
					global_memory_response_count = 0;
					//current_local_page_ref_epoch = 0;
					//current_migration_epoch = 0;
					//local_migrating_pages_available = 0;

					std::cerr << "Node: " << node_num << " Allocation policy: " << memoryAllocationPolicy << std::endl;

					coreInfo = new CorePrivateInfo[cores];
					for(uint32_t i=0; i<cores; i++) {
						coreInfo[i].latency = latency;
						coreInfo[i].nodeId = node_num;
						coreInfo[i].coreId = i;
						coreInfo[i].owner = owner;
					}

					memCntrlInfo = new MemoryPrivateInfo[memory_cntrls];
					for(uint32_t i=0; i<memory_cntrls; i++) {
						memCntrlInfo[i].latency = latency;
						memCntrlInfo[i].nodeId = node_num;
						memCntrlInfo[i].memContrlId = i;
						memCntrlInfo[i].owner = owner;
					}

					num_pages = ceil((memory_size *1024)/page_size);
					//for(uint64_t page=0; page<num_pages; page++) {
					//	local_PTR[page] = 0;
						//least_frequently_used_local_pages[page] = 0; // initially none of the pages are accessed
					//}
					//local_PTR_index = 1;

					//ipc_previous = 0;
					//ipc_current = 0;
					//ipc_init = 0;

					char* subID = (char*) malloc(sizeof(char) * 32);
					sprintf(subID, "%" PRIu32, node_num);

					statLocalMemUsage = owner->registerStatistic<uint64_t>("local_mem_usage", subID );
					statSharedMemUsage = owner->registerStatistic<uint64_t>("shared_mem_usage", subID );
					statLocalMemUnmapped = owner->registerStatistic<uint64_t>("local_mem_unmapped", subID );
					statSharedMemUnmapped = owner->registerStatistic<uint64_t>("shared_mem_unmapped", subID );
					statNumTlbShootdowns = owner->registerStatistic<uint64_t>("tlb_shootdowns", subID);
					statTlbShootdownDelayInitiator = owner->registerStatistic<uint64_t>("tlb_shootdown_delay_initiator", subID);
					statTlbShootdownDelaySlave = owner->registerStatistic<uint64_t>("tlb_shootdown_delay_slave", subID);
					statNumPagesMigrated = owner->registerStatistic<uint64_t>("num_of_pages_migrated", subID);

				}

				~NodePrivateInfo() {
				//	delete coreInfo;
				//	delete memCntrlInfo;
				//	delete pool;
				}


				void regiserCR3Address(int coreId, uint64_t pAddress, uint64_t vAddress) {
					coreInfo[coreId].cr3 = pAddress;
					cr3AddressTable[pAddress] = 0;
					//std::cerr << owner->getName().c_str() << " core: " << coreId << " inserting cr3 page: " << pAddress << " vaddress: " << vAddress << std::endl;
				}

				void insertFrame(uint64_t pAddress, uint64_t vAddress, int fault_level, SST::OpalComponent::MemType memType)
				{
					if( memType == SST::OpalComponent::MemType::LOCAL ) {
						//local_PTR[pAddress/4096] = 1;
						localPageTable[pAddress] = std::make_pair(vAddress,fault_level);
						//std::cerr << owner->getName().c_str() << " inserting local page: " << pAddress << " vaddress: " << vAddress << " fault_level: " << fault_level << std::endl;
						statLocalMemUsage->addData(1);
					}
					else{
						globalPageTable[pAddress] = std::make_pair(vAddress,fault_level);
						//std::cerr << owner->getName().c_str() << " inserting global page: " << pAddress << " vaddress: " << vAddress << " fault_level: " << fault_level << std::endl;
						statSharedMemUsage->addData(1);
					}
				}

				void removeFrame(uint64_t pAddress, SST::OpalComponent::MemType memType)
				{
					if( memType == SST::OpalComponent::MemType::LOCAL ) {
						//local_PTR[pAddress/4096] = 0;
						localPageTable.erase(pAddress);
						//std::cerr << owner->getName().c_str() << " removing local page: " << pAddress << std::endl;
						statLocalMemUnmapped->addData(1);
					}
					else{
						globalPageTable.erase(pAddress);
						//std::cerr << owner->getName().c_str() << " removing global page: " << pAddress << std::endl;
						statSharedMemUnmapped->addData(1);
					}
				}


				void registerGlobalPageReference(uint64_t page, uint64_t reference)
				{/*
					most_frequently_used_global_pages.push_back(page);
					//std::cerr << " node: " << node_num << " most frequently used global page: " << page << " count: " << reference << std::endl;
				*/
				}

				uint64_t getLocalPageToMigrate() {
					/*
					uint64_t page;
					int found = 0;
					for(int i=0; i<100; i++) { // number of checks to get a victim pages
						//std::cerr << " local_PTR page: " << (local_PTR_index)*4096 << " reference: " << local_PTR[local_PTR_index] << " PTR pointer: "<< local_PTR_index << std::endl;
						if(local_PTR[local_PTR_index] == 0 && cr3AddressTable.find(local_PTR_index*4096) == cr3AddressTable.end()) {
							page = local_PTR_index;
							local_PTR_index = (local_PTR_index+1)%(num_pages) == 0 ? 1 : (local_PTR_index+1)%(num_pages);
							found = 1;
							break;
						}
						else
							local_PTR[local_PTR_index] = 0;

						local_PTR_index = (local_PTR_index+1)%(num_pages) == 0 ? 1 : (local_PTR_index+1)%(num_pages);
					}

					while(!found) {
						//std::cerr << " local_PTR page: " << (local_PTR_index)*4096 << " reference: " << local_PTR[local_PTR_index] << " PTR pointer: "<< local_PTR_index << std::endl;
						if(cr3AddressTable.find(local_PTR_index*4096) == cr3AddressTable.end()) {
							local_PTR[local_PTR_index] = 0;
							page = local_PTR_index;
							local_PTR_index = (local_PTR_index+1)%(num_pages) == 0 ? 1 : (local_PTR_index+1)%(num_pages);
							found = 1;
							break;
						}
						local_PTR_index = (local_PTR_index+1)%(num_pages) == 0 ? 1 : (local_PTR_index+1)%(num_pages);
					}

					//std::cerr << " victim page: " << (page)*4096 << std::endl;
					return (page)*4096;
					*/
				}

				void printPTR() {
					/*
					for(int i=0; i<num_pages; i++){
						std::cerr << " PTR index: " << i << " page: " << (i)*4096 << " ref: " << local_PTR[i] << " pointer: " << local_PTR_index << std::endl;
					}
					*/
				}

				void migratePages()
				{
					/*
					if(most_frequently_used_global_pages.empty()) {
						page_migration_in_progress = 0;
						return;
					}

					//printPTR();
					int num_pages_to_migrate = most_frequently_used_global_pages.size();

					int pages_migrated = 0;
					for(int i=0; i<num_pages_to_migrate; i++)
					{
						uint64_t sm_paddress = most_frequently_used_global_pages[i];
						if(globalPageTable.find(sm_paddress) == globalPageTable.end())
							continue;
						std::pair<uint64_t,int> sm = globalPageTable[sm_paddress];

						uint64_t lm_paddress = getLocalPageToMigrate();

						if(localPageTable.find(lm_paddress) == localPageTable.end()) {
							//std::cerr << owner->getName().c_str() << " not found: " << lm_paddress << std::endl;
							REQRESPONSE response = pool->allocate_frame(1);
							if(response.address != lm_paddress)
								std::cerr << owner->getName().c_str() << " allocated frame: "<< response.address << " not equal : " << lm_paddress << std::endl;
							insertFrame(response.address, sm.first, sm.second, SST::OpalComponent::MemType::LOCAL);

							removeFrame(sm_paddress, SST::OpalComponent::MemType::SHARED);

							lm_paddress = response.address;
							invalid_addrs[lm_paddress] = std::make_pair(sm.first, sm.second);
							pages_migrated++;
						} else {
							std::pair<uint64_t,int> lm = localPageTable[lm_paddress];

							localPageTable[lm_paddress] = std::make_pair(sm.first, sm.second);
							globalPageTable[sm_paddress] = std::make_pair(lm.first, lm.second);

							invalid_addrs[lm_paddress] = std::make_pair(sm.first, sm.second);
							invalid_addrs[sm_paddress] = std::make_pair(lm.first, lm.second);
							pages_migrated++;
						}
					}

					most_frequently_used_global_pages.clear();

					if(pages_migrated)
						initiateTlbShootdown();
					*/
				}

				void initiateTlbShootdown()
				{
					/*
					//std::cerr << "node: " << node_num << " invalidating addresses " << invalid_addrs.size() << std::endl;

					// stall cores
					for(uint32_t c=0; c<cores; c++) {
						OpalEvent *tse = new OpalEvent(EventType::SHOOTDOWN);
						coreInfo[c].coreLink->send(tse);
					}

					std::map<uint64_t, std::pair<uint64_t,int> >::iterator it = invalid_addrs.begin();
					for(; it != invalid_addrs.end(); it++)
					{
						uint64_t paddress = it->first;
						statNumPagesMigrated->addData(1);
						//std::cerr << "node: " << node_num << " invalid paddress: " << paddress << " vaddress: " << it->second.first << " level: " << it->second.second << std::endl;
						for(uint32_t c=0; c<cores; c++) {
							OpalEvent *tse = new OpalEvent(EventType::REMAP);
							tse->setResp(it->second.first,paddress,0);
							tse->setFaultLevel(it->second.second);
							coreInfo[c].mmuLink->send(tse);
						}
					}
					invalid_addrs.clear();

					for(uint32_t c=0; c<cores; c++) {
						OpalEvent *tse = new OpalEvent(EventType::SHOOTDOWN);
						tse->setResp(0, 0, 0);
						coreInfo[c].mmuLink->send(100, tse);
						//std::cerr << "node: " << node_num << " core: " << c << " sending shootdown event" << std::endl;
					}

					tlb_shootdown_in_progress = 1;
					*/
				}

				int getUpdatedThreshold()
				{
					/*
					float ipc = 0;
					int thr_change = 0;
					for(int i=0; i<cores; i++)
						ipc = ipc + coreInfo[i].ipc;

					ipc = ipc/cores;
					//std::cerr << owner->getName().c_str() << " node: " << node_num << " ipc: " << ipc << " ipc_previous: " << ipc_previous << " ipc_current: " << ipc_current << std::endl;
					if(ipc_init > 2)
					{
						if(ipc < ipc_previous)
							thr_change = -10;
						else if(ipc > ipc_previous)
							thr_change = 10;
						else
							thr_change = 0;
					}
					else
						ipc_init++;

					ipc_previous = ipc_current;
					ipc_current = ipc;

					//std::cerr << owner->getName().c_str() << " node: " << node_num << " ipc: " << ipc << " ipc_previous: " << ipc_previous << " ipc_current: " << ipc_current << " thr_change: " << thr_change << std::endl;
					return thr_change;
					*/
				}


		};


	}// END OpalComponent
}//END SST





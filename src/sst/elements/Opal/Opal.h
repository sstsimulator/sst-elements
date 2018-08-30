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
				void processShootdownEvent(int node, int coreId, uint64_t vaddress, uint64_t paddress, int fault_level);
				void processInvalidAddrEvent(int node, int coreId, uint64_t vaddress);
				void processTLBShootdownAck(int node, int coreId, int shootdownId);
				void tlbShootdown(int node, int coreId, int shootdownId);
				void migratePages(int node, int coreId, int pages);

				std::queue<OpalEvent*> requestQ;

				~Opal() { };
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
							{ "tlb_shootdowns", "Number of tlb shootdowns initiated in a node", "requests", 1},
							{ "tlb_shootdown_delay", "Total tlb shootdown delays in a node(initiating core)", "requests", 1},
							{ "num_of_pages_migrated", "Number of pages migrated", "requests", 1},
							)

					SST_ELI_DOCUMENT_PORTS(
							{"requestLink%(num_ports)d", "Link to receive allocation requests", { "OpalComponent.OpalEvent", "" } },
							{"shootdownLink%(num_nodes*num_cores)d", "Link to send shootdown requests to Samba units", { "OpalComponent.OpalEvent", "" } },
							)

					// Optional since there is nothing to document
					SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
							)

			private:
					Opal();  // for serialization only
					Opal(const Opal&); // do not implement
					void operator=(const Opal&); // do not implement

					uint32_t num_nodes;
					uint32_t num_cores;

					//shared memory - memory pool number and its pool
					//std::map<int, Pool*> shared_mem;
					Pool **sharedMemoryInfo;
					uint64_t shared_mem_size;
					uint32_t num_shared_mempools;

					//node specific information
					NodePrivateInfo **nodeInfo;

					//TLB shootdown information
					std::map<int, std::pair<int, int> > tlbShootdownInfo;

					//reserved memory to communicate
					std::map<int, std::pair<std::list<int>*, std::list<uint64_t>* > > mmapFileIdHints;

					long long int max_inst;
					char* named_pipe;
					int* pipe_id;
					std::string user_binary;
					Output* output;
					int verbosity;

					Statistic<uint64_t>* statReadRequests;
					Statistic<uint64_t>* statWriteRequests;
					Statistic<uint64_t>* statAvgTime;
					Statistic<uint64_t>* statPagesMigrated;


		};// END Opal

		class CorePrivateInfo
		{
			public:

				int id;
				int nodeId;
				int coreId;
				SST::Link * coreLink;
				SST::Link * mmuLink;
				int dummy_address;
				CorePrivateInfo() { dummy_address = 0;}
				unsigned int latency;
				Opal *owner;
				void setOwner(Opal *owner_) { owner = owner_; }

				void handleRequest(SST::Event* e)
				{
					OpalEvent *ev =  static_cast<OpalComponent::OpalEvent*> (e);
					ev->setNodeId(nodeId);
					ev->setCoreId(coreId);
					owner->requestQ.push(ev);

				}

				// number of shootdown acknowledgments required
				uint32_t sdAckCount;

				// list of addresses to be invalidated
				std::list<std::pair<uint64_t, std::pair<uint64_t, int> > > invalidAddrs;

				void addInvalidAddress(uint64_t pAddress, uint64_t vAddress, int fault_level) { invalidAddrs.push_back(std::make_pair(pAddress,std::make_pair(vAddress,fault_level))); }
				void setInvalidAddresses (std::list<std::pair<uint64_t, std::pair<uint64_t, int> > > ia) { invalidAddrs = ia; }
				std::list<std::pair<uint64_t, std::pair<uint64_t, int> > > * getInvalidAddresses() { return &invalidAddrs; }

				uint64_t cr3;

		};// END CorePrivateInfo

		class NodePrivateInfo
		{
			public:
				NodePrivateInfo(Opal *_owner, uint32_t node, Params params) {
					owner = _owner;
					node_num = node;
					cores = (uint32_t) params.find<uint32_t>("cores", 1);
					clock = (uint32_t) params.find<uint32_t>("clock", 2000); // in MHz
					latency = (uint32_t) params.find<uint32_t>("latency", 1);
					memoryAllocationPolicy = (uint32_t) params.find<uint32_t>("allocation_policy", 0);
					page_migration = (uint32_t) params.find<uint32_t>("page_migration", 0);
					page_migration_policy = (uint32_t) params.find<uint32_t>("page_migration_policy", 0);
					num_pages_to_migrate = (uint32_t) params.find<uint32_t>("num_pages_to_migrate", 0);
					nextallocmem = 0;
					allocatedmempool = 0;
					pool = new Pool(owner, (Params) params.find_prefix_params("memory."), SST::OpalComponent::MemType::LOCAL, node);
					memory_size = (uint32_t) params.find<uint32_t>("memory.size", 1);
					page_size = (uint32_t) params.find<uint32_t>("memory.frame_size", 4);
					coreInfo = new CorePrivateInfo[cores];

					std::cerr << "Node: " << node_num << " Allocation policy: " << memoryAllocationPolicy << std::endl;

					for(uint32_t i=0; i<cores; i++) {
						coreInfo[i].latency = latency;
						coreInfo[i].nodeId = node_num;
						coreInfo[i].coreId = i;
						coreInfo[i].owner = owner;
					}

				}

				Opal *owner;
				uint32_t node_num;
				uint32_t cores;
				uint32_t clock;
				uint32_t latency;

				/* core specific information*/
				CorePrivateInfo *coreInfo;

				/* allocation policies */
				uint32_t memoryAllocationPolicy;
				int nextallocmem;
				int allocatedmempool;

				/*page migration information*/
				int page_migration;
				int page_migration_policy;
				int num_pages_to_migrate;

				/* local memory */
				Pool* pool;
				uint32_t page_size; // page size of the node in KB's
				uint32_t memory_size; // in pages
				uint32_t pages_available;
				std::map<uint64_t, std::pair<uint64_t, int> > localPageList; // allocated frame and virtual address, fault level

				//shared memory info
				std::map<uint64_t, std::pair<uint64_t, int> > globalPageList;

				//virtual address, fileId, size
				std::map<uint64_t, std::pair<int, std::pair<int, int> > > reservedSpace;



				void insertFrame(int coreId, uint64_t pAddress, uint64_t vAddress, int fault_level, SST::OpalComponent::MemType memType) {
					if(4==fault_level)
						coreInfo[coreId].cr3 = pAddress;
					else if( memType == SST::OpalComponent::MemType::LOCAL ) {
						localPageList[pAddress] = std::make_pair(vAddress,fault_level);
					}
					else if( memType == SST::OpalComponent::MemType::SHARED ) {
						globalPageList[pAddress] = std::make_pair(vAddress,fault_level);
					}
					else
						std::cout << "Opal: insert frame Error!!!!"  <<std::endl;

				}

				void removeFrame(uint64_t pAddress, SST::OpalComponent::MemType memType) {
					if( memType == SST::OpalComponent::MemType::LOCAL ) {
						localPageList.erase(pAddress);
					}
					else if( memType == SST::OpalComponent::MemType::SHARED ) {
						globalPageList.erase(pAddress);
					}
					else
						std::cout << "Opal: insert frame Error!!!!"  <<std::endl;
				}

				// choose pages to migrate randomly
				std::list<std::pair<uint64_t, std::pair<uint64_t, int> > > getPagesToMigrate(int pages) {
					std::list<std::pair<uint64_t, std::pair<uint64_t, int> > > migrate_pages;
					for(int i=0; i<pages; i++) {
						auto it = localPageList.begin();
						std::advance(it, rand() % localPageList.size());
						migrate_pages.push_back(std::make_pair(it->first, it->second));
						localPageList.erase(it->first);
					}

					return migrate_pages;
				}

				// choose a single page to migrate randomly
				std::pair<uint64_t, std::pair<uint64_t, int> > getPageToMigrate() {
					std::pair<uint64_t, std::pair<uint64_t, int> > migrate_page;
					auto it = localPageList.begin();
					std::advance(it, rand() % localPageList.size());
					migrate_page = std::make_pair(it->first, it->second);
					localPageList.erase(it->first);
					return migrate_page;
				}

		};


	}// END OpalComponent
}//END SST



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

#include "Opal_Event.h"
#include "mempool.h"

using namespace SST;

using namespace SST::OpalComponent;


namespace SST
{
	namespace OpalComponent
	{

		class OpalBase {

			public:
				OpalBase() { }

				~OpalBase() {

					while( !requestQ.empty() ) {
						delete requestQ.front();
						requestQ.pop();
					}

					std::map<int, std::pair<std::vector<int>*, std::vector<uint64_t>* > >::iterator it;
					for(it=mmapFileIdHints.begin(); it!=mmapFileIdHints.end(); it++){
						delete (it->second).second;
						delete (it->second).first;
					}
				}

				std::queue<OpalEvent*> requestQ; // stores page fault requests, hints and shootdown acknowledgement events from all the cores

				std::map<int, std::pair<std::vector<int>*, std::vector<uint64_t>* > > mmapFileIdHints; // used to store reserved memory which is useful for inter-node communication
		};

		class MemoryPrivateInfo
		{
			public:

				OpalBase *opalBase;

				int nodeId;

				int memContrlId;

				SST::Link * link;

				unsigned int latency;

				Pool* pool;

				MemoryPrivateInfo() { }

				MemoryPrivateInfo(OpalBase *base, uint32_t _id, Params params)
				{
					memContrlId = _id;
					opalBase = base;
					latency = (uint32_t) params.find<uint32_t>("latency", 1);
					pool = new Pool(params, SST::OpalComponent::MemType::SHARED, _id);
				}

				~MemoryPrivateInfo() {
					delete pool;
				}

				void setOwner(OpalBase *base) { opalBase = base; }

				void handleRequest(SST::Event* e)
				{
					OpalEvent *ev =  static_cast<OpalComponent::OpalEvent*> (e);
					ev->setMemContrlId(memContrlId);
					delete ev;	// delete event from memory controller for now
				}

				bool contains(uint64_t page)
				{
					return ((pool->start <= page) && (page < pool->start + pool->num_frames*pool->frsize)) ? true : false;
				}
		};

		class CorePrivateInfo
		{
			public:

				OpalBase *opalBase;

				int nodeId;

				int coreId;

				uint64_t cr3;

				SST::Link * coreLink;

				SST::Link * mmuLink;

				float ipc;

				CorePrivateInfo() { }

				~CorePrivateInfo() { }

				unsigned int latency;

				void setOwner(OpalBase *base) { opalBase = base; }

				void handleRequest(SST::Event* e)
				{
					OpalEvent *ev =  static_cast<OpalComponent::OpalEvent*> (e);
					ev->setNodeId(nodeId);
					ev->setCoreId(coreId);
					opalBase->requestQ.push(ev);
				}

		};

		class NodePrivateInfo
		{
			public:

				OpalBase *opalBase;

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

				std::map<uint64_t, std::pair<int, std::pair<int, int> > > reservedSpace; // stores pages that are reserved by nodes. these can be shared by other nodes for inter-node communication. fileds: virtual address, fileId, size

				Statistic<uint64_t>* statLocalMemUsage;
				Statistic<uint64_t>* statSharedMemUsage;

				NodePrivateInfo(OpalBase *base, uint32_t node, Params params)
				{
					opalBase = base;
					node_num = node;
					clock = (uint32_t) params.find<uint32_t>("clock", 2000); // in MHz
					cores = (uint32_t) params.find<uint32_t>("cores", 1);
					memory_cntrls = (uint32_t) params.find<uint32_t>("memory_cntrls", 1);
					latency = (uint32_t) params.find<uint32_t>("latency", 2000);	//2us

					memoryAllocationPolicy = (uint32_t) params.find<uint32_t>("allocation_policy", 0);
					nextallocmem = 0;
					allocatedmempool = 0;

					pool = new Pool((Params) params.get_scoped_params("memory"), SST::OpalComponent::MemType::LOCAL, node);
					memory_size = (uint32_t) params.find<uint32_t>("memory.size", 1);	// in KB's
					page_size = (uint32_t) params.find<uint32_t>("memory.frame_size", 4);
					page_size = page_size * 1024;

					coreInfo = new CorePrivateInfo[cores];
					for(uint32_t i=0; i<cores; i++) {
						coreInfo[i].latency = latency;
						coreInfo[i].nodeId = node_num;
						coreInfo[i].coreId = i;
						coreInfo[i].opalBase = opalBase;
					}

					memCntrlInfo = new MemoryPrivateInfo[memory_cntrls];
					for(uint32_t i=0; i<memory_cntrls; i++) {
						memCntrlInfo[i].latency = latency;
						memCntrlInfo[i].nodeId = node_num;
						memCntrlInfo[i].memContrlId = i;
						memCntrlInfo[i].opalBase = opalBase;
						memCntrlInfo[i].setOwner(opalBase);
						memCntrlInfo[i].pool = pool;
					}

					num_pages = ceil((memory_size *1024)/page_size);

				}

				~NodePrivateInfo() {
					delete[] coreInfo;
					delete[] memCntrlInfo;
				}

				void profileEvent(SST::OpalComponent::MemType memType)
				{
						if( memType == SST::OpalComponent::MemType::LOCAL ) {
								statLocalMemUsage->addData(1);
						}
						else{
								statSharedMemUsage->addData(1);
						}
				}

		};

		class Opal : public SST::Component
		{
			public:

				Opal( SST::ComponentId_t id, SST::Params& params);

				void setup()  { };

				void finish();

				bool tick(SST::Cycle_t x);

				void setNextMemPool( int node,int fault_level );

				REQRESPONSE allocateLocalMemory(int node, int coreId, uint64_t vAddress, int fault_level, int pages);

				REQRESPONSE allocateSharedMemory(int node, int coreId, uint64_t vAddress, int fault_level, int pages);

				REQRESPONSE allocateFromReservedMemory(int node, uint64_t reserved_vAddress, uint64_t vAddress, int pages);

				REQRESPONSE isAddressReserved(int node, uint64_t vAddress);

				bool processRequest(int node, int coreId, uint64_t vAddress, int fault_level, int size);

				void processHint(int node, int fileId, uint64_t vAddress, int size);

				void deallocateSharedMemory(uint64_t page, int N);

				~Opal() {
					for(uint32_t i=0; i<num_nodes; i++)
						delete nodeInfo[i];
					delete[] nodeInfo;

					for(uint32_t i=0; i<num_shared_mempools; i++)
						delete sharedMemoryInfo[i];
					delete[] sharedMemoryInfo;

					delete opalBase;
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
							)

					SST_ELI_DOCUMENT_PORTS(
							{"coreLink%(num_cores)d", "Link to receive core requests", { "OpalComponent.OpalEvent", "" } },
							{"mmuLink%(num_cores)d", "Link to receive mmu requests", { "OpalComponent.OpalEvent", "" } },
							{"memCntrLink%(num_memCntrls)d", "Link to receive memory controller requests", { "OpalComponent.OpalEvent", "" } },
							)

					// Optional since there is nothing to document
					SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
							)

					Opal();  // for serialization only

					Opal(const Opal&); // do not implement

					void operator=(const Opal&); // do not implement

					uint64_t cycles;

					Output* output;

					int verbosity;

					OpalBase* opalBase;

					long long int max_inst; //maximum instructions per cycle

					uint32_t num_nodes; // stores total number of nodes available in the system

					uint32_t num_cores; // stores total number of cores from all the nodes in the system

					uint32_t num_memCntrls; //

					MemoryPrivateInfo **sharedMemoryInfo; // stores private information of each shared memory pool

					uint64_t shared_mem_size; // stores total shared memory size

					uint32_t num_shared_mempools; // stores number of pools shared memory is divided into

					NodePrivateInfo **nodeInfo; // stores private information of each node

		};
	}
}





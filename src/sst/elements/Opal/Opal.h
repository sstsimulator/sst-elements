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
				void setNextMemPool( int node );
				long long int allocateMemory( SST::OpalComponent::MemType memType, int node, int requested_size, int *allocated_size);
				void handleEvent(SST::Event* event) {};
				void handleRequest( SST::Event* e );
				bool tick(SST::Cycle_t x);
				std::queue<OpalEvent*> requestQ;

				long long int isAddressReserved(int node,long long int vAddress);
				void registerHint(int node, int fileId, long long int vAddress, int size);
				long long int processRequest(int node, int coreId, long long int page_fault_vAddress, int size);
				long long int allocateFromReservedMemory(int node, long long int reserved_address, long long int vAddress, int requested_size, int *allocated_size);
				long long int allocateSharedMemory(int node, int size, int *allocated_size);
				long long int allocateLocalMemory(int node, int size, int *allocated_size);

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
							{ "local_mem_usage", "Number of local memory frames used", "requests", 1},
							{ "shared_mem_usage", "Number of local memory frames used", "requests", 1},
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
					uint32_t num_shared_mempools;

					//shared memory - memory pool number and its pool
					std::map<int, Pool*> shared_mem;
					long long int shared_mem_size;

					//node specific information
					NodePrivateInfo **nodeInfo;

					std::map<int, std::pair<std::list<int>, long long int> > mmapFileIdHints;

					long long int max_inst;
					char* named_pipe;
					int* pipe_id;
					std::string user_binary;
					Output* output;
					int verbosity;

					Statistic<long long int>* statReadRequests;
					Statistic<long long int>* statWriteRequests;
					Statistic<long long int>* statAvgTime;


		};// END Opal

		class core_handler
		{
			public:

				int nodeId;
				int linkId;
				SST::Link * singLink;
				int dummy_address;
				core_handler() { dummy_address = 0;}
				unsigned int latency;
				Opal *owner;
				void setOwner(Opal *owner_) { owner = owner_; }

				void handleRequest(SST::Event* e)
				{
					OpalEvent *ev =  dynamic_cast<OpalComponent::OpalEvent*> (e);

					switch(ev->getType())
					{
					case EventType::HINT:
						{

							//std::cerr << "MLM Hint(0) : level "<< ev->hint << " Starting address is "<< std::hex << ev->getAddress();
                            //std::cerr << std::dec << " Size: "<< ev->getSize();
                            //std::cerr << " Ending address is " << std::hex << ev->getAddress() + ev->getSize() - 1;
                            //std::cerr << std::dec << std::endl;
						}
						break;
					case EventType::MMAP:
						{
				            std::cerr << "MLM mmap(" << ev->fileID<< ") : level "<< ev->hint << " Starting address is "<< std::hex << ev->getAddress();
							std::cerr << std::dec << " Size: "<< ev->getSize();
							std::cerr << " Ending address is " << std::hex << ev->getAddress() + ev->getSize() - 1;
							std::cerr << std::dec << std::endl;
							ev->setNodeId(nodeId);
							ev->setLinkId(linkId);
							owner->requestQ.push(ev);
						}
						break;

					case EventType::UNMAP:
						{
							std::cout<<"Opal has received an UNMAP CALL with virtual address: "<<ev->getAddress()<<" Size: "<<ev->getSize()<<std::endl;
							owner->requestQ.push(ev);
						}
						break;

					default:
						{
							//std::cout<<"Opal has received a REQUEST CALL with virtual address: "<< std::hex << ev->getAddress()<<" Size: "<<ev->getSize()<<std::endl;
							ev->setNodeId(nodeId);
							ev->setLinkId(linkId);
							owner->requestQ.push(ev);
						}
					}
				}

		};// END core_handler

		class NodePrivateInfo
		{
			public:
				NodePrivateInfo(Opal *_owner, uint32_t node, Params params) {//int num_cores, int allocPolicy, Pool *pool, long long int size, int latency) {
					owner = _owner;
					node_num = node;
					cores = (uint32_t) params.find<uint32_t>("cores", 1);
					latency = (uint32_t) params.find<uint32_t>("latency", 1);
					memoryAllocationPolicy = (uint32_t) params.find<uint32_t>("allocation_policy", 0);
					nextallocmem = 0;
					allocatedmempool = 0;
					memory_pool = new Pool(owner, (Params) params.find_prefix_params("memory."), SST::OpalComponent::MemType::LOCAL, node);
					memory_size = (uint32_t) params.find<uint32_t>("memory.size", 1);
					page_size = (uint32_t) params.find<uint32_t>("memory.frame_size", 1);
					Handlers = new core_handler[cores*2];

					std::cerr << "Node: " << node_num << " Allocation policy: " << memoryAllocationPolicy << std::endl;

					for(uint32_t i=0; i<cores*2; i++) {
						Handlers[i].latency = latency;
						Handlers[i].nodeId = node_num;
						Handlers[i].linkId = i;
						Handlers[i].owner = owner;
					}

				}

				Opal *owner;
				uint32_t node_num;
				uint32_t cores;
				uint32_t latency;

				/* allocation policies */
				uint32_t memoryAllocationPolicy;
				int nextallocmem;
				int allocatedmempool;

				/* local memory */
				Pool* memory_pool;
				uint32_t page_size; // page size of the node in KB's
				uint32_t memory_size;

				/* links to Ariel Core and Samba MMU*/
				core_handler *Handlers;

				// virtual address, fileId, size
				std::map<long long int, std::pair<int, std::pair<int, int> > > reservedSpace;

		};


	}// END OpalComponent
}//END SST



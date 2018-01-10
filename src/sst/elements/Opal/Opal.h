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


/* Author: Amro Awad
 * E-mail: amro.awad@ucf.edu
 */



#include <sst/core/sst_types.h>
#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/interfaces/simpleMem.h>

#include "Opal_Event.h"
#include <sst/core/output.h>
#include "sst/core/elementinfo.h"
#include <cstring>
#include <string>
#include <fstream>
#include <sstream>
#include <map>

#include <stdio.h>
#include <stdint.h>
#include <poll.h>
#include "mempool.h"

using namespace std;
using namespace SST;

using namespace SST::OpalComponent;



namespace SST {
	namespace OpalComponent {

		class core_handler{
			public:

				int id;
				SST::Link * singLink;
				int dummy_address;
				core_handler() { dummy_address = 0;}				
				unsigned int latency;

				void handleRequest(SST::Event* e)
				{
					OpalEvent * temp_ptr =  dynamic_cast<OpalComponent::OpalEvent*> (e);


					if(temp_ptr->getType() == EventType::HINT)
					{

						std::cout<<"MLM Hint : level "<<temp_ptr->hint<<" Starting address is "<< temp_ptr->getAddress() <<" Size: "<<temp_ptr->getSize()<<std::endl;

					}
					else if (temp_ptr->getType() == EventType::MMAP)
					{

						std::cout<<"Opal has received an MMAP CALL with ID "<<temp_ptr->fileID<<std::endl;
					}
					else
					{
						OpalEvent * tse = new OpalEvent(EventType::RESPONSE);
						tse->setResp(temp_ptr->getAddress(),dummy_address++,4096);
						singLink->send(latency, tse);
					}
				}


		};


		class Opal : public SST::Component {
			public:

				Opal( SST::ComponentId_t id, SST::Params& params); 
				void setup()  { };
				void finish() {};
				void handleEvent(SST::Event* event) {};
				void handleRequest( SST::Event* e );
				bool tick(SST::Cycle_t x);
				core_handler * Handlers;
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
							{"clock",              "Internal Controller Clock Rate.", "1.0 Ghz"},
							{"latency", "The time to be spent to service a memory request", "1000"},
							{"num_nodes", "number of disaggregated nodes in the system", "1"},
							{"num_cores", "total number of cores. this will be used to account for TLB shootdown latency", "1"},
							{"num_pools",          "This determines the number of memory pools", "1"},
							{"num_domains", "The number of domains in the system, typically similar to number of sockets/SoCs", "1"},
							{"allocation_policy",          "0 is private pools, then clustered pools, then public pools", "0"},
							{"size%(num_pools)", "Size of each memory pool in KBs", "8388608"},
							{"startaddress%(num_pools)", "the starting physical address of the pool", "0"},
							{"type%(num_pools)", "0 means private for specific NUMA domain, 1 means shared among specific NUMA domains, 2 means public", "2"},
							{"cluster_size", "This determines the number of NUMA domains in each cluster, if clustering is used", "1"},
							{"memtype%(num_pools)", "0 for typical DRAM, 1 for die-stacked DRAM, 2 for NVM", "0"},
							{"typepriority%(num_pools)", "0 means die-stacked, typical DRAM, then NVM", "0"},
							)

					// Optional since there is nothing to document
					SST_ELI_DOCUMENT_STATISTICS(
							)

					SST_ELI_DOCUMENT_PORTS(
							{"requestLink%(num_cores*2)d", "Link to receive allocation requests", { "OpalComponent.OpalEvent", "" } },
							{"shootdownLink%(num_cores)d", "Link to send shootdown requests to Samba units", { "OpalComponent.OpalEvent", "" } },
							)

					// Optional since there is nothing to document
					SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
							)

			private:
					Opal();  // for serialization only
					Opal(const Opal&); // do not implement
					void operator=(const Opal&); // do not implement


					int core_count;

					SST::Link * m_memChan; 
					SST::Link ** samba_to_opal; 

					SST::Link * event_link; // Note that this is a self-link for events

					Pool * mempools; // This represents the memory pools of the system

					unsigned int latency; // The page fault latency/ the time spent by Opal to service a memory allocation request

					long long int max_inst;
					char* named_pipe;
					int* pipe_id;
					std::string user_binary;
					Output* output;


					Statistic<long long int>* statReadRequests;
					Statistic<long long int>* statWriteRequests;
					Statistic<long long int>* statAvgTime;



		};

	}
}

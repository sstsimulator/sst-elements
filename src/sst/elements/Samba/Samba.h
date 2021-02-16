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


/* Author: Amro Awad
 * E-mail: aawad@sandia.gov
 */



#include <sst/core/sst_types.h>
#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/interfaces/simpleMem.h>

#include <sst/core/output.h>

#include <cstring>
#include <string>
#include <fstream>
#include <sstream>
#include <map>

#include <stdio.h>
#include <stdint.h>
#include <poll.h>

#include "TLBhierarchy.h"
#include "PageTableWalker.h"
#include "PageFaultHandler.h"
#include <sst/elements/memHierarchy/memEventBase.h>

//#include "arielcore.h"

using namespace std;

namespace SST {
	namespace SambaComponent {

		class Samba : public SST::Component {
			public:

                SST_ELI_REGISTER_COMPONENT(
                    Samba,
                    "Samba",
                    "Samba",
                    SST_ELI_ELEMENT_VERSION(1,0,0),
                    "Memory Management Unit (MMU) Component of SST",
                    COMPONENT_CATEGORY_PROCESSOR
                )

                SST_ELI_DOCUMENT_STATISTICS(
                    { "tlb_hits",        "Number of TLB hits", "requests", 1},   // Name, Desc, Enable Level
                    { "tlb_misses",      "Number of TLB misses", "requests", 1},   // Name, Desc, Enable Level
                    { "total_waiting",   "The total waiting time", "cycles", 1},   // Name, Desc, Enable Level
                    { "write_requests",  "Stat write_requests", "requests", 1},
                    { "tlb_shootdown",   "Number of TLB clears because of page-frees", "shootdowns", 2 },
                    { "tlb_page_allocs", "Number of pages allocated by the memory manager", "pages", 2 }
                )

                SST_ELI_DOCUMENT_PARAMS(
                    {"corecount", "Number of CPU cores to emulate, i.e., number of private Sambas", "1"},
                    {"levels", "Number of TLB levels per Samba", "1"},
                    {"perfect", "This is set to 1, when modeling an ideal TLB hierachy with 100\% hit rate", "0"},
                    {"os_page_size", "This represents the size of frames the OS allocates in KB", "4"}, // This is a hack, assuming the OS allocated only one page size, this will change later
                    {"sizes_L%(levels)", "Number of page sizes supported by Samba", "1"},
                    {"page_size%(sizes)_L%(levels)d", "the page size of the supported page size number x in level y","4"},
                    {"max_outstanding_L%(levels)d", "the number of max outstanding misses","1"},
                    {"max_width_L%(levels)d", "the number of accesses on the same cycle","1"},
                    {"size%(sizes)_L%(levels)d", "the number of entries of page size number x on level y","1"},
                    {"upper_link_L%(levels)d", "the latency of the upper link connects to this structure","0"},
                    {"assoc%(sizes)_L%(levels)d", "the associativity of size number X in Level Y", "1"},
                    {"clock", "the clock frequency", "1GHz"},
                    {"latency_L%(levels)d", "the access latency in cycles for this level of memory","1"},
                    {"parallel_mode_L%(levels)d", "this is for the corner case of having a one cycle overlap with accessing cache","0"},
                    {"page_walk_latency", "Each page table walk latency in nanoseconds", "50"},
                    {"self_connected", "Determines if the page walkers are acutally connected to memory hierarchy or just add fixed latency (self-connected)", "0"},
                    {"emulate_faults", "This indicates if the page faults should be emulated through requesting pages from page fault handler", "0"}
                )

                SST_ELI_DOCUMENT_PORTS(
                    {"cpu_to_mmu%(corecount)d", "Each Samba has link to its core", {}},
                    {"mmu_to_cache%(corecount)d", "Each Samba to its corresponding cache", {}},
                    {"ptw_to_mem%(corecount)d", "Each TLB hierarchy has a link to the memory for page walking", {}},
                    {"alloc_link_%(corecount)d", "Each core's link to an allocation tracker (e.g. memSieve)", {}}
                )

                SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
                        {"pagefaulthandler", "subcomponent to manage page faults", "SST::SambaComponent::PageFaultHandler"}
                )

				Samba(SST::ComponentId_t id, SST::Params& params);
				void init(unsigned int phase);
                                void setup()  { };
				void finish() {for(int i=0; i<(int) core_count; i++) TLB[i]->finish();};
				void handleEvent(SST::Event* event) {};
				bool tick(SST::Cycle_t x);

				// Following are the page table components of the application running on the Ariel instance that owns this Samba unit
				// Note, the application might be multi-threaded, however, all threads will share the sambe page table components below

				Address_t CR3;
				std::map<Address_t, Address_t> PGD;
				std::map<Address_t, Address_t> PUD;
				std::map<Address_t, Address_t> PMD;
				std::map<Address_t, Address_t> PTE;
				std::map<Address_t,int>  MAPPED_PAGE_SIZE4KB;
				std::map<Address_t,int>  MAPPED_PAGE_SIZE2MB;
				std::map<Address_t,int>  MAPPED_PAGE_SIZE1GB;

				std::map<Address_t,int> PENDING_PAGE_FAULTS;
                std::map<Address_t,int> PENDING_PAGE_FAULTS_PGD;
                std::map<Address_t,int> PENDING_PAGE_FAULTS_PUD;
                std::map<Address_t,int> PENDING_PAGE_FAULTS_PMD;
                std::map<Address_t,int> PENDING_PAGE_FAULTS_PTE;
                int cr3I;
				std::map<Address_t,int> PENDING_SHOOTDOWN_EVENTS;


			private:
				Samba();  // for serialization only
				Samba(const Samba&); // do not implement
				void operator=(const Samba&); // do not implement

				int create_pinchild(char* prog_binary, char** arg_list){return 0;}

	    		        SST::Link * event_link; // Note that this is a self-link for events

				SST::Link ** cpu_to_mmu;

                                std::vector<TLBhierarchy*> TLB;

				int emulate_faults; // This indicates if pafe fault handler is used or not

				SST::Link ** mmu_to_cache;

				SST::Link ** ptw_to_mem;

				long long int max_inst;
				char* named_pipe;
				int* pipe_id;
				std::string user_binary;
				Output* output;

                                std::vector<TLBhierarchy*> cores;
				uint32_t core_count;
				SST::Link** Samba_link;

				PageFaultHandler* pageFaultHandler;

				Statistic<uint64_t>* statReadRequests;


		};

	}
}

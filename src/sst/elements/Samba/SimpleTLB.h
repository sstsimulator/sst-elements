#ifndef SIMPLETLB_H
#define SIMPLETLB_H

#include <sst/core/sst_types.h>
#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/interfaces/simpleMem.h>
// 
// #include <sst/core/output.h>

// #include <cstring>
// #include <string>
// #include <fstream>
// #include <sstream>
// #include <map>
// 
// #include <stdio.h>
// #include <stdint.h>
// #include <poll.h>
// 
// #include "TLBhierarchy.h"
// #include "PageTableWalker.h"
// #include "PageFaultHandler.h"

#include <sst/elements/memHierarchy/memEventBase.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include <sst/elements/memHierarchy/util.h>

using namespace std;

namespace SST {
    namespace SambaComponent {

        class SimpleTLB : public SST::Component {
            public:

                // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY                                               
                SST_ELI_REGISTER_COMPONENT(                                                                       
                    SimpleTLB,                           // Component class                                        
                    "Samba",             // Component library (for Python/library lookup)          
                    "SimpleTLB",                         // Component name (for Python/library lookup)             
                    SST_ELI_ELEMENT_VERSION(1,0,0),     // Version of the component (not related to SST version)  
                    "Simple TLB Component",     // Description                                            
                    COMPONENT_CATEGORY_UNCATEGORIZED    // Category                                               
                )                                                                                                 


                SST_ELI_DOCUMENT_STATISTICS(
                    { "tlb_hits",        "Number of TLB hits", "requests", 1},   // Name, Desc, Enable Level
                    { "tlb_misses",      "Number of TLB misses", "requests", 1},   // Name, Desc, Enable Level
                    // { "total_waiting",   "The total waiting time", "cycles", 1},   // Name, Desc, Enable Level
                    // { "write_requests",  "Stat write_requests", "requests", 1},
                    // { "tlb_shootdown",   "Number of TLB clears because of page-frees", "shootdowns", 2 },
                    // { "tlb_page_allocs", "Number of pages allocated by the memory manager", "pages", 2 }
                )

                SST_ELI_DOCUMENT_PARAMS(
                    {"verbose", "(uint) Output verbosity for warnings/errors. 0[fatal error only], 1[warnings], 2[full state dump on fatal error]","1"},

                    {"fixed_mapping_va_start", "Translate only this region of virt. addresses. 4kb-aligned", "0xFFFFFFFF_FFFFFFFF"},
                    {"fixed_mapping_pa_start", "Translate to this region of phys. addresses. 4kb-aligned", "0xFFFFFFFF_FFFFFFFF"},
                    {"fixed_mapping_len",      "Size in bytes of region to translate. (4kb-aligned)", "0KiB"},

                    // {"corecount", "Number of CPU cores to emulate, i.e., number of private Sambas", "1"},
                    // {"levels", "Number of TLB levels per Samba", "1"},
                    // {"perfect", "This is set to 1, when modeling an ideal TLB hierachy with 100\% hit rate", "0"},
                    // {"os_page_size", "This represents the size of frames the OS allocates in KB", "4"}, // This is a hack, assuming the OS allocated only one page size, this will change later
                    // {"sizes_L%(levels)", "Number of page sizes supported by Samba", "1"},
                    // {"page_size%(sizes)_L%(levels)d", "the page size of the supported page size number x in level y","4"},
                    // {"max_outstanding_L%(levels)d", "the number of max outstanding misses","1"},
                    // {"max_width_L%(levels)d", "the number of accesses on the same cycle","1"},
                    // {"size%(sizes)_L%(levels)d", "the number of entries of page size number x on level y","1"},
                    // {"upper_link_L%(levels)d", "the latency of the upper link connects to this structure","0"},
                    // {"assoc%(sizes)_L%(levels)d", "the associativity of size number X in Level Y", "1"},
                    // {"clock", "the clock frequency", "1GHz"},
                    // {"latency_L%(levels)d", "the access latency in cycles for this level of memory","1"},
                    // {"parallel_mode_L%(levels)d", "this is for the corner case of having a one cycle overlap with accessing cache","0"},
                    // {"page_walk_latency", "Each page table walk latency in nanoseconds", "50"},
                    // {"self_connected", "Determines if the page walkers are acutally connected to memory hierarchy or just add fixed latency (self-connected)", "0"},
                    // {"emulate_faults", "This indicates if the page faults should be emulated through requesting pages from page fault handler", "0"},
                )

                // {"Port name", "Description", { "list of event types that the port can handle"} }  
                SST_ELI_DOCUMENT_PORTS(
                    {"high_network", "Link to cpu", {"MemHierarchy.MemEventBase"}},
                    {"low_network", "Link toward caches", {"MemHierarchy.MemEventBase"}},


                    //{"cpu_to_mmu%(corecount)d", "Each Samba has link to its core", {}},
                    //{"mmu_to_cache%(corecount)d", "Each Samba to its corresponding cache", {}},
                    //{"ptw_to_mem%(corecount)d", "Each TLB hierarchy has a link to the memory for page walking", {}},
                    //{"alloc_link_%(corecount)d", "Each core's link to an allocation tracker (e.g. memSieve)", {}}

                    //{"port",  "Link to another component", { "simpleElementExample.basicEvent", ""} }

                )



                // Constructor. Components receive a unique ID and the set of parameters that were assigned in the Python input.
                SimpleTLB(SST::ComponentId_t id, SST::Params& params);
                ~SimpleTLB();


                // Event handler, called when an event is received on high or low link
                void handleEvent(SST::Event *ev, bool is_low);
                                                                                
                // Clock handler, called on each clock cycle                    
                virtual bool clockTick(SST::Cycle_t);                            
                                                                                
                // Init?? Called by someone?? Used to pass mem/cache init events back and forth
				void init(unsigned int phase);
                                                                                
                // Statistic                                                    
                //Statistic<uint64_t>* bytesReceived;                             
                Statistic<uint64_t>* statReadRequests; //NOT IMPLEMENTED
                                                                                
                                                                                
                                                                                




            private:
                // SST Output object, for printing, error messages, etc.        
                SST::Output* out;                                               

                // Links                                                        
                SST::Link* link_high;  // to cpu
                SST::Link* link_low;  // to cpu

                //Params for fixed-region translation (For quick-and-dirty virtual memory)
                uint64_t                fixed_mapping_len; // fixed mapping is active and values are validated iff (len != 0)
                SST::MemHierarchy::Addr fixed_mapping_va_start;
                SST::MemHierarchy::Addr fixed_mapping_pa_start;

                //Transates mEv, returns new m_ev
                //TODO: this will need to return either the result, or some sort of stall-handle in case of a TLB miss
                SST::MemHierarchy::MemEvent* translateMemEvent(SST::MemHierarchy::MemEvent *mEv);

                SST::MemHierarchy::Addr translatePage(SST::MemHierarchy::Addr virtPageAddr);

                //SST::Link * event_link; // Note that this is a self-link for events
                //SST::Link ** cpu_to_mmu;
                //SST::Link ** mmu_to_cache;
                //SST::Link ** ptw_to_mem;



                //SST::Link** Samba_link;



        };

    }
}

#endif /* SIMPLETLB_H */

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

/* Author: Amro Awad
 * E-mail: aawad@sandia.gov
*/


#include <sst_config.h>

#define SST_ELI_COMPILE_OLD_ELI_WITHOUT_DEPRECATION_WARNINGS

#include "sst/core/element.h"
#include "sst/core/component.h"

#include "Samba.h"
#include "TLBhierarchy.h"
#include<iostream>

using namespace SST;
using namespace SST::SambaComponent;


static Component* create_Samba(ComponentId_t id, Params& params)
{
return new Samba(id,params);
};


static const ElementInfoStatistic Samba_statistics[] = {
    { "tlb_hits",        "Number of TLB hits", "requests", 1},   // Name, Desc, Enable Level 
    { "tlb_misses",        "Number of TLB misses", "requests", 1},   // Name, Desc, Enable Level 
    { "total_waiting",        "The total waiting time", "cycles", 1},   // Name, Desc, Enable Level 
    { "write_requests",       "Stat write_requests", "requests", 1},
    { "tlb_shootdown",        "Number of TLB clears because of page-frees", "shootdowns", 2 },
    { "tlb_page_allocs",      "Number of pages allocated by the memory manager", "pages", 2 },
    { NULL, NULL, NULL, 0 }
};



static const ElementInfoParam Samba_params[] = {

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
    {"emulate_faults", "This indicates if the page faults should be emulated through requesting pages from Opal", "0"},
    {NULL, NULL, NULL},
};



static const ElementInfoPort Samba_ports[] = {
    {"cpu_to_mmu%(corecount)d", "Each Samba has link to its core", NULL},
    {"ptw_to_opal%(corecount)d", "Each Samba has link to page fault handler (memory manager)", NULL},
    {"mmu_to_cache%(corecount)d", "Each Samba to its corresponding cache", NULL},
    {"ptw_to_mem%(corecount)d", "Each TLB hierarchy has a link to the memory for page walking", NULL},
    {"alloc_link_%(corecount)d", "Each core's link to an allocation tracker (e.g. memSieve)", NULL},
    {NULL, NULL, NULL}
};




static const ElementInfoModule modules[] = {
    { NULL, NULL, NULL, NULL, NULL, NULL }
};



// Needs to have as much components as defined in the elemennt
static const ElementInfoComponent components[] = {
	{ "Samba",
        "Memory Management Unit (MMU) Componenet of SST",
		NULL,
        create_Samba,
        Samba_params,
        Samba_ports,
        COMPONENT_CATEGORY_PROCESSOR,
        Samba_statistics
	},
	{ NULL, NULL, NULL, NULL, NULL, NULL, 0 }
};


extern "C" {
	ElementLibraryInfo Samba_eli = {
		"Samba",
		"MMU Unit",
		components,
        	NULL, /* Events */
        	NULL, /* Introspectors */
        	modules
	};
}

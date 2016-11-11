// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>

#include "sst/core/element.h"
#include "sst/core/component.h"

//#include "ariel.h"
//#include "arielcpu.h"
//#include "arieltexttracegen.h"

//#ifdef HAVE_LIBZ
//#include "arielgzbintracegen.h"
//#endif

//#define STRINGIZE(input) #input

#include "Samba.h"
#include "TLBhierarchy.h"
#include<iostream>

using namespace SST;
using namespace SST::SambaComponent;





static Component* create_Samba(ComponentId_t id, Params& params)
{


return new Samba(id,params); //dynamic_cast<SST::Component*>( Samba(id,params));

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
    {"os_page_size", "This represents the size of frames the OS allocates in KB", "4"}, // This is a hack, assuming the OS allocated only one page size, this will change later
    {"sizes_L%(levels)", "Number of page sizes supported by Samba", "1"},
    {"page_size%(sizes)_L%(levels)d", "the page size of the supported page size number x in level y","4"},
    {"max_outstanding_L%(levels)d", "the number of max outstanding misses","1"},
    {"max_width_L%(levels)d", "the number of accesses on the same cycle","1"},
    {"size%(sizes)_L%(levels)d", "the number of entries of page size number x on level y","1"},
    {"upper_link_L%(levels)d", "the latency of the upper link connects to this structure","1"},
    {"assoc%(sizes)_L%(levels)d", "the associativity of size number X in Level Y", "1"},
    {"clock", "the clock frequency", "1GHz"},

    {"latency_L%(levels)d", "the access latency in cycles for this level of memory","1"},
    {"parallel_mode_L%(levels)d", "this is for the corner case of having a one cycle overlap with accessing cache","0"},
    {"page_walk_latency", "This is the page walk latency in cycles just in case no page walker", "50"},
    {NULL, NULL, NULL},
};



static const ElementInfoPort Samba_ports[] = {
    {"cpu_to_mmu%(corecount)d", "Each Samba link to its core", NULL},
    {"mmu_to_cache%(corecount)d", "Each Samba to its corresponding cache", NULL},
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

// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.



/*
 * File:   coherenceControllers.h
 * Author: Branden Moore / Caesar De la Paz III
 *  Email:  bjmoor@sandia.gov / caesar.sst@gmail.com
 */

#include <sst_config.h>

#include "sst/core/serialization.h"
#include "sst/core/element.h"
#include "sst/core/component.h"

#include "cacheController.h"
#include "bus.h"
#include "trivialCPU.h"
#include "streamCPU.h"
#include "memoryController.h"
#include "directoryController.h"
#include "dmaEngine.h"
#include "memHierarchyInterface.h"

using namespace SST;
using namespace SST::MemHierarchy;


static const char * memEvent_port_events[] = {"memHierarchy.MemEvent", NULL};
static const char * net_port_events[] = {"memHierarchy.MemRtrEvent", NULL};



static Component* create_Cache(ComponentId_t id, Params& params)
{
	return Cache::cacheFactory(id, params);
}

static const ElementInfoParam cache_params[] = {
    /* Required */
    {"cache_frequency",         "Cache clock frequency.  In the case of L1s, this is usually the same as the CPU's frequency."},
    {"cache_size",              "Cache size in bytes.  Eg.  4KB or 1MB"},
    {"associativity",           "Associativity of the cache. In seet associative mode, this is the number of ways."},
    {"replacement_policy",      "Replacement policy of the cache array.  Options:  LRU, LFU, Random, or MRU. "},
    {"access_latency_cycles",   "Latency (in Cycles) to lookup data in the cache array."},
    {"L1",                      "Parameter specifies whether cache is an L1 --0, or 1--"},
    {"coherence_protocol",      "Coherence protocol.  Supported --MESI, MSI--"},
    {"mshr_num_entries",        "Number of MSHR entries.  This parameter is not valid in an L1 since MemHierarchy assumes an L1 MSHR size matches the size of the load/store queue unit of the CPU", "-1"},
    /* Not required */
    {"stat_group_ids",          "Stat Grouping.  Instructions with sames IDs will be group for stats. Separated by commas.", ""},
    {"mshr_latency_cycles",     "Latency (in Cycles) that takes to processes responses in the cache (MSHR response hits). If not specified, simple intrapolation is used based on the access latency", "-1"},
    {"idle_max",                "Cache temporarily turns off its clock after this amount of idle cycles", "6"},
    {"cache_line_size",         "Size of a cache line [aka cache block] in bytes.", "64"},
    {"prefetcher",              "Prefetcher Module", ""},
    {"directory_at_next_level", "Parameter specifies if there is a flat directory-controller as the higher level memory: 0, 1", "0"},
    {"statistics",              "Print cache stats at end of simulation: 0, 1", "0"},
    {"network_bw",              "Network link bandwidth.", "0"},
    {"network_address",         "When using a directory controller, this parameter represents the network address of this cache.", "0"},
	{"network_num_vc",          "When using a directory controller, this parameter represents the number of VCS on the on-chip network.", "3"},
    {"debug",                   "Prints debug statements --0[No debugging], 1[STDOUT], 2[STDERR], 3[FILE]--", "0"},
    {"debug_level",             "Debugging level: 0 to 10", "0"},
    {"uncache_all_requests",     "Used for verification purposes.  All requests are considered to be 'uncached'", "0"},
    {NULL, NULL, NULL}
};

static const ElementInfoPort cache_ports[] = {
    {"low_network_%(low_network_ports)d",  "Ports connected to lower level caches (closer to main memory)", memEvent_port_events},
    {"high_network_%(high_network_ports)d", "Ports connected to higher level caches (closer to CPU)", memEvent_port_events},
    {"directory",       "Network link port", net_port_events},
    {NULL, NULL, NULL}
};




static Component* create_Bus(ComponentId_t id, Params& params)
{
	return new Bus( id, params );
}

static const ElementInfoParam bus_params[] = {
    {"bus_frequency",       "Bus clock frequency"},
    {"broadcast",           "If set, messages are broadcasted to all other ports", "0"},
    {"fanout",              "If set, messages from the high network are replicated and sent to all low network ports", "0"},
    {"bus_latency_cycles",  "Number of ports on the bus", "0"},
    {"idle_max",            "Bus temporarily turns off clock after this amount of idle cycles", "6"},
    {"debug",               "Prints debug statements --0[No debugging], 1[STDOUT], 2[STDERR], 3[FILE]--", "0"},
    {"debug_level",         "Debugging level: 0 to 10", "0"},
    {NULL, NULL}
};


static const ElementInfoPort bus_ports[] = {
    {"low_network_%(low_network_ports)d",  "Ports connected to lower level caches (closer to main memory)", memEvent_port_events},
    {"high_network_%(high_network_ports)d", "Ports connected to higher level caches (closer to CPU)", memEvent_port_events},
    {NULL, NULL, NULL}
};


static Component* create_trivialCPU(ComponentId_t id, Params& params){
	return new trivialCPU( id, params );
}

static const ElementInfoPort cpu_ports[] = {
    {"mem_link", "Connection to caches.", NULL},
    {NULL, NULL, NULL}
};

static const ElementInfoParam cpu_params[] = {
    {"verbose",             "Determine how verbose the output from the CPU is", "1"},
    {"rngseed",             "Set a seed for the random generation of addresses", "7"},
    {"workPerCycle",        "How much work to do per cycle."},
    {"commFreq",            "How often to do a memory operation."},
    {"memSize",             "Size of physical memory."},
    {"do_write",            "Enable writes to memory (versus just reads).", "1"},
    {"num_loadstore",       "Stop after this many reads and writes.", "-1"},
    {"uncachedRangeStart",  "Beginning of range of addresses that are uncacheable.", "0x0"},
    {"uncachedRangeEnd",    "End of range of addresses that are uncacheable.", "0x0"},
    {NULL, NULL, NULL}
};


static Component* create_streamCPU(ComponentId_t id, Params& params){
	return new streamCPU( id, params );
}



static Component* create_MemController(ComponentId_t id, Params& params){
	return new MemController( id, params );
}

static const ElementInfoParam memctrl_params[] = {
    {"mem_size",            "Size of physical memory in MB", "0"},
    {"range_start",         "Address Range where physical memory begins", "0"},
    {"interleave_size",     "Size of interleaved pages in KB.", "0"},
    {"interleave_step",     "Distance between sucessive interleaved pages on this controller in KB.", "0"},
    {"memory_file",         "Optional backing-store file to pre-load memory, or store resulting state", "N/A"},
    {"clock",               "Clock frequency of controller", NULL},
    {"divert_DC_lookups",   "Divert Directory controller table lookups from the memory system, use a fixed latency (access_time). Default:0", "0"},
    {"backend",             "Timing backend to use:  Default to simpleMem", "memHierarchy.simpleMem"},
    {"request_width",       "Size of a DRAM request in bytes.  Should be a power of 2 - default 64", "64"},
    {"direct_link_latency", "Latency when using the 'direct_link', rather than 'snoop_link'", "10 ns"},
    {"debug",               "0 (default): No debugging, 1: STDOUT, 2: STDERR, 3: FILE.", "0"},
    {"debug_level",         "Debugging level: 0 to 10", "0"},
    {"statistics",          "0 (default): Don't print, 1: STDOUT, 2: STDERR, 3: FILE.", "0"},
    {"trace_file",          "File name (optional) of a trace-file to generate.", ""},
    {"coherence_protocol",  "Coherence protocol.  Supported: MESI (default), MSI"},
    {NULL, NULL, NULL}
};


static const ElementInfoPort memctrl_ports[] = {
    {"direct_link",     "Directly connect to another component (like a Directory Controller).", memEvent_port_events},
    {"cube_link",       "Link to VaultSim.", NULL}, /* TODO:  Make this generic */
    {NULL, NULL, NULL}
};


static Module* create_Mem_SimpleSim(Component* comp, Params& params){
    return new SimpleMemory(comp, params);
}

static const ElementInfoParam simpleMem_params[] = {
    {"access_time",     "Constant latency of memory operation.", "100 ns"},
    {NULL, NULL}
};


#if defined(HAVE_LIBDRAMSIM)
static Module* create_Mem_DRAMSim(Component* comp, Params& params){
    return new DRAMSimMemory(comp, params);
}


static const ElementInfoParam dramsimMem_params[] = {
    {"device_ini",      "Name of DRAMSim Device config file", NULL},
    {"system_ini",      "Name of DRAMSim Device system file", NULL},
    {NULL, NULL, NULL}
};

#endif

#if defined(HAVE_LIBHYBRIDSIM)
static Module* create_Mem_HybridSim(Component* comp, Params& params){
    return new HybridSimMemory(comp, params);
}


static const ElementInfoParam hybridsimMem_params[] = {
    {"device_ini",      "Name of HybridSim Device config file", NULL},
    {"system_ini",      "Name of HybridSim Device system file", NULL},
    {NULL, NULL, NULL}
};

#endif

static Module* create_Mem_VaultSim(Component* comp, Params& params){
    return new VaultSimMemory(comp, params);
}

static const ElementInfoParam vaultsimMem_params[] = {
    {"access_time",     "When not using DRAMSim, latency of memory operation.", "100 ns"},
    {NULL, NULL, NULL}
};



static Module* create_MemInterface(Component *comp, Params &params) {
    return new MemHierarchyInterface(comp, params);
}


static Component* create_DirectoryController(ComponentId_t id, Params& params){
	return new DirectoryController( id, params );
}

static const ElementInfoParam dirctrl_params[] = {
    {"network_bw",          "Network link bandwidth.", NULL},
    {"network_address",     "Network address of component.", ""},
	{"network_num_vc",      "The number of VCS on the on-chip network.", "3"},
    {"addr_range_start",    "Start of Address Range, for this controller.", "0"},
    {"addr_range_end",      "End of Address Range, for this controller.", NULL},
    {"interleave_size",     "(optional) Size of interleaved pages in KB.", "0"},
    {"interleave_step",     "(optional) Distance between sucessive interleaved pages on this controller in KB.", "0"},
    {"clock",               "Clock rate of controller.", "1GHz"},
    {"entry_cache_size",    "Size (in # of entries) the controller will cache.", "0"},
    {"debug",               "0 (default): No debugging, 1: STDOUT, 2: STDERR, 3: FILE.", "0"},
    {"debug_level",         "Debugging level: 0 to 10", "0"},
    {"statistics",          "0 (default): Don't print, 1: STDOUT, 2: STDERR, 3: FILE.", "0"},
    {"cache_line_size",     "Size of a cache line [aka cache block] in bytes.", "64"},
    {NULL, NULL, NULL}
};

static const ElementInfoPort dirctrl_ports[] = {
    {"memory",      "Link to Memory Controller", NULL},
    {"network",     "Network Link", NULL},
    {NULL, NULL, NULL}
};



static Component* create_DMAEngine(ComponentId_t id, Params& params){
	return new DMAEngine( id, params );
}

static const ElementInfoParam dmaengine_params[] = {
    {"debug",           "0 (default): No debugging, 1: STDOUT, 2: STDERR, 3: FILE.", "0"},
    {"debug_level",     "Debugging level: 0 to 10", "0"},
    {"clockRate",       "Clock Rate for processing DMAs.", "1GHz"},
    {"netAddr",         "Network address of component.", NULL},
	{"network_num_vc",  "The number of VCS on the on-chip network.", "3"},
    {"printStats",      "0 (default): Don't print, 1: STDOUT, 2: STDERR, 3: FILE.", "0"},
    {NULL, NULL, NULL}
};


static const ElementInfoPort dmaengine_ports[] = {
    {"netLink",     "Network Link",     net_port_events},
    {NULL, NULL, NULL}
};


static const ElementInfoModule modules[] = {
    {
        "simpleMem",
        "Simple constant-access time memory",
        NULL, /* Advanced help */
        NULL, /* ModuleAlloc */
        create_Mem_SimpleSim, /* Module Alloc w/ params */
        simpleMem_params,
        "SST::MemHierarchy::MemBackend"
    },
#if defined(HAVE_LIBDRAMSIM)
    {
        "dramsim",
        "DRAMSim-driven memory timings",
        NULL, /* Advanced help */
        NULL, /* ModuleAlloc */
        create_Mem_DRAMSim, /* Module Alloc w/ params */
        dramsimMem_params,
        "SST::MemHierarchy::MemBackend"
    },
#endif
#if defined(HAVE_LIBHYBRIDSIM)
    {
        "hybridsim",
        "HybridSim-driven memory timings",
        NULL, /* Advanced help */
        NULL, /* ModuleAlloc */
        create_Mem_HybridSim, /* Module Alloc w/ params */
        hybridsimMem_params,
        "SST::MemHierarchy::MemBackend"
    },
#endif
    {
        "vaultsim",
        "VaultSim Memory timings",
        NULL, /* Advanced help */
        NULL, /* ModuleAlloc */
        create_Mem_VaultSim, /* Module Alloc w/ params */
        vaultsimMem_params,
        "SST::MemHierarchy::MemBackend"
    },
    {
        "memInterface",
        "Simplified interface to Memory Hierarchy",
        NULL,
        NULL,
        create_MemInterface,
        NULL,
        "SST::Interfaces::SimpleMem"
    },
    {NULL, NULL, NULL, NULL, NULL, NULL}
};


static const ElementInfoComponent components[] = {
	{ "Cache",
		"Cache Component",
		NULL,
        create_Cache,
        cache_params,
        cache_ports,
        COMPONENT_CATEGORY_MEMORY
	},
	{ "Bus",
		"Mem Hierarchy Bus Component",
		NULL,
		create_Bus,
        bus_params,
        bus_ports,
        COMPONENT_CATEGORY_MEMORY
	},
	{"MemController",
		"Memory Controller Component",
		NULL,
		create_MemController,
        memctrl_params,
        memctrl_ports,
        COMPONENT_CATEGORY_MEMORY
	},
	{"DirectoryController",
		"Coherencey Directory Controller Component",
		NULL,
		create_DirectoryController,
        dirctrl_params,
        dirctrl_ports,
        COMPONENT_CATEGORY_MEMORY
	},
	{"DMAEngine",
		"DMA Engine Component",
		NULL,
		create_DMAEngine,
        dmaengine_params,
        dmaengine_ports,
        COMPONENT_CATEGORY_MEMORY
	},
	{"trivialCPU",
		"Simple Demo CPU for testing",
		NULL,
		create_trivialCPU,
        cpu_params,
        cpu_ports,
        COMPONENT_CATEGORY_PROCESSOR
	},
	{"streamCPU",
		"Simple Demo STREAM CPU for testing",
		NULL,
		create_streamCPU,
        cpu_params,
        cpu_ports,
        COMPONENT_CATEGORY_PROCESSOR
	},
	{ NULL, NULL, NULL, NULL, NULL, NULL, 0}
};

static const ElementInfoEvent memHierarchy_events[] = {
	{ "MemEvent", "Event to interact with the memHierarchy", NULL, NULL },
	{ "DMACommand", "Event to interact with DMA engine", NULL, NULL },
	{ NULL, NULL, NULL, NULL }
};

extern "C" {
	ElementLibraryInfo memHierarchy_eli = {
		"memHierarchy",
		"Cache Hierarchy",
		components,
        memHierarchy_events, /* Events */
        NULL, /* Introspectors */
        modules,
	};
}

BOOST_CLASS_EXPORT(MemEvent)
BOOST_CLASS_EXPORT(DMACommand)


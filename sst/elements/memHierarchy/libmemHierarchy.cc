// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>

#include "sst/core/serialization.h"
#include "sst/core/element.h"
#include "sst/core/component.h"

#include "cache.h"
#include "bus.h"
#include "trivialCPU.h"
#include "streamCPU.h"
#include "memController.h"
#include "dircontroller.h"
#include "dmaEngine.h"

using namespace SST;
using namespace SST::MemHierarchy;


static const char * memEvent_port_events[] = {"interfaces.MemEvent"};
static const char * bus_port_events[] = {"memHierarchy.BusEvent"};
static const char * net_port_events[] = {"memHierarchy.MemRtrEvent"};



static Component* create_Cache(ComponentId_t id, Params& params)
{
	return new Cache( id, params );
}

static const ElementInfoParam cache_params[] = {
    {"prefetcher",      "Prefetcher to use with cache (loaded as a module)"},
    {"num_ways",        "Associativity of the cache."},
    {"num_rows",        "How many cache rows. (Must be a power of 2)"},
    {"blocksize",       "Size of a cache block in bytes."},
    {"num_upstream",    "How many upstream ports there are. Typically 1 or 0."},
    {"next_level",      "Name of the next level cache"},
    {"mode",            "INCLUSIVE, EXCLUSIVE, STANDARD (default)"},
    {"access_time",     "Time taken to lookup data in the cache."},
    {"net_addr",        "When using a directory controller, the network address of this cache."},
    {"maxL1ResponseTime","Maximum allowed response to CPU from L1.  (Useful only on L1 caches, and useful only for debugging.)"},
    {"debug",           "0 (default): No debugging, 1: STDOUT, 2: STDERR, 3: FILE."},
    {"printStats",      "0 (default): Don't print, 1: STDOUT, 2: STDERR, 3: FILE."},
    {NULL, NULL}
};

static const ElementInfoPort cache_ports[] = {
    {"upstream%d",      "Upstream ports, directly connected, count dependant on parameter 'numUpstream'", memEvent_port_events},
    {"downstream",      "Downstream, directly connected, port", memEvent_port_events},
    {"snoop_link",      "Link to a Snoopy Bus port", bus_port_events},
    {"directory",       "Network link port", net_port_events},
    {NULL, NULL, NULL}
};




static Component* create_Bus(ComponentId_t id, Params& params)
{
	return new Bus( id, params );
}

static const ElementInfoParam bus_params[] = {
    {"numPorts",        "Number of Ports on the bus."},
    {"busDelay",        "Delay time for the bus."},
    {"atomicDelivery",  "0 (default) or 1.  If true, delivery to this bus is atomic to ALL members of a coherency strategy."},
    {"debug",           "0 (default): No debugging, 1: STDOUT, 2: STDERR, 3: FILE."},
    {NULL, NULL}
};


static const ElementInfoPort bus_ports[] = {
    {"port%d",          "Ports, range from 0 to numPorts-1.", bus_port_events},
    {NULL, NULL, NULL}
};


static Component* create_trivialCPU(ComponentId_t id, Params& params)
{
	return new trivialCPU( id, params );
}


static const ElementInfoParam cpu_params[] = {
    {"workPerCycle",        "How much work to do per cycle."},
    {"commFreq",            "How often to do a memory operation."},
    {"memSize",             "Size of physical memory."},
    {"do_write",            "Enable writes to memory (versus just reads)."},
    {"num_loadstore",       "Stop after this many reads and writes."},
    {"uncachedRangeStart",  "Beginning of range of addresses that are uncacheable."},
    {"uncachedRangeEnd",    "End of range of addresses that are uncacheable."},
    {NULL, NULL}
};


static Component* create_streamCPU(ComponentId_t id, Params& params)
{
	return new streamCPU( id, params );
}



static Component* create_MemController(ComponentId_t id, Params& params)
{
	return new MemController( id, params );
}

static const ElementInfoParam memctrl_params[] = {
    {"mem_size",        "Size of physical memory in MB"},
    {"rangeStart",      "Address Range where physical memory begins"},
    {"interleaveSize",  "Size of interleaved pages in KB."},
    {"interleaveStep",  "Distance between sucessive interleaved pages on this controller in KB."},
    {"memory_file",     "Optional backing-store file to pre-load memory, or store resulting state"},
    {"clock",           "Clock frequency of controller"},
    {"divert_DC_lookups",  "Divert Directory controller table lookups from the memory system, use a fixed latency (access_time). Default:0"},
    {"backend",         "Timing backend to use:  Default to simpleMem"},
    {"request_width",   "Size of a DRAM request in bytes.  Should be a power of 2 - default 64"},
    {"direct_link_latency",   "Latency when using the 'direct_link', rather than 'snoop_link'"},
    {"debug",           "0 (default): No debugging, 1: STDOUT, 2: STDERR, 3: FILE."},
    {"printStats",      "0 (default): Don't print, 1: STDOUT, 2: STDERR, 3: FILE."},
    {"traceFile",       "File name (optional) of a trace-file to generate."},
    {NULL, NULL}
};


static const ElementInfoPort memctrl_ports[] = {
    {"snoop_link",      "Connect to a memHiearchy.bus", bus_port_events},
    {"direct_link",     "Directly connect to another component (like a Directory Controller).", memEvent_port_events},
    {"cube_link",       "Link to VaultSim.", NULL}, /* TODO:  Make this generic */
    {NULL, NULL, NULL}
};


static Module* create_Mem_SimpleSim(Component* comp, Params& params)
{
    return new SimpleMemory(comp, params);
}

static const ElementInfoParam simpleMem_params[] = {
    {"access_time",     "When not using DRAMSim, latency of memory operation."},
    {NULL, NULL}
};


#if defined(HAVE_LIBDRAMSIM)
static Module* create_Mem_DRAMSim(Component* comp, Params& params)
{
    return new DRAMSimMemory(comp, params);
}


static const ElementInfoParam dramsimMem_params[] = {
    {"device_ini",      "Name of DRAMSim Device config file"},
    {"system_ini",      "Name of DRAMSim Device system file"},
    {NULL, NULL}
};

#endif

#if defined(HAVE_LIBHYBRIDSIM)
static Module* create_Mem_HybridSim(Component* comp, Params& params)
{
    return new HybridSimMemory(comp, params);
}


static const ElementInfoParam hybridsimMem_params[] = {
    {"device_ini",      "Name of HybridSim Device config file"},
    {"system_ini",      "Name of HybridSim Device system file"},
    {NULL, NULL}
};

#endif

static Module* create_Mem_VaultSim(Component* comp, Params& params)
{
    return new VaultSimMemory(comp, params);
}

static const ElementInfoParam vaultsimMem_params[] = {
    {"access_time",     "When not using DRAMSim, latency of memory operation."},
    {NULL, NULL}
};




static Component* create_DirectoryController(ComponentId_t id, Params& params)
{
	return new DirectoryController( id, params );
}

static const ElementInfoParam dirctrl_params[] = {
    {"network_addr",        "Network address of component."},
    {"network_bw",          "Network link bandwidth."},
    {"backingStoreSize",    "Space reserved in backing store for controller information (default = 0x1000000 (16MB))."},
    {"addrRangeStart",      "Start of Address Range, for this controller."},
    {"addrRangeEnd",        "End of Address Range, for this controller."},
    {"interleaveSize",      "(optional) Size of interleaved pages in KB."},
    {"interleaveStep",      "(optional) Distance between sucessive interleaved pages on this controller in KB."},
    {"clock",               "Clock rate of controller."},
    {"entryCacheSize",      "Size (in # of entries) the controller will cache."},
    {"debug",           "0 (default): No debugging, 1: STDOUT, 2: STDERR, 3: FILE."},
    {"printStats",      "0 (default): Don't print, 1: STDOUT, 2: STDERR, 3: FILE."},
    {NULL, NULL}
};

static const ElementInfoPort dirctrl_ports[] = {
    {"memory",      "Link to Memory Controller", NULL},
    {"network",     "Network Link", NULL},
    {NULL, NULL, NULL}
};



static Component* create_DMAEngine(ComponentId_t id, Params& params)
{
	return new DMAEngine( id, params );
}

static const ElementInfoParam dmaengine_params[] = {
    {"debug",           "0 (default): No debugging, 1: STDOUT, 2: STDERR, 3: FILE."},
    {"clockRate",       "Clock Rate for processing DMAs."},
    {"netAddr",         "Network address of component."},
    {"printStats",      "0 (default): Don't print, 1: STDOUT, 2: STDERR, 3: FILE."},
    {NULL, NULL}
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
        simpleMem_params
    },
#if defined(HAVE_LIBDRAMSIM)
    {
        "dramsim",
        "DRAMSim-driven memory timings",
        NULL, /* Advanced help */
        NULL, /* ModuleAlloc */
        create_Mem_DRAMSim, /* Module Alloc w/ params */
        dramsimMem_params
    },
#endif
#if defined(HAVE_LIBHYBRIDSIM)
    {
        "hybridsim",
        "HybridSim-driven memory timings",
        NULL, /* Advanced help */
        NULL, /* ModuleAlloc */
        create_Mem_HybridSim, /* Module Alloc w/ params */
        hybridsimMem_params
    },
#endif
    {
        "vaultsim",
        "VaultSim Memory timings",
        NULL, /* Advanced help */
        NULL, /* ModuleAlloc */
        create_Mem_VaultSim, /* Module Alloc w/ params */
        vaultsimMem_params
    },
    {NULL, NULL, NULL, NULL, NULL, NULL}
};


static const ElementInfoComponent components[] = {
	{ "Cache",
		"Cache Component",
		NULL,
        create_Cache,
        cache_params,
        cache_ports
	},
	{ "Bus",
		"Mem Hierarchy Bus Component",
		NULL,
		create_Bus,
        bus_params,
        bus_ports
	},
	{"MemController",
		"Memory Controller Component",
		NULL,
		create_MemController,
        memctrl_params,
        memctrl_ports
	},
	{"DirectoryController",
		"Coherencey Directory Controller Component",
		NULL,
		create_DirectoryController,
        dirctrl_params,
        dirctrl_ports,
	},
	{"DMAEngine",
		"DMA Engine Component",
		NULL,
		create_DMAEngine,
        dmaengine_params,
        dmaengine_ports,
	},
	{"trivialCPU",
		"Simple Demo CPU for testing",
		NULL,
		create_trivialCPU,
        cpu_params
	},
	{"streamCPU",
		"Simple Demo STREAM CPU for testing",
		NULL,
		create_streamCPU,
        cpu_params
	},
	{ NULL, NULL, NULL, NULL }
};


extern "C" {
	ElementLibraryInfo memHierarchy_eli = {
		"memHierarchy",
		"Simple Memory Hierarchy",
		components,
        NULL, /* Events */
        NULL, /* Introspectors */
        modules,
	};
}

BOOST_CLASS_EXPORT(DMACommand)

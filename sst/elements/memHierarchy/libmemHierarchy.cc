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

static Component*
create_Cache(SST::ComponentId_t id,
		SST::Params& params)
{
	return new Cache( id, params );
}


static Component*
create_Bus(SST::ComponentId_t id,
		SST::Params& params)
{
	return new Bus( id, params );
}


static Component*
create_trivialCPU(SST::ComponentId_t id,
		SST::Params& params)
{
	return new trivialCPU( id, params );
}

static Component*
create_streamCPU(SST::ComponentId_t id,
		SST::Params& params)
{
	return new streamCPU( id, params );
}


static Component*
create_MemController(SST::ComponentId_t id,
		SST::Params& params)
{
	return new MemController( id, params );
}

static Component*
create_DirectoryController(SST::ComponentId_t id,
		SST::Params& params)
{
	return new DirectoryController( id, params );
}

static Component*
create_DMAEngine(SST::ComponentId_t id, SST::Params& params)
{
	return new DMAEngine( id, params );
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

static const ElementInfoParam bus_params[] = {
    {"numPorts",        "Number of Ports on the bus."},
    {"busDelay",        "Delay time for the bus."},
    {"atomicDelivery",  "0 (default) or 1.  If true, delivery to this bus is atomic to ALL members of a coherency strategy."},
    {"debug",           "0 (default): No debugging, 1: STDOUT, 2: STDERR, 3: FILE."},
    {NULL, NULL}
};

static const ElementInfoParam memctrl_params[] = {
    {"mem_size",        "Size of physical memory in MB"},
    {"rangeStart",      "Address Range where physical memory begins"},
    {"interleaveSize",  "Size of interleaved pages in KB."},
    {"interleaveStep",  "Distance between sucessive interleaved pages on this controller in KB."},
    {"memory_file",     "Optional backing-store file to pre-load memory, or store resulting state"},
    {"clock",           "Clock frequency of controller"},
    {"divert_DC_lookups",  "Divert Directory controller table lookups from the memory system, use a fixed latency (access_time). Default:0"},
    {"use_dramsim",     "0 to not use DRAMSim, 1 to use DRAMSim"},
    {"use_vaultSim",    "0 to not use vaults, 1 to use a connected chain of vaultSim components"},
    {"device_ini",      "Name of DRAMSim Device config file"},
    {"system_ini",      "Name of DRAMSim Device system file"},
    {"access_time",     "When not using DRAMSim, latency of memory operation."},
    {"request_width",   "Size of a DRAM request in bytes.  Should be a power of 2 - default 64"},
    {"direct_link_latency",   "Latency when using the 'direct_link', rather than 'snoop_link'"},
    {"debug",           "0 (default): No debugging, 1: STDOUT, 2: STDERR, 3: FILE."},
    {"printStats",      "0 (default): Don't print, 1: STDOUT, 2: STDERR, 3: FILE."},
    {"traceFile",       "File name (optional) of a trace-file to generate."},
    {NULL, NULL}
};

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


static const ElementInfoParam dmaengine_params[] = {
    {"debug",           "0 (default): No debugging, 1: STDOUT, 2: STDERR, 3: FILE."},
    {"clockRate",       "Clock Rate for processing DMAs."},
    {"netAddr",         "Network address of component."},
    {"printStats",      "0 (default): Don't print, 1: STDOUT, 2: STDERR, 3: FILE."},
    {NULL, NULL}
};



static const ElementInfoComponent components[] = {
	{ "Cache",
		"Cache Component",
		NULL,
        create_Cache,
        cache_params
	},
	{ "Bus",
		"Mem Hierarchy Bus Component",
		NULL,
		create_Bus,
        bus_params
	},
	{"MemController",
		"Memory Controller Component",
		NULL,
		create_MemController,
        memctrl_params
	},
	{"DirectoryController",
		"Coherencey Directory Controller Component",
		NULL,
		create_DirectoryController,
        dirctrl_params
	},
	{"DMAEngine",
		"DMA Engine Component",
		NULL,
		create_DMAEngine,
        dmaengine_params
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
	};
}

BOOST_CLASS_EXPORT(DMACommand)

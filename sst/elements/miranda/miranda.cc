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


#include <sst_config.h>
#include <sst/core/serialization.h>
#include <sst/core/element.h>

#include "mirandaCPU.h"
#include "generators/singlestream.h"
#include "generators/randomgen.h"
#include "generators/gupsgen.h"
#include "generators/streambench.h"
#include "generators/revsinglestream.h"
#include "generators/stencil3dbench.h"
#include "generators/nullgen.h"
#include "generators/copygen.h"
#include "generators/spmvgen.h"

using namespace SST;
using namespace SST::Miranda;

static Module* load_SingleStreamGenerator(Component* owner, Params& params) {
	return new SingleStreamGenerator(owner, params);
}

static Module* load_GUPSGenerator(Component* owner, Params& params) {
	return new GUPSGenerator(owner, params);
}

static Module* load_CopyGenerator(Component* owner, Params& params) {
	return new CopyGenerator(owner, params);
}

static Module* load_RevSingleStreamGenerator(Component* owner, Params& params) {
	return new ReverseSingleStreamGenerator(owner, params);
}

static Module* load_RandomGenerator(Component* owner, Params& params) {
	return new RandomGenerator(owner, params);
}

static Module* load_STREAMGenerator(Component* owner, Params& params) {
	return new STREAMBenchGenerator(owner, params);
}

static Module* load_EmptyGenerator(Component* owner, Params& params) {
	return new EmptyGenerator(owner, params);
}

static Module* load_Stencil3DGenerator(Component* owner, Params& params) {
	return new Stencil3DBenchGenerator(owner, params);
}

static Module* load_SPMVGenerator(Component* owner, Params& params) {
	return new SPMVGenerator(owner, params);
}

static Component* load_MirandaBaseCPU(ComponentId_t id, Params& params) {
	return new RequestGenCPU(id, params);
}

static const ElementInfoParam emptyGen_params[] = {
    { NULL, NULL, NULL },
};

static const ElementInfoParam singleStreamGen_params[] = {
    { "verbose",          "Sets the verbosity output of the generator", "0" },
    { "count",            "Count for number of items being requested", "1024" },
    { "length",           "Length of requests", "8" },
    { "max_address",      "Maximum address allowed for generation", "16384" },
    { "startat",          "Sets the start address for generation", "0" },
    { NULL, NULL, NULL }
};

static const ElementInfoParam stencil3dGen_params[] = {
    { "nx",               "Sets the dimensions of the problem space in X", "10"},
    { "ny",               "Sets the dimensions of the problem space in Y", "10"},
    { "nz",               "Sets the dimensions of the problem space in Z", "10"},
    { "verbose",          "Sets the verbosity output of the generator", "0" },
    { "datawidth",        "Sets the data width of the mesh element, typically 8 bytes for a double", "8" },
    { "startz",           "Sets the start location in Z-plane for this instance, parallelism implemented as Z-plane decomposition", "0" },
    { "endz",             "Sets the end location in Z-plane for this instance, parallelism implemented as Z-plane decomposition", "10" },
    { "iterations",       "Sets the number of iterations to perform over this mesh", "1"},
    { NULL, NULL, NULL }
};

static const ElementInfoParam revSingleStreamGen_params[] = {
    { "start_at",         "Sets the start *index* for this generator", "2048" },
    { "stop_at",          "Sets the stop *index* for this generator, stop < start", "0" },
    { "verbose",          "Sets the verbosity of the output", "0" },
    { "datawidth",        "Sets the width of the memory operation", "8" },
    { "stride",           "Sets the stride, since this is a reverse stream this is subtracted per iteration, def=1", "1" },
    { NULL, NULL, NULL }
};

static const ElementInfoParam copyGen_params[] = {
    { "read_start_address",  "Sets the start read address for this generator", "0" },
    { "write_start_address", "Sets the start target address for writes for the generator", "1024" },
    { "request_size",        "Sets the size of each request in bytes", "8" },
    { "request_count",       "Sets the number of items to be copied", "128" },
    { "verbose",             "Sets the verbosity of the output", "0" },
    { NULL, NULL, NULL }
};

static const ElementInfoParam randomGen_params[] = {
    { "verbose",          "Sets the verbosity output of the generator", "0" },
    { "count",            "Count for number of items being requested", "1024" },
    { "length",           "Length of requests", "8" },
    { "max_address",      "Maximum address allowed for generation", "16384" },
    { "issue_op_fences",  "Issue operation fences, \"yes\" or \"no\", default is yes", "yes" },
    { NULL, NULL, NULL }
};

static const ElementInfoParam gupsGen_params[] = {
    { "verbose",          "Sets the verbosity output of the generator", "0" },
    { "seed_a",           "Sets the seed-a for the random generator", "11" },
    { "seed_b", 	  "Sets the seed-b for the random generator", "31" },
    { "count",            "Count for number of items being requested", "1024" },
    { "length",           "Length of requests", "8" },
    { "iterations",       "Number of iterations to perform", "1" },
    { "max_address",      "Maximum address allowed for generation", "536870912" /* 512MB */ },
    { "issue_op_fences",  "Issue operation fences, \"yes\" or \"no\", default is yes", "yes" },
    { NULL, NULL, NULL }
};

static const ElementInfoParam streamBench_params[] = {
    { "verbose",          "Sets the verbosity output of the generator", "0" },
    { "n",                "Sets the number of elements in the STREAM arrays", "10000" },
    { "n_per_call",       "Sets the number of iterations to generate per call to the generation function", "1"},
    { "operandwidth",     "Sets the length of the request, default=8 (i.e. one double)", "8" },
    { "start_a",          "Sets the start address of the array a", "0" },
    { "start_b",          "Sets the start address of the array b", "1024" },
    { "start_c",          "Sets the start address of the array c", "2048" },
    { NULL, NULL, NULL }
};

static const ElementInfoParam spmvBench_params[] = {
    { NULL, NULL, NULL }
};

static const ElementInfoModule modules[] = {
	{
		"SingleStreamGenerator",
		"Creates a single stream of accesses to/from memory",
		NULL,
		NULL,
		load_SingleStreamGenerator,
		singleStreamGen_params,
		"SST::Miranda::RequestGenerator"
	},
	{
		"SPMVGenerator",
		"Creates a diagonal matrix access pattern",
		NULL,
		NULL,
		load_SPMVGenerator,
		spmvBench_params,
		"SST::Miranda::RequestGenerator"
	},
	{
		"EmptyGenerator",
		"Creates an empty generator which just completes and does not issue requests",
		NULL,
		NULL,
		load_EmptyGenerator,
		emptyGen_params,
		"SST::Miranda::RequestGenerator"
	},
	{
		"ReverseSingleStreamGenerator",
		"Creates a single reverse ordering stream of accesses to/from memory",
		NULL,
		NULL,
		load_RevSingleStreamGenerator,
		revSingleStreamGen_params,
		"SST::Miranda::RequestGenerator"
	},
	{
		"STREAMBenchGenerator",
		"Creates a representation of the STREAM benchmark",
		NULL,
		NULL,
		load_STREAMGenerator,
		streamBench_params,
		"SST::Miranda::RequestGenerator"
	},
	{
		"Stencil3DBenchGenerator",
		"Creates a representation of a 3D 27pt stencil benchmark",
		NULL,
		NULL,
		load_Stencil3DGenerator,
		stencil3dGen_params,
		"SST::Miranda::RequestGenerator"
	},
	{
		"RandomGenerator",
		"Creates a single random stream of accesses to/from memory",
		NULL,
		NULL,
		load_RandomGenerator,
		randomGen_params,
		"SST::Miranda::RequestGenerator"
	},
	{
		"CopyGenerator",
		"Creates a single copy of stream of reads/writes replicating an array copy pattern",
		NULL,
		NULL,
		load_CopyGenerator,
		copyGen_params,
		"SST::Miranda::RequestGenerator"
	},
	{
		"GUPSGenerator",
		"Creates a random stream of accesses to read-modify-write",
		NULL,
		NULL,
		load_GUPSGenerator,
		gupsGen_params,
		"SST::Miranda::RequestGenerator"
	},
	{ NULL, NULL, NULL, NULL, NULL, NULL }
};

static const ElementInfoStatisticEnable basecpu_stats[] = {
	{ "split_read_reqs",	"Number of read requests split over a cache line boundary",	2 },
	{ "split_write_reqs",	"Number of write requests split over a cache line boundary", 	2 },
	{ "read_reqs", 		"Number of read requests issued", 				1 },
	{ "write_reqs", 	"Number of write requests issued", 				1 },
	{ "total_bytes_read",   "Count the total bytes requested by read operations",		1 },
	{ "total_bytes_write",  "Count the total bytes requested by write operations",      	1 },
	{ "req_latency",        "Running total of all latency for all requests",                2 },
        { "cycles_with_issue",  "Number of cycles which CPU was able to issue requests",        1 },
        { "cycles_no_issue",    "Number of cycles which CPU was not able to issue requests",    1 },
        { "time",               "Nanoseconds spent issuing requests",                           1 },
	{ "cycles_hit_fence",   "Number of issue cycles which stop issue at a fence",           2 },
	{ "cycles_max_reorder", "Number of issue cycles which hit maximum reorder lookup",      2 },
	{ "cycles_max_issue",   "Cycles with maximum operation issue",                          2 },
	{ NULL,			NULL,								0 }
};

static const ElementInfoParam basecpu_params[] = {
     { "max_reqs_cycle",   "Maximum number of requests the CPU can issue per cycle (this is for all reads and writes)", "2" },
     { "max_reorder_lookups", "Maximum number of operations the CPU is allowed to lookup for memory reorder", "16" },
     { "cache_line_size",  "The size of the cache line that this prefetcher is attached to, default is 64-bytes", "64" },
     { "maxmemreqpending", "Set the maximum number of requests allowed to be pending", "16" },
     { "verbose", 		"Sets the verbosity of output produced by the CPU", 	"0" },
     { "generator",        "The generator to be loaded for address creation", "miranda.SingleStreamGenerator" },
     { "clock",            "Clock for the base CPU", "2GHz" },
     { "memoryinterface",  "Sets the memory interface module to use", "memHierarchy.memInterface" },
     { NULL, NULL, NULL }
};

static const ElementInfoPort basecpu_ports[] = {
    	{ "cache_link",      "Link to Memory Controller", NULL },
    	{ NULL, NULL, NULL }
};

static const ElementInfoComponent components[] = {
	{
		"BaseCPU",
		"Creates a base Miranda CPU ready to load an address generator",
		NULL,
		load_MirandaBaseCPU,
		basecpu_params,
		basecpu_ports,
		COMPONENT_CATEGORY_PROCESSOR,
		basecpu_stats
	},
	{ NULL, NULL, NULL, NULL, NULL, NULL, 0 }
};

extern "C" {
    ElementLibraryInfo miranda_eli = {
        "Miranda",
        "Address generator compatible with SST MemHierarchy",
        components,
        NULL,
        NULL,
        modules,
        NULL,
        NULL
    };
}

// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include <sst/core/serialization.h>
#include <sst/core/element.h>
#include <sst/core/subcomponent.h>

#include "mirandaCPU.h"
#include "generators/singlestream.h"
#include "generators/randomgen.h"
#include "generators/gupsgen.h"
#include "generators/streambench.h"
#include "generators/inorderstreambench.h"
#include "generators/revsinglestream.h"
#include "generators/stencil3dbench.h"
#include "generators/nullgen.h"
#include "generators/copygen.h"
#include "generators/spmvgen.h"

using namespace SST;
using namespace SST::Miranda;

static SubComponent* load_SingleStreamGenerator(Component* owner, Params& params) {
	return new SingleStreamGenerator(owner, params);
}

static SubComponent* load_GUPSGenerator(Component* owner, Params& params) {
	return new GUPSGenerator(owner, params);
}

static SubComponent* load_CopyGenerator(Component* owner, Params& params) {
	return new CopyGenerator(owner, params);
}

static SubComponent* load_RevSingleStreamGenerator(Component* owner, Params& params) {
	return new ReverseSingleStreamGenerator(owner, params);
}

static SubComponent* load_RandomGenerator(Component* owner, Params& params) {
	return new RandomGenerator(owner, params);
}

static SubComponent* load_STREAMGenerator(Component* owner, Params& params) {
	return new STREAMBenchGenerator(owner, params);
}

static SubComponent* load_InOrderSTREAMGenerator(Component* owner, Params& params) {
	return new InOrderSTREAMBenchGenerator(owner, params);
}

static SubComponent* load_EmptyGenerator(Component* owner, Params& params) {
	return new EmptyGenerator(owner, params);
}

static SubComponent* load_Stencil3DGenerator(Component* owner, Params& params) {
	return new Stencil3DBenchGenerator(owner, params);
}

static SubComponent* load_SPMVGenerator(Component* owner, Params& params) {
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

static const ElementInfoParam inOrderStreamBench_params[] = {
    { "verbose",          "Sets the verbosity output of the generator", "0" },
    { "n",                "Sets the number of elements in the STREAM arrays", "10000" },
    { "block_per_call",   "Sets the number of iterations to generate per call to the generation function", "1"},
    { "operandwidth",     "Sets the length of the request, default=8 (i.e. one double)", "8" },
    { "start_a",          "Sets the start address of the array a", "0" },
    { "start_b",          "Sets the start address of the array b", "1024" },
    { "start_c",          "Sets the start address of the array c", "2048" },
    { NULL, NULL, NULL }
};

static const ElementInfoParam spmvBench_params[] = {
    { "matrix_nx",	"Sets the horizontal dimension of the matrix", "10" },
    { "matrix_ny",	"Sets the vertical dimension of the matrix (the number of rows)", "10" },
    { "element_width",  "Sets the width of one matrix element, typically 8 for a double", "8" },
    { "lhs_start_addr", "Sets the start address of the LHS vector", "0" },
    { "rhs_start_addr", "Sets the start address of the RHS vector", "80" },
    { "local_row_start", "Sets the row at which this generator will start processing", "0" },
    { "local_row_end",  "Sets the end at which rows will be processed by this generator", "10" },
    { "ordinal_width",  "Sets the width of ordinals (indices) in the matrix, typically 4 or 8", "8"},
    { "matrix_row_indices_start_addr", "Sets the row indices start address for the matrix", "0" },
    { "matrix_col_indices_start_addr", "Sets the col indices start address for the matrix", "0" },
    { "matrix_element_start_addr", "Sets the start address of the elements array", "0" },
    { "iterations",     "Sets the number of repeats to perform" },
    { "matrix_nnz_per_row", "Sets the number of non-zero elements per row", "9" },
    { NULL, NULL, NULL }
};

static const ElementInfoSubComponent subcomponents[] = {
	{
		"SingleStreamGenerator",
		"Creates a single stream of accesses to/from memory",
		NULL,
		load_SingleStreamGenerator,
		singleStreamGen_params,
		NULL,	// Statistics
		"SST::Miranda::RequestGenerator"
	},
	{
		"SPMVGenerator",
		"Creates a diagonal matrix access pattern",
		NULL,
		load_SPMVGenerator,
		spmvBench_params,
		NULL,
		"SST::Miranda::RequestGenerator"
	},
	{
		"EmptyGenerator",
		"Creates an empty generator which just completes and does not issue requests",
		NULL,
		load_EmptyGenerator,
		emptyGen_params,
		NULL,
		"SST::Miranda::RequestGenerator"
	},
	{
		"ReverseSingleStreamGenerator",
		"Creates a single reverse ordering stream of accesses to/from memory",
		NULL,
		load_RevSingleStreamGenerator,
		revSingleStreamGen_params,
		NULL,
		"SST::Miranda::RequestGenerator"
	},
	{
		"STREAMBenchGenerator",
		"Creates a representation of the STREAM benchmark",
		NULL,
		load_STREAMGenerator,
		streamBench_params,
		NULL,
		"SST::Miranda::RequestGenerator"
	},
	{
		"InOrderSTREAMBenchGenerator",
		"Creates a representation of the STREAM benchmark for in-order CPUs",
		NULL,
		load_InOrderSTREAMGenerator,
		inOrderStreamBench_params,
		NULL,
		"SST::Miranda::RequestGenerator"
	},
	{
		"Stencil3DBenchGenerator",
		"Creates a representation of a 3D 27pt stencil benchmark",
		NULL,
		load_Stencil3DGenerator,
		stencil3dGen_params,
		NULL,
		"SST::Miranda::RequestGenerator"
	},
	{
		"RandomGenerator",
		"Creates a single random stream of accesses to/from memory",
		NULL,
		load_RandomGenerator,
		randomGen_params,
		NULL,
		"SST::Miranda::RequestGenerator"
	},
	{
		"CopyGenerator",
		"Creates a single copy of stream of reads/writes replicating an array copy pattern",
		NULL,
		load_CopyGenerator,
		copyGen_params,
		NULL,
		"SST::Miranda::RequestGenerator"
	},
	{
		"GUPSGenerator",
		"Creates a random stream of accesses to read-modify-write",
		NULL,
		load_GUPSGenerator,
		gupsGen_params,
		NULL,
		"SST::Miranda::RequestGenerator"
	},
	{ NULL, NULL, NULL, NULL, NULL, NULL }
};

static const ElementInfoStatistic basecpu_stats[] = {
	{ "split_read_reqs",	"Number of read requests split over a cache line boundary",	"requests", 2 },
	{ "split_write_reqs",	"Number of write requests split over a cache line boundary", 	"requests", 2 },
	{ "read_reqs", 		"Number of read requests issued", 				"requests", 1 },
	{ "write_reqs", 	"Number of write requests issued", 				"requests", 1 },
	{ "total_bytes_read",   "Count the total bytes requested by read operations",		"bytes",    1 },
	{ "total_bytes_write",  "Count the total bytes requested by write operations",      	"bytes",    1 },
	{ "req_latency",        "Running total of all latency for all requests",                "ns",       2 },
        { "cycles_with_issue",  "Number of cycles which CPU was able to issue requests",        "cycles",   1 },
        { "cycles_no_issue",    "Number of cycles which CPU was not able to issue requests",    "cycles",   1 },
        { "time",               "Nanoseconds spent issuing requests",                           "ns",       1 },
	{ "cycles_hit_fence",   "Number of issue cycles which stop issue at a fence",           "cycles",   2 },
	{ "cycles_max_reorder", "Number of issue cycles which hit maximum reorder lookup",      "cycles",   2 },
	{ "cycles_max_issue",   "Cycles with maximum operation issue",                          "cycles",   2 },
	{ "cycles",             "Cycles executed",                                              "cycles",   1 },
	{ NULL,			NULL,								NULL,       0 }
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
        "miranda",
        "Address generator compatible with SST MemHierarchy",
        components,
        NULL, // events
        NULL, // introspectors
	NULL, // modules
	subcomponents,
        NULL, // partitioners
        NULL, // python module generators
	NULL  // generators
    };
}

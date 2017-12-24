// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>

#define SST_ELI_COMPILE_OLD_ELI_WITHOUT_DEPRECATION_WARNINGS

#include "sst/core/element.h"
#include "sst/core/component.h"
#include "sst/core/subcomponent.h"

//#include "ariel.h"
#include "arielcpu.h"
#include "arieltexttracegen.h"

#include "arielmemmgr_simple.h"
#include "arielmemmgr_malloc.h"

#ifdef HAVE_LIBZ
#include "arielgzbintracegen.h"
#endif

#define STRINGIZE(input) #input

using namespace SST;
using namespace SST::ArielComponent;

static Component* create_Ariel(ComponentId_t id, Params& params)
{
	return new ArielCPU( id, params );
};

static Module* load_TextTrace( Component* comp, Params& params) {
	return new ArielTextTraceGenerator(comp, params);
};

static SubComponent* load_ArielMemoryManagerSimple(Component * owner, Params& params) {
    return new ArielMemoryManagerSimple(owner, params);
};

static SubComponent* load_ArielMemoryManagerMalloc(Component * owner, Params& params) {
    return new ArielMemoryManagerMalloc(owner, params);
};

static const ElementInfoStatistic ariel_statistics[] = {
    { "read_requests",        "Statistic counts number of read requests", "requests", 1},   // Name, Desc, Enable Level 
    { "write_requests",       "Statistic counts number of write requests", "requests", 1},
    { "read_request_sizes",   "Statistic for size of read requests", "bytes", 1},   // Name, Desc, Enable Level 
    { "write_request_sizes",  "Statistic for size of write requests", "bytes", 1},
    { "split_read_requests",  "Statistic counts number of split read requests (requests which come from multiple lines)", "requests", 1},
    { "split_write_requests", "Statistic counts number of split write requests (requests which are split over multiple lines)", "requests", 1},
    { "no_ops",               "Statistic counts instructions which do not execute a memory operation", "instructions", 1},
    { "instruction_count",    "Statistic for counting instructions", "instructions", 1 },
    { "max_insts", "Maximum number of instructions reached by a thread",	"instructions", 0},
    { "fp_dp_ins",            "Statistic for counting DP-floating point instructions", "instructions", 1 },
    { "fp_dp_simd_ins",       "Statistic for counting DP-FP SIMD instructons", "instructions", 1 },
    { "fp_dp_scalar_ins",     "Statistic for counting DP-FP Non-SIMD instructons", "instructions", 1 },
    { "fp_dp_ops",            "Statistic for counting DP-FP operations (inst * SIMD width)", "instructions", 1 },
    { "fp_sp_ins",            "Statistic for counting SP-floating point instructions", "instructions", 1 },
    { "fp_sp_simd_ins",       "Statistic for counting SP-FP SIMD instructons", "instructions", 1 },
    { "fp_sp_scalar_ins",     "Statistic for counting SP-FP Non-SIMD instructons", "instructions", 1 },
    { "fp_sp_ops",            "Statistic for counting SP-FP operations (inst * SIMD width)", "instructions", 1 },
    { "cycles",               "Statistic for counting cycles of the Ariel core.", "cycles", 1 },
    { NULL, NULL, NULL, 0 }
};

static const ElementInfoParam ariel_text_trace_params[] = {
    { "trace_prefix", "Sets the prefix for the trace file", "ariel-core-" },
    { NULL, NULL, NULL }
};

#ifdef HAVE_LIBZ
static Module* load_CompressedBinaryTrace( Component* comp, Params& params) {
	return new ArielCompressedBinaryTraceGenerator(comp, params);
};

static const ElementInfoParam ariel_gzbinary_trace_params[] = {
    { "trace_prefix", "Sets the prefix for the trace file", "ariel-core-" },
    { NULL, NULL, NULL }
};
#endif

static const ElementInfoParam ariel_params[] = {
    {"verbose", "Verbosity for debugging. Increased numbers for increased verbosity.", "0"},
    {"profilefunctions", "Profile functions for Ariel execution, 0 = none, >0 = enable", "0" },
    {"alloctracker", "Use an allocation tracker (e.g. memSieve)", "0"},
    {"corecount", "Number of CPU cores to emulate", "1"},
    {"checkaddresses", "Verify that addresses are valid with respect to cache lines", "0"},
    {"maxissuepercycle", "Maximum number of requests to issue per cycle, per core", "1"},
    {"maxcorequeue", "Maximum queue depth per core", "64"},
    {"maxtranscore", "Maximum number of pending transactions", "16"},
    {"pipetimeout", "Read timeout between Ariel and traced application", "10"},
    {"cachelinesize", "Line size of the attached caching strucutre", "64"},
    {"arieltool", "Path to the Ariel PIN-tool shared library", ""},
    {"launcher", "Specify the launcher to be used for instrumentation, default is path to PIN", STRINGIZE(PINTOOL_EXECUTABLE)},
    {"executable", "Executable to trace", ""},
    {"launchparamcount", "Number of parameters supplied for the launch tool", "0" },
    {"launchparam%(launchparamcount)", "Set the parameter to the launcher", "" },
    {"envparamcount", "Number of environment parameters to supply to the Ariel executable, default=-1 (use SST environment)", "-1"},
    {"envparamname%(envparamcount)", "Sets the environment parameter name", ""},
    {"envparamval%(envparamcount)", "Sets the environment parameter value", ""},
    {"appargcount", "Number of arguments to the traced executable", "0"},
    {"apparg%(appargcount)d", "Arguments for the traced executable", ""},
    {"arielmode", "Tool interception mode, set to 1 to trace entire program (default), set to 0 to delay tracing until ariel_enable() call., set to 2 to attempt auto-detect", "2"},
    {"arielinterceptcalls", "Toggle intercepting library calls", "0"},
    {"arielstack", "Dump stack on malloc calls (also requires enabling arielinterceptcalls). May increase overhead due to keeping a shadow stack.", "0"},
    {"tracePrefix", "Prefix when tracing is enable", ""},
    {"clock", "Clock rate at which events are generated and processed", "1GHz"},
    {"tracegen", "Select the trace generator for Ariel (which records traced memory operations", ""},
    {"memmgr", "Memory manager to use for address translation", "ariel.MemoryManagerSimple"},
    {"opal_enabled", "If enabled, MLM allocation hints will be communicated to the centralized memory manager", "0"},
    {NULL, NULL, NULL}
};

static const ElementInfoPort ariel_ports[] = {
    {"cache_link_%(corecount)d", "Each core's link to its cache", NULL},
    {"alloc_link_%(corecount)d", "Each core's link to an allocation tracker (e.g. memSieve)", NULL},
    {"opal_link_%(corecount)d", "Each core's link to a centralized memory manager (Opal)", NULL},
    {NULL, NULL, NULL}
};

static const ElementInfoParam ArielMemoryManagerSimple_params[] = {
    {"verbose",         "Verbosity for debugging. Increased numbers for increased verbosity.", "0"},
    {"vtop_translate",  "Set to yes to perform virt-phys translation (TLB) or no to disable", "yes"},
    {"pagemappolicy",   "Select the page mapping policy for Ariel [LINEAR|RANDOMIZED]", "LINEAR"},
    {"translatecacheentries", "Keep a translation cache of this many entries to improve emulated core performance", "4096"},
    {"pagesize0", "Page size", "4096"},
    {"pagecount0", "Page count", "131072"},
    {"page_populate_0", "Pre-populate/partially pre-poulate the page table, this is the file to read in.", ""},
    {NULL, NULL, NULL}
};

static const ElementInfoParam ArielMemoryManagerMalloc_params[] = {
    {"verbose",         "Verbosity for debugging. Increased numbers for increased verbosity.", "0"},
    {"vtop_translate",  "Set to yes to perform virt-phys translation (TLB) or no to disable", "yes"},
    {"pagemappolicy",   "Select the page mapping policy for Ariel [LINEAR|RANDOMIZED]", "LINEAR"},
    {"translatecacheentries", "Keep a translation cache of this many entries to improve emulated core performance", "4096"},
    {"memorylevels",    "Number of memory levels in the system", "1"},
    {"defaultlevel",    "Default memory level", "0"},
    {"pagesize%(memorylevels)d", "Page size for memory Level x", "4096"},
    {"pagecount%(memorylevels)d", "Page count for memory Level x", "131072"},
    {"page_populate_%(memorylevels)d", "Pre-populate/partially pre-populate a page table for a level in memory, this is the file to read in.", ""},
    {NULL, NULL, NULL}
};

static const ElementInfoStatistic ArielMemoryManager_statistics[] = {
    { "tlb_hits",             "Hits in the simple Ariel TLB", "hits", 2 },
    { "tlb_evicts",           "Number of evictions in the simple Ariel TLB", "evictions", 2 },
    { "tlb_translate_queries","Number of TLB translations performed", "translations", 2 },
    { "tlb_shootdown",        "Number of TLB clears because of page-frees", "shootdowns", 2 },
    { "tlb_page_allocs",      "Number of pages allocated by the memory manager", "pages", 2 },
    { NULL, NULL, NULL, 0 }
};

static const ElementInfoModule modules[] = {
    {
	"TextTraceGenerator",
	"Provides tracing to text file capabilities",
	NULL,
	NULL,
	load_TextTrace,
	ariel_text_trace_params,
	"SST::ArielComponent::ArielTraceGenerator"
    },
#ifdef HAVE_LIBZ
    {
	"CompressedBinaryTraceGenerator",
	"Provides tracing to compressed file capabilities",
	NULL,
	NULL,
	load_CompressedBinaryTrace,
	ariel_gzbinary_trace_params,
	"SST::ArielComponent::ArielTraceGenerator"
    },
#endif
    { NULL, NULL, NULL, NULL, NULL, NULL }
};


static const ElementInfoComponent components[] = {
	{ "ariel",
        "PIN-based CPU model",
		NULL,
        create_Ariel,
		ariel_params,
        ariel_ports,
        COMPONENT_CATEGORY_PROCESSOR,
        ariel_statistics
	},
	{ NULL, NULL, NULL, NULL, NULL, NULL, 0 }
};

static const ElementInfoSubComponent subcomponents[] = {
    {
        "MemoryManagerSimple",
        "Simple allocate-on-first-touch memory manager",
        NULL,
        load_ArielMemoryManagerSimple,
        ArielMemoryManagerSimple_params,
        ArielMemoryManager_statistics,
        "SST::ArielComponent::ArielMemoryManager"
    },
    {   
        "MemoryManagerMalloc",
        "MLM memory manager which supports malloc/free in different memory pools",
        NULL,
        load_ArielMemoryManagerMalloc,
        ArielMemoryManagerMalloc_params,
        ArielMemoryManager_statistics,
        "SST::ArielComponent::ArielMemoryManager"
    },
    { NULL, NULL, NULL, NULL, NULL, NULL }
};

extern "C" {
	ElementLibraryInfo ariel_eli = {
		"ariel",
		"PIN-based CPU models",
		components,
        	NULL, /* Events */
        	NULL, /* Introspectors */
        	modules,
                subcomponents
	};
}

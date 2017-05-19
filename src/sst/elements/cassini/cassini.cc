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


#include "sst_config.h"
#include <assert.h>

#define SST_ELI_COMPILE_OLD_ELI_WITHOUT_DEPRECATION_WARNINGS

#include "sst/core/element.h"
#include "nbprefetch.h"
#include "strideprefetch.h"
#include "addrHistogrammer.h"

using namespace SST;
using namespace SST::Cassini;

static SubComponent* load_NextBlockPrefetcher(Component* owner, Params& params){
    return new NextBlockPrefetcher(owner, params);
}

static const ElementInfoStatistic nbprefetch_statistics[] = {
	{ "prefetches_issued",		"Number of prefetch requests issued", "prefetches", 1 },
	{ "miss_events_processed",	"Number of cache misses received",    "misses",     2 },
	{ "hit_events_processed",       "Number of cache hits received",      "hits",       2 },
	{ NULL, NULL, NULL, 0 }
};

static const ElementInfoParam nextBlockPrefetcher_params[] = {
    	{"cache_line_size",             "The size of the cache line that this prefetcher is attached to, default is 64-bytes", "64"},
    	{ NULL, NULL, NULL }
};

static SubComponent* load_StridePrefetcher(Component* owner, Params& params){
    return new StridePrefetcher(owner, params);
}

static const ElementInfoParam stridePrefetcher_params[] = {
    	{ "verbose",                     "Controls the verbosity of the cassini components", ""},
    	{ "cache_line_size",             "Controls the cache line size of the cache the prefetcher is attached too", "64"},
    	{ "history",                     "Start of Address Range, for this controller.", "16"},
    	{ "reach",                       "Reach of the prefetcher (ie how far forward to make requests)", "2"},
    	{ "detect_range",                "Range to detact addresses over in request-counts, default is 4.", "4"},
    	{ "address_count",               "Number of addresses to keep in the prefetch table", "64"},
    	{ "page_size",                   "Start of Address Range, for this controller.", "4096"},
    	{ "overrun_page_boundaries",     "Allow prefetcher to run over page alignment boundaries, default is 0 (false)", "0"},
    	{ NULL, NULL, NULL }
};

static const ElementInfoStatistic stride_statistics[] = {
	{ "prefetches_issued",			  "Counts number of prefetches issued",	"prefetches", 1 },
	{ "prefetches_canceled_by_page_boundary", "Counts number of prefetches which spanned over page boundaries and so did not issue", "prefetches", 1 },
	{ "prefetches_canceled_by_history",       "Counts number of prefetches which did not get issued because of prefetch history", "prefetches", 1 },
	{ "prefetch_opportunities",               "Counts the number of prefetch opportunities", "prefetches", 1 },
	{ NULL, NULL, NULL, 0 }
};

static SubComponent* load_AddrHistogrammer(Component* owner, Params& params){
    return new AddrHistogrammer(owner, params);
}

static const ElementInfoParam addrHistogrammer_params[] = {
	 {"addr_cutoff",                 "Address above this cutoff won't be profiled. Helps to avoid a huge table to track the barren vastness of virtual addresses between the heap and stack regions", "16 GB"},
	 {NULL, NULL, NULL}
};

static const ElementInfoStatistic addrHistogrammer_statistics[] = {
	 { "histogram_reads",     "Histogram of page read counts", "counts", 1 },
	 { "histogram_writes",    "Histogram of page write counts", "counts", 1 },
	 { NULL, NULL, NULL, 0 }
};

static const ElementInfoSubComponent subcomponents[] = {
    { "NextBlockPrefetcher",
      "Creates a prefetch engine which automatically loads the next block",
      NULL,
      load_NextBlockPrefetcher,
      nextBlockPrefetcher_params,
      nbprefetch_statistics,
      "SST::MemHierarchy::CacheListener"
    },
    { "StridePrefetcher",
      "Creates a prefetch engine which automatically recognizes strides and pre-loads blocks of data",
      NULL,
      load_StridePrefetcher,
      stridePrefetcher_params,
      stride_statistics,
      "SST::MemHierarchy::CacheListener"
    },
    { "AddrHistogrammer",
      "Creates a histogrammer which tracks the reads and writes leaving this cache",
      NULL,
      load_AddrHistogrammer,
      addrHistogrammer_params,
      addrHistogrammer_statistics,
      "SST::MemHierarchy::CacheListener"
    },
    { NULL, NULL, NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo cassini_eli = {
        "Cassini",
        "CPU Prefetcher to used along with MemHierarchy",
        NULL, //components,
        NULL,
        NULL,
	NULL,
        subcomponents,
        NULL,
        NULL,
    };
}

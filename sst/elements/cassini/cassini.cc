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


#include "sst_config.h"
#include "sst/core/serialization.h"
#include <assert.h>

#include "sst/core/element.h"
#include "nbprefetch.h"
#include "strideprefetch.h"

using namespace SST;
using namespace SST::Cassini;

static Module* load_NextBlockPrefetcher(Params& params){
    return new NextBlockPrefetcher(params);
}

static const ElementInfoParam nextBlockPrefetcher_params[] = {
    {"cache_line_size",             "The size of the cache line that this prefetcher is attached to, default is 64-bytes", "64"},
    {NULL, NULL, NULL}
};

static Module* load_StridePrefetcher(Params& params){
    return new StridePrefetcher(params);
}

static const ElementInfoParam stridePrefetcher_params[] = {
    {"verbose",                     "Controls the verbosity of the cassini components", ""},
    {"cache_line_size",             "Controls the cache line size of the cache the prefetcher is attached too", "64"},
    {"history",                     "Start of Address Range, for this controller.", "16"},
    {"reach",                       "Reach of the prefetcher (ie how far forward to make requests)", "2"},
    {"detect_range",                "Range to detact addresses over in request-counts, default is 4.", "4"},
    {"address_count",               "Number of addresses to keep in the prefetch table", "64"},
    {"page_size",                   "Start of Address Range, for this controller.", "4096"},
    {"overrun_page_boundaries",     "Allow prefetcher to run over page alignment boundaries, default is 0 (false)", "0"},
    { NULL, NULL, NULL }
};

static const ElementInfoModule modules[] = {
    { "NextBlockPrefetcher",
      "Creates a prefetch engine which automatically loads the next block",
      NULL,
      load_NextBlockPrefetcher,
      NULL,
      nextBlockPrefetcher_params,
      "SST::MemHierarchy::CacheListener"
    },
    { "StridePrefetcher",
      "Creates a prefetch engine which automatically recognizes strides and pre-loads blocks of data",
      NULL,
      load_StridePrefetcher,
      NULL,
      stridePrefetcher_params,
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
        modules,
        NULL,
        NULL,
    };
}

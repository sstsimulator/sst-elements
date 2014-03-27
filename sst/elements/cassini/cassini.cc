
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
    {"cache_line_size",             "Network link bandwidth.", "64"},
    {NULL, NULL, NULL}
};

static Module* load_StridePrefetcher(Params& params){
    return new StridePrefetcher(params);
}

static const ElementInfoParam stridePrefetcher_params[] = {
    {"verbose",                     "xxxx", ""},
    {"cache_line_size",             "The number of VCS on the on-chip network.", "64"},
    {"history",                     "Start of Address Range, for this controller.", "16"},
    {"reach",                       "Network link bandwidth.", "2"},
    {"detect_range",                "Network address of component.", "4"},
	{"address_count",               "The number of VCS on the on-chip network.", "64"},
    {"page_size",                   "Start of Address Range, for this controller.", "4096"},
    {"overrun_page_boundaries",     "End of Address Range, for this controller.", "0"},
    {NULL, NULL, NULL}
};

static const ElementInfoModule modules[] = {
    { "NextBlockPrefetcher",
      "Creates a prefetch engine which automatically loads the next block",
      NULL,
      load_NextBlockPrefetcher,
      NULL,
    },
    { "StridePrefetcher",
      "Creates a prefetch engine which automatically recognizes strides and pre-loads blocks of data",
      NULL,
      load_StridePrefetcher,
      NULL,
    },
    { NULL, NULL, NULL, NULL, NULL }
};

/*extern "C" {
    ElementLibraryInfo cassini_eli = {
        "cassini",
        "Cassini Uncore Components",
        components,
    };
}*/

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

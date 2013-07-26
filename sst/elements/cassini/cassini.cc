
#include "sst_config.h"
#include "sst/core/serialization.h"
#include <assert.h>

#include "sst/core/element.h"
#include "nbprefetch.h"
#include "strideprefetch.h"

using namespace SST;
using namespace SST::Cassini;

static Module*
load_NextBlockPrefetcher(Params& params)
{
    return new NextBlockPrefetcher(params);
}

static Module*
load_StridePrefetcher(Params& params)
{
    return new StridePrefetcher(params);
}

/*static const ElementInfoParam component_params[] = {
    {"pending", "Maximum pending prefetch requests which can be in flight."},
    { NULL, NULL }
};*/

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
        "Cassini Uncore Processor Components",
        NULL, //components,
        NULL,
	NULL,
	modules,
        // partitioners,
        // generators,
        NULL,
	NULL,
    };
}

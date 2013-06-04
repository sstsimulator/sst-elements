
#include "sst_config.h"
#include "sst/core/serialization/element.h"
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

static Component*
create_StridePrefetcher(SST::ComponentId_t id, 
                  SST::Component::Params_t& params)
{
    return new StridePrefetcher( id, params );
}

static const ElementInfoParam component_params[] = {
    {"pending", "Maximum pending prefetch requests which can be in flight."},
    { NULL, NULL}
};

static const ElementInfoModule modules[] = {
    { "NextBlockPrefetcher",
      "Creates a prefetch engine which automatically loads the next block",
      NULL,
      load_NextBlockPrefetcher,
      NULL,
    },
    { NULL, NULL, NULL, NULL, NULL }
};

static const ElementInfoComponent components[] = {
    { "StridePrefetcher",
      "Prefetches blocks based on stride accesses",
      NULL,
      create_StridePrefetcher,
      component_params
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo cassini_eli = {
        "Cassini",
        "Cassini Uncore Components",
        components,
    };
}

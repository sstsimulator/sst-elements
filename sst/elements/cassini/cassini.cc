
#include "sst_config.h"
#include "sst/core/serialization/element.h"
#include <assert.h>

#include "sst/core/element.h"
#include "nbprefetch.h"

using namespace SST;
using namespace SST::Cassini;

static Component*
create_NextBlockPrefetcher(SST::ComponentId_t id, 
                  SST::Component::Params_t& params)
{
    return new NextBlockPrefetcher( id, params );
}

static const ElementInfoParam component_params[] = {
    {"pending", "Maximum pending prefetch requests which can be in flight."},
    { NULL, NULL}
};

static const ElementInfoComponent components[] = {
    { "NextBlockPrefetcher",
      "Prefetches block n+1 after block n miss",
      NULL,
      create_NextBlockPrefetcher,
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

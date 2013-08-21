
#include "sst_config.h"
#include "sst/core/serialization.h"
#include <assert.h>

#include "sst/core/element.h"
#include "nbprefetch.h"

using namespace SST;
using namespace SST::Vanadis;

static Component*
create_NextBlockPrefetcher(SST::ComponentId_t id, 
                  SST::Params& params)
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
    ElementLibraryInfo vanadis_eli = {
        "Vanadis",
        "Vanadis RISC Core",
        components,
    };
}

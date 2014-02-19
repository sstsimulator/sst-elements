
#include "sst_config.h"
#include "sst/core/serialization.h"

#include <assert.h>

#include "emberengine.h"

#include "sst/core/element.h"
#include "sst/core/params.h"

using namespace SST;
using namespace SST::Ember;

static Component*
create_EmberComponent(SST::ComponentId_t id,
                  SST::Params& params)
{
    return new EmberEngine( id, params );
}

static const ElementInfoParam component_params[] = {
    { "seed_w", "" },
    { "seed_z", "" },
    { "seed", "" },
    { "rng", "" },
    { "count", "" },
    { NULL, NULL}
};

static const ElementInfoComponent components[] = {
    { "EmberComponent",
      "Random number generation component",
      NULL,
      create_EmberComponent,
      component_params
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo EmberComponent_eli = {
        "Ember",
        "Message Pattern Generation Library",
        components,
    };
}

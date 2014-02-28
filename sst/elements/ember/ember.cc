
#include "sst_config.h"
#include "sst/core/serialization.h"

#include <assert.h>

#include "emberengine.h"

#include "sst/core/element.h"
#include "sst/core/params.h"

#include "motifs/emberpingpong.h"
#include "motifs/emberring.h"

using namespace SST;
using namespace SST::Ember;

static Component*
create_EmberComponent(SST::ComponentId_t id,
                  SST::Params& params)
{
    return new EmberEngine( id, params );
}

static Module*
load_PingPong( Component* comp, Params& params ) {
	return new EmberPingPongGenerator(comp, params);
}

static Module*
load_Ring( Component* comp, Params& params ) {
	return new EmberRingGenerator(comp, params);
}

static const ElementInfoParam component_params[] = {
    { "seed_w", "" },
    { "seed_z", "" },
    { "seed", "" },
    { "rng", "" },
    { "count", "" },
    { NULL, NULL}
};

static const ElementInfoModule modules[] = {
    { 	"EmberPingPongGenerator",
	"Performs a Ping-Pong Motif",
	NULL,
	NULL,
	load_PingPong,
	NULL
    },
    { 	"EmberRingGenerator",
	"Performs a Ring Motif",
	NULL,
	NULL,
	load_Ring,
	NULL
    },
    {   NULL, NULL, NULL, NULL, NULL, NULL  }
};

static const ElementInfoComponent components[] = {
    { "EmberEngine",
      "Base communicator motif engine.",
      NULL,
      create_EmberComponent,
      component_params
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo ember_eli = {
        "Ember",
        "Message Pattern Generation Library",
        components,
	NULL, 		// Events
	NULL,		// Introspector
	modules,
	NULL,
	NULL
    };
}

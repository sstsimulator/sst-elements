
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
    { "printStats", "Prints the statistics from the component", "0"},
    { "verbose", "Sets the output verbosity of the component", "0" },
    { "msgapi", "Sets the messaging API of the end point" },
    { "send_bin_width", "Bin width of the send time histogram" },
    { "compute_bin_width", "Bin width of the compute time histogram" },
    { "init_bin_width", "Bin width of the init time histogram" },
    { "finalize_bin_width", "Bin width of the finalize time histogram" },
    { "recv_bin_width", "Bin width of the recv time histogram" },
    { "buffersize", "Sets the size of the message buffer which is used to back data transmission" },
    { NULL, NULL, NULL }
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

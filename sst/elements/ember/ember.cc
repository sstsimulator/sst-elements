
#include "sst_config.h"
#include "sst/core/serialization.h"

#include <assert.h>

#include "emberengine.h"

#include "sst/core/element.h"
#include "sst/core/params.h"

#include "motifs/emberpingpong.h"
#include "motifs/emberring.h"
#include "motifs/emberfini.h"

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

static Module*
load_Fini( Component* comp, Params& params ) {
	return new EmberFiniGenerator(comp, params);
}

static const ElementInfoParam component_params[] = {
    { "printStats", "Prints the statistics from the component", "0"},
    { "verbose", "Sets the output verbosity of the component", "0" },
    { "msgapi", "Sets the messaging API of the end point" },
    { "generator", "Sets the event generator or motif for the engine", "ember.EmberPingPongGenerator" },
    { "start_bin_width", "Bin width of the start time histogram", "5" },
    { "send_bin_width", "Bin width of the send time histogram", "5" },
    { "compute_bin_width", "Bin width of the compute time histogram", "5" },
    { "init_bin_width", "Bin width of the init time histogram", "5" },
    { "finalize_bin_width", "Bin width of the finalize time histogram", "5"},
    { "recv_bin_width", "Bin width of the recv time histogram", "5" },
    { "buffersize", "Sets the size of the message buffer which is used to back data transmission", "32768"},
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
    { 	"EmberFiniGenerator",
	"Performs a Fini Motif",
	NULL,
	NULL,
	load_Fini,
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

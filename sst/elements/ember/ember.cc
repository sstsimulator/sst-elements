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

#include "emberengine.h"

#include "sst/core/element.h"
#include "sst/core/params.h"

#include "motifs/emberhalo2d.h"
#include "motifs/embersweep2d.h"
#include "motifs/emberpingpong.h"
#include "motifs/emberring.h"
#include "motifs/emberfini.h"
#include "motifs/emberbarrier.h"
#include "motifs/emberallpingpong.h"
#include "motifs/embernull.h"

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
load_AllPingPong( Component* comp, Params& params ) {
	return new EmberAllPingPongGenerator(comp, params);
}

static Module*
load_Barrier( Component* comp, Params& params ) {
	return new EmberBarrierGenerator(comp, params);
}

static Module*
load_Ring( Component* comp, Params& params ) {
	return new EmberRingGenerator(comp, params);
}

static Module*
load_Fini( Component* comp, Params& params ) {
	return new EmberFiniGenerator(comp, params);
}

static Module*
load_Halo2D( Component* comp, Params& params ) {
	return new EmberHalo2DGenerator(comp, params);
}

static Module*
load_Sweep2D( Component* comp, Params& params ) {
	return new EmberSweep2DGenerator(comp, params);
}

static Module*
load_Null( Component* comp, Params& params ) {
	return new EmberNullGenerator(comp, params);
}


static const ElementInfoParam component_params[] = {
    { "printStats", "Prints the statistics from the component", "0"},
    { "verbose", "Sets the output verbosity of the component", "0" },
    { "msgapi", "Sets the messaging API of the end point" },
    { "motif", "Sets the event generator or motif for the engine", "ember.EmberPingPongGenerator" },
    { "start_bin_width", "Bin width of the start time histogram", "5" },
    { "send_bin_width", "Bin width of the send time histogram", "5" },
    { "compute_bin_width", "Bin width of the compute time histogram", "5" },
    { "init_bin_width", "Bin width of the init time histogram", "5" },
    { "finalize_bin_width", "Bin width of the finalize time histogram", "5"},
    { "recv_bin_width", "Bin width of the recv time histogram", "5" },
    { "irecv_bin_width", "Bin width of the irecv time histogram", "5" },
    { "wait_bin_width", "Bin width of the wait time histogram", "5" },
    { "barrier_bin_width", "Bin width of the barrier time histogram", "5" },
    { "buffersize", "Sets the size of the message buffer which is used to back data transmission", "32768"},
    { "jobId", "Sets the job id", "-1"},
    { "noisegen", "Sets the noise generator for the system", "constant" },
    { "noisemean", "Sets the mean of a Gaussian noise generator", "1.0" },
    { "noisestddev", "Sets the standard deviation of a noise generator", "0.1" },
    { NULL, NULL, NULL }
};

static const ElementInfoModule modules[] = {
    { 	"PingPongMotif",
	"Performs a Ping-Pong Motif",
	NULL,
	NULL,
	load_PingPong,
	NULL
    },
    { 	"RingMotif",
	"Performs a Ring Motif",
	NULL,
	NULL,
	load_Ring,
	NULL
    },
    { 	"BarrierMotif",
	"Performs a Barrier Motif",
	NULL,
	NULL,
	load_Barrier,
	NULL
    },
    { 	"AllPingPongMotif",
	"Performs a All Ping Pong Motif",
	NULL,
	NULL,
	load_AllPingPong,
	NULL
    },
    { 	"Halo2DMotif",
	"Performs a 2D halo exchange Motif",
	NULL,
	NULL,
	load_Halo2D,
	NULL
    },
    { 	"Sweep2DMotif",
	"Performs a 2D sweep exchange Motif with multiple vertex communication ordering",
	NULL,
	NULL,
	load_Sweep2D,
	NULL
    },
    { 	"FiniMotif",
	"Performs a communication finalize Motif",
	NULL,
	NULL,
	load_Fini,
	NULL
    },
    { 	"NullMotif",
	"Does bupkis",
	NULL,
	NULL,
	load_Null,
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

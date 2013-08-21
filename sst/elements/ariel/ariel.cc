// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"
#include "sst/core/serialization.h"
#include <assert.h>

#include "sst/core/element.h"

#include "ariel.h"

using namespace SST;
using namespace SST::ArielComponent;

Ariel::Ariel(ComponentId_t id, Params& params) :
  Component(id) {

  DPRINTF("Constructing Ariel.");

  // tell the simulator not to end without us
  registerAsPrimaryComponent();
  primaryComponentDoNotEndSim();
}

Ariel::Ariel() :
    Component(-1)
{
    // for serialization only
}

void Ariel::handleEvent(Event *ev)
{

}

bool Ariel::tick( Cycle_t ) {
}

// Element Libarary / Serialization stuff
BOOST_CLASS_EXPORT(Ariel)

static Component*
create_ariel(SST::ComponentId_t id, 
                  SST::Params& params)
{
    return new Ariel( id, params );
}

static const ElementInfoComponent components[] = {
    { "ariel",
      "PIN-based Memory Tracing Component",
      NULL,
      create_ariel
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo ariel_eli = {
        "ariel",
        "PIN-based Memory Tracing Component",
        components,
    };
}

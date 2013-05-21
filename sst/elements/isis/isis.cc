

#include "sst_config.h"
#include "sst/core/serialization/element.h"
#include <assert.h>

#include "sst/core/element.h"

#include "isis.h"

using namespace SST;
using namespace SST::IsisComponent;

void IsisComponent::handleEvent( SST::Event *ev ) {

}

bool IsisComponent::clockTic( SST::Cycle_t ) {

}

static const ElementInfoParam component_params[] = {
    { "workPerCycle", "Count of busy work to do during a clock tick." },
    { "commFreq", "Approximate frequency of sending an event during a clock tick." },
    { "commSize", "Size of communication to send." },
    { NULL, NULL}
};

static const ElementInfoComponent components[] = {
    { "IsisComponent",
      "Isis Coarse-grained MPI simulation",
      NULL,
      create_isisComponent,
      component_params
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo isis_eli = {
        "IsisComponent",
        "Isis Coarse-grained MPI simulation",
        components,
    };
}

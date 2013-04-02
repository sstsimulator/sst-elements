
#include "sst_config.h"
#include "sst/core/serialization/element.h"
#include <assert.h>

#include "sst/core/element.h"

#include "verona.h"

using namespace SST;
using namespace SST::Verona;

VeronaCPU::VeronaCPU(ComponentId_t id, Params_t& params) :
  Component(id) {

  // tell the simulator not to end without us
  registerExit();
}

VeronaCPU::VeronaCPU() :
    Component(-1)
{
    // for serialization only
}

void VeronaCPU::handleEvent(Event *ev)
{
	delete ev;
}

bool VeronaCPU::tick( Cycle_t ) {
	return false;
}

// Element Libarary / Serialization stuff
BOOST_CLASS_EXPORT(VeronaCPU)

static Component*
create_verona(SST::ComponentId_t id, 
                  SST::Component::Params_t& params)
{
    return new VeronaCPU( id, params );
}

static const ElementInfoComponent components[] = {
    { "verona",
      "Verona RISC-V CPU Core",
      NULL,
      create_verona
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo verona_eli = {
        "verona",
        "Verona RISC-V CPU Core",
        components,
    };
}

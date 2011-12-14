

#include <sst_config.h>
#include <sst/core/serialization/element.h>

#include <sst/core/element.h>

#include <ptlNic/ptlNic.h>

using namespace SST;

static Component*
create_PtlNic(ComponentId_t id, Component::Params_t& params)
{
    return new PtlNic( id, params );
}

static const ElementInfoComponent components[] = {
    { "PtlNic",
      "PtlNic, Portals4 memory mapped nic",
      NULL,
      create_PtlNic
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo portals4_eli = {
        "portals4",
        "portals4",
        components,
    };
}

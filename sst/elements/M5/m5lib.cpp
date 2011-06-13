

#include <sst_config.h>
#include <sst/core/serialization/element.h>

#include <sst/core/element.h>

#include <bounce.h>
#include <ptlNic/ptlNic.h>
#include <m5.h>

using namespace SST;

static Component*
create_M5(ComponentId_t id, Component::Params_t& params)
{
    return new M5( id, params );
}

static Component*
create_Bounce(ComponentId_t id, Component::Params_t& params)
{
    return new Bounce( id, params );
}

static Component*
create_PtlNic(ComponentId_t id, Component::Params_t& params)
{
    return new PtlNic( id, params );
}


static const ElementInfoComponent components[] = {
    { "M5",
      "M5",
      NULL,
      create_M5
    },
    { "Bounce",
      "Bounce, test component",
      NULL,
      create_Bounce
    },
    { "PtlNic",
      "PtlNic, Portals4 memory mapped nic",
      NULL,
      create_PtlNic
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo m5C_eli = {
        "M5",
        "M5",
        components,
    };
}

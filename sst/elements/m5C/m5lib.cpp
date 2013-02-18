

#include <sst_config.h>
#include <sst/core/serialization/element.h>

#include <sst/core/element.h>

#include <bounce.h>
#include <m5.h>
#include <rawEvent.h>
#include <memEvent.h>

using namespace SST;

static Component*
create_M5(ComponentId_t id, Component::Params_t& params)
{
    return new SST::M5::M5( id, params );
}

static Component*
create_Bounce(ComponentId_t id, Component::Params_t& params)
{
    return new SST::M5::Bounce( id, params );
}

BOOST_CLASS_EXPORT(SST::M5::RawEvent);
BOOST_CLASS_EXPORT(SST::M5::MemEvent);

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
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo m5C_eli = {
        "M5",
        "M5",
        components,
    };
}

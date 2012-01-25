#include <sst_config.h>
#include <sst/core/serialization/element.h>

#include <sst/core/element.h>

using namespace SST;

#include "ptlNicMMIF.h"

static Component*
create_PtlNicMMIF(ComponentId_t id, Component::Params_t& params)
{
    return new PtlNicMMIF( id, params );
}

static const ElementInfoComponent components[] = {
    { "PtlNicMMIF",
      "PtlNicMMIF Component",
      NULL,
      create_PtlNicMMIF
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo palacios_eli = {
        "PtlNicMMIF",
        "PtlNicMMIF Component",
        components,
    };
}


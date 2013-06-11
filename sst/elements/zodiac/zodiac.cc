#include <sst_config.h>
#include "sst/core/serialization/element.h"

#include <sst/core/element.h>
#include <sst/core/configGraph.h>

#include <stdio.h>
#include <stdarg.h>

#include "ztrace.h"

using namespace std;
using namespace SST;
using namespace SST::Zodiac;

static Component*
create_ZodiacTraceReader(SST::ComponentId_t id,
                  SST::Component::Params_t& params)
{
    return new ZodiacTraceReader( id, params );
}

static const ElementInfoComponent components[] = {
    {
	"ZodiacTraceReader",
	"Application Communication Trace Reader",
	NULL,
	create_ZodiacTraceReader,
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo zodiac_eli = {
        "zodiac",
        "Application Communication Tracing Component",
        components,
	NULL,
	NULL,
	NULL, //modules,
	// partitioners,
	// generators,
	NULL,
	NULL,
    };
}

#include <sst_config.h>
#include "sst/core/serialization.h"

#include <sst/core/element.h>
#include <sst/core/configGraph.h>

#include <stdio.h>
#include <stdarg.h>

#include "ztrace.h"
#include "zsirius.h"

#ifdef HAVE_ZODIAC_DUMPI
#include "zdumpi.h"
#endif

#ifdef HAVE_ZODIAC_OTF
#include "zotf.h"
#endif

using namespace std;
using namespace SST;
using namespace SST::Zodiac;

static Component*
create_ZodiacTraceReader(SST::ComponentId_t id,
                  SST::Component::Params_t& params)
{
    return new ZodiacTraceReader( id, params );
}

static Component*
create_ZodiacSiriusTraceReader(SST::ComponentId_t id,
                  SST::Component::Params_t& params)
{
    return new ZodiacSiriusTraceReader( id, params );
}

#ifdef HAVE_ZODIAC_DUMPI
static Component*
create_ZodiacDUMPITraceReader(SST::ComponentId_t id,
                  SST::Component::Params_t& params)
{
    return new ZodiacDUMPITraceReader( id, params );
}
#endif

#ifdef HAVE_ZODIAC_OTF
static Component*
create_ZodiacOTFTraceReader(SST::ComponentId_t id,
                  SST::Component::Params_t& params)
{
    std::cout << "Constructing a Zodiac OTF Reader..." << std::endl;
    return new ZodiacOTFTraceReader( id, params );
}
#endif

static const ElementInfoComponent components[] = {
    {
	"ZodiacTraceReader",
	"Application Communication Trace Reader",
	NULL,
	create_ZodiacTraceReader,
    },
    {
	"ZodiacSiriusTraceReader",
	"Application Communication Sirius Trace Reader",
	NULL,
	create_ZodiacSiriusTraceReader,
    },
#ifdef HAVE_ZODIAC_DUMPI
    {
	"ZodiacDUMPITraceReader",
	"Application Communication DUMPI Trace Reader",
	NULL,
	create_ZodiacDUMPITraceReader,
    },
#endif
#ifdef HAVE_ZODIAC_OTF
    {
	"ZodiacOTFTraceReader",
	"Application Communication OTF Trace Reader",
	NULL,
	create_ZodiacOTFTraceReader,
    },
#endif
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

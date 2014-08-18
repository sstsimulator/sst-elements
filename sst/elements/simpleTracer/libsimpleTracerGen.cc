// SST generator file for SimpleTracer component

#include <sst_config.h>
#include <sst/core/serialization.h>
#include <sst/core/element.h>
#include <sst/core/component.h>
#include <sst/core/params.h>

#include <simpleTracer.h>

using namespace SST;
using namespace SST::SIMPLETRACER;

const char * memEvent_List[] = {"MemEvent", NULL};

static Component* create_simpleTracer(SST::ComponentId_t id, SST::Params& params) {
    return new simpleTracer(id, params);
};

static const ElementInfoParam component_params[] = {
    {"clock", "Frequency, same as system clock frequency", "1 Ghz"},
    {"statsPrefix", "writes stats to statsPrefix file", ""},
    {"tracePrefix", "writes trace to tracePrefix tracing is enable", ""},
    {"debug", "Print debug statements with increasing verbosity [0-10]", "0"},
    {"statistics", "0-No-stats, 1-print-stats", "0"},
    {"pageSize", "Page Size (bytes), used for selecting number of bins for address histogram ", "4096"},
    {"accessLatencyBins", "Number of bins for access latency histogram" "10"},
    {NULL, NULL}
};

static const ElementInfoPort component_ports[] = {
    {"northBus", "Connect towards cpu side", memEvent_List},
    {"southBus", "Connect towards memory side", memEvent_List},
    {NULL, NULL, NULL}
};

static const ElementInfoComponent components[] = {
    {
        "simpleTracer",
        "Simple tracer and stats collector component",
        NULL,
        create_simpleTracer,
        component_params,
        component_ports,
        COMPONENT_CATEGORY_UNCATEGORIZED
    },
    {NULL, NULL, NULL, NULL}
};

extern "C" {
    ElementLibraryInfo simpleTracer_eli = {
        "simpleTracer", 
        "Simple tracer and stats collector",
        components,
    };
}


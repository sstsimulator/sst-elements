#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/params.h>

#include <mem/bus.hh>

#include <debug.h>
#include <paramHelp.h>

using namespace SST;
using namespace std;

class Component;

extern "C" {
SimObject* create_Bus( Component*, string name, Params& params );
}

SimObject* create_Bus( Component*, string name, Params& params )
{
    BusParams& busP             = * new BusParams;

    busP.name = name;

    INIT_INT( busP, params, clock);
    INIT_BOOL( busP, params, responder_set );
    INIT_INT( busP, params, block_size);
    INIT_INT( busP, params, bus_id );
    INIT_INT( busP, params, header_cycles);
    INIT_INT( busP, params, width);

    return busP.create();
}

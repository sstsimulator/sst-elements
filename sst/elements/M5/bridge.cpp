#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/params.h>

#include <mem/bridge.hh>

#include <paramHelp.h>

using namespace SST;
using namespace std;

class Component;

extern "C" {
    SimObject* create_Bridge( Component*, string name, Params& params );
}

SimObject* create_Bridge( Component* comp, string name, Params& params )
{
    BridgeParams& bridgeP = *new BridgeParams;

    bridgeP.name = name;

    INIT_INT( bridgeP, params, delay );
    INIT_INT( bridgeP, params, nack_delay );
    INIT_BOOL( bridgeP, params, write_ack );
    INIT_INT( bridgeP, params, req_size_a );
    INIT_INT( bridgeP, params, req_size_b );
    INIT_INT( bridgeP, params, resp_size_a );
    INIT_INT( bridgeP, params, resp_size_b );

     bridgeP.filter_ranges_a.resize(1);
     bridgeP.filter_ranges_b.resize(1);

     bridgeP.filter_ranges_a[0].start = 0;
     bridgeP.filter_ranges_a[0].end = -1;

     bridgeP.filter_ranges_b[0].start = 0x00100000;
     bridgeP.filter_ranges_b[0].end =   0xffffffff;

#if 0
    INIT_HEX( bridgeP, params, filter_ranges_a.start );
    INIT_HEX( bridgeP, params, filter_ranges_a.end );

    INIT_HEX( bridgeP, params, filter_ranges_b.start );
    INIT_HEX( bridgeP, params, filter_ranges_b.end );
#endif

    return bridgeP.create( );
}


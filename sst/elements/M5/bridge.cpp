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

    INIT_LATENCY( bridgeP, params, delay );
    INIT_LATENCY( bridgeP, params, nack_delay );
    INIT_BOOL( bridgeP, params, write_ack );
    INIT_INT( bridgeP, params, req_size_a );
    INIT_INT( bridgeP, params, req_size_b );
    INIT_INT( bridgeP, params, resp_size_a );
    INIT_INT( bridgeP, params, resp_size_b );

    bridgeP.filter_ranges_a.resize(1);
    bridgeP.filter_ranges_b.resize(1);

    bridgeP.filter_ranges_a[0].start = 
                    params.find_integer( "filter_range_a.start", 0 );
    bridgeP.filter_ranges_a[0].end = 
                    params.find_integer( "filter_range_a.end", -1 );

    bridgeP.filter_ranges_b[0].start =
                    params.find_integer( "filter_range_b.start", 0 );
    bridgeP.filter_ranges_b[0].end =
                    params.find_integer( "filter_range_b.end", -1 );

    DBGC(1,"range_a %#lx %#lx\n",bridgeP.filter_ranges_a[0].start,
                            bridgeP.filter_ranges_a[0].end );
    DBGC(1,"range_b %#lx %#lx\n",bridgeP.filter_ranges_b[0].start,
                                bridgeP.filter_ranges_b[0].end );

    return bridgeP.create( );
}


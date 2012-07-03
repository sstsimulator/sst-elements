#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/params.h>

#include <mem/physical.hh>

#include <debug.h>
#include <paramHelp.h>
#include <loadMemory.h>

using namespace SST;
using namespace std;

class Component;

extern "C" {
  SimObject* create_PhysicalMemory( Component*, string name, Params& sParams );
}

SimObject* create_PhysicalMemory( Component*, string name, Params& sParams )
{
    PhysicalMemoryParams& params   = *new PhysicalMemoryParams;

    params.name = name;

    INIT_HEX( params, sParams, range.start );
    INIT_HEX( params, sParams, range.end );
    INIT_LATENCY( params, sParams, latency );
    INIT_LATENCY( params, sParams, latency_var );
    INIT_BOOL( params, sParams, null );
    INIT_BOOL( params, sParams, zero );
    INIT_STR( params, sParams, file );
    INIT_STR( params, sParams, ddrConfig );
    INIT_INT( params, sParams, tx_q );

    params.megsMem = params.range.size() / (1024 * 1024);
    PhysicalMemory* physmem = params.create();

    loadMemory( name + ".exe", physmem, sParams.find_prefix_params("exe") );

    return physmem;
}

// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include <sst/core/params.h>

#include <mem/cache/cache.hh>

#include <debug.h>
#include <paramHelp.h>

using namespace SST;
using namespace SST::M5;
using namespace std;

class Component;

extern "C" {
SimObject* create_BaseCache( Component*, string name, Params& params );
}

SimObject* create_BaseCache( Component*, string name, Params& params )
{
    BaseCacheParams& cache = * new BaseCacheParams;

    cache.name = name;

    INIT_INT( cache, params, trace_addr );
    INIT_INT( cache, params, max_miss_count);

    string tmp = params.find_string("prefetch_policy");

    if ( tmp.compare("none") == 0 ) {
        cache.prefetch_policy = Enums::none;
    } else if ( tmp.compare("tagged") == 0 ) {
        cache.prefetch_policy = Enums::tagged;
    } else if ( tmp.compare("stride") == 0 ) {
        cache.prefetch_policy = Enums::stride;
    } else if ( tmp.compare("ghb") == 0 ) {
        cache.prefetch_policy = Enums::ghb;
    } else {
        assert(0);
    }

    DBGC( 1, "%s %s\n",name.c_str(), tmp.c_str() );

    INIT_INT( cache, params, addr_range.start );
    INIT_INT( cache, params, addr_range.end );
    cache.repl = NULL;
    INIT_INT( cache, params, num_cpus);
    INIT_INT( cache, params, latency);
    INIT_INT( cache, params, prefetch_latency);
    INIT_BOOL( cache, params, forward_snoops );
    INIT_BOOL( cache, params, prefetch_data_accesses_only );
    INIT_BOOL( cache, params, prefetch_on_access );
    INIT_BOOL( cache, params, prefetch_past_page );
    INIT_BOOL( cache, params, prefetch_serial_squash );
    INIT_BOOL( cache, params, prefetch_use_cpu_id );
    INIT_BOOL( cache, params, prioritizeRequests );
    INIT_BOOL( cache, params, two_queue );
    INIT_INT( cache, params, assoc );
    INIT_INT( cache, params, block_size );
    INIT_INT( cache, params, hash_delay );
    INIT_INT( cache, params, mshrs );
    INIT_INT( cache, params, prefetch_degree );
    INIT_INT( cache, params, prefetcher_size );
    INIT_INT( cache, params, subblock_size );
    INIT_INT( cache, params, tgts_per_mshr );
    INIT_INT( cache, params, write_buffers);
    INIT_INT( cache, params, size );
    INIT_BOOL( cache, params, is_top_level );

    return cache.create();
}

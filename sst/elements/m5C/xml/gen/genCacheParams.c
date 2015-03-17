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

#include <stdio.h>

static char *str = 
"<cacheParams>\n\
    <trace_addr> 0 </trace_addr>\n\
    <max_miss_count> 0 </max_miss_count>\n\
    <prefetch_policy> none </prefetch_policy>\n\
    <addr_range.start> 0 </addr_range.start>\n\
    <addr_range.end> -1 </addr_range.end>\n\
    <latency> 1000 </latency>\n\
    <prefetch_latency> 4000 </prefetch_latency>\n\
    <forward_snoops> true </forward_snoops>\n\
    <prefetch_cache_check_push> true </prefetch_cache_check_push>\n\
    <prefetch_data_accesses_only> true </prefetch_data_accesses_only>\n\
    <prefetch_on_access> true </prefetch_on_access>\n\
    <prefetch_past_page> false </prefetch_past_page>\n\
    <prefetch_serial_squash> false </prefetch_serial_squash>\n\
    <prefetch_use_cpu_id> true </prefetch_use_cpu_id>\n\
    <prioritizeRequests> true </prioritizeRequests>\n\
    <two_queue> true </two_queue>\n\
    <assoc> 2 </assoc>\n\
    <block_size> 64 </block_size>\n\
    <hash_delay> 1 </hash_delay>\n\
    <mshrs> 32 </mshrs>\n\
    <prefetch_degree> 1  </prefetch_degree>\n\
    <prefetcher_size> 64 </prefetcher_size>\n\
    <subblock_size> 0 </subblock_size>\n\
    <tgts_per_mshr> 64 </tgts_per_mshr>\n\
    <write_buffers> 128 </write_buffers>\n\
    <size> 0x8000 </size>\n\
    <num_cpus> 1 </num_cpus>\n\
    <is_top_level> false </is_top_level>\n\
</cacheParams>\n";

void genCacheParams( FILE* fp )
{
    fprintf( fp, "%s",str);
}

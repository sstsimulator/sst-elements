# Simple SST python tests to demonstrate use of simpleTrace.
# This test generates a trace of all memory references going to memController
# Generated Files are -trace: test-2_memory_references_trace.txt, 
# stats: test-2_memory_references_stats.txt

## arch model
#
#  comp_cpu <-> comp_l1cache <-> comp_l2cache <-> comp_tracer <-> comp_memory
#
## 

import sst

# Define SST core options
sst.setProgramOption("stop-at", "1ms")

#define simulation components
comp_cpu = sst.Component("cpu0", "memHierarchy.standardCPU")
comp_cpu.addParams({
    "memFreq" : 5,
    "memSize" : "100KiB",
    "verbose" : 0,
    "clock" : "2GHz",
    "rngseed" : 111,
    "maxOutstanding" : 16,
    "opCount" : 100,
    "reqsPerIssue" : 2,
    "write_freq" : 35, # 35% writes
    "read_freq" : 65,  # 65% reads
})

iface = comp_cpu.setSubComponent("memory", "memHierarchy.standardInterface")

comp_l1cache = sst.Component("l1cache", "memHierarchy.Cache")
comp_l1cache.addParams({
    "access_latency_cycles" : "5",
    "cache_frequency"       : "2 Ghz",
    "replacement_policy"    : "lru",
    "coherence_protocol"    : "MSI",
    "associativity"         : "4",
    "cache_line_size"       : "64",
    "debug_level"           : "8",
    "L1"                    : "1",
    "debug"                 : "0",
    "cache_size"            : "4 KB",
})

comp_l2cache = sst.Component("l2cache", "memHierarchy.Cache")
comp_l2cache.addParams({
    "access_latency_cycles" : "20",
    "cache_frequency"       : "2 Ghz",
    "replacement_policy"    : "lru",
    "coherence_protocol"    : "MSI",
    "associativity"         : "4",
    "cache_line_size"       : "64",
    "debug_level"           : "8",
    "L1"                    : "0",
    "debug"                 : "0",
    "cache_size"            : "64 KB",
})

comp_memory = sst.Component("memory", "memHierarchy.MemController")
comp_memory.addParams({
    "clock"                 : "2 Ghz",
    "request_width"         : "64",
    "debug"                 : "0",
    "backend"               : "memHierarchy.simpleMem"
})

backend = comp_memory.setSubComponent("backend", "memHierarchy.simpleMem")
backend.addParams({ "mem_size"      : "1024MiB" })

comp_tracer = sst.Component("tracer", "cacheTracer.cacheTracer")
comp_tracer.addParams({
    "clock"      : "2 Ghz", 
    "debug"      : "8",
    "statistics" : "1",
    "pageSize"   : "4096",
    "accessLatencyBins" : "10",
    "tracePrefix" : "test_cacheTracer_2_mem_ref_trace.txt",
    "statsPrefix" : "test_cacheTracer_2_mem_ref_stats.txt",
 })

# define the simulation links
link_cpu_l1cache = sst.Link("link_cpu_l1cache")
link_cpu_l1cache.connect((iface, "lowlink", "100ps"),(comp_l1cache, "highlink", "100ps"))

link_l1cache_l2cache = sst.Link("link_l1cache_l2cache")
link_l1cache_l2cache.connect((comp_l1cache, "lowlink", "100ps"), (comp_l2cache, "highlink", "100ps"))

link_l2cache_tracer = sst.Link("link_l2cache_tracer")
link_l2cache_tracer.connect((comp_l2cache, "lowlink", "100ps"), (comp_tracer, "northBus", "100ps"))

link_tracer_mem = sst.Link("link_tracer_mem")
link_tracer_mem.connect((comp_tracer, "southBus", "100ps"), (comp_memory, "highlink", "100ps"))


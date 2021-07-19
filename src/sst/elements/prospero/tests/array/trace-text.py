# Automatically generated SST Python input
import sst
import os

# Define SST core options
sst.setProgramOption("timebase", "1ps")
sst.setProgramOption("stopAtCycle", "5s")

# Define the simulation components
comp_cpu = sst.Component("cpu", "prospero.prosperoCPU")
comp_cpu.addParams({
    "verbose" : "0",
    "reader" : "prospero.ProsperoTextTraceReader",
    "readerParams.file" : "sstprospero-0-0.trace"
})
comp_l1cache = sst.Component("l1cache", "memHierarchy.Cache")
comp_l1cache.addParams({
      "access_latency_cycles" : "1",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "L1" : "1",
      "cache_size" : "64 KB"
})
comp_memctrl = sst.Component("memory", "memHierarchy.MemController")
comp_memctrl.addParams({
      "clock" : "1GHz"
})
memory = comp_memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
      "access_time" : "1000 ns",
      "mem_size" : "4906MiB",
})

# Define the simulation links
link_cpu_cache_link = sst.Link("link_cpu_cache_link")
link_cpu_cache_link.connect( (comp_cpu, "cache_link", "1000ps"), (comp_l1cache, "high_network_0", "1000ps") )
link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link.connect( (comp_l1cache, "low_network_0", "50ps"), (comp_memctrl, "direct_link", "50ps") )
# End of generated output.

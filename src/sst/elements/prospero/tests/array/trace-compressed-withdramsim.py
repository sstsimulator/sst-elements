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
	"reader" : "prospero.ProsperoCompressedBinaryTraceReader",
	"readerParams.file" : "sstprospero-0-0-gz.trace"
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
comp_memory = sst.Component("memory", "memHierarchy.MemController")
comp_memory.addParams({
      "coherence_protocol" : "MESI",
      "clock" : "1GHz",
      "backend.access_time" : "1000 ns",
      "backend.device_ini" : "DDR3_micron_32M_8B_x4_sg125.ini",
      "backend.system_ini" : "system.ini",
      "backend.mem_size" : "512MiB",
      "backend" : "memHierarchy.dramsim"
})

# Define the simulation links
link_cpu_cache_link = sst.Link("link_cpu_cache_link")
link_cpu_cache_link.connect( (comp_cpu, "cache_link", "1000ps"), (comp_l1cache, "high_network_0", "1000ps") )
link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link.connect( (comp_l1cache, "low_network_0", "50ps"), (comp_memory, "direct_link", "50ps") )
# End of generated output.

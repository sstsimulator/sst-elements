# Automatically generated SST Python input
import sst

# Define SST core options
sst.setProgramOption("timebase", "1 ps")
sst.setProgramOption("stopAtCycle", "200000ns")

# Define the simulation components
cpu = sst.Component("cpu", "memHierarchy.trivialCPU")
cpu.addParams({
      "workPerCycle" : """1000""",
      "commFreq" : """100""",
      "memSize" : """0x1000""",
      "do_write" : """1""",
      "num_loadstore" : """1000"""
})
l1cache = sst.Component("l1cache", "memHierarchy.Cache")
l1cache.addParams({
      "cache_frequency" : """2 Ghz""",
      "cache_size" : """2 KB""",
      "coherence_protocol" : """MSI""",
      "replacement_policy" : """lru""",
      "associativity" : """4""",
      "access_latency_cycles" : """4""",
      "low_network_links" : """1""",
      "cache_line_size" : """32""",
      "L1" : """1""",
      "debug" : """${MEM_DEBUG}""",
      "statistics" : """1"""
})
memory = sst.Component("memory", "memHierarchy.MemController")
memory.addParams({
      "coherence_protocol" : """MSI""",
      "debug" : """${MEM_DEBUG}""",
      "access_time" : """100 ns""",
      "request_width" : """32""",
      "mem_size" : """512""",
      "clock" : """1GHz""",
      "backend" : """memHierarchy.dramsim""",
      "device_ini" : """DDR3_micron_32M_8B_x4_sg125.ini""",
      "system_ini" : """system.ini"""
})


# Define the simulation links
cpu_cache_link = sst.Link("cpu_cache_link")
cpu_cache_link.connect( (cpu, "mem_link", "1000ps"), (l1cache, "high_network_0", "1000ps") )
mem_bus_link = sst.Link("mem_bus_link")
mem_bus_link.connect( (l1cache, "low_network_0", "50ps"), (memory, "direct_link", "50ps") )
# End of generated output.

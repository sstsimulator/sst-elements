import sst
import os
from optparse import OptionParser

# options
op = OptionParser()
op.add_option("-n", "--neurons", action="store", type="int", dest="neurons", default=500)
# cache size in KiB
op.add_option("-c", "--cacheSz", action="store", type="int", dest="cacheSz", default=2)
#sts dispatch & parallelism
op.add_option("-s", "--STS", action="store", type="int", dest="sts", default=4)
# max memory out
op.add_option("-m", "--memOut", action="store", type="int", dest="memOut", default=4)
(options, args) = op.parse_args()

# Define the simulation components
comp_gna = sst.Component("GNA", "GNA.GNA")
comp_gna.addParams({
    "verbose" : 1,
    "neurons" : options.neurons,
    "clock" : "1GHz",
    "BWPperTic" : 1,
    "STSDispatch" : options.sts,
    "STSParallelism" : options.sts,
    "MaxOutMem" : options.memOut 
})

comp_l1cache = sst.Component("l1cache", "memHierarchy.Cache")
comp_l1cache.addParams({
    "access_latency_cycles" : "1",
    "cache_frequency" : "1 Ghz",
    "replacement_policy" : "lru",
    "coherence_protocol" : "MSI",
    "associativity" : "4",
    "cache_line_size" : "64",
    #"debug" : "1",
    #"debug_level" : "10",
    "verbose" : 0,
    "L1" : "1",
    "cache_size" : "%dKiB"%options.cacheSz
})

comp_memory = sst.Component("memory", "memHierarchy.MemController")
comp_memory.addParams({
      "debug" : 1,
      "coherence_protocol" : "MSI",
      "debug_level" : 10,
      "backend.access_time" : "200 ns",
      "backing" : "malloc", 
      "clock" : "1GHz",
      "backend.mem_size" : "512MiB"
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
sst.enableAllStatisticsForComponentType("memHierarchy.Cache")


# Define the simulation links
link_gna_cache = sst.Link("link_gna_mem")
link_gna_cache.connect( (comp_gna, "mem_link", "1000ps"), (comp_l1cache, "high_network_0", "1000ps") )
link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link.connect( (comp_l1cache, "low_network_0", "50ps"), (comp_memory, "direct_link", "50ps") )


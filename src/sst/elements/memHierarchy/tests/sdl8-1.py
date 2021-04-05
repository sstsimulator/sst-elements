# Automatically generated SST Python input
import sst
from mhlib import componentlist

DEBUG_L1 = 0
DEBUG_L2 = 0
DEBUG_L3 = 0
DEBUG_DIR = 0
DEBUG_MEM = 0

# Define the simulation components
comp_cpu = sst.Component("cpu", "memHierarchy.trivialCPU")
comp_cpu.addParams({
      "memSize" : "0x100000",
      "num_loadstore" : "10000",
      "commFreq" : "100",
      "do_write" : "1"
})
iface = comp_cpu.setSubComponent("memory", "memHierarchy.memInterface")

comp_l1cache = sst.Component("l1cache", "memHierarchy.Cache")
comp_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : DEBUG_L1,
      "debug_level" : 10,
      "verbose" : 2,
})
l1ToC = comp_l1cache.setSubComponent("cpulink", "memHierarchy.MemLink")
l1Tol2 = comp_l1cache.setSubComponent("memlink", "memHierarchy.MemLink")

comp_l2cache = sst.Component("l2cache", "memHierarchy.Cache")
comp_l2cache.addParams({
      "access_latency_cycles" : "20",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "cache_size" : "32 KB",
      "debug" : DEBUG_L2,
      "debug_level" : 10,
      "verbose" : 2,
})
l2Tol1 = comp_l2cache.setSubComponent("cpulink", "memHierarchy.MemLink")
l2Tol3 = comp_l2cache.setSubComponent("memlink", "memHierarchy.MemLink")

l3cache = sst.Component("l3cache", "memHierarchy.Cache")
l3cache.addParams({
      "access_latency_cycles" : "100",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "16",
      "cache_line_size" : "64",
      "cache_size" : "64 KB",
      "debug" : DEBUG_L3,
      "debug_level" : 10,
      "verbose" : 2,
})
l3Tol2 = l3cache.setSubComponent("cpulink", "memHierarchy.MemLink")
l3NIC = l3cache.setSubComponent("memlink", "memHierarchy.MemNIC")
l3NIC.addParams({
      #"debug" : 1,
      #"debug_level" : 10,
      "network_bw" : "25GB/s",
      "group" : 1,
      "verbose" : 2,
})

comp_chiprtr = sst.Component("chiprtr", "merlin.hr_router")
comp_chiprtr.addParams({
      "xbar_bw" : "1GB/s",
      "link_bw" : "1GB/s",
      "input_buf_size" : "1KB",
      "num_ports" : "2",
      "flit_size" : "72B",
      "output_buf_size" : "1KB",
      "id" : "0",
      "topology" : "merlin.singlerouter"
})
comp_chiprtr.setSubComponent("topology","merlin.singlerouter")

comp_dirctrl = sst.Component("dirctrl", "memHierarchy.DirectoryController")
comp_dirctrl.addParams({
      "coherence_protocol" : "MSI",
      "debug" : DEBUG_DIR,
      "debug_level" : "10",
      "entry_cache_size" : "16384",
      "addr_range_end" : "0x1F000000",
      "addr_range_start" : "0x0",
      "verbose" : 2,
})
dirNIC = comp_dirctrl.setSubComponent("cpulink", "memHierarchy.MemNIC")
dirNIC.addParams({
      "network_bw" : "25GB/s",
      "group" : 2,
      "verbose" : 2,
      #"debug" : 1,
      #"debug_level" : 10,
})
dirMemLink = comp_dirctrl.setSubComponent("memlink", "memHierarchy.MemLink") # Not on a network, just a direct link

memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "debug" : DEBUG_MEM,
    "debug_level" : 10,
    "clock" : "1GHz",
    "verbose" : 2,
    "addr_range_end" : 512*1024*1024-1,
})
memToDir = memctrl.setSubComponent("cpulink", "memHierarchy.MemLink")
memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
      "access_time" : "100 ns",
      "mem_size" : "512MiB"
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
for a in componentlist:
    sst.enableAllStatisticsForComponentType(a)

# Define the simulation links
link_cpu_l1cache = sst.Link("link_cpu_l1cache")
link_cpu_l1cache.connect( (iface, "port", "1000ps"), (l1ToC, "port", "1000ps") )

link_l1cache_l2cache = sst.Link("link_l1cache_l2cache")
link_l1cache_l2cache.connect( (l1Tol2, "port", "10000ps"), (l2Tol1, "port", "10000ps") )

link_l2cache_l3cache = sst.Link("link_l2cache_l3cache")
link_l2cache_l3cache.connect( (l2Tol3, "port", "10000ps"), (l3Tol2, "port", "10000ps") )

link_cache_net = sst.Link("link_cache_net")
link_cache_net.connect( (l3NIC, "port", "10000ps"), (comp_chiprtr, "port1", "2000ps") )

link_dir_net = sst.Link("link_dir_net")
link_dir_net.connect( (comp_chiprtr, "port0", "2000ps"), (dirNIC, "port", "2000ps") )

link_dir_mem = sst.Link("link_dir_mem")
link_dir_mem.connect( (dirMemLink, "port", "10000ps"), (memToDir, "port", "10000ps") )


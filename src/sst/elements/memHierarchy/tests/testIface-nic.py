# Automatically generated SST Python input
import sst
from mhlib import componentlist

DEBUG_L1 = 1
DEBUG_MEM = 0
DEBUG_CORE = 0
DEBUG_NIC = 0
DEBUG_LEVEL = 10

debug_params = { "debug" : 1, "debug_level" : 10 }

# Define the simulation components
cpu = sst.Component("cpu", "memHierarchy.standardCPU")
cpu.addParams({
      "opCount" : "1000",
      "memFreq" : "8",
      "memSize" : "1KiB",
      "llsc_freq" : 10,
})
iface = cpu.setSubComponent("memory", "memHierarchy.standardInterface")
iface.addParams(debug_params)
cpu_nic = iface.setSubComponent("memlink", "memHierarchy.MemNIC")
cpu_nic.addParams({"group" : 0, "network_bw" : "25GB/s"})
#cpu_nic.addParams(debug_params)

l1cache = sst.Component("c0.l1cache", "memHierarchy.Cache")
l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : DEBUG_L1,
      "debug_level" : DEBUG_LEVEL
})
l1_nic = l1cache.setSubComponent("cpulink", "memHierarchy.MemNIC")
l1_nic.addParams({"group" : 1, "network_bw" : "25GB/s"})
#l1_nic.addParams(debug_params)

chiprtr = sst.Component("chiprtr", "merlin.hr_router")
chiprtr.addParams({
      "xbar_bw" : "1GB/s",
      "id" : "0",
      "input_buf_size" : "1KB",
      "num_ports" : "3",
      "flit_size" : "72B",
      "output_buf_size" : "1KB",
      "link_bw" : "1GB/s",
      "topology" : "merlin.singlerouter"
})
chiprtr.setSubComponent("topology","merlin.singlerouter")

memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "debug" : DEBUG_MEM,
    "debug_level" : DEBUG_LEVEL,
    "clock" : "1GHz",
    "addr_range_end" : 512*1024*1024-1,
})
mem_nic = memctrl.setSubComponent("cpulink", "memHierarchy.MemNIC")
mem_nic.addParams({"group" : 2, "network_bw" : "25GB/s"})
#mem_nic.addParams(debug_params)

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
#          cpu/cpu_nic
#                 |
#  l1/l1_nic - chiprtr - mem_nic/mem
#
link_cpu_rtr = sst.Link("link_cpu")
link_cpu_rtr.connect( (cpu_nic, "port", "1000ps"), (chiprtr, "port0", "1000ps") )

link_l1_rtr = sst.Link("link_l1")
link_l1_rtr.connect( (l1_nic, "port", '1000ps'), (chiprtr, "port1", "1000ps") )

link_mem_rtr = sst.Link("link_mem")
link_mem_rtr.connect( (mem_nic, "port", "1000ps"), (chiprtr, "port2", "1000ps") )

# Automatically generated SST Python input
import sst
from mhlib import componentlist

DEBUG_L1 = 1
DEBUG_MEM = 1
DEBUG_CORE = 1
DEBUG_DIR = 1
DEBUG_NIC = 1
DEBUG_LEVEL = 10

debug_params = { "debug" : 1, "debug_level" : 10 }

# On network: (Core, L1), MMIO device, (dir, memory)
# Logical communication: Core->L1->dir->memory
#                        Core->(L1)->MMIO
#                        MMIO->dir->memory    
l1_group = 1
mmio_group = 2
dir_group = 3

l1_dst = [mmio_group, dir_group]
mmio_src = [l1_group]
mmio_dst = [dir_group]
dir_src = [l1_group,mmio_group]

# Constans shared across components
network_bw = "25GB/s"
clock = "2GHz"
mmio_addr = 1024


# Define the simulation components
cpu = sst.Component("cpu", "memHierarchy.standardCPU")
cpu.addParams({
      "opCount" : "1000",
      "memFreq" : "4",
      "memSize" : "1KiB",
      "mmio_freq" : 15,
      "mmio_addr" : mmio_addr, # Just above memory addresses
      "clock" : clock,
      "verbose" : 3,
})
iface = cpu.setSubComponent("memory", "memHierarchy.standardInterface")
iface.addParams(debug_params)

l1cache = sst.Component("l1cache", "memHierarchy.Cache")
l1cache.addParams({
      "access_latency_cycles" : "2",
      "cache_frequency" : clock,
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "2 KB",
      "L1" : "1",
      "debug" : DEBUG_L1,
      "debug_level" : DEBUG_LEVEL
})
l1_link = l1cache.setSubComponent("cpulink", "memHierarchy.MemLink") # Non-network link
l1_nic = l1cache.setSubComponent("memlink", "memHierarchy.MemNIC")   # Network link
l1_nic.addParams({ "group" : l1_group, 
                   "destinations" : l1_dst,
                   "network_bw" : network_bw})
#l1_nic.addParams(debug_params)

mmio = sst.Component("mmio", "memHierarchy.mmioEx")
mmio.addParams({
      "verbose" : 3,
      "clock" : clock,
      "base_addr" : mmio_addr,
})
mmio_iface = mmio.setSubComponent("iface", "memHierarchy.standardInterface")
mmio_iface.addParams(debug_params)
mmio_nic = mmio_iface.setSubComponent("memlink", "memHierarchy.MemNIC")
mmio_nic.addParams({"group" : mmio_group, 
                    "sources" : mmio_src,
                    "destinations" : mmio_dst,
                    "network_bw" : network_bw })
#mmio_nic.addParams(debug_params)

dir = sst.Component("directory", "memHierarchy.DirectoryController")
dir.addParams({
      "clock" : clock,
      "entry_cache_size" : 16384,
      "mshr_num_entries" : 16,
      "addr_range_end" : mmio_addr - 1,
      "debug" : DEBUG_DIR,
      "debug_level" : 10,
})

dir_link = dir.setSubComponent("memlink", "memHierarchy.MemLink")
dir_nic = dir.setSubComponent("cpulink", "memHierarchy.MemNIC")
dir_nic.addParams({
      "group" : dir_group,
      "sources" : dir_src,
      "network_bw" : network_bw,
      "network_input_buffer_size" : "2KiB",
      "network_output_buffer_size" : "2KiB",
})

memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "debug" : DEBUG_MEM,
    "debug_level" : DEBUG_LEVEL,
    "clock" : "1GHz",
    "addr_range_end" : mmio_addr - 1,
})

memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
      "access_time" : "100 ns",
      "mem_size" : "512MiB"
})

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

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
for a in componentlist:
    sst.enableAllStatisticsForComponentType(a)


# Define the simulation links
#                cpu/l1/l1_nic
#                     |
#  mmio/mmio_nic - chiprtr - dir_nic/dir/mem
#
link_cpu_l1 = sst.Link("link_cpu")
link_cpu_l1.connect( (iface, "port", "1000ps"), (l1_link, "port", "1000ps") )

link_l1_rtr = sst.Link("link_l1")
link_l1_rtr.connect( (l1_nic, "port", '1000ps'), (chiprtr, "port0", "1000ps") )

link_mmio_rtr = sst.Link("link_mmio")
link_mmio_rtr.connect( (mmio_nic, "port", "500ps"), (chiprtr, "port1", "500ps"))

link_dir_rtr = sst.Link("link_dir")
link_dir_rtr.connect( (dir_nic, "port", "1000ps"), (chiprtr, "port2", "1000ps"))

link_dir_mem = sst.Link("link_mem")
link_dir_mem.connect( (dir_link, "port", "1000ps"), (memctrl, "direct_link", "1000ps") )

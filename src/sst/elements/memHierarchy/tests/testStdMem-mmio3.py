import sst
from mhlib import componentlist

DEBUG_L1 = 0
DEBUG_MEM = 0
DEBUG_CORE = 0
DEBUG_DIR = 0
DEBUG_OTHERS = 0
DEBUG_LEVEL = 10

debug_params = { "debug" : DEBUG_OTHERS, "debug_level" : 10 }

# On network: (Core, L1.0), (MMIO device, L1.1), (dir, memory)
# Logical communication: Core->L1.0->dir->memory
#                        Core->(L1.0)->(L1.1)->MMIO
#                        MMIO->L1.1->dir->memory    
core_group = 1
mmio_group = 2
dir_group = 3
mem_group = 4

l1_dst = [dir_group,mmio_group]
mmio_src = [core_group]
mmio_dst = [dir_group]
dir_src = [core_group,mmio_group]
dir_dst = [mem_group]
mem_src = [dir_group]

# Constants shared across components
network_bw = "25GB/s"
clock = "2GHz"
mmio_addr = 1024


# Define the simulation components
cpu = sst.Component("core", "memHierarchy.standardCPU")
cpu.addParams({
      "opCount" : "1000",
      "memFreq" : "4",
      "memSize" : "1KiB",
      "mmio_freq" : 15,
      "mmio_addr" : mmio_addr, # Just above memory addresses
      "clock" : clock,
      "verbose" : 3,
})
cpu_iface = cpu.setSubComponent("memory", "memHierarchy.standardInterface")
cpu_iface.addParams(debug_params)

cpu_link = cpu_iface.setSubComponent("memlink", "memHierarchy.MemLink")
cpu_link.addParams(debug_params)

cpu_l1cache = sst.Component("cpu_l1cache", "memHierarchy.Cache")
cpu_l1cache.addParams({
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
cpu_l1_link = cpu_l1cache.setSubComponent("cpulink", "memHierarchy.MemLink") # Non-network link
cpu_l1_link.addParams(debug_params)
cpu_l1_nic = cpu_l1cache.setSubComponent("memlink", "memHierarchy.MemNIC")   # Network link
cpu_l1_nic.addParams({ "group" : core_group, 
                   "destinations" : l1_dst,
                   "network_bw" : network_bw})
cpu_l1_nic.addParams(debug_params)

mmio = sst.Component("mmio", "memHierarchy.mmioEx")
mmio.addParams({
    "verbose" : 3,
    "clock" : clock,
    "base_addr" : mmio_addr,
    "mem_accesses" : 4,
    "max_addr" : "1023"
})
mmio_iface = mmio.setSubComponent("iface", "memHierarchy.standardInterface")
mmio_iface.addParams(debug_params)

mmio_link = mmio_iface.setSubComponent("memlink", "memHierarchy.MemLink")
mmio_link.addParams(debug_params)

mmio_l1cache = sst.Component("mmio_l1", "memHierarchy.Cache")
mmio_l1cache.addParams({
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

mmio_l1_link = mmio_l1cache.setSubComponent("cpulink", "memHierarchy.MemLink") # Non-network link from device to device's L1
mmio_l1_link.addParams(debug_params)
mmio_l1_nic = mmio_l1cache.setSubComponent("memlink", "memHierarchy.MemNIC") # Network link

mmio_l1_nic.addParams({"group" : mmio_group, 
                    "sources" : mmio_src,
                    "destinations" : mmio_dst,
                    "network_bw" : network_bw })
mmio_l1_nic.addParams(debug_params)

dir = sst.Component("directory", "memHierarchy.DirectoryController")
dir.addParams({
      "clock" : clock,
      "entry_cache_size" : 16384,
      "mshr_num_entries" : 16,
      "addr_range_end" : mmio_addr - 1,
      "debug" : DEBUG_DIR,
      "debug_level" : 10,
})

dir_nic = dir.setSubComponent("cpulink", "memHierarchy.MemNIC")
dir_nic.addParams({
      "group" : dir_group,
      "sources" : dir_src,
      "destinations" : dir_dst,
      "network_bw" : network_bw,
      "network_input_buffer_size" : "2KiB",
      "network_output_buffer_size" : "2KiB",
})
dir_nic.addParams(debug_params)

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

mem_nic = memctrl.setSubComponent("cpulink", "memHierarchy.MemNIC")
mem_nic.addParams(debug_params)
mem_nic.addParams({
    "group" : mem_group,
    "sources" : mem_src,
    "network_bw" : network_bw,
    "network_input_buffer_size" : "2KiB",
    "network_output_buffer_size" : "2KiB"
})

chiprtr = sst.Component("chiprtr", "merlin.hr_router")
chiprtr.addParams({
      "xbar_bw" : "1GB/s",
      "id" : "0",
      "input_buf_size" : "1KB",
      "num_ports" : "4",
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
#                cpu/l1/cpu_l1_nic
#                     |
#  mmio/l1/mmio_l1_nic - chiprtr - dir_nic/dir/mem
#
# Connect CPU to CPU L1 via the CPU's interface and the L1's cpulink handler
link_cpu_l1 = sst.Link("link_cpu")
link_cpu_l1.connect( (cpu_link, "port", "1000ps"), (cpu_l1_link, "port", "1000ps") )

# Connect the CPU L1 to the network via the L1's memlink NIC handler
link_core_rtr = sst.Link("link_core")
link_core_rtr.connect( (cpu_l1_nic, "port", '1000ps'), (chiprtr, "port0", "1000ps") )

# Connect MMIO to MMIO L1 via the MMIO's interface and the L1's cpulink handler
link_mmio_l1 = sst.Link("link_mmio")
link_mmio_l1.connect( (mmio_link, "port", "500ps"), (mmio_l1_link, "port", "500ps") )

# Connect the MMIO L1 to the network via the L1's memlink NIC handler
link_device_rtr = sst.Link("link_device")
link_device_rtr.connect( (mmio_l1_nic, "port", "500ps"), (chiprtr, "port1", "500ps"))

# Connect directory to the network via the directory's NIC handler
link_dir_rtr = sst.Link("link_dir")
link_dir_rtr.connect( (dir_nic, "port", "1000ps"), (chiprtr, "port2", "1000ps"))

# Connect directory to the memory
link_dir_mem = sst.Link("link_mem")
link_dir_mem.connect( (mem_nic, "port", "1000ps"), (chiprtr, "port3", "1000ps") )

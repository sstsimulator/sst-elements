# test configuration for using llyr as an mmio device
import sst

DEBUG_L1 = 1
DEBUG_MEM = 1
DEBUG_CORE = 1
DEBUG_DIR = 1
DEBUG_OTHERS = 1
DEBUG_LEVEL = 10

debug_params = { "debug" : DEBUG_OTHERS, "debug_level" : 10 }

# Define SST core options
sst.setProgramOption("timebase", "1 ps")
sst.setProgramOption("stopAtCycle", "10000s")

# Constants shared across components
network_bw = "25GB/s"
cpu_clk = 2.6
tile_clk_mhz = 1
backing_size = 16384
mem_size = 256
mmio_addr = 256
statLevel = 16

# Network
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

# Define the simulation components
cpu = sst.Component("cpu", "memHierarchy.standardCPU")
cpu.addParams({
      "opCount" : "100",
      "memFreq" : "4",
      "memSize" : str(mem_size) + "B",
      "mmio_freq" : 15,
      "mmio_addr" : mmio_addr, # Just above memory addresses
      "clock" : str(cpu_clk) + "GHz",
      "verbose" : 3,
})

cpu_iface = cpu.setSubComponent("memory", "memHierarchy.standardInterface")
cpu_iface.addParams(debug_params)

cpu_link = cpu_iface.setSubComponent("memlink", "memHierarchy.MemLink")
cpu_link.addParams(debug_params)

l1cache = sst.Component("l1cache", "memHierarchy.Cache")
l1cache.addParams({
      "access_latency_cycles" : "2",
      "cache_frequency" : str(cpu_clk) + "GHz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "2 KB",
      "L1" : "1",
      "debug" : DEBUG_L1,
      "debug_level" : DEBUG_LEVEL
})

cpu_l1_link = l1cache.setSubComponent("cpulink", "memHierarchy.MemLink") # Non-network link
cpu_l1_link.addParams(debug_params)

cpu_l1_nic = l1cache.setSubComponent("memlink", "memHierarchy.MemNIC")   # Network link
cpu_l1_nic.addParams({ "group" : core_group,
                       "destinations" : l1_dst,
                       "network_bw" : network_bw})
cpu_l1_nic.addParams(debug_params)


df_0 = sst.Component("df_0", "llyr.LlyrDataflow")
df_0.addParams({
    "verbose": 20,
    "clock" : str(tile_clk_mhz) + "GHz",
    "device_addr"   : mmio_addr,
    "mem_init"      : "int-1.mem",
    #"application"   : "conditional.in",
    "application"   : "gemm.in",
    #"application"   : "llvm_in/cdfg-example-02.ll",
    "hardware_graph": "hardware.cfg",
    "mapper"        : "llyr.mapper.simple"

})

df_iface = df_0.setSubComponent("iface", "memHierarchy.standardInterface")
df_iface.addParams(debug_params)

df_link = df_iface.setSubComponent("memlink", "memHierarchy.MemLink")
df_link.addParams(debug_params)

df_l1cache = sst.Component("df_l1", "memHierarchy.Cache")
df_l1cache.addParams({
      "access_latency_cycles" : "2",
      "cache_frequency" : str(tile_clk_mhz) + "GHz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "cache_size" : "512B",
      "associativity" : "1",
      "cache_line_size" : "16",
      "L1" : "1",
      "debug" : DEBUG_L1,
      "debug_level" : DEBUG_LEVEL
})

df_l1_link = df_l1cache.setSubComponent("cpulink", "memHierarchy.MemLink") # Non-network link from device to device's L1
df_l1_link.addParams(debug_params)

df_l1_nic = df_l1cache.setSubComponent("memlink", "memHierarchy.MemNIC") # Network link
df_l1_nic.addParams({"group" : mmio_group,
                    "sources" : mmio_src,
                    "destinations" : mmio_dst,
                    "network_bw" : network_bw })
df_l1_nic.addParams(debug_params)

dir = sst.Component("directory", "memHierarchy.DirectoryController")
dir.addParams({
      "clock" : str(tile_clk_mhz) + "GHz",
      "entry_cache_size" : 16384,
      "mshr_num_entries" : 16,
      "addr_range_start" : 0,
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
    "clock" : str(tile_clk_mhz) + "GHz",
    "addr_range_end" : mmio_addr - 1,
})

memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
      "access_time" : "100 ns",
      "mem_size" : str(backing_size) + "B",
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


# Enable SST Statistics Outputs for this simulation
sst.setStatisticLoadLevel(statLevel)
sst.enableAllStatisticsForAllComponents({"type":"sst.AccumulatorStatistic"})
sst.setStatisticOutput("sst.statOutputTXT", { "filepath" : "output.csv" })


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
link_df_l1 = sst.Link("link_mmio")
link_df_l1.connect( (df_link, "port", "500ps"), (df_l1_link, "port", "500ps") )

# Connect the MMIO L1 to the network via the L1's memlink NIC handler
link_device_rtr = sst.Link("link_device")
link_device_rtr.connect( (df_l1_nic, "port", "500ps"), (chiprtr, "port1", "500ps"))

# Connect directory to the network via the directory's NIC handler
link_dir_rtr = sst.Link("link_dir")
link_dir_rtr.connect( (dir_nic, "port", "1000ps"), (chiprtr, "port2", "1000ps"))

# Connect directory to the memory
link_dir_mem = sst.Link("link_mem")
link_dir_mem.connect( (mem_nic, "port", "1000ps"), (chiprtr, "port3", "1000ps") )







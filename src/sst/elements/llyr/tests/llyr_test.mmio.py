# test configuration for using llyr as an mmio device
import sst

DEBUG_L1 = 0
DEBUG_MEM = 0
DEBUG_CORE = 1
DEBUG_DIR = 0
DEBUG_LEVEL = 10

debug_params = { "debug" : 0, "debug_level" : 10 }

# Define SST core options
sst.setProgramOption("timebase", "1 ps")
sst.setProgramOption("stopAtCycle", "10000s")

# Constants shared across components
network_bw = "25GB/s"
cpu_clk = 2.6
tile_clk_mhz = 1
mem_size = 1024
mmio_addr = 1024
statLevel = 16

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
iface = cpu.setSubComponent("memory", "memHierarchy.standardInterface")
iface.addParams(debug_params)

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
l1_cpu = l1cache.setSubComponent("cpulink", "memHierarchy.MemLink")
l1_nic = l1cache.setSubComponent("memlink", "memHierarchy.MemNIC")
l1_nic.addParams({ "group" : l1_group,
                   "destinations" : l1_dst,
                   "network_bw" : network_bw})
#l1_nic.addParams(debug_params)


df_0 = sst.Component("df_0", "llyr.LlyrDataflow")
df_0.addParams({
    "verbose": 20,
    "clock" : str(tile_clk_mhz) + "GHz",
    "device_addr"   : mmio_addr,
    "mem_init"      : "spmm-mem.in",
    "application"   : "spmm.in",
    "hardware_graph": "hardware.cfg",
    "mapper"        : "llyr.mapper.simple"
})
df_l1 = df_0.setSubComponent("memory", "memHierarchy.standardInterface")

df_l1cache = sst.Component("df_l1cache", "memHierarchy.Cache")
df_l1cache.addParams({
    "access_latency_cycles" : "2",
    "cache_frequency" : str(tile_clk_mhz) + "GHz",
    "replacement_policy" : "lru",
    "coherence_protocol" : "MESI",
    "associativity" : "1",
    "cache_line_size" : "16",
    "verbose" : 10,
    "debug" : 1,
    "debug_level" : 100,
    "L1" : "1",
    "cache_size" : "512B"
})
mmio_iface = df_l1cache.setSubComponent("cpulink", "memHierarchy.MemLink")
mmio_nic = df_l1cache.setSubComponent("memlink", "memHierarchy.MemNIC")
mmio_nic.addParams({"group" : mmio_group,
                    "sources" : mmio_src,
                    "destinations" : mmio_dst,
                    "network_bw" : network_bw })
#mmio_nic.addParams(debug_params)

dir = sst.Component("directory", "memHierarchy.DirectoryController")
dir.addParams({
      "clock" : str(cpu_clk) + "GHz",
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

# Enable SST Statistics Outputs for this simulation
sst.setStatisticLoadLevel(statLevel)
sst.enableAllStatisticsForAllComponents({"type":"sst.AccumulatorStatistic"})
sst.setStatisticOutput("sst.statOutputTXT", { "filepath" : "output.csv" })


# Define the simulation links
#                cpu/l1/l1_nic
#                     |
#  mmio/mmio_nic - chiprtr - dir_nic/dir/mem
#
link_cpu_l1 = sst.Link("link_cpu")
link_cpu_l1.connect( (iface, "port", "1000ps"), (l1_cpu, "port", "1000ps") )

link_l1_rtr = sst.Link("link_l1")
link_l1_rtr.connect( (l1_nic, "port", '1000ps'), (chiprtr, "port0", "1000ps") )

link_df_l1 = sst.Link("link_df0")
link_df_l1.connect( (df_l1, "port", "1000ps"), (mmio_iface, "port", "1000ps") )

link_dfl1_rtr = sst.Link("link_df_l1")
link_dfl1_rtr.connect( (mmio_nic, "port", '1000ps'), (chiprtr, "port1", "1000ps") )

link_dir_rtr = sst.Link("link_dir")
link_dir_rtr.connect( (dir_nic, "port", "1000ps"), (chiprtr, "port2", "1000ps"))

link_dir_mem = sst.Link("link_mem")
link_dir_mem.connect( (dir_link, "port", "1000ps"), (memctrl, "direct_link", "1000ps") )





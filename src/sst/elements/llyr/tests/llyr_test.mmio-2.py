# test configuration for using llyr as an mmio device
import sst

DEBUG_L1 = 1
DEBUG_MEM = 1
DEBUG_CORE = 1
DEBUG_NIC = 1
DEBUG_LEVEL = 10

debug_params = { "debug" : 0, "debug_level" : 10 }

# Define SST core options
sst.setProgramOption("timebase", "1 ps")
sst.setProgramOption("stopAtCycle", "10000s")

# Constants shared across components
network_bw = "25GB/s"
cpu_clk = 2.6
tile_clk_mhz = 1
mem_size = 256
mmio_addr = 256
statLevel = 16


# On network: Core, L1, MMIO device, memory
# Logical communication: Core->L1->memory
#                        Core->MMIO
#                        MMIO->memory
core_group = 0
l1_group = 1
mmio_group = 2
memory_group = 3

core_dst = [l1_group, mmio_group]
l1_src = [core_group]
l1_dst = [memory_group]
mmio_src = [core_group]
mmio_dst = [memory_group]
memory_src = [l1_group,mmio_group]

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
cpu_nic = iface.setSubComponent("memlink", "memHierarchy.MemNIC")
cpu_nic.addParams({"group" : core_group,
                   "destinations" : core_dst,
                   "network_bw" : network_bw})

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
      "addr_range_start" : 0,
      "addr_range_end" : mmio_addr - 1,
      "debug" : DEBUG_L1,
      "debug_level" : DEBUG_LEVEL
})
l1_nic = l1cache.setSubComponent("cpulink", "memHierarchy.MemNIC")
l1_nic.addParams({ "group" : l1_group,
                   "sources" : l1_src,
                   "destinations" : l1_dst,
                   "network_bw" : network_bw})
#l1_nic.addParams(debug_params)


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
mmio_iface = df_0.setSubComponent("iface", "memHierarchy.standardInterface")
#mmio_iface.addParams(debug_params)
mmio_nic = mmio_iface.setSubComponent("memlink", "memHierarchy.MemNIC")
mmio_nic.addParams({"group" : mmio_group,
                    "sources" : mmio_src,
                    "destinations" : mmio_dst,
                    "network_bw" : network_bw })
#mmio_nic.addParams(debug_params)

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

memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "debug" : DEBUG_MEM,
    "debug_level" : DEBUG_LEVEL,
    "clock" : "1GHz",
    "addr_range_end" : mmio_addr - 1,
})
mem_nic = memctrl.setSubComponent("cpulink", "memHierarchy.MemNIC")
mem_nic.addParams({"group" : memory_group,
                   "sources" : "[1,2]", # Group 1 = L1, Group 2 = MMIO
                   "network_bw" : network_bw})
#mem_nic.addParams(debug_params)

memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
      "access_time" : "100 ns",
      "mem_size" : "512MiB"
})

# Enable SST Statistics Outputs for this simulation
sst.setStatisticLoadLevel(statLevel)
sst.enableAllStatisticsForAllComponents({"type":"sst.AccumulatorStatistic"})
sst.setStatisticOutput("sst.statOutputTXT", { "filepath" : "output.csv" })


# Define the simulation links
#          cpu/cpu_nic
#                 |
#  l1/l1_nic - chiprtr - mem_nic/mem
#
link_cpu_rtr = sst.Link("link_cpu")
link_cpu_rtr.connect( (cpu_nic, "port", "1000ps"), (chiprtr, "port0", "1000ps") )

link_l1_rtr = sst.Link("link_l1")
link_l1_rtr.connect( (l1_nic, "port", '1000ps'), (chiprtr, "port1", "1000ps") )

link_mmio_rtr = sst.Link("link_mmio")
link_mmio_rtr.connect( (mmio_nic, "port", "500ps"), (chiprtr, "port2", "500ps"))

link_mem_rtr = sst.Link("link_mem")
link_mem_rtr.connect( (mem_nic, "port", "1000ps"), (chiprtr, "port3", "1000ps") )





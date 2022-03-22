# Automatically generated SST Python input
import sst
try:
    import ConfigParser
except ImportError:
    import configparser as ConfigParser
import argparse
from utils import *
# from mhlib import componentlist

# Arguments
parser = argparse.ArgumentParser()
parser.add_argument("-c", "--config", help="specify configuration file", required=True)
parser.add_argument("-v", "--verbose", help="increase verbosity of output", action="store_true")
parser.add_argument("-s", "--statfile", help="statistics file", default="./stats.out")
parser.add_argument("-l", "--statlevel", help="statistics level", type=int, default=16)
parser.add_argument("-x", "--binary", help="specify input binary", default="")
parser.add_argument("-a", "--arguments", help="colon sep binary arguments", default="")

args = parser.parse_args()

verbose = args.verbose
cfgFile = args.config
statFile = args.statfile
statLevel = args.statlevel
binaryFile = args.binary
binaryArgs = args.arguments

# Build Configuration Information
config = Config(cfgFile, verbose=verbose)

DEBUG_L1 = 0
DEBUG_MEM = 0
DEBUG_CORE = 0
DEBUG_NIC = 0
DEBUG_LEVEL = 10

debug_params = { "debug" : 0, "debug_level" : 10 }

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

# Constans shared across components
network_bw = "25GB/s"
clock = "2GHz"
mmio_addr = 1024


# Define the simulation components
cpu = sst.Component("cpu", "balar.balarTestCPU")
cpu.addParams({
      "opCount" : "1000",
      "memFreq" : "4",
      "memSize" : "1KiB",
      "clock" : clock,
      "verbose" : 3,
      "mmio_addr" : mmio_addr, # Just above memory addresses
      "gpu_addr": mmio_addr,
      
      "read_freq" : 0,
      "write_freq" : 0,
      "flush_freq" : 0,
      "flushinv_freq" : 0,
      "custom_freq" : 0,
      "llsc_freq" : 0,
      "mmio_freq" : 0,
      "gpu_freq" : 100,
})
iface = cpu.setSubComponent("memory", "memHierarchy.standardInterface")
iface.addParams(debug_params)
cpu_nic = iface.setSubComponent("memlink", "memHierarchy.MemNIC")
cpu_nic.addParams({"group" : core_group, 
                   "destinations" : core_dst,
                   "network_bw" : network_bw})
#cpu_nic.addParams(debug_params)

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

mmio = sst.Component("balar", "balar.balarMMIO")
mmio.addParams({
      "verbose" : 3,
      # "clock" : clock,
      "base_addr" : mmio_addr,
})
mmio.addParams(config.getGPUConfig())

mmio_iface = mmio.setSubComponent("iface", "memHierarchy.standardInterface")
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

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
# for a in componentlist:
#     sst.enableAllStatisticsForComponentType(a)


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

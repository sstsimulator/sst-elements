# Automatically generated SST Python input
# Run script: sst testBalar-simple.py --model-options="-c ariel-gpu-v100.cfg -v" > tmp.out 2>&1
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
parser.add_argument("-t", "--trace", help="CUDA api calls trace file path", default="cuda_calls.trace")
parser.add_argument("-x", "--binary", help="specify input cuda binary", default="")
parser.add_argument("-a", "--arguments", help="colon sep binary arguments", default="")


args = parser.parse_args()

verbose = args.verbose
cfgFile = args.config
statFile = args.statfile
statLevel = args.statlevel
traceFile = args.trace
binaryFile = args.binary
binaryArgs = args.arguments

# Build Configuration Information
config = Config(cfgFile, verbose=verbose)

DEBUG_L1 = 0
DEBUG_MEM = 0
DEBUG_CORE = 0
DEBUG_NIC = 0
DEBUG_LEVEL = 10

debug_params = { "debug" : 1, "debug_level" : 10 }

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


# Test CPU components and mem hierachy
cpu = sst.Component("cpu", "balar.BalarTestCPU")
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

      # Trace and executable info
      "trace_file": traceFile,
      "cuda_executable": binaryFile,
      "enable_memcpy_dump": False
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
      "clock" : clock,
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

# Define the simulation links for CPU
#          cpu/cpu_nic
#                 |
#  l1/l1_nic - chiprtr - mem_nic/mem/mmio
#
link_cpu_rtr = sst.Link("link_cpu")
link_cpu_rtr.connect( (cpu_nic, "port", "1000ps"), (chiprtr, "port0", "1000ps") )

link_l1_rtr = sst.Link("link_l1")
link_l1_rtr.connect( (l1_nic, "port", '1000ps'), (chiprtr, "port1", "1000ps") )

link_mmio_rtr = sst.Link("link_mmio")
link_mmio_rtr.connect( (mmio_nic, "port", "500ps"), (chiprtr, "port2", "500ps"))

link_mem_rtr = sst.Link("link_mem")
link_mem_rtr.connect( (mem_nic, "port", "1000ps"), (chiprtr, "port3", "1000ps") )

# GPU Memory hierarchy
#          mmio/GPU
#           |
#           L1
#           |
#           L2
#           |
#           mem port
#           
# GPU Memory hierarchy configuration
print ("Configuring GPU Network-on-Chip...")

gpu_router_ports = config.gpu_cores + config.gpu_l2_parts

GPUrouter = sst.Component("gpu_xbar", "shogun.ShogunXBar")
GPUrouter.addParams(config.getGPUXBarParams())
GPUrouter.addParams({
      "verbose" : 1,
      "port_count" : gpu_router_ports,
})

# Connect Cores & l1caches to the router
for next_core_id in range(config.gpu_cores):
    print ("Configuring GPU core %d..."%next_core_id)

    gpuPort = "requestGPUCacheLink%d"%next_core_id

    l1g = sst.Component("l1gcache_%d"%(next_core_id), "memHierarchy.Cache")
    l1g.addParams(config.getGPUL1Params())
    l1g.addParams(debug_params)

    connect("gpu_cache_link_%d"%next_core_id,
            mmio, gpuPort,
            l1g, "high_network_0",
            config.default_link_latency).setNoCut()

    connect("l1gcache_%d_link"%next_core_id,
            l1g, "cache",
            GPUrouter, "port%d"%next_core_id,
            config.default_link_latency)

# Connect GPU L2 caches to the routers
num_L2s_per_stack = config.gpu_l2_parts // config.hbmStacks
sub_mems = config.gpu_memory_controllers
total_mems = config.hbmStacks * sub_mems

interleaving = 256
next_mem = 0
cacheStartAddr = 0x00
memStartAddr = 0x00

if (config.gpu_l2_parts % total_mems) != 0:
   print ("FAIL Number of L2s (%d) must be a multiple of the total number memory controllers (%d)."%(config.gpu_l2_parts, total_mems))
   raise SystemExit

for next_group_id in range(config.hbmStacks):
   next_cache = next_group_id * sub_mems

   mem_l2_bus = sst.Component("mem_l2_bus_" + str(next_group_id), "memHierarchy.Bus")
   mem_l2_bus.addParams({
      "bus_frequency" : "4GHz",
      "drain_bus" : "1"
   })

   for sub_group_id in range(sub_mems):
      memStartAddr = 0 + (256 * next_mem)
      endAddr = memStartAddr + config.gpu_memory_capacity_inB - (256 * total_mems)
        
      if backend == "simple":
         # Create SimpleMem
         print ("Configuring Simple mem part" + str(next_mem) + " out of " + str(config.hbmStacks) + "...")
         mem = sst.Component("Simplehbm_" + str(next_mem), "memHierarchy.MemController")
         mem.addParams(config.get_GPU_mem_params(total_mems, memStartAddr, endAddr))
         membk = mem.setSubComponent("backend", "memHierarchy.simpleMem")
         membk.addParams(config.get_GPU_simple_mem_params())
      elif backend == "ddr":
         # Create DDR (Simple)
         print ("Configuring DDR-Simple mem part" + str(next_mem) + " out of " + str(config.hbmStacks) + "...")
         mem = sst.Component("DDR-shbm_" + str(next_mem), "memHierarchy.MemController")
         mem.addParams(config.get_GPU_ddr_memctrl_params(total_mems, memStartAddr, endAddr))
         membk = mem.setSubComponent("backend", "memHierarchy.simpleDRAM")
         membk.addParams(config.get_GPU_simple_ddr_params(next_mem))
      elif backend == "timing":
         # Create DDR (Timing)
         print ("Configuring DDR-Timing mem part" + str(next_mem) + " out of " + str(config.hbmStacks) + "...")
         mem = sst.Component("DDR-thbm_" + str(next_mem), "memHierarchy.MemController")
         mem.addParams(config.get_GPU_ddr_memctrl_params(total_mems, memStartAddr, endAddr))
         membk = mem.setSubComponent("backend", "memHierarchy.timingDRAM")
         membk.addParams(config.get_GPU_ddr_timing_params(next_mem))
      else :
         # Create CramSim HBM
         print ("Creating HBM controller " + str(next_mem) + " out of " + str(config.hbmStacks) + "...")

         mem = sst.Component("GPUhbm_" + str(next_mem), "memHierarchy.MemController")
         mem.addParams(config.get_GPU_hbm_memctrl_cramsim_params(total_mems, memStartAddr, endAddr))

         cramsimBridge = sst.Component("hbm_cs_bridge_" + str(next_mem), "CramSim.c_MemhBridge")
         cramsimCtrl = sst.Component("hbm_cs_ctrl_" + str(next_mem), "CramSim.c_Controller")
         cramsimDimm = sst.Component("hbm_cs_dimm_" + str(next_mem), "CramSim.c_Dimm")

         cramsimBridge.addParams(config.get_GPU_hbm_cramsim_bridge_params(next_mem))
         cramsimCtrl.addParams(config.get_GPU_hbm_cramsim_ctrl_params(next_mem))
         cramsimDimm.addParams(config.get_GPU_hbm_cramsim_dimm_params(next_mem))

         linkMemBridge = sst.Link("memctrl_cramsim_link_" + str(next_mem))
         linkMemBridge.connect( (mem, "cube_link", "2ns"), (cramsimBridge, "cpuLink", "2ns") )
         linkBridgeCtrl = sst.Link("cramsim_bridge_link_" + str(next_mem))
         linkBridgeCtrl.connect( (cramsimBridge, "memLink", "1ns"), (cramsimCtrl, "txngenLink", "1ns") )
         linkDimmCtrl = sst.Link("cramsim_dimm_link_" + str(next_mem))
         linkDimmCtrl.connect( (cramsimCtrl, "memLink", "1ns"), (cramsimDimm, "ctrlLink", "1ns") )

      print (" - Capacity: " + str(config.gpu_memory_capacity_inB // config.hbmStacks) + " per HBM")
      print (" - Start Address: " + str(hex(memStartAddr)) + " End Address: " + str(hex(endAddr)))
        
      connect("bus_mem_link_%d"%next_mem,
      mem_l2_bus, "low_network_%d"%sub_group_id,
      mem, "direct_link",
      "50ps").setNoCut()

      next_mem = next_mem + 1

   for next_mem_id in range(num_L2s_per_stack):
      cacheStartAddr = 0 + (256 * next_cache)
      endAddr = cacheStartAddr + config.gpu_memory_capacity_inB - (256 * total_mems)

      print ("Creating L2 controller %d-%d (%d)..."%(next_group_id, next_mem_id, next_cache))
      print (" - Start Address: " + str(hex(cacheStartAddr)) + " End Address: " + str(hex(endAddr)))

      l2g = sst.Component("l2gcache_%d"%(next_cache), "memHierarchy.Cache")
      l2g.addParams(config.getGPUL2Params(cacheStartAddr, endAddr))

      connect("l2g_xbar_link_%d"%(next_cache),
      GPUrouter, "port%d"%(config.gpu_cores+(next_cache)),
      l2g, "cache",
      config.default_link_latency).setNoCut()

      connect("l2g_mem_link_%d"%(next_cache),
      l2g, "low_network_0",
      mem_l2_bus, "high_network_%d"%(next_mem_id),
      config.default_link_latency).setNoCut()

      print ("++ %d-%d (%d)..."%(next_cache, sub_mems, (next_cache + 1) % sub_mems))
      if (next_cache + 1) % sub_mems == 0:
         next_cache = next_cache + total_mems - (sub_mems - 1)
      else:
         next_cache = next_cache + 1



# ===========================================================
# Enable statistics
sst.setStatisticLoadLevel(statLevel)
sst.enableAllStatisticsForAllComponents({"type":"sst.AccumulatorStatistic"})
sst.setStatisticOutput("sst.statOutputTXT", { "filepath" : statFile })

print ("Completed configuring the cuda-test model")

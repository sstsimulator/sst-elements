import sst
import sys
try:
    import ConfigParser
except ImportError:
    import configparser as ConfigParser
import argparse
from utils import *


# Parse commandline arguments
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

if 'ariel' in config.app:
   arielCPU = sst.Component("A0", "ariel.ariel")
   arielCPU.addParams(config.getCoreConfig(0))
   if binaryFile == "":
      myExec = config.executable
   else:
      myExec = binaryFile

   argList = binaryArgs.split(" ")

   print ("Num args " + str(len(argList)))
   print ("0 " + str(argList[0]))
   if len(argList) == 0:
      print ("No args")
      arielCPU.addParams({
         "executable" : myExec,
         })
   else:
      myArgs = "--" + str(argList[0])
      print ("App args " + str(myArgs))
      arielCPU.addParams({
         "executable" : myExec,
         "appargcount" : str(len(argList)),
         "apparg0" : myArgs,
         })

print ("Configuring CPU Network-on-Chip...")

#router = sst.Component("router", "merlin.hr_router")
#router.addParams(config.getRouterParams())
#router.addParam('id', 0)

router_ports = config.cpu_cores + 2

router = sst.Component("cpu_xbar", "shogun.ShogunXBar")
router.addParams(config.getXBarParams())
router.addParams({
      "verbose" : 1,
      "port_count" : router_ports,
})

# Add GPGPU-sim Component
gpu = sst.Component("gpu0", "balar.balar")
params_gpu = config.getGPUConfig()
params_gpu["verbose"] = 20
gpu.addParams(params_gpu)
# gpu.addParams(config.getGPUConfig())

# Configure CPU mem hirerchy
# Connect Cores & caches
for next_core_id in range(config.cpu_cores):
    print ("Configuring CPU core %d..."%next_core_id)

    if 'miranda' in config.app:
        cpu = sst.Component("cpu%d"%(next_core_id), "miranda.BaseCPU")
        cpu.addParams(config.getCoreConfig(next_core_id))
        cpuPort = "cache_link"
    elif 'ariel' in config.app:
        cpu = arielCPU
        cpuPort = "cache_link_%d"%next_core_id
        gpuPort = "requestMemLink%d"%next_core_id

    l1 = sst.Component("l1cache_%d"%(next_core_id), "memHierarchy.Cache")
    l1.addParams(config.getL1Params())

    l2 = sst.Component("l2cache_%d"%(next_core_id), "memHierarchy.Cache")
    l2.addParams(config.getL2Params())

    l1g = sst.Component("l1PCIcache_%d"%(next_core_id), "memHierarchy.Cache")
    l1g.addParams(config.getL1Params())

    l2g = sst.Component("l2PCIcache_%d"%(next_core_id), "memHierarchy.Cache")
    l2g.addParams(config.getL2Params())

    connect("cpu_cache_link_%d"%next_core_id,
            cpu, cpuPort,
            l1, "high_network_0",
            config.default_link_latency).setNoCut()

    connect("l2cache_%d_link"%next_core_id,
            l1, "low_network_0",
            l2, "high_network_0",
            config.default_link_latency).setNoCut()

    connect("l2_ring_link_%d"%next_core_id,
            l2, "directory",
            router, "port%d"%next_core_id,
            config.default_link_latency).setNoCut()

# Connect CPU Memory and Memory Controller to the ring
mem = sst.Component("memory", "memHierarchy.MemController")
mem.addParams(config.getMemCtrlParams())
membk = mem.setSubComponent("backend", "memHierarchy.simpleMem")
membk.addParams(config.getMemBkParams())

dc = sst.Component("dc", "memHierarchy.DirectoryController")
dc.addParams(config.getDCParams(0))

connect("mem_link_0",
        mem, "direct_link",
        dc, "memory",
        config.default_link_latency).setNoCut()

connect("dc_link_0",
        dc, "network",
        router, "port%d"%(config.cpu_cores+1),
        config.default_link_latency).setNoCut()


#Connect CPU with GPU using PCI
connect("gpu_PCI_link_%d"%next_core_id,
        gpu, "requestMemLink%d"%next_core_id,
        l1g, "high_network_0",
        config.gpu_cpu_latency).setNoCut()

#Assume we have PCI caches
connect("PCI_l1g_%d_link"%next_core_id,
        l1g, "low_network_0",
        l2g, "high_network_0",
        config.default_link_latency).setNoCut()

connect("PCI_l1g_l2g_ring_link_%d"%next_core_id,
        l2g, "directory",
        router, "port%d"%(config.cpu_cores),
        config.default_link_latency)

connect("cpu_gpu_command_link_%d"%next_core_id,
        cpu, "gpu_link_%d"%next_core_id,
        gpu, "requestLink%d"%(next_core_id),
        config.default_link_latency).setNoCut()


# Configure GPU mem hierarchy

print ("Configuring GPU Network-on-Chip...")

#GPUrouter = sst.Component("GPUrouter", "merlin.hr_router")
#GPUrouter.addParams(config.getGPURouterParams())
#GPUrouter.addParam('id', 1)

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

    connect("gpu_cache_link_%d"%next_core_id,
            gpu, gpuPort,
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


# ===============================================================================

# Enable SST Statistics Outputs for this simulation
sst.setStatisticLoadLevel(statLevel)
sst.enableAllStatisticsForAllComponents({"type":"sst.AccumulatorStatistic"})
sst.setStatisticOutput("sst.statOutputTXT", { "filepath" : statFile })

print ("Completed configuring the cuda-test model")


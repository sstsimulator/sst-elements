import sst
import sys
import ConfigParser, argparse

# Define SST core options
sst.setProgramOption("timebase", "1ps")
sst.setProgramOption("stopAtCycle", "0 ns")

# Tell SST what statistics handling we want
sst.setStatisticLoadLevel(4)

globalDebug = 0
globalVerbose = 10
globalLevel = 10

memDebug = 0
memVerbose = 0
memLevel = 10

core_clock = "2100MHz"

mirandaVerbose = 0
miranda_stride = 64
miranda_req_per_cycle = 4
miranda_oustanding_reqs = 64

#backend = "hbm"  ## hbm or simple
backend = "simple"
memory_mb = 16384
memory_capacity_inB = memory_mb * 1024 * 1024
memory_clock = "877MHz"

#
num_cpu = 3
num_l2 = 4
hbmRows = 16384
hbmChan = 8
hbmStacks = 2
memoryControllers = 2

router_ports = num_cpu + num_l2
next_port = 0

router = sst.Component("cpu_xbar", "shogun.ShogunXBar")
router.addParams({
   "verbose" : "0",
   "buffer_depth" : "64",
   "clock" : "1200MHz",
   "in_msg_per_cycle" : "1",
   "out_msg_per_cycle" : "1",
   "port_count" : router_ports,
})

for cpu_id in range(num_cpu):
   # Define the simulation components
   maxAddr = (memory_capacity_inB / num_cpu)
   addr_a = (cpu_id * int(maxAddr))

   comp_cpu = sst.Component("cpu_%d"%cpu_id, "miranda.BaseCPU")
   comp_cpu.addParams({
         "verbose" : mirandaVerbose,
         "clock"     : core_clock,
         "max_reqs_cycle" : miranda_req_per_cycle,
         "maxmemreqpending" : miranda_oustanding_reqs,
         "generator" : "miranda.SingleStreamGenerator",
         "generatorParams.verbose" : mirandaVerbose,
         "generatorParams.count" : 200000,
         "generatorParams.length" : int(miranda_stride),
         "generatorParams.startat" : int(addr_a),
         "generatorParams.max_address" : int(addr_a + int(maxAddr))
   })

   l1_cache = sst.Component("l1cache_%d"%(cpu_id), "memHierarchy.Cache")
   l1_cache.addParams({
      "verbose" : "0",
      "debug" : globalDebug,
      "debug_level" : globalLevel,
      "L1" : "1",
      "access_latency_cycles" : "4",
      "associativity" : "8",
      "cache_frequency" : "1200MHz",
      "cache_line_size" : "64",
      "cache_size" : "32KB",
      "memNIC.network_link_control" : "shogun.ShogunNIC",
   })

   corel1link = sst.Link("cpu_l1_link_" + str(cpu_id))
   corel1link.connect((comp_cpu, "cache_link", "100ps"), (l1_cache, "high_network_0", "100ps"))
   corel1link.setNoCut()

   l1xbarlink = sst.Link("l1_xbar_link_" + str(cpu_id))
   l1xbarlink.connect((l1_cache, "cache", "100ps"), (router, "port" + str(next_port), "100ps"))
   l1xbarlink.setNoCut()

   next_port = next_port + 1

# Connect L2 caches to the routers
num_L2s_per_stack = num_l2 / hbmStacks
sub_mems = memoryControllers
total_mems = hbmStacks * sub_mems

interleaving = 256
next_mem = 0
cacheStartAddr = 0x00
memStartAddr = 0x00

if (num_l2 % total_mems) != 0:
   print "FAIL Number of L2s (%d) must be a multiple of the total number memory controllers (%d)."%(num_l2, total_mems)
   raise SystemExit

for next_group_id in range(hbmStacks):
   next_cache = next_group_id * sub_mems

   mem_l2_bus = sst.Component("mem_l2_bus_" + str(next_group_id), "memHierarchy.Bus")
   mem_l2_bus.addParams({
      "bus_frequency" : "4GHz",
      "drain_bus" : "1"
   })

   for sub_group_id in range(sub_mems):
      memStartAddr = 0 + (256 * next_mem)
      endAddr = memStartAddr + memory_capacity_inB - (256 * total_mems)

      if backend == "simple":
         # Create DDR (Simple)
         mem = sst.Component("Simplehbm_" + str(next_mem), "memHierarchy.MemController")
         mem.addParams({
            "verbose" : 2,
            "debug_level" : 0,
            "debug_location" : 1,
            "backend" : "memHierarchy.simpleMem",
            "backend.access_time" : "30ns",
            "backend.mem_size" : str(memory_capacity_inB) + "B",
            "backend.debug_location" : 1,
            "backend.debug_level" : 0,
            "clock" : memory_clock,
            "max_requests_per_cycle": "8",
            "backing" : "none",
            "cpulink.accept_region" : "0",
            "cpulink.addr_range_end" : endAddr,
            "cpulink.addr_range_start" : memStartAddr,
            "cpulink.data.network_bw" : "32GB/s",
            "cpulink.fwd.network_bw"  : "32GB/s",
            "cpulink.req.network_bw"  : "32GB/s",
            "cpulink.ack.network_bw"  : "32GB/s",
            "cpulink.interleave_size" : "256B",
            "cpulink.interleave_step" : str(total_mems * 256) + "B",
            })
      else:
         # Create CramSim HBM
         mem = sst.Component("GPUhbm_" + str(next_mem), "memHierarchy.MemController")
         mem.addParams({
            "backend" : "memHierarchy.cramsim",
            "backend.access_time" : "1ns",
            "backend.max_outstanding_requests" : hbmChan * 128 * 100,
            "backend.mem_size" : str(memory_capacity_inB) + "B",
            "backend.request_width" : "64",
            "backing" : "none",
            "clock" : memory_clock,
            "cpulink.accept_region" : "0",
            "cpulink.addr_range_end" : endAddr,
            "cpulink.addr_range_start" : memStartAddr,
            "cpulink.data.network_bw" : "32GB/s",
            "cpulink.fwd.network_bw"  : "32GB/s",
            "cpulink.req.network_bw"  : "32GB/s",
            "cpulink.ack.network_bw"  : "32GB/s",
            "cpulink.interleave_size" : "256B",
            "cpulink.interleave_step" : str(total_mems * 256) + "B",
            "debug" : memDebug,
            "debug_level" : memLevel,
            "verbose" : memVerbose,
            "max_requests_per_cycle" : "-1",
         })

         cramsimBridge = sst.Component("hbm_cs_bridge_" + str(next_mem), "CramSim.c_MemhBridge")
         cramsimCtrl = sst.Component("hbm_cs_ctrl_" + str(next_mem), "CramSim.c_Controller")
         cramsimDimm = sst.Component("hbm_cs_dimm_" + str(next_mem), "CramSim.c_Dimm")

         cramsimBridge.addParams({
            "debug" : memDebug,
            "debug_level" : memLevel,
            "verbose" : memVerbose,
            "numTxnPerCycle" : hbmChan, # NumChannels
            "numBytesPerTransaction" : 32,
            "strControllerClockFrequency" : memory_clock,
            "maxOutstandingReqs" : hbmChan * 128 * 100, # NumChannels * 64 per channel
            "readWriteRatio" : "0.667",# Required but unused since we're driving with Ariel
            "randomSeed" : 0,
            "mode" : "seq",
         })

         cramsimCtrl.addParams({
            "debug" : memDebug,
            "debug_level" : memLevel,
            "verbose" : memVerbose,
            "TxnConverter" : "CramSim.c_TxnConverter",
            "AddrMapper"   : "CramSim.c_AddressHasher",
            "CmdScheduler" : "CramSim.c_CmdScheduler",
            "DeviceDriver" : "CramSim.c_DeviceDriver",
            "TxnScheduler" : "CramSim.c_TxnScheduler",
            "strControllerClockFrequency" : memory_clock,
            "boolEnableQuickRes" : 0,
            ### TxnConverter ###
            "relCommandWidth" : 1,
            "boolUseReadA" : 0,
            "boolUseWriteA" : 0,
            "bankPolicy" : "OPEN",
            ### AddressHasher ###
            "strAddressMapStr" : "r:14_b:2_B:2_l:3_C:3_l:3_c:1_h:6_",
            "numBytesPerTransaction" : 64,
            ### CmdScheduler ###
            "numCmdQEntries" : 32,
            "cmdSchedulingPolicy" : "BANK",
            ### DeviceDriver ###
            "boolDualCommandBus" : 1,
            "boolMultiCycleACT" : 1,
            "numChannels" : hbmChan,
            "numPChannelsPerChannel" : 2,
            "numRanksPerChannel" : 1,
            "numBankGroupsPerRank" : 4,
            "numBanksPerBankGroup" : 4,
            "numRowsPerBank" : hbmRows,
            "numColsPerBank" : 64,
            "boolUseRefresh" : 1,
            "boolUseSBRefresh" : 0,
            ##NOTE: This DRAM timing is for 850 MHZ HBMm if you change the mem freq, ensure to scale these timings as well.
            "nRC" : 40,
            "nRRD" : 3,
            "nRRD_L" : 3,
            "nRRD_S" : 2,
            "nRCD" : 12,
            "nCCD" : 2,
            "nCCD_L" : 4,
            "nCCD_L_WR" : 2,
            "nCCD_S" : 2,
            "nAL" : 0,
            "nCL" : 12,
            "nCWL" : 4,
            "nWR" : 10,
            "nWTR" : 2,
            "nWTR_L" : 6,
            "nWTR_S" : 3,
            "nRTW" : 13,
            "nEWTR" : 6,
            "nERTW" : 6,
            "nEWTW" : 6,
            "nERTR" : 6,
            "nRAS" : 28,
            "nRTP" : 3,
            "nRP" : 12,
            "nRFC" : 350,       # 2Gb=160, 4Gb=260, 8Gb=350ns
            "nREFI" : 3900,     # 3.9us for 1-8Gb
            "nFAW" : 16,
            "nBL" : 2,
            ### TxnScheduler ###
            "txnSchedulingPolicy" : "FRFCFS",
            "numTxnQEntries" : 1024,
            "boolReadFirstTxnScheduling" : 0,
            "maxPendingWriteThreshold" : 1,
            "minPendingWriteThreshold" : "0.2",
         })

         cramsimDimm.addParams({
            "debug" : memDebug,
            "debug_level" : memLevel,
            "verbose" : memVerbose,
            "numChannels" : hbmChan,
            "numPChannelsPerChannel" : 2,
            "numRanksPerChannel" : 1,
            "numBankGroupsPerRank" : 4,
            "numBanksPerBankGroup" : 4,
            "boolPowerCalc" : 0,
            "strControllerClockFrequency" : memory_clock,
            "nRP" : 12,
            "nRAS" : 28,
         })

         linkMemBridge = sst.Link("memctrl_cramsim_link_" + str(next_mem))
         linkMemBridge.connect( (mem, "cube_link", "2ns"), (cramsimBridge, "cpuLink", "2ns") )
         linkBridgeCtrl = sst.Link("cramsim_bridge_link_" + str(next_mem))
         linkBridgeCtrl.connect( (cramsimBridge, "memLink", "1ns"), (cramsimCtrl, "txngenLink", "1ns") )
         linkDimmCtrl = sst.Link("cramsim_dimm_link_" + str(next_mem))
         linkDimmCtrl.connect( (cramsimCtrl, "memLink", "1ns"), (cramsimDimm, "ctrlLink", "1ns") )

      bus_mem_link = sst.Link("bus_mem_link_" + str(next_mem))
      bus_mem_link.connect((mem_l2_bus, "low_network_%d"%sub_group_id, "100ps"), (mem, "direct_link", "100ps"))
      bus_mem_link.setNoCut()

      next_mem = next_mem + 1

   for next_mem_id in range(num_L2s_per_stack):
      cacheStartAddr = 0 + (256 * next_cache)
      endAddr = cacheStartAddr + memory_capacity_inB - (256 * total_mems)

      l2_cache = sst.Component("l2cache_%d"%(next_cache), "memHierarchy.Cache")
      l2_cache.addParams({
         "access_latency_cycles" : "100",
         "associativity" : "96",
         "cache_frequency" : "1312MHz",
         "cache_line_size" : "64",
         "cache_size" : "192KiB",
         "cache_type" : "noninclusive",
         "coherence_protocol" : "MESI",
         #"mshr_latency_cycles" : "32",
         "debug" : globalDebug,
         "debug_level" : globalLevel,
         "memNIC.addr_range_end" : endAddr,
         "memNIC.addr_range_start" : cacheStartAddr,
         "memNIC.group" : "2",
         "memNIC.interleave_size" : "256B",
         "memNIC.interleave_step" : str(num_l2 * 256) + "B",
         "memNIC.network_bw" : "37.5GB/s",
         "memNIC.network_link_control" : "shogun.ShogunNIC",
         "mshr_num_entries" : "2048",
         "replacement_policy" : "lru",
         "verbose" : "2",
      })

      l2xbarlink = sst.Link("l2_xbar_link_" + str(next_cache))
      l2xbarlink.connect((l2_cache, "cache", "100ps"), (router, "port" + str(next_port), "100ps"))
      l2xbarlink.setNoCut()

      next_port = next_port + 1

      l2_bus_link = sst.Link("l2g_mem_link_" + str(next_cache))
      l2_bus_link.connect((l2_cache, "low_network_0", "100ps"), (mem_l2_bus, "high_network_" + str(next_mem_id), "100ps"))
      l2_bus_link.setNoCut()

      if (next_cache + 1) % sub_mems == 0:
         next_cache = next_cache + total_mems - (sub_mems - 1)
      else:
         next_cache = next_cache + 1

# Enable SST Statistics Outputs for this simulation
sst.setStatisticLoadLevel(16)
sst.enableAllStatisticsForAllComponents({"type":"sst.AccumulatorStatistic"})


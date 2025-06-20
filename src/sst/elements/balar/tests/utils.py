import sst
import os
import argparse
try:
    import ConfigParser
except ImportError:
    import configparser as ConfigParser

# Balar argument parser
# Overwrite by env variables
balarTestParser = argparse.ArgumentParser()
balarTestParser.add_argument(
    "-c", "--config", help="Specify balar memory system configuration file", required=True)
balarTestParser.add_argument("-s", "--statfile",
                    help="Statistics file output path", default="./stats.out")
balarTestParser.add_argument("-l", "--statlevel",
                    help="Statistics level", type=int, default=16)

balarTestParser.add_argument(
    "-x", "--cuda-binary", help="Balar CUDA binary path. Should be ignored if the path in __cudaRegisterFatBinary is correct, otherwise need to provide it.", default="")

# Trace testcpu related configuration
balarTestParser.add_argument(
    "-t", "--trace", help="CUDA api calls trace file path", default="cuda_calls.trace")

# Vanadis related configuration
balarTestParser.add_argument(
    "--vanadis-binary", help="Vanadis binary path", default="")
balarTestParser.add_argument("--vanadis-args",
                    help="Vanadis binary arguments", default="")

# Verbosity config
balarTestParser.add_argument("-v", "--balar-verbosity",
                    help="Specify verbosity of balar", type=int, default=0)
balarTestParser.add_argument("--dma-verbosity",
                    help="Specify verbosity of dmaEngine", type=int, default=0)

def get_opt(args, envname, argname:dict, env_default=""):
    env = os.getenv(envname, env_default)
    if not env:
        return args.get(argname, "")
    else:
        return env


# GPU Memory system config
ddr4 = 2666 # Alternative is 2400
ddr_clock = "1333MHz" if ddr4 == 2666 else "1200MHz"
ddr_tCL = 19 if ddr4 == 2666 else 17
ddr_tCWL = 14 if ddr4 == 2666 else 12
ddr_tRCD = 19 if ddr4 == 2666 else 17
ddr_tRP = 19 if ddr4 == 2666 else 17

default_link_latency  = "100ps"
backend = "simple"      #simple, timing, ddr, hbm


def connect(name, c0, port0, c1, port1, latency):
    link = sst.Link(name)
    link.connect( (c0, port0, latency), (c1, port1, latency) )
    return link



class Config:
    def __init__(self, cfgFile, **kwargs):
        cp = ConfigParser.ConfigParser()
        if not cp.read(cfgFile):
            raise Exception('Unable to read file "%s"'%cfgFile)

        self.verbose = "verbose" in kwargs and kwargs["verbose"]

        self.default_link_latency = default_link_latency

        self.gpuclock = cp.get('GPU', 'clock')
        self.gpu_cores = cp.getint('GPU', 'gpu_cores')
        self.gpu_l2_parts = cp.getint('GPU', 'gpu_l2_parts')
        self.gpu_l2_capacity = cp.get('GPU', 'gpu_l2_capacity')
        self.gpu_l2_capacity_per_part_inB = ((int(''.join(filter(str.isdigit, self.gpu_l2_capacity)))* 1024))
        self.gpu_l2_capacity_inB = ((int(''.join(filter(str.isdigit, self.gpu_l2_capacity)))* 1024 * self.gpu_l2_parts))
        self.gpu_cpu_latency = cp.get('GPU', 'gpu_cpu_latency')

        self.gpu_memory_clock = cp.get('GPUMemory', 'clock')
        self.gpu_memory_network_bandwidth = cp.get('GPUMemory', 'network_bw')
        self.gpu_memory_capacity = cp.get('GPUMemory', 'capacity')
        self.gpu_memory_controllers = cp.getint('GPUMemory', 'memControllers')
        self.hbmChan = cp.getint('GPUMemory', 'hbmChan')
        self.hbmStacks = cp.getint('GPUMemory', 'hbmStacks')
        self.hbmRows = cp.getint('GPUMemory', 'hbmRows')
        self.gpu_memory_capacity_inB = ((int(''.join(filter(str.isdigit, self.gpu_memory_capacity)))* 1024 * 1024))
        self.gpu_memory_capacity_per_part_inB = int(((int(''.join(filter(str.isdigit, self.gpu_memory_capacity)))* 1024 * 1024)) / self.gpu_l2_parts)
        self.gpu_memory_capacity_per_stack_inB = int(((int(''.join(filter(str.isdigit, self.gpu_memory_capacity)))* 1024 * 1024)) / self.hbmStacks)

        self.gpu_xbar_clk_freq = cp.get('GPUNetwork', 'frequency')
        self.gpu_xbar_queue_depth = cp.get('GPUNetwork', 'buffer_depth')
        self.gpu_xbar_input_slots = cp.get('GPUNetwork', 'input_ports')
        self.gpu_xbar_output_slots = cp.get('GPUNetwork', 'output_ports')
        self.gpu_xbar_latency = cp.get('GPUNetwork', 'latency')
        self.gpu_xbar_bandwidth = cp.get('GPUNetwork', 'bandwidth')
        self.gpu_xbar_linkbandwidth = cp.get('GPUNetwork', 'linkbandwidth')
        self.gpu_xbar_flit_size = cp.get('GPUNetwork', 'flit_size')

    def getGPUConfig(self):
        params = dict({
                'clock': self.gpuclock,
                # 'verbose': int(self.verbose),
                'gpu_cores': int(self.gpu_cores),
                'maxtranscore': 1,
                # esnure that maxcachetrans is equal to GPU l1 mshr entries
                'maxcachetrans': 2048,
                })
        return params

    def getGPUL1Params(self):
        return dict({
            "verbose" : 0,
            "debug" : 0,
            "debug_level" : 10,
            "cache_frequency": self.gpuclock,
            "cache_size": "64KB",
            "associativity": 512,
            "access_latency_cycles": 28,
            "L1": 1,
            "cache_line_size": 32,
            "replacement_policy": "Random",
            "mshr_num_entries" : 200000,
            "coherence_protocol": "mesi",
            #"coherence_protocol": "none",
            #"max_requests_per_cycle" : 6,
            # Default params
            # "coherence_protocol": none,
            # "replacement_policy": "lru",
            # Not neccessary for simple cases:
            #"maxRequestDelay" : "1000000",
            })

    def getGPUL2Params(self, startAddr, endAddr):
        return dict({
            "verbose" : 0,
            "debug" : 0,
            "debug_level" : 10,
            "cache_frequency": self.gpuclock,
            "cache_size": self.gpu_l2_capacity,
            "associativity": 96,
            "access_latency_cycles": 100,
            "cache_line_size": 32,
            "replacement_policy": "lru",
            "mshr_num_entries" : 2048,
            "coherence_protocol": "mesi",
            "cache_type": "inclusive",
            "addr_range_end" : endAddr,
            "addr_range_start" : startAddr,
            "interleave_size": "256B",
            "interleave_step": str(self.gpu_l2_parts * 256) + "B",
            #"coherence_protocol": "none",
            #"cache_type": "noninclusive",
            #"max_requests_per_cycle" : 6,
            # Default params
            # "replacement_policy": "lru",
            # Not neccessary for simple cases:
            #"maxRequestDelay" : "1000000",
            })

    def get_GPU_mem_params(self, numParts, startAddr, endAddr):
        return dict({
            "verbose" : 0,
            "debug" : 0,
            "debug_level" : 10,
            "clock" : self.gpu_memory_clock,
            "backing" : "none",
            "max_requests_per_cycle": "100",
            "addr_range_start" : startAddr,
            "addr_range_end" : endAddr,
            "interleave_size" : "256B",
            "interleave_step" : str(numParts * 256) + "B",
            })

    def get_GPU_simple_mem_params(self):
        return dict({
            "access_time" : "45ns",
            "mem_size" : str(self.gpu_memory_capacity_per_part_inB)+ "B",
            })

    def get_GPU_ddr_memctrl_params(self, numParts, startAddr, endAddr):
        return dict({
            "verbose" : 0,
            "debug" : 0,
            "debug_level" : 10,
            "clock" : ddr_clock,
            "backing" : "none",
            "max_requests_per_cycle" : "-1",
            "addr_range_start" : startAddr,
            "addr_range_end" : endAddr,
            "interleave_size" : "256B",
            "interleave_step" : str(numParts * 256) + "B",
            })

    def get_GPU_simple_ddr_params(self, memID):
         return dict({
            "id" : memID,
            "mem_size" : str(self.gpu_memory_capacity_per_part_inB)+ "B",
            "tCAS" : 3, # 11@800MHz roughly coverted to 200MHz
            "tRCD" : ddr_tRCD,
            "tRP" : ddr_tRP,
            "cycle_time" : "5ns",
            "row_size" : "8KiB",
            "row_policy" : "open"
            })

    def get_GPU_ddr_timing_params(self, memID):
         return dict({
            "addrMapper" : "memHierarchy.roundRobinAddrMapper",
            "clock" : ddr_clock,
            "id" : memID,
            "channels" : 8,
            "channel.numRanks" : 8,
            "channel.transaction_Q_size" : 32,
            "channel.rank.numBanks" : 16,
            "channel.rank.bank.CL" : ddr_tCL,
            "channel.rank.bank.CL_WR" : ddr_tCWL,
            "channel.rank.bank.RCD" : ddr_tRCD,
            "channel.rank.bank.TRP" : ddr_tRP,
            "channel.rank.bank.dataCycles" : 4, # Cycles to return data (4 if burst8)
            "channel.rank.bank.pagePolicy" : "memHierarchy.simplePagePolicy",
            "channel.rank.bank.transactionQ" : "memHierarchy.reorderTransactionQ",
            "channel.rank.bank.pagePolicy.close" : 0,
            "mem_size" : str(self.gpu_memory_capacity_per_part_inB)+ "B"
         })

    def getGPURouterParams(self):
        return dict({
            "output_latency" : self.gpu_xbar_latency,
            "xbar_bw" : self.gpu_xbar_bandwidth,
            "input_buf_size" : "2KB",
            "input_latency" : self.gpu_xbar_latency,
            "num_ports" : self.gpu_cores + self.gpu_l2_parts,
            "flit_size" : self.gpu_xbar_flit_size,
            "output_buf_size" : "2KB",
            "link_bw" : self.gpu_xbar_linkbandwidth,
            "topology" : "merlin.singlerouter"
        })

    def getGPUXBarParams(self):
        return dict({
            "clock" : self.gpu_xbar_clk_freq,
            "buffer_depth" : self.gpu_xbar_queue_depth,
            "in_msg_per_cycle" : self.gpu_xbar_input_slots,
            "out_msg_per_cycle" : self.gpu_xbar_output_slots,
        })


##### CramSim #####

# Essentially:  16 psuedo channels w/ 16 banks per pCh (4banks/ bank group). 4H stack.
#               256GB/s -> Check the numbers match this...
#
#       Standards:
#               2Gb -> 16K rows, 64 col, 8 banks, 2KB page (1KB/pCh) Refr @ 8K/32ms (3.9us)
#               4Gb -> 16K rows, 64 col, 16 banks, 2KB page (1KB/pCh) Refr @ 8K/32ms (3.9us)
#               8Gb -> 32K rows, 64 col, 16 banks, 2KB page (1KB/pCh) Refr @ 8K/32ms (3.9us)
#               256b per access for legacy, 128b per access for pseudo
#
# Note: SK hynix says 2ns=1tCK. also tRC=45ns, tCCD=2ns. But does SK hynix make HBM2 or is this projected?
# All parameters from hbm_pseudo_4h.cfg setup in CramSim directory
# With the following changes to the default setup for hbm3:
#   channels = 16 and rows = 8192 (for same capacity & latency, 2x bandwidth)

#if self.hbmVersion == "2":
#    self.hbmChan = 8
#    self.hbmRows = 16384
#else :
#    self.hbmChan = 16
#    self.hbmRows = 8192

    #def get_GPU_hbm_memctrl_cramsim_params(self, memID):
        #return dict({
            #"clock" : self.gpu_memory_clock,
          #"memNIC.addr_range_start" : memID * 256,
          #"memNIC.addr_range_end" : self.gpu_memory_capacity_inB - (256 * memID),
          #"memNIC.interleave_size" : "256B",
          #"memNIC.interleave_step" : str(self.gpu_l2_parts * 256) + "B",
      ##   "max_requests_per_cycle" : 4, # Let CramSim self-limit instead of externally limiting
          #"backing" : "none",
          #"backend" : "memHierarchy.cramsim",
          #"backend.access_time" : "1ns", # PHY latency
          #"backend.max_outstanding_requests" : self.hbmChan * 128,   # Match CramSim bridge
          #"backend.request_width" : 32, # Should match CramSim -> 128b x4
          #"memNIC.req.network_bw" : self.gpu_memory_network_bandwidth,
          #"memNIC.ack.network_bw" : self.gpu_memory_network_bandwidth,
          #"memNIC.fwd.network_bw" : self.gpu_memory_network_bandwidth,
          #"memNIC.data.network_bw" : self.gpu_memory_network_bandwidth,
          #"backend.mem_size" : str(self.gpu_memory_capacity_per_part_inB)+ "B",
            #})

    def get_GPU_hbm_memctrl_cramsim_params(self, numParts, startAddr, endAddr):
        return dict({
          "verbose" : 0,
          "debug" : 0,
          "debug_level" : 10,
          "clock" : self.gpu_memory_clock,
          #"cpulink.group" : 3,
          "cpulink.accept_region" : 0, # Declaring our own address map
          "cpulink.addr_range_start" : startAddr,
          "cpulink.addr_range_end" : endAddr,
          "cpulink.interleave_size" : "256B",
          "cpulink.interleave_step" : str(numParts * 256) + "B",
          "max_requests_per_cycle" : "-1", # Let CramSim self-limit instead of externally limiting
          "backing" : "none",
          "backend" : "memHierarchy.cramsim",
          "backend.access_time" : "1ns", # PHY latency
          "backend.max_outstanding_requests" : self.hbmChan * 128 * 100,   # Match CramSim bridge
          "backend.request_width" : 32, # Should match CramSim -> 128b x4
          "cpulink.req.network_bw" :  self.gpu_memory_network_bandwidth,
          "cpulink.ack.network_bw" :  self.gpu_memory_network_bandwidth,
          "cpulink.fwd.network_bw" :  self.gpu_memory_network_bandwidth,
          "cpulink.data.network_bw" : self.gpu_memory_network_bandwidth,
          "backend.mem_size" : str(self.gpu_memory_capacity_per_stack_inB)+ "B",
            })

    def get_GPU_hbm_cramsim_bridge_params(self, memID):
        return dict({
          "verbose" : 0,
          "debug" : 0,
          "debug_level" : 10,
          "numTxnPerCycle" : self.hbmChan, # NumChannels
          "numBytesPerTransaction" : 32,
          "strControllerClockFrequency" : self.gpu_memory_clock,
          "maxOutstandingReqs" : self.hbmChan * 128 * 100, # NumChannels * 64 per channel
          "readWriteRatio" : "0.667",# Required but unused since we're driving with Ariel
          "randomSeed" : 0,
          "mode" : "seq",
            })


    def get_GPU_hbm_cramsim_ctrl_params(self, memID):
        return dict({
         "verbose" : 0,
         "debug" : 0,
         "debug_level" : 10,
         "TxnConverter" : "CramSim.c_TxnConverter",
         "AddrMapper"   : "CramSim.c_AddressHasher",
         "CmdScheduler" : "CramSim.c_CmdScheduler",
         "DeviceDriver" : "CramSim.c_DeviceDriver",
         "TxnScheduler" : "CramSim.c_TxnScheduler",
         "strControllerClockFrequency" : self.gpu_memory_clock,
         "boolEnableQuickRes" : 0,
         ### TxnConverter ###
         "relCommandWidth" : 1,
         "boolUseReadA" : 0,
         "boolUseWriteA" : 0,
         "bankPolicy" : "OPEN",
         ### AddressHasher ###
#         "strAddressMapStr" : "r:14_b:2_B:2_l:3_C:2_l:3_c:1_h:5_",
         "strAddressMapStr" : "r:14_b:2_B:2_l:3_C:2_l:3_c:1_h:5_",
         #"strAddressMapStr" : "r_b_B_c_l:3_C_l:3_h:5_",
         "numBytesPerTransaction" : 32,
         ### CmdScheduler ###
         "numCmdQEntries" : 32,
         "cmdSchedulingPolicy" : "BANK",
         ### DeviceDriver ###
         "boolDualCommandBus" : 1,
         "boolMultiCycleACT" : 1,
         "numChannels" : self.hbmChan,
         "numPChannelsPerChannel" : 2,
         "numRanksPerChannel" : 1,
         "numBankGroupsPerRank" : 4,
         "numBanksPerBankGroup" : 4,
         "numRowsPerBank" : self.hbmRows,
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

    def get_GPU_hbm_cramsim_dimm_params(self, memID):
        return dict({
          "verbose" : 0,
          "debug" : 0,
          "debug_level" : 10,
          "numChannels" : self.hbmChan,
          "numPChannelsPerChannel" : 2,
          "numRanksPerChannel" : 1,
          "numBankGroupsPerRank" : 4,
          "numBanksPerBankGroup" : 4,
          "boolPowerCalc" : 0,
          "strControllerClockFrequency" : self.gpu_memory_clock,
          "nRP" : 12,
          "nRAS" : 28,
            })








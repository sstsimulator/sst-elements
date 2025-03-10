# Automatically generated SST Python input
# Run cmd: sst testBalar-testcpu.py --model-options="-c gpu-v100-mem.cfg -v 0 -x ./vectorAdd/vectorAdd -t cuda_calls.trace"

# This run script will run balar with a test cpu that consumes CUDA API trace
# To get the trace file and associated cudamemcpy data payload, you will need the
# NVBit based CUDA API tracer tool in
#  `https://github.com/accel-sim/accel-sim-framework/tree/dev/util/tracer_nvbit/others/`
# Which after compilation, simply run
#  `LD_PRELOAD=PATH/TO/cuda_api_tracer.so PATH/TO/CUDA_APP` to
#  collect API trace
# Sample run:
#     LD_PRELOAD=cuda_api_tracer.so ./vectorAdd/vectorAdd

import sst
try:
    import ConfigParser
except ImportError:
    import configparser as ConfigParser
import argparse
from utils import *
import balarBlock

# Arguments
args = vars(balarTestParser.parse_args())

verbosity = args["balar_verbosity"]
cfgFile = args["config"]
statFile = args["statfile"]
statLevel = args["statlevel"]
traceFile = args["trace"]
binaryFile = args["cuda_binary"]
dma_verbosity = args["dma_verbosity"]

# Build Configuration Information
config = Config(cfgFile, verbose=verbosity)

DEBUG_L1 = 0
DEBUG_MEM = 0
DEBUG_CORE = 0
DEBUG_NIC = 0
DEBUG_LEVEL = 10

debug_params = {"debug": 1, "debug_level": 10}

# On network: Core, L1, MMIO device, memory
# Logical communication: Core->L1->memory
#                        Core->MMIO
#                        MMIO->memory
core_group = 0
mmio_group = 1
memory_group = 2

core_dst = [memory_group, mmio_group]
mmio_src = [core_group]
mmio_dst = [memory_group]
memory_src = [core_group, mmio_group]

# Constans shared across components
network_bw = "25GB/s"
clock = "2GHz"

# Balar builder
balarBuilder = balarBlock.Builder(args)
balar_mmio_iface, balar_mmio_testcpu_addr, dma_mem_if, dma_mmio_if = balarBuilder.buildTestCPU(cfgFile, verbosity, dma_verbosity)

mmio_nic = balar_mmio_iface.setSubComponent("memlink", "memHierarchy.MemNIC")
mmio_nic.addParams({"group": mmio_group,
                    "sources": mmio_src,
                    "destinations": mmio_dst,
                    "network_bw": network_bw})

# DMA's mem_iface like a core
dma_mem_nic = dma_mem_if.setSubComponent("memlink", "memHierarchy.MemNIC")
dma_mem_nic.addParams({"group": core_group,
                    "destinations": core_dst,
                    "network_bw": network_bw})

# DMA's mmio_iface like memory
dma_mmio_nic = dma_mmio_if.setSubComponent("memlink", "memHierarchy.MemNIC")
dma_mmio_nic.addParams({"group": memory_group,
                    "sources": memory_src,
                    "network_bw": network_bw})

# Test CPU components and mem hierachy
cpu = sst.Component("cpu", "balar.BalarTestCPU")
cpu.addParams({
    "clock": clock,
    "verbose": verbosity,
    "scratch_mem_addr": 0,
    "gpu_addr": balar_mmio_testcpu_addr,  # Just above memory addresses

    # Trace and executable info
    "trace_file": traceFile,
    "cuda_executable": binaryFile,
    "enable_memcpy_dump": False
})
iface = cpu.setSubComponent("memory", "memHierarchy.standardInterface")
iface.addParams(debug_params)
cpu_nic = iface.setSubComponent("memlink", "memHierarchy.MemNIC")
cpu_nic.addParams({"group": core_group,
                   "destinations": core_dst,
                   "network_bw": network_bw})

chiprtr = sst.Component("chiprtr", "merlin.hr_router")
chiprtr.addParams({
    "xbar_bw": "1GB/s",
    "id": "0",
    "input_buf_size": "1KB",
    "num_ports": "5",
    "flit_size": "72B",
    "output_buf_size": "1KB",
    "link_bw": "1GB/s",
    "topology": "merlin.singlerouter"
})
chiprtr.setSubComponent("topology", "merlin.singlerouter")

memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "debug": DEBUG_MEM,
    "debug_level": DEBUG_LEVEL,
    "clock": "1GHz",
    "addr_range_end": balar_mmio_testcpu_addr - 1,
})
mem_nic = memctrl.setSubComponent("cpulink", "memHierarchy.MemNIC")
mem_nic.addParams({"group": memory_group,
                   "sources" : memory_src,
                   "network_bw": network_bw})

memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
    "access_time": "100 ns",
    "mem_size": "512MiB"
})

# Define the simulation links for CPU
#          cpu/cpu_nic
#                 |
#  balar ----  chiprtr ---- mem_nic/mem/
#                 |
#                dma
#
link_cpu_rtr = sst.Link("link_cpu")
link_cpu_rtr.connect((cpu_nic, "port", "1000ps"), (chiprtr, "port0", "1000ps"))

link_mmio_rtr = sst.Link("link_mmio")
link_mmio_rtr.connect((mmio_nic, "port", "500ps"), (chiprtr, "port1", "500ps"))

link_mmio_rtr = sst.Link("link_dma_mem")
link_mmio_rtr.connect((dma_mem_nic, "port", "500ps"), (chiprtr, "port2", "500ps"))

link_mmio_rtr = sst.Link("link_dma_mmio")
link_mmio_rtr.connect((dma_mmio_nic, "port", "500ps"), (chiprtr, "port3", "500ps"))

link_mem_rtr = sst.Link("link_mem")
link_mem_rtr.connect((mem_nic, "port", "1000ps"), (chiprtr, "port4", "1000ps"))

# ===========================================================
# Enable statistics
sst.setStatisticLoadLevel(statLevel)
sst.enableAllStatisticsForAllComponents({"type": "sst.AccumulatorStatistic"})
sst.setStatisticOutput("sst.statOutputTXT", {"filepath": statFile})
print("Completed configuring Balar with BalarTestCPU")

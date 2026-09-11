# Contract test: minimal CUDA API (malloc) exercises scratch write + MMIO doorbell + DMA read.
# Run: sst testBalar-doorbell.py --model-options='-c gpu-v100-mem.cfg -s stats.out -x ./balar_trace/vectorAdd -t traces/doorbell_malloc.trace'

import sst
import argparse
from utils import *
import balarBlock
import balar_test_topology

args = vars(balarTestParser.parse_args())
balarBuilder = balarBlock.Builder(args)
cpu, _ = balar_test_topology.build_testcpu_router(
    balarBuilder, args["config"], args["balar_verbosity"], args["dma_verbosity"]
)
cpu.addParams({
    "trace_file": args["trace"],
    "cuda_executable": args["cuda_binary"],
})

sst.setStatisticLoadLevel(args["statlevel"])
sst.enableAllStatisticsForAllComponents({"type": "sst.AccumulatorStatistic"})
sst.setStatisticOutput("sst.statOutputTXT", {"filepath": args["statfile"]})

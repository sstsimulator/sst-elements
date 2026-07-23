# Doorbell-pattern malloc+free with L1, flush-before-doorbell, shared DRAM bus.
import sst
from utils import *
import balarBlock
import balar_test_topology

args = vars(balarTestParser.parse_args())
balar_builder = balarBlock.Builder(args)
cpu, _ = balar_test_topology.build_doorbell_topology(
    balar_builder, args["config"], mode="trace",
    balar_verbosity=args["balar_verbosity"], dma_verbosity=args["dma_verbosity"])
cpu.addParams({
    "trace_file": args["trace"],
    "cuda_executable": args["cuda_binary"],
})

sst.setStatisticLoadLevel(args["statlevel"])
sst.enableAllStatisticsForAllComponents({"type": "sst.AccumulatorStatistic"})
sst.setStatisticOutput("sst.statOutputTXT", {"filepath": args["statfile"]})

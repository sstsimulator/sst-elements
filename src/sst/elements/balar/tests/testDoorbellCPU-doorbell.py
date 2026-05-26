# Doorbell-pattern doorbell: fatbin register via scratch+flush+mmio doorbell (doorbell_only mode).
import sst
from utils import *
import balarBlock
import balar_test_topology

args = vars(balarTestParser.parse_args())
balar_builder = balarBlock.Builder(args)
cpu, _ = balar_test_topology.build_doorbell_topology(
    balar_builder, args["config"], mode="doorbell_only",
    balar_verbosity=args["balar_verbosity"], dma_verbosity=args["dma_verbosity"])
cpu.addParams({
    "cuda_executable": args["cuda_binary"],
})

sst.setStatisticLoadLevel(args["statlevel"])
sst.enableAllStatisticsForAllComponents({"type": "sst.AccumulatorStatistic"})
sst.setStatisticOutput("sst.statOutputTXT", {"filepath": args["statfile"]})

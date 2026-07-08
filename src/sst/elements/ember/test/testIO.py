import sst
import sys
from sst.merlin.base import *
from sst.merlin.endpoint import *
from sst.merlin.interface import *
from sst.merlin.topology import *
from sst.ember import *

# Optional model-options:
#   argv[1] = stats CSV filename (relative to sim cwd). When set, this script
#             writes a deterministic stats CSV and a sidecar identifying the
#             I/O NIDs. Existing refFile-diff workflows (no model-options)
#             remain unchanged.
stats_csv = sys.argv[1] if len(sys.argv) > 1 else None

PlatformDefinition.setCurrentPlatform("firefly-defaults")

topo = topoTorus()
topo.shape = "4"
topo.width = "1"
topo.local_ports = "1"
topo.link_latency = "20ns"

router = hr_router()
router.link_bw = "4GB/s"
router.flit_size = "8B"
router.xbar_bw = "4GB/s"
router.input_latency = "20ns"
router.output_latency = "20ns"
router.input_buf_size = "16kB"
router.output_buf_size = "16kB"
router.num_vns = "1"
router.xbar_arb = "merlin.xbar_arb_lru"

topo.router = router

def makeNetworkif():
    nif = ReorderLinkControl()
    nif.link_bw = "12GB/s"
    nif.input_buf_size = "16kB"
    nif.output_buf_size = "16kB"
    return nif

system = System()
system.setTopology(topo, 1)

io_nid_list = system.allocateIoNodes(2, "linear")
system.setIoNetworkInterface(makeNetworkif())

motif_args = "TestNetworkIO messageSize=4096 iterations=2 op=write fileSize=4294967296"
if stats_csv:
    motif_args += " rngSeedZ=12345 rngSeedW=67890"

job = EmberMPIJob(0, 2)
job.network_interface = makeNetworkif()
job.addMotif(motif_args)
system.allocateNodes(job, "linear")
job.useNetworkIO(system)

system.build()

if stats_csv:
    sst.setStatisticLoadLevel(7)
    sst.setStatisticOutput("sst.statOutputCSV", {"filepath": stats_csv, "separator": ", "})
    # firefly.SimpleSSD is loaded anonymously with INSERT_STATS, so its stats
    # are routed up to the parent firefly.nic at runtime — enable on nic.
    all_stats = ("sentByteCount", "rcvdByteCount", "sentPkts", "rcvdPkts",
                 "networkIoReadTargetNid", "networkIoWriteTargetNid",
                 "networkIoReadLatency_ns", "networkIoWriteLatency_ns",
                 "readRequests", "writeRequests", "readBytes", "writeBytes",
                 "readLatency_ns", "writeLatency_ns",
                 "queueDepthOnEnqueue", "pendingOnDispatch")
    for s in all_stats:
        sst.enableStatisticForComponentType("firefly.nic", s, {"type": "sst.AccumulatorStatistic"})
    with open("okmar_io_nids.txt", "w") as f:
        f.write(",".join(str(n) for n in io_nid_list))

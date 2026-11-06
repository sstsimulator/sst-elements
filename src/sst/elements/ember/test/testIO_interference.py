# Cross-job interference proxy for the §9 gate decision in pr-6-design.md.
#
# Today's NetworkIO model has a single, global I/O-node list; every compute
# job that calls EmberMPIJob.useNetworkIO(system, ...) targets the same set
# of storage NIDs.  If two compute jobs co-exist, their writes interleave on
# the (shared) storage NICs.  This SDL produces a deterministic latency
# distribution that the testsuite can compare against a single-job baseline
# to quantify the contention cost.
#
# Model-options contract:
#   argv[1] = mode               required: "baseline" | "contention"
#   argv[2] = iterations         optional int (default 32)
#   argv[3] = stats CSV filename optional; when set, enables
#             networkIoWriteLatency_ns CSV emit and writes the I/O NID list
#             to okmar_interference_io_nids.txt for the testsuite to filter
#             by storage endpoint.
#
# Topology: 6-node torus (shape=6), 2 storage NIDs (linear allocation
# claims 0,1), 2 compute jobs of 2 ranks each.
#   - baseline mode: only Job A is built; Job B's slots remain EmptyJob.
#     Used to capture per-write latency with zero cross-job interference.
#   - contention mode: Job A AND Job B are built, both bound to the same
#     I/O pool via useNetworkIO.  Used to capture per-write latency under
#     two-tenant pressure.
#
# Determinism: both jobs use seeded MarsagliaRNG via the arg.rngSeedZ/W
# motif params introduced in (b3).  Distinct seed pairs per job mimic
# independent workloads while keeping the run bit-stable.
#
# Stats coverage: networkIoWriteLatency_ns (Sum / Count / Min / Max from the
# AccumulatorStatistic) gives a coarse latency profile per compute NIC.
# That's sufficient as a gate proxy: mean latency and tail (Max) over the
# combined sample set captures the impact we care about (does adding a
# second tenant inflate end-to-end write latency?).

import sys
import sst
from sst.merlin.base import *
from sst.merlin.endpoint import *
from sst.merlin.interface import *
from sst.merlin.topology import *
from sst.ember import *

if len(sys.argv) < 2 or sys.argv[1] not in ("baseline", "contention"):
    raise RuntimeError(
        "testIO_interference.py: argv[1] must be 'baseline' or 'contention'")
mode = sys.argv[1]

iterations = 32
if len(sys.argv) > 2:
    try:
        iterations = int(sys.argv[2])
    except ValueError:
        raise RuntimeError(
            "testIO_interference.py: argv[2] must be an integer iteration count")

stats_csv = sys.argv[3] if len(sys.argv) > 3 else None

PlatformDefinition.setCurrentPlatform("firefly-defaults")

topo = topoTorus()
topo.shape = "6"
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

jobA = EmberMPIJob(0, 2)
jobA.network_interface = makeNetworkif()
motif_args_A = (
    "TestNetworkIO messageSize=4096 iterations={0} op=write fileSize=4294967296"
    " rngSeedZ=12345 rngSeedW=67890"
).format(iterations)
jobA.addMotif(motif_args_A)
system.allocateNodes(jobA, "linear")
jobA.useNetworkIO(system)

if mode == "contention":
    jobB = EmberMPIJob(1, 2)
    jobB.network_interface = makeNetworkif()
    motif_args_B = (
        "TestNetworkIO messageSize=4096 iterations={0} op=write fileSize=4294967296"
        " rngSeedZ=24680 rngSeedW=13579"
    ).format(iterations)
    jobB.addMotif(motif_args_B)
    system.allocateNodes(jobB, "linear")
    jobB.useNetworkIO(system)

system.build()

if stats_csv is not None:
    sst.setStatisticLoadLevel(7)
    sst.setStatisticOutput(
        "sst.statOutputCSV",
        {"filepath": stats_csv, "separator": ", "},
    )
    sst.enableStatisticForComponentType(
        "firefly.nic",
        "networkIoWriteLatency_ns",
        {"type": "sst.AccumulatorStatistic"},
    )
    with open("okmar_interference_io_nids.txt", "w") as f:
        f.write(",".join(str(n) for n in io_nid_list) + "\n")

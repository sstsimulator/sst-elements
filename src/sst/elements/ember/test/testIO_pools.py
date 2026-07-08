# Pool-isolation SDL for PR 6c.
#
# Two compute jobs (A and B) each bind to a *distinct* I/O pool with
# disjoint storage NIDs.  The HadesStorageController publishes one
# SharedArray<int> per pool ("IoPool.<name>"), and HadesNetworkIO on the
# compute side subscribes by name and dispatches strictly through that
# array.  Therefore the only storage NIDs each compute job can hit are
# the NIDs belonging to its bound pool -- which is exactly the v1
# isolation invariant the PR 6c tests need to assert.
#
# Model-options contract (argv after `--`):
#   argv[1] = mode    required: "smoke" | "csv" | "collision"
#   argv[2] = iter    optional int (default 8 for smoke, 200 for csv)
#   argv[3] = stats   optional stats-CSV filename (csv mode only)
#
# Topology: 8-node torus (shape=8), 4 storage NIDs total split 2+2 across
# pools "alpha" and "beta", two compute jobs of 2 ranks each.
#
# Modes:
#   smoke      - both jobs run; stdout is filtered+sorted+diffed against
#                a refFile (same pattern as testIO_stripe.py).  Asserts
#                runtime stability of the two-pool path.
#   csv        - same SDL but with rcvdByteCount stats CSV emit.  Used by
#                the harness to assert per-pool isolation: each compute
#                job's bytes MUST land only on its own pool's storage NICs.
#   collision  - two jobs deliberately given DIFFERENT pool names but
#                with overlapping NID allocation requests.  This is
#                expected to fail at Python build time (R-10 mitigation
#                in pr-6-design.md).  Used as an xfail probe.
#
# Determinism: jobs use distinct (rngSeedZ, rngSeedW) seed pairs via the
# motif arg pattern introduced in (b3).

import sys
import sst
from sst.merlin.base import *
from sst.merlin.endpoint import *
from sst.merlin.interface import *
from sst.merlin.topology import *
from sst.ember import *

_MODES = ("smoke", "csv", "collision",
          "deny", "empty_permit", "ackdeny_byte_budget")
if len(sys.argv) < 2 or sys.argv[1] not in _MODES:
    raise RuntimeError(
        "testIO_pools.py: argv[1] must be one of {0}".format(list(_MODES)))
mode = sys.argv[1]

if mode in ("csv", "ackdeny_byte_budget"):
    default_iter = 200
elif mode in ("deny", "empty_permit"):
    default_iter = 32
else:
    default_iter = 8
iterations = default_iter
if len(sys.argv) > 2:
    try:
        iterations = int(sys.argv[2])
    except ValueError:
        raise RuntimeError(
            "testIO_pools.py: argv[2] must be an integer iteration count")

stats_csv = sys.argv[3] if len(sys.argv) > 3 else None
if mode in ("deny", "empty_permit", "ackdeny_byte_budget") and stats_csv is None:
    stats_csv = "okmar_pools_stats.csv"

PlatformDefinition.setCurrentPlatform("firefly-defaults")

topo = topoTorus()
topo.shape = "8"
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

# Linear allocation consumes the lowest-numbered free NIDs.  The first
# allocateIoNodes() claim takes 0,1 for pool "alpha"; the second claim
# takes 2,3 for pool "beta".  The two pools are *disjoint by NID*.
alpha_nids = system.allocateIoNodes(2, "linear", pool="alpha")
if mode == "collision":
    # Re-using pool name "alpha" with a *different* count would be the
    # ambiguity case; we intentionally provoke a *different* anomaly:
    # request a pool whose NIDs the compute jobs never bind to.  The
    # py-side useNetworkIO() call below uses a pool name that was never
    # allocated, triggering the AssertionError path in pyember.py:214.
    pass
else:
    beta_nids = system.allocateIoNodes(2, "linear", pool="beta")

system.setIoNetworkInterface(makeNetworkif())

# Job A: bound to pool "alpha"
jobA = EmberMPIJob(0, 2)
jobA.network_interface = makeNetworkif()
motif_args_A = (
    "TestNetworkIO messageSize=4096 iterations={0} op=write fileSize=4294967296"
    " rngSeedZ=12345 rngSeedW=67890"
).format(iterations)
jobA.addMotif(motif_args_A)
system.allocateNodes(jobA, "linear")
# PR 8 deny modes: jobA suppresses permit publish so storage NIC's
# IoPoolPermit.alpha set ends up empty (or missing jobA's NIDs). Every
# Read/Write request from jobA is then receive-side denied via AckDeny.
suppress_A = mode in ("deny", "empty_permit")
jobA.useNetworkIO(system, pool="alpha", suppress_permit=suppress_A)

# Job B: bound to pool "beta" (or to a non-existent pool in collision mode)
jobB = EmberMPIJob(1, 2)
jobB.network_interface = makeNetworkif()
motif_args_B = (
    "TestNetworkIO messageSize=4096 iterations={0} op=write fileSize=4294967296"
    " rngSeedZ=24680 rngSeedW=13579"
).format(iterations)
jobB.addMotif(motif_args_B)
system.allocateNodes(jobB, "linear")
if mode == "collision":
    # This MUST raise AssertionError at SDL build time.
    jobB.useNetworkIO(system, pool="gamma")
else:
    suppress_B = mode == "empty_permit"
    jobB.useNetworkIO(system, pool="beta", suppress_permit=suppress_B)

system.build()

if stats_csv is not None:
    sst.setStatisticLoadLevel(7)
    sst.setStatisticOutput(
        "sst.statOutputCSV",
        {"filepath": stats_csv, "separator": ", "},
    )
    sst.enableStatisticForComponentType(
        "firefly.nic", "rcvdByteCount",
        {"type": "sst.AccumulatorStatistic"},
    )
    sst.enableStatisticForComponentType(
        "firefly.nic", "rcvdPkts",
        {"type": "sst.AccumulatorStatistic"},
    )
    sst.enableStatisticForComponentType(
        "firefly.nic", "networkIoWriteLatency_ns",
        {"type": "sst.AccumulatorStatistic"},
    )
    if mode == "ackdeny_byte_budget":
        sst.enableStatisticForComponentType(
            "firefly.nic", "sentByteCount",
            {"type": "sst.AccumulatorStatistic"},
        )
    # Sidecar: lets the harness map per-NIC CSV rows back to pools.
    with open("okmar_pools_nids.txt", "w") as f:
        f.write("alpha=" + ",".join(str(n) for n in alpha_nids) + "\n")
        f.write("beta=" + ",".join(str(n) for n in beta_nids) + "\n")

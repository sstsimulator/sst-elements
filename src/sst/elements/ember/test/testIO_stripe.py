import sys
import sst
from sst.merlin.base import *
from sst.merlin.endpoint import *
from sst.merlin.interface import *
from sst.merlin.topology import *
from sst.ember import *

_STRIPE_POLICIES = {
    "block": ("firefly.Block_NetworkIOMapper", {"bytesPerNode": "4KiB"}),
    "rr":    ("firefly.RR_NetworkIOMapper",    {"blockSize":   "4KiB"}),
    "hash":  ("firefly.Hash_NetworkIOMapper",  {"blockSize":   "4KiB"}),
}

policy = sys.argv[1] if len(sys.argv) > 1 else "block"
if policy not in _STRIPE_POLICIES:
    raise RuntimeError(
        "testIO_stripe.py: unknown stripe policy '{0}'; expected one of {1}".format(
            policy, list(_STRIPE_POLICIES.keys())))
mapper_name, mapper_params = _STRIPE_POLICIES[policy]

# Model-options contract: argv[1]=policy (required), argv[2]=iterations (optional int),
# argv[3]=stats CSV filename (optional; when set, enables rcvdByteCount and writes
# a sidecar io-nid list for the testsuite to filter CSV rows by storage endpoint).
NUM_ITERATIONS_PER_RANK = 8
if len(sys.argv) > 2:
    try:
        NUM_ITERATIONS_PER_RANK = int(sys.argv[2])
    except ValueError:
        raise RuntimeError(
            "testIO_stripe.py: second model-option must be an integer iteration count, got '{0}'".format(sys.argv[2]))

stats_csv = sys.argv[3] if len(sys.argv) > 3 else None

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

job = EmberMPIJob(0, 2)
job.network_interface = makeNetworkif()
motif_args = "TestNetworkIO messageSize=4096 iterations={0} op=write fileSize=4294967296".format(NUM_ITERATIONS_PER_RANK)
if stats_csv is not None:
    motif_args += " rngSeedZ=12345 rngSeedW=67890"
job.addMotif(motif_args)
system.allocateNodes(job, "linear")
job.useNetworkIO(system, mapper=mapper_name, mapper_params=mapper_params)

system.build()

if stats_csv is not None:
    sst.setStatisticLoadLevel(7)
    sst.setStatisticOutput("sst.statOutputCSV", {"filepath": stats_csv, "separator": ", "})
    sst.enableStatisticForComponentType("firefly.nic", "rcvdByteCount", {"type": "sst.AccumulatorStatistic"})
    with open("okmar_stripe_io_nids.txt", "w") as f:
        f.write(",".join(str(n) for n in io_nid_list) + "\n")

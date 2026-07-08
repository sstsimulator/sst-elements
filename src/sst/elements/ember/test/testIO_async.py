import sys
import sst
from sst.merlin.base import *
from sst.merlin.endpoint import *
from sst.merlin.interface import *
from sst.merlin.topology import *
from sst.ember import *

_MODES = ("single", "waitall_16", "cancel_inflight")
if len(sys.argv) < 2 or sys.argv[1] not in _MODES:
    raise RuntimeError(
        "testIO_async.py: argv[1] must be one of {0}".format(list(_MODES)))
mode = sys.argv[1]

if mode == "waitall_16":
    op_arg       = "async"
    batch        = 16
    cancel_flag  = 0
elif mode == "cancel_inflight":
    op_arg       = "async_cancel"
    batch        = 4
    cancel_flag  = 1
else:
    op_arg       = "async"
    batch        = 4
    cancel_flag  = 0

PlatformDefinition.setCurrentPlatform("firefly-defaults")

topo = topoTorus()
topo.shape       = "4"
topo.width       = "1"
topo.local_ports = "1"
topo.link_latency = "20ns"

router = hr_router()
router.link_bw         = "4GB/s"
router.flit_size       = "8B"
router.xbar_bw         = "4GB/s"
router.input_latency   = "20ns"
router.output_latency  = "20ns"
router.input_buf_size  = "16kB"
router.output_buf_size = "16kB"
router.num_vns         = "1"
router.xbar_arb        = "merlin.xbar_arb_lru"

topo.router = router


def makeNetworkif():
    nif = ReorderLinkControl()
    nif.link_bw         = "12GB/s"
    nif.input_buf_size  = "16kB"
    nif.output_buf_size = "16kB"
    return nif


system = System()
system.setTopology(topo, 1)

io_nid_list = system.allocateIoNodes(2, "linear")
system.setIoNetworkInterface(makeNetworkif())

motif_args = (
    "TestNetworkIO messageSize=1024 iterations={iter} op={op} "
    "fileSize=1048576 capacity=0 asyncBatch={batch} "
    "cancelInflight={cancel} rngSeedZ=13579 rngSeedW=24680"
).format(iter=batch, op=op_arg, batch=batch, cancel=cancel_flag)

job = EmberMPIJob(0, 2)
job.network_interface = makeNetworkif()
job.addMotif(motif_args)
system.allocateNodes(job, "linear")
job.useNetworkIO(system)

system.build()

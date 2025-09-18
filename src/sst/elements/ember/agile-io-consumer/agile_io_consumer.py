import sst
from sst.merlin.base import *
from sst.merlin.endpoint import *
from sst.merlin.interface import *
from sst.merlin.topology import *

from sst.ember import *

def example():
    PlatformDefinition.setCurrentPlatform("firefly-defaults")

    ### Setup the topology
    topo = topoDragonFly()
    topo.hosts_per_router = 2
    topo.routers_per_group = 4
    topo.intergroup_links = 2
    topo.num_groups = 4
    topo.algorithm = ["minimal","adaptive-local"]

    # Set up the routers
    router = hr_router()
    router.link_bw = "4GB/s"
    router.flit_size = "8B"
    router.xbar_bw = "6GB/s"
    router.input_latency = "20ns"
    router.output_latency = "20ns"
    router.input_buf_size = "4kB"
    router.output_buf_size = "4kB"
    router.num_vns = 2
    router.xbar_arb = "merlin.xbar_arb_lru"

    topo.router = router
    topo.link_latency = "20ns"

    ### set up the endpoint
    networkif = ReorderLinkControl()
    networkif.link_bw = "4GB/s"
    networkif.input_buf_size = "16kB"
    networkif.output_buf_size = "16kB"

    ep = EmberMPIJob(0,topo.getNumNodes())
    ep.network_interface = networkif
    ep.addMotif("Init")
    ep.addMotif("IOConsumer IONodes=[0,4,8,12]")
    ep.addMotif("Fini")
    ep.nic.nic2host_lat= "100ns"

    system = System()
    system.setTopology(topo)
    system.allocateNodes(ep,"linear")

    system.build()

if __name__ == "__main__":
    example()

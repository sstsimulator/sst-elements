#!/usr/bin/env python

# Test for anytopo with Slimfly topology

import os
import sys
import sst

# Add anytopo_utility directory to path
anytopo_utility = os.path.join(os.path.dirname(__file__), '..', 'topology', 'anytopo_ultility')
sys.path.insert(0, anytopo_utility)

from sst.merlin.base import *
from sst.merlin.endpoint import *
from sst.merlin.interface import *
from sst.merlin.topology import *
from sst.merlin.targetgen import *

try:
    import networkx as nx
except ImportError:
    print("NetworkX is required. Please install NetworkX and try again.")
    sys.exit(1)

from HPC_topos import sf_configs
from Slimfly import SlimflyTopo

if __name__ == "__main__":

    LOAD = 0.5
    UNIFIED_ROUTER_LINK_BW = 16

    # Create Slimfly topology
    sf_topo = SlimflyTopo(*sf_configs[0])  # Use first config: (18, 5)
    sf_topo.set_endpoints_per_router(2)
    G = sf_topo.get_nx_graph()

    ### Setup the topology
    topo = topoAny()
    topo.routing_mode = "source_routing"
    topo.topo_name = "slimfly"
    topo.import_graph(G)

    # Set up the routers
    router = hr_router()
    router.link_bw = f"{UNIFIED_ROUTER_LINK_BW}Gb/s"
    router.flit_size = "32B"
    router.xbar_bw = f"{UNIFIED_ROUTER_LINK_BW*2}Gb/s"  # 2x crossbar speedup
    router.input_latency = "20ns"
    router.output_latency = "20ns"
    router.input_buf_size = "32kB"
    router.output_buf_size = "32kB"
    router.num_vns = 2
    router.xbar_arb = "merlin.xbar_arb_rr"

    topo.router = router
    topo.link_latency = "20ns"
    topo.host_link_latency = "10ns"

    # Calculate routing table
    routing_table = topo.calculate_routing_table()

    ### use endpointNIC
    endpointNIC = EndpointNIC(use_reorderLinkControl=True, topo=topo)

    # source routing table is passed to the plugin here
    endpointNIC.addPlugin("sourceRoutingPlugin", routing_table=routing_table)
    #### the following parameters will be passed down to linkcontrol via callback
    endpointNIC.link_bw = f"{UNIFIED_ROUTER_LINK_BW}Gb/s"
    endpointNIC.input_buf_size = "32kB"
    endpointNIC.output_buf_size = "32kB"
    endpointNIC.vn_remap = [0]

    targetgen = UniformTarget()

    ep = OfferedLoadJob(0, topo.getNumNodes())
    ep.setEndpointNIC(endpointNIC)
    ep.pattern = targetgen
    ep.offered_load = LOAD
    ep.link_bw = f"{UNIFIED_ROUTER_LINK_BW}Gb/s"
    ep.message_size = "32B"
    ep.collect_time = "200us"
    ep.warmup_time = "200us"
    ep.drain_time = "1000us"

    system = System()
    system.setTopology(topo)
    system.allocateNodes(ep, "linear")

    system.build()

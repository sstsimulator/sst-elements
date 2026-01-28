#!/usr/bin/env python

# Test for anytopo with Ember using complete graph (4 vertices)

import os
import sys
import sst

# Add anytopo_utility directory to path
anytopo_path = os.path.join(os.path.dirname(__file__), '..', 'topology', 'anytopo_utility')
sys.path.insert(0, anytopo_path)

from sst.merlin.base import *
from sst.merlin.endpoint import *
from sst.merlin.interface import *
from sst.merlin.topology import *
from sst.ember import *

try:
    import networkx as nx
except ImportError:
    print("NetworkX is required. Please install NetworkX and try again.")
    sys.exit(1)

if __name__ == "__main__":

    UNIFIED_ROUTER_LINK_BW = 16

    BENCH = "FFT3D"
    BENCH_PARAMS = " nx=100 ny=100 nz=100 npRow=2"
    CPE = 1

    # Create a complete graph with 4 vertices
    G = nx.complete_graph(4)
    # Add endpoint attributes to each vertex
    for node in G.nodes():
        G.nodes[node]['endpoints'] = 2

    EPR = G.nodes[0]['endpoints']  # Endpoints per router

    ### Setup the topology
    topo = topoAny()
    topo.routing_mode = "source_routing"
    topo.topo_name = "complete_4_ember"
    topo.import_graph(G)

    defaults_z = PlatformDefinition.compose("firefly-defaults-test", [("firefly-defaults", "ALL")])
    defaults_z.addParamSet("nic", {
        "num_vNics": CPE,
    })
    PlatformDefinition.setCurrentPlatform("firefly-defaults-test")

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

    ep = EmberMPIJob(0, topo.getNumNodes(), numCores=CPE*EPR)
    ep.setEndpointNIC(endpointNIC)
    ep.addMotif("Init")
    ep.addMotif(BENCH + BENCH_PARAMS)
    ep.addMotif("Fini")
    ep.nic.nic2host_lat = "10ns"

    system = System()
    system.setTopology(topo)
    system.allocateNodes(ep, "linear")

    system.build()

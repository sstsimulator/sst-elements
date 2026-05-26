
import os
import sys
import sst
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


def prepare_complete_4(write_to_file: bool = False):
    '''
    Prepare a complete graph with 4 vertices.\n
    If write_to_file is True, write the graph to a GraphML file.\n
    Returns the graph (or file path) and topology name.
    '''
    # Create a complete graph with 4 vertices
    G = nx.complete_graph(4)
    # Add endpoint attributes to each vertex
    # For this test, we'll assign 2 endpoints per vertex
    for node in G.nodes():
        G.nodes[node]['endpoints'] = 2

    if write_to_file:
        # Write the graph to a GraphML file
        graph_file = "test_complete_4.graphml"
        nx.write_graphml(G, graph_file)
        print(f"Generated complete graph with 4 vertices and saved to {graph_file}")
        return graph_file, "complete_4"
    else:
        return G, "complete_4"

def prepare_cubical_graph(write_to_file: bool = False):
    '''
    Prepare a cubical graph with 8 vertices and 12 edges.\n
    If write_to_file is True, write the graph to a GraphML file.\n
    Returns the graph (or file path) and topology name.
    '''
    # Create a cubical graph with 8 vertices
    G = nx.cubical_graph()
    # Add endpoint attributes to each vertex
    # For this test, we'll assign 2 endpoints per vertex
    for node in G.nodes():
        G.nodes[node]['endpoints'] = 2

    if write_to_file:
        # Write the graph to a GraphML file
        graph_file = "test_cubical.graphml"
        nx.write_graphml(G, graph_file)
        print(f"Generated cubical graph with 8 vertices and saved to {graph_file}")
        return graph_file, "cubical"
    else:
        return G, "cubical"

def run_sst(input_graph: str | nx.Graph, topo_name: str, SR_routing_table: dict = None):

    UNIFIED_ROUTER_LINK_BW=UNIFIED_ROUTER_LINK_BW=16

    BENCH="FFT3D"
    BENCH_PARAMS=" nx=100 ny=100 nz=100 npRow=2"
    CPE=1
    EPR = input_graph.nodes[0]['endpoints']  # Endpoints per router

    ### Setup the topology, read graph from file
    topo = topoAny()
    topo.routing_mode = "source_routing"
    topo.topo_name = topo_name
    topo.import_graph(input_graph)
    # topo.set_verbose_level(16)  # Set verbose level for debugging

    defaults_z = PlatformDefinition.compose("firefly-defaults-Z",[("firefly-defaults","ALL")])
    defaults_z.addParamSet("nic",{
        "num_vNics": CPE,
        })
    PlatformDefinition.setCurrentPlatform("firefly-defaults-Z")
    # PlatformDefinition.setCurrentPlatform("firefly-defaults")


    # Set up the routers
    router = hr_router()
    router.link_bw = f"{UNIFIED_ROUTER_LINK_BW}Gb/s"
    router.flit_size = "32B"
    router.xbar_bw = f"{UNIFIED_ROUTER_LINK_BW*2}Gb/s" # 2x crossbar speedup
    router.input_latency = "20ns"
    router.output_latency = "20ns"
    router.input_buf_size = "32kB"
    router.output_buf_size = "32kB"
    router.num_vns = 2
    router.xbar_arb = "merlin.xbar_arb_rr"

    topo.router = router
    topo.link_latency = "20ns"
    topo.host_link_latency = "10ns"

    _SR_routing_table: dict

    if SR_routing_table is not None:
        # The source routing table can be passed from input arg
        _SR_routing_table = SR_routing_table
    else:
        # Otherwise, the source routing tables can be calculated here from the topology class method
        # by default, use All Shortest Paths (ASP) will be calculated
        _SR_routing_table = topo.calculate_routing_table()

    ### use endpointNIC
    endpointNIC = EndpointNIC(use_reorderLinkControl=True, topo=topo)

    # source routing table is passed to the plugin here
    endpointNIC.addPlugin("sourceRoutingPlugin", routing_table=_SR_routing_table)
    #### the following parameters will be passed down to linkcontrol via callback
    endpointNIC.link_bw = f"{UNIFIED_ROUTER_LINK_BW}Gb/s"
    endpointNIC.input_buf_size = "32kB"
    endpointNIC.output_buf_size = "32kB"
    endpointNIC.vn_remap = [0]

    # Set up VN remapping
    endpointNIC.vn_remap = [0]

    ep = EmberMPIJob(0,topo.getNumNodes(), numCores = CPE*EPR)
    ep.setEndpointNIC(endpointNIC)
    ep.addMotif("Init")
    ep.addMotif(BENCH+BENCH_PARAMS)
    ep.addMotif("Fini")
    ep.nic.nic2host_lat= "10ns"

    system = System()
    system.setTopology(topo)
    system.allocateNodes(ep,"linear")

    system.build()

if __name__ == "__main__":
    run_sst(*prepare_complete_4())
#!/usr/bin/env python
import os
import sys
import sst
import argparse

# Add parent directory to path for imports
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

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

    LOAD=0.5
    UNIFIED_ROUTER_LINK_BW=16

    ### Setup the topology, read graph from file
    topo = topoAny()
    topo.routing_mode = "source_routing"
    topo.topo_name = topo_name
    topo.import_graph(input_graph)
    # topo.set_verbose_level(16)  # Set verbose level for debugging
    
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
    
    targetgen=UniformTarget()

    ep = OfferedLoadJob(0,topo.getNumNodes())
    ep.setEndpointNIC(endpointNIC)
    ep.pattern=targetgen
    ep.offered_load = LOAD
    ep.link_bw = f"{UNIFIED_ROUTER_LINK_BW}Gb/s"
    ep.message_size = "32B"
    ep.collect_time = "200us"
    ep.warmup_time = "200us"
    ep.drain_time = "1000us" 

    system = System()
    system.setTopology(topo)
    system.allocateNodes(ep,"linear")

    system.build()
    # sst.setStatisticLoadLevel(10)
    # sst.setStatisticOutput("sst.statOutputCSV")
    # sst.setStatisticOutputOptions({
    #     "filepath" : f"stat_{topo_name}.csv"                                                                                                                                                                                                                                                                                                     ,
    #     "separator" : ", "
    # })
    # sst.enableAllStatisticsForAllComponents({"type":"sst.AccumulatorStatistic","rate":"0ns"})

from HPC_topos import *

if __name__ == "__main__":

    # Example: run complete graph with 4 vertices
    # run_sst(*prepare_complete_4())
    
    # from Slimfly import SlimflyTopo
    # sf_topo = SlimflyTopo(*sf_configs[0])  # Example parameters
    # sf_topo.set_endpoints_per_router(2)  # Example: set 2 endpoints per router
    # run_sst(sf_topo.get_nx_graph(), "slimfly")

    parser = argparse.ArgumentParser(description='Run SST simulation with different graph topologies')
    parser.add_argument('--topology', type=str, default='complete_4', 
                        choices=['complete_4', 'cubical', 'dallydragonfly', 'slimfly', 'polarfly', 'jellyfish'],
                        help='Topology to simulate.')
    parser.add_argument('--write-file', type=bool, default=False,
                        help='If True, write graph to GraphML file before running')
    
    args = parser.parse_args()
    
    if args.topology == 'complete_4':
        run_sst(*prepare_complete_4(write_to_file=args.write_file))
    elif args.topology == 'cubical':
        run_sst(*prepare_cubical_graph(write_to_file=args.write_file))
    elif args.topology == 'dallydragonfly':
        from DallyDragonfly import DallyDragonflyTopo
        ddf_topo = DallyDragonflyTopo(*ddf_configs[0])  # Example parameters
        ddf_topo.set_endpoints_per_router(2)  # Example: set 2 endpoints per router
        run_sst(ddf_topo.get_nx_graph(), "dallydragonfly")
    elif args.topology == 'slimfly':
        from Slimfly import SlimflyTopo
        sf_topo = SlimflyTopo(*sf_configs[0])  # Example parameters
        sf_topo.set_endpoints_per_router(2)  # Example: set 2 endpoints per router
        run_sst(sf_topo.get_nx_graph(), "slimfly")
    elif args.topology == 'polarfly':
        from Polarfly import PolarflyTopo
        pf_topo = PolarflyTopo(*pf_configs[0])  # Example parameters
        pf_topo.set_endpoints_per_router(2)  # Example: set 2 endpoints per router
        run_sst(pf_topo.get_nx_graph(), "polarfly")
    elif args.topology == 'jellyfish':
        from Jellyfish import JellyfishTopo
        jf_topo = JellyfishTopo(16, 4)  # Example parameters
        jf_topo.set_endpoints_per_router(2)  # Example: set 2 endpoints per router
        run_sst(jf_topo.get_nx_graph(), "jellyfish")
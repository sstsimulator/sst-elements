#!/usr/bin/env python
#
# Copyright 2009-2025 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2025, NTESS
# All rights reserved.
#
# Portions are copyright of other developers:
# See the file CONTRIBUTORS.TXT in the top level directory
# of the distribution for more information.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

"""
SST Merlin Any Topology Module

This module provides support for arbitrary network topologies using NetworkX graphs.
It supports source routing mode with routing table calculation.

For using source routing with this topology:
- Import graph: topo.import_graph(graph_input)
- Calculate routing table: routing_table = topo.calculate_routing_table()
- Pass to SourceRoutingPlugin via EndpointNIC.addPlugin()

See pymerlin-interface.py for SourceRoutingPlugin usage with EndpointNIC and routing table serialization.
"""

import sst
from sst.merlin.base import *
import os
from collections import defaultdict
from typing import Callable, Optional

try:
    import networkx as nx
except:
    pass

def _networkx_Dijkstra_shortest_path(input_nx_graph) -> dict:
    """
    An example implementation of *single* shortest path using Dijkstra's algorithm from NetworkX.
    Take the networkx graph object as input, return a routing table mapping from source router to destination router to a shortest path.
    Each path is represented as a list of router ids, and only one shortest path between two routers will be returned with weight 1.0.
    This will be used for source-routing mode in topoAny topology.
    """
    if input_nx_graph.number_of_nodes() == 0 or input_nx_graph.number_of_edges() == 0:
        raise AssertionError("Graph empty.")

    # assert that node IDs are integers
    vertices = list(input_nx_graph.nodes())
    if not all(isinstance(v, int) for v in vertices):
        raise AssertionError("Warning: Node IDs are not integers, mapping to integer IDs for internal processing.")

    # Compute shortest paths using Dijkstra's algorithm
    nx_routing_table = dict(nx.all_pairs_dijkstra_path(input_nx_graph))  # dict[int, dict[int, list[int]]]

    weighted_routing_table = defaultdict(lambda: defaultdict(list))  # dict[int, dict[int, list[tuple(float, list[int])]]]
    for src, dest_dict in nx_routing_table.items():
        for dest, path in dest_dict.items():
            weighted_routing_table[src][dest].append((1.0, path[1:]))  # Exclude source from hops

    return weighted_routing_table

def _networkx_Dijkstra_all_shortest_paths(input_nx_graph) -> dict:
    """
    An example implementation of *all* shortest paths using Dijkstra's algorithm from NetworkX.
    Take the networkx graph object as input, return a routing table mapping from source router to destination router to a list of paths.
    Each path is represented as a list of router ids, and the paths between two routers will be assigned equal weights.
    This will be used for source-routing mode in topoAny topology.
    """
    if input_nx_graph.number_of_nodes() == 0 or input_nx_graph.number_of_edges() == 0:
        raise AssertionError("Graph empty.")

    # assert that node IDs are integers
    vertices = list(input_nx_graph.nodes())
    if not all(isinstance(v, int) for v in vertices):
        raise AssertionError("Warning: Node IDs are not integers, mapping to integer IDs for internal processing.")

    # `nx.all_pairs_all_shortest_paths` is available for python 3.10+
    # https://networkx.org/documentation/stable/reference/algorithms/generated/networkx.algorithms.shortest_paths.generic.all_pairs_all_shortest_paths.html
    # The result is a dict of dicts keyed by source and target router IDs to a list paths.
    try:
        nx_routing_table = dict(nx.all_pairs_all_shortest_paths(input_nx_graph))
    except AttributeError:
        print("Might be due to old python version.")

    weighted_routing_table = defaultdict(lambda: defaultdict(list))  # dict[int, dict[int, list[tuple(float, list[int])]]]
    for src, dest_dict in nx_routing_table.items():
        for dest, paths in dest_dict.items():
            # Assign equal weight to all paths for this destination
            weight = 1.0 / len(paths)
            for path in paths:
                weighted_routing_table[src][dest].append((weight, path[1:]))  # Exclude source from hops

    return weighted_routing_table

class topoAny(Topology):

    def __init__(self):
        Topology.__init__(self)
        self._declareClassVariables(["link_latency", "host_link_latency", "topo_name", "simple_routing_entry_string",
                                     "loaded_graph", "edges", "connectivity_map", "num_R2R_ports_map",
                                     "tot_num_endpoints", "rtr_to_EPs", "EP_to_rtr", "simple_routing_table"])
        self._declareParams("shared",["num_routers", "routing_mode", "verbose_level", "source_routing_algo", "vn_ugal_num_valiant"])
        self._subscribeToPlatformParamSet("topology")
        self.loaded_graph = nx.empty_graph()
        self.rtr_to_EPs = defaultdict(list)   # dict[int, list[int]], default to empty list
        self.EP_to_rtr = {}                 # dict[int, int]
        self.simple_routing_table = {}

    def getName(self):
        return self.topo_name

    def set_verbose_level(self, level: int):
        self.verbose_level = level

    def add_endpoint_mapping(self, router_id: int, endpoint_id: int):
        # This will be used to store the mapping from router ID to list of endpoint IDs and vice versa
        self.rtr_to_EPs[router_id].append(endpoint_id)
        self.EP_to_rtr[endpoint_id] = router_id
        # print(f"In python, mapping endpoint {endpoint_id} to router {router_id}")

    def get_endpoint_router_mapping(self) -> dict:
        return self.EP_to_rtr

    def get_num_endpoints_for_router(self, router_id: int) -> int:
        return len(self.rtr_to_EPs[router_id])

    def calculate_routing_table(self, routingAlgo: Optional[Callable] = None):
        """This is an interface for generating a routing table. \n
        The implementation could be dijkstra with all shortest paths or other algorithms. \n
        It optionally takes an input function that is defined by the user to generate custom routing tables.

        Note that the routing table should be: a dictionary of dictionaries, mapping from source router ID to destination router ID to a list of (weight, path) tuples.
        The sum of weights of paths between each source-destination pair should be normalized to 1, but in this case all weights are 1 since there is only one path per pair.
        """
        # if no routingAlgo is provided, use default Dijkstra all shortest paths
        if routingAlgo is None:
            routingAlgo = _networkx_Dijkstra_shortest_path

        try:
            result = routingAlgo(self.loaded_graph)
            return result
        except Exception as e:
            raise AssertionError(f"Failed to generate routing table using the provided algorithm: {e}")

    def getNumNodes(self):
        return self.tot_num_endpoints

    def getRouterNameForId(self,rtr_id):
        return "router%d"%(rtr_id)

    # def findRouterByLocation(self,rtr_id):
    #     return self.getRouterNameForId(self.getRouterNameForId(rtr_id))

    def import_graph(self, graph_input):
        """
        Import a graph from either a NetworkX graph object or a file path.

        Args:
            graph_input: Either a NetworkX Graph object or a string path to a GraphML file
        """
        # Handle file path input
        if isinstance(graph_input, str):
            if not os.path.exists(graph_input):
                raise AssertionError(f"Graph file {graph_input} not found")

            try:
                G = nx.read_graphml(graph_input)
                print(f"Loaded graph from file {graph_input} with {len(G.nodes())} vertices and {len(G.edges())} edges")
            except Exception as e:
                raise AssertionError(f"Failed to load GraphML file: {e}")

        # Handle NetworkX graph object input
        elif isinstance(graph_input, (nx.Graph, nx.MultiGraph, nx.DiGraph, nx.MultiDiGraph)):
            G = graph_input
            print(f"Loaded NetworkX graph with {len(G.nodes())} vertices and {len(G.edges())} edges")

        else:
            raise AssertionError("graph_input must be either a file path (str) or a NetworkX Graph object")

        # Validate graph properties
        if G.is_directed():
            raise AssertionError("FATAL: Input graph must be undirected, in order to assure bidirectional links")
        if isinstance(G, nx.MultiGraph):
            print("NOTE: Inputting a multi-graph, multiple links can present between two routers. Please be aware of this.")

        # Convert node IDs to integers if they aren't already
        vertices = list(G.nodes())
        if not all(isinstance(v, int) for v in vertices):
            node_id_map = {node: i for i, node in enumerate(vertices)}
            G = nx.relabel_nodes(G, node_id_map, copy=True)

        # by default it should be an undirected simple graph (nx.Graph)
        self.loaded_graph = G
        self._process_graph()

    def _process_graph(self):
        # Extract local data structures
        vertices = list(self.loaded_graph.nodes())
        self.edges = list(self.loaded_graph.edges())

        # Verify all nodes are integers
        assert all(isinstance(v, int) for v in vertices), "Vertex IDs must be integers"

        self.num_routers = len(vertices)

        global_endpoint_id = 0
        # Get number of endpoints per vertex from graph attributes or use default
        for vertex in vertices:
            # Try to get endpoint counts from node attributes
            if 'endpoints' in self.loaded_graph.nodes[vertex]:
                for _ in range(int(self.loaded_graph.nodes[vertex]['endpoints'])):
                    self.add_endpoint_mapping(vertex, global_endpoint_id)
                    global_endpoint_id += 1
            else:
                raise AssertionError("Each vertex in the input graph must have an 'endpoints' attribute specifying the number of attached endpoints.")
        self.tot_num_endpoints = global_endpoint_id

        # Build connectivity map for each vertex and record number of R2R ports
        self.connectivity_map = {}
        self.num_R2R_ports_map = {}
        for vertex in vertices:
            self.connectivity_map[vertex] = {}
            port_counter = 0
            # Assign ports to neighboring routers, handling multi-edges
            for edge in self.loaded_graph.edges(vertex, data=False):
                # edge is a tuple (u, v) or (u, v, key) for MultiGraph
                neighbor = edge[1] if edge[0] == vertex else edge[0]
                if neighbor not in self.connectivity_map[vertex]:
                    self.connectivity_map[vertex][neighbor] = []
                self.connectivity_map[vertex][neighbor].append(port_counter)
                port_counter += 1
            # Record the number of R2R ports for this router
            self.num_R2R_ports_map[vertex] = port_counter

        # Calculate a simple routing table for the intra-router network, this will only be used for untimed packets
        self.simple_routing_table = self.calculate_routing_table()

    def build(self, endpoint):
        if self.loaded_graph.number_of_nodes() == 0 or self.loaded_graph.number_of_edges() == 0:
            raise AssertionError("Invalid graph data, please assign valid graph data via import_graph() before build().")

        # Create routers
        routers = []
        topos=[]
        for r in range(self.num_routers):
            router = self._instanceRouter(self.num_R2R_ports_map[r] + self.get_num_endpoints_for_router(r),r)
            routers.append(router)

            # Build connectivity string: "dest_router_id,port1,port2;dest_router_id,..."
            connectivity_entries = []
            for dest_router, ports in self.connectivity_map[r].items():
                port_str = ','.join(map(str, ports))
                connectivity_entries.append(f"{dest_router},{port_str}")
            connectivity_str = ';'.join(connectivity_entries)

            # # Build routing table string: "dest_router_id,next_hop1,next_hop2;dest_router_id,..."
            # routing_entries = []
            # for dest_router, next_hops in routing_tables[r].items():
            #     next_hop_str = ','.join(map(str, next_hops))
            #     routing_entries.append(f"{dest_router},{next_hop_str}")
            # routing_table_str = ';'.join(routing_entries)

            # Set topology component
            topo = routers[r].setSubComponent(self.router.getTopologySlotName(), "merlin.anytopo", 0)
            topo.addParams(self._getGroupParams("shared"))
            topo.addParams({
                "num_R2N_ports": self.get_num_endpoints_for_router(r),
                "num_R2R_ports": self.num_R2R_ports_map[r],
                "connectivity": connectivity_str
            })
            topo.addParam("simple_routing_entry_string", serialize_routing_entry(self.simple_routing_table[r], r))
            topos.append(topo)

        # Build links between routers
        for edge in self.edges:
            src_router, dst_router = edge
            src_ports = self.connectivity_map[src_router][dst_router]
            dst_ports = self.connectivity_map[dst_router][src_router]
            if len(src_ports) != len(dst_ports):
                raise AssertionError(f"The edge between routers {src_router} and {dst_router} must have an equal number of ports on both ends.")

            # Create links for each port pair
            for i, (src_port, dst_port) in enumerate(zip(src_ports, dst_ports)):
                link_name = f"link_{src_router}_{dst_router}_{i}"
                link = sst.Link(link_name)

                # Connect routers
                routers[src_router].addLink(link, f"port{src_port}", self.link_latency)
                routers[dst_router].addLink(link, f"port{dst_port}", self.link_latency)

        # Connect endpoints to routers
        for r in range(self.num_routers):
            endpoint_to_port_map = {}
            for local_id, global_id in enumerate(self.rtr_to_EPs[r]):
                (networkIF, port_name) = endpoint.build(global_id, {})
                if networkIF:
                    nicLink = sst.Link("link_r%d_localEP%d"%(r, local_id))
                    nicLink.connect( (networkIF, port_name, self.host_link_latency), (routers[r], "port%d"%(self.num_R2R_ports_map[r] + local_id), self.host_link_latency) )

                    endpoint_to_port_map[global_id] = self.num_R2R_ports_map[r] + local_id
                    # print(f"router {r}'s port{self.num_R2R_ports_map[r] + local_id} is connected to EP {global_id}'s port {port_name}")
                else:
                    raise AssertionError(f"Failed to build endpoint {global_id}, local EP ID {local_id} for router {r}")

            topos[r].addParam("endpoint_to_port_map", endpoint_to_port_map)
            topos[r].addParam("port_to_endpoint_map", {v: k for k, v in endpoint_to_port_map.items()})
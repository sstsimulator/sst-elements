import networkx as nx
from abc import ABC

# Some already-calculated HPC topology configurations
# 10 < R < 300
# slimfly configurations:
sf_configs = [(18, 5), (32, 6), (50, 7), (98, 11), (128, 12), (162, 13), (242, 17)]

ddf_configs = [(36, 5), (114, 8), (264, 11)]

# this contains irregular graph, which is not considered in this work
pf_configs = [(7, 3), (13, 4), (21, 5), (31, 6), (57, 8), (73, 9), (91, 10), (133, 12), (183, 14), (273, 17)]

pf_regular_configs = [(v,d) for v, d in pf_configs if v*d%2==0]

class HPC_topo(ABC):
    '''
    In general, we will use the number of routers `R` and the inter-router graph degree `d` to represent a HPC topology. \n
    See child classes for specific topologies.
    '''
    def __init__(self, topo_name: str = "", nx_graph: nx.Graph = None):
        self.nx_graph = nx_graph if nx_graph is not None else nx.Graph()
        self.topo_name = topo_name

    def get_nx_graph(self) -> nx.Graph:
        return self.nx_graph

    def get_topo_name(self) -> str:
        return self.topo_name

    def set_endpoints_per_router(self, num_endpoints_per_router: int):
        '''
        Set the number of endpoints per router for all routers in the topology.\n
        This will assign an equal number of endpoints to each router.
        '''
        for node in self.nx_graph.nodes():
            self.nx_graph.nodes[node]['endpoints'] = num_endpoints_per_router

    def calculate_all_shortest_paths_routing_table(self) -> dict:
        '''
        This uses networkx lib to calculates the shortest paths between all pairs of routers in the topology.\n
        Returns a dictionary of paths, one path per source-destination pair.

        the routing table should be: a dictionary of dictionaries, mapping from source router ID to destination router ID to a list of (weight, path) tuples.
        The sum of weights of paths between each source-destination pair should be normalized to 1.
        '''
        G = self.nx_graph
        vertices = G.nodes()
        routing_table = {}

        for v1 in vertices:
            routing_table[v1] = {}
            for v2 in vertices:
                if v1 != v2:
                    all_paths = list(nx.all_shortest_paths(G, source=v1, target=v2))  # (This is not an optimized implementation), consider to use `nx.all_pairs_all_shortest_paths` or other algorithms
                    num_paths = len(all_paths)
                    weight = 1.0 / num_paths if num_paths > 0 else 0.0
                    routing_table[v1][v2] = [(weight, path) for path in all_paths]

        return routing_table


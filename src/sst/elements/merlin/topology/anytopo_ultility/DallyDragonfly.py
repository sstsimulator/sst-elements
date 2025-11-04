from HPC_topos import *
from statistics import mean

class DallyDragonflyTopo(HPC_topo):
    '''
    Dally Dragonfly Topology, see paper: \n
    'Kim, John, et al. "Technology-driven, highly-scalable dragonfly topology." ACM SIGARCH Computer Architecture News 36.3 (2008): 77-88.'
    '''
    def __init__(self, *args, **kwargs):
        super().__init__("dally_dragonfly")

        # DallyDragonfly parameters:
        # Number of routers per group = a, Number of groups = g,
        # Number of inter-group links per router = h
        # Number of terminals per router = p (p is not modeled in the inter-router graph),
        # Router radix = k = p+a+h-1
        # a=2p=2h, g=ah+1
        # Input to this class is the number of routers = R = a*g; inter-router graph degree = d = a+h-1, then we deduct all parameters

        if len(args) == 2 and isinstance(args[0], int) and isinstance(args[1], int):
            self.generate_DDF_topo(args[0], args[1])
        else:
            raise ValueError('Input arguements not accepted.')

    def generate_DDF_topo(self, R, d): #Note that the k here includes host ports
        self.routers_per_group=(2*(d+1))//3 # a+h-1=d => a=2(d+1)/3
        if (2*(d+1))%3:
            raise ValueError("input DDF router radix wrong")
        intergroup_link_per_router=self.routers_per_group//2
        if self.routers_per_group%2:
            raise ValueError("input DDF router radix wrong")
        self.num_groups=self.routers_per_group*intergroup_link_per_router+1

        # Create a graph
        G = nx.Graph()

        # Add routers
        for group in range(self.num_groups):
            for router_in_group in range(self.routers_per_group):
                router_id = group * self.routers_per_group + router_in_group
                G.add_node(router_id, group=group, adjacency=dict())
                # adjacent_groups contains (core group id -> core router id)

        # Add intragroup links
        for group in range(self.num_groups):
            routers_in_group = list(range(group * self.routers_per_group, (group + 1) * self.routers_per_group))
            for src in routers_in_group:
                for dest in routers_in_group:
                    if src!=dest:
                        G.add_edge(src, dest, intergroup=False)

        next_router=[0]*self.num_groups #note that this is the relative router id in a group
        groups_to_connect=[]
        for i in range(self.num_groups):
            groups_to_connect.append([j for j in range(self.num_groups) if j!=i])
        # Add intergroup links
        for group in range(self.num_groups):
            while len(groups_to_connect[group])!=0:
                src_r = next_router[group]+group*self.routers_per_group
                dest_g = groups_to_connect[group].pop(0)
                groups_to_connect[dest_g].remove(group)
                dest_r = next_router[dest_g]+dest_g*self.routers_per_group
                next_router[group]=(next_router[group]+1)%self.routers_per_group
                next_router[dest_g]=(next_router[dest_g]+1)%self.routers_per_group
                assert(dest_r<R)
                assert(src_r<R)
                G.add_edge(src_r, dest_r, intergroup=True)
                G.nodes[src_r]["adjacency"][dest_g]=dest_r
                G.nodes[dest_r]["adjacency"][group]=src_r

        assert(mean([i[1] for i in list(G.degree)])==d)
        self.nx_graph=G
        return

    def default_routing(self):
        '''
        This calcuates the default Dally dragonfly routing paths between all pairs of routers.\n
        Returns a dictionary of paths, one path per source-destination pair, according to \n
        'Kim, John, et al. "Technology-driven, highly-scalable dragonfly topology." ACM SIGARCH Computer Architecture News 36.3 (2008): 77-88.'

        the routing table should be: a dictionary of dictionaries, mapping from source router ID to destination router ID to a list of (weight, path) tuples.
        The sum of weights of paths between each source-destination pair should be normalized to 1, but in this case all weights are 1 since there is only one path per pair.
        '''
        G=self.nx_graph
        vertices = G.nodes()
        vertex_pairs = [(v1, v2) for v1 in vertices for v2 in vertices if v1 != v2]
        routing_table={}

        for (v1, v2) in vertex_pairs:
            if v1 not in routing_table:
                routing_table[v1] = {}
            routing_table[v1][v2]=[]
            path=[]
            if G.nodes[v1]["group"]==G.nodes[v2]["group"]: # in the same group
                path=[v1, v2]
                assert(G.has_edge(v1, v2))
            else: # not in the same group
                if G.nodes[v2]["group"] in G.nodes[v1]["adjacency"].keys():
                    adjacent_r=G.nodes[v1]["adjacency"][G.nodes[v2]["group"]]
                    assert(G.has_edge(v1, adjacent_r))
                    if adjacent_r == v2:
                        path=[v1, v2]
                    else:
                        assert(G.nodes[v2]["group"]==G.nodes[adjacent_r]["group"] and G.has_edge(adjacent_r, v2))
                        path=[v1, adjacent_r, v2]
                else: #route to the correct router in the same group
                    group_id=G.nodes[v1]["group"]
                    routers_in_group = list(range(group_id * self.routers_per_group, (group_id + 1) * self.routers_per_group))
                    next_router=-1
                    for r in routers_in_group:
                        if r != v1:
                            if G.nodes[v2]["group"] in G.nodes[r]["adjacency"].keys():
                                next_router=r
                                break
                    assert(next_router!=-1)
                    adjacent_r=G.nodes[next_router]["adjacency"][G.nodes[v2]["group"]]
                    if adjacent_r == v2:
                        path=[v1, next_router, adjacent_r]
                    else:
                        assert(G.nodes[v2]["group"]==G.nodes[adjacent_r]["group"] and G.has_edge(adjacent_r, v2))
                        path=[v1, next_router, adjacent_r, v2]
            routing_table[v1][v2].append((1.0, path))

        return routing_table

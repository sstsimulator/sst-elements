from HPC_topos import *

class JellyfishTopo(HPC_topo):

    def __init__(self, *args, **kwargs):
        '''
        Jellyfish (Random Regular Graph) Topology, see paper: \n
        'Singla, Ankit, et al. "Jellyfish: Networking data centers randomly." 9th USENIX Symposium on Networked Systems Design and Implementation (NSDI 12). 2012.'
        '''
        if len(args) == 2 and isinstance(args[0], int) and isinstance(args[1], int):
            super(JellyfishTopo, self).__init__("jellyfish")
            degree=args[1]
            num_vertices=args[0]
            self.nx_graph = nx.random_regular_graph(degree, num_vertices, seed=0)

        elif len(args) == 3 and isinstance(args[0], int) and isinstance(args[1], int) and isinstance(args[2], int):
            super(JellyfishTopo, self).__init__("jellyfish")
            degree=args[1]
            num_vertices=args[0]
            self.nx_graph = nx.random_regular_graph(degree, num_vertices, seed=args[2])

        else:
            raise ValueError('Input arguements not accepted.')
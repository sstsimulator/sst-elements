#!/usr/bin/python

#import graphviz

total_pes = 8
pe_num = 0

#g = graphviz.Graph("Hardware Description", filename='hardware.cfg')

### create nodes
#for counter in range( 0, total_pes ):
    #g.node( str(counter), 'any')

### create edges for fully conntected graph
#for x in range( 0, total_pes ):
    #for y in range( x + 1, total_pes ):
        #g.edge( str(x), str(y))

##g.view()

import os
import networkx as nx
from networkx.algorithms import isomorphism
from networkx.drawing.nx_pydot import write_dot
from networkx.algorithms import isomorphism as iso

hw_graph = nx.DiGraph()
sw_graph = nx.DiGraph()

### create nodes
for counter in range( 0, total_pes ):
    hw_graph.add_node( counter, label="any")

## create edges for fully conntected graph
for x in range( 0, total_pes ):
    for y in range( x + 1, total_pes ):
        hw_graph.add_edge( x, y )

## app graphFile
sw_graph.add_node( 1, label="any")
sw_graph.add_node( 2, label="any")
sw_graph.add_node( 3, label="any")
sw_graph.add_node( 4, label="any")
sw_graph.add_node( 5, label="any")
sw_graph.add_node( 6, label="any")
sw_graph.add_node( 7, label="any")
sw_graph.add_node( 8, label="any")

sw_graph.add_edge( 1, 4 )
sw_graph.add_edge( 2, 4 )
#sw_graph.add_edge( 2, 5 )
#sw_graph.add_edge( 3, 5 )
#sw_graph.add_edge( 4, 6 )
#sw_graph.add_edge( 5, 7 )


# Nodes 'a', 'b', 'c' and 'd' form a column.
# Nodes 'g', 'h', 'i' and 'j' form a column.
g1edges = [['a', 'g'], ['a', 'h'], ['a', 'i'],
            ['b', 'g'], ['b', 'h'], ['b', 'j'],
            ['c', 'g'], ['c', 'i'], ['c', 'j'],
            ['d', 'h'], ['d', 'i'], ['d', 'j']]

# Nodes 1,2,3,4 form the clockwise corners of a large square.
# Nodes 5,6,7,8 form the clockwise corners of a small square
g2edges = [[1, 2], [2, 3], [3, 4], [4, 1],
            [5, 6], [6, 7], [7, 8], [8, 5],
            [1, 5], [2, 6], [3, 7], [4, 8]]

g1 = nx.Graph()
g2 = nx.Graph()
g1.add_edges_from(g1edges)
g2.add_edges_from(g2edges)
#g3 = g2.subgraph([1, 2, 3, 4])
#gm = iso.GraphMatcher(g1, g2)

gm = iso.DiGraphMatcher(hw_graph, sw_graph)
for subgraph in gm.subgraph_isomorphisms_iter():
    print( subgraph )

gm = iso.DiGraphMatcher(sw_graph, hw_graph)
for subgraph in gm.subgraph_isomorphisms_iter():
    print( subgraph )

nx.drawing.nx_pydot.write_dot(sw_graph, '/tmp/sw.dot')
nx.drawing.nx_pydot.write_dot(hw_graph, '/tmp/hw.dot')

os.system("dot -Tpdf /tmp/sw.dot -o /tmp/sw_test.pdf")
os.system("dot -Tpdf /tmp/hw.dot -o /tmp/hw_test.pdf")


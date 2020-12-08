#!/usr/bin/python

import graphviz

total_pes = 8
pe_num = 0

g = graphviz.Graph("Hardware Description", filename='hardware.cfg')

## create nodes
for counter in range( 0, total_pes ):
    g.node( str(counter), 'any')

## create edges for fully conntected graph
for x in range( 0, total_pes ):
    for y in range( x + 1, total_pes ):
        g.edge( str(x), str(y))

#g.view()


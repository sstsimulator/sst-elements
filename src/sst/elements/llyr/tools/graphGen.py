#!/usr/bin/env python3
import numpy as np
import networkx as nx
import matplotlib.pyplot as plt
from collections import defaultdict

# mesh, torus, all
output_type = "all"

# node operation
row_ops = 0
col_ops = 0
edge_ops = 0

G = nx.DiGraph()

#for rows in (2, 4, 8, 16, 32, 64):
for rows in (2, 4, 8, 16, 32):
   cols = rows
   num_nodes = rows * cols
   graph_topology = np.arange(num_nodes).reshape(rows, cols)

   fileName = "graph_" + output_type + "_" + str(num_nodes) + ".dot"

   for i in range(0, num_nodes):
      G.add_node(i)

   if( output_type == "mesh" ):
      node_num = 0
      for i in range(0, rows):
         for j in range(0, cols):
            num_neighbors = 0
            for i_x in range(max(0,i-1), min(cols,i+2)):
               for j_y in range(max(0,j-1), min(rows,j+2)):
                  if( i == i_x and j == j_y ):
                     continue
                  elif( i < i_x or i > i_x ):
                     if( j < j_y or j > j_y ):
                        continue
                  num_neighbors = num_neighbors + 1

            for i_x in range(max(0,i-1), min(cols,i+2)):
               for j_y in range(max(0,j-1), min(rows,j+2)):
                  if( i == i_x and j == j_y ):
                     continue
                  elif( i < i_x or i > i_x ):
                     if( j < j_y or j > j_y ):
                        continue
                  G.add_edge(node_num, graph_topology[i_x][j_y])

            node_num = node_num + 1

   elif( output_type == "all" ):
      node_list = {new_list: [] for new_list in range(num_nodes)}
      for node in node_list:
         node_num = 0
         for i in range(0, rows):
            for j in range(0, cols):
               if( str(node_num) != str(node) ):
                  node_list[node].append(node_num)
               node_num = node_num + 1

      for node in node_list:
         for neighbor in node_list[node]:
            G.add_edge(node, neighbor)

   else:
      node_num = 0
      for i in range(0, rows):
         for j in range(0, cols):
            num_neighbors = 0
            for i_x in range(max(0,i-1), min(cols,i+2)):
               for j_y in range(max(0,j-1), min(rows,j+2)):
                  if( i == i_x and j == j_y ):
                     continue
                  num_neighbors = num_neighbors + 1

            for i_x in range(max(0,i-1), min(cols,i+2)):
               for j_y in range(max(0,j-1), min(rows,j+2)):
                  if( i == i_x and j == j_y ):
                     continue
                  G.add_edge(node_num, graph_topology[i_x][j_y])
            node_num = node_num + 1

   ## All nodes can do this op
   if( row_ops == 0 and col_ops == 0 and edge_ops == 0 ):
      for node in G.nodes():
         G.nodes[node]["op"] = "any"
   else:
      for node in G.nodes():
         G.nodes[node]["op"] = "arithmetic"

   ## First and last row have different op
   if( row_ops == 1 ):
      node = 0
      for i in range(0, rows):
         for j in range(0, cols):
            if( i == 0 or i == rows - 1 ):
               G.nodes[node]["op"] = "memory"
            node = node + 1

   ## First and last column have different op
   if( col_ops == 1 ):
      node = 0
      for i in range(0, rows):
         for j in range(0, cols):
            if( j == 0 or j == cols - 1 ):
               G.nodes[node]["op"] = "memory"
            node = node + 1

   ## Edges have different op
   if( edge_ops == 1 ):
      node = 0
      for i in range(0, rows):
         for j in range(0, cols):
            if( j == 0 or j == cols - 1 or i == 0 or i == rows - 1 ):
               G.nodes[node]["op"] = "memory"
            node = node + 1

   nx.nx_pydot.write_dot(G, fileName)

   nx.nx_pydot.graphviz_layout(G, prog="neato") # prog options: neato, dot, fdp, sfdp, twopi, circo
   nx.draw(G, pos = nx.nx_pydot.graphviz_layout(G),
      node_size=1200, node_color='lightblue', linewidths=0.25,
      font_size=10, font_weight='bold', with_labels=True)
   #plt.show()



#!/usr/bin/python3
import numpy as np
from collections import defaultdict

#for file in graph_*; do ../convert_dot.py -i $file -o "${file}.dot"; done

# mesh, torus, all
output_type = "mesh"

#for rows in (2, 4, 8, 16, 32, 64):
for rows in (2, 4):
   cols = rows
   rows = 5
   cols = 5
   num_nodes = rows * cols
   graph_topology = np.arange(num_nodes).reshape(rows, cols)

   fileName = "graph_" + output_type + "_" + str(num_nodes)
   graphFile = open(fileName, "w")

   print("Generating graph file " + fileName)
   graphFile.write( str(num_nodes) + "\n\n" )
   for i in range(0, num_nodes):
      #print(i)
      graphFile.write( str(i) + " " + str(0) )
      graphFile.write( "\n" )

   graphFile.write( "\n" )

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
            graphFile.write( str(num_neighbors) )
            graphFile.write( "\n" )

            print( str(max(0,i-1)) + " --- " + str(min(cols,i+2)) )
            for i_x in range(max(0,i-1), min(cols,i+2)):
               for j_y in range(max(0,j-1), min(rows,j+2)):
                  print( str(i) + " " + str(i_x) + " " + str(j) + " " + str(j_y) )
                  if( i == i_x and j == j_y ):
                     continue
                  elif( i < i_x or i > i_x ):
                     if( j < j_y or j > j_y ):
                        continue
                  graphFile.write( str(node_num) + " " + str(graph_topology[i_x][j_y]) )
                  graphFile.write( "\n" )
            node_num = node_num + 1
            graphFile.write( "\n" )
   elif( output_type == "all" ):
      node_list = {new_list: [] for new_list in range(num_nodes)}
      for node in node_list:
         node_num = 0
         for i in range(0, rows):
            for j in range(0, cols):
               #print(str(node) + " " + str(node_num) + "\n")
               if( str(node_num) != str(node) ):
                  node_list[node].append(node_num)
                  #print("**" + str(node) + " " + str(node_num) + "\n")
               node_num = node_num + 1

      for node in node_list:
         graphFile.write( str(num_nodes - 1) )
         graphFile.write( "\n" )

         for neighbor in node_list[node]:
            graphFile.write( str(node) + " " + str(neighbor) )
            graphFile.write( "\n" )

         graphFile.write( "\n" )

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
            graphFile.write( str(num_neighbors) )
            graphFile.write( "\n" )

            for i_x in range(max(0,i-1), min(cols,i+2)):
               for j_y in range(max(0,j-1), min(rows,j+2)):
                  if( i == i_x and j == j_y ):
                     continue
                  graphFile.write( str(node_num) + " " + str(graph_topology[i_x][j_y]) )
                  graphFile.write( "\n" )
            node_num = node_num + 1
            graphFile.write( "\n" )

   graphFile.close()

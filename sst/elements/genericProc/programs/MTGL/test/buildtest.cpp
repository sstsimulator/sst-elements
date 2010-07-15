/*  _________________________________________________________________________
 *
 *  MTGL: The MultiThreaded Graph Library
 *  Copyright (c) 2008 Sandia Corporation.
 *  This software is distributed under the BSD License.
 *  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 *  the U.S. Government retains certain rights in this software.
 *  For more information, see the README file in the top MTGL directory.
 *  _________________________________________________________________________
 */

/****************************************************************************/
/*! \file buildtest.cpp

    \brief Use buildGraph in buildmap.c to create node and edge memory map
           files from DIMACS graph file and print vertex and edge sets from
           memory maps.  buildGraph will print summary output to be redirected
           to a file for use in loading node and edge memory map files.
 
    \author Vitus Leung (vjleung@sandia.gov)

    \date 1/5/2007
 */
/****************************************************************************/

#include <cstdlib>
#include <cstdio>

#include "qalloc.h"
#include "CPPGraph.h"

extern "C" {
C_Graph buildGraph(int argc, char* argv[]);
}

int main(int argc, char* argv[])
{
  C_Graph g;
  g = (C_Graph) buildGraph(argc, argv);
  int i, j, numnodes = g.order;
  C_Node* nodes = g.vertices;
  C_Edge* edges = g.edges;
  int* adjs = g.adjacencies;

  printf("V = {\n");
  for (i = 0; i < numnodes - 10; i += 10)
  {
    for (j = 0; j < 9; ++j) printf("%6d, ", nodes[i + j].id);

    printf("%6d,\n", nodes[i + 10].id);
  }

  for (j = i; j < numnodes - 1; ++j) printf("%6d, ", nodes[j].id);

  printf("%6d}\n", nodes[numnodes - 1].id);
  printf("E = {");

  for (i = 0; i < numnodes; ++i)
    for (j = 0; j < nodes[i].degree(); ++j)
      if (nodes[i].id == edges[adjs[nodes[i].offset + j]].vert1->id)
        printf("\n(%6d, %6d),",
               nodes[i].id,
               edges[adjs[nodes[i].offset + j]].vert2->id);

  printf("\b}\n");

  qalloc_cleanup();
  return 0;
}

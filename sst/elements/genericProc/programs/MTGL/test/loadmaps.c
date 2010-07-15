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
/*! \file loadmaps.c

    \brief Load node and edge memory map files from summary file generated when
           the memory map files were created.

    \author Vitus Leung (vjleung@sandia.gov)

    \date 1/5/2007
*/
/****************************************************************************/

#include <cstdio>
#include <cstring>
#include <unistd.h>

#ifndef __MTA__
#include <inttypes.h>              /* wcm: not on MTA */
#endif

#include "qalloc.h"
#include "Graph.h"

void input_error(char* error_msg)
{
  fprintf(stderr, "%s\n", error_msg);
  exit(1);
}

struct Graph loadGraph(char* filename)
{
  void* r;
  void* r2;
  void* r3;
  struct Graph g;
  struct Node* nodes;
  struct Edge* edges;
  int* adjs;
  int i;
  int j;

  char* fn = filename;

  int numnodes, numedges;

  char buf[80];             /* wcm: moved, can't declare after code in C */
  int retVal;

  FILE* file;

  /* open file */
  if ((file = fopen(fn, "r")) == NULL)
  {
    fprintf(stderr, "Cannot open file %s\n", fn);
    exit(1);
  }

  if (1 != (retVal = fscanf(file, "numnodes: %d\n", &numnodes)))
  {
    input_error("number of nodes not supplied");
  }

  g.order = numnodes;
  printf("numnodes: %d\n", numnodes);

  if (1 != (retVal = fscanf(file, "numedges: %d\n", &numedges)))
  {
    input_error("number of edges not supplied");
  }

  g.size = numedges;
  printf("numedges: %d\n", numedges);

  if (1 != (retVal = fscanf(file, "makestatmap(nodes): %s\n", buf)))
  {
    input_error("name of nodes map file not supplied");
  }

  /* loading nodes map */
  r = qalloc_loadmap(buf);
  printf("makestatmap(nodes): %s\n", buf);

  if (1 != (retVal = fscanf(file, "makestatmap(edges): %s\n", buf)))
  {
    input_error("name of edges map file not supplied");
  }

  /* loading edges map */
  r2 = qalloc_loadmap(buf);
  printf("makestatmap(edges): %s\n", buf);

  if (1 != (retVal = fscanf(file, "makestatmap(adjs): %s\n", buf)))
  {
    input_error("name of adjs map file not supplied");
  }

  /* loading adjs map */
  r3 = qalloc_loadmap(buf);
  printf("makestatmap(adjs): %s\n", buf);

  if (1 != (retVal = fscanf(file, "statmalloc(node): %p\n", &nodes)))
  {
    input_error("pointer to node data structure not supplied");
  }

  g.vertices = nodes;
  printf("statmalloc(node): %p\n", nodes);

  if (1 != (retVal = fscanf(file, "statmalloc(edge): %p\n", &edges)))
  {
    input_error("pointer to edge data structure not supplied");
  }

  g.edges = edges;
  printf("statmalloc(edge): %p\n", edges);

  if (1 != (retVal = fscanf(file, "statmalloc(adj): %p\n", &adjs)))
  {
    input_error("pointer to adj data structure not supplied");
  }

  g.adjacencies = adjs;
  printf("statmalloc(adj): %p\n", adjs);

  qalloc_checkpoint();

/*
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
  {
    for (j = 0; j < nodes[i].deg; ++j)
    {
      printf("\n(%6d, %6d),", nodes[i].id,
             edges[nodes[i].offset + j].vert2->id);

      printf("\b}\n");
    }
  }
*/

  return g;
}

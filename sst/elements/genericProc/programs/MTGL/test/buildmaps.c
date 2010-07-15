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
/*! \file buildmaps.c

    \brief Build node and edge memory map files from DIMACS graph file.  Also
           produces summary output to be redirected to a file for use in
           loading node and edge memory map files.

    \author Vitus Leung (vjleung@sandia.gov)

    \date 1/5/2007
*/
/****************************************************************************/

#include "qalloc.h"
#include "Graph.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <inttypes.h>
#include <sys/stat.h>

void input_error(char* error_msg, int line)
{
  fprintf(stderr, "line %i: %s\n", line, error_msg);
  exit(1);
}

struct Graph buildGraph(int argc, char** argv)
{
  void* r;
  void* r2;
  void* r3;
  struct Graph g;
  struct Node* nodes;
  struct Edge* edges;
  int* adjs;
  int i /*, j*/;

  char* fn = argv[1];

  int numnodes, numedges;

  FILE* file_r;
  FILE* file_w;
  struct stat info;
  char* problemDesc = NULL;
  /* open file_r */
  if ((file_r = fopen(fn, "r")) == NULL)
  {
    fprintf(stderr, "Cannot open file %s\n", fn);
    exit(1);
  }
  /*get size of file_r */
  stat(fn, &info);
  int numChars = info.st_size;
  char* buf = (char*) malloc((numChars + 1) * sizeof(char));

#ifdef __MTA__
  /* read file_r */
  fread(buf, sizeof(char), numChars, file_r);

  // find number of newlines
  int numLines = 0;
  for (i = 0; i < numChars; i++)
  {
    if (buf[i] == '\n') numLines++;
  }
  int* starts = (int*) malloc(sizeof(int) * (numLines + 1));
  starts[0] = 0;
  int x = 1;
  // find start of each line
  for (i = 0; i < numChars; i++)
  {
    if (buf[i] == '\n') starts[x++] = i + 1;
  }
  starts[numLines] = numChars;
#endif

  // look for the line with the problem size
  char* s;
  int lineNum = 0;
  while (1)
  {
#ifdef __MTA__
    s = buf + starts[lineNum];
#else
    fgets(buf, 80, file_r);
    s = buf;
#endif
    if (s[0] == 'p') break;
    lineNum++;
  }

  int retVal;
  // line looks like p sp <vertices> <edges>
  if (2 != (retVal = sscanf(s, "%*c %*2s %d %d", &numnodes, &numedges))) input_error("number of vertices and/or edges not supplied", lineNum);

  g.order = numnodes;
  fn = (char*) malloc((strlen(argv[1]) + 11) * sizeof(char));
  strcpy(fn, argv[1]);
  strcat(fn, ".imgs.dat");
  /* open file_w */
  if ((file_w = fopen(fn, "w")) == NULL)
  {
    fprintf(stderr, "Cannot open file %s\n", fn);
    exit(1);
  }
  ;
  fprintf(file_w, "numnodes: %d\n", numnodes);
  //numedges /= 2;
  fprintf(file_w, "numedges: %d\n", numedges);
  //printf ("%i vertices %i edges\n",numnodes,numedges);

  off_t size = numnodes * sizeof(struct Node);

  /* making maps */
  strcpy(fn, argv[1]);
  strcat(fn, ".nodes.img");
  r = qalloc_makestatmap(size + 100, NULL, fn, size, 1);
  if (r == NULL)
  {
    fprintf(stderr, "makestatmap nodes returned NULL!\n");
    exit(1);
  }
  fprintf(file_w, "makestatmap(nodes): %s\n", fn);
  size = numedges * sizeof(struct Edge);
  strcpy(fn, argv[1]);
  strcat(fn, ".edges.img");
  r2 = qalloc_makestatmap(size + 100, NULL, fn, size, 1);
  if (r2 == NULL)
  {
    fprintf(stderr, "makestatmap edges returned NULL!\n");
    exit(1);
  }
  fprintf(file_w, "makestatmap(edges): %s\n", fn);
  size = 2 * numedges * sizeof(int);
  strcpy(fn, argv[1]);
  strcat(fn, ".adjs.img");
  r3 = qalloc_makestatmap(size + 100, NULL, fn, size, 1);
  if (r3 == NULL)
  {
    fprintf(stderr, "makestatmap adjs returned NULL!\n");
    exit(1);
  }
  fprintf(file_w, "makestatmap(adjs): %s\n", fn);

  g.vertices = qalloc_statmalloc(r);
  if (g.vertices == NULL)
  {
    fprintf(stderr, "node statmalloc returned NULL!\n");
    exit(1);
  }
  nodes = g.vertices;
  fprintf(file_w, "statmalloc(node): %p\n", nodes);
  g.edges = qalloc_statmalloc(r2);
  if (g.edges == NULL)
  {
    fprintf(stderr, "edge statmalloc returned NULL!\n");
    exit(1);
  }
  edges = g.edges;
  fprintf(file_w, "statmalloc(edge): %p\n", edges);
  g.adjacencies = qalloc_statmalloc(r3);
  if (g.adjacencies == NULL)
  {
    fprintf(stderr, "adjs statmalloc returned NULL!\n");
    exit(1);
  }
  adjs = g.adjacencies;
  fprintf(file_w, "statmalloc(adj): %p\n", adjs);

  for (i = 0; i < numnodes; ++i)
  {
    nodes[i].id = i;
    nodes[i].deg = 0;
  }

  for (i = 0; i < numedges; ++i)
  {
    edges[i].id = i;
  }

  int source = -1;
  int arcStart = -1;
  i = lineNum + 1;
#ifdef __MTA__
  while (i < numLines)
  {

    //for (j= starts[i]; j< starts[i+1]; j++)
    //printf("%c", buf[j]);
    s = buf + starts[i];
#else
  while (1)
  {
    fgets(buf, 80, file_r);
    //printf("%s", buf);
    s = buf;
#endif
    switch (s[0])
    {
    // skip empty lines
    case 'c':
    case '\n':
    case '\0':
      break;

    // problem description
    case 't':
      problemDesc = (char*) malloc(sizeof(char) * strlen(s));
      strncpy(problemDesc, s + 2, strlen(s) - 3);
      break;

    // source description
    case 'n':
      if (source != -1) input_error("multiple source vertices specified", i);
      if (1 != sscanf(s, "%*c %d", &source)) input_error("source vertex id not supplied", i);
      if (source < 1 || source > numnodes) input_error("invalid source vertex id", i);
      //printf("source = %i\n",source);
      break;

    //edge (arc) description
    case 'a':
      arcStart = i;
      break;
    }
    if (arcStart != -1) break;
    i++;
  }
#ifdef __MTA__
  int num = (numLines - arcStart);   // 2;
#else
  int num = numedges;
#endif
    #pragma mta assert parallel
  for (i = 0; i < num; i++)
  {
    int i2 = i;  //*2;
#ifdef __MTA__
    int buf_offset = starts[arcStart + i2];
    s = buf + buf_offset;
#else
    s = buf;
#endif
    int to, from, weight;
    if (3 != sscanf(s, "%*c %d %d %d", &from, &to, &weight)) input_error("too few parameters when describing arc", arcStart + i2);
    --from;
    --to;
    if (from < 0 || from >= numnodes) input_error("first vertex id is invalid", arcStart + i2);
    if (to < 0 || to >= numnodes) input_error("second vertex id is invalid", arcStart + i2);
    if (weight < 0) input_error("negative weight supplied", arcStart + i2);

    //printf("edge (%i,%i), %i\n",from,to,weight);
    ++nodes[from].deg;
    if (from != to) ++nodes[to].deg;
    edges[i].vert1 = &nodes[from];
    edges[i].vert2 = &nodes[to];
#ifndef __MTA__
    fgets(buf, 80, file_r);
    //fgets(buf, 80, file_r);
#endif
  }

  int offset = 0;

  for (i = 0; i < numnodes; ++i)
  {
    nodes[i].offset = offset;
    offset += nodes[i].deg;
  }

  int* o2 = (int*) malloc(numnodes * sizeof(int));

  for (i = 0; i < numnodes; ++i) o2[i] = 0;

  for (i = 0; i < numedges; ++i)
  {
    int index = edges[i].vert1->id;
    adjs[nodes[index].offset + o2[index]++] = i;
    if (index != edges[i].vert2->id)
    {
      index = edges[i].vert2->id;
      adjs[nodes[index].offset + o2[index]++] = i;
    }
  }

  qalloc_checkpoint();
  /*printf("V = {\n");

  for (i = 0; i < numnodes - 10; i += 10) {
    for (j = 0; j < 9; ++j)
      printf("%6d, ", nodes[i + j].id);

    printf("%6d,\n", nodes[i + 10].id);
  }

  for (j = i; j < numnodes - 1; ++j)
    printf("%6d, ", nodes[j].id);

  printf("%6d}\n", nodes[numnodes - 1].id);
  printf("E = {");

  for (i = 0; i < numnodes; ++i)
    for (j = 0; j < nodes[i].deg; ++j)
      if (nodes[i].id == edges[adjs[nodes[i].offset + j]].vert1->id)
        printf("\n(%6d, %6d),",
           nodes[i].id,
           edges[adjs[nodes[i].offset + j]].vert2->id);

           printf("\b}\n");*/

  free(o2);

#ifdef __MTA__
  free(starts);
#endif
  free(buf);
  return g;
}

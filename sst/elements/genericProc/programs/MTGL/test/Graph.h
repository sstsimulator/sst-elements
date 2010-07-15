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
/*! \file Graph.h

    \brief C header for buildGraph and loadGraph in buildmap.c and loadmap.c to
           create and load node and edge memory map files from DIMACS graph
           file.  Defines struct Graph.

    \author Vitus Leung (vjleung@sandia.gov)

    \date 1/5/2007
*/
/****************************************************************************/

#ifndef MTGL_GRAPH_H
#define MTGL_GRAPH_H

#ifdef __cplusplus
extern "C" {
#endif

struct Edge;

struct Node {
  int id;
  short type;
  int deg;
  int offset;
};

struct Edge {
  struct Node* vert1;
  struct Node* vert2;
  int id;
  short type1;
  short type2;
};

struct Graph {
  struct Node* vertices;
  struct Edge* edges;
  int order;
  int size;
  int* adjacencies;
};

#ifdef __cplusplus
}
#endif

#endif

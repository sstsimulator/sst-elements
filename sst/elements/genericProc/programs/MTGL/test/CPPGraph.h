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
/*! \file CPPGraph.h

    \brief C++ header for using buildGraph and loadGraph in buildmap.c and
           loadmap.c to create and load node and edge memory map files from
           DIMACS graph file.  Defines class C_Graph.

    \author Vitus Leung (vjleung@sandia.gov)

    \date 1/5/2007
*/
/****************************************************************************/

#ifndef MTGL_CPPGRAPH_H
#define MTGL_CPPGRAPH_H

class C_Edge;

class C_Node {
public:
  static C_Edge* edges;
  static int* adjacencies;
  int id;
  short type;
  int deg;
  int offset;

  int degree() { return deg; }
};

class C_Edge {
public:
  C_Node* vert1;
  C_Node* vert2;
  int id;
  short type1;
  short type2;
};

class C_Graph {
public:
  C_Node* vertices;
  C_Edge* edges;
  int order;
  int size;
  int* adjacencies;
};

#endif

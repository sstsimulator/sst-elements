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
/*! \file debug_utils.hpp

    \author Jon Berry (jberry@sandia.gov)

    \date 12/4/2007
*/
/****************************************************************************/

#ifndef MTGL_DEBUG_UTILS_HPP
#define MTGL_DEBUG_UTILS_HPP

#include <mtgl/util.hpp>
#include <mtgl/mtgl_adapter.hpp>

//
// Handy utility file containing debugging routines and other helpful
// widgets.
//

namespace mtgl {

// Convenient function for printing out arrays of integers.
void print_array(int* A, int sz, char* lbl = NULL)
{
  int j = 0;
  assert(A != NULL);

  if (lbl) printf("%10s %4d [ ", lbl, sz);
  else printf("%10s %4d [ ", "", sz);

  for (int i = 0; i < sz; i++)
  {
    printf("%7d ", A[i]);
    j++;
    if (j == 10 && i < sz) { printf("\n(%5d) %10s ", i + 1, ""); j = 0; }
  }

  printf("]\n");
}

// Convenient function for printing out arrays of integers.
void print_array(bool* A, int sz, char* lbl = NULL)
{
  assert(A != NULL);

  if (lbl) printf("%15s [ ", lbl);
  else printf("%15s [ ", "");

  for (int i = 0; i < sz; i++)
  {
    printf("%1d ", A[i]);
  }

  printf("]\n");
}

// Convenient function for printing out arrays of integers.
void print_array_fltr(int* A, int sz, char* lbl = NULL, int fltr = -1)
{
  assert(A != NULL);

  if (lbl) printf("%15s [ ", lbl);
  else printf("%15s [ ", "");

  for (int i = 0; i < sz; i++)
  {
    if (A[i] == fltr) printf(" - ");
    else
      printf("%3d ", A[i]);
  }

  printf("]\n");
}

// Convenient function for printing out arrays of integers.
void print_array_fltr(bool* A, int sz, char* lbl = NULL, bool fltr = false)
{
  assert(A != NULL);

  if (lbl) printf("%15s [ ", lbl);
  else printf("%15s [ ", "");

  for (int i = 0; i < sz; i++)
  {
    if (A[i] == fltr) printf(" - ");
    else printf("%2d ", A[i]);
  }

  printf("]\n");
}

void print_array(double* A, int sz, char* lbl = NULL)
{
  assert(A != NULL);
  int j = 0;

  if (lbl) printf("%10s %4d [ ", lbl, sz);
  else printf("%10s %4d [ ", "", sz);

  for (int i = 0; i < sz; i++)
  {
    printf("%7.3f ", A[i]);
    j++;

    if (j == 10 && i < sz) { printf("\n(%5d) %11s ", i + 1, ""); j = 0; }
  }

  printf("]\n");
}

template <typename graph_adapter>
void print_graph(graph_adapter& ga)
{
  typedef typename graph_traits<graph_adapter>::vertex_descriptor
          vertex_descriptor;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_descriptor;
  typedef typename graph_traits<graph_adapter>::size_type size_type;
  typedef typename graph_traits<graph_adapter>::vertex_iterator vertex_iterator;
  typedef typename graph_traits<graph_adapter>::out_edge_iterator
          out_edge_iterator;

  vertex_id_map<graph_adapter> vipm = get(_vertex_id_map, ga);

  vertex_iterator verts = vertices(ga);
  size_type order = num_vertices(ga);
  for (size_type i = 0; i < order; ++i)
  {
    vertex_descriptor v = verts[i];
    int vid = get(vipm, v);

    printf("%-3d  { ", vid);

    out_edge_iterator inc_edgs = out_edges(v, ga);
    size_type out_deg = out_degree(v, ga);
    for (size_type j = 0; j < out_deg; ++j)
    {
      edge_descriptor e = inc_edgs[j];

      vertex_descriptor src = source(e, ga);
      vertex_descriptor trg = target(e, ga);

      int sid = get(vipm, src);
      int tid = get(vipm, trg);

      if (sid == vid) printf("%d ", tid);
    }

    printf("}\n");
  }
}

template <typename graph_adapter>
void print_graph_viz(FILE* stream, graph_adapter& ga)
{
  typedef typename graph_traits<graph_adapter>::size_type size_type;
  typedef typename graph_traits<graph_adapter>::vertex_descriptor
          vertex_descriptor;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_descriptor;
  typedef typename graph_traits<graph_adapter>::vertex_iterator vertex_iterator;
  typedef typename graph_traits<graph_adapter>::out_edge_iterator
          out_edge_iterator;

  vertex_id_map<graph_adapter> vipm = get(_vertex_id_map, ga);
  fprintf(stream, "digraph \"G\" {\n");
  fprintf(stream, "graph[overlap=false splines=true size=\"7.5,7.5\"]\n");
  //fprintf(stream,"size=\"7.5,7.5\";\n");

  vertex_iterator verts = vertices(ga);
  size_type order = num_vertices(ga);
  for (size_type i = 0; i < order; ++i)
  {
    vertex_descriptor v = verts[i];
    int vid = get(vipm, v);

    out_edge_iterator inc_edgs = out_edges(v, ga);
    size_type out_deg = out_degree(v, ga);
    for (size_type j = 0; j < out_deg; ++j)
    {
      edge_descriptor e = inc_edgs[j];

      vertex_descriptor src = source(e, ga);
      vertex_descriptor trg = target(e, ga);

      int sid = get(vipm, src);
      int tid = get(vipm, trg);

      if (sid == vid) fprintf(stream, "%d -> %d;\n", sid, tid);
    }
  }

  fprintf(stream, "};\n");
}

}

#endif

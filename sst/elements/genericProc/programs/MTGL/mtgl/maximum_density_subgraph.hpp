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
/*! \file maximum_density_subgraph.hpp

    \brief Finds the vertices representing the maximum density subgraph.

    \author Greg Mackey (gemacke@sandia.gov)

    \date 5/20/2009

    This is an implementation of the maximum density subgraph algorithm as
    described in Andrew Goldberg's paper "Finding a Maximum Density Subgraph."
    The answer is returned in v1 as a bitmap.  The maximum density subgraph
    is the subgraph induced by the vertices returned in v1.  v1 is expected
    to be of size of the number of verices in g1.  It doesn't need to be
    initialized.
*/
/****************************************************************************/

#ifndef MTGL_MAXIMUM_DENSITY_SUBGRAPH_HPP
#define MTGL_MAXIMUM_DENSITY_SUBGRAPH_HPP

#include <mtgl/disjoint_paths_max_flow.hpp>
#include <mtgl/static_graph_adapter.hpp>

namespace mtgl {

template<typename graph_adapter>
void
maximum_density_subgraph(graph_adapter& g1, int* v1)
{
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_descriptor;
  typedef typename graph_traits<graph_adapter>::edge_iterator edge_iterator;
  typedef typename graph_traits<graph_adapter>::vertex_iterator vertex_iterator;
  typedef typename graph_traits<graph_adapter>::size_type size_type;

  typedef static_graph_adapter<bidirectionalS> search_adapter;
  typedef typename graph_traits<search_adapter>::vertex_descriptor
          vertex_descriptor_srch;
  typedef typename graph_traits<search_adapter>::size_type size_type_srch;

  vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, g1);

  size_type g1_order = num_vertices(g1);
  size_type g1_size = num_edges(g1);
  size_type_srch g2_order = g1_order + 2;
  size_type_srch g2_size = 2 * g1_size + 2 * g1_order;

  // Initialize v1 to be empty.
  memset(v1, 0, g1_order * sizeof(int));

  // Note that g1 is expected to be undirected.
  //
  // Define the augmented directed graph g2 as follows:
  //   1.  Add v_i, all the vertices from g1.
  //   2.  Add e_ij and e_ji, the edges from g1 and their reverses.
  //   3.  Add s, a source vertex.
  //   4.  Add t, a target vertex.
  //   5.  Add an edge connecting s to every v_i.
  //   6.  Add an edge connecting every v_i to t.

  // Create g2 by creating the source and destination vectors of ids
  // representing its edges.
  size_type_srch* srcs =
    (size_type_srch*) malloc(g2_size * sizeof(size_type_srch));
  size_type_srch* dests =
    (size_type_srch*) malloc(g2_size * sizeof(size_type_srch));

  edge_iterator edgs = edges(g1);
  vertex_iterator verts = vertices(g1);

  // Add the edges from g1.  This defines the ids of the vertices from g1 as
  // 0 to g1_order - 1.
  #pragma mta assert nodep
  for (size_type i = 0; i < g1_size; ++i)
  {
    edge_descriptor e = edgs[i];

    srcs[i] = get(vid_map, source(e, g1));
    dests[i] = get(vid_map, target(e, g1));
  }

  // Add the reverse edges from g1.
  #pragma mta assert nodep
  for (size_type i = 0; i < g1_size; ++i)
  {
    edge_descriptor e = edgs[i];

    srcs[i + g1_size] = get(vid_map, target(e, g1));
    dests[i + g1_size] = get(vid_map, source(e, g1));
  }

  // Add the edges connecting s (with id g1_order) to every v_i.
  #pragma mta assert nodep
  for (size_type i = 0; i < g1_order; ++i)
  {
    srcs[i + 2 * g1_size] = g1_order;
    dests[i + 2 * g1_size] = i;
  }

  // Add the edges connecting every v_i to t (with id g1_order + 1).
  #pragma mta assert nodep
  for (size_type i = 0; i < g1_order; ++i)
  {
    srcs[i + 2 * g1_size + g1_order] = i;
    dests[i + 2 * g1_size + g1_order] = g1_order + 1;
  }

  search_adapter g2;
  init(g2_order, g2_size, srcs, dests, g2);

  free(srcs);
  free(dests);

  // Some definitions:
  //   1. d_i is the degree of v_i.
  //   2. m is the number of edges in g1.
  //   3. g is the probe value.
  //
  // The capacities of the edges in g2 are defined as follows:
  //   1. Edges in g1
  //        - 1
  //   2. Edges connecting s to every v_i
  //        - m
  //   3. Edges connecting every v_i to t
  //        - m + 2g - d_i
  //
  // The capacities have to be doubles because they include g, which is a
  // rational number.  We are using an integer maxflow code that has been
  // modified by allowing the capacities to be doubles.  This was basically
  // just replacing the data types, so there could still be problems with
  // accurracy and potentially long running times.

  double* capacities = (double*) malloc(g2_size * sizeof(double));

  // Set the capacities of the edges from g1 and their reverses to 1.  These
  // capacities remain constant throughout the algorithm.
  #pragma mta assert nodep
  for (size_type i = 0; i < 2 * g1_size; ++i) capacities[i] = 1;

  // Set the capacities of the s to v_i edges to m.  These capacities also
  // remain constant throughout the algorithm.
  #pragma mta assert nodep
  for (size_type i = 0; i < g1_order; ++i)
  {
    capacities[i + 2 * g1_size] = g1_size;
  }

  // The 2g part of the capacities of the v_i to t edges changes every
  // iteration of the loop as they depend on g, so the capacities of those
  // edges will be set below in the loop.  For now, calculate the constant
  // part of these edges' capacities, m - d_i.
  size_type* constant_capacity =
    (size_type*) malloc(g1_order * sizeof(size_type));
  for (size_type i = 0; i < g1_order; ++i)
  {
    constant_capacity[i] = g1_size - out_degree(verts[i], g1);
  }

  vertex_descriptor_srch source = get_vertex(g1_order, g2);
  vertex_descriptor_srch target = get_vertex(g1_order + 1, g2);

  double lower = 0;
  double upper = g1_size;
  double smallest_interval = 1.0 / (g1_order * (g1_order - 1));

  int* division = (int*) malloc(g2_order * sizeof(int));

  while ((upper - lower) >= smallest_interval)
  {
    double probe = (upper + lower) / 2;

    // Set the capacities of the v_i to t edges.  The constant part stored in
    // constant_capacity, m - d_i, + the variable part, 2g.
    #pragma mta assert nodep
    for (size_type i = 0; i < g1_order; ++i)
    {
      capacities[i + 2 * g1_size + g1_order] = constant_capacity[i] + 2 * probe;
    }

    disjoint_paths_max_flow<search_adapter, double>
      dpmf(g2, source, target, capacities);
    dpmf.minimum_cut(division);

    // Count how many vertices are in S.
    int sum = 0;
    for (size_type_srch i = 0; i < g2_order; ++i) sum += division[i];

    if (sum == 1)
    {
      // S = { source }.
      upper = probe;
    }
    else
    {
      lower = probe;

      // Copy S - { source } to V1.
      #pragma mta assert nodep
      for (size_type i = 0; i < g1_order; ++i) v1[i] = division[i];
    }
  }

  free(division);
  free(constant_capacity);
  free(capacities);
}

}

#endif

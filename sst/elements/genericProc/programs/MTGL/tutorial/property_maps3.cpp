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

#include <mtgl/static_graph_adapter.hpp>

using namespace mtgl;

int main(int argc, char* argv[])
{
  typedef static_graph_adapter<directedS> Graph;
  typedef graph_traits<Graph>::size_type size_type;
  typedef graph_traits<Graph>::vertex_descriptor vertex_descriptor;
  typedef graph_traits<Graph>::edge_descriptor edge_descriptor;
  typedef graph_traits<Graph>::vertex_iterator vertex_iterator;
  typedef graph_traits<Graph>::edge_iterator edge_iterator;
  typedef graph_traits<Graph>::out_edge_iterator out_edge_iterator;

  const size_type numVerts = 6;
  const size_type numEdges = 8;

  size_type sources[numEdges] = { 0, 0, 1, 1, 2, 3, 3, 4 };
  size_type targets[numEdges] = { 1, 2, 2, 3, 4, 4, 5, 5 };

  // Initialize the graph.
  Graph g;
  init(numVerts, numEdges, sources, targets, g);

  size_type order = num_vertices(g);
  size_type size = num_edges(g);

  edge_id_map<Graph> eid_map = get(_edge_id_map, g);

  size_type* lowest_degree_array = new size_type[size];
  memset(lowest_degree_array, 0, sizeof(size_type) * size);

  array_property_map<size_type, edge_id_map<Graph> >
  lowest_degree_apm(lowest_degree_array, eid_map);

  // For each edge store the lower degree of its endpoints.
  // Loop using double loop with out_edge_iterator.
  vertex_iterator vIter = vertices(g);
  #pragma mta assert nodep
  for (size_type i = 0; i < order; ++i)
  {
    vertex_descriptor u = vIter[i];

    size_type u_deg = out_degree(u, g);
    out_edge_iterator oeIter = out_edges(u, g);
    #pragma mta assert nodep
    for (size_type j = 0; j < u_deg; ++j)
    {
      edge_descriptor e = oeIter[j];

      vertex_descriptor v = target(e, g);
      size_type v_deg = out_degree(v, g);

      size_type val = u_deg < v_deg ? u_deg : v_deg;
      lowest_degree_apm[e] = val;
    }
  }

  // Print the lower degree of each edge's endpoints.
  printf("Using out edges:\n");
  edge_iterator eIter = edges(g);
  for (size_type i = 0; i < size; ++i)
  {
    edge_descriptor e = eIter[i];
    size_type eid = get(eid_map, e);

    printf("%lu: %lu\n", eid, lowest_degree_apm[e]);
  }
  printf("\n");

  // Reset the lowest degree array to 0.
  memset(lowest_degree_array, 0, sizeof(size_type) * size);

  // For each edge store the lower degree of its endpoints.
  // Loop using double loop with out_edge_iterator.
  #pragma mta assert nodep
  for (size_type i = 0; i < size; ++i)
  {
    edge_descriptor e = eIter[i];
    vertex_descriptor u = source(e, g);
    vertex_descriptor v = target(e, g);

    size_type u_deg = out_degree(u, g);
    size_type v_deg = out_degree(v, g);

    size_type val = u_deg < v_deg ? u_deg : v_deg;
    lowest_degree_apm[e] = val;
  }

  // Print the lower degree of each edge's endpoints.
  printf("Using edges:\n");
  for (size_type i = 0; i < size; ++i)
  {
    edge_descriptor e = eIter[i];
    size_type eid = get(eid_map, e);

    printf("%lu: %lu\n", eid, lowest_degree_apm[e]);
  }

  delete [] lowest_degree_array;

  return 0;
}

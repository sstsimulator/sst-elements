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
  typedef graph_traits<Graph>::vertex_iterator vertex_iterator;

  const size_type numVerts = 6;
  const size_type numEdges = 8;

  size_type sources[numEdges] = { 0, 0, 1, 1, 2, 3, 3, 4 };
  size_type targets[numEdges] = { 1, 2, 2, 3, 4, 4, 5, 5 };

  // Initialize the graph.
  Graph g;
  init(numVerts, numEdges, sources, targets, g);

  vertex_id_map<Graph> vid_map = get(_vertex_id_map, g);

  bool div_by_3[numVerts];

  array_property_map<bool, vertex_id_map<Graph> >
  divisible_by_3(div_by_3, vid_map);

  // Find the vertices whose id is divisible by 3.
  vertex_iterator verts = vertices(g);
  size_type order = num_vertices(g);
  for (size_type i = 0; i < order; ++i)
  {
    vertex_descriptor v = verts[i];
    size_type vid = get(vid_map, v);
    divisible_by_3[v] = vid % 3 == 0;
  }

  // Print the vertices whose id is divisible by 3.
  for (size_type i = 0; i < order; ++i)
  {
    vertex_descriptor v = verts[i];
    size_type vid = get(vid_map, v);

    if (divisible_by_3[v]) printf("%lu  ", vid);
  }
  printf("\n");

  return 0;
}

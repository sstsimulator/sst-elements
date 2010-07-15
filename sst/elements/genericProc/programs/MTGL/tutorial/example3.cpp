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

template <typename Graph>
void print_my_graph(Graph& g)
{
  typedef typename graph_traits<Graph>::size_type size_type;
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;
  typedef typename graph_traits<Graph>::vertex_iterator vertex_iterator;
  typedef typename graph_traits<Graph>::adjacency_iterator adjacency_iterator;

  vertex_id_map<Graph> vid_map = get(_vertex_id_map, g);

  vertex_iterator verts = vertices(g);
  size_type order = num_vertices(g);
  for (size_type i = 0; i < order; i++)
  {
    vertex_descriptor u = verts[i];
    size_type uid = get(vid_map, u);
    adjacency_iterator adjs = adjacent_vertices(u, g);
    size_type end = out_degree(u, g);
    for (size_type j = 0; j < end; j++)
    {
      vertex_descriptor v = adjs[j];
      size_type vid = get(vid_map, v);
      printf("(%lu, %lu)\n", uid, vid);
    }
  }
}

template <typename Graph>
typename graph_traits<Graph>::size_type compute_id_sum(Graph& g)
{
  typedef typename graph_traits<Graph>::size_type size_type;
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;
  typedef typename graph_traits<Graph>::vertex_iterator vertex_iterator;
  typedef typename graph_traits<Graph>::adjacency_iterator adjacency_iterator;

  size_type id_sum = 0;

  vertex_id_map<Graph> vid_map = get(_vertex_id_map, g);

  vertex_iterator verts = vertices(g);
  size_type order = num_vertices(g);
  for (size_type i = 0; i < order; i++)
  {
    vertex_descriptor u = verts[i];
    adjacency_iterator adjs = adjacent_vertices(u, g);
    size_type end = out_degree(u, g);
    for (size_type j = 0; j < end; j++)
    {
      vertex_descriptor v = adjs[j];
      size_type vid = get(vid_map, v);
      id_sum += vid;
    }
  }

  return id_sum;
}

int main(int argc, char* argv[])
{
  typedef static_graph_adapter<directedS> Graph;
  typedef graph_traits<Graph>::size_type size_type;

  size_type n;
  size_type m;

  // Read in the number of vertices and edges.
  std::cin >> n;
  std::cin >> m;

  size_type* srcs = new size_type[m];
  size_type* dests = new size_type[m];

  // Read in the ids of each edge's vertices.
  for (size_type i = 0; i < m; ++i)
  {
    std::cin >> srcs[i] >> dests[i];
  }

  // Initialize the graph.
  Graph g;
  init(n, m, srcs, dests, g);

  delete [] srcs;
  delete [] dests;

  // Print the graph.
  printf("Graph:\n");
  print_my_graph(g);
  printf("\n");

  // Find the sum of the ids of the adjacencies.
  size_type id_sum = compute_id_sum(g);

  printf("sum of endpoint id's: %lu\n", id_sum);

  return 0;
}

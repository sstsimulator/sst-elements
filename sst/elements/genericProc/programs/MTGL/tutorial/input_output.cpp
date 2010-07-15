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
#include <mtgl/mtgl_io.hpp>

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

  // Write the graph to disk.
  INTENTIONAL COMPILER ERROR: PLEASE CHANGE THESE FILENAMES TO YOUR OWN
  char sfname[] = "/scratch1/graph.srcs";
  char dfname[] = "/scratch1/graph.dests";

  write_binary(sfname, dfname, g);

  // Restore the graph from disk to a different graph.
  Graph dg;
  read_binary(sfname, dfname, dg);

  // Print the graphs.
  printf("Graph (g):\n");
  print_my_graph(g);
  printf("\n");

  printf("Graph (dg):\n");
  print_my_graph(dg);
  printf("\n");

  return 0;
}

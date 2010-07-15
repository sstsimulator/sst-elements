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
  typedef typename graph_traits<Graph>::edge_descriptor edge_descriptor;
  typedef typename graph_traits<Graph>::edge_iterator edge_iterator;

  vertex_id_map<Graph> vid_map = get(_vertex_id_map, g);
  edge_id_map<Graph> eid_map = get(_edge_id_map, g);

  edge_iterator edgs = edges(g);
  size_type size = num_edges(g);
  for (size_type i = 0; i < size; i++)
  {
    edge_descriptor e = edgs[i];
    size_type eid = get(eid_map, e);

    vertex_descriptor u = source(e, g);
    size_type uid = get(vid_map, u);

    vertex_descriptor v = target(e, g);
    size_type vid = get(vid_map, v);

    printf("%lu: (%lu, %lu)\n", eid, uid, vid);
  }
}

template <typename Graph>
void
find_independent_set(Graph& g, bool* active_verts)
{
  // Loop over all vertices.  In the inner loop, loop over each vertex's
  // adjacencies.  This lets us access all the edges in the graph.  In the
  // inner loop, play the game.  The game will decide a winning vertex and
  // a losing vertex for each edge.  Ignore self loops.  Initialize
  // active_verts to be all true.  When a vertex loses, set its active_verts
  // value to false.  After all edges have been visited, vertices with
  // active_verts values of true are the vertices in the independent set.

  // This is the game we will play.  The vertex with the lowest degree wins.
  // If there is a tie, the vertex with the lowest id wins.
}

int main(int argc, char* argv[])
{
  typedef static_graph_adapter<undirectedS> Graph;
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

  size_type num_verts = num_vertices(g);

  bool* active_verts = new bool[num_verts];

  // Find the independent set.
  find_independent_set(g, active_verts);

  // Print the independent set.
  printf("Independent set:\n");
  for (size_type i = 0; i < num_verts; ++i)
  {
    if (active_verts[i]) printf("%lu\n", i);
  }

  delete [] active_verts;

  return 0;
}

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

  typedef typename graph_traits<Graph>::size_type size_type;
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;
  typedef typename graph_traits<Graph>::vertex_iterator vertex_iterator;
  typedef typename graph_traits<Graph>::adjacency_iterator adjacency_iterator;

  size_type order = num_vertices(g);

  // Initialize all active_verts entries to be true.
  for (size_type i = 0; i < order; ++i) active_verts[i] = true;

  vertex_id_map<Graph> vid_map = get(_vertex_id_map, g);

  vertex_iterator verts = vertices(g);
  #pragma mta assert noalias *active_verts
  #pragma mta assert nodep
  for (size_type i = 0; i < order; ++i)
  {
    vertex_descriptor u = verts[i];
    size_type uid = get(vid_map, u);

    adjacency_iterator adjs = adjacent_vertices(u, g);
    size_type u_out_deg = out_degree(u, g);
    #pragma mta assert nodep
    for (size_type j = 0; j < u_out_deg; ++j)
    {
      vertex_descriptor v = adjs[j];
      size_type vid = get(vid_map, v);
      size_type v_out_deg = out_degree(v, g);

      // Only check edges that have both endpoints still active.  Ignore
      // self loops.
      if (active_verts[uid] && active_verts[vid] && uid != vid)
      {
        // The vertex with the lowest degree wins.  If there is a tie, the
        // vertex with the lowest id wins.
        size_type loser_id;

        if (u_out_deg < v_out_deg)
        {
          loser_id = vid;
        }
        else if (v_out_deg < u_out_deg)
        {
          loser_id = uid;
        }
        else if (uid < vid)
        {
          loser_id = vid;
        }
        else
        {
          loser_id = uid;
        }

        active_verts[loser_id] = false;
      }
    }
  }
}

int main(int argc, char* argv[])
{
  typedef static_graph_adapter<undirectedS> Graph;
  typedef graph_traits<Graph>::size_type size_type;

  if (argc < 2)
  {
    fprintf(stderr, "Usage: %s <srcs/dests prefix>\n", argv[0]);
    exit(1);
  }

  std::string srcs_file(argv[1]);
  std::string dests_file(argv[1]);

  srcs_file += ".srcs";
  dests_file += ".dests";

  // Initialize the graph.
  Graph g;
  read_binary(srcs_file.c_str(), dests_file.c_str(), g);

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

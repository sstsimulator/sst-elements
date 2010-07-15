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

#include <iostream>

#include <mtgl/static_graph_adapter.hpp>
#include <mtgl/adjacency_list_adapter.hpp>
#include <mtgl/triangles.hpp>

using namespace std;
using namespace mtgl;

template <class graph>
class triangle_printing_visitor {
public:
  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::vertex_descriptor vertex_descriptor;
  typedef typename graph_traits<graph>::edge_descriptor edge_descriptor;
  typedef typename graph_traits<graph>::edge_iterator edge_iterator;

  triangle_printing_visitor(graph& gg) : g(gg), edgs(edges(g)),
                                         vid_map(get(_vertex_id_map, g)) {}

  void operator()(size_type eid1, size_type eid2, size_type eid3)
  {
    edge_descriptor e1 = edgs[eid1];
    edge_descriptor e2 = edgs[eid2];
    edge_descriptor e3 = edgs[eid3];

    printf("triangle eids: {%lu,%lu,%lu}\n", eid1, eid2, eid3);

    vertex_descriptor e1src = source(e1, g);
    vertex_descriptor e1trg = target(e1, g);
    vertex_descriptor e2src = source(e2, g);
    vertex_descriptor e2trg = target(e2, g);
    vertex_descriptor e3src = source(e3, g);
    vertex_descriptor e3trg = target(e3, g);

    size_type e1srcid = get(vid_map, e1src);
    size_type e1trgid = get(vid_map, e1trg);
    size_type e2srcid = get(vid_map, e2src);
    size_type e2trgid = get(vid_map, e2trg);
    size_type e3srcid = get(vid_map, e3src);
    size_type e3trgid = get(vid_map, e3trg);

    printf("triangle: {(%lu,%lu), (%lu,%lu), (%lu,%lu)}\n",
           e1srcid, e1trgid, e2srcid, e2trgid, e3srcid, e3trgid);
  }

private:
  graph& g;
  edge_iterator edgs;
  vertex_id_map<graph> vid_map;
};

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
void print_all_triangles(Graph& g)
{
  typedef typename graph_traits<Graph>::size_type size_type;
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;
  typedef typename graph_traits<Graph>::vertex_iterator vertex_iterator;
  typedef typename graph_traits<Graph>::adjacency_iterator adjacency_iterator;

  size_type id_sum = 0;

  vertex_id_map<Graph> vid_map = get(_vertex_id_map, g);

  triangle_printing_visitor<Graph> tpv(g);
  find_triangles<Graph, triangle_printing_visitor<Graph> > ft(g, tpv);
  ft.run();
}

int main(int argc, char* argv[])
{
  typedef static_graph_adapter<directedS> Graph;
  typedef adjacency_list_adapter<directedS> DynamicGraph;
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

  // Initialize the graphs.
  Graph g;
  init(n, m, srcs, dests, g);

  DynamicGraph dg;
  init(n, m, srcs, dests, dg);

  delete [] srcs;
  delete [] dests;

  // Print the graphs.
  printf("Graph (g):\n");
  print_my_graph(g);
  printf("\n");

  printf("Graph (dg):\n");
  print_my_graph(dg);
  printf("\n");

  // Find the sums of the ids of the adjacencies.
  print_all_triangles(g);
  print_all_triangles(dg);

  return 0;
}

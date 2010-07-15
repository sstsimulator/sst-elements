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
/*! \file mtgl_boost_test.cpp

    \author Jon Berry (jberry@sandia.gov)

    \date 12/4/2007
*/
/****************************************************************************/

#include <iostream>

#include <mtgl/mtgl_boost_property.hpp>
#include <mtgl/graph_adapter.hpp>
#include <mtgl/psearch.hpp>

using namespace std;
using namespace mtgl;

class print_edges_vis {
public:
  typedef graph_traits<graph_adapter>::edge_descriptor edge_t;
  typedef graph_traits<graph_adapter>::vertex_descriptor vertex_t;

  print_edges_vis(graph_adapter& g) : ga(g)
  {
    vid_map = get(_vertex_id_map, ga);
  }

  void operator()(edge_t e)
  {
    vertex_t src = source(e, ga);
    vertex_t trg = target(e, ga);
    int sid = get(vid_map, src);
    int tid = get(vid_map, trg);
    printf("%d\t%d\n", sid, tid);
  }

private:
  graph_adapter& ga;
  vertex_id_map<graph_adapter> vid_map;
};

/*
void print_adj(graph_adapter& ga)
{
  typedef graph_traits<graph_adapter>::vertex_descriptor vertex_descriptor;
  typedef graph_traits<graph_adapter>::vertex_iterator vertex_iterator;
  typedef graph_traits<graph_adapter>::adjacency_iterator adjacency_iterator;

  vertex_id_map<graph_adapter> vipm = get(_vertex_id_map, ga);

  vertex_iterator verts = vertices(ga);
  size_type order = num_vertices(ga);
  for (size_type i = 0; i < order; ++i) {
  {
    int vid = get(vipm, *verts);

    std::cout << vid << ": { ";

    vertex_descriptor v = verts[i];
    adjacency_iterator adj_verts = adjacent_vertices(v, ga);
    size_type out_deg = out_degree(v, ga);
    for (size_type j = 0; j < out_deg; ++j)
    {
      vertex_descriptor trg = adj_verts[j];
      int tid = get(vipm, trg);

      std::cout << "(" << vid << ", " << tid << ") ";
    }

    std::cout << "}" << std::endl;
  }
}
*/

class search_printer : public default_psearch_visitor<graph_adapter> {
public:
  typedef graph_traits<graph_adapter>::vertex_descriptor vertex_descriptor;
  typedef graph_traits<graph_adapter>::edge_descriptor edge_descriptor;

  search_printer(graph_adapter& g) : vid_map(get(_vertex_id_map, g)) {}

  void psearch_root(vertex_descriptor& v)
  {
    std::cout << "psearch_root(" << get(vid_map, v) << ")" << std::endl;
  }

  void pre_visit(vertex_descriptor& v)
  {
    std::cout << "pre_visit(" << get(vid_map, v) << ")" << std::endl;
  }

  void tree_edge(edge_descriptor& e, vertex_descriptor& v)
  {
    vertex_descriptor v2 = other(e, v);
    std::cout << "tree_edge( (" << get(vid_map, v) << ", " << get(vid_map, v2)
              << "), " << get(vid_map, v) << ") " << std::endl;
  }

  void back_edge(edge_descriptor& e, vertex_descriptor& v)
  {
    vertex_descriptor v2 = other(e, v);
    std::cout << "back_edge( (" << get(vid_map, v) << ", " << get(vid_map, v2)
              << "), " << get(vid_map, v) << ") " << std::endl;
  }

private:
  vertex_id_map<graph_adapter> vid_map;
};

/*
class bfs_search_printer : public default_psearch_visitor<graph_adapter> {
public:
  typedef graph_traits<graph_adapter>::vertex_descriptor vertex_descriptor;
  typedef graph_traits<graph_adapter>::edge_descriptor edge_descriptor;

  search_printer(graph_adapter& g) : vid_map(get(_vertex_id_map, g)) {}

  void psearch_root(vertex_descriptor& v)
  {
    std::cout << "psearch_root(" << get(vid_map, v) << ")" << std::endl;
  }

  void pre_visit(vertex_descriptor& v)
  {
    std::cout << "pre_visit(" << get(vid_map, v) << ")" << std::endl;
  }

  void tree_edge(edge_descriptor& e, vertex_descriptor& v)
  {
    vertex_descriptor v2 = other(e, v);
    std::cout << "tree_edge( (" << get(vid_map, v) << ", " << get(vid_map, v2)
              << "), " << get(vid_map, v) << ") " << std::endl;
  }

  void back_edge(edge_descriptor& e, vertex_descriptor& v)
  {
    vertex_descriptor v2 = other(e, v);
    std::cout << "back_edge( (" << get(vid_map, v) << ", " << get(vid_map, v2)
              << "), " << get(vid_map, v) << ") " << std::endl;
  }

private:
  vertex_id_map<graph_adapter> vid_map;
};
*/

int main()
{
  typedef graph_traits<graph_adapter>::size_type size_type;

  size_type order = 3;
  size_type size = 3;

  size_type srcs[size] = { 0, 1, 2 };
  size_type dests[size] = { 1, 2, 0 };

  graph_adapter ga;

  init(order, size, srcs, dests, ga);

  print_edges_vis pev(ga);
  visit_edges(ga, pev);

  return 0;
}

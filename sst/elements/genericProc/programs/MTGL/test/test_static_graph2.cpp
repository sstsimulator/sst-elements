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
/*! \file test_static_graph1.cpp

    \author Jon Berry (jberry@sandia.gov)

    \date 1/27/2008
*/
/****************************************************************************/


#include <cstdlib>

#include <mtgl/static_graph_adapter.hpp>
#include <mtgl/adjacency_list_adapter.hpp>
#include <mtgl/generate_rmat_graph.hpp>
#include <mtgl/breadth_first_search_mtgl.hpp>

using namespace mtgl;

template <class Graph>
void compute_in_degree(Graph* g, int* in_degree)
{
  for (int i = 0; i < g->n; i++)
  {
    int begin = g->index[i];
    int end = g->index[i + 1];

    for (int j = begin; j < end; j++) in_degree[g->end_points[j]]++;
  }
}

template <class graph_adapter>
void compute_in_degree_via_edges(graph_adapter& g, int* in_degree)
{
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_t;
  typedef typename graph_traits<graph_adapter>::edge_iterator edge_iterator_t;
  typedef typename graph_traits<graph_adapter>::size_type size_type;

  vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, g);

  size_type m = num_edges(g);
  edge_iterator_t edgs = edges(g);

  #pragma mta assert parallel
  for (size_type i = 0; i < m; i++)
  {
    edge_t e = edgs[i];
    vertex_t trg = target(e, g);
    size_type tid = get(vid_map, trg);
    mt_incr(in_degree[tid], 1);
  }
}

template <class graph_adapter>
void compute_in_degree_via_adj(graph_adapter& g, int* in_degree)
{
  typedef typename graph_traits<graph_adapter>::vertex_descriptor
          vertex_descriptor;
  typedef typename graph_traits<graph_adapter>::adjacency_iterator
          adjacency_iterator;
  typedef typename graph_traits<graph_adapter>::vertex_iterator vertex_iterator;
  typedef typename graph_traits<graph_adapter>::size_type size_type;

  vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, g);

  size_type n = num_vertices(g);
  vertex_iterator verts = vertices(g);

  for (size_type i = 0; i < n; i++)
  {
    vertex_descriptor u = verts[i];
    adjacency_iterator adjs = adjacent_vertices(u, g);
    size_type deg = out_degree(u, g);
    size_type vid = get(vid_map, u);

    for (size_type j = 0; j < deg; j++)
    {
      vertex_descriptor neighbor = adjs[j];
      size_type nid = get(vid_map, neighbor);
      in_degree[nid]++;
    }
  }
}

int main(int argc, char* argv[])
{
  typedef static_graph_adapter<directedS> SGraph;    // Static graph.
  typedef adjacency_list_adapter<directedS> DGraph;  // Dynamic graph.

  srand48(0);
  SGraph sg;
  generate_rmat_graph(sg, atoi(argv[1]), 2);

  int order = num_vertices(sg);

  srand48(0);
  DGraph dg;
  generate_rmat_graph(dg, atoi(argv[1]), 2);

  int* indeg = new int[order];
  for (int i = 0; i < order; i++) indeg[i] = 0;

  mt_timer timer;
  timer.start();
  compute_in_degree(sg.get_graph(), indeg);
  timer.stop();
  printf("Pure C (static): %lf (check indeg[2]: %d)\n",
         timer.getElapsedSeconds(), indeg[2]);

  for (int i = 0; i < order; i++) indeg[i] = 0;
  timer.start();
  compute_in_degree_via_adj(sg, indeg);
  timer.stop();
  printf("MTGL adj iteration (static graph): %lf (check indeg[2]: %d)\n",
         timer.getElapsedSeconds(),
         indeg[2]);

  for (int i = 0; i < order; i++) indeg[i] = 0;
  timer.start();
  compute_in_degree_via_edges(sg, indeg);
  timer.stop();
  printf("MTGL edge iteration (static graph):%lf (check indeg[2]: %d)\n",
         timer.getElapsedSeconds(), indeg[2]);

  for (int i = 0; i < order; i++) indeg[i] = 0;
  timer.start();
  compute_in_degree_via_adj(dg, indeg);
  timer.stop();
  printf("MTGL adj iteration (dynamic graph):%lf (check indeg[2]: %d)\n",
         timer.getElapsedSeconds(), indeg[2]);

  for (int i = 0; i < order; i++) indeg[i] = 0;
  timer.start();
  compute_in_degree_via_edges(dg, indeg);
  timer.stop();
  printf("MTGL edge iteration (dynamic graph):%lf (check indeg[2]:%d)\n",
         timer.getElapsedSeconds(), indeg[2]);

  // This last example returns a different in_degree, since the
  // dynamic graph edge representation is different.  Specifically,
  // the edge list contains each undirected edge exactly once.
  // Undirected static_graph's iterate over (0,1) and (1,0), for
  // example, but graph's (dynamic graphs) just see (0,1).

  return 0;
}

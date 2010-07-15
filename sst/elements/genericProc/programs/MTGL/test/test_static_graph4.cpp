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
/*! \file test_static_graph4.cpp

    \brief This test program compares MTA performance of various traversals of
           a C compressed sparse row graph representation.

    \author Jon Berry (jberry@sandia.gov)

    \date 8/8/2008

    The representation stores out-edges, and our sample task is to compute
    the in-degree of each vertex by traversing the adjacency structure and
    therefore touching all of the graph.

    Unfortunately, this isn't a compelling teaching example since we could
    compute the information with traversing adjacency lists for this
    structure.  We'll proceed anyway, as touching the whole graph is involved
    in lots of ranking algorithms.
*/
/****************************************************************************/

#include <cstdlib>
#include <climits>

#include <mtgl/static_graph_adapter.hpp>
#include <mtgl/adjacency_list_adapter.hpp>
#include <mtgl/st_connectivity.hpp>
#include <mtgl/sssp_deltastepping.hpp>
#include <mtgl/st_search.hpp>
#include <mtgl/duplicate_adapter.hpp>
#include <mtgl/dynamic_array.hpp>

using namespace mtgl;

typedef static_graph_adapter<directedS> Graph;
typedef graph_traits<Graph>::size_type size_type;

// ********************************************************************
// ** compute_in_degree ***********************************************
// ********************************************************************
// ** Pure C traveral with MTA loop collapsing
// ********************************************************************
void compute_in_degree(static_graph<directedS>* g, size_type* in_degree)
{
  size_type i, j;
  #pragma mta assert parallel
  for (i = 0; i < g->n; i++)
  {
    size_type begin = g->index[i];
    size_type end = g->index[i + 1];
    #pragma mta assert parallel
    for (j = begin; j < end; j++)
    {
      mt_incr(in_degree[g->end_points[j]], 1);
    }
  }
}

// ********************************************************************
// ** compute_in_degree ***********************************************
// ********************************************************************
// ** Pure C traveral with an unsuccessful attempt to pipeline the
// ** inner loop
// ********************************************************************
void compute_in_degree2(static_graph<directedS>* g, size_type* in_degree)
{
  size_type i, j;
  #pragma mta assert parallel
  for (i = 0; i < g->n; i++)
  {
    size_type begin = g->index[i];
    size_type end = g->index[i + 1];
    #pragma mta assert nodep
    for (j = begin; j < end; j++)
    {
      mt_incr(in_degree[g->end_points[j]], 1);
    }
  }
}

// ********************************************************************
// ** compute_in_degree(static_graph_adapter..) ***********************
// ********************************************************************
// ** Manual traversal using an MTGL adapter to access vertex and edge
// ** information
// ********************************************************************
void compute_in_degree(Graph& g, size_type* in_degree)
{
  typedef graph_traits<Graph>::vertex_descriptor
  vertex_descriptor_t;
  typedef graph_traits<Graph>::adjacency_iterator
  adj_iterator_t;
  typedef graph_traits<Graph>::vertex_iterator
  vertex_iterator_t;

  size_type i, j;
  size_type n = num_vertices(g);
  vertex_id_map<Graph> vid_map = get(_vertex_id_map, g);
  vertex_iterator_t verts = vertices(g);

  #pragma mta assert parallel
  for (i = 0; i < n; i++)
  {
    vertex_descriptor_t v = verts[i];
    adj_iterator_t adjs = adjacent_vertices(v, g);
    size_type deg = out_degree(v, g);

    #pragma mta assert parallel
    for (j = 0; j < deg; j++)
    {
      vertex_descriptor_t neighbor = adjs[j];
      size_type nid = get(vid_map, neighbor);
      mt_incr(in_degree[nid], 1);
    }
  }
}

// ********************************************************************
// ** class in_degree_adj_visitor
// ********************************************************************
// ** Objects of this class are instantiated by the user and passed
// ** to generic MTGL functions that encapsulate the traversal
// ********************************************************************
template <typename graph>
class in_degree_adj_visitor {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;

  in_degree_adj_visitor(graph& gg, vertex_id_map<graph>& vidm, size_type* in_deg)
    : g(gg), vid_map(vidm), in_degree(in_deg) {}

  in_degree_adj_visitor(const in_degree_adj_visitor& cv)
    : g(cv.g), vid_map(cv.vid_map), in_degree(cv.in_degree) {}

  bool visit_test(vertex_t v) { return true; }
  void pre_visit(vertex_t v) {}
  void post_visit(vertex_t v) {}

  void operator()(edge_t e, vertex_t src, vertex_t dest)
  {
    size_type tid = get(vid_map, dest);
    mt_incr(in_degree[tid], 1);
  }

  // copy of operator() that gets inlined
  void process_edge(edge_t e, vertex_t src, vertex_t dest)
  {
    size_type tid = get(vid_map, dest);
    mt_incr(in_degree[tid], 1);
  }

private:
  graph& g;
  vertex_id_map<graph> vid_map;
  size_type* in_degree;
};

int main(int argc, char* argv[])
{
  srand48(0);

  size_type srcs[] =  { 0, 0, 2, 1, 2, 2, 3, 4, 5, 6 };
  size_type dests[] = { 1, 2, 1, 3, 3, 4, 4, 5, 6, 2 };
  Graph sga;
  init(7, 10, srcs, dests, sga);
  print(sga);
  int plen = 0, numvis = 0;
  st_connectivity<Graph, DIRECTED> stc(sga, 5, 3, plen, numvis);
  int stticks = stc.run();
  printf("path_length: %d, numvis: %d\n", plen, numvis);

  // G        [ IN] - reference to graph adapter.
  // s        [ IN] - source vertex id
  // realWt   [ IN] - edge weights (|E|)
  // cs       [OUT] - pointer to checksum value (scalar)
  // result   [OUT] - vertex weights (|V|)
  // vertTime [ IN] - vertex time stamps (optional) (|V|)
  double* weights = (double*) malloc(sizeof(double) * 10);
  double* result = (double*) malloc(sizeof(double) * 7);
  double* checksum = (double*) malloc(sizeof(double) * 7);

  for (size_type i = 0; i < 10; i++) weights[i] = 1.0;
  weights[0] = weights[2] = weights[5] = 0.5;

  for (size_type i = 0; i < 7; i++)
  {
    checksum[i] = 1.0;
    result[i] = DBL_MAX;
  }
  result[0] = 0.0;

  sssp_deltastepping<Graph, size_type*> sds(sga, 0, weights, checksum, result);
  double secs = sds.run();
  for (size_type i = 0; i < 7; i++)
  {
    printf("distance[%d]: %lf\n", i, result[i]);
  }

  dynamic_array<int> e_touched;
  // vertices touched by combined short paths s-t

  duplicate_adapter<Graph> dg(sga);
  print(dg);
  st_search<duplicate_adapter<Graph>, UNDIRECTED> sts(dg, 5, 3, e_touched);
  stticks = sts.run();
  printf("printing_path edge ids\n");
  for (size_type i = 0; i < e_touched.size(); i++)
  {
    printf("edges[%d]: %d\n", i, e_touched[i]);
  }
  printf("done printing_path_ids\n");

  if (argc < 2)
  {
    fprintf(stderr, "usage: %s <p>\n", argv[0]);
    fprintf(stderr, "(for 2^p vertex r-mat)\n");
    exit(1);
  }

  return 0;
}

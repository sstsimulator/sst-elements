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
/*! \file test_static_graph3.cpp

    \brief This test program compares MTA performance of various traversals of
           a C compressed sparse row graph representation.

    \author Jon Berry (jberry@sandia.gov)

    \date 1/28/2008

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

#include <mtgl/static_graph_adapter.hpp>
#include <mtgl/generate_rmat_graph.hpp>
#include <mtgl/breadth_first_search_mtgl.hpp>
#include <mtgl/visit_adj.hpp>

using namespace mtgl;

// ********************************************************************
// ** compute_in_degree ***********************************************
// ********************************************************************
// ** Pure C traversal - compiler foiled by parameter confusion
// ********************************************************************
template <typename Graph>
void compute_in_degree(Graph* g, unsigned long* in_degree)
{
  unsigned long i, j;
  unsigned long* index = g->index;
  unsigned long* end_points = g->end_points;

  #pragma mta assert parallel
  for (i = 0; i < g->n; i++)
  {
    unsigned long begin = index[i];
    unsigned long end = index[i + 1];

    #pragma mta assert parallel
    for (j = begin; j < end; j++)
    {
      mt_incr(in_degree[end_points[j]], 1);
    }
  }
}

// ********************************************************************
// ** compute_in_degree2 **********************************************
// ********************************************************************
// ** Pure C traversal:  compiler confused by parameter; no merge of
// ** loop
// ********************************************************************
template <typename Graph>
void compute_in_degree2(Graph* g, unsigned long* in_degree)
{
  unsigned long i, j, n = g->n;
  unsigned long* index = g->index;
  unsigned long* end_points = g->end_points;
  unsigned long* indeg = in_degree;

  #pragma mta assert parallel
  for (i = 0; i < n; i++)
  {
    unsigned long begin = index[i];
    unsigned long end = index[i + 1];

    for (j = begin; j < end; j++)
    {
      mt_incr(indeg[g->end_points[j]], 1);
    }
  }
}

// ********************************************************************
// ** compute_in_degree3 **********************************************
// ********************************************************************
// ** Pure C traversal:  compiler successfully merges loops
// ********************************************************************
template <typename Graph>
unsigned long* compute_in_degree3(Graph* g)
{
  unsigned long i, j, n = g->n;
  unsigned long* index = g->index;
  unsigned long* end_points = g->end_points;
  unsigned long* indeg = (unsigned long*) malloc(sizeof(unsigned long) * n);
  memset(indeg, 0, sizeof(unsigned long) * n);

  #pragma mta assert parallel
  for (i = 0; i < n; i++)
  {
    unsigned long begin = index[i];
    unsigned long end =   index[i + 1];

    for (j = begin; j < end; j++)
    {
      mt_incr(indeg[g->end_points[j]], 1);
    }
  }

  return indeg;
}

// ********************************************************************
// ** compute_in_degree(static_graph_adapter..) ***********************
// ********************************************************************
// ** Manual traversal using an MTGL adapter to access vertex and edge
// ** information
// ********************************************************************
template <typename graph_adapter>
void compute_in_degree_adapter(graph_adapter& g,
    typename graph_traits<graph_adapter>::size_type* in_degree)
{
  typedef typename graph_traits<graph_adapter>::vertex_descriptor
          vertex_descriptor_t;
  typedef typename graph_traits<graph_adapter>::adjacency_iterator
          adj_iterator_t;
  typedef typename graph_traits<graph_adapter>::vertex_iterator
          vertex_iterator_t;
  typedef typename graph_traits<graph_adapter>::size_type count_t;

  unsigned long i, j;
  count_t n = num_vertices(g);
  vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, g);
  vertex_iterator_t verts = vertices(g);

  #pragma mta assert parallel
  for (i = 0; i < n; i++)
  {
    vertex_descriptor_t v = verts[i];
    adj_iterator_t adjs = adjacent_vertices(v, g);
    count_t deg = out_degree(v, g);

    #pragma mta assert parallel
    for (j = 0; j < deg; j++)
    {
      vertex_descriptor_t neighbor = adjs[j];
      count_t nid = get(vid_map, neighbor);
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
  typedef typename graph_traits<graph>::size_type count_t;

  in_degree_adj_visitor(graph& gg, vertex_id_map<graph>& vidm,
                        count_t* in_deg) :
    g(gg), vid_map(vidm), in_degree(in_deg) {}

  in_degree_adj_visitor(const in_degree_adj_visitor& cv) :
    g(cv.g), vid_map(cv.vid_map), in_degree(cv.in_degree) {}

  bool visit_test(vertex_t v) { return true; }
  void pre_visit(vertex_t v) {}
  void post_visit(vertex_t v) {}

  void operator()(edge_t e, vertex_t src, vertex_t dest)
  {
    count_t tid = get(vid_map, dest);
    mt_incr(in_degree[tid], 1);
  }

  // Copy of operator() that gets inlined.
  void process_edge(edge_t e, vertex_t src, vertex_t dest)
  {
    count_t tid = get(vid_map, dest);
    mt_incr(in_degree[tid], 1);
  }

private:
  graph& g;
  vertex_id_map<graph> vid_map;
  count_t* in_degree;
};

template <typename graph>
class in_degree_adj_visitor2 {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::size_type count_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;
  typedef typename graph_traits<graph>::vertex_iterator vertex_iterator_t;

  in_degree_adj_visitor2(graph& gg, vertex_id_map<graph>& vidm,
                         count_t* in_deg) :
    g(gg), vid_map(vidm), in_degree(in_deg), order(num_vertices(gg)) {}

  in_degree_adj_visitor2(const in_degree_adj_visitor2& cv) :
    g(cv.g), vid_map(cv.vid_map), in_degree(cv.in_degree), order(cv.order) {}

  bool visit_test(vertex_t v) { return true; }
  void pre_visit(vertex_t v) {}
  void post_visit(vertex_t v) {}
  void post_visit() {}

  void operator()(vertex_t src, vertex_t dest)
  {
    count_t tid = get(vid_map, dest);
    mt_incr(in_degree[tid], 1);
  }

  // Copy of operator() that gets inlined.
  void process_adj(vertex_t src, vertex_t dest)
  {
    count_t tid = get(vid_map, dest);
    mt_incr(in_degree[tid], 1);
    count_t ind = dest % order;
    mt_incr(in_degree[ind], 1);
  }

private:
  graph& g;
  vertex_id_map<graph> vid_map;
  count_t* in_degree;
  count_t order;
};

int main(int argc, char* argv[])
{
  typedef static_graph_adapter<undirectedS> Graph;
  typedef graph_traits<Graph>::size_type count_t;
  typedef graph_traits<Graph>::vertex_iterator vertex_iterator_t;

  if (argc < 2)
  {
    fprintf(stderr, "usage: %s <p>\n", argv[0]);
    fprintf(stderr, "(for 2^p vertex r-mat)\n");
    exit(1);
  }

  Graph g;
  generate_rmat_graph(g, atoi(argv[1]), 8);

  count_t order = num_vertices(g);
  vertex_id_map<Graph> vid_map = get(_vertex_id_map, g);

  count_t* indeg = new count_t[order];

  static_graph<undirectedS>* sg = g.get_graph();
  mt_timer timer;
  int issues, memrefs, concur, streams, traps;

  // ********* sort degrees for load balancing *********************
  init_mta_counters(timer, issues, memrefs, concur, streams);
  count_t* degs = new count_t[order];

  vertex_iterator_t verts = vertices(g);
  #pragma mta assert nodep
  for (count_t i = 0; i < order; i++) degs[i] = out_degree(verts[i], g);

  count_t maxval = 0;

  #pragma mta assert nodep
  for (count_t i = 0; i < order; i++)
  {
    if (degs[i] > maxval) maxval = degs[i]; // Compiler removes reduction.
  }

  mtgl::countingSort(degs, order, maxval);
  count_t twenty_fifth = degs[order - 25];
  sample_mta_counters(timer, issues, memrefs, concur, streams);

  delete [] degs;

  printf("---------------------------------------------\n");
  printf("Sorting degrees to find 25th largest: \n");
  printf("---------------------------------------------\n");
  print_mta_counters(timer, num_edges(g), issues, memrefs, concur, streams);

  printf("hit enter to start\n");
  getchar();

  for (count_t i = 0; i < order; i++) indeg[i] = 0;

  // ****************** through MTGL adapter ***********************
  compute_in_degree_adapter(g, indeg);
  init_mta_counters(timer, issues, memrefs, concur, streams);
  compute_in_degree_adapter(g, indeg);
  sample_mta_counters(timer, issues, memrefs, concur, streams);
  printf("---------------------------------------------\n");
  printf("Through MTGL adapter: \n");
  printf("---------------------------------------------\n");
  print_mta_counters(timer, num_edges(g), issues, memrefs, concur,
                     streams);

  for (count_t i = 0; i < order; i++) indeg[i] = 0;

  // ****************** through MTGL visit_adj ***********************
  in_degree_adj_visitor<Graph> vis(g, vid_map, indeg);
  visit_adj(g, vis);
  init_mta_counters(timer, issues, memrefs, concur, streams);
  visit_adj(g, vis);
  sample_mta_counters(timer, issues, memrefs, concur, streams);
  printf("---------------------------------------------\n");
  printf("Through MTGL visit_adj: \n");
  printf("---------------------------------------------\n");
  print_mta_counters(timer, num_edges(g), issues, memrefs, concur, streams);

  for (count_t i = 0; i < order; i++) indeg[i] = 0;

  // ****************** through new visit_adj ***********************
  in_degree_adj_visitor2<Graph> vis2(g, vid_map, indeg);
  count_t* prefix_counts = 0, * start_nodes = 0;
  count_t num_threads = 0;
  init_mta_counters(timer, issues, memrefs, concur, streams);
  visit_adj(g, vis2, prefix_counts, start_nodes, num_threads, 50);
  sample_mta_counters(timer, issues, memrefs, concur, streams);
  printf("---------------------------------------------\n");
  printf("Through MTGL visit_jon: \n");
  printf("---------------------------------------------\n");
  print_mta_counters(timer, num_edges(g), issues, memrefs, concur, streams);

  for (count_t i = 0; i < 10; i++) printf("indeg[%lu]: %lu\n", i, indeg[i]);

  for (count_t i = 0; i < order; i++) indeg[i] = 0;

  init_mta_counters(timer, issues, memrefs, concur, streams);
  visit_adj(g, vis2, prefix_counts, start_nodes, num_threads, 50);
  sample_mta_counters(timer, issues, memrefs, concur, streams);
  printf("---------------------------------------------\n");
  printf("Through MTGL visit_jon: \n");
  printf("---------------------------------------------\n");
  print_mta_counters(timer, num_edges(g), issues, memrefs, concur, streams);

  for (count_t i = 0; i < 10; i++) printf("indeg[%lu]: %lu\n", i, indeg[i]);

  for (count_t i = 0; i < order; i++) indeg[i] = 0;

  // ****************** through MTGL visit_adj_high_low *************
  {
    in_degree_adj_visitor<Graph> vis(g, vid_map, indeg);
    visit_adj_high_low(g, vis, twenty_fifth);
    init_mta_counters(timer, issues, memrefs, concur, streams);
    visit_adj_high_low(g, vis, twenty_fifth);
    sample_mta_counters(timer, issues, memrefs, concur, streams);
    printf("---------------------------------------------\n");
    printf("Through MTGL visit_adj_high_low: \n");
    printf("---------------------------------------------\n");
    print_mta_counters(timer, num_edges(g), issues, memrefs, concur, streams);
  }

  for (count_t i = 0; i < order; i++) indeg[i] = 0;

  // ****************** pure C code ***********************
  compute_in_degree(sg, indeg);
  init_mta_counters(timer, issues, memrefs, concur, streams);
  compute_in_degree(sg, indeg);
  sample_mta_counters(timer, issues, memrefs, concur, streams);
  printf("---------------------------------------------\n");
  printf("Pure C: \n");
  printf("---------------------------------------------\n");
  print_mta_counters(timer, num_edges(g), issues, memrefs, concur, streams);

  for (count_t i = 0; i < order; i++) indeg[i] = 0;

  // ****************** pure C code ***********************
  compute_in_degree2(sg, indeg);
  init_mta_counters(timer, issues, memrefs, concur, streams);
  compute_in_degree2(sg, indeg);
  sample_mta_counters(timer, issues, memrefs, concur, streams);
  printf("---------------------------------------------\n");
  printf("Pure C (nodep): done\n");
  printf("---------------------------------------------\n");
  print_mta_counters(timer, num_edges(g), issues, memrefs, concur, streams);

  // ****************** pure C code ***********************
  compute_in_degree2(sg, indeg);
  init_mta_counters(timer, issues, memrefs, concur, streams);
  count_t* indeg2 = compute_in_degree3(sg);
  sample_mta_counters(timer, issues, memrefs, concur, streams);
  printf("---------------------------------------------\n");
  printf("Pure C (nodep): done\n");
  printf("---------------------------------------------\n");
  print_mta_counters(timer, num_edges(g), issues, memrefs, concur, streams);

  return 0;
}

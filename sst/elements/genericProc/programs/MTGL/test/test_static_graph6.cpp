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
/*! \file test_static_graph6.cpp

    \brief Compares visiting the adjacencies of a graph by accessing a CSR
           graph structure directly with using the MTGL interface with using
           the manually load-balanced version of visit_adj().

    \author Jon Berry (jberry@sandia.gov)
    \author Greg Mackey (gemacke@sandia.gov)

    /date 8/15/2009

    The inlined function test_pred_func() below represents a typical predicate
    that users may wish to introduce as an algorithmic filter.  Unfortunately,
    the use of this predicate via a function pointer in the inner loop of the
    function count_adjacencies_higher_id_func_ptr() prevents the loop-merge
    necessary for good performance.  The result is a serial inner loop, which
    has severe consequences when processing power-law data.
     
    The predicate is easily introduced into the function object
    test_pred_func_obj.  Using this function object instead of the function
    pointer restores the loop merge as demonstrated in the function
    count_adjacencies_higher_id_func_obj().
*/
/****************************************************************************/

#include <cstdlib>
#include <climits>

#include <mtgl/visit_adj.hpp>
#include <mtgl/static_graph_adapter.hpp>
#include <mtgl/generate_rmat_graph.hpp>
#include <mtgl/metrics.hpp>
#include <mtgl/read_matrix_market.hpp>
#include <mtgl/read_dimacs.hpp>
#include <mtgl/mtgl_io.hpp>

using namespace std;
using namespace mtgl;

typedef static_graph_adapter<directedS> Graph;
typedef graph_traits<Graph>::size_type size_type;
typedef graph_traits<Graph>::vertex_descriptor vertex_descriptor;
typedef graph_traits<Graph>::vertex_iterator vertex_iterator;

// ***************************************************************************
// ** count_adjacencies_higher_id ********************************************
// ***************************************************************************
// ** Pure C traveral
// ***************************************************************************
void count_adjacencies_higher_id(static_graph<directedS>* g, size_type* indeg)
{
  size_type order = g->n;
  size_type* index = g->index;
  size_type* end_points = g->end_points;

  #pragma mta assert noalias *indeg
  for (size_type i = 0; i < order; i++)
  {
    size_type begin = index[i];
    size_type end = index[i + 1];

    for (size_type j = begin; j < end; j++)
    {
      if (i < end_points[j]) mt_incr(indeg[end_points[j]], 1);
    }
  }
}

typedef bool (*pred_t)(size_type, size_type);

#pragma mta inline
inline bool test_pred_func(size_type i, size_type j)
{
  return (i < j);
}

void count_adjacencies_higher_id_func_ptr(static_graph<directedS>* g,
                                          size_type* indeg, pred_t my_func)
{
  size_type order = g->n;
  size_type* index = g->index;
  size_type* end_points = g->end_points;

  #pragma mta assert noalias *indeg
  #pragma mta assert parallel
  for (size_type i = 0; i < order; i++)
  {
    size_type begin = index[i];
    size_type end = index[i + 1];

    #pragma mta assert parallel
    for (size_type j = begin; j < end; j++)
    {
      if (my_func(i, end_points[j])) mt_incr(indeg[end_points[j]], 1);
    }
  }
}

class test_pred_func_obj {
public:
  inline bool operator()(size_type i, size_type j) { return (i < j); }
};

template <class pred>
void count_adjacencies_higher_id_func_obj(static_graph<directedS>* g,
                                          size_type* indeg, pred my_func)
{
  size_type order = g->n;
  size_type *index = g->index;
  size_type *end_points = g->end_points;

  #pragma mta assert noalias *indeg
  for (size_type i = 0; i < order; i++)
  {
    size_type begin = index[i];
    size_type end = index[i + 1];

    for (size_type j = begin; j < end; j++)
    {
      if (my_func(i, end_points[j])) mt_incr(indeg[end_points[j]], 1);
    }
  }
}

template <class Graph, class predicate>
void count_adjacencies_higher_id_mtgl(
    Graph& g, typename graph_traits<Graph>::size_type* indeg, predicate f)
{
  typedef typename graph_traits<Graph>::size_type size_type;
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;
  typedef typename graph_traits<Graph>::vertex_iterator vertex_iterator;
  typedef typename graph_traits<Graph>::adjacency_iterator adjacency_iterator;

  vertex_id_map<Graph> vid_map = get(_vertex_id_map, g);
  size_type order = num_vertices(g);
  vertex_iterator verts = vertices(g);

  #pragma mta assert noalias *indeg
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

      if (f(uid, vid)) mt_incr(indeg[vid], 1);
    }
  }
}

/// A visitor to the load-balanced visit_adj function.
template <typename graph, typename predicate>
class adjacencies_higher_id_computer {
public:
  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::vertex_descriptor vertex_descriptor;

  adjacencies_higher_id_computer(vertex_id_map<graph> vm, size_type *ind,
                                 predicate ff) :
    vid_map(vm), in_degree(ind), f(ff) {}

  inline void operator()(vertex_descriptor src, vertex_descriptor dest)
  {
    size_type v1id = get(vid_map, src);
    size_type v2id = get(vid_map, dest);

    if (f(v1id, v2id)) mt_incr(in_degree[v2id], 1);
  }

  void post_visit() {}

private:
  vertex_id_map<graph>& vid_map;
  size_type* in_degree;
  predicate f;
};

template <class Graph, class visitor>
void visit_adj_partial(Graph& g, visitor f)
{
  typedef typename graph_traits<Graph>::size_type size_type;
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;
  typedef typename graph_traits<Graph>::vertex_iterator vertex_iterator;

  const size_type* index = g.get_index();
  const vertex_descriptor* end_points = g.get_end_points();
  const size_type order = num_vertices(g);

  vertex_id_map<Graph> vid_map = get(_vertex_id_map, g);
  vertex_iterator verts = vertices(g);

  #pragma mta assert parallel
  for (size_type i = 0; i < order; i++)
  {
    vertex_descriptor u = verts[i];
    size_type begin = index[get(vid_map, u)];
    size_type end = index[get(vid_map, u) + 1];
    #pragma mta assert parallel
    for (size_type j = begin; j < end; j++)
    {
      vertex_descriptor v = verts[end_points[j]];
      f(u, v);
    }
  }
}

template <class Graph, class visitor>
void visit_adj_full(Graph& g, visitor f)
{
  typedef typename graph_traits<Graph>::size_type size_type;
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;
  typedef typename graph_traits<Graph>::adjacency_iterator adjacency_iterator;
  typedef typename graph_traits<Graph>::vertex_iterator vertex_iterator;

  const size_type *index = g.get_index();
  const vertex_descriptor *end_points = g.get_end_points();
  const size_type order = num_vertices(g);

  vertex_id_map<Graph> vid_map = get(_vertex_id_map, g);
  vertex_iterator verts = vertices(g);

  #pragma mta assert parallel
  for (size_type i = 0; i < order; i++)
  {
    vertex_descriptor u = verts[i];
    adjacency_iterator adjs = adjacent_vertices(u, g);
    size_type end = out_degree(u, g);
    #pragma mta assert parallel
    for (size_type j = 0; j < end; j++)
    {
      vertex_descriptor v = adjs[j];
      f(u, v);
    }
  }
}

template <class Graph, class visitor>
void visit_adj_partial(Graph& g,
         typename graph_traits<Graph>::vertex_descriptor* to_visit,
         typename graph_traits<Graph>::size_type num_to_visit,
         visitor f)
{
  typedef typename graph_traits<Graph>::size_type size_type;
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;

  const size_type *index = g.get_index();
  const vertex_descriptor *end_points = g.get_end_points();

  vertex_id_map<Graph> vid_map = get(_vertex_id_map, g);

  #pragma mta assert parallel
  for (size_type i = 0; i < num_to_visit; i++)
  {
    vertex_descriptor u = to_visit[i];
    size_type begin = index[get(vid_map, u)];
    size_type end = index[get(vid_map, u) + 1];
    #pragma mta assert parallel
    for (size_type j = begin; j < end; j++)
    {
      f(end_points[i], end_points[j]);
    }
  }
}

template <class Graph, class visitor>
void visit_adj(Graph& g,
         typename graph_traits<Graph>::vertex_descriptor* to_visit,
         typename graph_traits<Graph>::size_type num_to_visit,
         visitor f)
{
  typedef typename graph_traits<Graph>::size_type size_type;
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;
  typedef typename graph_traits<Graph>::adjacency_iterator adjacency_iterator;

  const size_type *index = g.get_index();
  const vertex_descriptor *end_points = g.get_end_points();
  vertex_id_map<Graph> vid_map = get(_vertex_id_map, g);

  #pragma mta assert parallel
  for (size_type i = 0; i < num_to_visit; i++)
  {
    vertex_descriptor u = to_visit[i];
    adjacency_iterator adjs = adjacent_vertices(u, g);
    size_type deg = out_degree(u, g);

    #pragma mta assert parallel
    for (size_type j = 0; j < deg; j++)
    {
      vertex_descriptor v = adjs[j];
      f(u, j);
    }
  }
}

void checkError(size_type* indeg, size_type* indeg2, size_type order)
{
  int error = -1;

  for (int i = 0; i < order; i++)
  {
    if (indeg[i] != indeg2[i]) error = i;
  }

  if (error != -1)
  {
    printf("Error in computation: pos %d\n", error);
  }
}

int main(int argc, char* argv[])
{
  srand48(0);

  if (argc < 2)
  {
    fprintf(stderr, "Usage: %s {<p>|<filename>} \n", argv[0]);
    fprintf(stderr, "where specifying p requests a generated r-mat graph "
            "with 2^p vertices,\n");
    fprintf(stderr, "and specifying filename requests that a DIMACS or "
            "MatrixMarket file (or the .srcs file of a .srcs,.dests "
            "snapshot pair) be read to build the graph;\n");
    fprintf(stderr, "delta is the convergence tolerance of "
            "the power iteration.\n");
    fprintf(stderr, "DIMACS files must end with suffix .dimacs\n");
    fprintf(stderr, "MatrixMarket files must end with suffix .mtx\n");
    exit(1);
  }

  // Determine which input format is used:  automatically generated rmat
  // or file-based input.
  int use_rmat = 1;
  int llen = strlen(argv[1]);

  for (int i = 0; i < llen; i++)
  {
    if (!(argv[1][i] >= '0' && argv[1][i] <= '9'))
    {
      // String contains non-numeric characters; it must be a filename.
      use_rmat = 0;
      break;
    }
  }

  Graph g;

  if (use_rmat)
  {
    generate_rmat_graph(g, atoi(argv[1]), 8);
  }
  else if (argv[1][llen - 1] == 'x')
  {
    // Matrix-market input.
    dynamic_array<int> weights;

    read_matrix_market(g, argv[1], weights);
  }
  else if (strcmp(&argv[1][llen - 4], "srcs") == 0)
  {
    // Snapshot input: <file>.srcs, <file>.dests
    char* srcs_fname = argv[1];
    char dests_fname[256];

    strcpy(dests_fname, srcs_fname);
    strcpy(&dests_fname[llen - 4], "dests");

    read_binary(g, srcs_fname, dests_fname);
  }
  else if (argv[1][llen - 1] == 's')
  {
    // DIMACS input.
    dynamic_array<int> weights;

    read_dimacs(g, argv[1], weights);
  }
  else
  {
    fprintf(stderr, "Invalid filename %s\n", argv[1]);
  }

  size_type order = num_vertices(g);
  size_type size = num_edges(g);
  size_type* indeg = new size_type[order];
  size_type* indeg2 = new size_type[order];

  test_pred_func_obj tpc;

  printf("order: %d, size: %d\n", order, size);

  static_graph<directedS> *sg = g.get_graph();

  for (int i = 0; i < order; i++) indeg[i] = 0;

  mt_timer timer;
  int issues, memrefs, concur, streams, traps;
  int phantoms, ready;

  init_mta_counters(timer, issues, memrefs, concur, streams, traps);

#ifdef __MTA__
  phantoms = mta_get_task_counter(RT_PHANTOM);
  ready = mta_get_task_counter(RT_READY);
#endif

  count_adjacencies_higher_id(sg, indeg);

  sample_mta_counters(timer, issues, memrefs, concur, streams, traps);

#ifdef __MTA__
  phantoms = mta_get_task_counter(RT_PHANTOM) - phantoms;
  ready = mta_get_task_counter(RT_READY) - ready;
#endif

  printf("---------------------------------------------\n");
  printf("count_adjacencies_higher_id(): \n");
  printf("---------------------------------------------\n");
  print_mta_counters(timer, num_edges(g), issues, memrefs, concur, streams,
                     traps);
#ifdef __MTA__
  printf("phantoms: %d\n", phantoms);
  printf("ready: %d\n", ready);
#endif

  for (int i = 0; i < order; i++) indeg2[i] = 0;

  init_mta_counters(timer, issues, memrefs, concur, streams, traps);

#ifdef __MTA__
  phantoms = mta_get_task_counter(RT_PHANTOM);
  ready = mta_get_task_counter(RT_READY);
#endif

  count_adjacencies_higher_id_func_ptr(sg, indeg2, test_pred_func);

  sample_mta_counters(timer, issues, memrefs, concur, streams, traps);

#ifdef __MTA__
  phantoms = mta_get_task_counter(RT_PHANTOM) - phantoms;
  ready = mta_get_task_counter(RT_READY) - ready;
#endif

  printf("---------------------------------------------\n");
  printf("count_adjacencies_higher_id_func_ptr(): \n");
  printf("---------------------------------------------\n");
  print_mta_counters(timer, num_edges(g), issues, memrefs, concur, streams,
                     traps);

#ifdef __MTA__
  printf("phantoms: %d\n", phantoms);
  printf("ready: %d\n", ready);
#endif

  checkError(indeg, indeg2, order);

  for (int i = 0; i < order; i++) indeg2[i] = 0;

  init_mta_counters(timer, issues, memrefs, concur, streams, traps);

#ifdef __MTA__
  phantoms = mta_get_task_counter(RT_PHANTOM);
  ready = mta_get_task_counter(RT_READY);
#endif

  count_adjacencies_higher_id_func_obj(sg, indeg2, tpc);

  sample_mta_counters(timer, issues, memrefs, concur, streams, traps);

#ifdef __MTA__
  phantoms = mta_get_task_counter(RT_PHANTOM) - phantoms;
  ready = mta_get_task_counter(RT_READY) - ready;
#endif

  printf("---------------------------------------------\n");
  printf("count_adjacencies_higher_id_func_obj(): \n");
  printf("---------------------------------------------\n");
  print_mta_counters(timer, num_edges(g), issues, memrefs, concur, streams,
                     traps);

#ifdef __MTA__
  printf("phantoms: %d\n", phantoms);
  printf("ready: %d\n", ready);
#endif

  checkError(indeg, indeg2, order);

  for (int i = 0; i < order; i++) indeg2[i] = 0;

  init_mta_counters(timer, issues, memrefs, concur, streams, traps);

#ifdef __MTA__
  phantoms = mta_get_task_counter(RT_PHANTOM);
  ready = mta_get_task_counter(RT_READY);
#endif

  count_adjacencies_higher_id_mtgl(g, indeg2, tpc);

  sample_mta_counters(timer, issues, memrefs, concur, streams, traps);

#ifdef __MTA__
  phantoms = mta_get_task_counter(RT_PHANTOM) - phantoms;
  ready = mta_get_task_counter(RT_READY) - ready;
#endif

  printf("---------------------------------------------\n");
  printf("count_adjacencies_higher_id_mtgl(): \n");
  printf("---------------------------------------------\n");
  print_mta_counters(timer, num_edges(g), issues, memrefs, concur, streams,
                     traps);

#ifdef __MTA__
  printf("phantoms: %d\n", phantoms);
  printf("ready: %d\n", ready);
#endif

  checkError(indeg, indeg2, order);

  for (int i = 0; i < order; i++) indeg2[i] = 0;

  adjacencies_higher_id_computer<Graph, test_pred_func_obj>
    idc(get(_vertex_id_map, g), indeg2, tpc);

  vertex_descriptor* to_visit =
    (vertex_descriptor*) malloc(sizeof(vertex_descriptor)*order);
  vertex_iterator_t verts = vertices(g);

  #pragma mta assert parallel
  for (int i = 0; i < order; i++) to_visit[i] = verts[i];

  size_type *prefix_counts, *started_nodes, num_threads;
  init_mta_counters(timer, issues, memrefs, concur, streams, traps);

#ifdef __MTA__
  phantoms = mta_get_task_counter(RT_PHANTOM);
  ready = mta_get_task_counter(RT_READY);
#endif

  visit_adj(g, idc, prefix_counts, started_nodes, num_threads);
  sample_mta_counters(timer, issues, memrefs, concur, streams, traps);

#ifdef __MTA__
  phantoms = mta_get_task_counter(RT_PHANTOM) - phantoms;
  ready = mta_get_task_counter(RT_READY) - ready;
#endif

  printf("---------------------------------------------\n");
  printf("visit_adj(): \n");
  printf("---------------------------------------------\n");
  print_mta_counters(timer, num_edges(g), issues, memrefs, concur, streams,
                     traps);

#ifdef __MTA__
  printf("phantoms: %d\n", phantoms);
  printf("ready: %d\n", ready);
#endif

  checkError(indeg, indeg2, order);

  return 0;
}

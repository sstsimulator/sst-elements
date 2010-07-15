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
/*! \file test_pagerank_bidir.cpp

    \author Brad Mancke
    \author Jon Berry (jberry@sandia.gov)

    \date 3/6/2008
*/
/****************************************************************************/

#include <cstdlib>

#include <mtgl/static_graph_adapter.hpp>
#include <mtgl/quicksort.hpp>
#include <mtgl/generate_rmat_graph.hpp>
#include <mtgl/breadth_first_search_mtgl.hpp>
#include <mtgl/visit_adj.hpp>
#include <mtgl/metrics.hpp>
#include <mtgl/read_matrix_market.hpp>
#include <mtgl/read_dimacs.hpp>
#include <mtgl/mtgl_io.hpp>

using namespace mtgl;

typedef struct {
  int degree;
#ifdef USING_QTHREADS
  aligned_t acc;
#else
  double acc;
#endif
  double rank;
} rank_info;

#ifdef USING_QTHREADS
struct cao_s {
  static_graph_adapter& g;
  rank_info* const rinfo;

  cao_s(static_graph_adapter& g, rank_info* const ri) : g(g), rinfo(ri) {}
};

void compute_acc_outer(qthread_t* me, const size_t startat,
                       const size_t stopat, void* arg)
{
  typedef graph_traits<static_graph_adapter>::vertex_descriptor
          vertex_descriptor;
  typedef graph_traits<static_graph_adapter>::adjacency_iterator
          adjacency_iterator;
  typedef graph_traits<static_graph_adapter>::vertex_iterator vertex_iterator;

  static_graph_adapter& g(((struct cao_s*) arg)->g);
  rank_info* const rinfo(((struct cao_s*) arg)->rinfo);
  vertex_id_map<static_graph_adapter> vid_map = get(_vertex_id_map, g);
  vertex_iterator verts = vertices(g);

  int j;

  for (size_t i = startat; i < stopat; i++)
  {
    vertex_descriptor v = verts[i];
    adjacency_iterator adjs = adjacent_vertices(v, g);
    int sid = get(vid_map, v);
    int deg = out_degree(v, g);

    for (j = 0; j < deg; j++)
    {
      vertex_descriptor neighbor = adjs[j];
      int tid = get(vid_map, neighbor);
      double r = rinfo[sid].rank;
      double d = (double) rinfo[sid].degree;
      qthread_incr(&(rinfo[tid].acc), (int) (10000 * r / d));
    }
  }
}

#endif

void compute_acc(static_graph_adapter& g, rank_info* rinfo)
{
  typedef graph_traits<static_graph_adapter>::vertex_descriptor
          vertex_descriptor;
  typedef graph_traits<static_graph_adapter>::adjacency_iterator
          adjacency_iterator;
  typedef graph_traits<static_graph_adapter>::vertex_iterator vertex_iterator;

  int i, j;
  int n = num_vertices(g);
  vertex_id_map<static_graph_adapter> vid_map = get(_vertex_id_map, g);

#ifdef USING_QTHREADS
  struct cao_s cao_arg(g, rinfo);
  qt_loop_balance(0, n, compute_acc_outer, &cao_arg);
#else
  vertex_iterator verts = vertices(g);
  #pragma mta assert parallel
  for (i = 0; i < n; i++)
  {
    vertex_descriptor v = verts[i];
    adjacency_iterator adjs = adjacent_vertices(v, g);
    int sid = get(vid_map, v);
    int deg = out_degree(v, g);

    #pragma mta assert parallel
    for (j = 0; j < deg; j++)
    {
      vertex_descriptor neighbor = adjs[j];
      int tid = get(vid_map, neighbor);
      double r = rinfo[sid].rank;
      rinfo[tid].acc += (r / (double) rinfo[sid].degree) * 10000;
    }
  }
#endif
}

// ********************************************************************
// ** class
// ********************************************************************
// ** Objects of this class are instantiated by the user and passed
// ** to generic MTGL functions that encapsulate the traversal
// ********************************************************************
template <typename graph>
class page_rank_visitor {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_descriptor;
  typedef typename graph_traits<graph>::edge_descriptor edge_descriptor;

  page_rank_visitor(graph& gg, vertex_id_map<graph>& vidm, rank_info* _rinfo) :
    g(gg), vid_map(vidm), rinfo(_rinfo) {}

  page_rank_visitor(const page_rank_visitor& cv) :
    g(cv.g), vid_map(cv.vid_map), rinfo(cv.rinfo) {}

  void pre_visit(vertex_descriptor v) {}

  void operator()(edge_descriptor e, vertex_descriptor src,
                  vertex_descriptor dest)
  {
    int sid = get(vid_map, src);
    int tid = get(vid_map, dest);
    double r = rinfo[sid].rank;
    rinfo[tid].acc += (r / (double) rinfo[sid].degree) * 10000;
  }

private:
  graph& g;
  vertex_id_map<graph> vid_map;
  rank_info* rinfo;
};

template <class graph_adapter>
int select_twenty_fifth(graph_adapter& g)
{
  typedef typename graph_traits<graph_adapter>::vertex_iterator vertex_iterator;

  // ********* sort degrees for load balancing *********************
  int order = num_vertices(g);
  int* degs = new int[order];

  vertex_iterator verts = vertices(g);
  #pragma mta assert nodep
  for (int i = 0; i < order; i++) degs[i] = out_degree(verts[i], g);

  int maxval = 0;

  #pragma mta assert nodep
  for (int i = 0; i < order; i++)
  {
    if (degs[i] > maxval) maxval = degs[i]; // compilers removes reduction
  }

  mtgl::countingSort(degs, order, maxval + 1);
  int twenty_fifth = degs[order - 25];

  delete [] degs;

  return twenty_fifth;
}

#ifdef USING_QTHREADS
extern "C" aligned_t setaffin(qthread_t* me, void* arg)
{
  cpu_set_t mask;
  int cpu = *(int*) arg;
  CPU_ZERO(&mask);
  CPU_SET(cpu, &mask);

  if (sched_setaffinity(0, sizeof(cpu_set_t), &mask) < 0)
  {
    perror("sched_setaffinity");
    return -1;
  }

  return 0;
}
#endif

int main(int argc, char* argv[])
{
  typedef static_graph_adapter<directedS> Graph;
  typedef graph_traits<Graph>::vertex_iterator vertex_iterator;
  typedef graph_traits<Graph>::size_type size_type;

  if (argc < 3)
  {
    fprintf(stderr, "Usage: %s {<p>|<filename>} <delta>\n", argv[0]);
    fprintf(stderr, "where specifying p requests a "
            "generated r-mat graph "
            "with 2^p vertices,\n");
    fprintf(stderr, "and specifying filename requests that a "
            "DIMACS or MatrixMarket "
            "file be read to build the graph;\n");
    fprintf(stderr, "delta is the convergence tolerance of "
            "the power iteration.\n");
    fprintf(stderr, "DIMACS files must end with suffix .dimacs\n");
    fprintf(stderr, "MatrixMarket files must end with "
            "suffix .mtx\n");
    exit(1);
  }

#ifdef USING_QTHREADS
  if (argc < 4)
  {
    fprintf(stderr, "usage: %s <p> <delta> <threads>\n", argv[0]);
    fprintf(stderr, "(for 2^p vertex r-mat)\n");
    exit(1);
  }

  int threads = atoi(argv[3]);
  qthread_init(threads);
  aligned_t rets[threads];
  int args[threads];

  for (int i = 0; i < threads; i++)
  {
    args[i] = i;
    qthread_fork_to(setaffin, args + i, rets + i, i);
  }

  for (int i = 0; i < threads; i++)
  {
    qthread_readFF(NULL, rets + i, rets + i);
  }
#endif

  double dampen = 0.8;
  double delta = (double) atof(argv[2]);

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

  // degree seq
  //int *degseq = degree_distribution(g);
  //for (int i=0; i<32; i++) {
  //  printf(" deg 2^%d: %d\n", i, degseq[i]);
  //}
  //delete [] degseq;

  int order = num_vertices(g);
  if (order < 20) print(g);

  //int twenty_fifth_degree = select_twenty_fifth(g);

  int big_degree = 5000;
  vertex_id_map<static_graph_adapter> vid_map = get(_vertex_id_map, g);

  rank_info* rinfo = new rank_info[order];
  vertex_iterator verts = vertices(g);
  #pragma mta assert nodep
  for (int i = 0; i < order; i++)
  {
    rinfo[i].rank = 1.0;
    rinfo[i].degree = out_degree(verts[i], g);
  }

  mt_timer timer;
  int issues, memrefs, concur, streams, traps;
  double maxdiff = 0;

  page_rank_visitor<Graph> vis(g, vid_map, rinfo);
  printf("starting.....\n");
  init_mta_counters(timer, issues, memrefs, concur, streams);

  do
  {
    #pragma mta assert nodep
    for (int i = 0; i < order; i++) rinfo[i].acc = 0;

    visit_adj_high_low(g, vis, big_degree);

    //break;
    //compute_acc(g, rinfo);

    maxdiff = 0;

    #pragma mta assert nodep
    for (int i = 0; i < order; i++)
    {
      double oldval = rinfo[i].rank;
      double newval = rinfo[i].rank = (1 - dampen) +
                                      (dampen * rinfo[i].acc / 10000.0);
      double absdiff = (oldval > newval) ? oldval - newval : newval - oldval;

      if (absdiff > maxdiff) maxdiff = absdiff;
    }

  } while (maxdiff > delta);

  sample_mta_counters(timer, issues, memrefs, concur, streams);
  printf("---------------------------------------------\n");
  printf("Page rank performance stats: \n");
  printf("---------------------------------------------\n");
  print_mta_counters(timer, num_edges(g), issues, memrefs, concur, streams);

  if (order >= 20)
  {
    printf("---------------------------------------------\n");
    printf("Top 20 page ranks: \n");
    printf("---------------------------------------------\n");
    double* ranks = new double[order];

    #pragma mta assert nodep
    for (int i = 0; i < order; i++) ranks[i] = rinfo[i].rank;

    quicksort<double> qs(ranks, order);
    qs.run();

    for (int i = order - 20; i < order; i++) printf("%lf\n", ranks[i]);

    delete[] ranks;
  }

#ifdef USING_QTHREADS
  qthread_finalize();
#endif

  return 0;
}

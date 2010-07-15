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
/*! \file test_neighborhoods.cpp

    \author Jon Berry (jberry@sandia.gov)

    \date 7/8/2009
*/
/****************************************************************************/

#include <mtgl/util.hpp>
#include <mtgl/metrics.hpp>
#include <mtgl/triangles.hpp>
#include <mtgl/read_dimacs.hpp>
#include <mtgl/read_matrix_market.hpp>
#include <mtgl/generate_rmat_graph.hpp>
#include <mtgl/static_graph_adapter.hpp>
#include <mtgl/bfs.hpp>
#include <mtgl/mtgl_io.hpp>
#include <mtgl/xmt_hash_table.hpp>

#define DOUBLE 1

#ifdef DOUBLE
  #include <mtgl/dneighborhoods.hpp>
  #define UNIT 1.0
  #else
  #include <mtgl/neighborhoods.hpp>
  #define UNIT 1024           // Unit increments to hash tables for floating
                              // point values are UNIT, and we obtain the
                              // floating point values at the end via
                              // division by UNIT.  This is to stick with
                              // integer arithmetic on the XMT.
#endif

#define NTHRESH 80

using namespace std;
using namespace mtgl;

class pseudo_dhashvis {
public:
  pseudo_dhashvis() {}
  void operator()(int64_t key, double value)
  {
    printf("Table[%jd]: %f\n", key, value);
  }

  void operator()(int64_t key, unsigned long value)
  {
    printf("Table[%jd]: %f\n", key, value / (double) UNIT);
  }

  void operator()(int64_t key, int64_t value)
  {
    printf("Table[%jd]: %f\n", key, value / (double) UNIT);
  }

};

template <typename graph>
class eweight_initializer {
public:
  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::vertex_descriptor vertex_descriptor;
  typedef typename graph_traits<graph>::edge_descriptor edge_descriptor;
#ifdef DOUBLE
  typedef xmt_hash_table<int64_t, double> hash_table_t;
#else
  typedef xmt_hash_table<int64_t, size_type> hash_table_t;
#endif

  eweight_initializer(graph& gg, hash_table_t& ew, int thresh = NTHRESH) :
    g(gg), threshold(thresh), e_weight(ew), order(num_vertices(gg))
  {
    vid_map = get(_vertex_id_map, g);
  }

  void operator()(vertex_descriptor src, vertex_descriptor dest)
  {
    size_type v1id = get(vid_map, src);
    size_type v2id = get(vid_map, dest);
    size_type my_degree = out_degree(src, g);
    size_type dest_deg = out_degree(dest, g);
    size_type src_id = get(vid_map, src);
    size_type dest_id = get(vid_map, dest);

    if (src_id > dest_id) return;

    int64_t key = src_id * order + dest_id;

    if (out_degree(src, g) <= NTHRESH && out_degree(dest, g) <= NTHRESH)
    {
      e_weight.insert(key, UNIT);              // See comment by #define UNIT.
    }
    else
    {
      e_weight.insert(key, 0);
    }
  }

  void post_visit() {}

private:
  graph& g;
  int threshold;
  unsigned int order;
  hash_table_t& e_weight;
  vertex_id_map<graph> vid_map;
};

int main(int argc, char* argv[])
{
#ifndef __MTA__
  srand48(0);
#endif

  typedef static_graph_adapter<undirectedS> Graph;
  typedef graph_traits<Graph>::size_type size_type;
  typedef graph_traits<Graph>::edge_descriptor edge_descriptor;
  typedef graph_traits<Graph>::edge_iterator edge_iterator;
#ifdef DOUBLE
  typedef xmt_hash_table<int64_t, double> hash_table_t;
#else
  typedef xmt_hash_table<int64_t, size_type> hash_table_t;
#endif

  if (argc < 2)
  {
    fprintf(stderr, "Usage: %s {<p>|<filename>}", argv[0]);
    fprintf(stderr, "where specifying p requests a "
            "generated r-mat graph "
            "with 2^p vertices,\n");
    fprintf(stderr, "and specifying filename requests that a "
            "DIMACS or MatrixMarket "
            "file be read to build the graph;\n");
    fprintf(stderr, "src is the index of the bfs source\n");
    fprintf(stderr, "DIMACS files must end with suffix .dimacs\n");
    fprintf(stderr, "MatrixMarket files must end with "
            "suffix .mtx\n");

    exit(1);
  }

#ifdef USING_QTHREADS
  if (argc < 3)
  {
    fprintf(stderr, "usage: %s {<p>|<filename>} <threads>\n", argv[0]);
    fprintf(stderr, "(for 2^p vertex r-mat)\n");
    exit(1);
  }

  int threads = atoi(argv[2]);
  printf("qthread_init(%d)\n", threads);
  fflush(stdout);
  qthread_init(threads);
  printf("qthread_init done(%d)\n", threads);
  fflush(stdout);
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

  typedef aligned_t uinttype;
#else
  typedef unsigned long uinttype;
#endif

  // Determine which input format is used:  automatically generated rmat
  // or file-based input.
  int use_rmat = 1;
  int llen = strlen(argv[1]);

  for (int i = 0; i < llen; i++)
  {
    if (!(argv[1][i] >= '0' && argv[1][i] <= '9'))
    {
      // String contains non-numeric characters;
      // it must be a filename.
      use_rmat = 0;
      break;
    }
  }

  double* weights = NULL;
  Graph ga;

  if (use_rmat)
  {
    generate_rmat_graph(ga, atoi(argv[1]), 8);
  }
  else if (argv[1][llen - 1] == 'x')
  {
    // Matrix-market input.
    dynamic_array<double> iweights;

    read_matrix_market(ga, argv[1], weights);

    edge_id_map<Graph> eid_map = get(_edge_id_map, ga);
    size_type m = num_edges(ga);
    edge_iterator edgs = edges(ga);

    for (size_type i = 0; i < m; i++)
    {
      edge_descriptor e = edgs[i];
      size_type eid = get(eid_map, e);
      weights[eid] = iweights[eid];
    }
  }
  else if (strcmp(&argv[1][llen - 4], "srcs") == 0)
  {
    // Snapshot input: <file>.srcs, <file>.dests
    char* srcs_fname = argv[1];
    char dests_fname[256];

    strcpy(dests_fname, srcs_fname);
    strcpy(&dests_fname[llen - 4], "dests");

    read_binary(ga, srcs_fname, dests_fname);

    size_type m = num_edges(ga);
    weights = (double*) malloc(m * sizeof(double));

    #pragma mta assert nodep
    for (size_type i = 0; i < m; i++) weights[i] = 1.0;
  }
  else if (strcmp(argv[1], "custom") == 0)
  {
    size_type num_verts = 5;
    size_type num_edges = 7;

    size_type* srcs = (size_type*) malloc(sizeof(size_type) * num_edges);
    size_type* dests = (size_type*) malloc(sizeof(size_type) * num_edges);

    srcs[0] = 0; dests[0] = 1;
    srcs[1] = 1; dests[1] = 2;
    srcs[2] = 2; dests[2] = 0;
    srcs[3] = 2; dests[3] = 3;
    srcs[4] = 3; dests[4] = 4;
    srcs[5] = 4; dests[5] = 2;
    srcs[6] = 0; dests[6] = 3;

    init(num_verts, num_edges, srcs, dests, ga);
  }
  else if (argv[1][llen - 1] == 's')
  {
    // DIMACS input.
    dynamic_array<double> iweights;

    read_dimacs(ga, argv[1], iweights);

    edge_id_map<Graph> eid_map = get(_edge_id_map, ga);
    size_type m = num_edges(ga);
    edge_iterator edgs = edges(ga);

    for (size_type i = 0; i < m; i++)
    {
      edge_descriptor e = edgs[i];
      size_type eid = get(eid_map, e);
      weights[eid] = iweights[eid];
    }
  }
  else
  {
    fprintf(stderr, "Invalid filename %s\n", argv[1]);
  }

  size_type order = num_vertices(ga);
  size_type size = num_edges(ga);

  vertex_id_map<Graph> vid_map = get(_vertex_id_map, ga);

  size_type* prefix_counts = 0, * started_nodes = 0;
  size_type num_threads = 0;

#ifdef DOUBLE
  weight_by_neighborhoods<Graph> wbn(ga, weights, NTHRESH);
  wbn.run(1);
#else
  hash_table_t en_count(int64_t(1.5 * size), true);
  hash_table_t good_en_count(int64_t(1.5 * size), true);

  weight_by_neighborhoods<Graph> wbn(ga, e_weight, NTHRESH, UNIT);
  wbn.run(1, good_en_count, en_count);
#endif

  if (order < 100)
  {
    print(ga);
    size_type m = num_edges(ga);
    edge_id_map<Graph> eid_map = get(_edge_id_map, ga);
    edge_iterator edgs = edges(ga);

    printf("about to print weights\n");
    fflush(stdout);

    for (size_type i = 0; i < m; i++)
    {
      edge_descriptor e = edgs[i];
      size_type eid = get(eid_map, e);

      printf("weight[%lu]: ", eid);
      printf(" %f\n", weights[eid]);
      fflush(stdout);
    }
  }

  return 0;
}

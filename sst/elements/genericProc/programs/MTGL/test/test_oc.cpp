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
/*! \file test_oc.cpp

    \author Jon Berry (jberry@sandia.gov)

    \date 9/8/2008
*/
/****************************************************************************/

#include <strstream>

#include "mtgl_test.hpp"

#include <mtgl/ufl.h>
#include <mtgl/util.hpp>
#include <mtgl/metrics.hpp>
#include <mtgl/SMVkernel.h>
#include <mtgl/static_graph_adapter.hpp>
#include <mtgl/generate_girvan_newman_graph.hpp>
#include <mtgl/neighborhoods.hpp>
#include <mtgl/connected_components.hpp>
#include <mtgl/contraction_adapter.hpp>
#include <mtgl/rand_fill.hpp>
#include <mtgl/visit_adj.hpp>
#include <mtgl/xmt_hash_table.hpp>

using namespace std;
using namespace mtgl;

template <typename graph_adapter>
class cost_filter {
public:
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_t;

  cost_filter(graph_adapter& gg, double* csts, double* rv, int& cn)
    : costs(csts), eid_map(get(_edge_id_map, gg)), rvals(rv), count(cn) {}

  bool operator()(edge_t e)
  {
    int eid = get(eid_map, e);
    if (rvals[eid] <= costs[eid])
    {
      count++;
      return true;
    }
    return false;
  }

private:
  edge_id_map<graph_adapter> eid_map;
  double* costs;
  double* rvals;
  int& count;
};

template <class graph_adapter>
int update_edge_support(graph_adapter* ga, int* comp, double* edge_support)
{
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_t;
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph_adapter>::edge_iterator edge_iterator_t;

  int size = num_edges(*ga);
  vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, *ga);

  edge_iterator_t edgs;
  for (int i = 0; i < size; i++)
  {
    edge_t e = edgs[i];
    vertex_t s = source(e, ga);
    vertex_t t = target(e, ga);
    int sid = get(vid_map, s);
    int tid = get(vid_map, t);

    int s_cr = comp[sid];
    int t_cr = comp[tid];

    if (s_cr == t_cr)
    {
      edge_support[i]++;
    }
  }
}

template <class graph_adapter>
double compute_squared_error(graph_adapter* ga, double* edge_support)
{
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_t;
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph_adapter>::edge_iterator edge_iterator_t;

  int size = num_edges(*ga);
  vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, *ga);
  double sqr_error = 0.0;

  edge_iterator_t edgs = edges(ga);
  for (int i = 0; i < size; i++)
  {
    edge_t e = edgs[i];
    vertex_t s = source(e, ga);
    vertex_t t = target(e, ga);
    int sid = get(vid_map, s);
    int tid = get(vid_map, t);

    if (sid / 32 == tid / 32)
    {
      sqr_error += ((1 - edge_support[i]) * (1 - edge_support[i]));
    }
    else
    {
      sqr_error += (edge_support[i] *edge_support[i]);
    }
  }

  return sqr_error;
}

template <typename graph_adapter>
int run_algorithm(graph_adapter& ga,
                  int& num_comm, double* rvals, FILE* avatar_fp)
{
  typedef graph_adapter Graph;
  typedef typename graph_traits<Graph>::edge_descriptor edge_t;
  typedef typename graph_traits<Graph>::edge_iterator edge_iter_t;
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_t;
  typedef xmt_hash_table<int64_t, int64_t> hash_table_t;

  int order = num_vertices(ga);
  int size =  num_edges(ga);
  printf("order: %d, size: %d\n", order, size);

  int* degseq = degree_distribution(ga);
  for (int i = 0; i < 32; i++)
  {
    printf(" deg 2^%d: %d\n", i, degseq[i]);
  }
  delete [] degseq;

  int* leader = new int[order];
  for (int i = 0; i < order; i++) leader[i] = i;

  int prev_num_components = order;
  Graph* cur_g = &ga;
  int* orig2contracted = 0;
  int* server = new int[size];
  double* primal = new double[size + 2 * size];
  double* support = 0;

  hash_table_t eweight(int64_t(10 * size), true);  // ouch! do better
  vertex_id_map<Graph> vid_map = get(_vertex_id_map, *cur_g);

  edge_iter_t edgs = edges(*cur_g);
  for (int i = 0; i < size; i++)
  {
    edge_t e = edgs[i];
    vertex_t v1 = source(e, *cur_g);
    vertex_t v2 = target(e, *cur_g);
    int v1id = get(vid_map, v1);
    int v2id = get(vid_map, v2);
    order_pair(v1id, v2id);
    int64_t key = v1id * order + v2id;
    eweight.insert(key, 1);
  }
////      DEBUG
  {
    nt num_verts = 4;
    int num_edges = 4;
    int* srcs = (int*) malloc(sizeof(int) * num_edges);
    int* dests = (int*) malloc(sizeof(int) * num_edges);
    srcs[0] =  0; dests[0] =  1;
    srcs[1] =  1; dests[1] =  2;
    srcs[2] =  2; dests[2] =  3;
    srcs[3] =  3; dests[3] =  0;
    Graph* ga2 = new Graph(UNDIRECTED);
    init(num_verts, num_edges, srcs, dests, *ga2);
    vertex_id_map<Graph> vid_map2 = get(_vertex_id_map, *ga2);

    edge_iter_t edgs = edges(*ga2);
    for (int i = 0; i < num_edges; i++)
    {
      edge_t e = edgs[i];
      vertex_t v1 = source(e, *ga2);
      vertex_t v2 = target(e, *ga2);
      int v1id = get(vid_map2, v1);
      int v2id = get(vid_map2, v2);
      printf("(%d,%d) key: ", v1id, v2id);
      order_pair(v1id, v2id);
      int64_t key = v1id * num_verts + v2id;
      eweight.insert(key, 1);
      printf("key: %jd \n", key);
    }

    neighbor_counts<Graph> nc(*ga2, eweight);
    int* vn_count = (int*) malloc(sizeof(int) * num_verts);
    hash_table_t en_count(int64_t(1.5 * size), true);
    hash_table_t good_en_count(int64_t(1.5 * size), true);
    nc.run(vn_count, en_count, good_en_count);

    for (int i = 0; i < num_verts; i++)
    {
      printf("n[%d]: %d\n", i, vn_count[i]);
    }

    hashvis h;
    en_count.visit(h);
    good_en_count.visit(h);
    exit(1);
  }

  neighbor_counts<Graph> nc(ga, eweight);
  int* vn_count = (int*) malloc(sizeof(int) * order);
  hash_table_t en_count(int64_t(1.5 * size), true);
  hash_table_t good_en_count(int64_t(1.5 * size), true);
  nc.run(vn_count, en_count, good_en_count);
  double* opening_costs = (double*) malloc(sizeof(double) * size);

  cost_visitor<Graph> cv(ga, opening_costs, vn_count, en_count, good_en_count);
  visit_edges(ga, cv);
  hashvis(h);
  printf("EDGE NEIGHBORHOOD\n");
  fflush(stdout);
  en_count.visit(h);
  printf("EDGE GOOD NEIGHBORHOOD\n");
  fflush(stdout);
  good_en_count.visit(h);
  edgs = edges(*cur_g);

  for (int i = 0; i < size; i++)
  {
    edge_t e = edgs[i];
    vertex_t v1 = source(e, *cur_g);
    vertex_t v2 = target(e, *cur_g);
    int v1id = get(vid_map, v1);
    int v2id = get(vid_map, v2);
    order_pair(v1id, v2id);
    int64_t key = v1id * order + v2id;
    bool intra = (v1id / 32 == v2id / 32);
    fprintf(avatar_fp, "%d,%d,%d,%d,%d,%d,%lf,%lf,%lf,%d\n",
            v1id, v2id, out_degree(v1, *cur_g), out_degree(v2, *cur_g),
            vn_count[v1id], vn_count[v2id], en_count[key] / 4.0,
            good_en_count[key] / 4.0,
            (good_en_count[key] / 4.0) / (en_count[key] / 4.0), intra);
  }

  int* component = (int*) malloc(sizeof(int) * order);
  for (int i = 0; i < order; i++) component[i] = i;

  int count = 0;
  //cost_filter<Graph> cf(ga, opening_costs, rvals,count);
  //connected_components_sv<graph_adapter, int,
  //        cost_filter<Graph> >
  //      csv(ga, component, cf);
  //num_comm = csv.run();
  //printf("filter count: %d\n", count);
  //update_edge_support(ga, component, edge_support);

  return 0;
}

int main(int argc, char* argv[])
{
  srand48(943200);

  if (argc < 3)
  {
    fprintf(stderr, "usage: %s [-debug] "
            " [-assort] <types> "
            " --graph_type <dimacs|cray> "
            " --level <levels> --graph <Cray graph number> "
            " --filename <dimacs graph filename> "
            " [<0..15>]\n", argv[0]);

    exit(1);
  }

  typedef static_graph_adapter<directedS> Graph;

  char* filenames[] = {
    "/home/jberry/tmp/newman_0.dimacs",
    "/home/jberry/tmp/newman_1.dimacs",
    "/home/jberry/tmp/newman_2.dimacs",
    "/home/jberry/tmp/newman_3.dimacs",
    "/home/jberry/tmp/newman_4.dimacs",
    "/home/jberry/tmp/newman_5.dimacs",
    "/home/jberry/tmp/newman_6.dimacs",
    "/home/jberry/tmp/newman_7.dimacs",
    "/home/jberry/tmp/newman_8.dimacs",
    "/home/jberry/tmp/newman_9.dimacs",
    "/home/jberry/tmp/newman_10.dimacs",
    "/home/jberry/tmp/newman_11.dimacs"
  };

  char* out_filenames[] = {
    "newman_0_ours.csv",
    "newman_1_ours.csv",
    "newman_2_ours.csv",
    "newman_3_ours.csv",
    "newman_4_ours.csv",
    "newman_5_ours.csv",
    "newman_6_ours.csv",
    "newman_7_ours.csv",
    "newman_8_ours.csv",
    "newman_9_ours.csv",
    "newman_10_ours.csv",
    "newman_11_ours.csv"
  };

  char* avatar_data_filenames[] = {
    "newman_0_ours.data",
    "newman_1_ours.data",
    "newman_2_ours.data",
    "newman_3_ours.data",
    "newman_4_ours.data",
    "newman_5_ours.data",
    "newman_6_ours.data",
    "newman_7_ours.data",
    "newman_8_ours.data",
    "newman_9_ours.data",
    "newman_10_ours.data",
    "newman_11_ours.data"
  };

  char* avatar_test_filenames[] = {
    "newman_0_ours.test",
    "newman_1_ours.test",
    "newman_2_ours.test",
    "newman_3_ours.test",
    "newman_4_ours.test",
    "newman_5_ours.test",
    "newman_6_ours.test",
    "newman_7_ours.test",
    "newman_8_ours.test",
    "newman_9_ours.test",
    "newman_10_ours.test",
    "newman_11_ours.test"
  };

//   FILE *fp = 0;
//   fp = fopen(out_filenames[newman_iter] , "w");

  bool training = false;
  FILE** afp = (FILE**) malloc(sizeof(FILE*) * 12);
  char** avatar_filenames;
  if (training)
  {
    avatar_filenames = (char**) avatar_data_filenames;
  }
  else
  {
    avatar_filenames = (char**) avatar_test_filenames;
  }

  for (int v_out = 0; v_out < 12; v_out++)
  {
    Graph ga;

    typedef graph_traits<Graph>::edge_descriptor edge_t;
    typedef graph_traits<Graph>::vertex_descriptor vertex_t;

    vertex_id_map<Graph> vid_map = get(_vertex_id_map, ga);

    int total_good = 0;
    int total_num_comm = 0;
    int num_runs = 1;

    afp[v_out] = fopen(avatar_filenames[v_out], "w");

    if (!afp[v_out])
    {
      fprintf(stderr, "ERROR, CAN'T OPEN FILE %s FOR WRITING\n",
              avatar_filenames[v_out]);
    }

    fprintf(afp[v_out], "#labels V_ONE_ID,V_TWO_ID,V1_DEGREE,V2_DEGREE,"
            "VN1_COUNT,VN2_COUNT,EN_TOTAL,EN_GOOD,EN_COHERENCE,"
            "CLASSIFICATION\n");

    for (int i = 0; i < num_runs; i++)
    {
      generate_girvan_newman_graph(ga, v_out);
      int nc = 0;
      int size = num_edges(ga);
      /*
      double* rvals = (double*) malloc(sizeof(double) * size);
      random_value* rv = (random_value*) malloc(size * sizeof(random_value));
      rand_fill::generate(size, rv);
      random_value rv_max = rand_fill::get_max();
            #pragma mta assert nodep
      for (int i=0; i<size; i++) {
        rvals[i] = ((double) rv[i])/rv_max;
      }
      */
      printf("run number: %d\n", i);
      fflush(stdout);
      total_good += run_algorithm<Graph>(ga, nc, 0, afp[v_out]);
      total_num_comm += nc;
      clear(ga);
    }

    fclose(afp[v_out]);
    double ave_good = total_good / (double) num_runs;
    double ave_num_comm = total_num_comm / (double) num_runs;
    //double sqr_err = compute_squared_error(ga, edge_supp);
    //printf("SQR ERROR(%d): %lf   [%lf communities]\n",
    //     v_out, sqr_err, ave_num_comm);

    //print out edge_supp array
    FILE* fp = 0;
    fp = fopen(out_filenames[v_out], "w");

    if (!fp)
    {
      fprintf(stderr, "ERROR, CAN'T OPEN FILE %s FOR WRITING\n",
              out_filenames[v_out]);
    }

    fprintf(fp, "VERT, TO_VERT, INTER_INTRA, SUPPORT, AVG_SCORE, "
            "SQUARED_ERROR\n");

    fclose(fp);
    fp = 0;

    //free(edge_supp);
    //edge_supp = 0;
  }

  return 0;
}

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
/*! \file test_bidirectional_graph.cpp

    \brief Tests the bidirectional version of static_graph_adapter.

    \author Brad Mancke

    \date 3/4/2008
*/
/****************************************************************************/

#include "mtgl_test.hpp"

#include <mtgl/util.hpp>
#include <mtgl/metrics.hpp>
#include <mtgl/static_graph_adapter.hpp>

using namespace mtgl;

template <typename graph_adapter>
void run_without_high_low(graph_adapter& ga)
{
  typedef typename graph_traits<graph_adapter>::vertex_descriptor
                   vertex_descriptor;
  typedef typename graph_traits<graph_adapter>::vertex_iterator vertex_iterator;
  typedef typename graph_traits<graph_adapter>::out_edge_iterator
                   out_edge_iterator;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_t;

  int n = num_vertices(ga);
  int m = num_edges(ga);

  double* accum = new double[m];
  vertex_iterator verts = vertices(ga);
  #pragma mta assert nodep
  for (int i = 0; i < n; i++) accum[i] = 0;

  vertex_id_map<graph_adapter> vipm = get(_vertex_id_map, ga);
  edge_id_map<graph_adapter> eipm = get(_edge_id_map, ga);

  #pragma mta assert parallel
  #pragma mta loop future
  for (int i = 0; i < n; i++)
  {
    vertex_descriptor v = verts[i];

    int vid = get(vipm, v);
    int deg = out_degree(v, ga);

    out_edge_iterator inc_edgs = out_edges(v, ga);

    #pragma mta assert parallel
    for (int j = 0; j < deg; j++)
    {
      edge_t e = inc_edges[j];

      vertex_descriptor trg = target(e, ga);
      int tid = get(vipm, trg);
      double incr = vid * 0.2;
      double& atrg = accum[tid];

      atrg += incr;
    }
  }

  if (n < 50)
  {
    for (int i = 0; i < n; i++)
    {
      printf("accum[%d]: %lf\n", i, accum[i]);
    }
  }

  delete [] accum;
}

template <>
void run_without_high_low<static_graph_adapter<bidirectionalS> >(
    static_graph_adapter<bidirectionalS>& ga)
{
  typedef static_graph_adapter<bidirectionalS> Graph;
  typedef graph_traits<Graph>::vertex_descriptor vertex_descriptor;
  typedef graph_traits<Graph>::vertex_iterator vertex_iterator;
  typedef graph_traits<Graph>::out_edge_iterator out_edge_iterator;
  typedef graph_traits<Graph>::in_edge_iterator in_edge_iterator;
  typedef graph_traits<Graph>::edge_descriptor edge_t;

  int n = num_vertices(ga);
  int m = num_edges(ga);

  double* acc = new double[m];
  #pragma mta assert nodep
  for (int i = 0; i < m; i++)
  {
    acc[i] = 0;
  }

  vertex_id_map<Graph> vipm = get(_vertex_id_map, ga);
  edge_id_map<Graph> eipm = get(_edge_id_map, ga);
  vertex_iterator verts = vertices(ga);

  #pragma mta assert parallel
  #pragma mta loop future
  for (int i = 0; i < n; i++)
  {
    vertex_descriptor v = verts[i];

    int vid = get(vipm, v);
    int deg = out_degree(v, ga);

    out_edge_iterator inc_edgs = out_edges(v, ga);

    #pragma mta assert parallel
    for (int j = 0; j < deg; j++)
    {
      edge_t e = inc_edgs[j];

      int eid = get(eipm, e);
      int ind = 0;

      ind = ga.get_cross_index(eid);

      if (eid > m - 10  || ind > m - 10)
      {
        printf("cross ind of %d: %d\n", eid, ind);
      }

      double incr = vid * 0.2;
      double& atrg = acc[ind];

      atrg += incr;
    }
  }

  double* accum = new double[m];

  #pragma mta assert nodep
  for (int i = 0; i < m; i++) accum[i] = 0;
  vertex_iterator verts = vertices(ga);

  #pragma mta assert parallel
  #pragma mta loop future
  for (int i = 0; i < n; i++)
  {
    vertex_descriptor v = verts[i];

    double tmp_accum = 0.0;
    int vid = get(vipm, v);
    int deg = in_degree(v, ga);

    in_edge_iterator inc_edgs = in_edges(v, ga);

    #pragma mta assert parallel
    for (int j = 0; j < deg; j++)
    {
      edge_t e = inc_edgs[j];
      int eid = get(eipm, e);
      tmp_accum += acc[eid];
    }

    accum[vid] = tmp_accum;
  }

  if (n < 50)
  {
    for (int i = 0; i < n; i++)
    {
      printf("accum[%d]: %lf\n", i, accum[i]);
    }
  }
}

template <typename graph_adapter>
void get_high_low(graph_adapter& ga, int& n_hd, int& n_ld, int*& hd, int*& ld,
                  const int degreeThreshold)
{
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph_adapter>::vertex_iterator vertex_iter_t;

  int ord = num_vertices(ga);
  vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, ga);

  n_hd = 0;
  n_ld = 0;
  vertex_iterator verts = vertices(ga);

  #pragma mta assert parallel
  for (int i = 0; i < ord; i++)
  {
    vertex_t v = verts[i];
    int id = get(vid_map, v);

    if (out_degree(v, ga) > degreeThreshold)
    {
      n_hd++;
    }
    else
    {
      n_ld++;
    }
  }

  if (n_hd > 0) hd = new int[n_hd];
  if (n_ld > 0) ld = new int[n_ld];

  int hd_iter = 0, ld_iter = 0;
  vertex_iterator verts = vertices(ga);

  #pragma mta assert parallel
  for (int i = 0; i < ord; i++)
  {
    vertex_t v = verts[i];
    int id = get(vid_map, v);

    if (out_degree(v, ga) > degreeThreshold)
    {
      hd[mt_incr(hd_iter, 1)] = id;
    }
    else
    {
      ld[mt_incr(ld_iter, 1)] = id;
    }
  }

  printf("n_hd: %d, n_ld: %d, sum: %d, order: %d\n", n_hd, n_ld,
         (n_hd + n_ld), ord);
}

template <>
void get_high_low<static_graph_adapter<bidirectionalS> >(
    static_graph_adapter<bidirectionalS>& ga, int& n_hd, int& n_ld,
    int*& hd, int*& ld, const int degreeThreshold)
{
  typedef static_graph_adapter<bidirectionalS> Graph;
  typedef graph_traits<Graph>::vertex_descriptor vertex_t;
  typedef graph_traits<Graph>::vertex_iterator vertex_iter_t;

  int ord = num_vertices(ga);

  vertex_id_map<Graph> vid_map = get(_vertex_id_map, ga);

  n_hd = 0;
  n_ld = 0;
  vertex_iterator verts = vertices(ga);

  #pragma mta assert parallel
  for (int i = 0; i < ord; i++)
  {
    vertex_t v = verts[i];
    int id = get(vid_map, v);

    if (ga.get_degree(id) > degreeThreshold)
    {
      n_hd++;
    }
    else
    {
      n_ld++;
    }
  }

  if (n_hd > 0) hd = new int[n_hd];
  if (n_ld > 0) ld = new int[n_ld];

  int hd_iter = 0, ld_iter = 0;
  vertex_iterator verts = vertices(ga);

  #pragma mta assert parallel
  for (int i = 0; i < ord; i++)
  {
    vertex_t v = verts[i];
    int id = get(vid_map, v);

    if (ga.get_degree(id) > degreeThreshold)
    {
      hd[mt_incr(hd_iter, 1)] = id;
    }
    else
    {
      ld[mt_incr(ld_iter, 1)] = id;
    }
  }

  printf("n_hd_iter:%d, n_ld: %d, sum: %d, order: %d\n", n_hd, n_ld,
         (n_hd + n_ld), ord);
}

template <typename graph_adapter>
void get_in_high_low(graph_adapter& ga, int& n_hd, int& n_ld,
                     int*& hd, int*& ld, const int degreeThreshold)
{
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph_adapter>::vertex_iterator vertex_iter_t;

  int ord = num_vertices(ga);

  vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, ga);

  n_hd = 0;
  n_ld = 0;
  vertex_iterator verts = vertices(ga);

  #pragma mta assert parallel
  for (int i = 0; i < ord; i++)
  {
    vertex_t v = verts[i];
    int id = get(vid_map, v);

    if (ga.get_in_degree(id) > degreeThreshold)
    {
      n_hd++;
    }
    else
    {
      n_ld++;
    }
  }

  if (n_hd > 0) hd = new int[n_hd];
  if (n_ld > 0) ld = new int[n_ld];

  int hd_iter = 0, ld_iter = 0;
  vertex_iterator verts = vertices(ga);

  #pragma mta assert parallel
  for (int i = 0; i < ord; i++)
  {
    vertex_t v = verts[i];
    int id = get(vid_map, v);

    if (ga.get_in_degree(id) > degreeThreshold)
    {
      hd[mt_incr(hd_iter, 1)] = id;
    }
    else
    {
      ld[mt_incr(ld_iter, 1)] = id;
    }
  }

  printf("in_n_hd: %d, in_n_ld: %d, sum: %d, order: %d\n", n_hd, n_ld,
         (n_hd + n_ld), ord);
}

template <typename graph_adapter>
void run_with_high_low(graph_adapter& ga, const int threshold,
                       int n_hd, int*& hd, int n_ld, int*& ld,
                       int in_n_hd, int*& in_hd, int in_n_ld, int*& in_ld)
{
  typedef typename graph_traits<graph_adapter>::vertex_descriptor
          vertex_descriptor;
  typedef typename graph_traits<graph_adapter>::vertex_iterator vertex_iterator;
  typedef typename graph_traits<graph_adapter>::out_edge_iterator
          out_edge_iterator;

  int n = num_vertices(ga);
  int m = num_edges(ga);

  n_hd = 0, n_ld = 0;
  hd = NULL, ld = NULL;

  get_high_low<graph_adapter>(ga, n_hd, n_ld, hd, ld, threshold);

  double* accum = new double[m];
  #pragma mta assert nodep
  for (int i = 0; i < n; i++) accum[i] = 0;

  vertex_id_map<graph_adapter> vipm = get(_vertex_id_map, ga);
  edge_id_map<graph_adapter> eipm = get(_edge_id_map, ga);

  vertex_iterator verts = vertices(ga);

  for (int i = 0; i < n_hd; i++)
  {
    vertex_descriptor v = verts[hd[i]];

    int vid = get(vipm, v);
    int deg = out_degree(v, ga);

    out_edge_iterator inc_edgs = out_edges(v, ga);
    #pragma mta assert parallel
    for (int j = 0; j < deg; j++)
    {
      edge_t e = inc_edgs[j];
      vertex_descriptor trg = target(e, ga);
      int tid = get(vipm, trg);
      double incr = vid * 0.2;
      double& atrg = accum[tid];

      atrg += incr;
    }
  }

  mt_timer timer;
  int issues, memrefs, concur, streams, traps;
  init_mta_counters(timer, issues, memrefs, concur, streams);

  #pragma mta assert parallel
  for (int i = 0; i < n_ld; i++)
  {
    vertex_descriptor v = verts[ld[i]];

    int vid = get(vipm, v);
    int deg = out_degree(v, ga);

    out_edge_iterator inc_edgs = out_edges(v, ga);
    for (int j = 0; j < deg; j++)
    {
      edge_t e = inc_edgs[j];
      vertex_descriptor trg = target(e, ga);
      int tid = get(vipm, trg);
      double incr = vid * 0.2;
      double& atrg = accum[tid];

      atrg += incr;
    }
  }

  sample_mta_counters(timer, issues, memrefs, concur, streams);
  printf("STATS FOR FINAL LOOP\n");
  print_mta_counters(timer, m, issues, memrefs, concur, streams);

  if (n < 50)
  {
    for (int i = 0; i < n; i++)
    {
      printf("accum[%d]: %lf\n", i, accum[i]);
    }
  }
}

template <>
void run_with_high_low<static_graph_adapter<bidirectionalS> >
           (static_graph_adapter<bidirectionalS>& ga, const int threshold,
            int n_hd, int*& hd, int n_ld, int*& ld,
            int in_n_hd, int*& in_hd, int in_n_ld, int*& in_ld)
{
  typedef static_graph_adapter<bidirectionalS> Graph;
  typedef graph_traits<Graph>::vertex_descriptor vertex_descriptor;
  typedef graph_traits<Graph>::vertex_iterator vertex_iterator;
  typedef graph_traits<Graph>::out_edge_iterator out_edge_iterator;

  int n = num_vertices(ga);
  int m = num_edges(ga);

  double* acc = new double[m];
  #pragma mta assert nodep
  for (int i = 0; i < m; i++) acc[i] = 0;

  vertex_id_map<Graph> vipm = get(_vertex_id_map, ga);
  edge_id_map<Graph> eipm = get(_edge_id_map, ga);

  vertex_iterator verts = vertices(ga);

  for (int i = 0; i < n_hd; i++)
  {
    vertex_descriptor v = verts[hd[i]];

    int vid = get(vipm, v);
    int deg = out_degree(v, ga);

    out_edge_iterator inc_edgs = out_edges(v, ga);
    #pragma mta assert parallel
    for (int j = 0; j < deg; j++)
    {
      edge_t e = inc_edgs[j];
      int eid = get(eipm, e);
      int ind = ga.get_cross_index(eid);
      double incr = vid * 0.2;
      double& atrg = acc[ind];

      atrg += incr;
    }
  }

  #pragma mta assert parallel
  for (int i = 0; i < n_ld; i++)
  {
    vertex_descriptor v = verts[ld[i]];

    int vid = get(vipm, v);
    int deg = out_degree(v, ga);

    out_edge_iterator inc_edgs = out_edges(v, ga);
    for (int j = 0; j < deg; j++)
    {
      edge_t e = inc_edgs[j];
      int eid = get(eipm, e);
      int ind = ga.get_cross_index(eid);
      double incr = vid * 0.2;
      double& atrg = acc[ind];

      atrg += incr;
    }
  }

  double* accum = new double[m];
  #pragma mta assert nodep
  for (int i = 0; i < m; i++) accum[i] = 0;

  for (int i = 0; i < in_n_hd; i++)
  {
    vertex_descriptor v = verts[in_hd[i]];

    double tmp_accum = 0.0;
    int vid = get(vipm, v);
    int deg = in_degree(v, ga);

    in_edge_iterator inc_edgs = in_edges(v, ga);
    #pragma mta assert parallel
    for (int j = 0; j < deg; j++)
    {
      edge_t e = inc_edgs[j];
      int eid = get(eipm, e);
      tmp_accum += acc[eid];
    }
    accum[vid] = tmp_accum;
  }

  mt_timer timer;
  int issues, memrefs, concur, streams, traps;
  init_mta_counters(timer, issues, memrefs, concur, streams);

  #pragma mta assert parallel
  for (int i = 0; i < in_n_ld; i++)
  {
    vertex_descriptor v = verts[in_ld[i]];

    double tmp_accum = 0.0;
    int vid = get(vipm, v);
    int deg = in_degree(v, ga);

    in_edge_iterator inc_edgs = in_edges(v, ga);
    for (int j = 0; j < deg; j++)
    {
      edge_t e = inc_edgs[j];
      int eid = get(eipm, e);
      tmp_accum += acc[eid];
    }
    accum[vid] = tmp_accum;
  }

  sample_mta_counters(timer, issues, memrefs, concur, streams);
  printf("STATS FOR FINAL LOOP\n");
  print_mta_counters(timer, m, issues, memrefs, concur, streams);

  if (n < 50)
  {
    for (int i = 0; i < n; i++)
    {
      printf("accum[%d]: %lf\n", i, accum[i]);
    }
  }
}

template <typename graph_adapter>
void run_algorithm(graph_adapter& ga, int n_hd, int*& hd,
                   int n_ld, int*& ld, int in_n_hd, int*& in_hd,
                   int in_n_ld, int*& in_ld, int threshold)
{
  mt_timer timer;
  int issues, memrefs, concur, streams, traps;
  init_mta_counters(timer, issues, memrefs, concur, streams);

  printf("starting accumulation\n");
  run_without_high_low<graph_adapter>(ga);

  sample_mta_counters(timer, issues, memrefs, concur, streams);

  printf("\n============================="
         "\nRESULTS FOR ACCUMULATION WITHOUT HIGH LOW"
         "\n=============================\n");

  print_mta_counters(timer, num_edges(ga), issues, memrefs, concur, streams);

  printf("\n=============================\n");

  init_mta_counters(timer, issues, memrefs, concur, streams);

  printf("starting high low accumulation\n");
  run_with_high_low<graph_adapter>(ga, threshold, n_hd, hd, n_ld, ld,
                                   in_n_hd, in_hd, in_n_ld, in_ld);

  sample_mta_counters(timer, issues, memrefs, concur, streams);

  printf("\n\n============================="
         "\nRESULTS FOR ACCUMULATION WITH HIGH LOW"
         "\n=============================\n\n");

  print_mta_counters(timer, num_edges(ga), issues, memrefs, concur, streams);

  printf("\n=============================\n");
}

int main(int argc, char* argv[])
{
  if (argc < 3)
  {
    fprintf(stderr, "usage: %s [-debug] [-assort] <types> "
            "--graph_type <dimacs|cray> --level <levels> "
            "--graph <Cray graph number> "
            "--filename <dimacs graph filename> [<0..15>]\n", argv[0]);
    exit(1);
  }

  kernel_test_info kti;
  kti.process_args(argc, argv);

  static_graph_adapter<bidirectionalS> ga;

  int n_hd = 0;
  int n_ld = 0;
  int in_n_hd = 0;
  int in_n_ld = 0;
  int* hd = NULL;
  int* ld = NULL;
  int* in_hd = NULL;
  int* in_ld = NULL;

  kti.gen_graph(ga);

  printf("\n*********************************"
         "\nSTARTING BIDIRECTIONAL GRAPH TEST"
         "\n*********************************\n");

  get_high_low(ga, n_hd, n_ld, hd, ld, kti.threshold);
  get_in_high_low(ga, in_n_hd, in_n_ld, in_hd, in_ld, kti.threshold);

  run_algorithm(ga, n_hd, hd, n_ld, ld, in_n_hd, in_hd,
                in_n_ld, in_ld, kti.threshold);

  delete [] hd;
  delete [] ld;
  delete [] in_hd;
  delete [] in_ld;

  int n = num_vertices(ga);

  printf("v[n-2]: indeg: %d, outdeg: %d\n",
         out_degree(get_vertex(n - 2, ga), ga),
         in_degree(get_vertex(n - 2, ga), ga));

  printf("v[n-1]: indeg: %d, outdeg: %d\n",
         out_degree(get_vertex(n - 1, ga), ga),
         in_degree(get_vertex(n - 1, ga), ga));

  return 0;
}

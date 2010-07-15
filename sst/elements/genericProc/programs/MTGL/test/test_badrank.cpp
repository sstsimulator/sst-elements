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
/*! \file test_badrank.cpp

    \author Jon Berry (jberry@sandia.gov)

    \date 2/8/2008
*/
/****************************************************************************/

#define PUB_ALG

#include <cstdlib>

#include <mtgl/static_graph_adapter.hpp>
#include <mtgl/quicksort.hpp>
#include <mtgl/generate_rmat_graph.hpp>
#include <mtgl/breadth_first_search_mtgl.hpp>
#include <mtgl/visit_adj.hpp>
#include <mtgl/metrics.hpp>

using namespace mtgl;

typedef struct {
  int out_degree;
  int in_degree;
  double init;
  double acc;
  double rank;
} rank_info;

template <typename graph_adapter>
void compute_acc(graph_adapter& g, rank_info* rinfo)
{
  typedef graph_traits<graph_adapter>::vertex_descriptor vertex_descriptor_t;
  typedef graph_traits<graph_adapter>::adjacency_iterator adj_iterator_t;
  typedef graph_traits<graph_adapter>::vertex_iterator vertex_iterator_t;

  int i, j;
  int n = num_vertices(g);
  vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, g);
  vertex_iterator_t verts = vertices(g);

  #pragma mta assert parallel
  for (i = 0; i < n; i++)
  {
    vertex_descriptor_t v = verts[i];
    adj_iterator_t adjs = adjacent_vertices(v, g);
    int sid = get(vid_map, v);
    int deg = out_degree(v, g);

    #pragma mta assert parallel
    for (j = 0; j < deg; j++)
    {
      vertex_descriptor_t neighbor = adjs[j];
      int tid = get(vid_map, neighbor);
      double r = rinfo[tid].rank;
      rinfo[sid].acc += (r / (double) rinfo[tid].in_degree);
    }
  }
}

/// \brief Manual traversal using an MTGL adapter to access vertex and edge
///        information.
template <typename graph_adapter>
void compute_in_degree(graph_adapter& g, rank_info* rinfo)
{
  typedef graph_traits<graph_adapter>::vertex_descriptor vertex_descriptor_t;
  typedef graph_traits<graph_adapter>::adjacency_iterator adj_iterator_t;

  int i, j;
  int n = num_vertices(g);
  vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, g);
  vertex_iterator_t verts = vertices(g);

  #pragma mta assert parallel
  for (i = 0; i < n; i++)
  {
    vertex_descriptor_t v = verts[i];
    adj_iterator_t adjs = adjacent_vertices(v, g);
    int deg = out_degree(v, g);

    #pragma mta assert parallel
    for (j = 0; j < deg; j++)
    {
      vertex_descriptor_t neighbor = adjs[j];
      int nid = get(vid_map, neighbor);
      mt_incr(rinfo[nid].in_degree, 1);
    }
  }
}

int main(int argc, char* argv[])
{
  typedef static_graph_adapter<undirectedS> Graph;
  typedef graph_traits<graph_adapter>::vertex_iterator vertex_iterator_t;

  if (argc < 3)
  {
    fprintf(stderr, "usage: %s <p> <#bad> <delta>\n", argv[0]);
    fprintf(stderr, "(for 2^p vertex r-mat)\n");
    exit(1);
  }

  double dampen = 0.8;
  double delta = (double) atof(argv[3]);
  int num_bad = atoi(argv[2]);
  Graph g;
  generate_rmat_graph(g, atoi(argv[1]), 2);

  // degree seq
  int* degseq = degree_distribution(g);
  for (int i = 0; i < 32; i++) printf(" deg 2^%d: %d\n", i, degseq[i]);
  delete [] degseq;

  int order = num_vertices(g);
  if (order < 20) print(g);
  vertex_id_map<Graph> vid_map = get(_vertex_id_map, g);

  rank_info* rinfo = new rank_info[order];
  vertex_iterator_t vets = vertices(g);

  #pragma mta assert nodep
  for (int i = 0; i < order; i++)
  {
    rinfo[i].acc = 0.0;
    rinfo[i].out_degree = out_degree(verts[i], g);
    rinfo[i].in_degree = 0;

    if (i >= order - num_bad)
    {
      rinfo[i].init = 1.0;
      rinfo[i].rank = 1.0;
    }
    else
    {
      rinfo[i].rank = 0;
      rinfo[i].init = 0;
    }
  }

  compute_in_degree(g, rinfo);

  mt_timer timer;
  int issues, memrefs, concur, streams, traps;
  double maxdiff = 0;

  init_mta_counters(timer, issues, memrefs, concur, streams);

  do
  {
    compute_acc(g, rinfo);

    maxdiff = 0;

    #pragma mta assert nodep
    for (int i = 0; i < order; i++)
    {
      double oldval = rinfo[i].rank;
      double newval;
#ifdef PUB_ALG
      newval = rinfo[i].rank = rinfo[i].init +
                               dampen * rinfo[i].acc;
#else
      if (!rinfo[i].out_degree)
      {
        newval = rinfo[i].init;
      }
      else
      {
        newval = rinfo[i].rank = rinfo[i].init +
                                 dampen * rinfo[i].acc / rinfo[i].out_degree;
      }
#endif
      double absdiff = (oldval > newval) ? oldval - newval : newval - oldval;

      if (absdiff > maxdiff) maxdiff = absdiff;

      rinfo[i].acc = 0.0;
    }

  } while (maxdiff > delta);

  sample_mta_counters(timer, issues, memrefs, concur, streams);
  printf("---------------------------------------------\n");
  printf("Bad rank performance stats: \n");
  printf("---------------------------------------------\n");
  print_mta_counters(timer, num_edges(g), issues, memrefs, concur, streams);

  printf("---------------------------------------------\n");
  printf("Bad Guy Indegree: \n");
  printf("---------------------------------------------\n");

  for (int i = 1; i <= num_bad; i++ )
  {
    fprintf(stdout, "%d\n", rinfo[order - i].in_degree);
  }

  printf("---------------------------------------------\n");
  printf("Top 20 bad ranks: \n");
  printf("---------------------------------------------\n");

  int num_ranks = order - num_bad;
  if (num_ranks <= 0)
  {
    fprintf(stderr, "badness greater than order.. exit\n");
    exit(-1);
  }

  double* ranks = new double[num_ranks];
  int j = 0;

  #pragma mta assert nodep
  for (int i = 0; i < order; i++)
  {
    if (!rinfo[i].init)
    {
      int index = mt_incr(j, 1);
      ranks[index] = rinfo[i].rank;
    }
  }

  quicksort<double> qs(ranks, num_ranks);
  qs.run();

  for (int i = num_ranks - 20; i < num_ranks; i++) printf("%lf\n", ranks[i]);

  delete[] ranks;

  return 0;
}

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
/*! \file test_knn.cpp

    \author Jon Berry (jberry@sandia.gov)

    \date 12/4/2007
*/
/****************************************************************************/

#include <cassert>

#include <mtgl/util.hpp>
#include <mtgl/dynamic_array.hpp>
#include <mtgl/mtgl_adapter.hpp>
#include <mtgl/k_distance_neighborhood.hpp>
#include <mtgl/debug_utils.hpp>
#include <mtgl/static_graph_adapter.hpp>

#ifdef __MTA__
#include <sys/mta_task.h>
#include <machine/runtime.h>
#endif

#include "mtgl_test.hpp"

using namespace mtgl;

double get_freq()
{
#ifdef __MTA__
  double freq = mta_clock_freq();
#else
  double freq = 1000000;
#endif

  return(freq);
}

template <typename graph_adapter>
void test_knn(graph_adapter& ga, int v1, int v2, int v3, int v4, int k,
              const int dir)
{
  //FILE *OFP = fopen("graph.out","w");
  //print_graph_viz(OFP, ga);
  //fclose(OFP);

  mt_timer mttimer;
  dynamic_array<int> knn_vids;
  dynamic_array<int> src_vids;

  if (v1 != -1) src_vids.push_back(v1);
  if (v2 != -1) src_vids.push_back(v2);
  if (v3 != -1) src_vids.push_back(v3);
  if (v4 != -1) src_vids.push_back(v4);
  if (src_vids.size() == 0) src_vids.push_back(rand() % num_vertices(ga));

  printf("k param  = %d\n", k);
  printf("src_vids = "); src_vids.print();

#ifdef __MTA__
  int issues = mta_get_task_counter(RT_ISSUES);
  int memrefs = mta_get_task_counter(RT_MEMREFS);
  int m_nops = mta_get_task_counter(RT_M_NOP);
  int a_nops = mta_get_task_counter(RT_M_NOP);
  int fp_ops = mta_get_task_counter(RT_FLOAT_TOTAL);
#endif

  mttimer.start();

  if (dir == DIRECTED)
  {
    k_distance_neighborhood<graph_adapter, DIRECTED> knn(ga, k, knn_vids);
    knn.run(src_vids);
  }
  else if (dir == UNDIRECTED)
  {
    k_distance_neighborhood<graph_adapter, UNDIRECTED> knn(ga, k, knn_vids);
    knn.run(src_vids);
  }
  else if (dir == REVERSED)
  {
    k_distance_neighborhood<graph_adapter, REVERSED> knn(ga, k, knn_vids);
    knn.run(src_vids);
  }

  mttimer.stop();

#ifdef __MTA__
  issues = mta_get_task_counter(RT_ISSUES) - issues;
  memrefs = mta_get_task_counter(RT_MEMREFS) - memrefs;
  m_nops = mta_get_task_counter(RT_M_NOP)  - m_nops;
  a_nops = mta_get_task_counter(RT_A_NOP)  - a_nops;
  a_nops = mta_get_task_counter(RT_FLOAT_TOTAL) - fp_ops;
  fprintf(stderr, "issues: %d, memrefs: %d, fp_ops: %d, "
          "ints+muls: %d, memratio:%6.2f\n",
          issues, memrefs, fp_ops, (issues - a_nops - m_nops),
          memrefs / (double) issues);
#endif

  printf("(1) knn_vids = "); knn_vids.print();

#if 0
  // If you want to iterate the search to continue getting
  // new subsets iteratively, this code shows how to do it.
  int iters = 3;
  for (int i = 2; i <= iters; i++)
  {
    // say I wanted to iterate this twice
    src_vids.clear();
    src_vids = knn_vids;
    knn_vids.clear();
    knn.run(src_vids, false);
    printf("(%d) knn_vids = ", i); knn_vids.print();
  }
#endif

  long ticks = mttimer.getElapsedTicks();
  fprintf(stdout, "knn +%2d %7.4lf\n", k, ticks / get_freq());
}

int main(int argc, char* argv[])
{
  if (argc < 5)
  {
    fprintf(stderr, "usage: %s [-debug] "
            " [-knn] "
            " --graph_type <dimacs|cray>\n"
            "\t\t--level <levels>"
            " --graph <Cray graph number> [<0..15>]\n"
            "\t\t--filename <dimacs graph filename>\n"
            "\t\t--graph_struct <dyn|csr>\n"
            "\t\t[--v1 vid] [--v2 vid] [--v3 vid] [--v4 vid]"
            " [--k n]\n"
            "\t\t[--direction <directed|undirected|reversed>\n"
            , argv[0]);
    exit(1);
  }

  typedef static_graph_adapter<directedS> Graph;
  Graph ga;

  kernel_test_info kti;
  kti.process_args(argc, argv);
  kti.gen_graph(ga);

  if (find(kti.algs, kti.algCount, "knn"))
  {
    int v1 = kti.v1;
    int v2 = kti.v2;
    int v3 = kti.v3;
    int v4 = kti.v4;
    int k  = kti.k_param;
    int dir = kti.direction;
    test_knn<Graph>(ga, v1, v2, v3, v4, k, dir);
  }
  else
  {
    fprintf(stderr, "error: missing -knn argument\n");
  }

  return(0);
}

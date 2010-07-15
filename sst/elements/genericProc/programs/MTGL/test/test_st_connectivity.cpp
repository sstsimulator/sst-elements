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
/*! \file test_st_connectivity.cpp

    \author Jon Berry (jberry@sandia.gov)

    \date 12/4/2007
*/
/****************************************************************************/

#include <cassert>

#include <mtgl/util.hpp>
#include <mtgl/mtgl_adapter.hpp>
#include <mtgl/st_connectivity.hpp>
#include <mtgl/debug_utils.hpp>
#include <mtgl/static_graph_adapter.hpp>

#include "mtgl_test.hpp"

#define TEST_BFS 1

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
void test_st_connectivity(graph_adapter& ga, int vs, int vt, const int dir)
{
  typedef typename graph_traits<graph_adapter>::size_type size_type;

  if (num_vertices(ga) < 100) print_graph(ga);

  printf("   vs: %d\n", vs);
  printf("   vt: %d\n", vt);

  if (vs == -1)
  {
    vs = rand() % num_vertices(ga);
    printf("vs = %d\n", vs);
  }

  if (vt == -1)
  {
    do
    {
      vt = rand() % num_vertices(ga);
    } while (vt == vs);

    printf("vt = %d\n", vt);
  }

#ifdef __MTA__
  int issues = mta_get_task_counter(RT_ISSUES);
  int memrefs = mta_get_task_counter(RT_MEMREFS);
  int m_nops = mta_get_task_counter(RT_M_NOP);
  int a_nops = mta_get_task_counter(RT_M_NOP);
  int fp_ops = mta_get_task_counter(RT_FLOAT_TOTAL);
#endif

  mt_timer mttimer;
  mttimer.start();

  size_type distance, num_visited;

  if (dir == DIRECTED)
  {
    st_connectivity<graph_adapter, DIRECTED>
    sts(ga, vs, vt, distance, num_visited);
    sts.run();
  }
  else if (dir == UNDIRECTED)
  {
    st_connectivity<graph_adapter, UNDIRECTED>
    sts(ga, vs, vt, distance, num_visited);
    sts.run();
  }
  else if (dir == REVERSED)
  {
    st_connectivity<graph_adapter, REVERSED>
    sts(ga, vs, vt, distance, num_visited);
    sts.run();
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

  printf("distance    = %d\n", distance);
  printf("num_visited = %d\n", num_visited);

  long ticks = mttimer.getElapsedTicks();
  fprintf(stdout, "st %9d -> %-9d %7.4lf %3d\n",
          vs, vt, ticks / get_freq(), distance);
}

int main(int argc, char* argv[])
{
  if (argc < 5)
  {
    fprintf(stderr, "usage: %s [-debug] "
            " --graph_type <dimacs|cray> "
            " --level <levels>\n"
            "\t\t--graph <Cray graph number> "
            " --filename <dimacs graph filename> "
            " [<0..15>]\n"
            "\t\t[--vs <-1|n>] [--vt <-1|n>]\n"
            "\t\t--direction <directed|undirected|reversed>\n"
            , argv[0]);
    exit(1);
  }

  typedef static_graph_adapter<directedS> Graph;
  Graph ga;

  kernel_test_info kti;
  kti.process_args(argc, argv);
  kti.gen_graph(ga);

  test_st_connectivity<Graph>(ga, kti.vs, kti.vt, kti.direction);

  return(0);
}

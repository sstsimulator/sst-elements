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
/*! \file test_independent_set.cpp

    \author Jon Berry (jberry@sandia.gov)

    \date 12/4/2007
*/
/****************************************************************************/

#include "mtgl_test.hpp"

#include <mtgl/util.hpp>
#include <mtgl/independent_set.hpp>
#include <mtgl/static_graph_adapter.hpp>

using namespace mtgl;

double get_freq()
{
#ifdef __MTA__
  double freq = mta_clock_freq();
#else
  double freq = 1000000;
#endif
  return freq;
}

template <typename graph, typename result_type>
void test_maximal_independent_set(graph& g, result_type& result)
{
  typedef graph_traits<graph>::size_type size_type;

  size_type order = num_vertices(g);
  size_type size = num_edges(g);

  result = (bool*) malloc(order * sizeof(bool));
  bool* active = (bool*) malloc(order * sizeof(bool));
  int* sums = (int*) malloc(order * sizeof(int));

  #pragma mta assert nodep
  for (size_type i = 0; i < order; ++i)
  {
    result[i] = true;
    active[i] = true;
  }

#ifdef __MTA__
  mta_resume_event_logging();
  int issues = mta_get_task_counter(RT_ISSUES);
  int memrefs = mta_get_task_counter(RT_MEMREFS);
  int m_nops = mta_get_task_counter(RT_M_NOP);
  int a_nops = mta_get_task_counter(RT_M_NOP);
  int fp_ops = mta_get_task_counter(RT_FLOAT_TOTAL);
  int traps  = mta_get_task_counter(RT_TRAP);
#endif

  bool unsv[] = { true, true, true, true, true, true };
  bool unse[] = { true, true, true, true, true, true };

  mt_timer mttimer;
  mttimer.start();

  int issize = 0;
  maximal_independent_set<graph, result_type> mis(g, unsv, unse);
  mis.run(result, issize);

  mttimer.stop();
  double seconds = mttimer.getElapsedSeconds();

#ifdef __MTA__
  issues = mta_get_task_counter(RT_ISSUES) - issues;
  memrefs = mta_get_task_counter(RT_MEMREFS) - memrefs;
  m_nops = mta_get_task_counter(RT_M_NOP)  - m_nops;
  a_nops = mta_get_task_counter(RT_A_NOP)  - a_nops;
  //a_nops = mta_get_task_counter(RT_FLOAT_TOTAL) - fp_ops;
  traps  = mta_get_task_counter(RT_TRAP);
  printf("memref/edge: (%lf)\n", memrefs / (double) size);
  printf("issues/edge: (%lf)\n", issues / (double) size);
  printf("traps/edge: (%lf)\n", traps / (double) size);
  printf("traps: (%d)\n", traps);
  printf("time: (%lf)s\n", seconds);
#endif

  prefix_sums<bool, int>(result, sums, order);

  fprintf(stderr, "RESULT: independent_set %d (%6.2lf,0): %d\n",
          size, seconds, sums[order - 1]);
}

template <typename graph>
void test_independent_set(graph& g, bool*& result)
{
  typedef graph_traits<graph>::size_type size_type;

  size_type order = num_vertices(g);
  size_type size = num_edges(g);

  result = (bool*) malloc(order * sizeof(bool));
  bool* active = (bool*) malloc(order * sizeof(bool));
  int* sums = (int*) malloc(order * sizeof(int));

  #pragma mta assert nodep
  for (size_type i = 0; i < order; ++i)
  {
    result[i] = true;
    active[i] = true;
  }

#ifdef __MTA__
  mta_resume_event_logging();
  int issues = mta_get_task_counter(RT_ISSUES);
  int memrefs = mta_get_task_counter(RT_MEMREFS);
  int m_nops = mta_get_task_counter(RT_M_NOP);
  int a_nops = mta_get_task_counter(RT_M_NOP);
  int fp_ops = mta_get_task_counter(RT_FLOAT_TOTAL);
  int traps  = mta_get_task_counter(RT_TRAP);
#endif

  bool unsv[] = { true, true, true, true, true, true };
  bool unse[] = { true, true, true, true, true, true };

  mt_timer mttimer;
  mttimer.start();

  int issize = 0;
  independent_set<graph, bool*> is(g, unsv, unse);
  is.run(result, issize);

  mttimer.stop();
  double seconds = mttimer.getElapsedSeconds();

  for (int i = 0; i < order; i++) printf("is[%d]: %d\n", i, result[i]);

#ifdef __MTA__
  issues = mta_get_task_counter(RT_ISSUES) - issues;
  memrefs = mta_get_task_counter(RT_MEMREFS) - memrefs;
  m_nops = mta_get_task_counter(RT_M_NOP)  - m_nops;
  a_nops = mta_get_task_counter(RT_A_NOP)  - a_nops;
  //a_nops = mta_get_task_counter(RT_FLOAT_TOTAL) - fp_ops;
  traps  = mta_get_task_counter(RT_TRAP);
  printf("memref/edge: (%lf)\n", memrefs / (double) size);
  printf("issues/edge: (%lf)\n", issues / (double) size);
  printf("traps/edge: (%lf)\n", traps / (double) size);
  printf("traps: (%d\n", traps);
#endif

  prefix_sums<bool, int>(result, sums, order);

  fprintf(stderr, "RESULT: independent_set %d (%6.2lf,0): %d\n",
          size, seconds, sums[order - 1]);
}

int main(int argc, char* argv[])
{
  //mta_suspend_event_logging();
  if (argc < 5)
  {
    fprintf(stderr, "usage: %s [-debug] "
            " [-is | -mis] <types> "
            " --graph_type <dimacs|cray> "
            " --level <levels> --graph <Cray graph number> "
            " --filename <dimacs graph filename> "
            " [<0..15>]\n"
            , argv[0]);
    exit(1);
  }

  static_graph_adapter<directedS> ga;

  kernel_test_info kti;
  kti.process_args(argc, argv);
  kti.gen_graph(ga);

  if (find(kti.algs, kti.algCount, "mis"))
  {
    if (kti.graph_type != MMAP)
    {
      test_maximal_independent_set<graph_adapter>(ga, result);
    }
  }
  else if (find(kti.algs, kti.algCount, "is"))
  {
    if (kti.graph_type != MMAP)
    {
      test_independent_set<graph_adapter>(ga, result);
    }
  }

  return 0;
}

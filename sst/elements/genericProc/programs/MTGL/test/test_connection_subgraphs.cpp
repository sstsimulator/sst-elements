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
/*! \file test_connection_subgraphs.cpp

    \author Jon Berry (jberry@sandia.gov)

    \date 12/4/2007
*/
/****************************************************************************/

#include <list>
#include <cmath>

#include "mtgl_test.hpp"

#include <mtgl/util.hpp>
#include <mtgl/static_graph_adapter.hpp>
#include <mtgl/connection_subgraphs.hpp>
#include <mtgl/rand_fill.hpp>
#include <mtgl/dynamic_array.hpp>

using namespace mtgl;

#define ASSERT 1

#ifdef __MTA__
  #define BEGIN_LOGGING()                                              \
    mta_resume_event_logging();                                        \
    int issues      = mta_get_task_counter(RT_ISSUES);                 \
    int streams     = mta_get_task_counter(RT_STREAMS);                \
    int concurrency = mta_get_task_counter(RT_CONCURRENCY);            \
    int traps       = mta_get_task_counter(RT_TRAP);

  #define END_LOGGING()                                                \
  {                                                                    \
    issues      = mta_get_task_counter(RT_ISSUES) - issues;            \
    streams     = mta_get_task_counter(RT_STREAMS) - streams;          \
    concurrency = mta_get_task_counter(RT_CONCURRENCY) - concurrency;  \
    traps       = mta_get_task_counter(RT_TRAP) - traps;               \
    mta_suspend_event_logging();                                       \
    printf("issues: %d, streams: %lf, "                                \
           "concurrency: %lf, "                                        \
           "traps: %d\n",                                              \
           issues,                                                     \
           streams / (double) issues,                                  \
           concurrency / (double) issues,                              \
           traps);                                                     \
  }

#else
  #define BEGIN_LOGGING() ;
  #define END_LOGGING()   ;
#endif

template <typename T>
void generate_range(T min, T max, T** Wt, int sz)
{
  assert(max > min);
  random_value maxWt = 0;
  T* wgts  = (T*) malloc(sizeof(T) * sz);
  random_value* rvals = (random_value*) malloc(sizeof(random_value) * sz);

  rand_fill::generate(sz, rvals);

  #pragma mta assert nodep
  for (int i = 0; i < sz; i++)
  {
    if (min == 0)
    {
      wgts[i] = rvals[i] % (max + 1);
    }
    else
    {
      wgts[i] = rvals[i] % (max - min) + min;
    }
  }

  free(rvals);
  rvals = NULL;
  *Wt = wgts;
}

template <typename graph>
double run_csg_search(graph& ga, typename graph_traits<graph>::size_type vs,
                      typename graph_traits<graph>::size_type vt,
                      typename graph_traits<graph>::size_type expansions)
{
  typedef typename graph_traits<graph>::size_type size_type;
  mt_timer mttimer;
  dynamic_array<size_type> vidList;
  size_type distPastShortestPath = 3;  // Only look 3 hops beyond shortest path.
  double penaltyMultiplier = 1.0;

  BEGIN_LOGGING();
  mttimer.start();
  connection_subgraph<graph> csg(ga, vs, vt, vidList, distPastShortestPath,
                                 expansions, penaltyMultiplier);
  printf("in run: calling csg.run\n");
  csg.run();
  mttimer.stop();
  END_LOGGING();

  for (size_type i = 0; i < vidList.size(); i++) printf("%d ", (int)vidList[i]);
  printf("\n");

  double the_time = mttimer.getElapsedSeconds();

  printf("RESULT:\tsrcV: %-8d  trgV: %-8d   sz: %-4d time: %8.3lf\n",
         (int)vs, (int)vt, (int)vidList.size(), the_time);

  return(the_time);
}

template <typename graph>
void test_csg_search(graph& ga, typename graph_traits<graph>::size_type vs,
                     typename graph_traits<graph>::size_type vt,
                     typename graph_traits<graph>::size_type trgSize,
                     typename graph_traits<graph>::size_type nSrcs = 1,
                     typename graph_traits<graph>::size_type nTrials = 1)
{
  typedef typename graph_traits<graph>::size_type size_type;
  double the_time = 0.0;
  size_type* srcs = NULL;
  printf("in csg_search\n");

  srand48(3324545);
  generate_range<size_type>(0, num_vertices(ga) - 1, &srcs, nSrcs);

  if (vs != -1)
  {
    nSrcs   = 1;
    srcs[0] = vs;
  }

  for (size_type i = 0; i < nSrcs; i++)
  {
    printf("srcs[i]: %d\n", (int)srcs[i]);
    fflush(stdout);

    for (size_type j = 0; j < nTrials; j++)
    {
      printf("in csg_search: calling run\n");
      the_time += run_csg_search<graph>(ga, srcs[i], vt, trgSize);
    }

    printf("\n");
  }

  printf("Avg time: %8.3lf\n", the_time / (nTrials * nSrcs));

  free(srcs);
}

int main(int argc, char* argv[])
{
  typedef static_graph_adapter<undirectedS> Graph;

  if (argc < 5)
  {
    fprintf(stderr, "usage: %s [-debug] "
            " [-csg] "
            " --graph_type <dimacs|rmat>"
            " --graph_struct <dyn|static>"
            " --level <levels> --graph <SCALE>"
            " --filename <dimacs graph filename>"
            " --vs <vsource>"
            " --vt <vtarget>"
            " --target_size <# of expansions>"
            "\n" , argv[0]);
    exit(1);
  }

  kernel_test_info kti;
  kti.process_args(argc, argv);

  mt_timer mttimer;

  printf("Generating graph:\n");
  fflush(stdout);

  Graph ga;

  mttimer.start();
  kti.gen_graph(ga);
  mttimer.stop();

  printf("Graph generated in %.3f seconds.\n", mttimer.getElapsedSeconds());
  fflush(stdout);

  printf("nVerts = %lu\n", num_vertices(ga));
  printf("nEdges = %lu\n", num_edges(ga));
  fflush(stdout);

  if (find(kti.algs, kti.algCount, "csg"))
  {
    test_csg_search<Graph>(ga, kti.vs, kti.vt, kti.target_size,
                           kti.num_srcs, kti.trials);
  }
  else
  {
    printf("Did you forget to add -csg?\n");
  }
}

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
/*! \file strongly_connected_components_dfs.hpp

    \brief This code contains the scc() routine that invokes visitors defined
           in scc_visitor.hpp.

    \author Jon Berry (jberry@sandia.gov)

    \date 3/2/2005

    Strongly connected components algorithms:
      * strongly_connected_components_dfs:
          experimentatal algorithm for graphs without extremely large scc's.
          Uses Cormen, Leiserson, Rivest dfs-based algorithm.
*/
/****************************************************************************/

#ifndef MTGL_STRONGLY_CONNECTED_COMPONENTS_DFS_HPP
#define MTGL_STRONGLY_CONNECTED_COMPONENTS_DFS_HPP

#include <cstdio>
#include <climits>
#include <cassert>

#include <mtgl/util.hpp>
#include <mtgl/scc_visitor.hpp>
#include <mtgl/depth_first_search.hpp>
#include <mtgl/topsort.hpp>

#ifdef __MTA__
#include <sys/mta_task.h>
#include <machine/runtime.h>
#endif

namespace mtgl {

template <typename graph>
class strongly_connected_components_dfs {
public:
  typedef typename graph_traits<graph>::size_type size_type;

  strongly_connected_components_dfs(graph* gg, int* res, int* subprob = 0) :
    g(gg), result(res), remainder(subprob) {}

  int run()
  {
    printf("strongly_connected_components_dfs\n");

    mt_timer mttimer;
    mttimer.start();

#ifdef __MTA__
    mta_resume_event_logging();
    int issues = mta_get_task_counter(RT_ISSUES);
    int memrefs = mta_get_task_counter(RT_MEMREFS);
    int m_nops = mta_get_task_counter(RT_M_NOP);
    int a_nops = mta_get_task_counter(RT_M_NOP);
    int fp_ops = mta_get_task_counter(RT_FLOAT_TOTAL);
#endif

    size_type order = num_vertices(*g);
    int* start_time = (int*) malloc(sizeof(int) * order);
    int* finish_time = (int*) malloc(sizeof(int) * order);
    int* psearch_parent = (int*) malloc(sizeof(int) * order);
    int* remainder_copy = (int*) malloc(sizeof(int) * order);

    #pragma mta assert nodep
    for (size_type i = 0; i < order; i++)
    {
      remainder[i] = 1;
      start_time[i] = 0;
      finish_time[i] = 0;
      remainder_copy[i] = remainder[i];
    }


    dfs_event_times<graph> det(g, start_time, finish_time, 10000,
                               remainder, 1, 0);
    det.run();

    int num_scc = 0;
    int numtriv = 0;

    #pragma mta assert nodep
    for (size_type i = 0; i < order; i++)
    {
      if (remainder[i] == 0)
      {
        num_scc++;
        numtriv++;
        result[i] = i;
      }
    }

    int* searchColor = (int*) malloc(sizeof(int) * order);
    #pragma mta assert nodep
    for (size_type i = 0; i < order; i++) searchColor[i] = 0;

    scc_labeling_visitor<graph> news_vis(finish_time, remainder, result, g);

    bool done = false;
    while (!done)
    {
      int next = -1;
      int maxfin = 0;

      #pragma mta assert nodep
      for (size_type i = 0; i < order; i++)
      {
        int val = remainder[i] * finish_time[i];
        if (val > maxfin)
        {
          maxfin = val;
          next = i;
        }
      }

      if (next == -1) break;

      printf("%dth scc leader: %d: ft = %d\n", num_scc,
             next, finish_time[next]);

      finish_time[next] = 0;
      result[next] = next;

      psearch<graph, int*, scc_labeling_visitor<graph>, AND_FILTER, REVERSED>
        psrch(g, searchColor, news_vis);
      psrch.run(g->vertices[next]);

      num_scc++;
    }

    printf("num_scc: %d\n", num_scc);

    mttimer.stop();
    int postticks = mttimer.getElapsedTicks();

#ifdef __MTA__
    issues = mta_get_task_counter(RT_ISSUES) - issues;
    memrefs = mta_get_task_counter(RT_MEMREFS) - memrefs;
    m_nops = mta_get_task_counter(RT_M_NOP)  - m_nops;
    a_nops = mta_get_task_counter(RT_A_NOP)  - a_nops;
    a_nops = mta_get_task_counter(RT_FLOAT_TOTAL) - fp_ops;
#endif

    free(start_time);
    free(finish_time);
    free(psearch_parent);

    return postticks;
  }

private:
  graph* g;
  int* result;
  int* remainder;
};

}

#endif

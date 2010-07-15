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
/*! \file st_subgraph_search.hpp

    \brief S-T search implementation.

    \author William McLendon (wcmclen@sandia.gov)

    \date 1/5/2007
*/
/****************************************************************************/

#ifndef MTGL_ST_SUBGRAPH_SEARCH_HPP
#define MTGL_ST_SUBGRAPH_SEARCH_HPP

#include <cstdio>
#include <cstdlib>
#include <climits>
#include <cstring>

#include <mtgl/graph.hpp>
#include <mtgl/copier.hpp>
#include <mtgl/util.hpp>
#include <mtgl/dynamic_array.hpp>
#include <mtgl/st_search.hpp>
#include <mtgl/psearch.hpp>

#ifdef __MTA__
  #include <sys/mta_task.h>
  #include <machine/runtime.h>
#endif

#define TIMEIT 0

namespace mtgl {

void __foo__(void) { printf("__foo__\n"); }

/// \brief Not a simple s-t connectivity computation (see st_connectivity.hpp
///        for that).  This code records the subgraph of breadth-first
///        shortest paths connecting s and t.
class st_subgraph_search {
public:
  st_subgraph_search(graph* g_, int vs_, int vt_, int& distance_,
                     int& numVisited_, dynamic_array<int>& shortPathVerts_,
                     int& ticks_) :
    g(g_), vs(vs_), vt(vt_), distance(distance_), numVisited(numVisited_),
    shortPathVerts(shortPathVerts_), ticks(ticks_) {}

  int run()
  {
    double freq = 1000000;
    int sp_found  = 0;
    unsigned int nVerts = num_vertices(*g);
    int nedges = num_edges(*g);
    int sp_length = 0;

    mt_timer mttimer;
    mttimer.start();

#ifdef __MTA__
    int issues  = mta_get_task_counter(RT_ISSUES);
    int memrefs = mta_get_task_counter(RT_MEMREFS);
    int m_nops  = mta_get_task_counter(RT_M_NOP);
    int a_nops  = mta_get_task_counter(RT_M_NOP);
    int fp_ops  = mta_get_task_counter(RT_FLOAT_TOTAL);

    freq = mta_clock_freq();
    mta_resume_event_logging();
#endif

    // -------------------------------------------------------------
    // --- Start Discovery Phase ---
    // -------------------------------------------------------------
    int level_fwd = 0;
    int level_bwd = 0;
    int sp_exists = 0;

    if (vs == vt)
    {
      printf("vs and vt are the same vertex\n");
      return(0);
    }

    // Allocate and initialize the distance vectors
    int* dvs        = (int*) malloc(sizeof(int) * nVerts);
    int* dvt        = (int*) malloc(sizeof(int) * nVerts);
    int* color_map  = (int*) malloc(sizeof(int) * nVerts);
    int* meetptMask = (int*) malloc(sizeof(int) * nVerts);
    int* rec_colors = (int*) malloc(sizeof(int) * nVerts);

    #pragma mta assert nodep
    #pragma mta block schedule
    for (int i = 0; i < nVerts; i++)
    {
      dvs[i] = UNKNOWN;
      dvt[i] = UNKNOWN;
      color_map[i]  = 0;
      meetptMask[i] = 0;
      rec_colors[i] = COLOR_BLACK;
    }

    dvs[vs] = 0;
    dvt[vt] = 0;
    color_map[vs] = 1;

#if TIMEIT
    mttimer.stop();
    printf("\tINIT: %.4f s\n", mttimer.getElapsedTicks() / freq);
    mttimer.start();
#endif

    // create short-path visitor
    vis_discover spv_fwd(vs, vt, nVerts, &sp_found,  DIR_FWD,
                         &level_fwd, &level_bwd, dvs, dvt,
                         meetptMask, rec_colors);

    vis_discover spv_bwd(vs, vt, nVerts, &sp_found, DIR_BWD,
                         &level_fwd, &level_bwd, dvs, dvt,
                         meetptMask, rec_colors);

    // Instantiate breadth_first_search objects for fwd and bwd searches
    breadth_first_search<int*, vis_discover> bfs_fwd(g, color_map, spv_fwd);
    breadth_first_search<int*, vis_discover> bfs_bwd(g, color_map, spv_bwd);

    bool expand_fwd = true;            // T if we expand from fwd dir
    int q_start;                       // starting index of queue
    int q_end;                         // ending index of queue
    int frontier_fsize = 0;            // size of fwd frontier
    int frontier_bsize = 0;            // size of bwd frontier

    // expand vs by 1 level
    bfs_fwd.run(vs);
    level_fwd = level_fwd + 1;
    distance  = distance + 1;

    // If vt got visited we're done.
    if (dvs[vt] != UNKNOWN) sp_found = 1;

    // If we're still going, expand vt by 1 level
    if (!sp_found)
    {
      color_map[vt] = 1;
      bfs_bwd.run(vt);
      level_bwd = level_bwd + 1;
      distance  = distance + 1;

      // Seed the search loop counters; expand the smallest frontier
      if (bfs_fwd.count <= bfs_bwd.count)
      {
        // Expand FWD next iteration
        expand_fwd = true;
        q_start = 0;
        q_end = bfs_fwd.count;
        frontier_bsize = bfs_bwd.count;
      }
      else
      {
        // Expand BWD next iteration
        expand_fwd = false;
        q_start = 0;
        q_end = bfs_bwd.count;
        frontier_fsize = bfs_fwd.count;
      }
    }

    int* ptr_Queue;
    while ( !sp_found && (q_end - q_start) > 0)
    {
      if (expand_fwd)
      {
        ptr_Queue = bfs_fwd.Q.Q;

        #pragma mta assert parallel
        #pragma mta interleave schedule
        #pragma mta loop future
        for (int vi = q_start; vi < q_end; vi++) bfs_fwd.run(ptr_Queue[vi]);

        frontier_fsize = bfs_fwd.count - q_end;

#ifdef __MTA__
        int_fetch_add(&level_fwd, 1);
#else
        level_fwd++;
#endif
      }
      else
      {
        ptr_Queue = bfs_bwd.Q.Q;

        #pragma mta assert parallel
        #pragma mta interleave schedule
        #pragma mta loop future
        for (int vi = q_start; vi < q_end; vi++) bfs_bwd.run(ptr_Queue[vi]);

        frontier_bsize = bfs_bwd.count - q_end;
        level_bwd++;
      }
      distance++;

      // Config next iter to expand f/m the smallest frontier.
      if ( frontier_fsize <= frontier_bsize )
      {
        expand_fwd = true;
        q_start = bfs_fwd.count - frontier_fsize;
        q_end   = bfs_fwd.count;
      }
      else
      {
        expand_fwd = false;
        q_start = bfs_bwd.count - frontier_bsize;
        q_end   = bfs_bwd.count;
      }
    }

#if TIMEIT
    mttimer.stop();
    printf("\tDISC: %.4f s\n", mttimer.getElapsedTicks() / freq);
    mttimer.start();
#endif

    if (sp_found)
    {
      // ------------------------------------------------------------
      // --- Start Recording Phase ---
      // ------------------------------------------------------------
      #pragma mta trace "ST-Recording"

      sp_length = level_fwd + level_bwd;

#if 0    // Debugging
      print_array_fltr(dvs, nVerts, " dvs", UNKNOWN);
      print_array_fltr(dvt, nVerts, " dvt", UNKNOWN);
      print_array_fltr(meetptMask, nVerts, "mask", 0);
      print_array_fltr(rec_colors, nVerts, "colr", COLOR_BLACK);
#endif

      vis_recorder rvis(dvs, dvt, sp_length, &shortPathVerts, rec_colors);

      #pragma mta assert parallel
      #pragma mta interleave schedule
      for (int vi = 0; vi < nVerts; vi++)
      {
        if ( meetptMask[vi] )
        {
          vertex* vv = g->get_vertex(vi);
          shortPathVerts.push_back(vi);
          psearch<int*, vis_recorder, PURE_FILTER, UNDIRECTED>
          psrch(g, rec_colors, rvis);
          psrch.run(vv);
        }
      }
    }

#if TIMEIT
    mttimer.stop();
    printf("\tRCRD: %.4f s\n", mttimer.getElapsedTicks() / freq);
    mttimer.start();
#endif

    distance = sp_length;

    free(meetptMask);  meetptMask = NULL;
    free(rec_colors);  rec_colors = NULL;
    free(dvs);         dvs = NULL;
    free(dvt);         dvt = NULL;
    free(color_map);   color_map = NULL;

#if TIMEIT
    mttimer.stop();
    printf("\tFREE: %.4f s\n", mttimer.getElapsedTicks() / freq);
    mttimer.start();
#endif

#ifdef __MTA__
    mta_suspend_event_logging();
    issues  = mta_get_task_counter(RT_ISSUES) - issues;
    memrefs = mta_get_task_counter(RT_MEMREFS) - memrefs;
    m_nops  = mta_get_task_counter(RT_M_NOP) - m_nops;
    a_nops  = mta_get_task_counter(RT_A_NOP) - a_nops;
#endif

    mttimer.stop();
    ticks = mttimer.getElapsedTicks();

    return(sp_length);
  }

private:
  graph* g;
  int vs;
  int vt;
  int& distance;
  int& numVisited;
  dynamic_array<int>& shortPathVerts;
  int& ticks;
};

}

#endif

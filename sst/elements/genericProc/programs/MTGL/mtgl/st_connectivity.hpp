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
/*! \file st_connectivity.hpp

    \brief Implementation of David Bader's Shortest Path Algorithm 2
           (breadth_first_searches from either s or t side) in Strawman.

    \author Kamesh Madduri

    \date 7/2005
*/
/****************************************************************************/

#ifndef MTGL_ST_CONNECTIVITY_HPP
#define MTGL_ST_CONNECTIVITY_HPP

#include <cstdio>
#include <climits>

#include <mtgl/util.hpp>
#include <mtgl/breadth_first_search.hpp>
#include <mtgl/mtgl_adapter.hpp>
#include <mtgl/graph_traits.hpp>
#include <mtgl/connectivity_visitor.hpp>

#ifdef __MTA__
#include <sys/mta_task.h>
#include <machine/runtime.h>
#endif

namespace mtgl {

/*! \brief Implements an s-t connectivity search.  This determines whether
           or not a path exists between two vertices.  If a path does exist
           the shortest distance between them is returned as well as the
           number of vertices visited during the search.
 */
template <typename graph_adapter, int dir = DIRECTED>
class st_connectivity {
public:
  typedef typename graph_traits<graph_adapter>::size_type size_type;

  /*! \brief Constructor.

      \param g (IN) Pointer to a graph_adapter object.
      \param s (IN) Source vertex id.
      \param t (IN) Target vertex id.
      \param distance (OUT) Returns the distance from s to t.
                      0 if s and t are the same vertex.
                      -1 if there is no path between s and t.
      \param numVisited (OUT) Returns the number of vertices visited
                        during the search.
  */
  st_connectivity(graph_adapter& ga_, size_type s_, size_type t_,
                  size_type& distance_, size_type& numVisited_) :
    ga(ga_), s(s_), t(t_), distance(distance_), numVisited(numVisited_) {}


  /// Executes the st_connectivity search.
  size_type run()
  {
    size_type start =  (size_type) 0, end = (size_type) 0;
    size_type extentS = (size_type) 0, extentT = (size_type) 0;

    size_type doS  = 0;
    size_type doT  = 0;
    distance = 0;

    if (s == t)
    {
      printf("Same vertex\n");
      return 0;
    }

    mt_timer mttimer;
    mttimer.start();

#ifdef __MTA__
    size_type issues = mta_get_task_counter(RT_ISSUES);
    size_type memrefs = mta_get_task_counter(RT_MEMREFS);
    size_type m_nops = mta_get_task_counter(RT_M_NOP);
    size_type a_nops = mta_get_task_counter(RT_M_NOP);
    size_type fp_ops = mta_get_task_counter(RT_FLOAT_TOTAL);
#endif

    vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, ga);

    size_type done;
    size_type n = num_vertices(ga);

    size_type* color = (size_type*)
                       malloc(sizeof(size_type) * num_vertices(ga));

    for (size_type i = 0; i < n; i++) mt_purge(color[i]);
    sp_visitor<graph_adapter> spv(s, t, color, &done, vid_map);

    size_type* searchColor = (size_type*)
                             malloc(sizeof(size_type) * num_vertices(ga));

    for (size_type i = 0; i < n; i++) searchColor[i] = 0;
    searchColor[s] = 1;

    size_type dirFwd;
    size_type dirBwd;

    if (dir == DIRECTED)
    {
      dirFwd = DIRECTED;
      dirBwd = REVERSED;
    }
    else if (dir == UNDIRECTED)
    {
      dirFwd = UNDIRECTED;
      dirBwd = UNDIRECTED;
    }
    else if (dir == REVERSED)
    {
      dirFwd = REVERSED;
      dirBwd = DIRECTED;
    }

    bool S_finished = false;
    bool T_finished = false;

    breadth_first_search<graph_adapter, size_type*, sp_visitor<graph_adapter> >
    spS(ga, searchColor, spv, dirFwd);

    breadth_first_search<graph_adapter, size_type*, sp_visitor<graph_adapter> >
    spT(ga, searchColor, spv, dirBwd);

    spS.run(s);

    if (spS.count > 0)
    {
      mt_incr(distance, 1);
    }
    else
    {
      S_finished = true;
    }

    if (spS.color[t] !=  0)
    {
      *spv.done = 1;
      mt_incr(numVisited, -1);                  // t will be added later
    }

    if (!*spv.done)
    {
      mt_write(color[t], -1);
      searchColor[t] = 2;

      #pragma mta trace "breadth_first_search from T start"
      spT.run(t);
      #pragma mta trace "breadth_first_search from T end"

      if (spT.count > 0)
      {
        mt_incr(distance, 1);
      }
      else
      {
        T_finished = true;
      }

      if (T_finished || (spT.count > spS.count))
      {
        doS = 1;
        doT = 0;
        start = 0;
        end = spS.count;
        extentT = spT.count;
      }
      else
      {
        doT = 1;
        doS = 0;
        start = 0;
        end = spT.count;
        extentS = spS.count;
      }
    }

    while ((!*spv.done) && (!S_finished || !T_finished))
    {
      if (doS && !S_finished)
      {
        mt_incr(distance, 1);

        #pragma mta trace "Parallel visits S start"
        size_type* theQ = spS.Q.Q;

        #pragma mta assert parallel
        for (size_type i = start; i < end; i++) spS.run(theQ[i]);

        #pragma mta trace "Parallel visits S end"
        extentS = spS.count - end;
        if (extentS == 0) S_finished = true;

        if (*spv.done) break;
      }
      else if (!T_finished)
      {
        mt_incr(distance, 1);

        #pragma mta trace "Parallel visits T start"
        size_type* theQ = spT.Q.Q;

        #pragma mta assert parallel
        for (size_type i = start; i < end; i++) spT.run(theQ[i]);

        #pragma mta trace "Parallel visits T end"
        extentT = spT.count - end;

        if (extentT == 0) S_finished = true;

        if (*spv.done) break;

      }

      if (T_finished || (extentS < extentT))
      {
        doS = 1;
        doT = 0;
        start = spS.count - extentS;
        end = spS.count;
      }
      else
      {
        doT = 1;
        doS = 0;
        start = spT.count - extentT;
        end = spT.count;
      }
    }

    mt_incr(numVisited, spS.count + spT.count + 2);

    free(searchColor);
    free(color);

    mttimer.stop();
    long bfsticks = mttimer.getElapsedTicks();

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

    if ( !*spv.done)
    {
      distance = -1;                   /* if not connected, distance = 0! */
      //printf("fail: dist: %d\n", distance);
    }

#if ST_DEBUG
    if (*spv.done)
    {
      fprintf(stderr, "Path length - %d, Nodes visited - %d\n",
              distance, spS.count + spT.count);
    }
    else
    {
      fprintf(stderr, "Not connected, Nodes Visited - %d\n",
              spS.count + spT.count);
    }
#endif

    return bfsticks;
  }

private:
  graph_adapter& ga;
  size_type s;
  size_type t;
  size_type& distance;
  size_type& numVisited;
};

}

#endif

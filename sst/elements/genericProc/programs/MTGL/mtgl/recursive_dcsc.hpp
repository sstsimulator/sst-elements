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
/*! \file recursive_dcsc.hpp

    \brief Implements the Recursive DCSC heuristic for strongly-connected
           component finding.

    \author William McLendon (wcmclen@sandia.gov)

    \date 6/30/2008
*/
/****************************************************************************/

#ifndef MTGL_RECURSIVE_DCSC_HPP
#define MTGL_RECURSIVE_DCSC_HPP

#include <cstdio>
#include <climits>
#include <cassert>
#include <ctime>

#include <mtgl/mtgl_adapter.hpp>
#include <mtgl/breadth_first_search_mtgl.hpp>
#include <mtgl/visit_adj.hpp>
#include <mtgl/psearch.hpp>
#include <mtgl/util.hpp>
#include <mtgl/induced_subgraph.hpp>

// #define DEBUG 1

#define USE_RECURSIVE_FUTURES 0

#define MEASURE_TIME 1
#define MEASURE_TRAPS 0
#define DETAILED_METRICS 0

#if MEASURE_TIME
    #define CREATE_TIMER(t)   mt_timer t;
    #define START_TIMER(t)    t.start();
    #define STOP_TIMER(t, tag) {\
    t.stop();\
    printf("T %15s:  %-9.4f\n", tag, t.getElapsedSeconds());\
    fflush(stdout);\
}
#else
    #define CREATE_TIMER(t)   ;
    #define START_TIMER(t)    ;
    #define STOP_TIMER(t, tag) ;
#endif

#if defined(__MTA__) && MEASURE_TRAPS
    #define START_TRAP(t) t = mta_get_task_counter(RT_TRAP);
    #define STOP_TRAP(t, tag) {\
    t = mta_get_task_counter(RT_TRAP) - t;\
    printf("%25s\t%9d\n", tag, t); fflush(stdout);\
}
#else
    #define START_TRAP(t)     ;
    #define STOP_TRAP(t, tag) ;
#endif

#ifdef DEBUG
    #define my_assert(x) if (!x) {\
    printf("%s:%i: assertion failed: ", __FILE__, __LINE__);\
    assert(x);\
}
#else
    #define my_assert(x)      ;
#endif

namespace mtgl {

class mt_metrics {
public:
  mt_metrics(char* _tag = NULL) : tag(_tag)
  {
    isMeasuring = false;
    started = false;
  }

  #pragma mta inline
  void start(void)
  {
#if defined(__MTA__) && DETAILED_METRICS
    started = true;
    isMeasuring = true;
    mta_resume_event_logging();
    issues      = mta_get_task_counter(RT_ISSUES);
    memrefs     = mta_get_task_counter(RT_MEMREFS);
    streams     = mta_get_task_counter(RT_STREAMS);
    concurrency = mta_get_task_counter(RT_CONCURRENCY);
    phantoms    = mta_get_task_counter(RT_PHANTOM);
    ready       = mta_get_task_counter(RT_READY);
    traps       = mta_get_task_counter(RT_TRAP);
#endif
  }

  #pragma mta inline
  void suspend(void)
  {
#if defined(__MTA__) && DETAILED_METRICS
    mta_suspend_event_logging();
#endif
  }

  #pragma mta inline
  void resume(void)
  {
#if defined(__MTA__) && DETAILED_METRICS
    mta_resume_event_logging();
#endif
  }

  #pragma mta inline
  void stop(void)
  {
#if defined(__MTA__) && DETAILED_METRICS
    issues      = mta_get_task_counter(RT_ISSUES) - issues;
    memrefs     = mta_get_task_counter(RT_MEMREFS) - memrefs;
    streams     = mta_get_task_counter(RT_STREAMS) - streams;
    concurrency = mta_get_task_counter(RT_CONCURRENCY) - concurrency;
    phantoms    = mta_get_task_counter(RT_PHANTOM) - phantoms;
    ready       = mta_get_task_counter(RT_READY) - ready;
    traps       = mta_get_task_counter(RT_TRAP) - traps;
    mta_suspend_event_logging();
    isMeasuring = false;
#endif
  }

  void print(void)
  {
#if defined(__MTA__) && DETAILED_METRICS
    int _issues, _memrefs, _streams, _concurrency, _phantoms, _ready, _traps;
    if (started)
    {
      if (isMeasuring)
      {
        _issues      = mta_get_task_counter(RT_ISSUES) - issues;
        _memrefs     = mta_get_task_counter(RT_MEMREFS) - memrefs;
        _streams     = mta_get_task_counter(RT_STREAMS) - streams;
        _concurrency = mta_get_task_counter(RT_CONCURRENCY) - concurrency;
        _phantoms    = mta_get_task_counter(RT_PHANTOM) - phantoms;
        _ready       = mta_get_task_counter(RT_READY) - ready;
        _traps       = mta_get_task_counter(RT_TRAP) - traps;
      }
      else
      {
        _issues = issues;
        _memrefs = memrefs;
        _streams = streams;
        _concurrency = concurrency;
        _phantoms = phantoms;
        _ready = ready;
        _traps = traps;
      }

      if (tag)
      {
        printf("-----[%10s]----------------------------------\n", tag);
      }
      else
      {
        printf("----------------------------------------------------\n");
      }

      printf("\tissues   = %d\n", _issues);
      printf("\tmemrefs  = %d\n", _memrefs);
      printf("\tstreams  = %lf\n", _streams / (double) _issues);
      printf("\tconcurr  = %lf\n", _concurrency / (double) _issues);
      printf("\tphantoms = %lf\n", _phantoms / (double) _issues);
      printf("\tready    = %lf\n", _ready / (double) _issues);
      printf("\ttraps    = %d\n", _traps);
      printf("----------------------------------------------------\n");
    }
#endif
  }

private:
  bool started;
  bool isMeasuring;
  int traps;
  int issues;
  int streams;
  int ready;
  int concurrency;
  int memrefs;
  int phantoms;
  char* tag;
};

template <class visitor_t, int parcutoff = 20>
class my_parallel_loop {
public:
  my_parallel_loop(visitor_t& _vis, int _num_iters) :
    vis(_vis), num_iters(_num_iters)
  {
    assert(num_iters >= 0);
  }

  ~my_parallel_loop() {}

  void run()
  {
    if (num_iters >= parcutoff)
    {
      #pragma mta assert parallel
      for (int i = 0; i < num_iters; i++)
      {
        vis(i);
      }
    }
    else
    {
      for (int i = 0; i < num_iters; i++)
      {
        vis(i);
      }
    }
  }

private:
  visitor_t& vis;
  int num_iters;
};

// ===================================================================
template<class G, typename cmap_t, class visitor_t, int filter = NO_FILTER,
         int directed = REVERSED, int pcutoff = 20>
class bfs_search_dcsc {
  typedef typename graph_traits<G>::vertex_descriptor vertex_t;
  typedef typename graph_traits<G>::edge_descriptor edge_t;
  typedef typename graph_traits<G>::out_edge_iterator inc_eiter_t;
  typedef typename graph_traits<G>::in_edge_iterator in_eiter_t;
  typedef typename graph_traits<G>::vertex_iterator viter_t;
  typedef typename graph_traits<G>::size_type size_type;

public:
  bfs_search_dcsc(G& _ga, cmap_t _cmap, visitor_t& _vis,
                  int* _subproblem = NULL, int _my_subp = 0) :
    ga(_ga), cmap(_cmap), vis(_vis), subproblem(_subproblem), my_subp(_my_subp)
  {
    my_assert(cmap);
  }

  void run(vertex_t& v)
  {
    vertex_id_map<G> vid_map = get(_vertex_id_map, ga);
    size_type num_verts = num_vertices(ga);
    size_type root_vid = get(vid_map, v);

    // We should exit now if the starting vertex doesn't jive!
    if (subproblem && subproblem[root_vid] != my_subp)
    {
      printf("WARNING: bfs_search_dcsc() starting vert not in subprob!\n");
      printf("\tsubproblem[%d]=%d // my_subp=%d\n", root_vid,
             subproblem[root_vid], my_subp);

      return;
    }

    int* Q = (int*) malloc(num_verts * sizeof(int));
    int qS, qE;                 // Start, End

    qS = 0;
    qE = 1;

    Q[0] = root_vid;
    cmap[root_vid] = 1;

    viter_t verts = vertices(ga);

    while (qE > qS && qS < num_verts)
    {
      int _qE = qE;
      int _qS = qS;
      qS = qE;

      #pragma mta assert parallel
      #pragma mta interleave schedule
      for (int i = _qS; i < _qE; i++)
      {
        int vid = Q[i];
        vertex_t src = verts[vid];
        vis.pre_visit(src);

        if (directed == REVERSED)
        {
          size_type deg = in_degree(src, ga);
          in_eiter_t eit = in_edges(src, ga);

          #pragma mta assert parallel
          for (size_type u = 0; u < deg; u++)
          {
            edge_t e = eit[u];

            if ((filter == AND_FILTER && vis.visit_test(e, src)) ||
                filter == NO_FILTER)
            {
              vertex_t v2   = target(e, ga);
              size_type v2id = get(vid_map, v2);
              if (subproblem && subproblem[v2id] == my_subp)
              {
                int clr  = mt_incr(cmap[v2id], 1);
                if (clr == 0)
                {
                  vis.tree_edge(e, src);
                  Q[ mt_incr(qE, 1) ] = v2id;
                }
                else
                {
                  vis.back_edge(e, src);
                }
              }
            }
          }
        }
        else if (directed == DIRECTED)
        {
          size_type deg = out_degree(src, ga);
          inc_eiter_t eit = out_edges(src, ga);

          #pragma mta assert parallel
          for (size_type u = 0; u < deg; u++)
          {
            edge_t e = eit[u];

            if ((filter == AND_FILTER && vis.visit_test(e, src)) ||
                filter == NO_FILTER)
            {
              vertex_t v2   = target(e, ga);
              size_type v2id = get(vid_map, v2);
              if (subproblem && subproblem[v2id] == my_subp)
              {
                int clr  = mt_incr(cmap[v2id], 1);
                if (clr == 0)
                {
                  vis.tree_edge(e, src);
                  Q[ mt_incr(qE, 1) ] = v2id;
                }
                else
                {
                  vis.back_edge(e, src);
                }
              }
            }
          }
        }
      }
    }

    free(Q);
    Q = NULL;
  }

private:
  G& ga;
  cmap_t cmap;
  visitor_t& vis;
  int* subproblem;
  int my_subp;
};

template <typename graph_t>
class recursive_dcsc {
public:
  typedef typename graph_traits<graph_t>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph_t>::edge_descriptor edge_t;
  typedef typename graph_traits<graph_t>::out_edge_iterator inc_eiter_t;
  typedef typename graph_traits<graph_t>::in_edge_iterator in_eiter_t;
  typedef typename graph_traits<graph_t>::vertex_iterator viter_t;
  typedef typename graph_traits<graph_t>::size_type size_type;

  // ===================================================================
  recursive_dcsc(graph_t& gg, int& _numTriv, int& _numNonTriv, int* _scc) :
    ga(gg), numTriv(_numTriv), numNonTriv(_numNonTriv), scc(_scc)
  {
    my_assert(scc);
    mark_none = 0;
    mark_fwd  = 1 << 0;
    mark_bwd  = 1 << 1;
    mark_scc  = mark_fwd | mark_bwd;
    numTriv = 0;
    numNonTriv = 0;
  }

  // ===================================================================
  int run()
  {
//    mt_metrics mtmetrics("run");

    CREATE_TIMER(my_timer);
    START_TIMER(my_timer);
//    mtmetrics.start();
    const size_type num_verts = num_vertices(ga);
    const size_type num_edges = num_edges(ga);
    vertex_id_map<graph_t> vid_map = get(_vertex_id_map, ga);
    edge_id_map<graph_t>   eid_map = get(_edge_id_map,   ga);

    int* subprob    = (int*) malloc(sizeof(int) * num_verts);
    int* in_degree  = (int*) malloc(sizeof(int) * num_verts);
    int* out_degree = (int*) malloc(sizeof(int) * num_verts);

    // Compute in and out degrees of every vertex...
    compute_degrees(in_degree, out_degree);

    // Simple trick.  Verts which have 0 indegree or 0 outdegree can be
    // filtered out.
    int num_zero_degree = 0;
    int num_incl_verts  = 0;

    #pragma mta assert parallel
    for (size_type i = 0; i < num_verts; i++)
    {
      if (in_degree[i] == 0) mt_incr(num_zero_degree, 1);
      else if (out_degree[i] == 0) mt_incr(num_zero_degree, 1);
      else mt_incr(num_incl_verts, 1);
    }

    assert(num_zero_degree + num_incl_verts == num_verts);

    int* incl_verts = (int*) malloc(sizeof(int) * num_incl_verts);
    int cur_vidx = 0;

    #pragma mta assert parallel
    for (size_type i = 0; i < num_verts; i++)
    {
      if (!(in_degree[i] == 0 || out_degree[i] == 0))
      {
        int l_cur_vidx = mt_incr(cur_vidx, 1);
        incl_verts[l_cur_vidx] = i;
        subprob[i] = 1;
      }
      else
      {
        mt_incr(numTriv, 1);
        scc[i] = i;
        subprob[i] = 0;
      }
    }

    free(in_degree); in_degree = NULL;
    free(out_degree); out_degree = NULL;

    int* cmapF      = (int*) malloc(sizeof(int) * num_verts);                   // global color map
    int* cmapB      = (int*) malloc(sizeof(int) * num_verts);                   // cmap for backward searches
    int* vert_marks = (int*) malloc(sizeof(int) * num_verts);

    #pragma mta assert no dependence
    for (size_type i = 0; i < num_verts; i++)
    {
      cmapF[i]      = 0;
      cmapB[i]      = 0;
      vert_marks[i] = 0;
    }

    next_subp = 2;

    r_run(1, subprob, incl_verts, num_incl_verts, vert_marks, cmapF, cmapB);

    free(subprob);
    free(cmapF);
    free(cmapB);
    free(vert_marks);
    // incl_verts is freed down inside r_run before recursing

    STOP_TIMER(my_timer, "recursive_dcsc");

    return(0);
  }

  // ===================================================================
  int r_run(const int current_subprob,
            int* _subprob,
            int* _incl_verts,
            const int num_incl_verts,
            int* _vert_marks,
            int* _cmapF,
            int* _cmapB)
  {
    vertex_id_map<graph_t> vid_map = get(_vertex_id_map, ga);
    edge_id_map<graph_t>   eid_map = get(_edge_id_map,   ga);

    const size_type num_verts = num_vertices(ga);
    const size_type num_edges = num_edges(ga);

    // create local copies of global 'pointers'
    int* subprob = _subprob;
    int* incl_verts = _incl_verts;
    int* vert_marks = _vert_marks;
    int* cmapF = _cmapF;
    int* cmapB = _cmapB;

    if (num_incl_verts == 0)
    {
      return(0);
    }
    else if (num_incl_verts == 1)
    {
      return(0);
    }
    else
    {
      int pivot_vid = incl_verts[ num_incl_verts / 2 ];

#ifdef ASSERT
      assert(pivot_vid < num_verts);
#endif

      vertex_t pivot_vert = get_vertex(pivot_vid, ga);

      if (num_incl_verts > 20)
      {
        #pragma mta assert parallel
        for (int i = 0; i < num_incl_verts; i++)
        {
          const int vidx = incl_verts[i];
          vert_marks[ vidx ] = mark_none;
          cmapF[ vidx ] = 0;
          cmapB[ vidx ] = 0;
        }
      }
      else
      {
        for (int i = 0; i < num_incl_verts; i++)
        {
          const int vidx = incl_verts[i];
          vert_marks[ vidx ] = mark_none;
          cmapF[ vidx ] = 0;
          cmapB[ vidx ] = 0;
        }
      }

      // Run bfs search in forward directed direction.
      mark_visitor_t visf(ga, mark_fwd, vert_marks, current_subprob,
                          subprob, vid_map, eid_map);
      bfs_search_dcsc<graph_t, int*, mark_visitor_t, AND_FILTER, DIRECTED, 20>
        mark_F(ga, cmapF, visf, subprob, current_subprob);

#ifdef __MTA__
      future int __mark_F__fut;
      future __mark_F__fut(mark_F, pivot_vert)
      {
        mark_F.run(pivot_vert);
        return(1);
      }
#else
      mark_F.run(pivot_vert);
#endif

      // Run our bfs search to traverse edges in a reversed direction
      mark_visitor_t visb(ga, mark_bwd, vert_marks, current_subprob,
                          subprob, vid_map, eid_map);
      bfs_search_dcsc<graph_t, int*, mark_visitor_t, AND_FILTER, REVERSED, 20>
        mark_B(ga, cmapB, visb, subprob, current_subprob);
      mark_B.run(pivot_vert);

#ifdef __MTA__
      touch(&__mark_F__fut);
#endif

      // cleanup when done...
      if (num_incl_verts > 20)
      {
        #pragma mta assert parallel
        for (int i = 0; i < num_incl_verts; i++)
        {
          const int vidx = incl_verts[i];
          cmapF[ vidx ] = 0;
          cmapB[ vidx ] = 0;
        }
      }
      else
      {
        for (int i = 0; i < num_incl_verts; i++)
        {
          const int vidx = incl_verts[i];
          cmapF[ vidx ] = 0;
          cmapB[ vidx ] = 0;
        }
      }

      int scc_size = 0;
      int fwd_size = 0;
      int bwd_size = 0;
      int oth_size = 0;

      #pragma mta assert parallel
      for (int i = 0; i < num_incl_verts; i++)
      {
        const int vid = incl_verts[i];

        if (vert_marks[vid] == mark_scc)
        {
          mt_incr(scc_size, 1);
          scc[vid] = pivot_vid;
        }
        else if (vert_marks[vid] == mark_fwd)
        {
          mt_incr(fwd_size, 1);
        }
        else if (vert_marks[vid] == mark_bwd)
        {
          mt_incr(bwd_size, 1);
        }
        else if (vert_marks[vid] == mark_none)
        {
          mt_incr(oth_size, 1);
        }
      }
#ifdef DEBUG
      printf("\tsize of SCC = %d\n", scc_size);
      printf("\tsize of FWD = %d\n", fwd_size);
      printf("\tsize of BWD = %d\n", bwd_size);
      printf("\tsize of OTH = %d\n", oth_size);
      fflush(stdout);
#endif

      // Deal with SCCs: just need to update the appropriate counter.
      if (scc_size > 1)
      {
        mt_incr(numNonTriv, 1);
      }
      else
      {
        mt_incr(numTriv, 1);
      }

      int* fwd_verts = NULL;   int fwd_pos = 0;   int fwd_subp_id = -1;
      int* bwd_verts = NULL;   int bwd_pos = 0;   int bwd_subp_id = -1;
      int* oth_verts = NULL;   int oth_pos = 0;   int oth_subp_id = -1;

      // Deal with "fwd" partition.
      if (fwd_size > 0)
      {
        fwd_verts = (int*) malloc(sizeof(int) * fwd_size);
        fwd_subp_id = mt_incr(next_subp, 1);
      }
      // Deal with "bwd" partition.
      if (bwd_size > 0)
      {
        bwd_verts = (int*) malloc(sizeof(int) * bwd_size);
        bwd_subp_id = mt_incr(next_subp, 1);
      }
      // Deal with "other" vertices
      if (oth_size > 0)
      {
        oth_verts = (int*) malloc(sizeof(int) * oth_size);
        oth_subp_id = mt_incr(next_subp, 1);
      }

      #pragma mta assert parallel
      for (int i = 0; i < num_incl_verts; i++)
      {
        const int vid = incl_verts[i];

        if (vert_marks[vid] == mark_fwd)
        {
          int pos = mt_incr(fwd_pos, 1);
          fwd_verts[ pos ] = vid;
          subprob[ vid ] = fwd_subp_id;
        }
        else if (vert_marks[vid] == mark_bwd)
        {
          int pos = mt_incr(bwd_pos, 1);
          bwd_verts[ pos ] = vid;
          subprob[ vid ] = bwd_subp_id;
        }
        else if (vert_marks[vid] == mark_none)
        {
          int pos = mt_incr(oth_pos, 1);
          oth_verts[ pos ] = vid;
          subprob[ vid ] = oth_subp_id;
        }
      }

#ifdef DEBUG
      print_1d_int_array(subprob, num_verts, "subprob");
      printf("%d", fwd_subp_id);
      print_1d_int_array(fwd_verts, fwd_size, "fwd_verts");
      printf("%d", bwd_subp_id);
      print_1d_int_array(bwd_verts, bwd_size, "bwd_verts");
      printf("%d", oth_subp_id);
      print_1d_int_array(oth_verts, oth_size, "oth_verts");
      fflush(stdout);
#endif

      // Free up our working set before recursing (or memory will blow up!).
      free(incl_verts);
      incl_verts = NULL;

#if defined(__MTA__) && USE_RECURSIVE_FUTURES
      bool use_fut = false;
      bool fut_fwd = false;
      bool fut_bwd = false;
      if (fwd_size > 50 || bwd_size > 50 || oth_size > 50)
      {
        use_fut = true;
      }
      future int __fwd, __bwd;
#endif

      if (fwd_size > 1)
      {
#if defined(__MTA__) && USE_RECURSIVE_FUTURES
        if (use_fut)
        {
          fut_fwd = true;
          future __fwd(fwd_subp_id, subprob, fwd_verts, fwd_size,
                       vert_marks, cmapF, cmapB)
          {
            r_run(fwd_subp_id, subprob, fwd_verts, fwd_size, vert_marks,
                  cmapF, cmapB);
            return(1);
          }

        }
        else
        {
          r_run(fwd_subp_id, subprob, fwd_verts, fwd_size, vert_marks,
                cmapF, cmapB);
        }

#else
        r_run(fwd_subp_id, subprob, fwd_verts, fwd_size, vert_marks,
              cmapF, cmapB);
#endif

      }
      else if (fwd_size == 1)
      {
        mt_incr(numTriv, 1);
        scc[ fwd_verts[0] ] = fwd_verts[0];
        free(fwd_verts); fwd_verts = NULL; fwd_size = 0;
      }

      if (bwd_size > 1)
      {
#if defined(__MTA__) && USE_RECURSIVE_FUTURES
        if (use_fut)
        {
          fut_bwd = true;
          future __bwd(bwd_subp_id, subprob, bwd_verts, bwd_size,
                       vert_marks, cmapF, cmapB)
          {
            r_run(bwd_subp_id, subprob, bwd_verts, bwd_size, vert_marks,
                  cmapF, cmapB);
            return(1);
          }

        }
        else
        {
          r_run(bwd_subp_id, subprob, bwd_verts, bwd_size, vert_marks,
                cmapF, cmapB);
        }
#else
        r_run(bwd_subp_id, subprob, bwd_verts, bwd_size, vert_marks,
              cmapF, cmapB);
#endif
      }
      else if (bwd_size == 1)
      {
        mt_incr(numTriv, 1);
        scc[ bwd_verts[0] ] = bwd_verts[0];
        free(bwd_verts); bwd_verts = NULL; bwd_size = 0;
      }

      if (oth_size > 1)
      {
        // In some graphs, the 'other' set may have lots of disconnected
        // vertices (i.e., rmat), this just compresses this other-set.
        subprob_trim(oth_subp_id, subprob, oth_verts, oth_size);
      }
      if (oth_size > 1)
      {
        r_run(oth_subp_id, subprob, oth_verts, oth_size, vert_marks,
              cmapF, cmapB);
      }
      else if (oth_size == 1)
      {
        mt_incr(numTriv, 1);
        scc[ oth_verts[0] ] = oth_verts[0];
        free(oth_verts); oth_verts = NULL; oth_size = 0;
      }

#if defined(__MTA__) && USE_RECURSIVE_FUTURES
      if (fut_fwd) touch(&__fwd);
      if (fut_bwd) touch(&__bwd);
#endif
    }

    return(0);
  }

protected:
  // ===================================================================
  void subprob_trim(int& current_subprob,
                    int* subprob,
                    int* incl_verts,
                    int& num_incl_verts)
  {
    vertex_id_map<graph_t> vid_map = get(_vertex_id_map, ga);

    int* num_neighbors = (int*) malloc(sizeof(int) * num_incl_verts);

    #pragma mta assert no dependence
    for (int i = 0; i < num_incl_verts; i++)
    {
      num_neighbors[i] = 0;
    }

    viter_t verts = vertices(ga);

    #pragma mta assert parallel
    #pragma mta interleave schedule
    for (int i = 0; i < num_incl_verts; i++)
    {
      size_type deg = 0;
      int vid = incl_verts[i];
      vertex_t vi = verts[vid];

      // Check one direction.
      deg = in_degree(vi, ga);
      in_eiter_t ieit  = in_edges(vi, ga);

      #pragma mta assert parallel
      for (size_type u = 0; u < deg; u++)
      {
        edge_t e = ieit[u];

        size_type v2id = get(vid_map, target(e, ga));
        if (subprob && current_subprob == subprob[v2id])
        {
          mt_incr(num_neighbors[i], 1);
        }
      }

      // Check the other direction.
      deg = out_degree(vi, ga);
      inc_eiter_t eit = out_edges(vi, ga);

      #pragma mta assert parallel
      for (size_type u = 0; u < deg; u++)
      {
        edge_t e = eit[u];

        size_type v2id = get(vid_map, target(e, ga));
        if (subprob && current_subprob == subprob[v2id])
        {
          mt_incr(num_neighbors[i], 1);
        }
      }
    }

    // Ok, how many do we have that are(n't) connected to another vert in
    // this set.  Vertices who have no neighbors should be stuck in their
    // own little happy SCC place.
    int num_verts_with_neighbors = 0;

    #pragma mta assert parallel
    for (int i = 0; i < num_incl_verts; i++)
    {
      if (num_neighbors[i] > 0)
      {
        mt_incr(num_verts_with_neighbors, 1);
      }
      else
      {
        int vid = incl_verts[i];
        scc[ vid ] = vid;
      }
    }

    if (num_verts_with_neighbors > 0)
    {
      // Ruh roh, we have some stuff that still lives in this subproblem, so
      // we need to basically repack our incl_verts array and modify our
      // num_incl_verts array to match.
      int* new_verts = (int*) malloc(sizeof(int) * num_verts_with_neighbors);

      int idx = 0;

      #pragma mta assert parallel
      for (int i = 0; i < num_incl_verts; i++)
      {
        if (num_neighbors[i] > 0)
        {
          int cur_idx = mt_incr(idx, 1);
          new_verts[cur_idx] = incl_verts[i];
        }
      }

      #pragma mta assert no dependence
      for (int i = 0; i < num_verts_with_neighbors; i++)
      {
        incl_verts[i] = new_verts[i];
      }

      free(new_verts);  new_verts = NULL;
      num_incl_verts = num_verts_with_neighbors;
    }
    else
    {
      num_incl_verts = 0;
    }

    free(num_neighbors);
    num_neighbors = NULL;
  }

  // ===================================================================
  template<typename T>
  void print_1d_int_array(T A, int n, const char* tag = NULL)
  {
    if (tag)
    {
      printf("%15s: [ ", tag);
    }
    else
    {
      printf("1d int array   : [ ");
    }

    int c = 0;
    for (int i = 0; i < n; i++)
    {
      printf("%d ", A[i]);
      c++;
      if (c == 25 && i + 1 < n) printf("\n%15d:   ", i + 1); c = 0;
    }

    printf("]\n");
    fflush(stdout);
  }

  // ===================================================================
  void printGraph()
  {
    vertex_id_map<graph_t> vid_map = get(_vertex_id_map, ga);
    edge_id_map<graph_t> eid_map = get(_edge_id_map,   ga);

    viter_t verts = vertices(ga);
    size_type num_verts = num_vertices(ga);

    for (size_type vi = 0; vi < num_verts; vi++)
    {
      printf("%d\t{ ", vi);

      const vertex_t v = verts[vi];
      const size_type deg = out_degree(v, ga);
      inc_eiter_t inc_edgs = out_edges(v, ga);

      for (size_type i = 0; i < deg; i++)
      {
        edge_t e = inc_edgs[i];
        const size_type tid = get(vid_map, target(e, ga));
        printf("%d ", tid);
      }

      printf("}\n");
    }
  }

  // ===================================================================
  int select_pivot(int* activeVerts, int activeCount)
  {
    int cur_idx   = 0;
    int piv_idx   = 0;
    int ctr = activeCount / 2;

    size_type num_verts = num_vertices(ga);

    #pragma mta assert parallel
    for (size_type vi = 0; vi < num_verts; vi++)
    {
      if (activeVerts[vi] && mt_incr(ctr, -1) == 0)
      {
        piv_idx = vi;
      }
    }

    return(piv_idx);
  }

  // ===================================================================
  void compute_degrees(int* inDegree, int* outDegree)
  {
    my_assert(inDegree);
    my_assert(outDegree);
    vertex_id_map<graph_t> vid_map = get(_vertex_id_map, ga);

    viter_t verts = vertices(ga);
    size_type num_verts = num_vertices(ga);

    #pragma mta assert no dependence
    for (size_type vi = 0; vi < num_verts; vi++)
    {
      const vertex_t v = verts[vi];
      inDegree[vi] = 0;
      outDegree[vi] = out_degree(v, ga);
    }

    #pragma mta assert parallel
    for (size_type vi = 0; vi < num_verts; vi++)
    {
      const vertex_t v = verts[vi];
      const size_type deg = out_degree(v, ga);
      inc_eiter_t inc_edges = out_edges(v, ga);

      for (size_type u = 0; u < deg; u++)
      {
        edge_t e = inc_edges[u];
        const size_type uid = get(vid_map, target(e, ga));
        mt_incr(inDegree[uid], 1);
      }
    }
  }

private:
  // ===================================================================
  class mark_visitor_t : public default_psearch_visitor<graph_t>{
public:
    mark_visitor_t(graph_t& _ga,
                   int _mark_value,
                   int* _mark_array,
                   const int _subprob_id,
                   int* _subprob_v,
                   vertex_id_map<graph_t>& __vid_map,
                   edge_id_map<graph_t>& __eid_map) :
      ga(_ga), mark_value(_mark_value), mark_array(_mark_array),
      subprob_id(_subprob_id), subprob_v(_subprob_v),
      vid_map(__vid_map), eid_map(__eid_map) {}

    bool visit_test(edge_t& e, vertex_t& v) const
    {
      bool ret = true;
      size_type sid = get(vid_map, source(e, ga));
      size_type tid = get(vid_map, target(e, ga));

      if (subprob_v[sid] != subprob_v[tid]) ret = false;

      return(ret);
    }

    void pre_visit(vertex_t& v) const
    {
      size_type vid = get(vid_map, v);
      int tmp = mt_readfe(mark_array[vid]);
      tmp |= mark_value;
      mt_write(mark_array[vid], tmp);
    }

private:
    graph_t& ga;
    int mark_value;
    int* mark_array;
    int* subprob_v;
    int subprob_id;
    vertex_id_map<graph_t>& vid_map;
    edge_id_map<graph_t>& eid_map;
  };

  // ===================================================================
  // DATA MEMBERS
  // ===================================================================
  graph_t& ga;
  int* scc;                             // scc labels.
  int& numTriv;                         // # of trivial SCCs
  int& numNonTriv;                      // # of nontrivial SCCs

  int next_subp;                        // next subp #

  int mark_none;
  int mark_fwd;
  int mark_bwd;
  int mark_scc;
};

}

#undef USE_RECURSIVE_FUTURES
#undef MEASURE_TIME
#undef MEASURE_TRAPS
#undef DETAILED_METRICS

#ifdef DEBUG
    #undef my_assert
#endif
#if MEASURE_TIME
    #undef CREATE_TIMER
    #undef START_TIMER
    #undef STOP_TIMER
#endif

#endif

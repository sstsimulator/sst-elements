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
/*! \file triangles.hpp

    \author Jon Berry (jberry@sandia.gov)

    \date 11/2006
*/
/****************************************************************************/

#ifndef MTGL_TRIANGLES_HPP
#define MTGL_TRIANGLES_HPP

#include <stdint.h>
#include <cstdio>
#include <cstdlib>
#include <climits>
#include <cassert>

#include <mtgl/util.hpp>
#include <mtgl/graph_traits.hpp>
#include <mtgl/mtgl_adapter.hpp>
#include <mtgl/visit_adj.hpp>
#include <mtgl/independent_set.hpp>
#include <mtgl/induced_subgraph.hpp>
#include <mtgl/psearch.hpp>
#include <mtgl/dynamic_array.hpp>

namespace mtgl {

template <class graph>
class default_triangles_visitor {
public:
  typedef typename graph_traits<graph>::size_type size_type;

  default_triangles_visitor() {}
  void operator()(size_type v1, size_type v2, size_type v3) {}
};

template <typename graph, typename visitor = default_triangles_visitor<graph> >
class find_triangles_phases {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;
  typedef typename graph_traits<graph>::edge_iterator edge_iterator;
  typedef typename graph_traits<graph>::vertex_iterator vertex_iterator;

  find_triangles_phases(graph& gg, visitor& vis,
                        size_type* subp = 0, size_type my_subp = 0) :
    g(gg), tri_visitor(vis), subproblem(subp), my_subproblem(my_subp),
    indset((bool*) 0), marker((vertex_t*) 0),
    has_marker(new bool[2 * num_vertices(gg)])
  {
    size_type order = num_vertices(g);
    size_type size = num_edges(g);

    vertex_id_map<graph> vid_map = get(_vertex_id_map, g);
    edge_id_map<graph> eid_map = get(_edge_id_map, g);

    // Better subproblem handling needed to avoid this big
    // allocation for small subproblems.
    unsettled_v = (bool*) malloc(sizeof(bool) * order);
    unsettled_e = (bool*) malloc(sizeof(bool) * size);
    mark = (size_type*)  malloc(sizeof(size_type) * order);
    confounding = (int*)  malloc(sizeof(int) * order);
    n_unsettled_v = order;

    if (subproblem)
    {
      #pragma mta assert nodep
      for (size_type i = 0; i < order; i++)
      {
        has_marker[i] = false;

        if (subproblem[i] == my_subproblem)
        {
          unsettled_v[i] = true;
          mt_incr(n_unsettled_v, 1);
        }
        else
        {
          unsettled_v[i] = false;
        }
      }

      edge_iterator edgs = edges(g);

      #pragma mta assert nodep
      for (size_type i = 0; i < size; i++)  // hot
      {
        edge_t e = edgs[i];
        size_type eid = get(eid_map, e);
        vertex_t v1 = source(e, g);
        vertex_t v2 = target(e, g);
        size_type v1id = get(vid_map, v1);
        size_type v2id = get(vid_map, v2);

        if ((unsettled_v[v1id]) || (unsettled_v[v2id]))
        {
          unsettled_e[eid] = true;
        }
      }
    }
    else
    {
      #pragma mta assert nodep
      for (size_type i = 0; i < order; i++)
      {
        has_marker[i] = false;
        unsettled_v[i] = true;
      }

      #pragma mta assert nodep
      for (size_type i = 0; i < size; i++)
      {
        unsettled_e[i] = true;
      }

      n_unsettled_v = order;
    }
  }

  ~find_triangles_phases()
  {
    free(unsettled_v);
    free(unsettled_e);
    free(mark);
    delete [] has_marker;
    free(confounding);
  }

  class mark_vis {
    //  Independent set vertices try to mark their neighbors.
public:
    typedef typename graph_traits<graph>::size_type size_type;

    mark_vis(graph& g, bool* us, size_type* mk, int* con, bool* hm,
             vertex_id_map<graph>& vm, edge_id_map<graph>& em) :
      unsettled_e(us), mark(mk), confounding(con),
      has_marker(hm), vid_map(vm), eid_map(em) {}

    bool visit_test(vertex_t v) { return true; }
    void pre_visit(vertex_t v) {}
    void post_visit(vertex_t v) {}

    void operator()(edge_t e, vertex_t src, vertex_t dest)
    {
      size_type eid = get(eid_map, e);
      size_type sid = get(vid_map, src);
      size_type tid = get(vid_map, dest);

      if (!unsettled_e[eid]) return;

      // No self loops (we'll say that a vertex with
      // no adjacencies other than to itself can be
      // considered 'independent').
      if (sid == tid) return;

      // Here's a design decision: confounding could
      // be a bool array to save space, but then we'd
      // have to use f/e bits to enforce synchronization.
      // If we make confounding an int array, we can
      // use the atomic instruction and speed up.
      // confounding[id] is init'd to -1, so one reader
      // will make it 0. confounding >=1 indicates the
      // conflict.
      int cur = mt_incr(confounding[tid], 1);

      // Later, we might define dominance; for
      // now, we simply let the first marker win
      // in order to save memory references.
      if (cur == -1)
      {
        mark[tid] = mark[sid] + 1;
        has_marker[mark[sid] + 1] = true;
      }
      else if (mark[tid] == mark[sid] + 1)
      {
        // Multiple edge src->dest.
        mt_incr(confounding[tid], -1);
      }
    }

private:
    bool* has_marker;
    bool* unsettled_e;
    size_type* mark;
    int* confounding;
    vertex_id_map<graph>& vid_map;
    edge_id_map<graph>& eid_map;
  };

  void mark_phase(int round, size_type* active_deg)
  {
    size_type order = num_vertices(g);
    size_type size = num_edges(g);
    vertex_id_map<graph> vid_map = get(_vertex_id_map, g);
    edge_id_map<graph> eid_map = get(_edge_id_map, g);
    size_type issize, token = 2;
    indset = (bool*) malloc(sizeof(bool) * order);

    maximal_independent_set<graph, bool*>
    mis(g, active_deg, unsettled_v, unsettled_e);
    mis.run(indset, issize);

/*
    independent_set<graph, bool*> is(g, active_deg, unsettled_v, unsettled_e);
    is.run(indset, issize);
*/
    printf("tri: issize: %lu\n", (long unsigned) issize);

    if (issize == 0)
    {
      fprintf(stderr, "error: zero independent set\n");
      exit(1);
    }

    max_mark = 2 * issize;

    marker = (vertex_t*) malloc(sizeof(vertex_t) * 2 * (issize + 1));

    // We now have an indicating bit vector for the ind. set.

    #pragma mta assert nodep
    for (size_type i = 0; i < order; i++)
    {
      mark[i] = 0;
      confounding[i] = -1;
    }

    // Everybody in the independent set takes a token so that
    // we'll be able to use counting sort later.

    vertex_iterator verts = vertices(g);

    #pragma mta assert parallel
    for (size_type i = 0; i < order; i++)
    {
      if (indset[i])
      {
        vertex_t vi = verts[i];
        size_type tok = mt_incr(token, 2);
        mark[i] = tok - 1;
        marker[tok] = vi;
        marker[tok - 1] = vi;
        has_marker[tok] = true;
        has_marker[tok - 1] = true;
      }
    }

    mark_vis mvis(g, unsettled_e, mark, confounding, has_marker,
                  vid_map, eid_map);
    visit_adj<graph, mark_vis>(g, mvis, indset, unsettled_e);
  }

  static void register_triangle(visitor& tv, vertex_id_map<graph>& vid_map,
                                vertex_t v1, vertex_t v2, vertex_t v3)
  {
    size_type v1id = get(vid_map, v1);
    size_type v2id = get(vid_map, v2);
    size_type v3id = get(vid_map, v3);

    if (v1id < v2id && v1id < v3id)
    {
      if (v2id < v3id)
      {
        tv(v1, v2, v3);
      }
      else
      {
        tv(v1, v3, v2);
      }
    }
    else if (v2id < v1id && v2id < v3id)
    {
      if (v1id < v3id)
      {
        tv(v2, v1, v3);
      }
      else
      {
        tv(v2, v3, v1);
      }
    }
    else if (v3id < v1id && v3id < v2id)
    {
      if (v1id < v2id)
      {
        tv(v3, v1, v2);
      }
      else
      {
        tv(v3, v2, v1);
      }
    }
  }

  class edge_sweep_vis {
public:
    edge_sweep_vis(graph& gg, bool* is, size_type* mk, vertex_t* mrkr,
                   bool* uns_v, bool* uns_e, visitor& tri_vis,
                   vertex_id_map<graph>& vm, edge_id_map<graph>& em) :
      g(gg), indset(is), mark(mk), marker(mrkr), unsettled_v(uns_v),
      unsettled_e(uns_e), tri_visitor(tri_vis), vid_map(vm), eid_map(em) {}

    bool visit_test(edge_t e)
    {
      vertex_t v1 = source(e, g);
      vertex_t v2 = target(e, g);
      size_type v1id = get(vid_map, v1);
      size_type v2id = get(vid_map, v2);
      size_type eid = get(eid_map, e);

      // Should probably make an 'info' struct to keep
      // these data together.
      return (mark[v1id] != 0 && unsettled_e[eid] &&
              unsettled_v[v1id] && unsettled_v[v2id] &&
              !indset[v1id] &&
              !indset[v2id] && (mark[v1id] == mark[v2id]));
    }

    void operator()(edge_t e)
    {
      vertex_t v1 = source(e, g);
      vertex_t v2 = target(e, g);
      printf("edge_sweep_vis: (%d,%d)\n", get(vid_map, v1), get(vid_map, v2));
      size_type v1id = get(vid_map, v1);

      if (mark[v1id] == 0) return;  // Indset didn't go there.

      vertex_t v3 = marker[mark[v1id]];

      if (v1 != v2 && v2 != v3)
      {
        register_triangle(tri_visitor, vid_map, v1, v2, v3);
      }
    }

private:
    graph&  g;
    bool* indset;
    bool* unsettled_v;
    bool* unsettled_e;
    size_type* mark;
    vertex_t* marker;
    visitor& tri_visitor;
    vertex_id_map<graph>& vid_map;
    edge_id_map<graph>& eid_map;
  };

  void edge_sweep_phase()
  {
    vertex_id_map<graph> vid_map = get(_vertex_id_map, g);
    edge_id_map<graph> eid_map = get(_edge_id_map, g);
    edge_sweep_vis esv(g, indset, mark, marker, unsettled_v,
                       unsettled_e, tri_visitor, vid_map, eid_map);
    visit_edges_filtered<graph, edge_sweep_vis>(g, esv);
  }

  class confounding_vis {
public:
    confounding_vis(graph& gg, size_type* mk, bool* hm, vertex_t* mrkr,
                    size_type mm, visitor& tri_vis, bool* unsv,
                    vertex_id_map<graph>& vm, edge_id_map<graph>& em) :
      g(gg), order(num_vertices(g)), mark(mk), has_marker(hm), marker(mrkr),
      unsettled_v(unsv), max_mark(mm), count(0), tri_visitor(tri_vis),
      vid_map(vm), eid_map(em) {}

    bool visit_test(vertex_t v) { return true; }

    void pre_visit(vertex_t v)
    {
      neigh_id = (size_type*) malloc(sizeof(size_type) * out_degree(v, g));
      neigh_mark = (size_type*) malloc(sizeof(size_type) * out_degree(v, g));
    }

    void operator()(edge_t e, vertex_t src, vertex_t dest)
    {
      size_type sid = get(vid_map, src);
      size_type tid = get(vid_map, dest);

      if (!unsettled_v[tid]) return;

      size_type index = mt_incr(count, 1);
      neigh_id[index] = tid;
      neigh_mark[index] = mark[tid];
    }

#pragma mta debug level 0
    void post_visit(vertex_t cv)
    {
      size_type cvid = get(vid_map, cv);
      size_type cv_mark = mark[cvid];
      size_type cv_marker = get(vid_map, marker[cv_mark]);

      if (count > 100)
      {
        bucket_sort<size_type, size_type>(neigh_mark, count,
                                          max_mark, neigh_id);
      }
      else
      {
        insertion_sort<size_type, size_type>(neigh_mark, count,
                                             neigh_id);
      }

      size_type* cur_mark = (size_type*) malloc(sizeof(size_type) * count);

      cur_mark[0] = (neigh_mark[0] % 2 == 1) *
                    (neigh_mark[0] != (cv_mark - 1)) *
                    neigh_mark[0];
      for (size_type i = 1; i < count; i++)
      {
        cur_mark[i] = (cur_mark[i - 1] * (neigh_mark[i] % 2 == 0)) +
                      (cur_mark[i - 1] * (neigh_mark[i] % 2 == 1) *
                       (neigh_mark[i] == (cv_mark - 1))) +
                      (neigh_mark[i] * (neigh_mark[i] % 2 == 1) *
                       (neigh_mark[i] != (cv_mark - 1)));
      }
#if 0
      printf("tri conflict res: count: %d---------\n", count);
      printf("tri cv_mark: %d\n", cv_mark);

      for (size_type i = 0; i < count; i++)
      {
        printf("tri i: %d\n", i);
        printf("tri neigh_mark[%d]: %d\n", i, neigh_mark[i]);
      }

      for (size_type i = 0; i < count; i++)
      {
        printf("tri i: %d\n", i);
        printf("tri cur_mark[%d]: %d\n", i, cur_mark[i]);
      }

      for (size_type i = 0; i < count; i++)
      {
        printf("tri i: %d\n", i);

        if (has_marker[cur_mark[i]])
        {
          size_type id = get(vid_map, marker[cur_mark[i]]);
          printf("tri marker[%d]: %d\n", cur_mark[i], id);
        }
      }
#endif

      vertex_iterator verts = vertices(g);

      #pragma mta assert parallel
      for (size_type i = 0; i < count; i++)
      {
        /*******************************
        if (unsettled_v[neigh_id[i]] && cur_mark[i]%2==1)
        {
          printf("unsettled neighbor[%i]: %d \n", i, neigh_id[i]);

          if (!cur_mark) printf("%d: cur_mark is 0\n", i);

          if (!cur_mark[i]) printf("cur_mark[%d] is 0\n", i);

          if (!marker[cur_mark[i]]) printf("marker[cur_mark[%d]] is 0\n", i);

          if (!neigh_mark[i]) printf("neigh_mark[%d] is 0\n", i);

          if (!marker[neigh_mark[i]])
          {
            printf("marker[neigh_mark[%d]] is 0\n", i);
          }
        }
        *******************************/

        size_type cmarker_id = 0;
        size_type nmarker_id = 0;

        if (has_marker[cur_mark[i]])
        {
          cmarker_id = get(vid_map, marker[cur_mark[i]]);
        }

        if (has_marker[neigh_mark[i]])
        {
          nmarker_id = get(vid_map, marker[neigh_mark[i]]);
        }

        if (unsettled_v[neigh_id[i]] && (cur_mark[i] % 2 == 1) &&
            (cur_mark[i] + 1 != cv_mark) && has_marker[cur_mark[i]] &&
            (cmarker_id == nmarker_id) && (neigh_mark[i] % 2 == 0))
        {

          register_triangle(tri_visitor, vid_map, cv,
                            verts[neigh_id[i]], marker[cur_mark[i]]);
        }
      }

      free(neigh_mark);
      free(neigh_id);
      free(cur_mark);
    }

#pragma mta debug level default

private:
    graph& g;
    size_type max_mark;
    size_type* mark;
    vertex_t* marker;
    bool* has_marker;
    bool* unsettled_v;
    size_type* neigh_mark;
    size_type* neigh_id;
    size_type count;
    size_type order;
    visitor& tri_visitor;
    vertex_id_map<graph>& vid_map;
    edge_id_map<graph>& eid_map;
  };

  class edge_settling_vis {
public:
    edge_settling_vis(graph& gg, bool* con, bool* un_v, bool* un_e,
                      size_type* mk, vertex_t* mrkr, size_type* act_deg,
                      vertex_id_map<graph>& vm, edge_id_map<graph>& em) :
      g(gg), confounding(con), unsettled_v(un_v), unsettled_e(un_e),
      mark(mk), marker(mrkr), vid_map(vm), eid_map(em), active_deg(act_deg) {}

    bool visit_test(vertex_t v) { return true; }
    void pre_visit(vertex_t v) {}
    void post_visit(vertex_t v) {}

    void operator()(edge_t e, vertex_t src, vertex_t dest)
    {
      size_type sid = get(vid_map, src);
      size_type tid = get(vid_map, dest);
      size_type eid = get(eid_map, e);

      if ((confounding[tid]) && (marker[mark[tid]] != src))
      {
        unsettled_v[sid] = true;
      }
      else
      {
        unsettled_e[eid] = false;
        mt_incr(active_deg[tid], -1);
        mt_incr(active_deg[sid], -1);
      }
    }

private:
    bool* confounding;
    bool* unsettled_v;
    bool* unsettled_e;
    size_type* mark;
    size_type* active_deg;
    vertex_t* marker;
    vertex_id_map<graph>& vid_map;
    edge_id_map<graph>& eid_map;
    graph& g;
  };

  void conflict_resolution_phase(size_type* active_deg)
  {
    size_type order = num_vertices(g);
    vertex_id_map<graph> vid_map = get(_vertex_id_map, g);
    edge_id_map<graph>   eid_map = get(_edge_id_map, g);

    // Some of the independent set vertices will be settled
    // now; we'll find them.  First, we'll mark all of them
    // as 'settled', then we'll unmark the confounded ones.
    #pragma mta assert nodep
    for (size_type i = 0; i < order; i++)
    {
      if (indset[i]) unsettled_v[i] = false;
    }

    bool* c = (bool*) malloc(sizeof(bool) * order);

    #pragma mta assert nodep
    for (size_type i = 0; i < order; i++) c[i] = (confounding[i] > 0);

    size_type con_count = 0;
    size_type is_count = 0;
    size_type set_v_count = 0;
    size_type set_e_count = 0;
    size_type size = num_edges(g);

    edge_settling_vis cv(g, c, unsettled_v, unsettled_e,
                         mark, marker, active_deg, vid_map, eid_map);
    visit_adj<graph, edge_settling_vis>(g, cv, indset, unsettled_e);

    confounding_vis cfv(g, mark, has_marker, marker, max_mark,
                        tri_visitor, unsettled_v, vid_map, eid_map);
    visit_adj<graph, confounding_vis>(g, cfv, c, unsettled_e);

    for (size_type i = 0; i < order; i++)
    {
      if (indset[i]) is_count++;
    }

    for (size_type i = 0; i < order; i++)
    {
      if (!unsettled_v[i]) set_v_count++;
    }

    for (size_type i = 0; i < order; i++)
    {
      if (c[i]) con_count++;
    }

    for (size_type i = 0; i < size; i++)
    {
      if (!unsettled_e[i]) set_e_count++;
    }

    free(c);
  }

  void cleanup(graph& g, bool* unsettled_v, bool* unsettled_e,
               size_type* active_deg, vertex_id_map<graph>& vid_map,
               edge_id_map<graph>& eid_map)
  {
    // Degree one vertices in the remaining graph can settle themselves.
    typedef typename graph_traits<graph>::edge_descriptor edge_t;
    typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
    typedef typename graph_traits<graph>::vertex_iterator v_iter_t;
    typedef typename graph_traits<graph>::out_edge_iterator e_iter_t;

    size_type num_cleaned = 0;

    size_type order = num_vertices(g);
    v_iter_t verts = vertices(g);

    #pragma mta assert parallel
    for (size_type i = 0; i < order; i++)
    {
      vertex_t v = verts[i];
      size_type vid = get(vid_map, v);

      if (unsettled_v[vid] && (active_deg[vid] <= 1))
      {
        unsettled_v[vid] = false;
        size_type deg = out_degree(v, g);
        e_iter_t inc_edges = out_edges(v, g);

        #pragma mta assert parallel
        for (size_type j = 0; j < deg; j++)
        {
          edge_t e = inc_edges[j];
          vertex_t src = source(e, g);
          vertex_t trg = target(e, g);
          size_type sid = get(vid_map, src);
          size_type tid = get(vid_map, trg);
          size_type eid = get(eid_map, e);

          if (unsettled_e[eid])
          {
            unsettled_e[eid] = false;
            mt_incr(num_cleaned, 1);
            size_type edge_loser;

            if (sid == vid)
            {
              edge_loser = tid;
            }
            else
            {
              edge_loser = sid;
            }

            active_deg[edge_loser]--;
          }
        }
      }
    }
  }

  int run()
  {
    typedef typename graph_traits<graph>::edge_descriptor edge_t;

    vertex_id_map<graph> vid_map = get(_vertex_id_map, g);
    edge_id_map<graph> eid_map = get(_edge_id_map, g);
    size_type order = num_vertices(g);
    size_type size = num_edges(g);

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

    size_type* active_deg = (size_type*) malloc(order * sizeof(size_type));

    vertex_iterator verts = vertices(g);

    #pragma mta assert nodep
    for (size_type i = 0; i < order; i++)
    {
      active_deg[i] = out_degree(verts[i], g);
    }

    bool done = false;
    int round = 0;

    while (!done)
    {
      done = true;

      mark_phase(round++, active_deg);
      edge_sweep_phase();
      conflict_resolution_phase(active_deg);
      cleanup(g, unsettled_v, unsettled_e, active_deg, vid_map, eid_map);

      free(indset);
      free(marker);

      size_type sv_ct = 0;
      size_type se_ct = 0;

      #pragma mta assert nodep
      for (size_type i = 0; i < order; i++)
      {
        if (!unsettled_v[i]) sv_ct++;
      }

      if (sv_ct < order) done = false;

      for (size_type i = 0; i < size; i++)
      {
        if (!unsettled_e[i]) se_ct++;
      }

      printf("tri settled v: %lu\n", (long unsigned) sv_ct);
      printf("tri settled e: %lu\n", (long unsigned) se_ct);
    }

    mttimer.stop();
    long postticks = mttimer.getElapsedTicks();

#ifdef __MTA__
    issues = mta_get_task_counter(RT_ISSUES) - issues;
    memrefs = mta_get_task_counter(RT_MEMREFS) - memrefs;
    m_nops = mta_get_task_counter(RT_M_NOP)  - m_nops;
    a_nops = mta_get_task_counter(RT_A_NOP)  - a_nops;
    a_nops = mta_get_task_counter(RT_FLOAT_TOTAL) - fp_ops;
#endif

    return postticks;
  }

private:
  graph& g;
  visitor&  tri_visitor;
  bool* indset;
  int* confounding;
  size_type* mark;
  size_type max_mark;
  bool* unsettled_v;
  bool* unsettled_e;
  bool* has_marker;
  vertex_t* marker;
  size_type* subproblem;
  size_type my_subproblem;
  size_type n_unsettled_v;
};

static int uint_cmp(const void* u, const void* v)
{
  return (int) ((*(uint64_t*) u) - (*(uint64_t*) v));
}

template <typename size_type>
class tri_edge_struct {
public:
  tri_edge_struct(uint64_t k = 0, size_type et = 0) : key(k), eid(et) {}
  uint64_t key;    // s * order + t   : a hash/search key
  size_type eid;   // mtgl edge id: good for property map lookups
};

template <typename size_type>
static int e_cmp(const void* u, const void* v)
{
  uint64_t ukey = ((tri_edge_struct<size_type>*)u)->key;
  uint64_t vkey = ((tri_edge_struct<size_type>*)v)->key;
  return (int) (ukey - vkey);
}

template <typename size_type>
static inline 
size_type endpoint_winner(size_type *deg, size_type src, size_type dest)
{
     size_type winner = -1;
     if (deg[src] < deg[dest]) {
          winner = src;
     } else if (deg[src] > deg[dest]) {
          winner = dest;
     } else if (src < dest) {
          winner = src;
     } else {
          winner =dest;
     }
     return winner;
}

template <typename graph, typename visitor = default_triangles_visitor<graph> >
class find_triangles {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;
  typedef typename graph_traits<graph>::vertex_iterator vertex_iterator;
  typedef typename graph_traits<graph>::edge_iterator edge_iterator;

  find_triangles(graph& gg, visitor& vis) :
    g(gg), tri_visitor(vis), threshold(num_vertices(g)) {}

  find_triangles(graph& gg, visitor& vis, size_type thresh) :
    g(gg), tri_visitor(vis), threshold(thresh) {}

  ~find_triangles() {}

  void run()
  {
    size_type order = num_vertices(g);
    size_type size = num_edges(g) * 2;

    vertex_id_map<graph> vid_map = get(_vertex_id_map, g);
    edge_id_map<graph> eid_map = get(_edge_id_map, g);

    if (order < 100) print(g);

    size_type* deg = (size_type*) malloc(sizeof(size_type) * order);
    size_type* next = (size_type*) malloc(sizeof(size_type) * order);
    size_type* my_edges  = (size_type*) malloc(sizeof(size_type) * order);

    vertex_iterator verts = vertices(g);

    #pragma mta assert parallel
    for (size_type i = 0; i < order; i++)
    {
      deg[i] = out_degree(verts[i], g);
      next[i] = 0;
      my_edges[i] = 0;
    }

    size_type* prefix_counts = 0;
    size_type* started_nodes = 0;
    size_type num_threads;

    size_type m = num_edges(g);
    edge_iterator edgs = edges(g);
    for (size_type i = 0; i < m; i++)
    {
      edge_t e = edgs[i];
      size_type eid = get(eid_map, e);
      size_type src_id = get(vid_map, source(e, g));
      size_type dest_id = get(vid_map, target(e, g));
      size_type dsrc = deg[src_id];
      size_type ddst = deg[dest_id];

      // We assume we'll see each undir. edge as two dir. edg.
      if ((dsrc > threshold) || (ddst > threshold)) continue;

      size_type smaller_deg_v = endpoint_winner(deg, src_id, dest_id);

      mt_incr(my_edges[smaller_deg_v], 1);
    }

    size_type* start = (size_type*) malloc(sizeof(size_type*) * (order + 1));
    start[0] = 0;

    for (size_type i = 1; i <= order; i++)
    {
      start[i] = start[i - 1] + my_edges[i - 1];
    }

    size_type max_bucket_size = 0;
    size_type max_degree = 0;

    #pragma mta assert nodep
    for (size_type i = 0; i < order; i++)
    {
      if (my_edges[i] > max_bucket_size) max_bucket_size = my_edges[i];
    }

    #pragma mta assert nodep
    for (size_type i = 0; i < order; i++)
    {
      if (deg[i] > max_degree) max_degree = deg[i];
    }

    tri_edge_struct<size_type>* ekeys =
      (tri_edge_struct<size_type>*)
      malloc(sizeof(tri_edge_struct<size_type>) * size);

    for (size_type i = 0; i < m; i++)
    {
      edge_t e = edgs[i];
      size_type eid = get(eid_map, e);
      size_type src_id = get(vid_map, source(e, g));
      size_type dest_id = get(vid_map, target(e, g));
      size_type dsrc = deg[src_id];
      size_type ddst = deg[dest_id];

      //if (is_undirected(g) && (src_id > dest_id)) continue;

      // We assume we'll see each undir. edge as two dir. edg.
      if ((dsrc > threshold) || (ddst > threshold)) continue;

      size_type smaller_deg_v = endpoint_winner(deg, src_id, dest_id);
      order_pair(src_id, dest_id);
      uint64_t ekey = src_id * order + dest_id;
      size_type pos = mt_incr(next[smaller_deg_v], 1);
      ekeys[start[smaller_deg_v] + pos] = tri_edge_struct<size_type>(ekey, eid);
    }

    free(prefix_counts);
    free(started_nodes);

    #pragma mta assert parallel
    for (size_type i = 0; i < order; i++)
    {
      qsort((void*) &ekeys[start[i]], my_edges[i],
            sizeof(tri_edge_struct<size_type>), e_cmp<size_type>);
    }

    size_type tri = 0;

    #pragma mta assert parallel // nodep if visitor is "simple"
    for (size_type i = 0; i < order; i++)
    {
      visitor my_visitor = tri_visitor;

      for (size_type j = 0; j < my_edges[i]; j++)
      {
        size_type eid1 = ekeys[start[i] + j].eid;
        size_type src1 = get(vid_map, source(edgs[eid1], g));
        size_type dest1 = get(vid_map, target(edgs[eid1], g));

        for (size_type k = j + 1; k < my_edges[i]; k++)
        {
          size_type eid2 = ekeys[start[i] + k].eid;
          size_type src2 = get(vid_map, source(edgs[eid2], g));
          size_type dest2 = get(vid_map, target(edgs[eid2], g));
          size_type s, t, v;
          s = (src1 == i) ? dest1 : src1;
          t = (src2 == i) ? dest2 : src2;
          order_pair(s, t);
          tri_edge_struct<size_type>* eresult;
          tri_edge_struct<size_type> ekey(s * order + t, 0);
          v = endpoint_winner(deg, s, t);
          if ((eresult = (tri_edge_struct<size_type>*)bsearch((void*) &ekey,
                                                              (void*) &ekeys[start[v]], my_edges[v],
                                                              sizeof(tri_edge_struct<size_type>),
                                                              e_cmp<size_type>)) != 0)
          {
            my_visitor(eid1, eid2, eresult->eid);
            mt_incr(tri, 1);
          }
        }
      }
    }

    #pragma mta trace "done"
    printf("max b: %lu\n", max_bucket_size);
    printf("max d: %lu\n", max_degree);
    printf("num tri: %lu\n", tri);
    fflush(stdout);

    return;
  }

private:
  graph& g;
  visitor& tri_visitor;
  size_type threshold;
};

}

#endif

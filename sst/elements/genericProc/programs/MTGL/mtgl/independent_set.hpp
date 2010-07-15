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
/*! \file independent_set.hpp

    \brief This file contains multithreaded independent set and maximal
           independent set algorithms.  These are currently set to favor
           high degree vertices in the independent set rather than the most
           vertices.  This affects the performance of the triangle finding
           algorithm, for example.

    \author Jon Berry (jberry@sandia.gov)

    \date 2007
*/
/****************************************************************************/

#ifndef MTGL_INDEPENDENT_SET_HPP
#define MTGL_INDEPENDENT_SET_HPP

#include <climits>

#include <mtgl/visit_adj.hpp>

namespace mtgl {

#ifdef USING_QTHREADS
template <class graph>
class qt_luby_game_args {
public:
  typedef typename graph_traits<graph>::size_type count_t;

  qt_luby_game_args(graph& gg, count_t* actind,
                    bool* act, bool* indst, double* val) :
    g(gg), active_ind(actind), active(act), indset(indst), value(val) {}

  graph& g;
  count_t* active_ind;
  bool* indset;
  bool* active;
  double* value;
};

template <class graph>
void luby_game_outer(qthread_t* me, const size_t startat,
                     const size_t stopat, void* arg)
{
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::size_type count_t;

  qt_luby_game_args<graph>* qt_linfo = (qt_luby_game_args<graph>*)arg;

  for (size_t k = startat; k < stopat; k++)
  {
    count_t* my_active_ind = qt_linfo->active_ind;
    count_t i = my_active_ind[k];
    count_t begin = qt_linfo->g[i];
    count_t end = qt_linfo->g[i + 1];

    bool* my_indset = qt_linfo->indset;
    bool* my_active = qt_linfo->active;
    double* my_value = qt_linfo->value;
    vertex_t* my_end_points = qt_linfo->g.get_end_points();

    for (count_t j = begin; j < end; j++)
    {
      count_t dest = (count_t) my_end_points[j];

      if (my_active[dest])
      {
        double cs = my_value[i];
        double cd = my_value[dest];

        if (!(my_indset[i] && my_indset[dest])) continue;

        if (cs < cd)
        {
          my_indset[i] = false;
        }
        else if (cs > cd)
        {
          my_indset[dest] = false;
        }
        else if (i < dest)
        {
          my_indset[dest] = false;
        }
        else if (i > dest)
        {
          my_indset[i] = false;
        }
      }
    }
  }
}

template <class graph>
class qt_luby_deactivate_args {
public:
  typedef typename graph_traits<graph>::size_type count_t;

  qt_luby_deactivate_args(graph& gg, count_t* actind,
                          bool* act, bool* indst, int& new_indset_size) :
    g(gg), active_ind(actind), active(act), indset(indst),
    new_ind_set_size(new_indset_size) {}

  graph& g;
  count_t* active_ind;
  bool* indset;
  bool* active;
  int& new_ind_set_size;
};

template <class graph>
void luby_deactivate_outer(qthread_t* me, const size_t startat,
                           const size_t stopat, void* arg)
{
  typedef typename graph_traits<graph>::size_type count_t;
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;

  qt_luby_deactivate_args<graph>* qt_linfo =
    (qt_luby_deactivate_args<graph>*) arg;

  for (size_t k = startat; k < stopat; k++)
  {
    count_t* my_active_ind = qt_linfo->active_ind;
    count_t i = my_active_ind[k];
    count_t begin = qt_linfo->g[i];
    count_t end = qt_linfo->g[i + 1];
    bool* my_indset = qt_linfo->indset;
    bool* my_active = qt_linfo->active;
    int& new_ind_set_size = qt_linfo->new_ind_set_size;
    vertex_t* my_end_points = qt_linfo->g.get_end_points();

    if (my_indset[i])
    {
      mt_incr(new_ind_set_size, 1);

      if (my_active[i]) my_active[i] = false;
    }

    for (count_t j = begin; j < end; j++)
    {
      count_t dest = (count_t) my_end_points[j];

      if (my_indset[i] && my_active[dest])
      {
        my_active[dest] = false;
      }
    }
  }
}

template <class graph>
class qt_luby_setup_args {
public:
  typedef typename graph_traits<graph>::size_type count_t;

  qt_luby_setup_args(graph& gg, count_t* actind, int* next_actind,
                     bool* act, bool* indst, int& new_numactive) :
    g(gg), active_ind(actind), next_active_ind(next_actind),
    active(act), indset(indst), new_num_active(new_numactive) {}

  graph& g;
  count_t* active_ind;
  int* next_active_ind;
  bool* indset;
  bool* active;
  int& new_num_active;
};

template <class graph>
void luby_setup_outer(qthread_t* me,
                      const size_t startat, const size_t stopat, void* arg)
{
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;

  qt_luby_setup_args<graph>* qt_linfo = (qt_luby_setup_args<graph>*)arg;

  for (size_t k = startat; k < stopat; k++)
  {
    int* my_active_ind = qt_linfo->active_ind;
    int* my_next_active_ind = qt_linfo->next_active_ind;
    int i = my_active_ind[k];
    bool* my_indset = qt_linfo->indset;
    bool* my_active = qt_linfo->active;
    int& new_num_active = qt_linfo->new_num_active;

    if (my_active[i])
    {
      int pos = mt_incr(new_num_active, 1);
      my_next_active_ind[pos] = i;
      my_indset[i] = true;
    }
  }
}

#endif

///
template <typename graph, typename result_type,
          typename graph_traits<graph>::size_type maxdeg = INT_MAX>
class ind_set_visitor {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::size_type count_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;

  ind_set_visitor(graph& gg, vertex_id_map<graph>& vm,
                  result_type in_s, count_t* active_d, bool* actve = 0) :
    g(gg), in_set(in_s), active_v(actve), vid_map(vm), active_deg(active_d) {}

  bool visit_test(edge_t e)
  {
    vertex_t v1 = source(e, g);
    vertex_t v2 = target(e, g);
    count_t v1id = get(vid_map, v1);
    count_t v2id = get(vid_map, v2);

    return (!active_v || (active_v[v1id] && active_v[v2id]))
           && (out_degree(v1, g) <= maxdeg) && (out_degree(v2, g) <= maxdeg);
  }

  void pre_visit(edge_t e) {}
  void post_visit(edge_t e) {}

  void operator()(edge_t e)
  {
    vertex_t v1 = source(e, g);
    vertex_t v2 = target(e, g);
    count_t v1id = get(vid_map, v1);
    count_t v2id = get(vid_map, v2);
    count_t d1  = (active_deg) ? active_deg[v1id] : 0;
    count_t d2  = (active_deg) ? active_deg[v2id] : 0;

    if (d1 > d2)
    {
      in_set[v2id] = false;
    }
    else if (d1 < d2)
    {
      in_set[v1id] = false;
    }
    else if (v1id < v2id)
    {
      in_set[v2id] = false;
    }
    else if (v2id > v1id)
    {
      in_set[v1id] = false;
    }
  }

private:
  graph& g;
  vertex_id_map<graph>& vid_map;
  result_type in_set;
  bool* active_v;
  count_t* active_deg;
};

template <typename graph, typename result_type,
          typename graph_traits<graph>::size_type maxdeg = INT_MAX>
class ind_set_visitor2 {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::size_type count_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;

  ind_set_visitor2(graph& gg, vertex_id_map<graph>& vm, edge_id_map<graph>& em,
                   result_type in_s, bool* act_v, bool* act_e,
                   count_t* active_d) :
    g(gg), in_set(in_s), active_v(act_v), active_e(act_e),
    vid_map(vm), eid_map(em), active_deg(active_d) {}

  bool visit_test(edge_t e)
  {
    count_t eid = get(eid_map, e);

    if (!active_e[eid]) return false;

    vertex_t v1 = source(e, g);
    vertex_t v2 = target(e, g);
    count_t v1id = get(vid_map, v1);
    count_t v2id = get(vid_map, v2);
    assert(active_v != 0);

    if (!active_v[v1id] || !active_v[v2id]) return false;

    // JWB: degree matters?
    return ((out_degree(v1, g) <= maxdeg) && (out_degree(v2, g) <= maxdeg));
  }

  void pre_visit(edge_t e) {}

  void operator()(edge_t e)
  {
    vertex_t v1 = source(e, g);
    vertex_t v2 = target(e, g);
    count_t v1id = get(vid_map, v1);
    count_t v2id = get(vid_map, v2);
    count_t d1  = (active_deg) ? active_deg[v1id] : 0;
    count_t d2  = (active_deg) ? active_deg[v2id] : 0;

    assert(d1 >= 0);
    assert(d2 >= 0);

    if (d1 > d2)
    {
      in_set[v2id] = false;
    }
    else if (d1 < d2)
    {
      in_set[v1id] = false;
    }
    else if (v1id < v2id)
    {
      in_set[v2id] = false;
    }
    else if (v1id > v2id)
    {
      in_set[v1id] = false;
    }
  }

private:
  graph& g;
  vertex_id_map<graph>& vid_map;
  edge_id_map<graph>&   eid_map;
  result_type in_set;
  bool* active_v;
  bool* active_e;
  count_t* active_deg;
};

///
template <typename graph, typename result_type>
class independent_set {
public:
  typedef typename graph_traits<graph>::size_type count_t;

  independent_set(graph& gg, int* subprob, int my_subprob) :
    g(gg), has_emask(false), i_own_active(true), active_deg((count_t) 0)
  {
    count_t order = num_vertices(g);
    active_v = (bool*) malloc(order * sizeof(bool));

    if (subprob)
    {
      #pragma mta assert nodep
      for (count_t i = 0; i < order; ++i)
      {
        active_v[i] = (subprob[i] == my_subprob);
      }
    }
    else
    {
      #pragma mta assert nodep
      for (count_t i = 0; i < order; ++i)
      {
        active_v[i] = true;
      }
    }
  }

  independent_set(graph& gg, bool* subprob = 0, bool emsk = false) :
    g(gg), active_v(subprob), active_e(subprob),
    has_emask(emsk), i_own_active(false)
  {
    // is this constructor used?
    active_deg = 0;

    if (has_emask)
    {
      active_v = 0;
    }
    else
    {
      active_e = 0;
    }
  }

  independent_set(graph& gg, count_t* act_d, bool* act_v, bool* act_e) :
    g(gg), active_deg(act_d), active_v(act_v), active_e(act_e),
    has_emask(true), i_own_active(false) {}

  ~independent_set()
  {
    if (i_own_active)
    {
      free(active_v);

      if (active_deg) free(active_deg);
    }
  }

  void run(result_type& result, count_t& issize)
  {
    count_t order = num_vertices(g);
    count_t* sums = (count_t*) malloc(sizeof(count_t) * order);
    vertex_id_map<graph> vid_map = get(_vertex_id_map, g);
    edge_id_map<graph> eid_map = get(_edge_id_map, g);
    issize = 0;

    if (active_v)
    {
      #pragma mta assert nodep
      for (count_t i = 0; i < order; i++)
      {
        result[i] = active_v[i];
      }
    }
    else
    {
      #pragma mta assert nodep
      for (count_t i = 0; i < order; i++)
      {
        result[i] = 1;
      }
    }

    if (has_emask)
    {
      ind_set_visitor2<graph, result_type>isv(g, vid_map, eid_map, result,
                                              active_v, active_e, active_deg);
      visit_edges_filtered(g, isv);
    }
    else
    {
      ind_set_visitor<graph, result_type> isv(g, vid_map, result, active_deg,
                                              active_v);
      visit_edges_filtered(g, isv);
    }

    prefix_sums<bool, count_t>(result, sums, order);
    issize = sums[order - 1];

    free(sums);
  }

private:
  bool* active_v;
  bool* active_e;
  count_t* active_deg;
  graph& g;
  bool has_emask;
  bool i_own_active;
};

template <typename graph, typename is_type,
          typename graph_traits<graph>::size_type maxdeg = INT_MAX>
class turnoff {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::size_type count_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;

  turnoff(graph& gg, vertex_id_map<graph>& vm, edge_id_map<graph>& em,
          is_type& ind_s, bool* a, count_t& ct) :
    g(gg), vid_map(vm), eid_map(em), ind_set(ind_s), active_v(a), count(ct) {}

  bool visit_test(edge_t e)
  {
    vertex_t v1 = source(e, g);
    vertex_t v2 = target(e, g);
    count_t v1id = get(vid_map, v1);
    count_t v2id = get(vid_map, v2);

    return (!active_v || (active_v[v1id] && active_v[v2id]))
           && (out_degree(v1, g) <= maxdeg) && (out_degree(v2, g) <= maxdeg);
  }

  void pre_visit(edge_t e)  {}

  void operator() (edge_t e)
  {
    vertex_t v1 = source(e, g);
    vertex_t v2 = target(e, g);
    count_t sid = get(vid_map, v1);
    count_t tid = get(vid_map, v2);

    if (ind_set[sid])
    {
      active_v[tid] = false;
    }
    else if (ind_set[tid])
    {
      active_v[sid] = false;
    }

    // don't deactivate indset verts yet; would disrupt visit_test.
  }

private:
  graph& g;
  vertex_id_map<graph>& vid_map;
  edge_id_map<graph>&   eid_map;
  is_type& ind_set;
  bool* active_v;
  count_t& count;
};

template <typename graph, typename is_type,
          typename graph_traits<graph>::size_type maxdeg = INT_MAX>
class turnoff2 {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::size_type count_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;

  turnoff2(graph& gg, vertex_id_map<graph>& vm, edge_id_map<graph>& em,
           is_type& ind_s, bool* av, bool* ae, count_t& ct) :
    g(gg), ind_set(ind_s), active_v(av), active_e(ae),
    count(ct), vid_map(vm), eid_map(em) {}

  bool visit_test(edge_t e)
  {
    count_t eid = get(eid_map, e);

    if (!active_e[eid]) return false;

    vertex_t v1 = source(e, g);
    vertex_t v2 = target(e, g);

    // JWB: do we care about degree anymore?
    return ((out_degree(v1, g) <= maxdeg) && (out_degree(v2, g) <= maxdeg));
  }

  void pre_visit(edge_t e)  {}

  void operator() (edge_t e)
  {
    vertex_t v1 = source(e, g);
    vertex_t v2 = target(e, g);
    count_t sid = get(vid_map, v1);
    count_t tid = get(vid_map, v2);

    if (ind_set[sid])
    {
      active_v[tid] = false;
    }
    else if (ind_set[tid])
    {
      active_v[sid] = false;
    }

    // don't deactivate indset verts yet; would disrupt visit_test.
  }

private:
  graph& g;
  vertex_id_map<graph>& vid_map;
  edge_id_map<graph>&   eid_map;
  is_type& ind_set;
  bool* active_v;
  bool* active_e;
  count_t& count;
};

template <typename graph, typename result_type, int parcut = 20>
class maximal_independent_set {
public:
  typedef typename graph_traits<graph>::size_type count_t;
  maximal_independent_set(graph& gg, int* subprob, int my_subprob) :
    g(gg), active_deg((count_t) 0)
  {
    // what if result type is a map?  need to handle this
    count_t order = num_vertices(g);
    active_v = (bool*) malloc(order * sizeof(bool));

    if (subprob)
    {
      #pragma mta assert nodep
      for (count_t i = 0; i < order; ++i)
      {
        active_v[i] = (subprob[i] == my_subprob);
      }
    }
    else
    {
      #pragma mta assert nodep
      for (count_t i = 0; i < order; ++i)
      {
        active_v[i] = true;
      }
    }
  }

  maximal_independent_set(graph& gg, bool* subprob = 0) :
    g(gg), active_e(0), active_deg((count_t) 0)
  {
    count_t order = num_vertices(g);
    active_v = (bool*) malloc(order * sizeof(bool));
    active_deg = (count_t*) malloc(order * sizeof(count_t));

    if (subprob)
    {
      #pragma mta assert nodep
      for (count_t i = 0; i < order; i++)
      {
        active_v[i] = subprob[i];
      }
    }
    else
    {
      #pragma mta assert nodep
      for (count_t i = 0; i < order; i++)
      {
        active_v[i] = true;
      }
    }
  }

  maximal_independent_set(graph& gg, count_t* act_d, bool* act_v, bool* act_e) :
    g(gg), active_e(act_e), active_deg(act_d)
  {
    count_t order = num_vertices(g);

    if (!act_v)
    {
      fprintf(stderr, "error: mis(g,active_v,active_e): "
              "active_v cannot be null\n");
      exit(1);
    }

    active_v = (bool*) malloc(order * sizeof(bool));

    if (act_v)
    {
      #pragma mta assert nodep
      for (count_t i = 0; i < order; i++)
      {
        active_v[i] = act_v[i];
      }
    }
    else
    {
      #pragma mta assert nodep
      for (count_t i = 0; i < order; i++)
      {
        active_v[i] = true;
      }
    }
  }

  ~maximal_independent_set() { free(active_v); }

  void run(result_type& result, count_t& issize)
  {
    count_t i;
    count_t order = num_vertices(g);
    int round = 1;
    count_t tcount = 0;
    count_t prev_issize = 0;
    bool* backup = (bool*) malloc(sizeof(bool) * order);
    vertex_id_map<graph> vid_map = get(_vertex_id_map, g);
    edge_id_map<graph> eid_map = get(_edge_id_map, g);

    #pragma mta assert nodep
    for (count_t i = 0; i < order; i++) backup[i] = false;

    issize = 0;

    while (1)
    {
      count_t next_issize = 0;

      if (active_e)
      {
        independent_set<graph, bool*>
        is(g, active_deg, active_v, active_e);
        is.run(result, next_issize);

        #pragma mta assert nodep
        for (count_t i = 0; i < order; i++)
        {
          backup[i] = result[i] | backup[i];
        }

        if (next_issize <= 0) break;

        issize += next_issize;

        turnoff2<graph, bool*> toff(g, vid_map, eid_map, result,
                                    active_v, active_e, tcount);
        visit_edges_filtered(g, toff);
      }
      else
      {
        independent_set<graph, bool*> is(g, active_v);
        is.run(result, next_issize);

        #pragma mta assert nodep
        for (count_t i = 0; i < order; i++)
        {
          backup[i] = result[i] | backup[i];
        }

        if (next_issize <= 0) break;

        issize += next_issize;

        turnoff<graph, bool*> toff(g, vid_map, eid_map,
                                   result, active_v, tcount);
        visit_edges_filtered(g, toff);
      }

      #pragma mta assert nodep
      for (count_t i = 0; i < order; ++i)
      {
        if (result[i])
        {
          active_v[i] = false;
        }
        if (active_v[i])
        {
          result[i] = true;
        }
      }

      round++;
      prev_issize = issize;
    }

    #pragma mta assert nodep
    for (count_t i = 0; i < order; i++) result[i] = backup[i];

    free(backup);
  }

private:
  graph& g;
  count_t* active_deg;
  bool* active_v;
  bool* active_e;
};

template <typename graph>
class luby_maximal_independent_set {
public:
  typedef typename graph_traits<graph>::size_type count_t;
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;

  luby_maximal_independent_set(graph& gg, bool* act = 0) :
    g(gg), active(act), i_own_active(false)
  {
    if (act == 0)
    {
      count_t order = num_vertices(g);
      i_own_active = true;
      active = new bool[order];
      memset(active, 0, sizeof(bool) * order);
    }
  }

  ~luby_maximal_independent_set() { if (i_own_active) delete active; }

  void run(bool* result, count_t& issize)
  {
    count_t i, j, order = num_vertices(g);
    mt_timer timer;
    double* value = new double[order];
    bool* indset = new bool[order];
    count_t* active_ind = new count_t[order];
    count_t* next_active_ind = new count_t[order];
    memset(value, 0, order * sizeof(double));
    memset(indset, 1, order * sizeof(bool));
    memset(active, 1, order * sizeof(bool));

    count_t prev_ind_set_size = 0, ind_set_size = 0;
    vertex_t* end_points = g.get_end_points();

    #pragma mta assert nodep
    for (int i = 0; i < order; i++) active_ind[i] = i;

    int num_active = order;

    vertex_id_map<graph> vid_map = get(_vertex_id_map, g);

    do
    {
      prev_ind_set_size = ind_set_size;

#ifdef USING_QTHREADS
      qt_luby_game_args<graph> la(g, active_ind, active, indset, value);
      qt_loop_balance(0, num_active, luby_game_outer<graph>, &la);
#else
      #pragma mta trace "about to run contest"
      #pragma mta assert nodep
      for (count_t a = 0; a < num_active; a++)
      {
        count_t i = active_ind[a];
        count_t begin = g[i];
        count_t end = g[i + 1];
        bool* my_indset = indset;
        double* my_value = value;
        vertex_t* my_end_points = end_points;

        #pragma mta assert nodep
        for (count_t j = begin; j < end; j++)
        {
          count_t dest = get(vid_map, my_end_points[j]);

          if (active[dest])
          {
            double cs = my_value[i];
            double cd = my_value[(count_t) dest];

            if (!(my_indset[i] && my_indset[dest])) continue;

            if (cs < cd)
            {
              my_indset[i] = false;
            }
            else if (cs > cd)
            {
              my_indset[dest] = false;
            }
            else if (i < dest)
            {
              my_indset[dest] = false;
            }
            else if (i > dest)
            {
              my_indset[i] = false;
            }
          }
        }
      }
#endif

      #pragma mta trace "about to deactivate losers"

      int new_ind_set_size = 0;

#ifdef USING_QTHREADS
      qt_luby_deactivate_args<graph> da(g, active_ind, active, indset,
                                        new_ind_set_size);
      qt_loop_balance(0, num_active, luby_deactivate_outer<graph>, &da);
#else
      #pragma mta assert nodep
      for (int a = 0; a < num_active; a++)
      {
        int i = active_ind[a];
        int begin = g[i];
        int end = g[i + 1];

        if (indset[i])
        {
          new_ind_set_size++;

          if (active[i]) active[i] = false;
        }

        #pragma mta assert nodep
        for (int j = begin; j < end; j++)
        {
          int dest = end_points[j];

          if (indset[i] && active[dest]) active[dest] = false;
        }
      }
#endif

      int new_num_active = 0;
      ind_set_size += new_ind_set_size;

      #pragma mta trace "about to setup next round"
      #pragma mta assert nodep
      for (int a = 0; a < num_active; a++)
      {
        int i = active_ind[a];

        if (active[i])
        {
          int pos = mt_incr(new_num_active, 1);
          next_active_ind[pos] = i;
          indset[i] = true;
        }
      }

      count_t* tmp = next_active_ind;
      next_active_ind = active_ind;
      active_ind = tmp;
      num_active = new_num_active;
      printf("size: %d, active: %d\n", (int)ind_set_size, (int)num_active);

    } while (prev_ind_set_size < ind_set_size);

#ifdef __MTA__
    int issues, memrefs, concur, streams;
    sample_mta_counters(timer, issues, memrefs, concur, streams);
    printf("indset rank performance stats: \n");
    printf("---------------------------------------------\n");
    print_mta_counters(timer, num_edges(g), issues, memrefs, concur, streams);
#endif

    printf("size: %d\n", (int)ind_set_size);
    valid_max_ind_set(g, indset);
  }

private:
  graph& g;
  bool* active;
  bool i_own_active;
};

}

#endif

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
/*! \file snl_community.hpp

    \author Jon Berry (jberry@sandia.gov)

    \date 7/2007
*/
/****************************************************************************/

#ifndef MTGL_SNL_COMMUNITY_HPP
#define MTGL_SNL_COMMUNITY_HPP

#include <cstdio>
#include <climits>
#include <cmath>

#include <mtgl/mtgl_adapter.hpp>
#include <mtgl/util.hpp>
#include <mtgl/triangles.hpp>
#include <mtgl/ufl.h>
#include <mtgl/metrics.hpp>
#include <mtgl/rand_fill.hpp>

#ifdef __MTA__
#include <sys/mta_task.h>
#include <machine/runtime.h>
#endif

namespace mtgl {

#if 0
WCM: only used in class m_e_vis so I moved this into that class as a
protected method since having a function definition outside a class
in a.h * file causes multiple definition errors

int m_e_numerator(int* a, int lower, int upper)
{
  if (lower >= upper) return 0;

  int mid = lower + (upper - lower) / 2;
  int sum1 = 0, sum2 = 0;

  for (int i = lower; i <= mid; i++)      // Does recomputation.
  {
    sum1 += a[i];
  }

  for (int i = mid + 1; i <= upper; i++)  // Does recomputation.
  {
    sum2 += a[i];
  }

  if (lower >= upper - 1)
  {
    return sum1 * sum2;
  }

  int result = sum1 * sum2 + m_e_numerator(a, lower, mid) +
               m_e_numerator(a, mid + 1, upper);

  return result;
}
#endif

template <class graph>
class m_e_vis {
//  Independent set vertices try to mark their neighbors.
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;

  m_e_vis(graph& gg, vertex_id_map<graph>& vm, int* m_e_num) :
    g(gg), vid_map(vm), m_e_numer(m_e_num) {}

  bool visit_test(vertex_t v) { return true; }

  void pre_visit(vertex_t v)
  {
    vdeg = out_degree(v, g);
    degrees = new int[vdeg + 1];
    degrees[0] = vdeg;
    count = 1;
  }

  void operator()(edge_t e, vertex_t src, vertex_t dest)
  {
    int next = mt_incr(count, 1);
    degrees[next] = out_degree(dest, g);
  }

  void post_visit(vertex_t v)
  {
    int m_e_num = m_e_numerator(degrees, 0, vdeg);
    int vid = get(vid_map, v);
    m_e_numer[vid] = m_e_num;
    delete[] degrees;
  }

protected:
  // wcm: Relocated this function here rather than leaving it outside
  //      the class declaration.
  int m_e_numerator(int* a, int lower, int upper)
  {
    if (lower >= upper) return 0;

    int mid = lower + (upper - lower) / 2;
    int sum1 = 0, sum2 = 0;

    for (int i = lower; i <= mid; i++)      // Does recomputation.
    {
      sum1 += a[i];
    }

    for (int i = mid + 1; i <= upper; i++)  // Does recomputation.
    {
      sum2 += a[i];
    }

    if (lower >= upper - 1)
    {
      return sum1 * sum2;
    }

    int result = sum1 * sum2 + m_e_numerator(a, lower, mid) +
                 m_e_numerator(a, mid + 1, upper);

    return result;
  }

private:
  graph& g;
  vertex_id_map<graph>& vid_map;
  int* degrees;
  int* m_e_numer;
  int count;
  int vdeg;
};

template <class graph>
class e_rs_vis {
//  For each vertex, suppose it is a community leader, and assume
//  that it serves all of its neighbors.  Count the number of inter-
//  community edges connecting its community to the rest of the world.
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;

  e_rs_vis(graph& gg, vertex_id_map<graph>& vm, int* one_nc, int* e_rs_nm) :
    g(gg), vid_map(vm), e_rs_num(e_rs_nm), one_neigh_size(one_nc),
    neigh_deg_count(0) {}

  bool visit_test(vertex_t v) { return true; }

  void pre_visit(vertex_t v)
  {
    int vid = get(vid_map, v);
    vdeg = out_degree(v, g);
    e_rs_num[vid] = 2 * one_neigh_size[vid];  // Degree version of Q.
  }

  void operator()(edge_t e, vertex_t src, vertex_t dest)
  {
    int sid = get(vid_map, src);
    int tid = get(vid_map, dest);

    if (sid == tid) return;

    int dest_deg = out_degree(dest, g);
    int next = mt_incr(neigh_deg_count, dest_deg - 1);
  }

  void post_visit(vertex_t v)
  {
    int vid = get(vid_map, v);
    e_rs_num[vid] += neigh_deg_count - 2 * (one_neigh_size[vid] - vdeg);

    // This won't work if there are self loops and/or mult. edges.
  }

private:
  graph& g;
  vertex_id_map<graph>& vid_map;
  int* one_neigh_size;
  int* e_rs_num;
  int neigh_deg_count;
  int vdeg;
};

template <typename graph>
class cross_community_edge_counter {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;

  cross_community_edge_counter(graph& gg, vertex_id_map<graph>& vm,
                               int* mp, int& ct) :
    g(gg), vid_map(vm), count(ct), leader_map(mp) {}

  void operator()(edge_t e)
  {
    vertex_t v1 = source(e, g);
    vertex_t v2 = target(e, g);
    int v1id = get(vid_map, v1);
    int v2id = get(vid_map, v2);

    if (leader_map[v1id] != leader_map[v2id]) mt_incr(count, 1);
  }

private:
  graph& g;
  vertex_id_map<graph>& vid_map;
  int* leader_map;
  int& count;
};

template <typename graph>
class support_init {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;

  support_init(graph& gg, vertex_id_map<graph>& vm,
               edge_id_map<graph>& em, double* psol, double* supp) :
    g(gg), vid_map(vm), eid_map(em), primal_sol(psol), support(supp) {}

  void operator()(edge_t e)
  {
    vertex_t v1 = source(e, g);
    vertex_t v2 = target(e, g);
    int v1id = get(vid_map, v1);
    int v2id = get(vid_map, v2);
    int eid =  get(eid_map, e);
    support[eid] = (1 - primal_sol[v1id]) * (1 - primal_sol[v2id]);
  }

private:
  graph& g;
  vertex_id_map<graph>& vid_map;
  edge_id_map<graph>&   eid_map;
  double* support;
  double* primal_sol;
};

/*
template <typename graph>
class subgraph_edge_constructor {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;

  subgraph_edge_constructor(graph& gg, vertex_id_map<graph>& vm,
                            int *s, int *d int *imp) :
    g(gg), vid_map(vm), srcs(s), dests(d), inversemap(imp), count(0) {}

  void operator()(edge_t e)
  {
    vertex_t v1 = source(e, g);
    vertex_t v2 = target(e, g);
    int v1id = get(vid_map, v1);
    int v2id = get(vid_map, v2);
    if (inversemap[v1id] != inversemap[v2id]) mt_incr(count, 1);
  }

private:
  graph& g;
  vertex_id_map<graph>& vid_map;
  int *leader_map;
  int &count;
};
*/

template <class graph>
class one_neighborhood_edge_count : public default_triangles_visitor<graph> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  one_neighborhood_edge_count(graph& gg, int* ct, vertex_id_map<graph>& vm) :
    g(gg), count(ct), vipm(vm) {}

  void operator()(vertex_t v1, vertex_t v2, vertex_t v3)
  {
    int v1id = get(vipm, v1);
    int v2id = get(vipm, v2);
    int v3id = get(vipm, v3);

    mt_incr(count[v1id], 1);
    mt_incr(count[v2id], 1);
    mt_incr(count[v3id], 1);
  }

private:
  int* count;
  graph& g;
  vertex_id_map<graph>& vipm;
};

template <class graph_adapter>
int foo(graph_adapter& ga,
        typename graph_traits<graph_adapter>::vertex_descriptor v,
        typename graph_traits<graph_adapter>::vertex_descriptor w,
        edge_id_map<graph_adapter>& eid_map)
{
  return out_degree(v, ga) + out_degree(w, ga);
}

template <class graph_adapter>
int get_edge_id(graph_adapter& ga,
                typename graph_traits<graph_adapter>::vertex_descriptor v,
                typename graph_traits<graph_adapter>::vertex_descriptor w,
                vertex_id_map<graph_adapter>& vid_map,
                edge_id_map<graph_adapter>& eid_map,
                int par_cutoff = 100)
{
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_t;
  typedef typename graph_traits<graph_adapter>::out_edge_iterator
          out_edge_iterator;

  int result = -1;
  int vid = get(vid_map, v);
  int wid = get(vid_map, w);
  int vdeg = out_degree(v, ga);
  int wdeg = out_degree(w, ga);

  if (vdeg <= wdeg)
  {
    out_edge_iterator inc_edges = out_edges(v, ga);

    if (vdeg >= par_cutoff)
    {
      #pragma mta assert parallel
      for (int i = 0; i < vdeg; i++)
      {
        edge_t e = inc_edges[i];
        vertex_t oth = other(e, v, ga);

        int wid = get(vid_map, w);
        int oth_id = get(vid_map, oth);

        if (wid == oth_id) result = get(eid_map, e);
      }
    }
    else
    {
      for (int i = 0; i < vdeg; i++)
      {
        edge_t e = inc_edges[i];
        vertex_t oth = other(e, v, ga);
        int vid = get(vid_map, v);
        int wid = get(vid_map, w);
        int oth_id = get(vid_map, oth);

        if (wid == oth_id)
        {
          result = get(eid_map, e);
        }

        {
          vertex_t src = source(e, ga);
          vertex_t trg = target(e, ga);
          int sid = get(vid_map, src);
          int tid = get(vid_map, trg);
          vertex_t v_oth = other(e, v, ga);
          int v_oth_id = get(vid_map, v_oth);
        }
      }
    }
  }
  else
  {
    out_edge_iterator inc_edges = out_edges(w, ga);

    if (wdeg >= par_cutoff)
    {
      #pragma mta assert parallel
      for (int i = 0; i < wdeg; i++)
      {
        edge_t e = inc_edges[i];
        vertex_t oth = other(e, w, ga);

        int wid = get(vid_map, w);
        int oth_id = get(vid_map, oth);

        if (vid == oth_id) result = get(eid_map, e);
      }
    }
    else
    {
      for (int i = 0; i < wdeg; i++)
      {
        edge_t e = inc_edges[i];
        vertex_t oth = other(e, w, ga);
        int vid = get(vid_map, v);
        int wid = get(vid_map, w);
        int oth_id = get(vid_map, oth);

        if (vid == oth_id)
        {
          result = get(eid_map, e);
        }

        {
          vertex_t src = source(e, ga);
          vertex_t trg = target(e, ga);
          int sid = get(vid_map, src);
          int tid = get(vid_map, trg);
          vertex_t w_oth = other(e, w, ga);
          int w_oth_id = get(vid_map, w_oth);
        }
      }
    }
  }

  return result;
}

template <class graph>
class edge_support_count : public default_triangles_visitor<graph> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  edge_support_count(graph& gg, double* psol, double* supp,
                     vertex_id_map<graph>& vm, edge_id_map<graph>& em) :
    g(gg), primal_sol(psol), support(supp), vid_map(vm), eid_map(em) {}

  void operator()(vertex_t v1, vertex_t v2, vertex_t v3)
  {
    int v1id = get(vid_map, v1);
    int v2id = get(vid_map, v2);
    int v3id = get(vid_map, v3);
    int f    = foo(g, v1, v2, eid_map);
    int e1id = get_edge_id(g, v1, v2, vid_map, eid_map);
    int e2id = get_edge_id(g, v2, v3, vid_map, eid_map);
    int e3id = get_edge_id(g, v3, v1, vid_map, eid_map);

    if (e1id < 0)
    {
      printf("warning: tri(%d,%d,%d): edge (%d,%d) missing\n",
             v1id, v2id, v3id, v1id, v2id);
      return;
    }

    if (e2id < 0)
    {
      printf("warning: tri(%d,%d,%d): edge (%d,%d) missing\n",
             v1id, v2id, v3id, v2id, v3id);
      return;
    }

    if (e3id < 0)
    {
      printf("warning: tri(%d,%d,%d): edge (%d,%d) missing\n",
             v1id, v2id, v3id, v3id, v2id);
      return;
    }

    assert(e1id >= 0);
    assert(e2id >= 0);
    assert(e3id >= 0);

    support[e1id] *= (1 - primal_sol[v3id]);
    support[e2id] *= (1 - primal_sol[v1id]);
    support[e3id] *= (1 - primal_sol[v2id]);
  }

private:
  double* primal_sol;
  double* support;
  graph& g;
  vertex_id_map<graph>& vid_map;
  edge_id_map<graph>& eid_map;
};

template <class graph_adapter>
class self_loop_col_iterator {
public:
  typedef typename graph_adapter::vertex_descriptor vertex_descriptor;
  typedef typename graph_adapter::adjacency_iterator adjacency_iterator;

  self_loop_col_iterator(const vertex_descriptor& v,
                         const graph_adapter& g,
                         adjacency_iterator& ait, int pos) :
    src(v), citer(ait), position(pos), vipm(get(_vertex_id_map, g))
  {
    last_pos = out_degree(v, g);  // 0..<deg>-1 are real adj.; <deg> is
                                  // a self-loop for ufl.
  }

  bool operator<(const self_loop_col_iterator& cit)
  {
    return position < cit.position;
  }

  bool operator!=(const self_loop_col_iterator& cit)
  {
    return position != cit.position;
  }

  void operator++(int) { position++; }
  int get_position() const { return position; }
  void set_position(int p) { position = p; }

  int operator*()
  {
    int sid = get(vipm, src);

    if (position == last_pos) return get(vipm, src);

    citer.set_position(position);
    int tid = get(vipm, *citer);

    return get(vipm, *citer);
  }

private:
  adjacency_iterator citer;
  vertex_descriptor src;
  vertex_id_map<graph_adapter> vipm;
  int position;
  int last_pos;
};

template <class graph_adapter>
class self_loop_val_iterator {
public:
  typedef typename graph_adapter::vertex_descriptor vertex_descriptor;
  typedef typename graph_adapter::adjacency_iterator adjacency_iterator;

  self_loop_val_iterator(const vertex_descriptor& v,
                         const graph_adapter& g, adjacency_iterator& ait,
                         int pos, int index_i, random_value* rnd,
                         double* rati, double* q) :
    src(v), citer(ait), position(pos), vid_map(get(_vertex_id_map, g)),
    _index_i(index_i), rvals(rnd), ratio(rati), Q(q)
  {
    last_pos = out_degree(v, g);   // 0..<deg>-1 are real adj.; <deg> is
                                   // a self-loop for ufl.

    ave_deg = 2 * num_edges(g) / (double) num_vertices(g);
  }

  bool operator<(const self_loop_val_iterator& cit)
  {
    return position < cit.position;
  }

  bool operator!=(const self_loop_val_iterator& cit)
  {
    return position != cit.position;
  }

  void operator++(int) { position++; }
  int get_position() const { return position; }
  void set_position(int p) { position = p; }
  double operator*()
  {
    return 0;

//TODO: Make sure that this should just return!  Commented out rest to get
//      rid of warning.

//              double service_cost, discount;
//              bool good=false;// Local modularity of server is at least
//                              // as good as local modularity of served
//              int sid = get(vid_map, src);
//              int tid =-1;

//              citer.set_position(position);
//              if (position == last_pos) {
//                      discount = 1.0;
//                      good = true;
//              } else {
// //            tid = get(vid_map, *citer);
// //            double rsrc = ratio[sid];
// //            double rdest = ratio[tid];
// //            good = (Q[sid] >= Q[tid]);
// //            discount = rdest/rsrc;
//              }

// //           if (position == last_pos) {
// //                   printf("service cost[%d]: %lf\n", position, 1.0);
// //                   service_cost = 0.0;
// //                              // serving yourself costs more (to
// //                              // discourage opening facilities)
// //                   return service_cost;
// //           }

//              double denom;
// #ifdef WIN32
//              denom = 32768;
// #else
//              denom = (double)INT_MAX;
// #endif
//              double perturb = 0.1 * rvals[_index_i+position]/denom;
//              //service_cost = perturb + (discount * ave_deg);
//              if (good)
//                      service_cost = 0;
//              else
//                      service_cost = 1000000;

//              return service_cost;
  }

private:
  adjacency_iterator citer;
  vertex_descriptor src;
  int position;
  int last_pos;
  vertex_id_map<graph_adapter> vid_map;
  random_value* rvals;
  int _index_i;
  double ave_deg;
  double* ratio;        //   m_1[i]/m_e[i]
  double* Q;
};

template <class graph_adapter>
class csr_self_loop_adapter : public graph_adapter {
public:
  typedef typename graph_adapter::vertex_descriptor vertex_descriptor;
  typedef typename graph_adapter::edge_descriptor edge_descriptor;
  typedef typename graph_adapter::vertex_iterator vertex_iterator;
  typedef typename graph_adapter::edge_iterator edge_iterator;
  typedef typename graph_adapter::out_edge_iterator out_edge_iterator;
  typedef typename graph_adapter::adjacency_iterator adjacency_iterator;
  typedef self_loop_col_iterator<graph_adapter> column_iterator;
  typedef self_loop_val_iterator<graph_adapter> value_iterator;
  typedef void graph_type;       // none!

  #pragma mta debug level 0

  csr_self_loop_adapter(graph_adapter& ga) : graph_adapter(ga)
  {
    unsigned int ord = num_vertices(ga);
    vertex_iterator verts = vertices(ga);
    int ind = 0;

    int* a = new int[ord + 1];
    a[0] = 0;
    int upper = ord + 1;

    for (int i = 1; i < upper; i++)
    {
      vertex_descriptor v = verts[i - 1];
      int incr = out_degree(v, ga) + 1;
      //int incr = 0;
      a[i] = a[i - 1] + incr;
    }

    _index = a;

    int nnz = num_nonzero();
    rvals = (random_value*) malloc(nnz * sizeof(random_value));
    rand_fill::generate(nnz, rvals);

    compute_vertex_load_balance();
  }

  #pragma mta debug level default

  unsigned int MatrixRows() const { return num_vertices(*this); }
  int num_nonzero() const
  {
    unsigned int ord = num_vertices(*this);
    return _index[ord];
  }

  int col_index(int i) const { return _index[i]; }
  int* get_index() const { return _index; }

  int get_degree(vertex_descriptor v) const
  {
    return out_degree(v, *this);  // Won't ack the self loop.
  }

  column_iterator col_indices_begin(int i) const
  {
    vertex_descriptor w = get_vertex(i, *this);
    adjacency_iterator ait = adjacent_vertices(*this, w);
    return column_iterator(w, *this, ait, 0);
  }

  column_iterator col_indices_end(int i)
  {
    vertex_descriptor w = get_vertex(i, *this);
    adjacency_iterator ait = adjacent_vertices(*this, w);
    int end_pos = out_degree(w, *this) + 1;
    return column_iterator(w, *this, ait, end_pos);
  }

  value_iterator col_values_begin(int i) const
  {
    vertex_descriptor w = get_vertex(i, *this);
    adjacency_iterator ait = adjacent_vertices(*this, w);
    return value_iterator(w, *this, ait, 0, _index[i], rvals, ratio, Q);
  }

  value_iterator col_values_end(int i)
  {
    vertex_descriptor w = get_vertex(i, *this);
    adjacency_iterator ait = adjacent_vertices(*this, w);
    int end_pos = out_degree(w, *this) + 1;
    return value_iterator(w, *this, ait, end_pos, _index[i], rvals, ratio, Q);
  }

  ~csr_self_loop_adapter()
  {
    delete [] _index;
    free(rvals);
  }

  void compute_vertex_load_balance()
  {
    unsigned int ord = num_vertices(*this);
    int sumd = 0;
    int* s = new int[ord];
    int* d = new int[ord];

    vertex_iterator verts = vertices(*this);

    #pragma mta assert parallel
    for (int i = 0; i < ord; i++)
    {
      d[i] = out_degree(verts[i], *this);
      sumd += d[i];
    }

    int work_per_iter = (sumd > 100000) ? sumd / 1000 : sumd;
    s[0] = d[0];

    for (int i = 1; i < ord; i++)
    {
      int incr = (((s[i - 1] != 0) *
                   ((-s[i - 1] *((s[i - 1] + d[i]) >  work_per_iter)) +
                    (d[i]    *((s[i - 1] + d[i]) <= work_per_iter)))) +
                  ((s[i - 1] == 0) *
                   ((d[i - 1] + d[i]) * ((d[i - 1] + d[i]) <= work_per_iter))));

      s[i] = s[i - 1] + incr;
    }

    d[0] = 0;

    for (int i = 1; i < ord; i++) d[i] = d[i - 1] + (s[i] == 0);

    num_blocks = d[ord - 1] + 1;
    blocks = new int[num_blocks];
    blocks[0] = 0;

    for (int i = 1; i < ord; i++)
    {
      if (d[i] > d[i - 1]) blocks[d[i]] = i;
    }

    delete [] d;
    delete [] s;
  }

  void set_ratio(double* r) { ratio = r; }
  void set_Q(double* q) { Q = q; }
  int* get_blocks() const { return blocks; }
  int  get_num_blocks() const { return num_blocks; }

private:
  int* _index;              // 0..n
  random_value* rvals;
  double* ratio;            // m_1[i]/m_e[i]
  double* Q;                // local modularity
  int* blocks;              // vertex index bounds for balanced adjlist trav
  int num_blocks;
};

template <class graph_adapter>
typename graph_traits<csr_self_loop_adapter<graph_adapter> >::vertex_iterator
vertices(const csr_self_loop_adapter<graph_adapter>& g)
{
  return vertices((graph_adapter) g);
}

template <class graph_adapter>
typename graph_traits<csr_self_loop_adapter<graph_adapter> >::adjacency_iterator
adjacent_vertices(
    const typename csr_self_loop_adapter<graph_adapter>::vertex_descriptor& v,
    const csr_self_loop_adapter<graph_adapter>& g)
{
  return adjacent_vertices((typename graph_adapter::vertex_descriptor) v,
                           (graph_adapter) g);
}

template <class graph_adapter>
typename graph_traits<csr_self_loop_adapter<graph_adapter> >::edge_iterator
edges(const csr_self_loop_adapter<graph_adapter>& g)
{
  return edges((graph_adapter) g);
}

template <class graph_adapter>
typename graph_traits<csr_self_loop_adapter<graph_adapter> >::out_edge_iterator
out_edges(const typename csr_self_loop_adapter<graph_adapter>
          ::vertex_descriptor& v,
          const csr_self_loop_adapter<graph_adapter>& g)
{
  return out_edges((typename graph_adapter::vertex_descriptor) v,
                   (graph_adapter) g);
}

template <class graph_adapter>
class vertex_id_map<csr_self_loop_adapter<graph_adapter> > :
  public vertex_id_map<graph_adapter> {
public:
  vertex_id_map(csr_self_loop_adapter<graph_adapter>& ga) :
    vertex_id_map<graph_adapter>(ga) {}

  vertex_id_map() : vertex_id_map<graph_adapter>() {}
};

template <class graph_adapter>
class edge_id_map<csr_self_loop_adapter<graph_adapter> > :
  public edge_id_map<graph_adapter> {
public:
  edge_id_map(csr_self_loop_adapter<graph_adapter>& ga) :
    edge_id_map<graph_adapter>(ga) {}

  edge_id_map() : edge_id_map<graph_adapter>() {}
};

template <class graph>
class s_tilde_vis {
//  For each vertex, suppose it is a community leader, and assume
//  that it serves all of its neighbors.  Given that the support has
//  already been computed, compute s_tilde, the average support of its
//  incident edges.

public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;

  s_tilde_vis(graph& gg, vertex_id_map<graph>& vm,
              edge_id_map<graph>& em, double* supp, double* _s_tilde) :
    g(gg), vid_map(vm), eid_map(em), support(supp), s_tilde(_s_tilde) {}

  bool visit_test(vertex_t v) { return true; }

  void pre_visit(vertex_t v)
  {
    int vid = get(vid_map, v);
    s_tilde[vid] = 0;
  }

  void operator()(edge_t e, vertex_t src, vertex_t dest)
  {
    int sid = get(vid_map, src);
    int tid = get(vid_map, dest);
    int eid = get_edge_id(g, src, dest, vid_map, eid_map);
    s_tilde[sid] += support[eid];
  }

  void post_visit(vertex_t v)
  {
    int vid = get(vid_map, v);
    s_tilde[vid] /= out_degree(v, g);
  }

private:
  graph& g;
  vertex_id_map<graph>& vid_map;
  edge_id_map<graph>& eid_map;
  double* s_tilde;
  double* support;
};

template <class graph>
class sum_s_tilde_vis {
//  For each vertex, suppose it is a community leader, and assume
//  that it serves all of its neighbors.  Given that the support has
//  already been computed, compute s_tilde, the average support of its
//  incident edges.

public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;

  sum_s_tilde_vis(graph& gg, vertex_id_map<graph>& vm,
                  edge_id_map<graph>& em, double* _s_tilde, double* priml,
                  double* sum_s_tild, double* av_s_tilde, double* sum_priml) :
    g(gg), vid_map(vm), eid_map(em), primal(priml), s_tilde(_s_tilde),
    sum_s_tilde(sum_s_tild), ave_s_tilde(av_s_tilde), sum_primal(sum_priml) {}

  bool visit_test(vertex_t v) { return true; }

  void pre_visit(vertex_t v)
  {
    int vid = get(vid_map, v);
    sum_s_tilde[vid] = s_tilde[vid];
    sum_primal[vid] = primal[vid];
  }

  void operator()(edge_t e, vertex_t src, vertex_t dest)
  {
    int sid = get(vid_map, src);
    int tid = get(vid_map, dest);
    sum_s_tilde[sid] += s_tilde[tid];
    sum_primal[sid] += primal[tid];
  }

  void post_visit(vertex_t v)
  {
    int sid = get(vid_map, v);
    ave_s_tilde[sid] = sum_s_tilde[sid] / (out_degree(v, g) + 1);
  }

private:
  graph& g;
  vertex_id_map<graph>& vid_map;
  edge_id_map<graph>& eid_map;
  double* s_tilde;
  double* primal;
  double* sum_s_tilde;
  double* ave_s_tilde;
  double* sum_primal;
};

/*! ********************************************************************
    \class snl_community
    \brief
 *  ********************************************************************
*/
template <typename graph>
class snl_community {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;
  typedef typename graph_traits<graph>::vertex_iterator vertex_iterator;
  typedef typename csr_self_loop_adapter<graph>::column_iterator
          column_iterator;

  snl_community(graph& gg, int* res, double* psol = 0, double* supp = 0) :
    g(gg), server(res), primal(psol), support(supp) {}

  int run()
  {
    mt_timer mttimer;
    mttimer.start();
    csr_self_loop_adapter<graph> sga(g);

    vertex_id_map<csr_self_loop_adapter<graph> > vid_map =
      get(_vertex_id_map, sga);
    edge_id_map<csr_self_loop_adapter<graph> > eid_map =
      get(_edge_id_map, sga);

    unsigned int ord = num_vertices(g);
    int size = num_edges(g);

    for (int i = 0; i < ord; i++)
    {
      int ind = sga.col_index(i);
      column_iterator cbegin = sga.col_indices_begin(i);
      column_iterator cend   = sga.col_indices_end(i);
      column_iterator cit = cbegin;

      for ( ; cit != cend; cit++) int id = *cit;
    }

    int nnz = sga.num_nonzero();
    int* servcosts = (int*) malloc(sizeof(int) * nnz);

    printf("nnz: %d\n", nnz);

    #pragma mta assert nodep
    for (int i = 0; i < (nnz - ord); i++) servcosts[i] = 1;

    for (int i = nnz - ord; i < nnz; i++) servcosts[i] = 1;

    int num_leaders = 0;         // Will do ufl soon.
    bool* leaders = (bool*) malloc(ord * sizeof(bool));
    double* opening_costs = (double*) malloc(ord * sizeof(double));
    int* one_neigh_size = (int*) malloc(ord * sizeof(int));
    int* m_e_num = (int*) malloc(ord * sizeof(int));

    vertex_iterator verts = vertices(g);

    #pragma mta assert parallel
    for (int i = 0; i < ord; i++)
    {
      one_neigh_size[i] = out_degree(verts[i], g);
    }

    one_neighborhood_edge_count<csr_self_loop_adapter<graph> >
    ctv(sga, one_neigh_size, vid_map);

#ifndef __MTA__
    find_triangles_phases<csr_self_loop_adapter<graph>,
                          one_neighborhood_edge_count<
                            csr_self_loop_adapter<graph> > > ft(sga, ctv);
#else
    find_triangles<csr_self_loop_adapter<graph>,
                   one_neighborhood_edge_count<
                     csr_self_loop_adapter<graph> > > ft(sga, ctv);
#endif

    printf("snl_community: finding triangles\n");
    fflush(stdout);
    ft.run();
    printf("snl_community: done finding triangles\n");
    fflush(stdout);

/*
    m_e_vis<csr_self_loop_adapter<graph> > mevis(sga, vid_map, m_e_num);
    visit_adj<csr_self_loop_adapter<graph>,
              m_e_vis<csr_self_loop_adapter<graph> > >(sga, mevis);

    double max_ratio = (m_e_num[0] > 0) ?
                       2 * size * one_neigh_size[0]/(double)m_e_num[0] : 0;
    double *ratio = new double[ord];

    for (int i=0; i<ord; i++)
    {
      ratio[i] = (m_e_num[i] > 0) ?
                 2* size * one_neigh_size[i]/(double)m_e_num[i] : 0;

      if (ratio[i] > max_ratio) max_ratio = ratio[i];
    }

    sga.set_ratio(ratio);
*/

    // ***************************************************************
    // ** local modularity
    // ***************************************************************
    int* e_rs_num = new int[ord];
    double* Q = new double[ord];

    for (int i = 0; i < ord; i++) e_rs_num[i] = 0;

    e_rs_vis<csr_self_loop_adapter<graph> >
    ersvis(sga, vid_map, one_neigh_size, e_rs_num);
    visit_adj<csr_self_loop_adapter<graph>,
              e_rs_vis<csr_self_loop_adapter<graph> > >(sga, ersvis);

    double min_q = (double) INT_MAX, max_q = 0;
    double tot_deg = 2 * size;
    double ave_deg = tot_deg / (double) ord;
    double p = ave_deg / (double) (ord - 1);

    #pragma mta assert parallel
    for (int i = 0; i < ord; i++)
    {
      int dist_two_node_bound = e_rs_num[i] - (2 * one_neigh_size[i]);
      double two_neigh_node_bound =
        1                         // node i
        + out_degree(verts[i], g)     // 1-neighbors
        + dist_two_node_bound;

      double two_neigh_adj_bound = two_neigh_node_bound *
                                   (two_neigh_node_bound - 1);

      //double two_neigh_est = e_rs_num[i] +
      //                       (2*max_neigh_edge_est*p);

      double denom = (two_neigh_adj_bound > 0)
                     ? two_neigh_adj_bound : 1;

      double a_s = e_rs_num[i] / denom;

      // Local modularity 0
      Q[i] = 2 * one_neigh_size[i] / (double) denom - a_s * a_s;

      //printf("Q[%d]: %d/%lf - (%d*%d)/%lf/%lf = %lf\n",
      //   i, 2*one_neigh_size[i], denom,
      //   e_rs_num[i], e_rs_num[i],
      //   denom, denom, Q[i]);
      // Newman modularity 0
      //Q[i] = one_neigh_size[i]/(double)size -
      //      a_s*a_s;    // where a_s has denom 'size'
      //printf("Q[%d]: %d/%d - (%d*%d)/%d/%d = %lf\n",
      //     i, one_neigh_size[i], size,
      //     e_rs_num[i], e_rs_num[i],
      //     size, size, Q[i]);
      // Newman modularity 1
      //Q[i] = one_neigh_size[i]/(double)size -
      //      a_s * a_s;
      //printf("Q[%d]: %d/%d - (%d*%d)/%lf/%lf = %lf\n", i,
      //  one_neigh_size[i], size, e_rs_num[i], e_rs_num[i],
      //  tot_deg, tot_deg, Q[i]);

      if (Q[i] < min_q) min_q = Q[i];
      if (Q[i] > max_q) max_q = Q[i];
    }

    sga.set_Q(Q);

    // ***************************************************************

    #pragma mta assert parallel
    for (int i = 0; i < ord; i++)
    {
      vertex_t v = verts[i];
      leaders[i] = false;
      int deg = out_degree(v, g);

      //if (ratio[i] == 0) ratio[i] = max_ratio;

      opening_costs[i] = 1 - Q[i];
      //opening_costs[i] = 1000*(max_q - Q[i])+1;
      //opening_costs[i] = 5000*(max_q - Q[i]) + 1;
      //opening_costs[i] = 100*(max_ratio - ratio[i]) + ave_deg*deg;
      //opening_costs[i] = (max_ratio - ratio[i]) + 1;
    }

    // NOTE: the version that tried load balance masks is in svn 1330
    ufl_module<double, csr_self_loop_adapter<graph> >(ord, ord, nnz,
                                                      num_leaders,
                                                      opening_costs, sga, 0.001,
                                                      leaders, server, primal);

    mttimer.stop();

    double sec = mttimer.getElapsedSeconds();

    printf("snl_community time: %lf\n", sec);

    free(servcosts);

/*
             num_leaders = 0;
             for (int i=0; i<ord; i++) {
                if (leaders[i]) {
                        num_leaders++;
                        printf("leader[%d] = 1\n", i);
                }
             }
             //delete [] ratio;
             delete [] Q;


             // ********************************************************
             // ** now recurse to finish dendogram *********************
             // ********************************************************
             int *idmap = new int[num_leaders];
             int *id_inversemap = new int[ord];
             int token = 0;
             for (int i=0; i<ord; i++) {
                if (leaders[i]) {
                        int my_token = mt_incr(token, 1);
                        idmap[my_token] = i;
                        id_inversemap[i] = my_token;
                }
             }
//       for (int i=0; i<ord; i++) {
//    id_inversemap[i] = id_inversemap[server[i]];
//       }
//       int count=0;
//           cross_community_edge_counter<graph>ccec(
//          g, vid_map, id_inversemap, count);
//       visit_edges(g, ccec);
//       int *srcs  = new int[count];
//       int *dests = new int[count];


        vertex_id_map<graph> vid_map2 = get(_vertex_id_map, g);
        edge_id_map<graph> eid_map2 = get(_edge_id_map, g);

        support_init<graph> sup_in(g, vid_map2, eid_map2, primal, support);
        visit_edges(g, sup_in);
        edge_support_count<graph> esc(g, primal, support, vid_map2, eid_map2);
        find_triangles_phases<graph, edge_support_count<graph> > ft2(g, esc);
        ft2.run();
        for (int i=0; i<size; i++) {
                support[i] = 1 - support[i];
        }
        double *s_tilde = new double[ord];
        double *sum_s_tilde = new double[ord];
        double *ave_s_tilde = new double[ord];
        double *sum_primal = new double[ord];
        s_tilde_vis<graph> stv(sga, vid_map2, eid_map2,
                                        support, s_tilde);
        visit_adj<graph, s_tilde_vis<graph> >(g, stv);

        sum_s_tilde_vis<graph> sstv(sga, vid_map2,
                                        eid_map2, s_tilde, primal,
                                        sum_s_tilde, ave_s_tilde, sum_primal);

        visit_adj<graph,
                  sum_s_tilde_vis<graph> >(g, sstv);

        for (int i=0; i<ord; i++) {
                printf(" %d: pr:%4.2lf  s:\t%4.2lf\ts_tilde:%4.2lf\t"
                        "sst:%4.2lf\tast:%lf\n",
                        i, primal[i], sum_primal[i], s_tilde[i], sum_s_tilde[i],
                        ave_s_tilde[i]);
        }

        int i, **type_mix_mat = (int **)malloc(num_leaders * sizeof(int *));

        for(i = 0; i < num_leaders; ++i)
          type_mix_mat[i] = (int *)calloc(num_leaders, sizeof(int));

        find_type_mixing_matrix<graph> fa(g, type_mix_mat, num_leaders,
                                          server, id_inversemap);
        fa.run();
        {
        double all = 2 * num_edges(g);
        double ps, Q = 0, sq;

        for (i = 0; i < num_leaders; ++i) {
          ps = 0;

          for (int j = 0; j < num_leaders; ++j) {
              ps += type_mix_mat[i][j];
          }

          ps /= all;
          sq = ps * ps;
          Q += type_mix_mat[i][i] / all - sq;
          //printf("comm %d: %d / %lf - (%lf *%lf) = %lf\n",
        //  i, type_mix_mat[i][i], all, ps, ps,
        //  type_mix_mat[i][i]/all - sq);
        }

        fprintf(stdout, "modularity = %lf\n", Q);
        }
        for (int i=0; i<ord; i++) {
                printf("psol[%d]: %lf\n", i, primal[i]);
        }
*/

    // ********************************************************

    return 0;  // Maybe return a meaningful value eventually, or else
               // make this a void method.

  }  // run()

  //DEPRECATED
// int run_with_masks()
//    {
//      mt_timer mttimer;
//      mttimer.start();
//      csr_self_loop_adapter<graph> sga(g);

//      vertex_id_map<csr_self_loop_adapter<graph> > vid_map =
//                         get(_vertex_id_map, sga);
//      edge_id_map<csr_self_loop_adapter<graph> > eid_map =
//                         get(_edge_id_map, sga);

//      vertex_iterator verts = vertices(g);

//      unsigned int ord = num_vertices(g);
//      int size= num_edges(g);
//      for (int i=0; i<ord; i++) {
//         int ind = sga.col_index(i);
//         column_iterator cbegin = sga.col_indices_begin(i);
//         column_iterator cend   = sga.col_indices_end(i);
//         column_iterator cit = cbegin;
//         for (; cit != cend; cit++) {
//                 int id = *cit;
//         }
//      }
//      int nnz = sga.num_nonzero();
//      int *servcosts = (int*) malloc(sizeof(int)*nnz);
//      printf("nnz: %d\n", nnz);
//      #pragma mta assert nodep
//      for (int i=0; i<(nnz - ord); i++) {
//         servcosts[i] = 1;
//      }
//      for (int i=nnz-ord; i<nnz; i++) {
//         servcosts[i] = 1;
//      }
//      int num_leaders=0;  // will do ufl soon
//      bool *leaders = (bool*) malloc(ord*sizeof(bool));
//      double *opening_costs=(double*)malloc(ord*sizeof(double));
//      int   *one_neigh_size=(int*)malloc(ord*sizeof(int));
//      int   *m_e_num       =(int*)malloc(ord*sizeof(int));
//      for (int i=0; i<ord; i++) {
//                 one_neigh_size[i] = out_degree(verts[i], g);
//      }

//      one_neighborhood_edge_count<csr_self_loop_adapter<graph> >
//              ctv(sga, one_neigh_size, vid_map);
//      find_triangles<csr_self_loop_adapter<graph>,
//                     one_neighborhood_edge_count<
//                              csr_self_loop_adapter<graph> > > ft(sga, ctv);
//      printf("snl_community: finding triangles\n");
//      fflush(stdout);
//      ft.run();
//      printf("snl_community: done finding triangles\n");
//      fflush(stdout);

//      /*
//      m_e_vis<csr_self_loop_adapter<graph> > mevis(sga, vid_map, m_e_num);
//      visit_adj<csr_self_loop_adapter<graph>,
//                m_e_vis<csr_self_loop_adapter<graph> > >(sga, mevis);
//      double max_ratio = (m_e_num[0] > 0) ?
//                      2 * size * one_neigh_size[0]/(double)m_e_num[0] : 0;
//      double *ratio = new double[ord];
//      for (int i=0; i<ord; i++) {
//              ratio[i] = (m_e_num[i] > 0) ?
//                      2* size * one_neigh_size[i]/(double)m_e_num[i] : 0;
//              if (ratio[i] > max_ratio)
//                      max_ratio = ratio[i];
//      }
//      sga.set_ratio(ratio);
//      */
//      // ***************************************************************
//      // ** local modularity
//      // ***************************************************************
//      int *e_rs_num = new int[ord];
//      double *Q = new double[ord];
//      for (int i=0; i<ord; i++) {
//              e_rs_num[i] = 0;
//      }
//      e_rs_vis<csr_self_loop_adapter<graph> >
//                      ersvis(sga, vid_map, one_neigh_size, e_rs_num);
//      visit_adj<csr_self_loop_adapter<graph>,
//                e_rs_vis<csr_self_loop_adapter<graph> > >(sga, ersvis);
//      double min_q= (double) INT_MAX, max_q = 0;
//      double tot_deg = 2*size;
//      double ave_deg = tot_deg/(double)ord;
//      double p = ave_deg / (double) (ord-1);

//      for (int i=0; i<ord; i++) {
//              int dist_two_node_bound = e_rs_num[i] - (2*one_neigh_size[i]);
//              double two_neigh_node_bound =
//                      1 // node i
//                      + out_degree(verts[i], g) // 1-neighbors
//                      + dist_two_node_bound;
//              double two_neigh_adj_bound = two_neigh_node_bound *
//                                           (two_neigh_node_bound-1);
//              //double two_neigh_est = e_rs_num[i] +
//              //               (2*max_neigh_edge_est*p);

//              double denom = (two_neigh_adj_bound > 0)
//                              ? two_neigh_adj_bound : 1;

//              double a_s = e_rs_num[i]/denom;
//              // Local modularity 0
//              Q[i] = 2*one_neigh_size[i]/(double)denom -
//                     a_s * a_s;
//              //printf("Q[%d]: %d/%lf - (%d*%d)/%lf/%lf = %lf\n",
//              //     i, 2*one_neigh_size[i], denom,
//              //     e_rs_num[i], e_rs_num[i],
//              //     denom, denom, Q[i]);
//              // Newman modularity 0
//              //Q[i] = one_neigh_size[i]/(double)size -
//              //      a_s*a_s;        // where a_s has denom 'size'
//              //printf("Q[%d]: %d/%d - (%d*%d)/%d/%d = %lf\n",
//      //         i, one_neigh_size[i], size,
//      //         e_rs_num[i], e_rs_num[i],
//      //         size, size, Q[i]);
//              // Newman modularity 1
//              //Q[i] = one_neigh_size[i]/(double)size -
//               //      a_s * a_s;
//              //printf("Q[%d]: %d/%d - (%d*%d)/%lf/%lf = %lf\n", i,
//              //    one_neigh_size[i], size, e_rs_num[i], e_rs_num[i],
//              //    tot_deg, tot_deg, Q[i]);
//              if (Q[i] < min_q) {
//                      min_q = Q[i];
//              }
//              if (Q[i] > max_q) {
//                      max_q = Q[i];
//              }
//      }
//      sga.set_Q(Q);
//      // ***************************************************************
//      for (int i=0; i<ord; i++) {
//              vertex_t v = verts[i];
//              leaders[i] = false;
//              int deg = out_degree(v, g);
//              //if (ratio[i] == 0) ratio[i] = max_ratio;
//              opening_costs[i] = 1-Q[i];
//              //opening_costs[i] = 1000*(max_q - Q[i])+1;
//              //opening_costs[i] = 5000*(max_q - Q[i]) + 1;
//              //opening_costs[i] = 100*(max_ratio - ratio[i]) + ave_deg*deg;
//              //opening_costs[i] = (max_ratio - ratio[i]) + 1;
//      }

//              // NOTE: the version that tried load balance masks is in svn 1330
//           ufl_module<double, csr_self_loop_adapter<graph> >(ord,
//                              ord, nnz, num_leaders, opening_costs,
//                                      sga, 0.001, leaders, server, primal,
//                              0, g.get_load_balance_masks(),
//                              g.get_load_balance_totals(),
//                                 g.get_load_balance_parallelize_flags(),
//                                 g.get_num_load_balance_masks());
//           mttimer.stop();
//           double sec = mttimer.getElapsedSeconds();
//           printf("snl_community time: %lf\n", sec);
//           free(servcosts);
// /*
//           num_leaders = 0;
//           for (int i=0; i<ord; i++) {
//              if (leaders[i]) {
//                      num_leaders++;
//                      printf("leader[%d] = 1\n", i);
//              }
//           }
//           //delete [] ratio;
//           delete [] Q;

//           // ********************************************************
//           // ** now recurse to finish dendogram *********************
//           // ********************************************************
//           int *idmap = new int[num_leaders];
//           int *id_inversemap = new int[ord];
//           int token = 0;
//           for (int i=0; i<ord; i++) {
//              if (leaders[i]) {
//                      int my_token = mt_incr(token, 1);
//                      idmap[my_token] = i;
//                      id_inversemap[i] = my_token;
//              }
//           }
// //      for (int i=0; i<ord; i++) {
// //        id_inversemap[i] = id_inversemap[server[i]];
// //      }
// //      int count=0;
// //      cross_community_edge_counter<graph>ccec(
// //           g, vid_map, id_inversemap, count);
// //      visit_edges(g, ccec);
// //      int *srcs  = new int[count];
// //      int *dests = new int[count];

//      vertex_id_map<graph> vid_map2 = get(_vertex_id_map, g);
//      edge_id_map<graph> eid_map2 = get(_edge_id_map, g);

//      support_init<graph> sup_in(g, vid_map2, eid_map2, primal, support);
//      visit_edges(g, sup_in);
//      edge_support_count<graph> esc(g, primal, support, vid_map2, eid_map2);
//      find_triangles<graph, edge_support_count<graph> > ft2(g, esc);
//      ft2.run();
//      for (int i=0; i<size; i++) {
//              support[i] = 1 - support[i];
//      }
//      double *s_tilde = new double[ord];
//      double *sum_s_tilde = new double[ord];
//      double *ave_s_tilde = new double[ord];
//      double *sum_primal = new double[ord];
//      s_tilde_vis<graph> stv(sga, vid_map2, eid_map2,
//                                      support, s_tilde);
//      visit_adj<graph, s_tilde_vis<graph> >(g, stv);

//      sum_s_tilde_vis<graph> sstv(sga, vid_map2,
//                                      eid_map2, s_tilde, primal,
//                                      sum_s_tilde, ave_s_tilde, sum_primal);

//      visit_adj<graph,
//                sum_s_tilde_vis<graph> >(g, sstv);

//      for (int i=0; i<ord; i++) {
//              printf(" %d: pr:%4.2lf  s:\t%4.2lf\ts_tilde:%4.2lf\t"
//                      "sst:%4.2lf\tast:%lf\n",
//                      i, primal[i], sum_primal[i], s_tilde[i], sum_s_tilde[i],
//                      ave_s_tilde[i]);
//      }

//         int i, **type_mix_mat = (int **)malloc(num_leaders * sizeof(int *));

//      for(i = 0; i < num_leaders; ++i)
//        type_mix_mat[i] = (int *)calloc(num_leaders, sizeof(int));

//      find_type_mixing_matrix<graph> fa(g, type_mix_mat, num_leaders,
//                                        server, id_inversemap);
//      fa.run();
//      {
//      double all = 2 * num_edges(g);
//      double ps, Q = 0, sq;

//      for (i = 0; i < num_leaders; ++i) {
//        ps = 0;

//        for (int j = 0; j < num_leaders; ++j) {
//            ps += type_mix_mat[i][j];
//        }

//        ps /= all;
//        sq = ps * ps;
//        Q += type_mix_mat[i][i] / all - sq;
//        //printf("comm %d: %d / %lf - (%lf *%lf) = %lf\n",
//      //         i, type_mix_mat[i][i], all, ps, ps,
//      //         type_mix_mat[i][i]/all - sq);
//      }

//      fprintf(stdout, "modularity = %lf\n", Q);
//      }
//      for (int i=0; i<ord; i++) {
//              printf("psol[%d]: %lf\n", i, primal[i]);
//      }
// */
//           // ********************************************************
//           return 0;  // maybe return a meaningful value eventually, or else
//                      // make this a void method.

//      } // run()

private:
  graph& g;
  int* server;
  double* primal;
  double* support;
  random_value* rvals;
};

}

#endif

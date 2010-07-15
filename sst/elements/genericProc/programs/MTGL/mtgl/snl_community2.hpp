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
/*! \file snl_community2.hpp

    \author Jon Berry (jberry@sandia.gov)

    \date 6/25/2008
*/
/****************************************************************************/

#ifndef MTGL_SNL_COMMUNITY2_HPP
#define MTGL_SNL_COMMUNITY2_HPP

#include <cstdio>
#include <climits>
#include <cstdlib>
#include <list>

#include <mtgl/mtgl_adapter.hpp>
#include <mtgl/util.hpp>
#include <mtgl/triangles.hpp>
#include <mtgl/ufl.h>
#include <mtgl/metrics.hpp>
#include <mtgl/rand_fill.hpp>
#include <mtgl/xmt_hash_table.hpp>
#include <mtgl/topsort.hpp>
#include <mtgl/visit_adj.hpp>

#ifdef __MTA__
#include <sys/mta_task.h>
#include <machine/runtime.h>
#endif

namespace mtgl {

template <class graph_adapter>
class edge_server_col_iterator {
public:
  typedef typename graph_adapter::edge_descriptor edge_t;
  typedef typename graph_adapter::vertex_descriptor vertex_t;

  edge_server_col_iterator(const graph_adapter& g, int id, int pos = 0) :
    position(pos), g_size(num_edge(g)), eid(id), ga(g),
    vipm(get(_vertex_id_map, g)), order(num_vertices(g))
  {
    if (eid < g_size)
    {
      last_pos = 1;
    }
    else
    {
      last_pos = 0;            // We'll never iterate, so a sensible
    }                          // program won't invoke the op*.
  }

  bool operator<(const edge_server_col_iterator& cit)
  {
    return position < cit.position;
  }

  bool operator!=(const edge_server_col_iterator& cit)
  {
    return position != cit.position;
  }

  void operator++(int) { position++; }
  int get_position() const { return position; }
  void set_position(int p) { position = p; }

  int operator*()
  {
    edge_t my_edge = get_edge(eid, ga);
    vertex_t g_src = source(my_edge, ga);
    vertex_t g_trg = target(my_edge, ga);

    int g_sid = get(vipm, g_src);
    int g_tid = get(vipm, g_trg);

    if (position == 0)
    {
      return g_sid;
    }
    else if (position == 1)
    {
      return g_tid;
    }
    else
    {
      return -1;
    }
  }

private:
  const graph_adapter& ga;
  vertex_id_map<graph_adapter> vipm;
  int position, eid;
  int g_size;
  unsigned int order;
  int last_pos;
};

template <class graph_adapter>
class edge_server_val_iterator {
public:
  typedef typename graph_adapter::edge_descriptor edge_t;
  typedef typename graph_adapter::vertex_descriptor vertex_t;

  edge_server_val_iterator(const graph_adapter& g, int id, int pos = 0) :
    position(pos), g_size(num_edge(g)), eid(id), ga(g)
  {
    if (eid < g_size)
    {
      last_pos = 1;
    }
    else
    {
      last_pos = 0;            // We'll never iterate, so a sensible
    }                          // program won't invoke the op*.
  }

  bool operator<(const edge_server_val_iterator& cit)
  {
    return position < cit.position;
  }

  bool operator!=(const edge_server_val_iterator& cit)
  {
    return position != cit.position;
  }

  void operator++(int) { position++; }
  int get_position() const { return position; }
  void set_position(int p) { position = p; }
  int operator*() { return 0; }

private:
  const graph_adapter& ga;
  int position, eid;
  int g_size;
  int last_pos;
};

template <class graph_adapter>
class edge_server_adapter : public graph_adapter {
public:
  typedef typename graph_adapter::vertex_descriptor vertex_descriptor;
  typedef typename graph_adapter::edge_descriptor edge_descriptor;
  typedef typename graph_adapter::vertex_iterator vertex_iterator;
  typedef typename graph_adapter::edge_iterator edge_iterator;
  typedef typename graph_adapter::out_edge_iterator out_edge_iterator;
  typedef typename graph_adapter::adjacency_iterator adjacency_iterator;
  typedef edge_server_col_iterator<edge_server_adapter<graph_adapter> >
          column_iterator;
  typedef edge_server_val_iterator<edge_server_adapter<graph_adapter> >
          value_iterator;

  #pragma mta debug level 0
  edge_server_adapter(graph_adapter& ga) : graph_adapter(ga)
  {
    g_order = num_vertices(ga);
    g_size  = num_edge(ga);
    order = g_size;               //  + g_order;
    size = 2 * g_size;
  }

  int* get_index() const { return 0; }
  unsigned int MatrixRows() const { return order; }
  int num_nonzero() const { return size; }

  int col_index(int i) const
  {
    if (i < g_size)
    {
      return 2 * i;
    }
    else
    {
      return 2 * g_size;
    }
  }

  column_iterator col_indices_begin(int i) const
  {
    return column_iterator(*this, i);
  }

  column_iterator col_indices_end(int i) const
  {
    return column_iterator(*this, i, 1);
  }

  value_iterator col_values_begin(int i) const
  {
    return value_iterator(*this, i);
  }

  value_iterator col_values_end(int i) const
  {
    return value_iterator(*this, i, 1);
  }

  unsigned int get_order() const { return order; }
  int get_size() const { return size; }

private:
  unsigned int order, g_order;
  int size, g_size;
};

template <class graph_adapter>
typename graph_traits<edge_server_adapter<graph_adapter> >::edge_iterator
edges(const edge_server_adapter<graph_adapter>& g)
{
  return edges((graph_adapter) g);
}

template <class graph_adapter>
typename graph_traits<edge_server_adapter<graph_adapter> >::out_edge_iterator
out_edges(const typename edge_server_adapter<graph_adapter>::vertex_descriptor& v,
          const edge_server_adapter<graph_adapter>& g)
{
  return out_edges((typename graph_adapter::vertex_descriptor) v, (graph_adapter) g);
}

template <class graph_adapter>
typename graph_traits<edge_server_adapter<graph_adapter> >::vertex_iterator
vertices(const edge_server_adapter<graph_adapter>& g)
{
  return vertices((graph_adapter) g);
}

template <class graph_adapter>
typename graph_traits<edge_server_adapter<graph_adapter> >::adjacency_iterator
adjacent_vertices(
    const typename edge_server_adapter<graph_adapter>::vertex_descriptor& v,
    const edge_server_adapter<graph_adapter>& g)
{
  return adjacent_vertices((typename graph_adapter::vertex_descriptor) v,
                           (graph_adapter) g);
}

template <class graph_adapter>
class vertex_id_map<edge_server_adapter<graph_adapter> > :
  public vertex_id_map<graph_adapter> {
public:
  vertex_id_map(edge_server_adapter<graph_adapter>& ga) :
    vertex_id_map<graph_adapter>(ga) {}

  vertex_id_map<edge_server_adapter<graph_adapter> >() :
    vertex_id_map<graph_adapter>() {}
};

template <class graph_adapter>
class edge_id_map<edge_server_adapter<graph_adapter> > :
  public edge_id_map<graph_adapter> {
public:
  edge_id_map(edge_server_adapter<graph_adapter>& ga) :
    edge_id_map<graph_adapter>(ga) {}

  edge_id_map() : edge_id_map<graph_adapter>() {}
};

typedef struct {
  int degree;

#ifdef USING_QTHREADS
  aligned_t acc;
#else
  double acc;
#endif

  double rank;
} rank_info;

template <class T>
static list<list<T> > choose(T* a, int sz, int k)
{
  list<list<T> > result;

  if (sz == k)
  {
    list<T> l;
    for (int i = 0; i < sz; i++) l.push_back(a[i]);
    result.push_back(l);
    return result;
  }
  else if (sz < k)
  {
    return result;
  }
  else if (k == 0)
  {
    list<T> l;
    result.push_back(l);
    return result;
  }

  for (int i = 0; i < sz; i++)
  {
    T elem = a[i];
    T* nexta = (T*) malloc(sizeof(T) * (sz - 1));
    int ind = 0;

    for (int j = 0; j < sz; j++)
    {
      if (j != i && a[j] > a[i]) nexta[ind++] = a[j];
    }

    list<list<T> > tmp_result = choose(nexta, ind, k - 1);

    typename list<list<T> >::iterator it = tmp_result.begin();
    for ( ; it != tmp_result.end(); it++)
    {
      list<T> next = *it;
      next.push_back(a[i]);
      result.push_back(next);
    }

    free(nexta);
  }

  return result;
}

template <typename graph>
class hash_real_edges {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;
  typedef xmt_hash_table<int64_t, int64_t> hash_table_t;

  hash_real_edges(graph& gg, vertex_id_map<graph>& vm, hash_table_t& tpc,
                  hash_table_t& eids, hash_table_t& td, hash_table_t& ons,
                  int* vons, hash_table_t& gons, rank_info* rin, int thresh) :
    g(gg), vid_map(vm), real_edge_tp_count(tpc), rinfo(rin),
    real_edge_ids(eids), threshold(thresh), order(num_vertices(gg)),
    tent_degree(td), en_count(ons), good_en_count(gons), vn_count(vons)
  {
    eid_map = get(_edge_id_map, g);
  }

  void operator()(edge_t e)
  {
    vertex_t v1 = source(e, g);
    vertex_t v2 = target(e, g);

    int v1id = get(vid_map, v1);
    int v2id = get(vid_map, v2);
    int eid = get(eid_map, e);

    if ((rinfo[v1id].degree <= threshold) && (rinfo[v2id].degree <= threshold))
    {
      order_pair(v1id, v2id);
      int htsz = real_edge_tp_count.size();
      int64_t key = v1id * order + v2id;

      real_edge_tp_count.insert(key, 0);
      real_edge_ids.insert(key, eid);

      // Edge one-neighborhood size.  Start with n(v1)+n(v2).
      // We'll apply corrections in subsequent passes.
      int ons = vn_count[v1id] + vn_count[v2id] - 1;

      en_count.insert(key, 4 * ons);
      good_en_count.insert(key, 4);

      if (key == 176) printf("gec(5,6): init 4\n");
      if (key == 176) printf("gec(5,6): v5: %d, v6: %d\n",
                             vn_count[v1id], vn_count[v2id]);

      tent_degree.insert(key, 0);

      printf("(%d, %d) -> %jd\n", v1id, v2id, key);
    }
  }

private:
  graph& g;
  vertex_id_map<graph>& vid_map;
  edge_id_map<graph> eid_map;
  rank_info* rinfo;
  hash_table_t& real_edge_tp_count;
  hash_table_t& real_edge_ids;
  hash_table_t& tent_degree;
  hash_table_t& en_count;
  hash_table_t& good_en_count;
  int* vn_count;
  int threshold;
  uint64_t order;
};

class hashvis {
public:
  hashvis() {}

  void operator()(int64_t key, int64_t value)
  {
    printf("Table[%jd]: %jd\n", (intmax_t) key, (intmax_t) value);
  }
};

class dhashvis {
public:
  dhashvis() {}

  void operator()(int64_t key, double value)
  {
    printf("Table[%jd]: %lf\n", (intmax_t) key, value);
  }
};

template <typename graph>
class tent_counter {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;
  typedef xmt_hash_table<int64_t, int64_t> hash_table_t;

  tent_counter(graph& gg, hash_table_t& real, hash_table_t& fake,
               hash_table_t& td, int thresh = 4) :
    g(gg), threshold(thresh), next(0), real_edges(real),
    fake_edges(fake), tent_degree(td) {}

  bool visit_test(vertex_t src)
  {
    my_degree = out_degree(src, g);
    return (my_degree <= threshold);
  }

  void pre_visit(vertex_t src)
  {
    my_neighbors = (vertex_t*) malloc(sizeof(vertex_t) * my_degree);
  }

  void operator()(edge_t e, vertex_t src, vertex_t dest)
  {
    int my_ind = mt_incr(next, 1);
    my_neighbors[my_ind] = dest;
  }

  void post_visit(vertex_t src)
  {
    typedef list<list<vertex_t> > list_list_t;
    typedef typename list<vertex_t>::iterator l_iter_t;
    typedef typename list<list<vertex_t> >::iterator ll_iter_t;

    vertex_id_map<graph> vid_map = get(_vertex_id_map, g);
    int vid = get(vid_map, src);
    unsigned int order = num_vertices(g);

    list<list<vertex_t> > all_pairs = choose(my_neighbors, my_degree, 2);

    ll_iter_t it =  all_pairs.begin();
    for ( ; it != all_pairs.end(); it++)
    {
      list<vertex_t> l = *it;
      l_iter_t lit = l.begin();
      vertex_t first = *lit++;
      vertex_t second = *lit++;

      int64_t v1id = get(vid_map, first);
      int64_t v2id = get(vid_map, second);

      if ((out_degree(first, g)  <= threshold) &&
          (out_degree(second, g) <= threshold))
      {
        order_pair(v1id, v2id);

        int64_t key = v1id * order + v2id;

        if (!real_edges.member(key))
        {
          if (!fake_edges.member(key))
          {
            fake_edges.insert(key, 1);
          }
          else
          {
            mt_incr((int64_t &)fake_edges[key], (int64_t) 1);
          }
        }
        else
        {
          mt_incr((int64_t &)real_edges[key], (int64_t) 1);
          mt_incr((int64_t &)tent_degree[key], -my_degree);
        }
      }
    }

    free(my_neighbors);
  }

  graph& g;
  int threshold;
  int64_t my_degree;
  int next;
  vertex_t* my_neighbors;
  hash_table_t& real_edges;
  hash_table_t& fake_edges;
  hash_table_t& tent_degree;
};

template <typename graph>
class ncount_corrector {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;
  typedef xmt_hash_table<int64_t, int64_t> hash_table_t;

  ncount_corrector(graph& gg, hash_table_t& real, hash_table_t& fake,
                   hash_table_t& enc, hash_table_t& genc, hash_table_t& ew,
                   int thresh = 4) :
    g(gg), threshold(thresh), next(0), real_edges(real), fake_edges(fake),
    en_count(enc), good_en_count(genc), eweight(ew), order(num_vertices(gg))
  {
    eid_map = get(_edge_id_map, g);
    vid_map = get(_vertex_id_map, g);
  }

  bool visit_test(vertex_t src)
  {
    my_degree = out_degree(src, g);
    return (my_degree <= threshold);
  }

  void pre_visit(vertex_t src)
  {
    my_neighbors = (vertex_t*) malloc(sizeof(vertex_t) * my_degree);
    my_ekeys = (int64_t*) malloc(sizeof(int64_t) * my_degree);
  }

  void operator()(edge_t e, vertex_t src, vertex_t dest)
  {
    int64_t v1id = get(vid_map, src);
    int64_t v2id = get(vid_map, dest);
    int my_ind = mt_incr(next, 1);
    my_neighbors[my_ind] = dest;
    order_pair(v1id, v2id);
    my_ekeys[my_ind] = v1id * order + v2id;
  }

  void post_visit(vertex_t src)
  {
    typedef list<list<vertex_t> > list_list_t;
    typedef typename list<vertex_t>::iterator l_iter_t;
    typedef typename list<list<vertex_t> >::iterator ll_iter_t;

    int vid = get(vid_map, src);
    unsigned int order = num_vertices(g);

    list<list<vertex_t> > all_pairs = choose(my_neighbors, my_degree, 2);

    ll_iter_t it =  all_pairs.begin();
    for ( ; it != all_pairs.end(); it++)
    {
      list<vertex_t> l = *it;
      l_iter_t lit = l.begin();
      vertex_t first = *lit++;
      vertex_t second = *lit++;

      int64_t v1id = get(vid_map, first);
      int64_t v2id = get(vid_map, second);

      if ((out_degree(first, g)  <= threshold) &&
          (out_degree(second, g) <= threshold))
      {
        order_pair(v1id, v2id);
        int64_t key = v1id * order + v2id;

        if (real_edges.member(key))
        {
          // Triangle edge - decrement by two tent poles.
          mt_incr((int64_t &)en_count[key], (int64_t) (-4 * 2));
          mt_incr((int64_t &)good_en_count[key], (int64_t) (4 * 2));

          if (key == 176) printf("v: %d, cross e:(%d,%d): gec(5,6)incr 8\n",
                                 vid, v1id, v2id); // , (int64_t&)good_en_count[key]);
          fflush(stdout);

          int k = (int) real_edges[key];
          int64_t incr = k - 1;

          for (int i = 0; i < my_degree; i++)
          {
            if((my_neighbors[i] == first) || (my_neighbors[i] == second))
            {
              mt_incr((int64_t &) en_count[my_ekeys[i]] , -incr);
              mt_incr( (int64_t &) good_en_count[my_ekeys[i]] , incr);  // add!

              int64_t nid = get(vid_map, my_neighbors[i]);

              if (my_ekeys[i] == 176)
              {
                printf("v: %d, cross e: (%d,%d), e:(%d,%d): gec(5,6): incr: "
                       "%d\n", vid, v1id, v2id, vid, nid, -incr); // , good_en_count[my_ekeys[i]]);
              }

              fflush(stdout);
            }
          }
        }
        else
        {
          int k = (int) fake_edges[key];
          int64_t incr = k - 1;

          for (int i = 0; i < my_degree; i++)
          {
            if((my_neighbors[i] == first) || (my_neighbors[i] == second))
            {
              mt_incr((int64_t &) en_count[my_ekeys[i]] , incr);
              mt_incr( (int64_t &) good_en_count[my_ekeys[i]] , incr);

              int64_t nid = get(vid_map, my_neighbors[i]);

              if (my_ekeys[i] == 176)
              {
                printf("v: %d, fake cross e: (%d,%d), e:(%d,%d): gec(5,6): "
                       "incr: %d\n", vid, v1id, v2id, vid, nid, incr); // , good_en_count[my_ekeys[i]]);
              }

              fflush(stdout);
            }
          }
        }
      }
    }

    free(my_neighbors);
    free(my_ekeys);
  }

  graph& g;
  int threshold;
  int my_degree;
  int next;
  unsigned int order;
  vertex_t* my_neighbors;
  int64_t*      my_ekeys;
  hash_table_t& real_edges;
  hash_table_t& fake_edges;
  hash_table_t& en_count;
  hash_table_t& good_en_count;
  hash_table_t& eweight;
  vertex_id_map<graph> vid_map;
  edge_id_map<graph> eid_map;
};

template <class graph>
class v_one_neighborhood_count : public default_triangles_visitor<graph> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge;
  typedef xmt_hash_table<int64_t, int64_t> hash_table_t;

  v_one_neighborhood_count(graph& gg, int* ct, vertex_id_map<graph>& vm,
                           hash_table_t& ew) :
    g(gg), count(ct), vipm(vm), eweight(ew), order(num_vertices(gg)) {}

  void operator()(vertex_t v1, vertex_t v2, vertex_t v3)
  {
    int v1id = get(vipm, v1);
    int v2id = get(vipm, v2);
    int v3id = get(vipm, v3);

    int64_t o_v1 = v1id;
    int64_t o_v2 = v2id;
    int64_t o_v3 = v3id;
    order_pair(o_v1, o_v2);

    int64_t v1_v2_key = o_v1 * order + o_v2;

    o_v2 = v2id;
    o_v3 = v3id;
    order_pair(o_v2, o_v3);

    int64_t v2_v3_key = o_v2 * order + o_v3;

    o_v1 = v1id;
    o_v3 = v3id;
    order_pair(o_v1, o_v3);

    int64_t v1_v3_key = o_v1 * order + o_v3;

    mt_incr(count[v1id], (int) eweight[v2_v3_key]);
    mt_incr(count[v2id], (int) eweight[v1_v3_key]);
    mt_incr(count[v3id], (int) eweight[v1_v2_key]);

    printf("vn: incr %d's by %d\n", v1id, (int) eweight[v2_v3_key]);
    printf("vn: incr %d's by %d\n", v2id, (int) eweight[v1_v3_key]);
    printf("vn: incr %d's by %d\n", v3id, (int) eweight[v1_v2_key]);
  }

private:
  int* count;
  unsigned int order;
  graph& g;
  vertex_id_map<graph>& vipm;
  hash_table_t& eweight;
};

template <class graph>
class e_rs_vis {
//  For each vertex, suppose it is a community leader, and assume
//  that it serves all of its neighbors.  Count the number of inter-
//  community edges connecting its community to the rest of the world.

public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;

  e_rs_vis(graph& gg, vertex_id_map<graph>& vm, int* e_rs_nm) :
    g(gg), vid_map(vm), e_rs_num(e_rs_nm) {}

  bool visit_test(vertex_t v) { return true; }

  void pre_visit(vertex_t v)
  {
    int vid = get(vid_map, v);
    vdeg = out_degree(v, g);
    neigh_deg_count = vdeg;
  }

  void operator()(edge_t e, vertex_t src, vertex_t dest)
  {
    int sid = get(vid_map, src);
    int tid = get(vid_map, dest);

    if (sid == tid) return;

    int dest_deg = out_degree(dest, g);
    int next = mt_incr(neigh_deg_count, dest_deg);
  }

  void post_visit(vertex_t v)
  {
    int vid = get(vid_map, v);
    e_rs_num[vid] = neigh_deg_count;
  }

private:
  graph& g;
  vertex_id_map<graph>& vid_map;
  int* e_rs_num;
  int neigh_deg_count;
  int vdeg;
};

template <class graph>
class e_rs_e_vis {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;
  typedef xmt_hash_table<int64_t, int64_t> hash_table_t;

  e_rs_e_vis(graph& gg, vertex_id_map<graph>& vm, hash_table_t& ers,
             int* e_rs_v) :
    g(gg), vid_map(vm), e_rs_e_num(ers),
    e_rs_v_num(e_rs_v), order(num_vertices(g)) {}

  void operator()(edge_t e)
  {
    vertex_t src = source(e, g);
    vertex_t dest = target(e, g);
    int sid = get(vid_map, src);
    int tid = get(vid_map, dest);

    if (sid == tid) return;

    order_pair(sid, tid);

    int64_t key = sid * order + tid;
    int64_t incr = e_rs_v_num[sid];

    mt_incr((int64_t &)e_rs_e_num[key], incr + e_rs_v_num[tid]);
  }

  void post_visit(vertex_t v) {}

private:
  graph& g;
  vertex_id_map<graph>& vid_map;
  hash_table_t& e_rs_e_num;
  int* e_rs_v_num;
  unsigned int order;
};

template <class graph>
class cost_visitor {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;
  typedef xmt_hash_table<int64_t, int64_t> hash_table_t;

  cost_visitor(graph& gg, int* ldrs, double* open, int* vn_cn,
               hash_table_t& en_cn, hash_table_t& good_en_cn) :
    g(gg), leaders(ldrs), opening_costs(open), vn_count(vn_cn),
    en_count(en_cn), good_en_count(good_en_cn),
    vid_map(get(_vertex_id_map, g)),
    eid_map(get(_edge_id_map, g)), order(num_vertices(g)) {}

  void operator()(edge_t e)
  {
    int eid = get(eid_map, e);
    leaders[eid] = false;

    vertex_t src = source(e, g);
    vertex_t dest = target(e, g);
    int sid = get(vid_map, src);
    int tid = get(vid_map, dest);

    if (sid == tid) return;

    order_pair(sid, tid);
    int64_t key = sid * order + tid;
    double en = en_count[key] / 4.0;
    double good_en = good_en_count[key] / 4.0;
    int64_t sn = vn_count[sid];
    int64_t tn = vn_count[tid];
    int64_t numer = (int) ceil(sn + tn - en);

    // en counts opposing edges on chordless 4-cycles, so
    // might be negative.
    if (numer < 0) numer = 0;

    opening_costs[eid] = 1 - good_en / en;
    //opening_costs[eid] = 1 - numer/en;
    //printf("oc(%d,%d) k:%jd, id: %d, 1 - "
    //       "ceil(%d + %d - %lf)/%lf=%lf\n", sid, tid, key, eid,
    //       sn, tn, en, en, opening_costs[eid]);
    printf("oc(%d,%d) k:%jd, id: %d, 1 - ceil(%lf/%lf=%lf)\n", sid, tid,
           key, eid, good_en, en, opening_costs[eid]);
  }

private:
  graph& g;
  vertex_id_map<graph> vid_map;
  edge_id_map<graph> eid_map;
  hash_table_t& en_count;
  hash_table_t& good_en_count;
  int* leaders;
  int* vn_count;
  unsigned int order;
  double* opening_costs;
};

template <class graph>
class Q_e_vis {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;
  typedef xmt_hash_table<int64_t, int64_t> hash_table_t;
  typedef xmt_hash_table<int64_t, double> dhash_table_t;

  Q_e_vis(graph& gg, vertex_id_map<graph>& vm, hash_table_t& ons,
          hash_table_t& gons, int* vnc, hash_table_t& ers,
          int* ersv, dhash_table_t& q) :
    g(gg), vid_map(vm), e_rs_e_num(ers), e_rs_v(ersv), en_count(ons),
    good_en_count(gons), Q(q), order(num_vertices(g)), vn_count(vnc),
    tot_deg(2 * num_edge(g)) {}

  void operator()(edge_t e)
  {
    vertex_t src = source(e, g);
    vertex_t dest = target(e, g);
    int sid = get(vid_map, src);
    int tid = get(vid_map, dest);

    if (sid == tid) return;

    order_pair(sid, tid);
    int64_t key = sid * order + tid;
    double neigh_sz = en_count[key] / 4.0;
    double good_neigh_sz = good_en_count[key] / 4.0;
    int64_t e_rs = e_rs_e_num[key];
    double a_s = e_rs / tot_deg;
    double q = (2 * neigh_sz / tot_deg) - (a_s * a_s);

    printf("(%d,%d): en: %.1lf, vn(%d): %d, vn(%d): %d,  ers: %jd, ers(%d): "
           "%d ers(%d): %d\n", sid, tid, neigh_sz, sid, vn_count[sid], tid,
           vn_count[tid], e_rs, sid, e_rs_v[sid], tid, e_rs_v[tid]);
  }

private:
  graph& g;
  vertex_id_map<graph>& vid_map;
  hash_table_t& e_rs_e_num;
  int* e_rs_v;
  int* vn_count;
  hash_table_t& en_count;
  hash_table_t& good_en_count;
  dhash_table_t& Q;
  double tot_deg;
  unsigned int order;
};

/*! ********************************************************************
    \class snl_community2
    \brief
 *  ********************************************************************
*/
template <typename graph>
class snl_community2 {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::adjacency_iterator adj_vertex_iter_t;
  typedef typename graph_traits<graph>::vertex_iterator vertex_iter_t;
  typedef graph Graph;
  typedef xmt_hash_table<int64_t, int64_t> hash_table_t;
  typedef xmt_hash_table<int64_t, double> dhash_table_t;

  snl_community2(graph& gg, hash_table_t& ew, int* res,
                 double* psol = 0, double* supp = 0) :
    g(gg), server(res), primal(psol), support(supp), eweight(ew) {}

  int run()
  {
    mt_timer mttimer;
    mttimer.start();

    edge_server_adapter<graph> ega(g);

    vertex_id_map<graph> vid_map = get(_vertex_id_map, g);
    edge_id_map<graph> eid_map = get(_edge_id_map, g);

    vertex_id_map<graph> es_vid_map = get(_vertex_id_map, ega);
    edge_id_map<graph> es_eid_map = get(_edge_id_map, ega);

    unsigned int order = num_vertices(g);
    int size = num_edge(g);
    if (order < 50) print(g);

    rank_info* rinfo = new rank_info[order];

    vertex_iter_t verts = vertices(g);

    #pragma mta assert nodep
    for (unsigned int i = 0; i < order; i++)
    {
      rinfo[i].rank = 1.0 / order;
      rinfo[i].degree = out_degree(verts[i], g);
    }

    int* vn_count = (int*) malloc(sizeof(int) * order);
    memset(vn_count, 0, order * sizeof(int));

    for (unsigned int i = 0; i < order; i++)
    {
      //vn_count[i] = out_degree(verts[i], g);
      vertex_t v = verts[i];
      int vid = get(vid_map, v);

      int deg = out_degree(v, g);
      adj_vertex_iter_t adj_verts = adjacent_vertices(v, g);

      for (int j = 0; j < deg; j++)
      {
        vertex_t trg = adj_verts[j];

        int trg_id = get(vid_map, trg);
        int64_t sid = vid;
        int64_t tid = trg_id;

        order_pair(sid, tid);

        int64_t key = sid * order + tid;
        int64_t val = (int64_t) eweight[key];

        vn_count[i] += val;
      }

      printf("vn: init %d's with %d\n", vid, (int) vn_count[i]);
    }

    v_one_neighborhood_count<Graph> onec(g, vn_count, vid_map, eweight);
    find_triangles<Graph, v_one_neighborhood_count<Graph> >ft(g, onec);
    ft.run();

    hash_table_t real_edges(int64_t(1.5 * size), true);
    hash_table_t fake_edges(int64_t(35 * size), true);
    hash_table_t real_edge_ids(int64_t(1.5 * size), true);
    hash_table_t en_count(int64_t(1.5 * size), true);
    hash_table_t good_en_count(int64_t(1.5 * size), true);
    hash_table_t e_rs_e_num(int64_t(1.5 * size), true);
    hash_real_edges<Graph> hre(g, vid_map, real_edges, real_edge_ids,
                               e_rs_e_num, en_count, vn_count,
                               good_en_count, rinfo, 40);

    visit_edges(g, hre);

    tent_counter<Graph> tpc(g, real_edges, fake_edges, e_rs_e_num, 40);
    visit_adj(g, tpc);

    printf("real edges: %d\n", real_edges.size());
    printf("fake edges: %d\n", fake_edges.size());

    ncount_corrector<Graph> ncc(g, real_edges, fake_edges, en_count,
                                good_en_count, eweight, 40);
    visit_adj(g, ncc);

    hashvis h;
    dhashvis dh;

    printf("final edge one neigh counts\n");
    en_count.visit(h);

    printf("final edge good one neigh counts\n");
    good_en_count.visit(h);

    int* e_rs_v = (int*) malloc(sizeof(int) * order);

    memset(e_rs_v, 0, order * sizeof(int));

    e_rs_vis<Graph> ersvis(g, vid_map, e_rs_v);
    visit_adj<Graph, e_rs_vis<Graph> >(g, ersvis);

    for (unsigned int i = 0; i < order; i++)
    {
      printf("ersv[%d]: %d\n", i, e_rs_v[i]);
    }

    e_rs_e_vis<Graph> erse(g, vid_map, e_rs_e_num, e_rs_v);
    visit_edges<Graph, e_rs_e_vis<Graph> >(g, erse);
    e_rs_e_num.visit(h);

    // ************************************************************
    // ** experimenting with edge one-neighborhood modularity
    // ************************************************************
    dhash_table_t Q_e(int64_t(1.5 * size), true);
    Q_e_vis<Graph> qv(g, vid_map, en_count, good_en_count, vn_count,
                      e_rs_e_num, e_rs_v, Q_e);
    visit_edges(g, qv);

    int nnz = ega.num_nonzero();

    printf("nnz: %d\n", nnz);

    int num_leaders = 0;         // Will do ufl soon.
    bool* leaders = (bool*) malloc(size * sizeof(bool));
    double* opening_costs = (double*) malloc(size * sizeof(double));

    // ***************************************************************

    cost_visitor<Graph> cv(g, server, opening_costs, vn_count, en_count,
                           good_en_count);
    visit_edges(g, cv);

    // NOTE: the version that tried load balance masks is in svn 1330.
    ufl_module<double, edge_server_adapter<graph> >(size, order, nnz,
                                                    num_leaders, opening_costs,
                                                    ega, 0.001, leaders,
                                                    server, primal);

    mttimer.stop();
    double sec = mttimer.getElapsedSeconds();
    printf("snl_community time: %lf\n", sec);

/*
    free(servcosts);
    num_leaders = 0;

    for (int i=0; i<ord; i++)
    {
      if (leaders[i])
      {
        num_leaders++;
        printf("leader[%d] = 1\n", i);
      }
    }

    vertex_id_map<graph> vid_map2 = get(_vertex_id_map, g);
    edge_id_map<graph> eid_map2 = get(_edge_id_map, g);
*/
    // ********************************************************
    return 0;           // maybe return a meaningful value eventually, or else
                        // make this a void method.

  }    // run()

private:
  graph& g;
  hash_table_t& eweight;
  int* server;
  double* primal;
  double* support;
};

}

#endif

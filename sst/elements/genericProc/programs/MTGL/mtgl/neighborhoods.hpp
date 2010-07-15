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
/*! \file neighborhoods.hpp

    \author Jon Berry (jberry@sandia.gov)

    \date 9/2008
*/
/****************************************************************************/

#ifndef MTGL_NEIGHBORHOODS_HPP
#define MTGL_NEIGHBORHOODS_HPP

#include <cstdio>
#include <climits>
#include <cstdlib>
#include <sys/time.h>

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

#define THRESH  400     // Above this vertex degree, don't record stats.

namespace mtgl {

typedef struct {
  int degree;

#ifdef USING_QTHREADS
  aligned_t acc;
#else
  double acc;
#endif

  double rank;
} rank_info;

template <typename graph>
class hash_real_edges {
public:
  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;
  typedef xmt_hash_table<int64_t, size_type> hash_table_t;

  hash_real_edges(graph& gg, vertex_id_map<graph>& vm, hash_table_t& tpc,
                  hash_table_t& ons, hash_table_t& gons, hash_table_t& el,
                  hash_table_t& er, hash_table_t& ew, rank_info* rin,
                  int thresh) :
    g(gg), vid_map(vm), real_edge_tp_count(tpc), rinfo(rin),
    threshold(thresh), order(num_vertices(gg)), en_count(ons),
    good_en_count(gons), e_left(el), e_right(er), e_weight(ew) {}

  void operator()(edge_t e)
  {
    vertex_t v1 = source(e, g);
    vertex_t v2 = target(e, g);
    int v1id = get(vid_map, v1);
    int v2id = get(vid_map, v2);

    if ((rinfo[v1id].degree <= threshold) && (rinfo[v2id].degree <= threshold))
    {
      order_pair(v1id, v2id);

      int64_t key = v1id * order + v2id;
      size_type wgt = e_weight[key];
      real_edge_tp_count.insert(key, 0);
      cout << "real edge " << key << " (" << v1id << ", " << v2id << ")" << endl;
      e_left.insert(key, 0);
      e_right.insert(key, 0);
      en_count.insert(key, 0);

      if (!good_en_count.member(key))
      {
        good_en_count.insert(key, wgt);
      }
      else
      {
        good_en_count.update(key, wgt);
      }
    }
  }

private:
  graph& g;
  vertex_id_map<graph>& vid_map;
  rank_info* rinfo;
  hash_table_t& real_edge_tp_count;
  hash_table_t& en_count;
  hash_table_t& good_en_count;
  hash_table_t& e_left;
  hash_table_t& e_right;
  hash_table_t& e_weight;
  int threshold;
  uint64_t order;
};

/*
template <typename graph>
class encount_initializer {
public:
  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;
  typedef xmt_hash_table<int64_t, size_type> hash_table_t;

  encount_initializer(graph& gg, vertex_id_map<graph>& vm,
                      size_type *vnc, hash_table_t& enc, hash_table_t& ew) :
    g(gg), vid_map(vm), vn_count(vnc), en_count(enc),
    e_weight(ew), order(num_vertices(gg)) {}

  void operator()(edge_t e)
  {
    vertex_t v1 = source(e,g);
    vertex_t v2 = target(e,g);
    size_type v1id = get(vid_map, v1);
    size_type v2id = get(vid_map, v2);
    size_type vn1 = vn_count[v1id];
    size_type vn2 = vn_count[v2id];

    order_pair(v1id, v2id);

    int64_t key = v1id * order + v2id;
    size_type wgt = e_weight[key];
    size_type incr = (vn1 + vn2) - wgt;
    en_count.int_fetch_add(key,incr);

    //printf("encount init: (%d,%d): after incr:(%d + %d - %d)=%d, enc: %d\n",
    //       v1id,v2id,vn1, vn2, wgt, incr,
    //       (size_type)en_count[key]);
  }
private:
  graph& g;
  unsigned int order;
  vertex_id_map<graph>& vid_map;
  size_type *vn_count;
  hash_table_t& en_count;
  hash_table_t& e_weight;
};
*/

template <typename graph>
class encount_initializer {
public:
  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;
  typedef xmt_hash_table<int64_t, size_type> hash_table_t;

  encount_initializer(graph& gg, vertex_id_map<graph>& vm, size_type* vnc,
                      hash_table_t& enc, hash_table_t& ew,
                      int thresh = THRESH) :
    g(gg), vid_map(vm), vn_count(vnc), en_count(enc),
    e_weight(ew), order(num_vertices(gg)), threshold(thresh) {}

  void operator()(vertex_t src, vertex_t dest)
  {
    size_type v1id = get(vid_map, src);
    size_type v2id = get(vid_map, dest);

    size_type my_degree = out_degree(src, g);
    size_type dest_deg = out_degree(dest, g);

    if (my_degree > threshold) return;
    if (dest_deg > threshold) return;

    size_type src_id = get(vid_map, src);
    size_type dest_id = get(vid_map, dest);

    if (src_id > dest_id) return;

    int64_t key = src_id * order + dest_id;
    size_type wgt = e_weight[key];
    size_type vn1 = vn_count[src_id];
    size_type vn2 = vn_count[dest_id];
    size_type incr = (vn1 + vn2) - wgt;
    en_count.int_fetch_add(key, incr);

    //printf("encount init: (%d,%d): after incr:(%d + %d - %d)=%d, enc: %d\n",
    //       v1id,v2id,vn1, vn2, wgt, incr,
    //       (size_type)en_count[key]);
  }

  void post_visit() {}

private:
  graph& g;
  unsigned int order;
  vertex_id_map<graph>& vid_map;
  size_type* vn_count;
  hash_table_t& en_count;
  hash_table_t& e_weight;
  int threshold;
};

typedef int64_t intmax_t;

class hashvis {
public:
  hashvis() {}

  void operator()(int64_t key, int64_t value)
  {
    printf("Table[%jd]: %jd\n", (intmax_t) key, (intmax_t) value);
    fflush(stdout);
  }
};
class dhashvis {
public:
  dhashvis() {}

  void operator()(int64_t key, double value)
  {
    printf("Table[%jd]: %f\n", (intmax_t) key, value);
  }
};

template <typename graph>
class fake_edge_finder {
public:
  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;
  typedef typename graph_traits<graph>::adjacency_iterator adj_iter_t;
  typedef xmt_hash_table<int64_t, size_type> hash_table_t;

  fake_edge_finder(graph& gg, rank_info* rin, hash_table_t& real,
                   hash_table_t& fake, hash_table_t& el, hash_table_t& er,
                   int thresh = THRESH) :
    g(gg), threshold(thresh), next(0), real_edges(real), fake_edges(fake),
    e_left(el), e_right(er), rinfo(rin),
    vid_map(get(_vertex_id_map, g)), order(num_vertices(g)) {}

  void operator()(vertex_t src, vertex_t dest)
  {
    size_type my_degree = out_degree(src, g);
    size_type dest_deg = out_degree(dest, g);

    if (my_degree > threshold) return;
    if (dest_deg > threshold) return;

    size_type src_id = get(vid_map, src);
    size_type dest_id = get(vid_map, dest);

    cout << "fef edge (" << src_id << ", " << dest_id << ")" << endl;

    adj_iter_t adj_verts = adjacent_vertices(src, g);

    for (size_type j = 0; j < my_degree; j++)
    {
      vertex_t sibling = adj_verts[j];
      size_type sib_id = get(vid_map, sibling);
      if (sib_id == dest_id) continue;

      size_type sib_deg = out_degree(sibling, g);
      if (sib_deg > threshold) continue;

      if (dest_id < sib_id)
      {
        int64_t key = dest_id * order + sib_id;

        cout << "hashing (" << src_id << ", " << sib_id << ")" << endl;

        if (!real_edges.member(key))
        {
          cout << "not a real edge " << key << " (" << dest_id << ", "
               << sib_id << ")" << endl;

          if (!fake_edges.member(key))
          {
            cout << "adding fake edge (" << dest_id << ", " << sib_id << ")"
                 << endl;

            fake_edges.insert(key, 1);
            e_left.insert(key, 0);
            e_right.insert(key, 0);
          }
        }
      }
    }
  }

  void post_visit() {}

private:
  graph& g;
  int threshold;
  size_type order;
  int next;
  vertex_id_map<graph> vid_map;
  vertex_t* my_neighbors;
  hash_table_t& real_edges;
  hash_table_t& fake_edges;
  hash_table_t& e_left;
  hash_table_t& e_right;
  rank_info* rinfo;
};

template <typename graph>
class vn_initializer {
public:
  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;
  typedef xmt_hash_table<int64_t, size_type> hash_table_t;

  vn_initializer(graph& gg, size_type* vnc, hash_table_t& ew,
                 int thresh = THRESH) :
    g(gg), threshold(thresh), eweight(ew), vn_count(vnc),
    order(num_vertices(gg))
  {
    vid_map = get(_vertex_id_map, g);
  }

  void operator()(vertex_t src, vertex_t dest)
  {
    size_type v1id = get(vid_map, src);
    size_type v2id = get(vid_map, dest);

    size_type my_degree = out_degree(src, g);
    size_type dest_deg = out_degree(dest, g);

    if (my_degree > threshold) return;
    if (dest_deg > threshold) return;

    size_type src_id = get(vid_map, src);
    size_type v_id = src_id;
    size_type dest_id = get(vid_map, dest);

    order_pair(v_id, dest_id);

    int64_t key = v_id * order + dest_id;
    size_type wgt = eweight[key];
    mt_incr(vn_count[src_id], wgt);
    printf("vn_count[%d]: init incr by %d due to (%d,%d)\n",
           src_id, wgt, v_id, dest_id);
  }

  void post_visit() {}

private:
  graph& g;
  int threshold;
  unsigned int order;
  size_type* vn_count;
  hash_table_t& eweight;
  vertex_id_map<graph> vid_map;
};

template <typename graph>
class en_finalizer {
public:
  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;
  typedef xmt_hash_table<int64_t, size_type> hash_table_t;

  en_finalizer(graph& gg, hash_table_t& ew, hash_table_t& genc,
               hash_table_t& enc, int un = 1024, int thresh = THRESH) :
    g(gg), threshold(thresh), eweight(ew), good_en_count(genc),
    en_count(enc), unit(un), order(num_vertices(gg))
  {
    vid_map = get(_vertex_id_map, g);
  }

  void operator()(vertex_t src, vertex_t dest)
  {
    size_type my_degree = out_degree(src, g);
    size_type dest_deg = out_degree(dest, g);

    if (my_degree > threshold) return;
    if (dest_deg > threshold) return;

    size_type src_id = get(vid_map, src);
    size_type dest_id = get(vid_map, dest);
    if (dest_id < src_id) return;

    int64_t key = src_id * order + dest_id;
    size_type genc = good_en_count[key];
    size_type enc = en_count[key];

    if (genc > enc)
    {
      printf("ERROR %jd->(%jd,%jd), good: %d, enc: %d\n",
             key, src_id, dest_id, genc, enc);
    }

    if (enc > 0)
    {
      eweight[key] = (size_type) (unit * (good_en_count[key] / (double) enc));
    }
    else
    {
      eweight[key] = (size_type) 0;
    }
    //printf("ew[%jd]: (%d/%d) = %d\n", key, good_en_count[key],
    //      en_count[key], eweight[key]);
  }

  void post_visit() {}

private:
  graph& g;
  int threshold;
  int unit;
  unsigned int order;
  size_type* vn_count;
  hash_table_t& good_en_count;
  hash_table_t& en_count;
  hash_table_t& eweight;
  vertex_id_map<graph> vid_map;
};

template <typename graph>
class error_accumulator {
public:
  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::adjacency_iterator adj_iter_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;
  typedef xmt_hash_table<int64_t, size_type> hash_table_t;

  error_accumulator(graph& gg, hash_table_t& real, hash_table_t& fake,
                    hash_table_t& enc, hash_table_t& genc, hash_table_t& ew,
                    hash_table_t& el, hash_table_t& er, int thresh = THRESH) :
    g(gg), threshold(thresh), next(0), real_edges(real), fake_edges(fake),
    en_count(enc), good_en_count(genc), eweight(ew),
    order(num_vertices(gg)), e_left(el), e_right(er)
  {
    eid_map = get(_edge_id_map, g);
    vid_map = get(_vertex_id_map, g);
  }

  void operator()(vertex_t src, vertex_t dest)
  {
    size_type my_degree = out_degree(src, g);
    size_type dest_deg = out_degree(dest, g);

    if (my_degree > threshold) return;
    if (dest_deg > threshold) return;

    size_type src_id = get(vid_map, src);
    size_type dest_id = get(vid_map, dest);

    adj_iter_t adj_verts = adjacent_vertices(src, g);

    for (size_type j = 0; j < my_degree; j++)
    {
      vertex_t sibling = adj_verts[j];

      size_type sib_id = get(vid_map, sibling);
      if (sib_id == dest_id) continue;

      size_type sib_deg = out_degree(sibling, g);
      if (sib_deg > threshold) continue;

      if (dest_id < sib_id)
      {
        size_type v_id = src_id;
        size_type v1id = dest_id;
        size_type v2id = sib_id;

        order_pair(v1id, v2id);

        int64_t key = v1id * order + v2id;
        v1id = dest_id;
        v2id = sib_id;

        order_pair(v_id, v1id);

        int64_t l_key = v_id * order + v1id;
        size_type wgt = (size_type) eweight[l_key];
        //e_left.update(key, e_left[key]+wgt);
        e_left[key] += wgt;
        v_id = src_id;
        //printf("e_left[%jd]: now %d\n", key,e_left[key]);

        order_pair(v_id, v2id);

        int64_t r_key = v_id * order + v2id;
        wgt = eweight[r_key];
        //e_right.update(key, e_right[key]+wgt);
        e_right[key] += wgt;
        //printf("e_right[%jd]: now %d\n", key,e_right[key]);
      }
    }
  }

  void post_visit() {}

private:
  graph& g;
  int threshold;
  int my_degree;
  int my_id;
  int next;
  unsigned int order;
  vertex_t* my_neighbors;
  hash_table_t& real_edges;
  hash_table_t& fake_edges;
  hash_table_t& e_left;
  hash_table_t& e_right;
  hash_table_t& en_count;
  hash_table_t& good_en_count;
  hash_table_t& eweight;
  vertex_id_map<graph> vid_map;
  edge_id_map<graph> eid_map;
};

template <typename graph>
class ncount_correcter {
public:
  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;
  typedef typename graph_traits<graph>::adjacency_iterator adj_iter_t;
  typedef xmt_hash_table<int64_t, size_type> hash_table_t;

  ncount_correcter(graph& gg, hash_table_t& real, hash_table_t& fake,
                   hash_table_t& enc, hash_table_t& genc, hash_table_t& ew,
                   hash_table_t& el, hash_table_t& er, int thresh = THRESH) :
    g(gg), threshold(thresh), next(0), real_edges(real), fake_edges(fake),
    en_count(enc), good_en_count(genc), eweight(ew),
    order(num_vertices(gg)), e_left(el), e_right(er)
  {
    eid_map = get(_edge_id_map, g);
    vid_map = get(_vertex_id_map, g);
  }

  void operator()(vertex_t src, vertex_t dest)
  {
    size_type my_degree = out_degree(src, g);
    size_type dest_deg = out_degree(dest, g);

    if (my_degree > threshold) return;
    if (dest_deg > threshold) return;

    size_type src_id = get(vid_map, src);
    size_type dest_id = get(vid_map, dest);

    adj_iter_t adj_verts = adjacent_vertices(src, g);

    for (size_type j = 0; j < my_degree; j++)
    {
      vertex_t sibling = adj_verts[j];

      size_type sib_id = get(vid_map, sibling);
      if (sib_id == dest_id) continue;

      size_type sib_deg = out_degree(sibling, g);
      if (sib_deg > threshold) continue;

      if (dest_id < sib_id)
      {
        size_type v1id = dest_id;
        size_type v2id = sib_id;
        size_type v1id_bak = v1id;
        size_type v2id_bak = v2id;

        order_pair(v1id, v2id);

        int64_t key = v1id * order + v2id;
        size_type v_id = src_id;
        v1id = v1id_bak;
        v2id = v2id_bak;

        order_pair(v_id, v1id);

        int64_t l_key = v_id * order + v1id;
        size_type l_wgt = eweight[l_key];
        v_id = src_id;

        order_pair(v_id, v2id);

        int64_t r_key = v_id * order + v2id;
        size_type r_wgt = eweight[r_key];
        size_type er = e_right[key];
        size_type el = e_left[key];

        if (real_edges.member(key))
        {
          //printf("%d -> %jd(%d,%d): add %lf to %jd, add %lf to %jd\n",
          //       src_id, key, v1id_bak, v2id_bak,
          //       - 0.25*(er-r_wgt), l_key,
          //       - 0.25*(el-l_wgt), r_key);
          //en_count[l_key] += (size_type)(- 0.25*(er-r_wgt));
          //en_count[r_key] += (size_type)(- 0.25*(el-l_wgt));
          //good_en_count[l_key]+=(size_type)(0.25*(er-r_wgt));
          //good_en_count[r_key]+=(size_type)(0.25*(el-l_wgt));
        }
        else if (fake_edges.member(key))
        {
          //printf("%d -> %jd(%d,%d): add %lf to %jd, add %lf to %jd\n",
          //       src_id, key, v1id_bak, v2id_bak,
          //       + 0.25*(er-r_wgt), l_key,
          //       + 0.25*(el-l_wgt), r_key);
          //en_count[l_key] += (size_type)(0.25*(er-r_wgt));
          //en_count[r_key] += (size_type)(0.25*(el-l_wgt));
          //good_en_count[l_key] += (size_type)(0.25*(er-r_wgt));
          //good_en_count[r_key] += (size_type)(0.25*(el-l_wgt));
          if (el > l_wgt || er > r_wgt)                         // "spokes"
          {     printf("%d -> %jd(%d,%d): add %jd to %jd, add %jd to %jd\n",
                       src_id, key, v1id_bak, v2id_bak,
                       + r_wgt, l_key,
                       + l_wgt, r_key);
            good_en_count[l_key] += r_wgt;
            good_en_count[r_key] += l_wgt;
          }
        }
      }
    }
  }

  void post_visit() {}

private:
  graph& g;
  int threshold;
  int next;
  unsigned int order;
  vertex_t* my_neighbors;
  hash_table_t& real_edges;
  hash_table_t& fake_edges;
  hash_table_t& e_left;
  hash_table_t& e_right;
  hash_table_t& en_count;
  hash_table_t& good_en_count;
  hash_table_t& eweight;
  vertex_id_map<graph> vid_map;
  edge_id_map<graph> eid_map;
};

template <typename graph>
class goodness_correcter {
public:
  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;
  typedef xmt_hash_table<int64_t, size_type> hash_table_t;

  goodness_correcter(graph& gg, size_type* vnc, hash_table_t& enc,
                     hash_table_t& genc, hash_table_t& ew,
                     int thresh = THRESH) :
    g(gg), threshold(thresh), en_count(enc), vn_count(vnc),
    good_en_count(genc), eweight(ew), order(num_vertices(gg))
  {
    vid_map = get(_vertex_id_map, g);
  }

  void operator()(vertex_t src, vertex_t dest)
  {
    size_type my_degree = out_degree(src, g);
    size_type dest_deg = out_degree(dest, g);

    if (my_degree > threshold) return;
    if (dest_deg > threshold) return;

    size_type src_id = get(vid_map, src);
    size_type dest_id = get(vid_map, dest);

    if (src_id > dest_id) return;

    int64_t key = src_id * order + dest_id;
    size_type my_weight = eweight[key];
    size_type num_bad = vn_count[src_id] + vn_count[dest_id]
                        - my_weight - good_en_count[key];
    good_en_count[key] = en_count[key] - num_bad;
  }

  void post_visit() {}

private:
  graph& g;
  int threshold;
  size_type order;
  size_type* vn_count;
  hash_table_t& en_count;
  hash_table_t& good_en_count;
  hash_table_t& eweight;
  vertex_id_map<graph> vid_map;
};

template <class graph>
class v_one_neighborhood_count : public default_triangles_visitor<graph> {
public:
  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge;
  typedef xmt_hash_table<int64_t, size_type> hash_table_t;

  v_one_neighborhood_count(graph& gg, size_type* ct, vertex_id_map<graph>& vm,
                           hash_table_t& real_e, hash_table_t& ew,
                           hash_table_t& enc, hash_table_t& genc,
                           rank_info* rin, int thresh = THRESH) :
    g(gg), count(ct), vipm(vm), eweight(ew), order(num_vertices(gg)),
    real_edges(real_e), en_count(enc), good_en_count(genc), rinfo(rin),
    threshold(thresh) {}

  void operator()(vertex_t v1, vertex_t v2, vertex_t v3)
  {
    int v1id = get(vipm, v1);
    int v2id = get(vipm, v2);
    int v3id = get(vipm, v3);

    this->operator()(v1id, v2id, v3id);
  }

  void operator()(size_type v1id, size_type v2id, size_type v3id)
  {
    if ((rinfo[v1id].degree > threshold) ||
        (rinfo[v1id].degree > threshold) ||
        (rinfo[v1id].degree > threshold))
    {
      return;
    }

    size_type wgt;
    int64_t o_v1 = v1id, o_v2 = v2id, o_v3 = v3id;

    order_pair(o_v1, o_v2);

    int64_t v1_v2_key = o_v1 * order + o_v2;
    o_v2 = v2id; o_v3 = v3id;

    order_pair(o_v2, o_v3);

    int64_t v2_v3_key = o_v2 * order + o_v3;
    o_v1 = v1id; o_v3 = v3id;

    order_pair(o_v1, o_v3);

    int64_t v1_v3_key = o_v1 * order + o_v3;
    size_type w_v1_v2 = eweight[v1_v2_key];
    size_type w_v2_v3 = eweight[v2_v3_key];
    size_type w_v1_v3 = eweight[v1_v3_key];

    //mt_incr(count[v1id], w_v2_v3);  JWB: to compute the
    //mt_incr(count[v2id], w_v1_v3);  true one-neigh sizes,
    //mt_incr(count[v3id], w_v1_v2);  need these.  In the
    //meantime, leave out (for reslimit paper)
    //printf("tri(%d,%d,%d): add %jd to vc(%jd)\n",
    //       v1id,v2id,v3id, w_v2_v3, v1id);
    //printf("tri(%d,%d,%d): add %jd to vc(%jd)\n",
    //       v1id,v2id,v3id, w_v1_v3, v2id);
    //printf("tri(%d,%d,%d): add %jd to vc(%jd)\n",
    //       v1id,v2id,v3id, w_v1_v2, v3id);

    size_type v1_v2_incr = (w_v1_v3 + w_v2_v3);
    size_type v2_v3_incr = (w_v1_v2 + w_v1_v3);
    size_type v1_v3_incr = (w_v1_v2 + w_v2_v3);

    //en_count.int_fetch_add(v1_v3_key, -v1_v3_incr);
    //en_count.int_fetch_add(v2_v3_key, -v2_v3_incr);
    //en_count.int_fetch_add(v1_v2_key, -v1_v2_incr);
    //printf("tri(%d,%d,%d): add %jd to enc(%jd)\n",
    //       1id,v2id,v3id, -v1_v3_incr, v1_v3_key);
    //printf("tri(%d,%d,%d): add %jd to enc(%jd)\n",
    //       1id,v2id,v3id, -v2_v3_incr, v2_v3_key);
    //printf("tri(%d,%d,%d): add %jd to enc(%jd)\n",
    //       1id,v2id,v3id, -v1_v2_incr, v1_v2_key);

    good_en_count.int_fetch_add(v1_v2_key, v1_v2_incr);
    good_en_count.int_fetch_add(v1_v3_key, v1_v3_incr);
    good_en_count.int_fetch_add(v2_v3_key, v2_v3_incr);
  }

private:
  size_type* count;
  unsigned int order;
  graph& g;
  vertex_id_map<graph>& vipm;
  hash_table_t& real_edges;
  hash_table_t& eweight;
  hash_table_t& en_count;
  hash_table_t& good_en_count;
  rank_info* rinfo;
  int threshold;
};

template <typename graph>
class neighbor_counts {
public:
  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;
  typedef typename graph_traits<graph>::adjacency_iterator adj_vertex_iter_t;
  typedef typename graph_traits<graph>::vertex_iterator vertex_iter_t;
  typedef graph Graph;
  typedef xmt_hash_table<int64_t, size_type> hash_table_t;

  neighbor_counts(graph& gg, hash_table_t& ew, size_type thr = THRESH) :
    g(gg), eweight(ew), thresh(thr) {}

  void run(size_type* vn_count, hash_table_t& en_count,
           hash_table_t& good_en_count)
  {
    vertex_id_map<graph> vid_map = get(_vertex_id_map, g);
    edge_id_map<graph> eid_map   = get(_edge_id_map, g);

    unsigned int order = num_vertices(g);
    int size = num_edges(g);
    hashvis h;

    if (order < 50) print(g);

    #pragma mta trace "before inits"

    rank_info* rinfo = new rank_info[order];

    vertex_iter_t verts = vertices(g);

    #pragma mta assert nodep
    for (unsigned int i = 0; i < order; i++)
    {
      rinfo[i].rank = 1.0 / order;
      rinfo[i].degree = out_degree(verts[i], g);
    }

    memset(vn_count, 0, order * sizeof(size_type));

    printf("hash size: %d\n", eweight.size());
    printf("hash alloc size: %d\n", eweight.alloc_size());
    fflush(stdout);

    #pragma mta trace "after inits"
    #pragma mta trace "before vn_init"

    size_type* prefix_counts = 0, * started_nodes = 0;
    size_type num_threads = 0;

    vn_initializer<graph> vni(g, vn_count, eweight, thresh);
    visit_adj(g, vni, prefix_counts, started_nodes, num_threads);

    #pragma mta trace "after vn_init"
    #pragma mta trace "before more inits"

    printf("hash size: %d\n", eweight.size());
    printf("hash alloc size: %d\n", eweight.alloc_size());

    hash_table_t real_edges(int64_t(1.5 * size), true);
    hash_table_t fake_edges(int64_t(thresh * size), true); // need estimat. num
    hash_table_t e_left(int64_t(thresh * size), true);     // need to est. num
    hash_table_t e_right(int64_t(thresh * size), true);    // fake edges

    #pragma mta trace "after more inits"
    #pragma mta trace "before hash_real_edges"

    hash_real_edges<Graph> hre(g, vid_map, real_edges,
                               en_count, good_en_count, e_left, e_right,
                               eweight, rinfo, thresh);

    visit_edges(g, hre);

    #pragma mta trace "after hash_real_edges"

    v_one_neighborhood_count<Graph>
    onec(g, vn_count, vid_map, real_edges, eweight, en_count,
         good_en_count, rinfo);
    find_triangles<Graph, v_one_neighborhood_count<Graph> >ft(g, onec, thresh);

    // Change to "find_triangles" for MTA  (and static_graph).
    mt_timer tri_timer;
    int issues, memrefs, concur, streams, traps;
    init_mta_counters(tri_timer, issues, memrefs, concur, streams, traps);

    #pragma mta trace "before triangles"
    ft.run();
    #pragma mta trace "after triangles"

    sample_mta_counters(tri_timer, issues, memrefs, concur, streams, traps);
    print_mta_counters(tri_timer, num_edges(g),
                       issues, memrefs, concur, streams, traps);

    //for (int i=0; i<order; i++) {
    //  printf("vn_count[%d]: %d\n", i, vn_count[i]);
    //}

    encount_initializer<Graph> nci(g, vid_map, vn_count, en_count,
                                   eweight, thresh);
    mt_timer en_timer;
    init_mta_counters(en_timer, issues, memrefs, concur, streams, traps);

    #pragma mta trace "before encount_init"
    visit_adj(g, nci, prefix_counts, started_nodes, num_threads);
    #pragma mta trace "after encount_init"

    sample_mta_counters(en_timer, issues, memrefs, concur, streams, traps);
    printf("called encount_init\n"); fflush(stdout);
    print_mta_counters(en_timer, num_edges(g),
                       issues, memrefs, concur, streams, traps);

    fake_edge_finder<Graph> tpc(g, rinfo, real_edges, fake_edges, e_left,
                                e_right, thresh);

    mt_timer fe_timer;
    init_mta_counters(fe_timer, issues, memrefs, concur, streams, traps);

    #pragma mta trace "before fake_edge_finder"
    visit_adj(g, tpc, prefix_counts, started_nodes, num_threads);
    #pragma mta trace "after fake_edge_finder"

    sample_mta_counters(fe_timer, issues, memrefs, concur, streams, traps);
    printf("found fake edges\n"); fflush(stdout);
    print_mta_counters(fe_timer, num_edges(g),
                       issues, memrefs, concur, streams, traps);
    printf("real edges: %d\n", real_edges.size());
    printf("fake edges: %d\n", fake_edges.size());

    error_accumulator<Graph> ea(g, real_edges, fake_edges, en_count,
                                good_en_count, eweight, e_left,
                                e_right, thresh);

    mt_timer nc_timer;
    init_mta_counters(nc_timer, issues, memrefs, concur, streams, traps);

    #pragma mta trace "before error_accum"
    visit_adj(g, ea, prefix_counts, started_nodes, num_threads);
    #pragma mta trace "after error_accum"

    sample_mta_counters(nc_timer, issues, memrefs, concur, streams, traps);
    printf("called error_accum\n"); fflush(stdout);
    print_mta_counters(nc_timer, num_edges(g),
                       issues, memrefs, concur, streams, traps);

    ncount_correcter<Graph> ncc(g, real_edges, fake_edges, en_count,
                                good_en_count, eweight, e_left,
                                e_right, thresh);

    mt_timer ncc_timer;
    init_mta_counters(ncc_timer, issues, memrefs, concur, streams, traps);

    #pragma mta trace "before ncount_corr"
    visit_adj(g, ncc, prefix_counts, started_nodes, num_threads);
    #pragma mta trace "after ncount_corr"

    sample_mta_counters(ncc_timer, issues, memrefs, concur, streams, traps);
    printf("called ncount_corr\n"); fflush(stdout);
    print_mta_counters(ncc_timer, num_edges(g),
                       issues, memrefs, concur, streams, traps);

    // *******************************************************
    // JWB: the code below is deprecated.  It was correct,
    //      but not helpful.
    // *******************************************************
    // Now we have to correct the goodness computation.  First,
    // we have to restore the sum of the weights of incident
    // edges for each vertex.

    //memset(vn_count, 0, order*sizeof(size_type));
    //visit_adj(g, vni, prefix_counts, started_nodes, num_threads);

    //goodness_correcter<Graph> gc(g, vn_count, en_count,
    //                             good_en_count, eweight, THRESH);
    //mt_timer gc_timer;
    //init_mta_counters(gc_timer, issues, memrefs, concur,
    //                    streams, traps);
    //#pragma mta trace "before goodness_corr"
    //visit_adj(g, gc,prefix_counts, started_nodes, num_threads);
    //#pragma mta trace "after goodness_corr"
    //sample_mta_counters(ncc_timer, issues, memrefs, concur,
    //                    streams, traps);
    //printf("called goodness_corr\n"); fflush(stdout);
    //print_mta_counters(ncc_timer, num_edges(g),
    //                   issues, memrefs, concur, streams, traps);
    // *******************************************************
  }

  void run_without_rectangles(size_type* vn_count, hash_table_t& en_count,
                              hash_table_t& good_en_count)
  {
    vertex_id_map<graph> vid_map = get(_vertex_id_map, g);
    edge_id_map<graph> eid_map = get(_edge_id_map, g);

    unsigned int order = num_vertices(g);
    int size = num_edges(g);
    hashvis h;

    if (order < 50) print(g);

    rank_info* rinfo = new rank_info[order];

    vertex_iter_t verts = vertices(g);

    #pragma mta assert nodep
    for (unsigned int i = 0; i < order; i++)
    {
      rinfo[i].rank = 1.0 / order;
      rinfo[i].degree = out_degree(verts[i], g);
    }

    memset(vn_count, 0, order * sizeof(size_type));

    for (unsigned int i = 0; i < order; i++)
    {
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
        mt_incr(vn_count[i], eweight[key]);

        printf("init: after (%d,%d), vc[%d]: %d\n", sid, tid, i, vn_count[i]);
      }
    }

    hash_table_t real_edges(int64_t(1.5 * size), true);
    hash_table_t fake_edges(int64_t(35 * size), true);  // need estimat. num
    hash_table_t e_left(int64_t(1.5 * size), true);     // need to est. num
    hash_table_t e_right(int64_t(1.5 * size), true);    // fake edges

    hash_real_edges<Graph> hre(g, vid_map, real_edges, en_count,
                               good_en_count, e_left, e_right,
                               eweight, rinfo, thresh);

    visit_edges(g, hre);

    v_one_neighborhood_count<Graph>
    onec(g, vn_count, vid_map, real_edges, eweight, en_count,
         good_en_count, rinfo);
    find_triangles<Graph, v_one_neighborhood_count<Graph> >ft(g, onec);
    ft.run();

    encount_initializer<Graph> nci(g, vid_map, vn_count, en_count, eweight);
    visit_edges(g, nci);

    fake_edge_finder<Graph> tpc(g, rinfo, real_edges,
                                fake_edges, e_left, e_right, thresh);

    size_type* prefix_counts = 0, * started_nodes = 0;
    size_type num_threads = 0;

    visit_adj(g, tpc);

    printf("real edges: %d\n", real_edges.size());
    printf("fake edges: %d\n", fake_edges.size());

    error_accumulator<Graph> ea(g, real_edges, fake_edges, en_count,
                                good_en_count, eweight, e_left,
                                e_right, thresh);

    visit_adj(g, ea, prefix_counts, started_nodes, num_threads);

    ncount_correcter<Graph> ncc(g, real_edges, fake_edges, en_count,
                                good_en_count, eweight, e_left, e_right,
                                thresh);

    visit_adj(g, ncc, prefix_counts, started_nodes, num_threads);

    goodness_correcter<Graph> gc(g,  en_count, good_en_count,
                                 eweight, thresh);

    visit_adj(g, gc, prefix_counts, started_nodes, num_threads);
  }

private:
  graph& g;
  hash_table_t& eweight;
  size_type thresh;
};

template <typename graph>
class weight_by_neighborhoods {
public:
  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;
  typedef typename graph_traits<graph>::edge_iterator edge_iter_t;
  typedef graph Graph;
  typedef xmt_hash_table<int64_t, size_type> hash_table_t;

  weight_by_neighborhoods(graph& gg, hash_table_t& ew, size_type thr = THRESH,
                          size_type un = 1024) :
    g(gg), eweight(ew), unit(un), thresh(thr) {}

  void run(int num_iter, hash_table_t& good_en_count, hash_table_t& en_count)
  {
    vertex_id_map<Graph> vid_map = get(_vertex_id_map, g);

    unsigned int order = num_vertices(g);
    int size = num_edges(g);

    for (int iter = 0; iter < num_iter; iter++)
    {
      size_type* vn_count = (size_type*) malloc(sizeof(size_type) * order);

      neighbor_counts<Graph> nc(g, eweight, thresh);
      nc.run(vn_count, en_count, good_en_count);

      size_type* prefix_counts = 0, * started_nodes = 0;
      size_type num_threads = 0;
      en_finalizer<graph> enf(g, eweight, good_en_count, en_count,
                              unit, thresh);
      visit_adj(g, enf, prefix_counts, started_nodes, num_threads);

/*
      edge_iter_t edgs = edges(g);

      for (int i = 0; i < size; ++i)
      {
        edge_t e = edgs[i];
        vertex_t v1 = source(e, g);
        vertex_t v2 = target(e, g);
        int v1id = get(vid_map, v1);
        int v2id = get(vid_map, v2);
        order_pair(v1id,v2id);
        int64_t key = v1id * order + v2id;
        //eweight[key] = 1.0;
        size_type genc = good_en_count[key];
        size_type enc = en_count[key];
        if (genc > enc)
        {
          printf("ERROR %jd->(%jd,%jd), good: %d, enc: %d\n",
                 key, v1id, v2id, genc, enc);
        }
        if (enc > 0)
        {
          eweight[key] = (size_type) (unit*(good_en_count[key]/(double)enc));
        }
        else
        {
          eweight[key] = (size_type) 0;
        }

        //printf("ew[%jd]: (%d/%d) = %d\n", key, good_en_count[key],
        //      en_count[key], eweight[key]);
      }  // reset weights
*/

      free(vn_count);
    }              // iter
  }

private:
  Graph& g;
  hash_table_t& eweight;
  size_type unit;             // for simulating floating point with int
  size_type thresh;           // below this vertex degree, ignore
};

}

#endif

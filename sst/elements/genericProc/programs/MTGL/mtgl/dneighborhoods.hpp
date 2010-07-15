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
/*! \file dneighborhoods.hpp

    \author Jon Berry (jberry@sandia.gov)

    \date 9/2008
*/
/****************************************************************************/

#ifndef MTGL_DNEIGHBORHOODS_HPP
#define MTGL_DNEIGHBORHOODS_HPP

#include <cstdio>
#include <climits>
#include <cstdlib>
#include <list>

#ifndef WIN32
#include <sys/time.h>
#endif

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

#define THRESH 400

typedef struct {
  int degree;

#ifdef USING_QTHREADS
  aligned_t acc;
#else
  double acc;
#endif

  double rank;
} rank_info;

inline double getWtime()
{
  // Returns a double value indicating seconds elapsed since some arbitrary
  // point in the past.
#ifndef WIN32
  timeval theTimeVal;
  gettimeofday(&theTimeVal, 0);

  return static_cast<double>(theTimeVal.tv_sec) +
         (static_cast<double>(theTimeVal.tv_usec) / 1000000);
#else
  return(0.0);
#endif
}

namespace mtgl {

template <class T>
inline static std::list<std::list<T> > choose(T* a, int sz, int k)
{
  std::list<std::list<T> > result;

  if (sz == k)
  {
    std::list<T> l;

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
    std::list<T> l;
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

    std::list<std::list<T> > tmp_result = choose(nexta, ind, k - 1);
    typename std::list<std::list<T> >::iterator it = tmp_result.begin();
    for ( ; it != tmp_result.end(); it++)
    {
      std::list<T> next = *it;
      next.push_back(a[i]);
      result.push_back(next);
    }

    free(nexta);
  }

  return result;
}

template <typename graph>
class vn_initializer {
public:
  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;
  typedef xmt_hash_table<int64_t, double> dhash_table_t;

  vn_initializer(graph& gg, double* vnc, dhash_table_t& ew,
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

    if (my_degree > threshold) return;

    size_type dest_deg = out_degree(dest, g);

    if (dest_deg > threshold) return;

    size_type src_id = get(vid_map, src);
    size_type v_id = src_id;
    size_type dest_id = get(vid_map, dest);

    order_pair(v_id, dest_id);

    int64_t key = v_id * order + dest_id;
    double wgt = eweight[key];
    double oldwgt = mt_readfe(vn_count[src_id]);

    mt_write(vn_count[src_id], oldwgt + wgt);
    //printf("vn_count[%d]: init incr by %f due to (%d,%d)\n",
    //       src_id, wgt, v_id, dest_id);
    //printf("vn_count[%d]: now: %f\n", src_id,(double)vn_count[src_id]);
  }

  void post_visit() {}

private:
  graph& g;
  int threshold;
  unsigned int order;
  double* vn_count;
  dhash_table_t& eweight;
  vertex_id_map<graph> vid_map;
};

template <typename graph>
class hash_real_edges {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;
  typedef xmt_hash_table<int64_t, int64_t> hash_table_t;
  typedef xmt_hash_table<int64_t, double> dhash_table_t;

  hash_real_edges(graph& gg, vertex_id_map<graph>& vm, hash_table_t& tpc,
                  hash_table_t& eids, double *ons, double *gons,
                  dhash_table_t& el, dhash_table_t& er,
                  double *ew, rank_info* rin, int thresh) :
    g(gg), vid_map(vm), real_edge_tp_count(tpc), rinfo(rin),
    real_edge_ids(eids), threshold(thresh), order(num_vertices(gg)),
    en_count(ons), good_en_count(gons), e_left(el), e_right(er), e_weight(ew)
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

    if ((rinfo[v1id].degree <= threshold) &&
        (rinfo[v2id].degree <= threshold))
    {
      order_pair(v1id, v2id);

      int htsz = real_edge_tp_count.size();
      int64_t key = v1id * order + v2id;
      double wgt = e_weight[eid];

      real_edge_tp_count.insert(key, 0);
      real_edge_ids.insert(key, eid);
      e_left.insert(key, 0);
      e_right.insert(key, 0);
      en_count[eid] = 0;
      mt_incr(good_en_count[eid], wgt);
      //printf("init good_en_count[%lu (%lu,%lu)]: %f\n", eid, v1id, v2id, wgt);
    }
    else
    {
      printf("OOOPS! (%d,%d) isn't good to itself\n", v1id, v2id);
    }
  }

private:
  graph& g;
  vertex_id_map<graph>& vid_map;
  edge_id_map<graph> eid_map;
  rank_info* rinfo;
  hash_table_t& real_edge_tp_count;
  hash_table_t& real_edge_ids;
  double * en_count;
  double * good_en_count;
  dhash_table_t& e_left;
  dhash_table_t& e_right;
  double * e_weight;
  int threshold;
  uint64_t order;
};

template <typename graph>
class encount_initializer {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;
  typedef xmt_hash_table<int64_t, int64_t> hash_table_t;
  typedef xmt_hash_table<int64_t, double> dhash_table_t;

  encount_initializer(graph& gg, vertex_id_map<graph>& vm,
                      double* vnc, dhash_table_t& enc, dhash_table_t& ew) :
    g(gg), vid_map(vm), vn_count(vnc), en_count(enc),
    e_weight(ew), order(num_vertices(gg)) {}

  void operator()(edge_t e)
  {
    vertex_t v1 = source(e, g);
    vertex_t v2 = target(e, g);
    int v1id = get(vid_map, v1);
    int v2id = get(vid_map, v2);
    double vn1 = vn_count[v1id];
    double vn2 = vn_count[v2id];

    order_pair(v1id, v2id);

    int64_t key = v1id * order + v2id;
    double incr = ((vn1 + vn2) - e_weight[key]);
    en_count[key] += incr;
  }

private:
  graph& g;
  unsigned int order;
  vertex_id_map<graph>& vid_map;
  double* vn_count;
  dhash_table_t& en_count;
  dhash_table_t& e_weight;
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
    printf("Table[%jd]: %lf\n", (intmax_t) key, value);
  }
};

template <typename graph>
class fake_edge_finder {
public:
  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::edge_iterator edge_iterator;
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;
  typedef xmt_hash_table<int64_t, int64_t> hash_table_t;
  typedef xmt_hash_table<int64_t, double> dhash_table_t;

  fake_edge_finder(graph& gg, hash_table_t& real, hash_table_t& fake,
                   dhash_table_t& el, dhash_table_t& er, int thresh = THRESH) :
    g(gg), threshold(thresh), next(0), real_edges(real),
    fake_edges(fake), e_left(el), e_right(er), eid_map(get(_edge_id_map,gg)) {}

  bool visit_test(vertex_t src)
  {
    my_degree = out_degree(src, g);
    return (my_degree <= threshold);
  }

  void pre_visit(vertex_t src)
  {
    my_edges = (size_type*) malloc(sizeof(size_type) * my_degree);
  }

  void operator()(edge_t e, vertex_t src, vertex_t dest)
  {
    int my_ind = mt_incr(next, 1);
    my_edges[my_ind] = get(eid_map, e);
  }

  void post_visit(vertex_t src)
  {
    std::list<int> l;
    typedef std::list<std::list<size_type> > list_list_t;
    typedef typename std::list<size_type>::iterator l_iter_t;
    typedef typename std::list<std::list<size_type> >::iterator ll_iter_t;

    ll_iter_t it;
    vertex_id_map<graph> vid_map = get(_vertex_id_map, g);
    edge_iterator edgs = edges(g);

    int vid = get(vid_map, src);
    unsigned int order = num_vertices(g);

    std::list<std::list<size_type> > all_pairs =
      choose(my_edges, my_degree, 2);

    it = all_pairs.begin();
    for ( ; it != all_pairs.end(); it++)
    {
      std::list<size_type> l = *it;
      l_iter_t lit = l.begin();
      size_type eid1 = *lit++;
      size_type eid2 = *lit++;
      edge_t e1 = edgs[eid1];
      edge_t e2 = edgs[eid2];
      vertex_t v1 = other(e1, src, g);
      vertex_t v2 = other(e2, src, g);
      int64_t v1id = get(vid_map, v1);
      int64_t v2id = get(vid_map, v2);

      order_pair(v1id, v2id);

      int64_t key = v1id * order + v2id;

      if (!real_edges.member(key))
      {
        if (!fake_edges.member(key))
        {
          fake_edges.insert(key, 1);
          e_left.insert(key, 0);
          e_right.insert(key, 0);
        }
      }
    }

    free(my_edges);
  }

  graph& g;
  int threshold;
  int64_t my_degree;
  int next;
  size_type* my_edges;
  hash_table_t& real_edges;
  hash_table_t& fake_edges;
  dhash_table_t& e_left;
  dhash_table_t& e_right;
  edge_id_map<graph> eid_map;
};

//
//  error_accumulator_full
//
//     This was used to compute corrections when we were counting the 
//     edge 1-neighborhoods.  Ultimately, we didn't use this for the 
//     resolution limit paper.  If we revisit edge 1-neighborhoods,
//     this deprecated code will have to be updated to accommodate the 
//     change in eweight from hashtable to array.  The pairs of neighbors
//     will have to be edge id's rather than target vertices.  See 
//     fake_edge_finder above for a reference.
template <typename graph>
class error_accumulator_full {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;
  typedef xmt_hash_table<int64_t, int64_t> hash_table_t;
  typedef xmt_hash_table<int64_t, double> dhash_table_t;

  error_accumulator_full(graph& gg, hash_table_t& real, hash_table_t& fake,
                    dhash_table_t& ew,
                    dhash_table_t& el, dhash_table_t& er, int thresh = THRESH) :
    g(gg), threshold(thresh), next(0), real_edges(real), fake_edges(fake),
    eweight(ew),
    order(num_vertices(gg)), e_left(el), e_right(er)
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
    my_id = get(vid_map, src);
    my_neighbors = (vertex_t*) malloc(sizeof(vertex_t) * my_degree);
  }

  void operator()(edge_t e, vertex_t src, vertex_t dest)
  {
    int64_t v1id = get(vid_map, src);
    int64_t v2id = get(vid_map, dest);
    int my_ind = mt_incr(next, 1);
    my_neighbors[my_ind] = dest;
    order_pair(v1id, v2id);
  }

  void post_visit(vertex_t src)
  {
    typedef std::list<std::list<vertex_t> > list_list_t;
    typedef typename std::list<vertex_t>::iterator l_iter_t;
    typedef typename std::list<std::list<vertex_t> >::iterator ll_iter_t;

    ll_iter_t it;
    int vid = get(vid_map, src);
    unsigned int order = num_vertices(g);

    std::list<std::list<vertex_t> > all_pairs =
      choose(my_neighbors, my_degree, 2);

    it = all_pairs.begin();
    for ( ; it != all_pairs.end(); it++)
    {
      int vid = my_id;
      std::list<vertex_t> l = *it;
      l_iter_t lit = l.begin();
      vertex_t first = *lit++;
      vertex_t second = *lit++;
      int v1id = get(vid_map, first);
      int v2id = get(vid_map, second);

      order_pair(v1id, v2id);

      int64_t key = v1id * order + v2id;

      order_pair(vid, v1id);

      int64_t l_key = vid * order + v1id;
      double wgt = eweight[l_key];

      e_left.update<hash_mt_incr<double> >(key, wgt, 
                                           hash_mt_incr<double>());
      //e_left[key] += wgt;
      vid = my_id;

      order_pair(vid, v2id);

      int64_t r_key = vid * order + v2id;
      wgt = eweight[r_key];
      e_right.update<hash_mt_incr<double> >(key, wgt, 
                                           hash_mt_incr<double>());
      //e_right[key] += wgt;
    }

    free(my_neighbors);
  }

  graph& g;
  int threshold;
  int my_degree;
  int my_id;
  int next;
  unsigned int order;
  vertex_t* my_neighbors;
  hash_table_t& real_edges;
  hash_table_t& fake_edges;
  dhash_table_t& e_left;
  dhash_table_t& e_right;
  dhash_table_t& eweight;
  vertex_id_map<graph> vid_map;
  edge_id_map<graph> eid_map;
};

//  This version supports counting spokes only, so we don't really need the
//  edge weights.  We'll just add 1 to each fake edge for each set of tent 
//  poles.  The distinction between "left" and "right" errors is deprecated,
//  but the structures are retained to support future work on edge 
//  1-neighborhoods.
template <typename graph>
class error_accumulator {
public:
  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::edge_iterator edge_iterator;
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;
  typedef xmt_hash_table<int64_t, int64_t> hash_table_t;
  typedef xmt_hash_table<int64_t, double> dhash_table_t;

  error_accumulator(graph& gg, hash_table_t& fake, double *ew,
                    dhash_table_t& el, dhash_table_t& er, int thresh = THRESH) :
    g(gg), threshold(thresh), next(0), fake_edges(fake),
    order(num_vertices(gg)), eweight(ew), e_left(el), e_right(er)
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
    my_id = get(vid_map, src);
    my_edges = (size_type*) malloc(sizeof(size_type) * my_degree);
  }

  void operator()(edge_t e, vertex_t src, vertex_t dest)
  {
    int my_ind = mt_incr(next, 1);
    my_edges[my_ind] = get(eid_map, e);
  }

  void post_visit(vertex_t src)
  {
    typedef std::list<std::list<size_type> > list_list_t;
    typedef typename std::list<size_type>::iterator l_iter_t;
    typedef typename std::list<std::list<size_type> >::iterator ll_iter_t;

    edge_iterator edgs = edges(g);

    ll_iter_t it;
    int vid = get(vid_map, src);
    unsigned int order = num_vertices(g);

    std::list<std::list<size_type> > all_pairs =
      choose(my_edges, my_degree, 2);

    it = all_pairs.begin();
    for ( ; it != all_pairs.end(); it++)
    {
      int vid = my_id;
      std::list<size_type> l = *it;
      l_iter_t lit = l.begin();
      size_type eid1 = *lit++;
      size_type eid2 = *lit++;
      edge_t e1 = edgs[eid1];
      edge_t e2 = edgs[eid2];
      vertex_t v1 = other(e1, src, g);
      vertex_t v2 = other(e2, src, g);
      size_type v1id = get(vid_map, v1);
      size_type v2id = get(vid_map, v2);
      if (v2id < v1id) {      // the lower id is considered "left"
           size_type tmp = eid1;
           eid1 = eid2;
           eid2 = tmp;
      }

      order_pair(v1id, v2id);
      int64_t key = v1id * order + v2id;
      if (!fake_edges.member(key)) continue;


      double l_wgt = eweight[eid1];
      double r_wgt = eweight[eid2];
      double lincr = 1.0;
      double rincr = 1.0;

      e_left.update<hash_mt_incr<double> >(key, l_wgt, 
                                           hash_mt_incr<double>());
      e_right.update<hash_mt_incr<double> >(key, r_wgt, 
                                           hash_mt_incr<double>());
      double lcur, rcur;
      e_left.lookup(key, lcur);
      e_right.lookup(key, rcur);
      //e_left[key] += 1;	// simply note how many tent poles
      //e_right[key] += 1;
    }

    free(my_edges);
  }

  graph& g;
  int threshold;
  int next;
  hash_table_t& fake_edges;
  unsigned int order;
  double *eweight;
  dhash_table_t& e_left;
  dhash_table_t& e_right;
  vertex_id_map<graph> vid_map;
  edge_id_map<graph> eid_map;
  size_type* my_edges;
  int my_degree;
  int my_id;
};

template <typename graph>
class ncount_correcter {
public:
  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_iterator edge_iterator;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;
  typedef xmt_hash_table<int64_t, int64_t> hash_table_t;
  typedef xmt_hash_table<int64_t, double> dhash_table_t;

  ncount_correcter(graph& gg, hash_table_t& real, 
                   double * enc, double * genc, double * ew,
                   dhash_table_t& el, dhash_table_t& er, int thresh = THRESH) :
    g(gg), threshold(thresh), next(0), real_edges(real),
    en_count(enc), good_en_count(genc), eweight(ew),
    order(num_vertices(gg)), e_left(el), e_right(er)
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
    my_id = get(vid_map, src);
    my_edges = (size_type*) malloc(sizeof(size_type) * my_degree);
  }

  void operator()(edge_t e, vertex_t src, vertex_t dest)
  {
    int my_ind = mt_incr(next, 1);
    my_edges[my_ind] = get(eid_map, e);
  }

  void post_visit(vertex_t src)
  {
    typedef std::list<std::list<size_type> > list_list_t;
    typedef typename std::list<size_type>::iterator l_iter_t;
    typedef typename std::list<std::list<size_type> >::iterator ll_iter_t;

    edge_iterator edgs = edges(g);

    ll_iter_t it;
    int vid = get(vid_map, src);
    unsigned int order = num_vertices(g);

    std::list<std::list<size_type> > all_pairs =
      choose(my_edges, my_degree, 2);

    it = all_pairs.begin();
    for ( ; it != all_pairs.end(); it++)
    {
      int vid = my_id;
      std::list<size_type> l = *it;
      l_iter_t lit = l.begin();
      size_type eid1 = *lit++;
      size_type eid2 = *lit++;
      edge_t e1 = edgs[eid1];
      edge_t e2 = edgs[eid2];
      vertex_t v1 = other(e1, src, g);
      vertex_t v2 = other(e2, src, g);
      int v1id = get(vid_map, v1);
      int v2id = get(vid_map, v2);
      int v1id_bak = v1id;
      int v2id_bak = v2id;

      if (v2id < v1id) { // the lower id is considered "left"
           size_type tmp = eid1;
           eid1 = eid2;
           eid2 = tmp;
      }

      order_pair(v1id, v2id);

      int64_t key = v1id * order + v2id;
      if (real_edges.member(key)) continue;

      double e1_wgt = eweight[eid1];
      double e2_wgt = eweight[eid2];
      double er;
      double el;
      e_right.lookup(key, er);
      e_left.lookup(key, el);

      if (real_edges.member(key))
      {
        //printf("%d -> %jd(%d,%d): add %lf to %jd, add %lf to %jd\n",
        //       my_id, key, v1id_bak, v2id_bak,
        //       - 0.25*(er-r_wgt), l_key,
        //       - 0.25*(el-l_wgt), r_key);
        //en_count[l_key] += (- 0.25*(er-r_wgt));
        //en_count[r_key] += (- 0.25*(el-l_wgt));
        //good_en_count[l_key] += (0.25*(er-r_wgt));
        //good_en_count[r_key] += (0.25*(el-l_wgt));
      }
      else
      {
        //printf("%d -> %jd(%d,%d): add %lf to %jd, add %lf to %jd\n",
        //       my_id, key, v1id_bak, v2id_bak,
        //       + 0.25*(er-r_wgt), l_key,
        //       + 0.25*(el-l_wgt), r_key);
        //en_count[l_key] += (0.25*(er-r_wgt));
        //en_count[r_key] += (0.25*(el-l_wgt));
        //good_en_count[l_key] += (0.25*(er-r_wgt));
        //good_en_count[r_key] += (0.25*(el-l_wgt));
        if (el > e1_wgt || er > e2_wgt)                           // >1 tent top
        {       //printf("%d -> %jd(%d,%d): add %lf to %jd, add %lf to %jd\n",
                 //      my_id, key, v1id_bak, v2id_bak,
                  //     + r_wgt, l_key,
                   //    + l_wgt, r_key);
          // SPOKES
          good_en_count[eid1] += e2_wgt;
          good_en_count[eid2] += e1_wgt;
	  //printf("SPOKE*: good_en_count[(%lu,%lu)] (for (%lu,%lu): %f\n", vid, v1id, vid, v2id, good_en_count[eid1]);
	  //printf("SPOKE*: good_en_count[(%lu,%lu)] (for (%lu,%lu): %f\n", vid, v2id, vid, v1id, good_en_count[eid2]);
        }
      }
    }

    free(my_edges);
  }

  graph& g;
  int threshold;
  int my_degree;
  int my_id;
  int next;
  unsigned int order;
  size_type* my_edges;
  hash_table_t& real_edges;
  dhash_table_t& e_left;
  dhash_table_t& e_right;
  double * en_count;
  double * good_en_count;
  double * eweight;
  vertex_id_map<graph> vid_map;
  edge_id_map<graph> eid_map;
};

template <typename graph>
class goodness_correcter {
public:
  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef xmt_hash_table<int64_t, double> dhash_table_t;

  goodness_correcter(graph& gg, double* vnc, dhash_table_t& enc,
                     dhash_table_t& genc, dhash_table_t& ew,
                     int thresh = THRESH) :
    g(gg), threshold(thresh), en_count(enc), vn_count(vnc),
    good_en_count(genc), eweight(ew), order(num_vertices(gg))
  {
    vid_map = get(_vertex_id_map, g);
  }

  void operator()(vertex_t src, vertex_t dest)
  {
    size_type my_degree = out_degree(src, g);

    if (my_degree > threshold) return;

    size_type dest_deg = out_degree(dest, g);

    if (dest_deg > threshold) return;

    size_type src_id = get(vid_map, src);
    size_type dest_id = get(vid_map, dest);

    if (src_id > dest_id) return;

    int64_t key = src_id * order + dest_id;
    double my_weight = eweight[key];
    double num_bad = vn_count[src_id] + vn_count[dest_id]
                     - my_weight - good_en_count[key];

    good_en_count[key] = en_count[key] - num_bad;
  }

  void post_visit() {}

private:
  graph& g;
  int threshold;
  size_type order;
  double* vn_count;
  dhash_table_t& en_count;
  dhash_table_t& good_en_count;
  dhash_table_t& eweight;
  vertex_id_map<graph> vid_map;
};

template <class graph>
class v_one_neighborhood_count : public default_triangles_visitor<graph> {
public:
  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge;
  typedef xmt_hash_table<int64_t, int64_t> hash_table_t;
  typedef xmt_hash_table<int64_t, double> dhash_table_t;

  v_one_neighborhood_count(graph& gg, double* ct, vertex_id_map<graph>& vm,
                           double * ew,
                           double * enc, double * genc) :
    g(gg), count(ct), vipm(vm), eweight(ew), order(num_vertices(gg)),
    en_count(enc), good_en_count(genc) {}

/*
  void operator()(vertex_t v1, vertex_t v2, vertex_t v3)
  {
    double wgt;
    size_type v1id = get(vipm, v1);
    size_type v2id = get(vipm, v2);
    size_type v3id = get(vipm, v3);

    this->operator()(v1id, v2id, v3id);  // can't have this when size_type
  }					 // is equal to vertex_descriptor
*/
  void operator()(size_type e1id, size_type e2id, size_type e3id)
  {
    double wgt;
    
    mt_incr(good_en_count[e1id], eweight[e2id]+eweight[e3id]);
    mt_incr(good_en_count[e2id], eweight[e1id]+eweight[e3id]);
    mt_incr(good_en_count[e3id], eweight[e1id]+eweight[e2id]);
  }

/*
  void operator()(size_type v1id, size_type v2id, size_type v3id)
  {
    double wgt;
    size_type o_v1 = v1id, o_v2 = v2id, o_v3 = v3id;

    order_pair(o_v1, o_v2);

    size_type v1_v2_key = o_v1 * order + o_v2;
    o_v2 = v2id; o_v3 = v3id;

    order_pair(o_v2, o_v3);

    size_type v2_v3_key = o_v2 * order + o_v3;
    o_v1 = v1id; o_v3 = v3id;

    order_pair(o_v1, o_v3);

    size_type v1_v3_key = o_v1 * order + o_v3;
    double w_v1_v2 = eweight[v1_v2_key];
    double w_v2_v3 = eweight[v2_v3_key];
    double w_v1_v3 = eweight[v1_v3_key];

    // JWB: the following lines go into correct encount computation
    // ************  commented out for reslimit paper
    //wgt = mt_readfe(count[v1id]);
    //mt_write(count[v1id], wgt + w_v2_v3);
    //wgt = mt_readfe(count[v2id]);
    //mt_write(count[v2id], wgt + w_v1_v3);
    //wgt = mt_readfe(count[v3id]);
    //mt_write(count[v3id], wgt + w_v1_v2);
    // *************
    //printf("tri(%d,%d,%d): add %f to vc(%jd)\n",
    //       v1id,v2id,v3id, w_v2_v3, v1id);
    //printf("tri(%d,%d,%d): add %f to vc(%jd)\n",
    //       v1id,v2id,v3id, w_v1_v3, v2id);
    //printf("tri(%d,%d,%d): add %f to vc(%jd)\n",
    //       v1id,v2id,v3id, w_v1_v2, v3id);

    double v1_v2_incr = (w_v1_v3 + w_v2_v3);
    double v2_v3_incr = (w_v1_v2 + w_v1_v3);
    double v1_v3_incr = (w_v1_v2 + w_v2_v3);

    // JWB: the following lines go into correct encount computation
    // ************  commented out for reslimit paper
    //en_count.int_fetch_add(v1_v3_key, -v1_v3_incr);
    //en_count.int_fetch_add(v2_v3_key, -v2_v3_incr);
    //en_count.int_fetch_add(v1_v2_key, -v1_v2_incr);
    // ************
    //printf("tri(%d,%d,%d): add %f to enc(%jd)\n",
    //       v1id,v2id,v3id, -v1_v3_incr, v1_v3_key);
    //printf("tri(%d,%d,%d): add %f to enc(%jd)\n",
    //       v1id,v2id,v3id, -v2_v3_incr, v2_v3_key);
    //printf("tri(%d,%d,%d): add %f to enc(%jd)\n",
    //       v1id,v2id,v3id, -v1_v2_incr, v1_v2_key);

    good_en_count.int_fetch_add(v1_v2_key, v1_v2_incr);
    good_en_count.int_fetch_add(v1_v3_key, v1_v3_incr);
    good_en_count.int_fetch_add(v2_v3_key, v2_v3_incr);
    //printf("tri(%d,%d,%d): add %f to genc(%jd)\n",
    //       v1id,v2id,v3id, v1_v3_incr, v1_v3_key);
    //printf("tri(%d,%d,%d): add %f to genc(%jd)\n",
    //       v1id,v2id,v3id, v2_v3_incr, v2_v3_key);
    //printf("tri(%d,%d,%d): add %f to genc(%jd)\n",
    //       v1id,v2id,v3id, v1_v2_incr, v1_v2_key);
  }
*/

private:
  double* count;
  unsigned int order;
  graph& g;
  vertex_id_map<graph>& vipm;
  double * eweight;
  double * en_count;
  double * good_en_count;
};

/*! ********************************************************************
    \class neighbor_counts
    \brief
 *  ********************************************************************
*/
template <typename graph>
class neighbor_counts {
public:
  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;
  typedef typename graph_traits<graph>::adjacency_iterator adj_vertex_iter_t;
  typedef typename graph_traits<graph>::vertex_iterator vertex_iter_t;
  typedef typename graph_traits<graph>::edge_iterator edge_iterator;
  typedef typename graph_traits<graph>::out_edge_iterator out_edge_iterator;
  typedef graph Graph;
  typedef xmt_hash_table<int64_t, int64_t> hash_table_t;
  typedef xmt_hash_table<int64_t, double> dhash_table_t;

  neighbor_counts(graph& gg, double* ew, int thr = THRESH) :
    g(gg), eweight(ew), thresh(thr) {}

  void run(double* vn_count, double * en_count,
           double * good_en_count)
  {
    mt_timer mttimer;
    mttimer.start();

    vertex_id_map<graph> vid_map = get(_vertex_id_map, g);
    edge_id_map<graph> eid_map   = get(_edge_id_map, g);

    unsigned int order = num_vertices(g);
    int size = num_edges(g);
    hashvis h;
    dhashvis dh;
    if (order < 50) print(g);

    rank_info* rinfo = new rank_info[order];

    vertex_iter_t verts = vertices(g);

    #pragma mta assert nodep
    for (unsigned int i = 0; i < order; i++)
    {
      rinfo[i].rank = 1.0 / order;
      rinfo[i].degree = out_degree(verts[i], g);
    }

//    dhash_table_t the_keys((int) (1.5 * num_edges(g)));

    memset(vn_count, 0, order * sizeof(double));

    double startTime = getWtime();

    #pragma mta assert nodep
    for (unsigned int i = 0; i < order; i++)
    {
      vertex_t v = verts[i];
      int vid = get(vid_map, v);

      int deg = out_degree(v, g);
      out_edge_iterator eiter = out_edges(v, g);

      #pragma mta assert nodep
      for (int j = 0; j < deg; j++)
      {
        edge_t e= eiter[j];
	size_type eid = get(eid_map, e);
        mt_incr(vn_count[i], eweight[eid]);
      }
    }

    double endTime = getWtime();

    printf("Time: %f.\n", endTime - startTime);

    hash_table_t real_edges(int64_t(1.5 * size));
    hash_table_t fake_edges(int64_t(35 * size));      // need estimat. num
    hash_table_t real_edge_ids(int64_t(1.5 * size));
    dhash_table_t e_left(int64_t(thresh * size));     // need to est. num
    dhash_table_t e_right(int64_t(thresh * size));    // fake edges

    hash_real_edges<Graph> hre(g, vid_map, real_edges, real_edge_ids,
                               en_count, good_en_count, e_left, e_right,
                               eweight, rinfo, thresh);

    visit_edges(g, hre);

    v_one_neighborhood_count<Graph>
    onec(g, vn_count, vid_map, eweight, en_count,
         good_en_count);

    printf("finding triangles...\n");

    find_triangles<Graph, v_one_neighborhood_count<Graph> >ft(g, onec);
    ft.run();


    //encount_initializer<Graph> nci(g, vid_map, vn_count, en_count, eweight);
    //visit_edges(g, nci);

    edge_iterator edgs = edges(g);
    size_type m = num_edges(g);
    #pragma mta assert nodep
    for (size_type i=0; i<m; i++) {
	edge_t e = edgs[i];
	size_type eid = get(eid_map, e);  // should use property map
	vertex_t v1 = source(e, g);
	vertex_t v2 = target(e, g);
	size_type v1id = get(vid_map, v1);
	size_type v2id = get(vid_map, v2);
    	double vn1 = vn_count[v1id];
    	double vn2 = vn_count[v2id];
	double incr = ((vn1 + vn2) - eweight[eid]);
	mt_incr(en_count[eid], incr);
     order_pair(v1id,v2id);
     cout << "encount init(" << v1id << "," << v2id << ")" << en_count[eid]
          << endl;
    }

    fake_edge_finder<Graph> tpc(g, real_edges, fake_edges, e_left,
                                e_right, thresh);
    visit_adj(g, tpc);

    printf("real edges: %d\n", real_edges.size());
    printf("fake edges: %d\n", fake_edges.size());

    error_accumulator<Graph> ea(g, fake_edges, eweight, e_left, e_right, thresh);
    visit_adj(g, ea);

    ncount_correcter<Graph> ncc(g, real_edges, en_count,
                                good_en_count, eweight, e_left,
                                e_right, thresh);
    visit_adj(g, ncc);

    //size_type *prefix_counts=0, *started_nodes=0, num_threads;
    //memset(vn_count, 0, sizeof(double)*order);
    //vn_initializer<graph> vni(g, vn_count, eweight,thresh);
    //visit_adj(g, vni, prefix_counts, started_nodes, num_threads);
    //goodness_correcter<Graph> gc(g, vn_count, en_count,
    //                             good_en_count, eweight, thresh);
    //visit_adj(g, gc, prefix_counts, started_nodes, num_threads);
    //printf("called goodness_corr\n"); fflush(stdout);
  }

private:
  graph& g;
  double * eweight;
  int thresh;
};

template <typename graph>
class weight_by_neighborhoods {
public:
  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;
  typedef typename graph_traits<graph>::edge_iterator edge_iter_t;
  typedef graph Graph;
  typedef xmt_hash_table<int64_t, int64_t> hash_table_t;
  typedef xmt_hash_table<int64_t, double> dhash_table_t;

  weight_by_neighborhoods(graph& gg, double* ew, int thr = THRESH) :
    g(gg), eweight(ew), thresh(thr) {}

  void run(int num_iter)
  {
    vertex_id_map<Graph> vid_map = get(_vertex_id_map, g);
    edge_id_map<Graph> eid_map = get(_edge_id_map, g);
    unsigned int order = num_vertices(g);
    int size = num_edges(g);

    edge_iter_t edgs = edges(g);

    for (int iter = 0; iter < num_iter; iter++)
    {
      double* vn_count = (double*) calloc(order, sizeof(double));
      double * en_count= (double*) calloc(size, sizeof(double));
      double * good_en_count= (double*) calloc(size, sizeof(double));

      neighbor_counts<Graph> nc(g, eweight, thresh);
      nc.run(vn_count, en_count, good_en_count);

      for (int i = 0; i < size; i++)
      {
        edge_t e = edgs[i];
        vertex_t v1 = source(e, g);
        vertex_t v2 = target(e, g);
	size_type eid = get(eid_map, e);
        eweight[eid] = good_en_count[eid]/en_count[eid];
        printf("eweight[%lu (%lu,%lu)]: (%f/%f) = %f\n", eid, 
			get(vid_map, v1), get(vid_map, v2), good_en_count[eid],
			en_count[eid], good_en_count[eid]/en_count[eid]);
      }            // reset weights

      free(vn_count);
    }              // iter
  }

private:
  Graph& g;
  double* eweight;
  int thresh;
};

}

#endif

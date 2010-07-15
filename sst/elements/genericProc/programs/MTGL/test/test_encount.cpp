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
/*! \file test_encount.cpp

    \author Jon Berry (jberry@sandia.gov)

    \date 6/20/2008
*/
/****************************************************************************/

#include <cstdlib>
#include <list>

#include <mtgl/static_graph_adapter.hpp>
#include <mtgl/generate_rmat_graph.hpp>
#include <mtgl/read_dimacs.hpp>
#include <mtgl/breadth_first_search_mtgl.hpp>
#include <mtgl/topsort.hpp>
#include <mtgl/visit_adj.hpp>
#include <mtgl/triangles.hpp>
#include <mtgl/metrics.hpp>
#include <mtgl/read_matrix_market.hpp>
#include <mtgl/util.hpp>
#include <mtgl/xmt_hash_table.hpp>

using namespace std;
using namespace mtgl;

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
static void order_pair(T& a, T& b)
{
  if (a > b)
  {
    T tmp = a;
    a = b;
    b = tmp;
  }
}

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
      if (j != i && a[j] > a[i])  nexta[ind++] = a[j];
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
  typedef typename graph_traits<graph>::vertex_descriptor vertex_descriptor;
  typedef typename graph_traits<graph>::edge_descriptor edge_descriptor;
  typedef xmt_hash_table<int64_t, int64_t> hash_table_t;

  hash_real_edges(graph& gg, vertex_id_map<graph>& vm, hash_table_t& tpc,
                  hash_table_t& eids, hash_table_t& td, hash_table_t& ons,
                  int* vons, rank_info* rin, int thresh) :
    g(gg), vid_map(vm), real_edge_tp_count(tpc), rinfo(rin),
    real_edge_ids(eids), threshold(thresh), order(num_vertices(gg)),
    tent_degree(td), en_count(ons), vn_count(vons)
  {
    eid_map = get(_edge_id_map, g);
  }

  void operator()(edge_descriptor e)
  {
    vertex_descriptor v1 = source(e, g);
    vertex_descriptor v2 = target(e, g);
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
      // edge one-neighborhood size.  Start with n(v1)+n(v2)
      // we'll apply corrections in subsequent passes
      int ons = vn_count[v1id] + vn_count[v2id] - 1;
      en_count.insert(key, 4 * ons);
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
  int* vn_count;
  int threshold;
  int64_t order;
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
  typedef typename graph_traits<graph>::vertex_descriptor vertex_descriptor;
  typedef typename graph_traits<graph>::edge_descriptor edge_descriptor;
  typedef xmt_hash_table<int64_t, int64_t> hash_table_t;

  tent_counter(graph& gg, hash_table_t& real, hash_table_t& fake,
               hash_table_t& td, int thresh = 4) :
    g(gg), threshold(thresh), next(0), real_edges(real), fake_edges(fake),
    tent_degree(td) {}

  bool visit_test(vertex_descriptor src)
  {
    my_degree = out_degree(src, g);
    return (my_degree <= threshold);
  }

  void pre_visit(vertex_descriptor src)
  {
    my_neighbors =
      (vertex_descriptor*) malloc(sizeof(vertex_descriptor) * my_degree);
  }

  void operator()(edge_descriptor e, vertex_descriptor src,
                  vertex_descriptor dest)
  {
    int my_ind = mt_incr(next, 1);
    my_neighbors[my_ind] = dest;
  }

  void post_visit(vertex_descriptor src)
  {
    typedef list<list<vertex_descriptor> > list_list_t;
    typedef typename list<vertex_descriptor>::iterator l_iter_t;
    typedef typename list<list<vertex_descriptor> >::iterator ll_iter_t;

    ll_iter_t it;
    vertex_id_map<graph> vid_map = get(_vertex_id_map, g);
    int vid = get(vid_map, src);
    int order = num_vertices(g);
    list<list<vertex_descriptor> > all_pairs = choose(my_neighbors,
                                                      my_degree, 2);
    it = all_pairs.begin();

    for ( ; it != all_pairs.end(); it++)
    {
      list<vertex_descriptor> l = *it;
      l_iter_t lit = l.begin();
      vertex_descriptor first = *lit++;
      vertex_descriptor second = *lit++;
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
            mt_incr(fake_edges[key](), (int64_t) 1);
          }
        }
        else
        {
          mt_incr(real_edges[key](), (int64_t) 1);
          mt_incr(tent_degree[key](), -my_degree);
        }
      }
    }

    free(my_neighbors);
  }

  graph& g;
  int threshold;
  int64_t my_degree;
  int next;
  vertex_descriptor* my_neighbors;
  hash_table_t& real_edges;
  hash_table_t& fake_edges;
  hash_table_t& tent_degree;
};

template <typename graph>
class ncount_corrector {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_descriptor;
  typedef typename graph_traits<graph>::edge_descriptor edge_descriptor;
  typedef xmt_hash_table<int64_t, int64_t> hash_table_t;

  ncount_corrector(graph& gg, hash_table_t& real, hash_table_t& fake,
                   hash_table_t& enc, int thresh = 4) :
    g(gg), threshold(thresh), next(0), real_edges(real), fake_edges(fake),
    en_count(enc), order(num_vertices(gg))
  {
    eid_map = get(_edge_id_map, g);
    vid_map = get(_vertex_id_map, g);
  }

  bool visit_test(vertex_descriptor src)
  {
    my_degree = out_degree(src, g);
    return (my_degree <= threshold);
  }

  void pre_visit(vertex_descriptor src)
  {
    my_neighbors =
      (vertex_descriptor*) malloc(sizeof(vertex_descriptor) * my_degree);
    my_ekeys = (int64_t*) malloc(sizeof(int64_t) * my_degree);
  }

  void operator()(edge_descriptor e, vertex_descriptor src,
                  vertex_descriptor dest)
  {
    int64_t v1id = get(vid_map, src);
    int64_t v2id = get(vid_map, dest);
    int my_ind = mt_incr(next, 1);
    my_neighbors[my_ind] = dest;
    order_pair(v1id, v2id);
    my_ekeys[my_ind] = v1id * order + v2id;
  }

  void post_visit(vertex_descriptor src)
  {
    typedef list<list<vertex_descriptor> > list_list_t;
    typedef typename list<vertex_descriptor>::iterator l_iter_t;
    typedef typename list<list<vertex_descriptor> >::iterator ll_iter_t;
    ll_iter_t it;
    int vid = get(vid_map, src);
    int order = num_vertices(g);
    list<list<vertex_descriptor> > all_pairs = choose(my_neighbors,
                                                      my_degree, 2);
    it =  all_pairs.begin();

    for ( ; it != all_pairs.end(); it++)
    {
      list<vertex_descriptor> l = *it;
      l_iter_t lit = l.begin();
      vertex_descriptor first = *lit++;
      vertex_descriptor second = *lit++;
      int64_t v1id = get(vid_map, first);
      int64_t v2id = get(vid_map, second);

      if ((out_degree(first, g)  <= threshold) &&
          (out_degree(second, g) <= threshold))
      {
        order_pair(v1id, v2id);
        int64_t key = v1id * order + v2id;

        if (real_edges.member(key))
        {
          // triangle edge - decrement by two
          // tent poles
          mt_incr(en_count[key](), (int64_t) (-4 * 2));
          int k = real_edges[key]();
          int64_t incr = k - 1;

          for (int i = 0; i < my_degree; i++)
          {
            if((my_neighbors[i] == first) || (my_neighbors[i] == second))
            {
              mt_incr(en_count[my_ekeys[i]](), -incr);
            }
          }
        }
        else
        {
          int k = fake_edges[key]();
          int64_t incr = k - 1;

          for (int i = 0; i < my_degree; i++)
          {
            if((my_neighbors[i] == first) || (my_neighbors[i] == second))
            {
              mt_incr(en_count[my_ekeys[i]](), incr);
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
  int order;
  vertex_descriptor* my_neighbors;
  int64_t* my_ekeys;
  hash_table_t& real_edges;
  hash_table_t& fake_edges;
  hash_table_t& en_count;
  vertex_id_map<graph> vid_map;
  edge_id_map<graph> eid_map;
};

template <class graph>
class v_one_neighborhood_count : public default_triangles_visitor<graph> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_descriptor;

  v_one_neighborhood_count(graph& gg, int* ct, vertex_id_map<graph>& vm) :
    g(gg), count(ct), vipm(vm) {}

  void operator()(vertex_descriptor v1, vertex_descriptor v2,
                  vertex_descriptor v3)
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

template <class graph>
class e_rs_vis {
//  For each vertex, suppose it is a community leader, and assume
//  that it serves all of its neighbors.  Count the number of inter-
//  community edges connecting its community to the rest of the world

public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_descriptor;
  typedef typename graph_traits<graph>::edge_descriptor edge_descriptor;

  e_rs_vis(graph& gg, vertex_id_map<graph>& vm, int* e_rs_nm) :
    g(gg), vid_map(vm), e_rs_num(e_rs_nm) {}

  bool visit_test(vertex_descriptor v) { return true; }

  void pre_visit(vertex_descriptor v)
  {
    int vid = get(vid_map, v);
    vdeg = out_degree(v, g);
    neigh_deg_count = vdeg;
  }

  void operator()(edge_descriptor e, vertex_descriptor src,
                  vertex_descriptor dest)
  {
    int sid = get(vid_map, src);
    int tid = get(vid_map, dest);

    if (sid == tid) return;

    int dest_deg = out_degree(dest, g);
    int next = mt_incr(neigh_deg_count, dest_deg);
  }

  void post_visit(vertex_descriptor v)
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
  typedef typename graph_traits<graph>::vertex_descriptor vertex_descriptor;
  typedef typename graph_traits<graph>::edge_descriptor edge_descriptor;
  typedef xmt_hash_table<int64_t, int64_t> hash_table_t;

  e_rs_e_vis(graph& gg, vertex_id_map<graph>& vm, hash_table_t& ers,
             int* e_rs_v) :
    g(gg), vid_map(vm), e_rs_e_num(ers), e_rs_v_num(e_rs_v),
    order(num_vertices(g)) {}

  void operator()(edge_descriptor e)
  {
    vertex_descriptor src = source(e, g);
    vertex_descriptor dest = target(e, g);
    int sid = get(vid_map, src);
    int tid = get(vid_map, dest);

    if (sid == tid) return;

    order_pair(sid, tid);
    int64_t key = sid * order + tid;
    int64_t incr = e_rs_v_num[sid];
    mt_incr(e_rs_e_num[key](), incr + e_rs_v_num[tid]);
  }

  void post_visit(vertex_descriptor v) {}

private:
  graph& g;
  vertex_id_map<graph>& vid_map;
  hash_table_t& e_rs_e_num;
  int* e_rs_v_num;
  int order;
};

template <class graph>
class Q_e_vis {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_descriptor;
  typedef typename graph_traits<graph>::edge_descriptor edge_descriptor;
  typedef xmt_hash_table<int64_t, int64_t> hash_table_t;
  typedef xmt_hash_table<int64_t, double> dhash_table_t;

  Q_e_vis(graph& gg, vertex_id_map<graph>& vm, hash_table_t& ons,
          int* vnc, hash_table_t& ers, int* ersv, dhash_table_t& q) :
    g(gg), vid_map(vm), e_rs_e_num(ers), e_rs_v(ersv), en_count(ons), Q(q),
    order(num_vertices(g)), vn_count(vnc), tot_deg(2 * num_edges(g)) {}

  void operator()(edge_descriptor e)
  {
    vertex_descriptor src = source(e, g);
    vertex_descriptor dest = target(e, g);
    int sid = get(vid_map, src);
    int tid = get(vid_map, dest);

    if (sid == tid) return;

    order_pair(sid, tid);
    int64_t key = sid * order + tid;
    double neigh_sz = en_count[key]() / 4.0;
    int64_t e_rs     = e_rs_e_num[key]();
    double a_s      = e_rs / tot_deg;
    double q = (2 * neigh_sz / tot_deg) - (a_s * a_s);

    printf("(%d,%d): en: %.1lf, vn(%d): %d, vn(%d): %d, ers: %jd, ers(%d): %d"
           " ers(%d): %d\n", sid, tid, neigh_sz, sid, vn_count[sid], tid,
           vn_count[tid], e_rs, sid, e_rs_v[sid], tid, e_rs_v[tid]);
  }

private:
  graph& g;
  vertex_id_map<graph>& vid_map;
  hash_table_t& e_rs_e_num;
  int* e_rs_v;
  int* vn_count;
  hash_table_t& en_count;
  dhash_table_t& Q;
  double tot_deg;
  int order;
};

int main(int argc, char* argv[])
{
  typedef xmt_hash_table<int64_t, int64_t> hash_table_t;
  typedef xmt_hash_table<int64_t, double> dhash_table_t;

  typedef static_graph_adapter<undirectedS> Graph;
  typedef graph_traits<Graph>::size_type size_type;
  typedef graph_traits<Graph>::vertex_iterator vertex_iterator;

  if (argc < 2)
  {
    fprintf(stderr, "Usage: %s {<p>|<filename>} \n", argv[0]);
    fprintf(stderr, "where specifying p requests a "
            "generated r-mat graph "
            "with 2^p vertices,\n");
    fprintf(stderr, "and specifying filename requests that a "
            "DIMACS or MatrixMarket "
            "file be read to build the graph;\n");
    fprintf(stderr, "DIMACS files must end with suffix .dimacs\n");
    fprintf(stderr, "MatrixMarket files must end with "
            "suffix .mtx\n");
    exit(1);
  }

  // Determine which input format is used:  automatically generated rmat
  // or file-based input.
  int use_rmat = 1;
  int llen = strlen(argv[1]);
  for (int i = 0; i < llen; i++)
  {
    if (!(argv[1][i] >= '0' && argv[1][i] <= '9'))
    {
      // String contains non-numeric characters;
      // it must be a filename.
      use_rmat = 0;
      break;
    }
  }

  Graph g;

  if (0)
  {
    int num_verts = 4;
    int num_edges = 6;

    int* srcs = (int*) malloc(sizeof(int) * num_edges);
    int* dests = (int*) malloc(sizeof(int) * num_edges);

    srcs[0] = 0; dests[0] = 1;
    srcs[1] = 1; dests[1] = 2;
    srcs[2] = 2; dests[2] = 3;
    srcs[3] = 3; dests[3] = 0;
    srcs[4] = 1; dests[4] = 3;
    srcs[5] = 0; dests[5] = 2;

    init(num_verts, num_edges, srcs, dests, g);
  }
  else
  {
    if (use_rmat)
    {
      generate_rmat_graph(g, atoi(argv[1]), 2);
    }
    else if (argv[1][llen - 1] == 'x')
    {
      // Matrix-market input.
      dynamic_array<int> weights;

      read_matrix_market(g, argv[1], weights);
    }
    else if (argv[1][llen - 1] == 's')
    {
      // DIMACS input.
      dynamic_array<int> weights;

      read_dimacs(g, argv[1], weights);
    }
    else
    {
      fprintf(stderr, "Invalid filename %s\n", argv[1]);
    }
  }

  int order = num_vertices(g);
  int size = num_edges(g);
  if (order < 50) print(g);
  vertex_id_map<Graph> vid_map = get(_vertex_id_map, g);

  rank_info* rinfo = new rank_info[order];
  vertex_iterator verts = vertices(g);
  #pragma mta assert nodep
  for (int i = 0; i < order; i++)
  {
    rinfo[i].rank = 1.0 / order;
    rinfo[i].degree = out_degree(verts[i], g);
  }

  int* vn_count = (int*) malloc(sizeof(int) * order);
  memset(vn_count, 0, order);
  for (int i = 0; i < order; i++)
  {
    vn_count[i] = out_degree(verts[i], g);
  }

  v_one_neighborhood_count<Graph> onec(g, vn_count, vid_map);
  find_triangles<Graph, v_one_neighborhood_count<Graph> > ft(g, onec);
  ft.run();

  hash_table_t real_edges(int64_t(1.5 * size), true);
  hash_table_t fake_edges(int64_t(35 * size), true);
  hash_table_t real_edge_ids(int64_t(1.5 * size), true);
  hash_table_t en_count(int64_t(1.5 * size), true);
  hash_table_t e_rs_e_num(int64_t(1.5 * size), true);
  hash_real_edges<Graph> hre(g, vid_map, real_edges, real_edge_ids,
                             e_rs_e_num, en_count, vn_count, rinfo, 40);

  visit_edges(g, hre);

  tent_counter<Graph> tpc(g, real_edges, fake_edges, e_rs_e_num, 40);
  visit_adj(g, tpc);
  printf("real edges: %d\n", real_edges.size());
  printf("fake edges: %d\n", fake_edges.size());
  ncount_corrector<Graph> ncc(g, real_edges, fake_edges, en_count, 40);
  visit_adj(g, ncc);
  printf("final edge one neigh counts\n");
  en_count.visit(h);

  hashvis h;
  dhashvis dh;

  int* e_rs_v = (int*) malloc(sizeof(int) * order);
  memset(e_rs_v, 0, order);
  e_rs_vis<Graph> ersvis(g, vid_map, e_rs_v);
  visit_adj<Graph, e_rs_vis<Graph> >(g, ersvis);
  for (int i = 0; i < order; i++)
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
  Q_e_vis<Graph> qv(g, vid_map, en_count, vn_count, e_rs_e_num, e_rs_v, Q_e);
  visit_edges(g, qv);
  Q_e.visit(dh);

  return 0;
}

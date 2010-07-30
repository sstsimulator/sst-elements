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
/*! \file pagerank.hpp

    \author Jon Berry (jberry@sandia.gov)
    \author Karen Devine (kddevin@sandia.gov)

    \date 8/8/2008
*/
/****************************************************************************/

#ifndef MTGL_PAGERANK_HPP
#define MTGL_PAGERANK_HPP

#include <cstdlib>

#include <mtgl/static_graph_adapter.hpp>
#include <mtgl/quicksort.hpp>
#include <mtgl/breadth_first_search_mtgl.hpp>
#include <mtgl/visit_adj.hpp>
#include <mtgl/metrics.hpp>

#include "ppcPimCalls.h"

namespace mtgl {

typedef struct {
  int degree;
  double acc;
  double rank;
} rank_info;

#ifdef USING_QTHREADS

template <typename graph_adapter>
class qt_rank_info {
public:
    qt_rank_info(graph_adapter& gg, rank_info* const ri, unsigned int *types, unsigned int *fh) : 
        g(gg), rinfo(ri), types(types), fh(fh) {}

  graph_adapter& g;
  rank_info* const rinfo;
    unsigned int *types;
    unsigned int *fh;
};

template <class graph_adapter>
class qt_rank_info_ptr {
public:
  qt_rank_info_ptr(graph_adapter& gg, rank_info* const ri, void* extra) :
    g(gg), rinfo(ri), extra(extra) {}

  graph_adapter& g;
  rank_info* const rinfo;
  void* extra;
};

template <class graph_adapter>
class qt_rank_info_zerocnt {
public:
  qt_rank_info_zerocnt(graph_adapter& gg, rank_info* const ri,
                       int* zerodegvtx) :
    g(gg), rinfo(ri), zerodegvtx(zerodegvtx) {}

  graph_adapter& g;
  rank_info* const rinfo;
  int* zerodegvtx;
};

template <class graph_adapter>
class qt_rank_info_vector {
public:
  qt_rank_info_vector(graph_adapter& gg, rank_info* const ri, double* vec,
                      double extra, double extra2) :
    g(gg), rinfo(ri), vec(vec), extra(extra), extra2(extra2) {}

  graph_adapter& g;
  rank_info* const rinfo;
  double* vec;
  double extra;
  double extra2;
};

template <class graph_adapter>
void compute_acc_outer(qthread_t* me, const size_t startat, const size_t stopat, void* arg)
{
  typedef typename graph_traits<graph_adapter>::vertex_descriptor
          vertex_descriptor_t;
  typedef typename graph_traits<graph_adapter>::adjacency_iterator
          adj_iterator_t;
  typedef typename graph_traits<graph_adapter>::vertex_iterator vert_iter_t;

  qt_rank_info<graph_adapter>* qt_rinfo = (qt_rank_info<graph_adapter>*)arg;
  vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, qt_rinfo->g);
  int j;

  vert_iter_t verts = vertices(qt_rinfo->g);

  for (size_t i = startat; i < stopat; i++)
  {
    vertex_descriptor_t v = verts[i];
    adj_iterator_t adj_verts = adjacent_vertices(v, qt_rinfo->g);

    int sid = get(vid_map, v);
    int deg = out_degree(v, qt_rinfo->g);

    for (j = 0; j < deg; j++)
    {
      vertex_descriptor_t neighbor = adj_verts[j];

      int tid = get(vid_map, neighbor);
      double r = qt_rinfo->rinfo[sid].rank;
      double d = qt_rinfo->rinfo[sid].degree;

      mt_incr(qt_rinfo->rinfo[tid].acc, r / d);
    }
  }
}

template<>
void compute_acc_outer<static_graph_adapter<bidirectionalS> >(
    qthread_t* me, const size_t startat, const size_t stopat, void* arg)
{
  typedef static_graph_adapter<bidirectionalS> Graph;
  typedef graph_traits<Graph>::vertex_descriptor vertex_descriptor_t;

  qt_rank_info<Graph>* qt_rinfo = (qt_rank_info<Graph>*)arg;

  vertex_id_map<Graph> vid_map = get(_vertex_id_map, qt_rinfo->g);
  unsigned int *types = qt_rinfo->types;
  unsigned int *fh = qt_rinfo->fh;

  vertex_descriptor_t* rev_end_points = qt_rinfo->g.get_rev_end_points();

  for (size_t i = startat ; i < stopat ; ++i)
  {
    int begin = qt_rinfo->g[i];
    int end = qt_rinfo->g[i + 1];

#if defined(PIM_EXTENSIONS)
    // note: we really should include the begin and end lookups in the
    // PIM portion of the algorithm. future work.
    int result = 0;
    do {
      result = PIM_PageRank(begin, end, i, types, fh, (unsigned int*)rev_end_points, qt_rinfo->rinfo);
    } while (result == 0);
    while (PIM_OutstandingMR() > 31) {;}
#else
    double total = 0.0;
    // ARUN - FILTER IS HERE
    if (types[get(vid_map, i)] == 0 || fh[get(vid_map, i)] < 10) { continue; }

    for (int j  = begin ; j < end ; ++j)
    {
      vertex_descriptor_t src = rev_end_points[j];
      // ARUN - FILTER IS HERE
      if (types[get(vid_map, src)] == 0 || fh[get(vid_map, src)] < 10) { continue; }
      double r = qt_rinfo->rinfo[src].rank;
      double incr = (r / qt_rinfo->rinfo[src].degree);
      total += incr;
    }

    qt_rinfo->rinfo[i].acc = total;
#endif
  }
#if defined(PIM_EXTENSIONS)
  // make sure everyone is done
  while (PIM_OutstandingMR() > 0) {;}
#endif
}

template <typename graph_adapter>
void compute_zerodegcnt(qthread_t* me, const size_t startat,
                        const size_t stopat, void* arg, void* ret)
{
  typedef typename graph_traits<graph_adapter>::vertex_iterator vert_iter_t;

  qt_rank_info<graph_adapter>* qt_rinfo = (qt_rank_info<graph_adapter>*)arg;

  unsigned long order = num_vertices(qt_rinfo->g);
  aligned_t lcl_zerodegcnt = 0;

  vert_iter_t verts = vertices(qt_rinfo->g);

  for (size_t i = startat ; i < stopat ; ++i)
  {
    qt_rinfo->rinfo[i].rank = 1.0 / order;
    qt_rinfo->rinfo[i].degree = out_degree(verts[i], qt_rinfo->g);

    if (qt_rinfo->rinfo[i].degree == 0) lcl_zerodegcnt++;
  }

  //if (lcl_zerodegcnt != 0) mt_incr(zerodegcnt, lcl_zerodegcnt);
  *(aligned_t*) ret = lcl_zerodegcnt;
}

template <typename graph_adapter>
void prepare_compute_acc(qthread_t* me, const size_t startat,
                         const size_t stopat, void* arg, void* ret)
{
  qt_rank_info<graph_adapter>* qt_rinfo = (qt_rank_info<graph_adapter>*)arg;
  double lcl_sum = 0.0;

  for (size_t i = startat ; i < stopat ; ++i)
  {
    qt_rinfo->rinfo[i].acc = 0.0;
    lcl_sum += qt_rinfo->rinfo[i].rank;
  }

  *(double*) ret = lcl_sum;
}

template <typename graph_adapter>
void adjust_zero_outdeg(qthread_t* me, const size_t startat,
                        const size_t stopat, void* arg, void* ret)
{
  qt_rank_info_zerocnt<graph_adapter>* qt_rinfo =
    (qt_rank_info_zerocnt<graph_adapter>*)arg;

  double lcl_sum = 0.0;

  for (size_t i = startat ; i < stopat ; ++i)
  {
    lcl_sum += qt_rinfo->rinfo[qt_rinfo->zerodegvtx[i]].rank;
  }

  *(double*) ret = lcl_sum;
}

template <typename graph_adapter>
void compute_normvec(qthread_t* me, const size_t startat,
                     const size_t stopat, void* arg)
{
  qt_rank_info_vector<graph_adapter>* qt_rinfo =
    (qt_rank_info_vector<graph_adapter>*)arg;

  double adjustment = qt_rinfo->extra;
  double dampen = qt_rinfo->extra2;

  for (size_t i = startat ; i < stopat ; ++i)
  {
    qt_rinfo->rinfo[i].acc = adjustment + (dampen * qt_rinfo->rinfo[i].acc);

    double tmp = (qt_rinfo->rinfo[i].acc >= 0 ?
                  qt_rinfo->rinfo[i].acc : -1.0 * qt_rinfo->rinfo[i].acc);

    qt_rinfo->vec[i] = tmp;
  }
}

template <typename graph_adapter>
void compute_diff_vec(qthread_t* me, const size_t startat,
                      const size_t stopat, void* arg)
{
  qt_rank_info_vector<graph_adapter>* qt_rinfo =
    (qt_rank_info_vector<graph_adapter>*)arg;

  double normy = qt_rinfo->extra;

  for (size_t i = startat ; i < stopat ; ++i)
  {
    double oldval = qt_rinfo->rinfo[i].rank;
    double newval = qt_rinfo->rinfo[i].rank = qt_rinfo->rinfo[i].acc / normy;

    double absdiff = (oldval > newval) ?
                     oldval - newval : newval - oldval;

    qt_rinfo->vec[i] = absdiff;
  }
}

#endif

template <class graph_adapter>
void compute_acc(graph_adapter& g, rank_info* rinfo, unsigned int *types, unsigned int *fh)
{
  typedef graph_adapter Graph;
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor_t;
  typedef typename graph_traits<Graph>::adjacency_iterator adj_iterator_t;
  typedef typename graph_traits<Graph>::vertex_iterator vert_iter_t;

  unsigned int n = num_vertices(g);
#ifdef USING_QTHREADS
  qt_rank_info<Graph> ri(g, rinfo, types, fh);
  qt_loop_balance(0, n, compute_acc_outer<Graph>, &ri);
#else
  int i, j;
  vertex_id_map<Graph> vid_map = get(_vertex_id_map, g);

  vert_iter_t verts = vertices(g);

  #pragma mta assert parallel
  for (i = 0; i < n; i++)
  {
    vertex_descriptor_t v = verts[i];
    adj_iterator_t adj_verts = adjacent_vertices(v, g);

    int sid = get(vid_map, v);
    int deg = out_degree(v, g);

    #pragma mta assert parallel
    for (j = 0; j < deg; j++)
    {
      vertex_descriptor_t neighbor = adj_verts[j];
      int tid = get(vid_map, neighbor);

      double r = rinfo[sid].rank;
      rinfo[tid].acc += (r / (double) rinfo[sid].degree);
    }
  }
#endif
}

/*! \brief This version solves the hotspotting in compute_acc_over_edges and
           also runs faster in raw time.  The scaling extends through 40
           processors on the MTA-2 (if not perfectly).

    We leverage the bidirectional graph adapter and take advantage of the MTA
    compiler to remove reductions for the accumulation of propagated ranks to
    each vertex.  Note that canal fails to tell us this is happening, but in
    early versions of this code, I did see canal annotations indicating that
    all was well.  I also deduce that the reduction is removed since we scale
    from 20 to 40 processors on (0.57,0.19,0.19,0.05) R-MAT graphs.
*/
template <>
void compute_acc<static_graph_adapter<bidirectionalS> >(
                                                        static_graph_adapter<bidirectionalS>& g, 
                                                        rank_info* rinfo, unsigned int *types, unsigned int *fh)
{
  typedef static_graph_adapter<bidirectionalS> Graph;
  typedef graph_traits<Graph>::vertex_descriptor vertex_descriptor_t;

  unsigned int n = num_vertices(g);

#ifdef USING_QTHREADS
  //AFR
  qt_rank_info<Graph> ri(g, rinfo, types, fh);
  PIM_quickPrint(0,0,0);
  qt_loop_balance(0, n, compute_acc_outer<Graph>, &ri);
  PIM_quickPrint(0,0,1);
#else
  int i, j;
  vertex_id_map<Graph> vid_map = get(_vertex_id_map, g);
  vertex_descriptor_t* rev_end_points = g.get_rev_end_points();

  #pragma mta assert nodep
  #pragma mta use 100 streams
  for (int i = 0; i < n; i++)
  {
    double total = 0.0;
    int begin = g[i];
    int end = g[i + 1];

    for (int j = begin; j < end; j++)
    {
      int src = rev_end_points[j];
      double r = rinfo[src].rank;
      double incr = (r / (double) rinfo[src].degree);
      total += incr;
    }

    rinfo[i].acc = total;
  }
#endif
}

/// \brief Iterate through the list of edges (graph_adapter does this more
///        efficiently than static_graph_adapter).  This is the version used
///        in the May 8 report.  It hot spots with vertices of sufficiently
///        high degree.
template <class graph_adapter>
void compute_acc_over_edges(graph_adapter& g, rank_info* rinfo)
{
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph_adapter>::edge_iterator edge_iter_t;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_t;

  vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, g);

  int m = num_edges(g);
  edge_iter_t edgs = edges(g);

  #pragma mta assert parallel
  for (int i = 0; i < m; i++)
  {
    edge_t e = edgs[i];

    vertex_t src = source(e, g);
    vertex_t trg = target(e, g);

    int sid = get(vid_map, src);
    int tid = get(vid_map, trg);

    double r = rinfo[sid].rank;
    double incr = (r / (double) rinfo[sid].degree);
    rinfo[tid].acc += (r / (double) rinfo[sid].degree);
  }
}

template <>
void compute_acc<static_graph_adapter<undirectedS> >(
                                                     static_graph_adapter<undirectedS>& g, rank_info* rinfo, unsigned int *types, unsigned int *fh)
{
  typedef static_graph_adapter<undirectedS> Graph;
  typedef graph_traits<Graph>::vertex_descriptor vertex_descriptor_t;
  typedef graph_traits<Graph>::adjacency_iterator adj_iterator_t;
  typedef graph_traits<Graph>::vertex_iterator vert_iter_t;

  unsigned int n = num_vertices(g);
  vertex_id_map<Graph> vid_map = get(_vertex_id_map, g);

#ifdef USING_QTHREADS
  qt_rank_info<Graph> qt_rinfo(g, rinfo, types, fh);
  qt_loop_balance(0, n, compute_acc_outer<Graph>, &qt_rinfo);
#else
  vert_iter_t verts = vertices(g);

  #pragma mta assert parallel
  for (unsigned int i = 0; i < n; i++)
  {
    vertex_descriptor_t v = verts[i];
    adj_iterator_t adj_verts = adjacent_vertices(v, g);

    int sid = get(vid_map, v);
    int deg = out_degree(v, g);

    #pragma mta assert parallel
    for (int j = 0; j < deg; j++)
    {
      vertex_descriptor_t neighbor = adj_verts[j];
      int tid = get(vid_map, neighbor);

      double r = rinfo[sid].rank;
      rinfo[tid].acc += (r / (double) rinfo[sid].degree);
    }
  }
#endif
}

template <>
void compute_acc<static_graph_adapter<directedS> >(
                                                   static_graph_adapter<directedS>& g, rank_info* rinfo, unsigned int *types, unsigned int *fh)
{
  typedef static_graph_adapter<directedS> Graph;
  typedef graph_traits<Graph>::vertex_descriptor vertex_descriptor_t;
  typedef graph_traits<Graph>::adjacency_iterator adj_iterator_t;
  typedef graph_traits<Graph>::vertex_iterator vert_iter_t;

  unsigned int n = num_vertices(g);
  vertex_id_map<Graph> vid_map = get(_vertex_id_map, g);

#ifdef USING_QTHREADS
  qt_rank_info<Graph> qt_rinfo(g, rinfo, types, fh);
  qt_loop_balance(0, n, compute_acc_outer<Graph>, &qt_rinfo);
#else
  vert_iter_t verts = vertices(g);

  #pragma mta assert parallel
  for (unsigned int i = 0; i < n; i++)
  {
    vertex_descriptor_t v = verts[i];
    adj_iterator_t adj_verts = adjacent_vertices(v, g);

    int sid = get(vid_map, v);
    int deg = out_degree(v, g);

    #pragma mta assert parallel
    for (int j = 0; j < deg; j++)
    {
      vertex_descriptor_t neighbor = adj_verts[j];
      int tid = get(vid_map, neighbor);

      double r = rinfo[sid].rank;
      rinfo[tid].acc += (r / (double) rinfo[sid].degree);
    }
  }
#endif
}

template <typename graph>
void compute_acc_visit_adj(graph& G, int*& prefix_counts, int*& start_nodes,
                           int& num_threads, rank_info* rinfo,
                           int chunk_size = 500)
{
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::adjacency_iterator adj_iter_t;
  typedef typename graph_traits<graph>::vertex_iterator vert_iter_t;

  unsigned int ord = num_vertices(G);

  vertex_id_map<graph> vid_map = get(_vertex_id_map, G);

  if (start_nodes == 0)
  {
    if (prefix_counts != 0) free(prefix_counts);

    gen_load_balance_info(G, prefix_counts, start_nodes,
                          num_threads, chunk_size);
  }
  else if (prefix_counts == 0)
  {
    if (start_nodes != 0) free(start_nodes);

    gen_load_balance_info(G, prefix_counts, start_nodes,
                          num_threads, chunk_size);
  }

  vert_iter_t verts = vertices(G);

  #pragma mta assert parallel
  for (int i = 0; i < num_threads; i++)
  {
    int count  = 0;
    int curr_n = start_nodes[i];
    int my_offset = i * chunk_size - prefix_counts[curr_n];

    while (count < chunk_size && curr_n < ord)
    {
      vertex_t u = verts[curr_n];
      int uid = get(vid_map, u);
      int deg = out_degree(u, G);

      adj_iter_t adj_verts = adjacent_vertices(u, G);

      for (int j = my_offset; j < deg; j++)
      {
        vertex_t v = adj_verts[j];
        int vid = get(vid_map, v);

        count++;

        double r = rinfo[uid].rank;
        rinfo[vid].acc += (r / (double) rinfo[uid].degree);

        if (count == chunk_size) break;
      }

      my_offset = 0;
      curr_n++;
    }
  }
}

template <typename graph>
class page_rank_visitor {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;

  page_rank_visitor(graph& gg, vertex_id_map<graph>& vidm,
                    rank_info* _rinfo) :
    g(gg), vid_map(vidm), rinfo(_rinfo) {}

  page_rank_visitor(const page_rank_visitor& cv) :
    g(cv.g), vid_map(cv.vid_map), rinfo(cv.rinfo) {}

  void pre_visit(vertex_t v) {}

  void operator()(edge_t e, vertex_t src, vertex_t dest)
  {
    int sid = get(vid_map, src);
    int tid = get(vid_map, dest);
    double r = rinfo[sid].rank;
    rinfo[tid].acc += (r / (double) rinfo[sid].degree);
  }

  void process_edge(edge_t e, vertex_t src, vertex_t dest)
  {
    int sid = get(vid_map, src);
    int tid = get(vid_map, dest);
    double r = rinfo[sid].rank;
    rinfo[tid].acc += (r / (double) rinfo[sid].degree);
  }

private:
  graph& g;
  vertex_id_map<graph> vid_map;
  rank_info* rinfo;
};

template <class graph_adapter>
class pagerank {
public:
  typedef typename graph_traits<graph_adapter>::vertex_iterator vert_iter_t;

    pagerank(graph_adapter& gg, unsigned int* types, unsigned int *fh) : g(gg), types(types), fh(fh) {}

  rank_info* run(double delta)
  {
    double dampen = 0.8;
    unsigned int order = num_vertices(g);
    rank_info* rinfo = new rank_info[order];
    unsigned long zerodegcnt = 0;    // Number of vertices with out-degree = 0.

#ifdef USING_QTHREADS
    // Used to store the indidual values for the normy and maxdiff reductions.
    double* rank_update = new double[order];
#endif

#ifdef WANT_TIMING
    double time_prep = 0.0, time_acc = 0.0, time_zdc = 0.0;
    double time_normy = 0.0, time_maxdiff = 0.0;
    struct timeval start, stop;
#endif

#ifdef USING_QTHREADS
    qt_rank_info<graph_adapter> zerodegcnt_rinfo(g, rinfo, types, fh);
    qt_loopaccum_balance(0, order, sizeof(zerodegcnt), &zerodegcnt,
                         compute_zerodegcnt<graph_adapter>, &zerodegcnt_rinfo,
                         qt_uint_add_acc);
#else
    vert_iter_t verts = vertices(g);

    #pragma mta assert nodep
    for (unsigned int i = 0; i < order; i++)
    {
      rinfo[i].rank = 1.0 / order;
      rinfo[i].degree = out_degree(verts[i], g);

      if (rinfo[i].degree == 0) zerodegcnt++;
    }
#endif

    int* zerodegvtx = NULL;   // Indices of vertices with out-degree = 0.

    if (zerodegcnt)
    {
      zerodegvtx = new int[zerodegcnt];
      zerodegcnt = 0;

      for (unsigned int i = 0; i < order; i++)
      {
        if (rinfo[i].degree == 0) zerodegvtx[zerodegcnt++] = i;
      }
    }

    int iter_cnt = 0;
    double maxdiff = 0.0;

    do
    {
      iter_cnt++;

      double sum = 0.;

#ifdef WANT_TIMING
      gettimeofday(&start, NULL);
#endif

#ifdef USING_QTHREADS
      {
          qt_rank_info<graph_adapter> qt_ri_sum(g, rinfo, types, fh);
        qt_loopaccum_balance(0, order, sizeof(sum), &sum,
                             prepare_compute_acc<graph_adapter>,
                             &qt_ri_sum, qt_dbl_add_acc);
      }
#else
      #pragma mta assert nodep
      for (unsigned int i = 0; i < order; i++)
      {
        rinfo[i].acc = 0.0;
        sum += rinfo[i].rank;
      }
#endif

#ifdef WANT_TIMING
      gettimeofday(&stop, NULL);
      time_prep += ((stop.tv_sec - start.tv_sec) +
                    ((stop.tv_usec - start.tv_usec) / 1000000.0));
      gettimeofday(&start, NULL);
#endif

      compute_acc<graph_adapter>(g, rinfo, types, fh);
      double adjustment = (1. - dampen) / order * sum;

#ifdef WANT_TIMING
      gettimeofday(&stop, NULL);
      time_acc += ((stop.tv_sec - start.tv_sec) +
                   ((stop.tv_usec - start.tv_usec) / 1000000.0));
      gettimeofday(&start, NULL);
#endif

      // Adjustment for zero-outdegree vertices.
      sum = 0.;

#ifdef USING_QTHREADS
      {
        qt_rank_info_zerocnt<graph_adapter> qt_ri_zeroadj(g, rinfo, zerodegvtx);
        qt_loopaccum_balance(0, zerodegcnt, sizeof(sum),
                             &sum, adjust_zero_outdeg<graph_adapter>,
                             &qt_ri_zeroadj, qt_dbl_add_acc);
      }
#else
      #pragma mta assert nodep
      for (int i = 0; i < zerodegcnt; i++) sum += rinfo[zerodegvtx[i]].rank;
#endif

      adjustment += dampen * sum / order;

#ifdef WANT_TIMING
      gettimeofday(&stop, NULL);
      time_zdc += ((stop.tv_sec - start.tv_sec) +
                   ((stop.tv_usec - start.tv_usec) / 1000000.0));
      gettimeofday(&start, NULL);
#endif

      // Compute new solution vector and scaling factor.
      double normy = 0.;

#if defined(USING_QTHREADS)
      {
        qt_rank_info_vector<graph_adapter>
          qt_rinfo(g, rinfo, rank_update, adjustment, dampen);

        qt_loop_balance(0, order, compute_normvec<graph_adapter>, &qt_rinfo);
        normy = qt_double_max(rank_update, order, 0);
      }
#else
      #pragma mta assert parallel
      for (unsigned int i = 0; i < order; i++)
      {
        rinfo[i].acc = adjustment + (dampen * rinfo[i].acc);

        double tmp = (rinfo[i].acc >= 0.  ? rinfo[i].acc : -1. * rinfo[i].acc);

        if (tmp > normy) normy = tmp;
      }
#endif

#ifdef WANT_TIMING
      gettimeofday(&stop, NULL);
      time_normy += ((stop.tv_sec - start.tv_sec) +
                     ((stop.tv_usec - start.tv_usec) / 1000000.0));
      gettimeofday(&start, NULL);
#endif

      maxdiff = 0;

#ifdef USING_QTHREADS
      {
        qt_rank_info_vector<graph_adapter>
          qt_rinfo(g, rinfo, rank_update, normy, 0.0);

        qt_loop_balance(0, order, compute_diff_vec<graph_adapter>, &qt_rinfo);
        maxdiff = qt_double_max(rank_update, order, 0);
      }
#else
      int bottleneck = 0;

      #pragma mta assert parallel
      for (unsigned int i = 0; i < order; i++)
      {
        double oldval = rinfo[i].rank;
        double newval = rinfo[i].rank = rinfo[i].acc / normy;
        double absdiff = (oldval > newval) ? oldval - newval : newval - oldval;

        if (absdiff > maxdiff)
        {
          maxdiff = absdiff;
          bottleneck = i;
        }
      }
#endif

#ifdef WANT_TIMING
      gettimeofday(&stop, NULL);
      time_maxdiff += ((stop.tv_sec - start.tv_sec) +
                       ((stop.tv_usec - start.tv_usec) / 1000000.0));
#endif

    } while (maxdiff > delta);
    printf("iterations: %d\n", iter_cnt);

#ifdef WANT_TIMING
    printf("prep: %f, acc: %f, zero comp: %f, normy: %f, maxdiff: %f\n",
           time_prep, time_acc, time_zdc, time_normy, time_maxdiff);
#endif

    return rinfo;
  }

private:
  graph_adapter& g;
    unsigned int *types;
    unsigned int *fh;
};

} // namespace

#endif

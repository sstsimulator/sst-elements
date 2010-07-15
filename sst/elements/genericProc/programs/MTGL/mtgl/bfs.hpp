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
/*! \file bfs.hpp

    \brief  This is the version of breadth-first search that currently
            performs the best on graphs with power law degree distributions.
            It is also the version used in the 2009 MTAAP paper on Qthreads
            and the MTGL.

    \author Jon Berry (jberry@sandia.gov)

    \date 2007
*/
/****************************************************************************/

#ifndef MTGL_BFS_HPP
#define MTGL_BFS_HPP

#ifdef USING_QTHREADS
#include <qthread/qutil.h>
#endif

#include <limits>
#include <cassert>

#include <mtgl/util.hpp>
#include <mtgl/mtgl_boost_property.hpp>
#include <mtgl/mtgl_adapter.hpp>
#include <mtgl/visit_adj.hpp>

#define BFS_CHUNK 256     // Hardcoded for qthreads/Niagara to avoid malloc.

namespace mtgl {

#ifndef BLACK
#define BLACK 1;
#endif

// from deprecated breadth_first_search_mtgl.hpp
template <typename graph>
struct default_bfs2_visitor {
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  void bfs_root(vertex& v) const {}
  void pre_visit(vertex& v) {}
  int start_test(vertex& v) { return false; }
  int visit_test(edge& e, vertex& v) { return false; }
  void tree_edge(edge& e, vertex& src) const {}
  void back_edge(edge& e, vertex& src) const {}
  void finish_vertex(vertex& v) const {}
};

template <class graph>
class default_bfs_user_vis {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  default_bfs_user_vis() {}
  void operator()(vertex_t v, graph &g) {}
};

template <class graph, class colormap,
          class user_visitor = default_bfs_user_vis<graph> >
class bfs_chunked_visitor {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::size_type count_t;

  bfs_chunked_visitor(vertex_t* q, count_t& tl, graph& g, colormap& colr,
                      user_visitor usrv, vertex_t* buf, count_t& bpos,
                      count_t bc) :
#ifdef USING_QTHREADS
      what_I_labeled(new vertex_t[BFS_CHUNK]),
#endif
    Q(q), tail(tl), G(g), color(colr), user_vis(usrv), count(0),
    buffer(buf), buffer_pos(bpos), bufcap(bc) {}

  // The copy constructor is the mechanism to give each thread its visitor
  // instance. On the XMT, we'll take our position in the buffer here.

  bfs_chunked_visitor(const bfs_chunked_visitor& bfsv) :
#ifdef USING_QTHREADS
      what_I_labeled(new vertex_t[BFS_CHUNK]),
#endif
    Q(bfsv.Q), tail(bfsv.tail), G(bfsv.G), color(bfsv.color),
    user_vis(bfsv.user_vis),
    count(0), buffer(bfsv.buffer),
    buffer_pos(bfsv.buffer_pos), bufcap(bfsv.bufcap)
  {
#ifndef USING_QTHREADS
    count_t my_start = mt_incr(buffer_pos, BFS_CHUNK);
    assert (my_start < bufcap);
    buffer = buffer + my_start;
#endif
  }

#ifdef USING_QTHREADS
  ~bfs_chunked_visitor() { delete what_I_labeled; }
#endif

#ifndef USING_QTHREADS
  void operator()(vertex_t u, vertex_t v)
  {
    if (color[v] == std::numeric_limits<count_t>::max())
    {
      count_t mark = mt_readfe(color[v]);
      count_t newmark = mark == std::numeric_limits<count_t>::max() ? u : mark;
      mt_write(color[v], newmark);

      if (mark == std::numeric_limits<count_t>::max())
      {
        buffer[count++] = v;
        user_vis(u, v, G);
      }
    }
  }

#else
  void operator()(vertex_t u, vertex_t v)
  {
    if (color[v] == std::numeric_limits<count_t>::max())
    {
      color[v] = u;
      what_I_labeled[count++] = v;
      user_vis(u, v, G);
    }
  }
#endif

#ifndef USING_QTHREADS
  void post_visit()
  {
    if (count > 0)
    {
      count_t my_q_ptr = mt_incr(tail, count);

      assert(my_q_ptr + count <= num_vertices(G));

      for (count_t i = 0; i < count; i++)
      {
        Q[my_q_ptr++] = buffer[i];
      }

      count = 0;
    }
  }

#else
  void post_visit()
  {
    count_t my_buffer_pos = mt_incr(buffer_pos, count);
    count_t my_end = my_buffer_pos + count;
    count_t i, j;
    assert(my_end < 2 * num_vertices(G));

    for (i = my_buffer_pos, j = 0; i < my_end; i++, j++)
    {
      buffer[i] = what_I_labeled[j];
    }

    count = 0;
  }
#endif

private:
#ifdef USING_QTHREADS
  vertex_t *what_I_labeled;
#endif
  vertex_t* Q;
  count_t& tail;
  graph& G;
  colormap& color;
  user_visitor user_vis;
  count_t count;
  vertex_t* buffer;
  count_t& buffer_pos;
  count_t bufcap;
};

template <class vertex_t>
int vertex_t_cmp(const void* u, const void* v)  // Works for integral type.
{
  vertex_t* uu = (vertex_t*) u;
  vertex_t* vv = (vertex_t*) v;
  return *uu - *vv;
}

//  bfsCountingSort:  Why not just use the mtgl countingSort?
//                    The latter is templated, and canal can't give us
//                    annotations for different instantiations.  Canal
//                    was claiming that countingSort was parallelized,
//                    but in practice it ran in serial (for unknown reasons).
//

/* Since this file may be included in multiple places in an app, this
   function needs to be inline, or static (or moved to another file). */

#pragma mta debug level 0
inline
void bfsCountingSort(unsigned long* array, unsigned long asize,
                     unsigned long maxval)  // size_type?
{
  unsigned long nbin = (unsigned long) maxval + 1;
  unsigned long* count  = (unsigned long*) calloc(nbin, sizeof(unsigned long));
  unsigned long* result = (unsigned long*) calloc(asize, sizeof(unsigned long));

  for (unsigned long i = 0; i < asize; i++) count[array[i]]++;

  unsigned long* start = (unsigned long*) malloc(nbin * sizeof(unsigned long));

  start[0] = 0;

  for (unsigned long i = 1; i < nbin; i++)
  {
    start[i] = start[i - 1] + count[i - 1];
  }

  #pragma mta assert nodep
  for (unsigned long i = 0; i < asize; i++)
  {
    unsigned long loc = mt_incr(start[array[i]], 1);
    result[loc] = array[i];
  }

  for (unsigned long i = 0; i < asize; i++) array[i] = result[i];

  free(count);
  free(start);
  free(result);
}

#pragma mta debug level default

template <class graph>
void
treeCheck(typename graph_traits<graph>::size_type root,
          typename graph_traits<graph>::size_type maxDist,
          graph& G,
          typename graph_traits<graph>::size_type* bfs_parent)
{
  typedef typename graph_traits<graph>::size_type index_t;
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;

//  printf("entering treeCheck\n");
//  fflush(stdout);

  index_t N = num_vertices(G);
  index_t* dist = (index_t*) malloc(N * sizeof(index_t));

  #pragma mta noalias *dist
  const index_t* Marked = bfs_parent;

  for(index_t i = 0; i < N; i++) dist[i] = ULONG_MAX;

  dist[root] = 0;

  for(index_t l = 1; l <= maxDist; l++)
  {
    #pragma mta assert nodep *dist
    for(index_t i = 0; i < N; i++)
    {
      index_t parent = Marked[i];

      if (parent != ULONG_MAX && dist[i] == ULONG_MAX && dist[parent] == l - 1)
      {
        dist[i] = l;
      }
    }
  }

  const index_t* numNeighbors = G.get_index();
  const vertex_t* Neighbors = G.get_end_points();

  unsigned shortcuts = 0, notreachable = 0;
  const unsigned MAXSHORTCUT = 1000;

  struct Short
  {
    index_t u, v;
  } shortcut[MAXSHORTCUT];

  #pragma mta assert parallel
  for(index_t u = 0; u < N; u++)
  {
    index_t uDist = dist[u];

    if (uDist != ULONG_MAX)
    {
      // If any of the edges is a shortcut, the tree is not
      // a shortest path tree.
      for(size_t j = numNeighbors[u]; j < numNeighbors[u + 1]; j++)
      {
        vertex_t v = Neighbors[j];
        assert (v < N);

        if (dist[v] == ULONG_MAX || uDist + 1 < dist[v])
        {
          unsigned s = mt_incr(shortcuts, 1);

          if (s < MAXSHORTCUT)
          {
            shortcut[s].u = u;
            shortcut[s].v = v;
          }
        }
      }
    }
    else
    {
      if (Marked[u] < ULONG_MAX) notreachable++;
    }
  }

  if (notreachable)
  {
    printf("%u nodes are marked, but not reachable from %lu\n",
           notreachable, root);
  }

  if (shortcuts)
  {
    printf("there are %u shortcut edges\n", shortcuts);
    index_t minshort = (MAXSHORTCUT < shortcuts) ? MAXSHORTCUT : shortcuts;

    for(index_t i = 0; i < minshort; i++)
    {
      index_t u = shortcut[i].u;
      index_t v = shortcut[i].v;
      assert (u < N && v < N);
      printf("%lu %lu %lu %lu %lu %lu\n", u, v, dist[u], dist[v], Marked[u],
             Marked[v]);
    }
  }

  free(dist);

//  printf("treeCheck complete\n");
//  fflush(stdout);
}

template <class graph, class user_visitor>
void expand_one_edge(graph& G, user_visitor user_vis,
            typename graph_traits<graph>::vertex_descriptor *Q,
            typename graph_traits<graph>::size_type& head,
            typename graph_traits<graph>::size_type& tail,
            typename graph_traits<graph>::size_type* colrs)
{
    typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
    typedef typename graph_traits<graph>::size_type size_type;
    typedef array_property_map<size_type, vertex_id_map<graph> > colormap;

    vertex_id_map<graph> vid = get(_vertex_id_map, G);
    colormap color = make_array_property_map<size_type,
                                           vertex_id_map<graph> >(colrs, vid);
    size_type *prefix_counts = 0;
    size_type *started_nodes = 0;
    size_type  num_threads = 0;
    size_type buffer_pos = 0;
    size_type bufcap = 0;

    size_type si = head;
    head = tail;
    size_type work_this_phase = 0;

#ifndef USING_QTHREADS
    #pragma mta assert nodep
    for (size_type i = si; i < tail; i++)
    {
      work_this_phase += degree(Q[i], G);
    }

    size_type avail_streams = (work_this_phase + BFS_CHUNK - 1) / BFS_CHUNK;
    bufcap = (avail_streams + 1) * BFS_CHUNK;
#else
    bufcap = 2 * G.get_order();
    work_this_phase = G.get_order();
#endif

//    printf("bufcap: %lu\n", bufcap);
    vertex_t* buffer = (vertex_t*) malloc(sizeof(vertex_t) * bufcap);

    bfs_chunked_visitor<graph, colormap, user_visitor>
    bfsvis(Q, tail, G, color, user_vis, buffer, buffer_pos, bufcap);

    mt_timer timer;
//    int issues, memrefs, concur, streams, traps;
//    init_mta_counters(timer, issues, memrefs, concur, streams, traps);

    if (work_this_phase > BFS_CHUNK)
    {
      visit_adj(G, &Q[si], tail - si, bfsvis, prefix_counts,
                started_nodes, num_threads, BFS_CHUNK);
    }
    else
    {
      visit_adj_serial(G, &Q[si], tail - si, bfsvis);
    }

//    sample_mta_counters(timer, issues, memrefs, concur, streams, traps);
//    printf("---------------------------------------------\n");
//    printf("visit_adj: \n");
//    printf("---------------------------------------------\n");
//    print_mta_counters(timer, G.get_size(), issues, memrefs, concur,
//                       streams, traps);
//    printf("num threads: %d, chunk_size: %d\n", num_threads, BFS_CHUNK);

#ifdef USING_QTHREADS
    if (buffer_pos > 0)
    {
      qutil_aligned_qsort(qthread_self(), (aligned_t*) buffer, buffer_pos);

      for (size_type i = 0; i < buffer_pos - 1; i++)
      {
        vertex_t v = buffer[i];
        vertex_t v2 = buffer[i + 1];

        if (v != v2) Q[tail++] = v;  // Not hot; single threaded.
      }

      vertex_t v = buffer[buffer_pos - 1];
      Q[tail++] = v;
    }
#endif

    free(buffer);

#if DEBUG
    printf("after phase %d: start: %d, end: %d\n", numPhases,
           head, tail);
    fflush(stdout);
#endif

    free(prefix_counts);
    free(started_nodes);
    #pragma mta trace "level loop done in bfs"
}

template<class graph, class user_visitor>
typename graph_traits<graph>::size_type
bfs_chunked(graph& G, user_visitor user_vis,
            typename graph_traits<graph>::vertex_descriptor s,
            typename graph_traits<graph>::size_type* colrs = 0)
{
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::size_type size_type;
  typedef array_property_map<size_type, vertex_id_map<graph> > colormap;

  vertex_id_map<graph> vid = get(_vertex_id_map, G);

  size_type* prefix_counts = 0;   // Sum of degrees of vertices preceding i.
  size_type* started_nodes = 0;   // Number of nodes for which processing has
                                  // started (for each thread).
  size_type num_threads = 0;
  size_type numPhases = 0;

#if DEBUG
  int avgV, avgE;
  double avgT;
  size_type* numEdges = G.get_index();
  size_type* nVertsInPhase = (size_type*) calloc(1000000, sizeof(size_type));
  size_type* nEdgesInPhase = (size_type*) calloc(1000000, sizeof(size_type));
  double* time = (double*) calloc(1000000, sizeof(double));
#endif

  size_type n = num_vertices(G);

  vertex_t* Q  = (vertex_t*) malloc(n * sizeof(vertex_t));
  bool i_own_colrs = (colrs == 0);

  if (i_own_colrs)
  {
    colrs = (size_type*) malloc(sizeof(size_type) * n);
  }

  size_type bufcap = 0;

  for (size_type i = 0; i < n; ++i)
  {
    colrs[i] = std::numeric_limits<size_type>::max();
  }

  colormap color = make_array_property_map<size_type,
                                           vertex_id_map<graph> >(colrs, vid);

  color[s] = BLACK;

  Q[0] = s;
  size_type startIndex = 0;
  size_type endIndex = 1;

  while (endIndex - startIndex > 0)
  {
    size_type buffer_pos = 0;

#if DEBUG
    nVertsInPhase[numPhases] = endIndex - startIndex;

    for (size_type i = startIndex; i < endIndex; i++)
    {
      size_type v_ind = get(vid, Q[i]);
      mt_incr(&nEdgesInPhase[numPhases], numEdges[v_ind + 1] - numEdges[v_ind]);
    }
#endif

    size_type si = startIndex;
    startIndex = endIndex;
    size_type work_this_phase = 0;

#ifndef USING_QTHREADS
    // The buffer size is a "rounded" version of the amount of work to be
    // performed this phase.
    #pragma mta assert nodep
    for (size_type i = si; i < endIndex; i++)
    {
      work_this_phase += out_degree(Q[i], G);
    }

    size_type avail_streams = (work_this_phase + BFS_CHUNK - 1) / BFS_CHUNK;
    bufcap = (avail_streams + 1) * BFS_CHUNK;
#else
    bufcap = 2 * num_vertices(G);
    work_this_phase = num_vertices(G);
#endif

//    printf("bufcap: %lu\n", bufcap);
    vertex_t* buffer = (vertex_t*) malloc(sizeof(vertex_t) * bufcap);

    bfs_chunked_visitor<graph, colormap, user_visitor>
    bfsvis(Q, endIndex, G, color, user_vis, buffer, buffer_pos, bufcap);

    mt_timer timer;
//    int issues, memrefs, concur, streams, traps;
//    init_mta_counters(timer, issues, memrefs, concur, streams, traps);

    if (work_this_phase > BFS_CHUNK)
    {
      visit_adj(G, &Q[si], endIndex - si, bfsvis, prefix_counts,
                started_nodes, num_threads, BFS_CHUNK);
    }
    else
    {
      visit_adj_serial(G, &Q[si], endIndex - si, bfsvis);
    }

//    sample_mta_counters(timer, issues, memrefs, concur, streams, traps);
//    printf("---------------------------------------------\n");
//    printf("visit_adj: \n");
//    printf("---------------------------------------------\n");
//    print_mta_counters(timer, num_edges(G), issues, memrefs, concur,
//                       streams, traps);
//    printf("num threads: %d, chunk_size: %d\n", num_threads, BFS_CHUNK);

#ifdef USING_QTHREADS
    if (buffer_pos > 0)
    {
      qutil_aligned_qsort(qthread_self(), (aligned_t*) buffer, buffer_pos);

      for (size_type i = 0; i < buffer_pos - 1; i++)
      {
        vertex_t v = buffer[i];
        vertex_t v2 = buffer[i + 1];

        if (v != v2) Q[endIndex++] = v;  // Not hot; single threaded.
      }

      vertex_t v = buffer[buffer_pos - 1];
      Q[endIndex++] = v;
    }
#endif

#if DEBUG
    printf("after phase %d: start: %d, end: %d\n", numPhases,
           startIndex, endIndex);
    fflush(stdout);
#endif

    numPhases++;

    free(buffer);
    free(prefix_counts);
    free(started_nodes);

    prefix_counts = 0;
    started_nodes = 0;

    #pragma mta trace "level loop done in bfs"
  }

#if DEBUG
  avgV = 0;
  avgE = 0;
  avgT = 0;

  for (size_type i = 0; i < numPhases; i++)
  {
    avgV += nVertsInPhase[i];
    avgE += nEdgesInPhase[i];
    avgT += time[i];
  }

  size_type m = num_edges(G);

  FILE* outfp = fopen("stats.txt", "a");
  fprintf(outfp, "n:%d, m:%d\n", n, m);
  fprintf(outfp, "numPhases: %d, avg. verts per phase %d (%d), avg. edges "
          "per phase %d (%d)\n", numPhases, n / numPhases, avgV / numPhases,
          m / numPhases, avgE / numPhases);
  fprintf(outfp, "Time taken: %lf, avg time %lf\n", avgT, avgT / numPhases);
  fprintf(outfp, "Phase Work and running times:\n");

  for (size_type i = 0; i < numPhases; i++)
  {
    fprintf(outfp, "%d  -- n:%d, m:%d, t:%lf\n", i, nVertsInPhase[i],
            nEdgesInPhase[i], time[i]);
  }

  fprintf(outfp, "\n");
  fclose(outfp);

  free(nVertsInPhase);
  free(nEdgesInPhase);
  free(time);
#endif

  free(Q);

  if (i_own_colrs) free(colrs);

  return numPhases;
}

#ifdef USING_QTHREADS
template <typename graph>
struct simple_bfs_s {
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::size_type count_t;
  typedef array_property_map<count_t, vertex_id_map<graph> > colormap;

  graph& g;
  vertex_t* Q;
  colormap& color;
  count_t* index;
  vertex_t* endV;
  count_t& count;

  simple_bfs_s(graph& gg,  vertex_t* q, colormap& cl, count_t* ind,
               vertex_t* edV, count_t& ct) :
    g(gg), Q(q), color(cl), index(ind), endV(edV), count(ct) {}
};

template <typename graph>
void simple_bfs_inner(qthread_t* me, const size_t startat, const size_t endat,
                      void* arg)
{
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::size_type count_t;
  typedef array_property_map<count_t, vertex_id_map<graph> > colormap;

  simple_bfs_s<graph>* args = (simple_bfs_s<graph>*)arg;
  graph& G = args->g;
  vertex_t* Q = args->Q;
  colormap& color = args->color;
  count_t* index = args->index;
  vertex_t* endV = args->endV;
  count_t& count = args->count;

  for (count_t i = startat; i < endat; i++)
  {
    count_t j = 0;
    count_t u = Q[i];

    count_t startIter = index[u];
    count_t endIter = index[u + 1];

    for (j = startIter; j < endIter; j++)
    {
      count_t v = endV[j];
      count_t clr = mt_incr(color[v], 1);

      if (clr == 0)
      {
        count_t index = mt_incr(count, 1);
        Q[index] = v;
      }
    }
  }
}
#endif

template<class graph, class user_visitor>
void bfs(graph& G, user_visitor user_vis,
         typename graph_traits<graph>::vertex_descriptor s)
{
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::size_type count_t;
  typedef array_property_map<count_t, vertex_id_map<graph> > colormap;

  vertex_id_map<graph> vid = get(_vertex_id_map, G);
  count_t i, m, n;
  vertex_t* Q;
  count_t startIndex, endIndex, count = 0;
  count_t* prefix_counts = 0;   // Sum of degrees of vertices preceding i.
  count_t* start_nodes = 0;     // First node for each thread.
  int num_threads = 0;
  count_t* numEdges = G.get_index();
  int* endV = G.get_end_points();

#if DEBUG
  int BFSticks;
  count_t* nVertsInPhase, * nEdgesInPhase, numPhases;
  double* time;
  FILE* outfp;
  int avgV, avgE;
  double avgT;
  nVertsInPhase = (count_t*) calloc(1000000, sizeof(count_t));
  nEdgesInPhase = (count_t*) calloc(1000000, sizeof(count_t));
  time = (double*) calloc(1000000, sizeof(double));
  numPhases = 0;
#endif

  n = num_vertices(G);
  m = num_edges(G);

  Q  = (vertex_t*) malloc(n * sizeof(vertex_t));
  count_t* colrs = (count_t*) malloc(sizeof(count_t) * n);
  memset(colrs, 0, sizeof(count_t) * n);  // Need mt_init.

  colormap color = make_array_property_map<count_t,
                                           vertex_id_map<graph> >(colrs, vid);
  //colormap color(colrs, vid);

  color[s] = BLACK;

  Q[0] = s;
  startIndex = 0;
  endIndex = 1;

  mt_timer bfs_timer;
  bfs_timer.start();

  while (endIndex - startIndex > 0)
  {
    count = endIndex;

#if DEBUG
    numPhases++;
    nVertsInPhase[numPhases] = endIndex - startIndex;
    for (i = startIndex; i < endIndex; i++)
    {
      mt_incr(nEdgesInPhase[numPhases], numEdges[Q[i] + 1] - numEdges[Q[i]]);
    }
#endif

#ifndef USING_QTHREADS
    #pragma mta assert parallel
    for (i = startIndex; i < endIndex; i++)
    {
      count_t j = 0;
      count_t u = Q[i];

      count_t startIter = numEdges[u];
      count_t endIter = numEdges[u + 1];

      //printf("degree[%d]: %d\n", u, endIter-startIter);
      // #pragma mta assert parallel
      for (j = startIter; j < endIter; j++)
      {
        count_t v    = endV[j];
        count_t clr = mt_incr(color[v], 1);

        if (clr == 0)
        {
          count_t index = mt_incr(count, 1);
          Q[index] = v;
        }
      }
    }
#else
    simple_bfs_s<graph> args(G, Q, color, numEdges, endV, count);
    qt_loop_balance(startIndex, endIndex, simple_bfs_inner<graph>, &args);
#endif

    startIndex = endIndex;
    endIndex = count;
  }

  bfs_timer.stop();
  printf("bfs only (w/o init) secs: %f\n", bfs_timer.getElapsedSeconds());

#if DEBUG
  avgV = 0;
  avgE = 0;
  avgT = 0;

  for (i = 0; i < numPhases; i++)
  {
    avgV += nVertsInPhase[i];
    avgE += nEdgesInPhase[i];
    avgT += time[i];
  }

  outfp = fopen("stats.txt", "a");
  fprintf(outfp, "n:%d, m:%d\n", n, m);
  fprintf(outfp, "numPhases: %d, avg. verts per phase %d (%d), avg. edges "
          "per phase %d (%d)\n", numPhases, n / numPhases, avgV / numPhases,
          m / numPhases, avgE / numPhases);
  fprintf(outfp, "Time taken: %lf, avg time %lf\n", avgT, avgT / numPhases);
  fprintf(outfp, "Phase Work and running times:\n");

  for (i = 0; i < numPhases; i++)
  {
    fprintf(outfp, "%d  -- n:%d, m:%d, t:%lf\n", i, nVertsInPhase[i],
            nEdgesInPhase[i], time[i]);
  }

  fprintf(outfp, "\n");

  fclose(outfp);

  free(nVertsInPhase);
  free(nEdgesInPhase);
  free(time);
#endif

  free(Q);
  free(colrs);
}

}

#endif

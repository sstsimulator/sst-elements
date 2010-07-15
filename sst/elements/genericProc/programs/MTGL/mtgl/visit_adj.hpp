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
/*! \file visit_adj.hpp

    \author Jon Berry (jberry@sandia.gov)

    \date 12/4/2007
*/
/****************************************************************************/

#ifndef MTGL_VISIT_ADJ_HPP
#define MTGL_VISIT_ADJ_HPP

#include <mtgl/util.hpp>
#include <mtgl/graph_traits.hpp>
#include <mtgl/mtgl_adapter.hpp>

#ifdef USING_QTHREADS
#include <qthread/qloop.h>
#endif

namespace mtgl {

template <typename graph, typename user_visitor>
class internal_adj_visitor {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;

  internal_adj_visitor(user_visitor uv, vertex_t v, vertex_id_map<graph>& vm,
                       graph& _g, int* sp = NULL, int msp = 0) :
    uservis(uv), vid_map(vm), src(v), subproblem(sp),
    my_subproblem(msp), g(_g) {}

  bool visit_test(vertex_t v)
  {
    int vid = get(vid_map, v);
    if (subproblem && (subproblem[vid] != my_subproblem)) return false;
    return uservis.visit_test(v);
  }

  void pre_visit(vertex_t v) { uservis.pre_visit(v); }
  void post_visit(vertex_t v) { uservis.post_visit(v); }

  void operator()(edge_t e)
  {
    vertex_t dest = other(e, src, g);
    int tid = get(vid_map, dest);
    if (subproblem && (subproblem[tid] != my_subproblem)) return;
    uservis(e, src, dest);
  }

  void set_data(vertex_t v) { src = v; }

private:
  vertex_t src;
  user_visitor uservis;
  vertex_id_map<graph>& vid_map;
  int* subproblem;
  int my_subproblem;
  graph& g;
};

template <typename graph, typename user_visitor>
class internal_adj_visitor2 {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;

  internal_adj_visitor2(user_visitor uv, vertex_t v,
                        vertex_id_map<graph>& vm, bool* sp, graph& _g) :
    uservis(uv), src(v), subproblem(sp), vid_map(vm), g(_g) {}

  bool visit_test(vertex_t v)
  {
    int vid = get(vid_map, v);
    if (subproblem && !subproblem[vid]) return false;
    return uservis.visit_test(v);
  }

  void pre_visit(vertex_t v) { uservis.pre_visit(v); }
  void post_visit(vertex_t v) { uservis.post_visit(v); }

  void operator()(edge_t e)
  {
    vertex_t dest = other(e, src, g);
    int tid = get(vid_map, dest);
    if (subproblem && !subproblem[tid]) return;
    uservis(e, src, dest);
  }

  void set_data(vertex_t v) { src = v; }

private:
  user_visitor uservis;
  vertex_t src;
  bool* subproblem;
  vertex_id_map<graph>& vid_map;
  graph& g;
};

template <typename graph, typename user_visitor>
class internal_adj_visitor3 {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;

  internal_adj_visitor3(user_visitor uv, vertex_t v, vertex_id_map<graph>& vm,
                        edge_id_map<graph>& em, bool* act_v, bool* act_e,
                        graph& _g) :
    uservis(uv), src(v), active_v(act_v),
    vid_map(vm), eid_map(em), active_e(act_e), g(_g) {}

  bool visit_test(vertex_t v)
  {
    int vid = get(vid_map, v);
    if (active_v && !active_v[vid]) return false;
    return uservis.visit_test(v);
  }

  void pre_visit(vertex_t v) { uservis.pre_visit(v); }
  void post_visit(vertex_t v) { uservis.post_visit(v); }

  void operator()(edge_t e)
  {
    int eid = get(eid_map, e);

    if (active_e && !active_e[eid])
    {
      vertex_t dest = other(e, src, g);
      int sid = get(vid_map, src);
      int tid = get(vid_map, dest);

      return;
    }

    vertex_t dest = other(e, src, g);
    uservis(e, src, dest);
  }

  void set_data(vertex_t v) { src = v; }

private:
  vertex_t src;
  user_visitor uservis;
  bool* active_v;
  bool* active_e;
  vertex_id_map<graph>& vid_map;
  edge_id_map<graph>&   eid_map;
  graph& g;
};

template <typename graph, typename user_visitor>
class internal_edge_visitor {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;

  internal_edge_visitor(user_visitor& uv) : uservis(uv) {}

  bool visit_test(edge_t e) { return uservis.visit_test(e); }
  void operator()(edge_t e) { uservis(e); }

private:
  user_visitor& uservis;
  bool* subproblem;
};

#ifdef USING_QTHREADS
template <typename graph, typename visitor>
struct va_s {
  graph& g;
  visitor& user_vis;
  vertex_id_map<graph>& vid_map;
  const int parcutoff;

  va_s(graph& g_i, visitor& uv, vertex_id_map<graph>& vm, const int pco) :
    g(g_i), user_vis(uv), vid_map(vm), parcutoff(pco) {}
};

template <typename graph, typename visitor>
struct va_index {
  graph& g;
  int* index;
  visitor& user_vis;
  vertex_id_map<graph>& vid_map;
  const int parcutoff;

  va_index(graph& g_i, visitor& uv, vertex_id_map<graph>& vm,
           int* ind, const int pco) :
    g(g_i), user_vis(uv), vid_map(vm), parcutoff(pco), index(ind) {}
};

template <typename graph, typename visitor>
void visit_adj_inner_ind(qthread_t* me, const size_t startat,
                         const size_t endat, void* arg)
{
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::vertex_iterator vertex_iterator;

  va_index<graph, visitor>* args = (va_index<graph, visitor>*)arg;
  vertex_iterator verts = vertices(args->g);

  for (size_t i = startat; i < endat; i++)
  {
    vertex_t v = verts[args->index[i]];
    internal_adj_visitor2<graph, visitor>
    ivis(args->user_vis, v, args->vid_map, (bool*) 0, args->g);

    if (!ivis.visit_test(v)) continue;

    ivis.pre_visit(v);
    visit_edges(args->g, v, ivis, args->parcutoff, false);

    ivis.post_visit(v);
  }
}

template <typename graph, typename visitor>
void visit_adj_inner(qthread_t* me, const size_t startat, const size_t endat,
                     void* arg)
{
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::vertex_iterator vertex_iterator;

  va_s<graph, visitor>* args = (va_s<graph, visitor>*)arg;
  vertex_iterator verts = vertices(args->g);

  for (size_t i = startat; i < endat; i++)
  {
    vertex_t v = verts[i];
    internal_adj_visitor2<graph, visitor>
    ivis(args->user_vis, v, args->vid_map, (bool*) 0, args->g);

    if (!ivis.visit_test(v)) continue;

    ivis.pre_visit(v);
    visit_edges(args->g, v, ivis, args->parcutoff, false);

    ivis.post_visit(v);
  }
}

#endif

template <typename graph, typename visitor>
void visit_adj(graph& g, visitor user_vis, int parcutoff = 5000)
{
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::vertex_iterator vertex_iterator;

  unsigned int order = num_vertices(g);
  vertex_id_map<graph> vid_map = get(_vertex_id_map, g);

#ifdef USING_QTHREADS
  va_s<graph, visitor> args(g, user_vis, vid_map, parcutoff);
  qt_loop_balance(0, order, visit_adj_inner<graph, visitor>, &args);
#else
  vertex_iterator verts = vertices(g);

  #pragma mta assert parallel
  #pragma mta loop future
  for (unsigned int i = 0; i < order; i++)
  {
    vertex_t v = verts[i];
    internal_adj_visitor2<graph, visitor>
    ivis(user_vis, v, vid_map, (bool*) 0, g);

    if (!ivis.visit_test(v)) continue;

    ivis.pre_visit(v);
    visit_edges(g, v, ivis, parcutoff, false);

    ivis.post_visit(v);
  }
#endif
}

template <typename graph, typename visitor>
void visit_adj_high_low(graph& g, visitor user_vis,
                        int*& indicesOfBigs, int& num_big,
                        int*& indicesOfSmalls, int& num_small,
                        int degreeThreshold = 5000)
{
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;
  typedef typename graph_traits<graph>::out_edge_iterator inc_edge_iter_t;
  typedef typename graph_traits<graph>::vertex_iterator vertex_iterator;

  unsigned int ord = num_vertices(g);
  vertex_id_map<graph> vid_map = get(_vertex_id_map, g);
  vertex_iterator verts = vertices(g);

#ifdef USING_QTHREADS
  for (int i = 0; i < num_big; i++)             // In serial.
  {
    va_s<graph, visitor> va(g, user_vis, vid_map, degreeThreshold);

    visit_adj_inner<graph, visitor>(qthread_self(),
                                    (size_t) indicesOfBigs[i],
                                    (size_t) indicesOfBigs[i], &va);
  }
#else
/*
  for (int i=0; i<num_big; i++)  // In serial.
  {
    vertex_t v = verts[indicesOfBigs[i]];

    internal_adj_visitor2<graph, visitor>
    ivis(user_vis, v, vid_map, (bool*) 0, g);

    ivis.pre_visit(v);
    visit_edges(g, v, ivis, degreeThreshold, false);
  }
*/

  mt_timer timer3;
  int issues, memrefs, concur, streams, traps;
  init_mta_counters(timer3, issues, memrefs, concur, streams);

  for (int i = 0; i < num_big; i++)
  {
    vertex_t v = verts[indicesOfBigs[i]];
    inc_edge_iter_t inc_edges = out_edges(v, g);
    int deg = out_degree(v, g);

    #pragma mta assert parallel
    for (int j = 0; j < deg; j++)
    {
      edge_t e = inc_edges[j];
      vertex_t w = other(e, v, g);
      user_vis.process_edge(e, v, w);
    }
  }

  sample_mta_counters(timer3, issues, memrefs, concur, streams);
  printf("---------------------------------------------\n");
  printf("visit_adj (large) : %d\n", num_big);
  printf("---------------------------------------------\n");
  print_mta_counters(timer3, num_edges(g), issues, memrefs, concur, streams);
#endif
#ifdef USING_QTHREADS
  va_index<graph, visitor> args(g, user_vis, vid_map,
                                indicesOfSmalls, degreeThreshold);
  qt_loop_balance(0, num_small, visit_adj_inner_ind<graph, visitor>, &args);
#else
  mt_timer timer4;
  init_mta_counters(timer4, issues, memrefs, concur, streams);

  #pragma mta assert parallel
  for (int i = 0; i < num_small; i++)
  {
    vertex_t v = verts[indicesOfSmalls[i]];
    inc_edge_iter_t inc_edges = out_edges(v, g);
    int deg = out_degree(v, g);

    for (int j = 0; j < deg; j++)
    {
      edge_t e = inc_edges[j];
      vertex_t w = other(e, v, g);
      user_vis.process_edge(e, v, w);
    }
  }

  sample_mta_counters(timer4, issues, memrefs, concur, streams);
  printf("---------------------------------------------\n");
  printf("visit_adj (small) : \n");
  printf("---------------------------------------------\n");
  print_mta_counters(timer4, num_edges(g), issues, memrefs, concur, streams);
#endif
}

template <typename graph, typename visitor>
void visit_adj_high_low(graph& g, visitor user_vis, int degreeThreshold = 5000)
{
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;
  typedef typename graph_traits<graph>::out_edge_iterator inc_edge_iter_t;
  typedef typename graph_traits<graph>::vertex_iterator vertex_iterator;

  unsigned int ord = num_vertices(g);
  int* indicesOfBigs = (int*) malloc(sizeof(int) * ord);
  int* indicesOfSmalls = (int*) malloc(sizeof(int) * ord);
  int nextBig = 0;
  int nextSmall = 0;
  vertex_id_map<graph> vid_map = get(_vertex_id_map, g);
  vertex_iterator verts = vertices(g);

  #pragma mta assert parallel
  for (int i = 0; i < ord; i++)
  {
    vertex_t v = verts[i];
    int id = vid_map[v];

    if (out_degree(v, g) > degreeThreshold)
    {
      int mine = mt_incr(nextBig, 1);
      indicesOfBigs[mine] = id;
    }
    else
    {
      int mine = mt_incr(nextSmall, 1);
      indicesOfSmalls[mine] = id;
    }
  }
#ifdef USING_QTHREADS
  for (int i = 0; i < nextBig; i++)         // In serial.
  {
    va_s<graph, visitor> va(g, user_vis, vid_map, degreeThreshold);
    visit_adj_inner<graph, visitor>(qthread_self(),
                                    (size_t) indicesOfBigs[i],
                                    (size_t) indicesOfBigs[i], &va);
  }
#else
/*
  for (int i=0; i<nextBig; i++)  // In serial.
  {
    vertex_t v = verts[indicesOfBigs[i]];

    internal_adj_visitor2<graph, visitor>
    ivis(user_vis, v, vid_map, (bool*) 0, g);

    ivis.pre_visit(v);
    visit_edges(g, v, ivis, degreeThreshold, false);
  }
*/

  for (int i = 0; i < nextBig; i++)
  {
    vertex_t v = verts[indicesOfBigs[i]];
    inc_edge_iter_t inc_edges = out_edges(v, g);
    int deg = out_degree(v, g);

    #pragma mta assert parallel
    for (int j = 0; j < deg; j++)
    {
      edge_t e = inc_edges[j];
      vertex_t w = other(e, v, g);
      user_vis.process_edge(e, v, w);
    }
  }
#endif
#ifdef USING_QTHREADS
  va_index<graph, visitor> args(g, user_vis, vid_map,
                                indicesOfSmalls, degreeThreshold);
  qt_loop_balance(0, nextSmall, visit_adj_inner_ind<graph, visitor>, &args);
#else

  #pragma mta assert parallel
  for (int i = 0; i < nextSmall; i++)
  {
    vertex_t v = verts[indicesOfSmalls[i]];
    inc_edge_iter_t inc_edges = out_edges(v, g);
    int deg = out_degree(v, g);

    for (int j = 0; j < deg; j++)
    {
      edge_t e = inc_edges[j];
      vertex_t w = other(e, v, g);
      user_vis.process_edge(e, v, w);
    }
  }
#endif

  free(indicesOfBigs);
  free(indicesOfSmalls);
}

template <typename graph, typename visitor>
void visit_adj(graph& g, visitor user_vis,
               int* subproblem, int my_subproblem = 0, int parcutoff = 20)
{
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;
  typedef typename graph_traits<graph>::vertex_iterator vertex_iterator;

  unsigned int order = num_vertices(g);
  vertex_id_map<graph> vid_map = get(_vertex_id_map, g);
  vertex_iterator verts = vertices(g);

  #pragma mta assert parallel
  #pragma mta loop future
  for (unsigned int i = 0; i < order; i++)
  {
    vertex_t v = verts[i];
    internal_adj_visitor<graph, visitor>
    ivis(user_vis, v, vid_map, g, subproblem, my_subproblem);

    if (!ivis.visit_test(v)) continue;

    ivis.pre_visit(v);
    visit_edges(g, v, ivis, parcutoff, false);

    ivis.post_visit(v);
  }
}

template <typename graph, typename visitor>
void visit_adj(graph& g, visitor user_vis,
               bool* subproblem, int parcutoff = 20)
{
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::vertex_iterator vertex_iterator;

  unsigned int order = num_vertices(g);
  vertex_id_map<graph> vid_map = get(_vertex_id_map, g);
  vertex_iterator verts = vertices(g);

  #pragma mta assert parallel
  #pragma mta loop future
  for (unsigned int i = 0; i < order; i++)
  {
    vertex_t v = verts[i];
    internal_adj_visitor2<graph, visitor>
    ivis(user_vis, v, vid_map, subproblem, g);

    if (!ivis.visit_test(v)) continue;

    ivis.pre_visit(v);
    visit_edges(g, v, ivis, parcutoff, false);

    ivis.post_visit(v);
  }
}

template <typename graph, typename visitor>
void visit_adj(graph& g, visitor user_vis,
               bool* active_v, bool* active_e, int parcutoff = 20)
{
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::vertex_iterator vertex_iterator;

  unsigned int order = num_vertices(g);
  vertex_id_map<graph> vid_map = get(_vertex_id_map, g);
  edge_id_map<graph> eid_map = get(_edge_id_map, g);
  vertex_iterator verts = vertices(g);

  #pragma mta assert parallel
  #pragma mta loop future
  for (unsigned int i = 0; i < order; i++)
  {
    vertex_t v = verts[i];
    internal_adj_visitor3<graph, visitor>
    ivis(user_vis, v, vid_map, eid_map, active_v, active_e, g);

    if (!ivis.visit_test(v)) continue;

    ivis.pre_visit(v);
    int vid = get(vid_map, v);
    visit_edges(g, v, ivis, parcutoff, false);

    ivis.post_visit(v);
  }
}

template <typename graph, typename visitor>
void visit_adj(graph& g, int i, visitor user_vis, int parcutoff = 20)
{
#ifdef ASSERT
  assert(i >= 0);
  assert(i < num_vertices(g));
#endif

  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;

  vertex_t v = get_vertex(i, g);
  vertex_id_map<graph> vid_map = get(_vertex_id_map, g);
  internal_adj_visitor<graph, visitor> ivis(user_vis, v, vid_map, g);

  if (!ivis.visit_test(v)) return;

  visit_edges(g, v, ivis, parcutoff, false);
}

template <class graph>
void gen_load_balance_info(
    graph& G,
    typename graph_traits<graph>::size_type*& prefix_counts,
    typename graph_traits<graph>::size_type*& started_nodes,
    typename graph_traits<graph>::size_type& num_threads,
    int chunk_size)
{
  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::vertex_iterator vertex_iterator;

  size_type ord = num_vertices(G);
  size_type size = num_edges(G);
  vertex_iterator verts = vertices(G);

  // Set the number of threads to be the number of chunks.

  prefix_counts = (size_type*) malloc(sizeof(size_type) * (ord + 1));
  size_type* deg = (size_type*) malloc(sizeof(size_type) * ord);

  for (size_type i = 0; i < ord; i++) deg[i] = out_degree(verts[i], G);

  // Calculate the prefix counts which is the number of adjacencies that occur
  // before a given vertex.
  size_type* pc = prefix_counts;
  pc[0] = 0;
  for (size_type i = 1; i <= ord; i++) pc[i] = pc[i - 1] + deg[i - 1];

  size_type total_deg = prefix_counts[ord];

  num_threads = total_deg / chunk_size;
  if (size % chunk_size > 0) num_threads++;

  size_type s_nodes_size = num_threads + 1;

  size_type* s_nodes = deg;
  for (size_type i = 0; i < s_nodes_size; i++) s_nodes[i] = 0;

  #pragma mta assert nodep
  for (size_type i = 0; i < ord; i++)
  {
    size_type starting_chunk = prefix_counts[i] / chunk_size;
    mt_incr(s_nodes[starting_chunk], 1);
  }

  size_type* accum = (size_type*) malloc(sizeof(size_type) * s_nodes_size);
  accum[0] = s_nodes[0];
  for (size_type i = 1; i < s_nodes_size; i++)
  {
    accum[i] = accum[i - 1] + s_nodes[i];
  }

  // At this point, accum[i] holds the number of nodes that will
  // have started by the end of thread i.

  started_nodes = accum;

#if 0
  printf("ord(b): %d\n", ord);
  for (size_type i = 0; i <= ord; i++)
  {
    printf("pc[%d]: %d\t", i, prefix_counts[i]);
  }

  printf("\n");
  for (size_type i = 0; i < s_nodes_size; i++)
  {
    printf("sn[%d]: %d\n", i, started_nodes[i]);
  }

  printf("\n");
  for (size_type i = 0; i < s_nodes_size; i++)
  {
    printf("s_n[%d]: %d\n", i, s_nodes[i]);
  }

  printf("\n");
  fflush(stdout);
#endif

  free(deg);
}

template <class graph>
void gen_load_balance_info(
    graph& G,
    typename graph_traits<graph>::vertex_descriptor* to_visit,
    typename graph_traits<graph>::size_type num_to_visit,
    typename graph_traits<graph>::size_type*& prefix_counts,
    typename graph_traits<graph>::size_type*& started_nodes,
    typename graph_traits<graph>::size_type& num_threads,
    int chunk_size)
{
  typedef typename graph_traits<graph>::size_type size_type;

  size_type ord = num_to_visit;

  prefix_counts = (size_type*) malloc(sizeof(size_type) * (ord + 1));
  size_type* deg = (size_type*) malloc(sizeof(size_type) * ord);

  #pragma mta assert nodep
  for (size_type i = 0; i < ord; i++) deg[i] = out_degree(to_visit[i], G);

  // Calculate the prefix counts which is the number of adjacencies that occur
  // before a given vertex.
  size_type* pc = prefix_counts;
  pc[0] = 0;
  for (size_type i = 1; i <= ord; i++) pc[i] = pc[i - 1] + deg[i - 1];

  size_type total_deg = pc[ord];

  free(deg);

  // Set the number of threads to be the number of chunks.
  num_threads = total_deg / chunk_size;
  if (total_deg % chunk_size > 0) num_threads++;

  size_type s_nodes_size = num_threads + 1;
  size_type* s_nodes = (size_type*) malloc(sizeof(size_type) * s_nodes_size);

  #pragma mta assert nodep
  for (size_type i = 0; i < s_nodes_size; i++) s_nodes[i] = 0;

  #pragma mta assert nodep
  for (size_type i = 0; i < ord; i++)
  {
    size_type starting_chunk = prefix_counts[i] / chunk_size;
    mt_incr(s_nodes[starting_chunk], 1);
  }

  size_type* accum = (size_type*) malloc(sizeof(size_type) * s_nodes_size);
  accum[0] = s_nodes[0];
  for (size_type i = 1; i < s_nodes_size; i++)
  {
    accum[i] = accum[i - 1] + s_nodes[i];
  }

  // At this point, accum[i] holds the number of nodes that will
  // have started by the end of thread i.

  started_nodes = accum;

#if 0
  printf("ord(b): %d\n", ord);
  for (size_type i = 0; i <= ord; i++)
  {
    printf("pc[%d]: %d\t", i, prefix_counts[i]);
  }

  printf("\n");
  for (size_type i = 0; i < s_nodes_size; i++)
  {
    printf("sn[%d]: %d\n", i, started_nodes[i]);
  }

  printf("\n");
  for (size_type i = 0; i < s_nodes_size; i++)
  {
    printf("s_n[%d]: %d\n", i, s_nodes[i]);
  }

  printf("\n");
  fflush(stdout);
#endif

  free(s_nodes);
}

#ifdef USING_QTHREADS
template <typename graph, typename visitor>
struct chunk_s {
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::size_type size_type;

  graph& g;
  visitor user_vis;
  size_type* prefix_counts;
  size_type* started_nodes;
  vertex_t* to_visit;
  int chunk_size;

  chunk_s(graph& gg, visitor vis, size_type* pc,
          size_type* sn, vertex_t* to_vis, int cs) :
    g(gg), user_vis(vis), prefix_counts(pc),
    started_nodes(sn), to_visit(to_vis), chunk_size(cs) {}
};

template <typename graph, typename visitor>
void visit_adj_chunked_inner(qthread_t* me, const size_t startat,
                             const size_t endat, void* arg)
{
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::adjacency_iterator adj_iter_t;
  typedef typename graph_traits<graph>::vertex_iterator vertex_iterator;

  chunk_s<graph, visitor>* args = (chunk_s<graph, visitor>*)arg;
  visitor my_vis = args->user_vis;
  graph& G = args->g;
  size_type* prefix_counts = args->prefix_counts;
  size_type* started_nodes = args->started_nodes;
  vertex_t* to_visit = args->to_visit;
  size_type chunk_size = args->chunk_size;

  vertex_iterator verts = vertices(G);

  for (size_t i = startat; i < endat; i++)
  {
    size_type count  = 0;
    size_type curr_n;
    size_type next_n = started_nodes[i];

    if (i == 0)
    {
      curr_n = 0;
    }
    else
    {
      size_type last_started_by_prev_thread = started_nodes[i - 1] - 1;
      size_type pc = prefix_counts[last_started_by_prev_thread];
      size_type his_processed_neighs = i * chunk_size - pc;
      size_type his_deg;

      if (to_visit)
      {
        his_deg = out_degree(to_visit[last_started_by_prev_thread], G);
      }
      else
      {
        his_deg = out_degree(verts[last_started_by_prev_thread], G);
      }

      if (his_deg > his_processed_neighs)
      {
        curr_n = last_started_by_prev_thread;
        // First node already started, but not finished.
      }
      else
      {
        curr_n = last_started_by_prev_thread + 1;
      }
    }

    for ( ; curr_n < next_n; curr_n++)
    {
      size_type my_processed_neighs = 0;

      if (prefix_counts[curr_n] < i * chunk_size)
      {
        // curr_n was started by the previous thread.
        my_processed_neighs = i * chunk_size - prefix_counts[curr_n];
      }

      vertex_t u;

      if (to_visit)
      {
        u = to_visit[curr_n];
      }
      else
      {
        u = verts[curr_n];
      }

      size_type deg = out_degree(u, G);
      adj_iter_t adj_verts = adjacent_vertices(u, G);
      size_type end_inner_iter = deg;

      if (count + deg - my_processed_neighs > chunk_size)
      {
        end_inner_iter = my_processed_neighs + chunk_size - count;
      }

      for (size_type j = my_processed_neighs; j < end_inner_iter; j++)
      {
        vertex_t v = adj_verts[j];
        my_vis(u, v);
        count++;
      }
    }

    my_vis.post_visit();
  }
}

template <typename graph, typename visitor>
void visit_adj(graph& G, visitor vis,
               typename graph_traits<graph>::size_type*& prefix_counts,
               typename graph_traits<graph>::size_type*& started_nodes,
               typename graph_traits<graph>::size_type& num_threads,
               int chunk_size = 50)
{
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::size_type size_type;

  size_type ord = num_vertices(G);

  if (started_nodes == 0)
  {
    if (prefix_counts != 0) free(prefix_counts);

    gen_load_balance_info(G, prefix_counts, started_nodes,
                          num_threads, chunk_size);
  }
  else if (prefix_counts == 0)
  {
    if (started_nodes != 0) free(started_nodes);

    gen_load_balance_info(G, prefix_counts, started_nodes,
                          num_threads, chunk_size);
  }

  chunk_s<graph, visitor> args(G, vis, prefix_counts,
                               started_nodes, (vertex_t*) 0, chunk_size);

  qt_loop_balance(0, num_threads,
                  visit_adj_chunked_inner<graph, visitor>, &args);
}

template <typename graph, typename visitor>
void visit_adj(graph& G,
               typename graph_traits<graph>::vertex_descriptor* to_visit,
               typename graph_traits<graph>::size_type num_to_visit,
               visitor vis,
               typename graph_traits<graph>::size_type*& prefix_counts,
               typename graph_traits<graph>::size_type*& started_nodes,
               typename graph_traits<graph>::size_type& num_threads,
               int chunk_size = 50)
{
  if (started_nodes == 0)
  {
    if (prefix_counts != 0) free(prefix_counts);

    gen_load_balance_info(G, to_visit, num_to_visit,
                          prefix_counts, started_nodes,
                          num_threads, chunk_size);
  }
  else if (prefix_counts == 0)
  {
    if (started_nodes != 0) free(started_nodes);

    gen_load_balance_info(G, to_visit, num_to_visit,
                          prefix_counts, started_nodes,
                          num_threads, chunk_size);
  }

  chunk_s<graph, visitor> args(G, vis, prefix_counts,
                               started_nodes, to_visit, chunk_size);

  qt_loop_balance(0, num_threads,
                  visit_adj_chunked_inner<graph, visitor>, &args);
}

#else // no qthreads

template <typename graph, typename visitor>
void visit_adj(graph& G, visitor& vis,
               typename graph_traits<graph>::size_type*& prefix_counts,
               typename graph_traits<graph>::size_type*& started_nodes,
               typename graph_traits<graph>::size_type& num_threads,
               typename graph_traits<graph>::size_type chunk_size = 256)
{
//  printf("Inside visit_adj().\n");

  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::adjacency_iterator adj_iter_t;
  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::vertex_iterator vertex_iterator;

//  mt_timer timer;
//  timer.start();

  if (started_nodes == 0)
  {
    if (prefix_counts != 0) free(prefix_counts);

    gen_load_balance_info(G, prefix_counts, started_nodes,
                          num_threads, chunk_size);
  }
  else if (prefix_counts == 0)
  {
    if (started_nodes != 0) free(started_nodes);

    gen_load_balance_info(G, prefix_counts, started_nodes,
                          num_threads, chunk_size);
  }

//  timer.stop();
//  printf("\n  Precalc Time: %f\n", timer.getElapsedSeconds());
//  timer.start();

  vertex_iterator verts = vertices(G);

  #pragma mta assert parallel
  #pragma mta interleave schedule
  for (size_type i = 0; i < num_threads; i++)
  {
    size_type* my_prefix_counts = prefix_counts;
    size_type* my_started_nodes = started_nodes;
    visitor my_vis = vis;
    size_type count = 0;
    size_type curr_n;
    size_type next_n = my_started_nodes[i];

    if (i == 0)
    {
      curr_n = 0;
    }
    else
    {
      size_type last_started_by_prev_thread = my_started_nodes[i - 1] - 1;
      size_type pc = my_prefix_counts[last_started_by_prev_thread];
      size_type his_processed_neighs = i * chunk_size - pc;
      size_type his_deg =
        out_degree(verts[last_started_by_prev_thread], G);

      if (his_deg > his_processed_neighs)
      {
        // First node already started, but not finished.
        curr_n = last_started_by_prev_thread;
      }
      else
      {
        curr_n = last_started_by_prev_thread + 1;
      }
    }

    for ( ; curr_n < next_n; curr_n++)
    {
      size_type my_processed_neighs = 0;
      if (my_prefix_counts[curr_n] < i * chunk_size)
      {
        // curr_n was started by the previous thread.
        my_processed_neighs = i * chunk_size - my_prefix_counts[curr_n];
      }

      vertex_t u = verts[curr_n];
      size_type deg = out_degree(u, G);

      adj_iter_t adj_verts = adjacent_vertices(u, G);
      size_type end_inner_iter = deg;
      if (count + deg - my_processed_neighs > chunk_size)
      {
        end_inner_iter = my_processed_neighs + chunk_size - count;
      }

      for (size_type j = my_processed_neighs; j < end_inner_iter; j++)
      {
        vertex_t v = adj_verts[j];
        my_vis(u, v);
        count++;
      }
    }

    my_vis.post_visit();
  }

//  timer.stop();
//  printf("Main Work Time: %f\n\n", timer.getElapsedSeconds());
}

template <typename graph, typename visitor>
void visit_adj(graph& G,
               typename graph_traits<graph>::vertex_descriptor* to_visit,
               typename graph_traits<graph>::size_type num_to_visit,
               visitor& vis,
               typename graph_traits<graph>::size_type*& prefix_counts,
               typename graph_traits<graph>::size_type*& started_nodes,
               typename graph_traits<graph>::size_type& num_threads,
               typename graph_traits<graph>::size_type chunk_size)
{
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::adjacency_iterator adj_iter_t;
  typedef typename graph_traits<graph>::size_type size_type;

  size_type ord = num_vertices(G);

  if (started_nodes == 0)
  {
    if (prefix_counts != 0) free(prefix_counts);

    #pragma mta trace "gen_load_bal"
    gen_load_balance_info(G, to_visit, num_to_visit, prefix_counts,
                          started_nodes, num_threads, chunk_size);

    #pragma mta trace "gen_load_bal done"
  }
  else if (prefix_counts == 0)
  {
    if (started_nodes != 0) free(started_nodes);

    gen_load_balance_info(G, to_visit, num_to_visit, prefix_counts,
                          started_nodes, num_threads, chunk_size);
  }

  #pragma mta trace "level"
  #pragma mta assert parallel
  #pragma mta interleave schedule
  for (size_type i = 0; i < num_threads; i++)
  {
    size_type* my_to_visit = to_visit;
    size_type* my_prefix_counts = prefix_counts;
    size_type* my_started_nodes = started_nodes;
    visitor my_vis = vis;
    size_type count  = 0;
    size_type curr_n;
    size_type next_n = my_started_nodes[i];

    if (i == 0)
    {
      curr_n = 0;
    }
    else
    {
      size_type last_started_by_prev_thread = my_started_nodes[i - 1] - 1;
      size_type pc = my_prefix_counts[last_started_by_prev_thread];
      size_type his_processed_neighs = i * chunk_size - pc;
      size_type his_deg =
        out_degree(my_to_visit[last_started_by_prev_thread], G);

      if (his_deg > his_processed_neighs)
      {
        // First node already started, but not finished.
        curr_n = last_started_by_prev_thread;
      }
      else
      {
        curr_n = last_started_by_prev_thread + 1;
      }
    }

    for ( ; curr_n < next_n; curr_n++)
    {
      size_type my_processed_neighs = 0;
      if (my_prefix_counts[curr_n] < i * chunk_size)
      {
        // curr_n was started by the previous thread
        my_processed_neighs = i * chunk_size - my_prefix_counts[curr_n];
      }

      vertex_t u = my_to_visit[curr_n];
      size_type deg = out_degree(u, G);

      adj_iter_t adj_verts = adjacent_vertices(u, G);
      size_type end_inner_iter = deg;
      if (count + deg - my_processed_neighs > chunk_size)
      {
        end_inner_iter = my_processed_neighs + chunk_size - count;
      }

      for (size_type j = my_processed_neighs; j < end_inner_iter; j++)
      {
        vertex_t v = adj_verts[j];
        my_vis(u, v);
        count++;
      }
    }

    my_vis.post_visit();
  }

  #pragma mta trace "level done"
}

#endif // USING_QTHREADS

template <typename graph, typename visitor>
void visit_adj_serial(
    graph& G,
    typename graph_traits<graph>::vertex_descriptor* to_visit,
    typename graph_traits<graph>::size_type num_to_visit,
    visitor vis)
{
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::adjacency_iterator adj_iter_t;
  typedef typename graph_traits<graph>::size_type size_type;

  //size_type ord = num_vertices(G);
  #pragma mta trace "level"

  for (size_type i = 0; i < num_to_visit; i++)
  {
    vertex_t u = to_visit[i];
    size_type deg = out_degree(u, G);
    adj_iter_t adj_verts = adjacent_vertices(u, G);

    for (size_type j = 0; j < deg; j++)
    {
      vertex_t v = adj_verts[j];
      vis(u, v);
    }

    vis.post_visit();
  }

  #pragma mta trace "level done"
}

}

#endif

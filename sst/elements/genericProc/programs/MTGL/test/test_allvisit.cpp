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
/*! \file test_allvisit.cpp

    \author Jon Berry (jberry@sandia.gov)

    \date 12/4/2007
*/
/****************************************************************************/

#include "mtgl_test.hpp"

#include <mtgl/util.hpp>
#include <mtgl/metrics.hpp>
#include <mtgl/psearch.hpp>
#include <mtgl/breadth_first_search.hpp>
#include <mtgl/visit_adj.hpp>
#include <mtgl/independent_set.hpp>
#include <mtgl/bfs.hpp>

using namespace mtgl;

// How do you do this right?
#ifdef __MTA__
  #define WORDSZ 64
#else
  #define WORDSZ 32
#endif

double get_freq()
{
#ifdef __MTA__
  double freq = mta_clock_freq();
#else
  double freq = 1000000;
#endif
  return freq;
}

template <typename graph>
int test_allvisit1(graph& g, int parcutoff = 20)
{
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  int order = num_vertices(g);
  int count = 0;

  mt_timer mttimer;
  mttimer.start();

#ifdef __MTA__
  mta_resume_event_logging();

  int issues = mta_get_task_counter(RT_ISSUES);
  int memrefs = mta_get_task_counter(RT_MEMREFS);
  int m_nops = mta_get_task_counter(RT_M_NOP);
  int a_nops = mta_get_task_counter(RT_M_NOP);
  int fp_ops = mta_get_task_counter(RT_FLOAT_TOTAL);
  int traps  = mta_get_task_counter(RT_TRAP);
#endif

  #pragma mta assert parallel
  #pragma mta loop future
  for (int i = 0; i < order; i++)
  {
    vertex* v = g.vertices[i];
    edge** edges = v->edges.get_data();
    int deg = v->degree();
    if (deg >= parcutoff)
    {
      #pragma mta assert parallel
      for (int j = 0; j < deg; j++)
      {
        edge* e = edges[j];
        vertex* v2 = e->other(v);
        bool v1_odd = (v->id << WORDSZ - 1) >> WORDSZ - 1;
        bool v2_odd = (v2->id << WORDSZ - 1) >> WORDSZ - 1;
        if (v1_odd && v2_odd) mt_incr(count, 1);
      }
    }
    else
    {
      for (int j = 0; j < deg; j++)
      {
        edge* e = edges[j];
        vertex* v2 = e->other(v);
        bool v1_odd = (v->id << WORDSZ - 1) >> WORDSZ - 1;
        bool v2_odd = (v2->id << WORDSZ - 1) >> WORDSZ - 1;
        if (v1_odd && v2_odd) mt_incr(count, 1);
      }
    }
  }

  mttimer.stop();
  double seconds = mttimer.getElapsedSeconds();

#ifdef __MTA__
  issues = mta_get_task_counter(RT_ISSUES) - issues;
  memrefs = mta_get_task_counter(RT_MEMREFS) - memrefs;
  m_nops = mta_get_task_counter(RT_M_NOP)  - m_nops;
  a_nops = mta_get_task_counter(RT_A_NOP)  - a_nops;
  //a_nops = mta_get_task_counter(RT_FLOAT_TOTAL) - fp_ops;
  traps  = mta_get_task_counter(RT_TRAP);

  int size = num_edges(g);

  printf("memref/edge: (%lf)\n", memrefs / (double) size);
  printf("issues/edge: (%lf)\n", issues / (double) size);
  printf("traps/edge: (%lf)\n", traps / (double) size);
  printf("traps: (%d\n", traps);
#endif

  printf("looping through all adjacency lists: %lf s\n", seconds);

  return count;
}

template <typename graph>
class slcounter {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;
  slcounter(vertex* v, int& ctr) : count(ctr), src(v) {}

  void operator()(edge* e)
  {
    vertex* v2 = e->other(src);
    bool v1_odd = (src->id << WORDSZ - 1) >> WORDSZ - 1;
    bool v2_odd = (v2->id << WORDSZ - 1) >> WORDSZ - 1;
    if (v1_odd && v2_odd) mt_incr(count, 1);
  }

  void set_data(vertex* v) { src = v; }

private:
  int& count;
  vertex* src;
};

template <typename graph>
int test_allvisit2(graph& g, int parcutoff = 20)
{
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  int order = num_vertices(g);
  int count = 0;

  mt_timer mttimer;
  mttimer.start();

#ifdef __MTA__
  mta_resume_event_logging();

  int issues = mta_get_task_counter(RT_ISSUES);
  int memrefs = mta_get_task_counter(RT_MEMREFS);
  int m_nops = mta_get_task_counter(RT_M_NOP);
  int a_nops = mta_get_task_counter(RT_M_NOP);
  int fp_ops = mta_get_task_counter(RT_FLOAT_TOTAL);
  int traps  = mta_get_task_counter(RT_TRAP);
#endif

  #pragma mta assert parallel
  #pragma mta loop future
  for (int i = 0; i < order; i++)
  {
    vertex* v = g.vertices[i];
    slcounter<graph> slc(v, count);
    int deg = v->degree();
    if (deg >= parcutoff)
    {
      v->unsafe_visitedges(slc, true, false);
    }
    else
    {
      v->unsafe_visitedges(slc, false, false);
    }
  }

  mttimer.stop();
  double seconds = mttimer.getElapsedSeconds();

#ifdef __MTA__
  issues = mta_get_task_counter(RT_ISSUES) - issues;
  memrefs = mta_get_task_counter(RT_MEMREFS) - memrefs;
  m_nops = mta_get_task_counter(RT_M_NOP)  - m_nops;
  a_nops = mta_get_task_counter(RT_A_NOP)  - a_nops;
  //a_nops = mta_get_task_counter(RT_FLOAT_TOTAL) - fp_ops;
  traps  = mta_get_task_counter(RT_TRAP);

  int size = num_edges(g);

  printf("memref/edge: (%lf)\n", memrefs / (double) size);
  printf("issues/edge: (%lf)\n", issues / (double) size);
  printf("traps/edge: (%lf)\n", traps / (double) size);
  printf("traps: (%d)\n", traps);
#endif

  printf("looping through vertices, applying visitor: %lf s\n", seconds);

  return count;
}

template <typename graph>
class self_loop_visitor : public default_psearch_visitor<graph> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  self_loop_visitor(int& ct) : count(ct) {}

  void pre_visit(vertex* v) { vid = v->id; }

  void tree_edge(edge* e, vertex* v)
  {
    vertex* v2 = e->other(v);
    bool v1_odd = (vid << WORDSZ - 1) >> WORDSZ - 1;
    bool v2_odd = (v2->id << WORDSZ - 1) >> WORDSZ - 1;
    if (v1_odd && v2_odd) mt_incr(count, 1);
  }

  void back_edge(edge* e, vertex* v) { tree_edge(e, v); }

private:
  int vid;
  int& count;
};

template <typename graph>
int test_allvisit4(graph& g, int parcut = 20)
{
  mt_timer mttimer;
  mttimer.start();

#ifdef __MTA__
  mta_resume_event_logging();

  int issues = mta_get_task_counter(RT_ISSUES);
  int memrefs = mta_get_task_counter(RT_MEMREFS);
  int m_nops = mta_get_task_counter(RT_M_NOP);
  int a_nops = mta_get_task_counter(RT_M_NOP);
  int fp_ops = mta_get_task_counter(RT_FLOAT_TOTAL);
  int traps  = mta_get_task_counter(RT_TRAP);
#endif

  int count = 0;
  self_loop_visitor<graph> slv(count);
  psearch_high_low<graph, self_loop_visitor<graph>, UNDIRECTED, 100>
  ps(g, slv);
  ps.run();

  mttimer.stop();
  double seconds = mttimer.getElapsedSeconds();

#ifdef __MTA__
  issues = mta_get_task_counter(RT_ISSUES) - issues;
  memrefs = mta_get_task_counter(RT_MEMREFS) - memrefs;
  m_nops = mta_get_task_counter(RT_M_NOP)  - m_nops;
  a_nops = mta_get_task_counter(RT_A_NOP)  - a_nops;
  //a_nops = mta_get_task_counter(RT_FLOAT_TOTAL) - fp_ops;
  traps  = mta_get_task_counter(RT_TRAP);

  int size = num_edges(g);

  printf("memref/edge: (%lf)\n", memrefs / (double) size);
  printf("issues/edge: (%lf)\n", issues / (double) size);
  printf("traps/edge: (%lf)\n", traps / (double) size);
  printf("traps: (%d)\n", traps);
#endif

  printf("psearch: %lf s\n", seconds);

  return count;
}

template <typename graph>
int test_allvisit5(graph& g, int parcut = 20)
{
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  int size = num_edges(g);
  int count = 0;

  mt_timer mttimer;
  mttimer.start();

#ifdef __MTA__
  mta_resume_event_logging();

  int issues = mta_get_task_counter(RT_ISSUES);
  int memrefs = mta_get_task_counter(RT_MEMREFS);
  int m_nops = mta_get_task_counter(RT_M_NOP);
  int a_nops = mta_get_task_counter(RT_M_NOP);
  int fp_ops = mta_get_task_counter(RT_FLOAT_TOTAL);
  int traps  = mta_get_task_counter(RT_TRAP);
#endif

  edge** edges = g.edges.get_data();
  #pragma mta assert parallel
  for (int i = 0; i < size; i++)
  {
    edge* e = edges[i];
    vertex* v1 = e->vert1;
    vertex* v2 = e->vert2;
    bool v1_odd = (v1->id << WORDSZ - 1) >> WORDSZ - 1;
    bool v2_odd = (v2->id << WORDSZ - 1) >> WORDSZ - 1;
    if (v1_odd && v2_odd) mt_incr(count, 1);
  }

  mttimer.stop();
  double seconds = mttimer.getElapsedSeconds();

#ifdef __MTA__
  issues = mta_get_task_counter(RT_ISSUES) - issues;
  memrefs = mta_get_task_counter(RT_MEMREFS) - memrefs;
  m_nops = mta_get_task_counter(RT_M_NOP)  - m_nops;
  a_nops = mta_get_task_counter(RT_A_NOP)  - a_nops;
  //a_nops = mta_get_task_counter(RT_FLOAT_TOTAL) - fp_ops;
  traps  = mta_get_task_counter(RT_TRAP);

  printf("memref/edge: (%lf)\n", memrefs / (double) size);
  printf("issues/edge: (%lf)\n", issues / (double) size);
  printf("traps/edge: (%lf)\n", traps / (double) size);
  printf("traps: (%d)\n", traps);
#endif

  printf("looping through edges: %lf s\n", seconds);

  return count;
}

template <typename graph>
class self_loop_edge_visitor {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  self_loop_edge_visitor(int& ct) : count(ct)  {}

  void visit(edge* e)
  {
    vertex* v1 = e->vert1;
    vertex* v2 = e->vert2;
    bool v1_odd = (v1->id << 31) >> 31;
    bool v2_odd = (v2->id << 31) >> 31;
    if (v1_odd && v2_odd) mt_incr(count, 1);
  }

private:
  int& count;
};

template <typename graph>
int test_allvisit6(graph& g)
{
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  int size = num_edges(g);
  int count = 0;

  mt_timer mttimer;
  mttimer.start();

#ifdef __MTA__
  mta_resume_event_logging();

  int issues = mta_get_task_counter(RT_ISSUES);
  int memrefs = mta_get_task_counter(RT_MEMREFS);
  int m_nops = mta_get_task_counter(RT_M_NOP);
  int a_nops = mta_get_task_counter(RT_M_NOP);
  int fp_ops = mta_get_task_counter(RT_FLOAT_TOTAL);
  int traps  = mta_get_task_counter(RT_TRAP);
#endif

  edge** edges = g.edges.get_data();
  #pragma mta assert parallel
  for (int i = 0; i < size; i++)
  {
    self_loop_edge_visitor<graph> slev(count);
    slev.visit(edges[i]);
  }

  mttimer.stop();
  double seconds = mttimer.getElapsedSeconds();

#ifdef __MTA__
  issues = mta_get_task_counter(RT_ISSUES) - issues;
  memrefs = mta_get_task_counter(RT_MEMREFS) - memrefs;
  m_nops = mta_get_task_counter(RT_M_NOP)  - m_nops;
  a_nops = mta_get_task_counter(RT_A_NOP)  - a_nops;
  //a_nops = mta_get_task_counter(RT_FLOAT_TOTAL) - fp_ops;
  traps  = mta_get_task_counter(RT_TRAP);

  printf("memref/edge: (%lf)\n", memrefs / (double) size);
  printf("issues/edge: (%lf)\n", issues / (double) size);
  printf("traps/edge: (%lf)\n", traps / (double) size);
  printf("traps: (%d)\n", traps);
#endif

  printf("looping through edges: %lf s\n", seconds);

  return count;
}


template <typename graph>
class bfs_self_loop_visitor : public default_bfs_visitor<graph> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  bfs_self_loop_visitor(int& ct) : count(ct) {}

  void tree_edge(edge* e, vertex* v, vertex* v2) const
  {
    bool v1_odd = (v->id << WORDSZ - 1) >> WORDSZ - 1;
    bool v2_odd = (v2->id << WORDSZ - 1) >> WORDSZ - 1;
    if (v1_odd && v2_odd) mt_incr(count, 1);
  }

  void back_edge(edge* e, vertex* v, vertex* v2) const { tree_edge(e, v, v2); }

private:
  int vid;
  int& count;
};

template <typename graph>
int test_allvisit7(graph& g)
{
  int order = num_vertices(g);
  int* search_color = (int*) malloc(sizeof(int) * order);

  #pragma mta assert nodep
  for (int i = 0; i < order; i++) search_color[i] = 0;

  mt_timer mttimer;
  mttimer.start();

#ifdef __MTA__
  mta_resume_event_logging();

  int issues = mta_get_task_counter(RT_ISSUES);
  int memrefs = mta_get_task_counter(RT_MEMREFS);
  int m_nops = mta_get_task_counter(RT_M_NOP);
  int a_nops = mta_get_task_counter(RT_A_NOP);
  int fp_ops = mta_get_task_counter(RT_FLOAT_TOTAL);
  int traps  = mta_get_task_counter(RT_TRAP);
#endif

  int count = 0;
  bfs_self_loop_visitor<graph> bfsv(count);
  breadth_first_search<graph, int*, bfs_self_loop_visitor<graph> >
  bfs(g, search_color, bfsv);
  bfs.run(0);

  mttimer.stop();
  double seconds = mttimer.getElapsedSeconds();

#ifdef __MTA__
  issues = mta_get_task_counter(RT_ISSUES) - issues;
  memrefs = mta_get_task_counter(RT_MEMREFS) - memrefs;
  m_nops = mta_get_task_counter(RT_M_NOP)  - m_nops;
  a_nops = mta_get_task_counter(RT_A_NOP)  - a_nops;
  //a_nops = mta_get_task_counter(RT_FLOAT_TOTAL) - fp_ops;
  traps  = mta_get_task_counter(RT_TRAP);

  int touched = 0;

  #pragma mta assert nodep
  for (int i = 0; i < order; i++)
  {
    if (search_color[i] != 0) mt_incr(touched, 1);
  }

  printf("touched: (%d)\n", touched);

  if (touched > 0)
  {
    printf("memref/touch: (%lf)\n", memrefs / (double) touched);
    printf("issues/touch: (%lf)\n", issues / (double) touched);
    printf("traps/touch: (%lf)\n", traps / (double) touched);
  }

  printf("traps: (%d)\n", traps);
#endif

  printf("psearch: %lf s\n", seconds);

  return count;
}

template <typename graph>
class bfs_slvisitor {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  bfs_slvisitor(int& ctr) : count(ctr) {}

  void pre_visit(vertex* v) {}

  void tree_edge(edge* e, vertex* src, vertex* v2)
  {
    bool v1_odd = (src->id << WORDSZ - 1) >> WORDSZ - 1;
    bool v2_odd = (v2->id << WORDSZ - 1) >> WORDSZ - 1;

    if (v1_odd && v2_odd) mt_incr(count, 1);
  }

  void back_edge(edge* e, vertex* src, vertex* v2) {}

private:
  int& count;
};

template <typename graph>
class bfs_slcounter {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  bfs_slcounter(int* _Q, int* sc, vertex* v, int& qind, int& ctr) :
    Q(_Q), color(sc), q_index(qind), src(v), count(ctr) {}

  void operator()(edge* e)
  {
    vertex* v2 = e->other(src);
    int v2color = mt_incr(color[v2->id], 1);

    if (v2color == 0)
    {
      bool v1_odd = (src->id << WORDSZ - 1) >> WORDSZ - 1;
      bool v2_odd = (v2->id << WORDSZ - 1) >> WORDSZ - 1;

      if (v1_odd && v2_odd) mt_incr(count, 1);

      int ind = mt_incr(q_index, 1);
      Q[ind] = v2->id;
    }
  }

  void set_data(vertex* v) { src = v; }

private:
  vertex* src;
  int& count;
  int& q_index;
  int* Q;
  int* color;
};

template <typename graph>
class user_counter {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  user_counter(int& ct) : count(ct) {}

  int operator()(edge* e, vertex* src, vertex* dest)
  {
    bool v1_odd = (src->id << WORDSZ - 1) >> WORDSZ - 1;
    bool v2_odd = (dest->id << WORDSZ - 1) >> WORDSZ - 1;

    if (v1_odd && v2_odd) mt_incr(count, 1);

    return 0;
  }

  int& count;
};

template <typename graph, typename user_visitor>
class temp_bfs_visitor {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  temp_bfs_visitor(user_visitor& uv, int* _Q, int* sc, vertex* v, int& qind) :
    uservis(uv), Q(_Q), color(sc), q_index(qind), src(v) {}

  void operator()(edge* e)
  {
    vertex* dest = e->other(src);
    int code = uservis(e, src, dest);

    if (code > 0)
    {
      int v2color = mt_incr(color[dest->id], 1);
      int ind = mt_incr(q_index, 1);
      Q[ind] = dest->id;
    }
    else if (code == 0)
    {
      int v2color = mt_incr(color[dest->id], 1);

      if (v2color == 0)
      {
        int ind = mt_incr(q_index, 1);
        Q[ind] = dest->id;
      }
    }
  }

  void set_data(vertex* v) { src = v; }

private:
  vertex* src;
  int& q_index;
  int* Q;
  int* color;
  user_visitor& uservis;
};

template <typename graph>
int test_allvisit8(graph& g, int parcutoff = 20)
{
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  int order = num_vertices(g);
  int count = 0;
  int* search_color = (int*) malloc(sizeof(int) * order);
  int* Q            = (int*) malloc(sizeof(int) * order);

  #pragma mta assert nodep
  for (int i = 0; i < order; i++) search_color[i] = Q[i] = 0;

  mt_timer mttimer;
  mttimer.start();

#ifdef __MTA__
  mta_resume_event_logging();

  int issues = mta_get_task_counter(RT_ISSUES);
  int memrefs = mta_get_task_counter(RT_MEMREFS);
  int m_nops = mta_get_task_counter(RT_M_NOP);
  int a_nops = mta_get_task_counter(RT_M_NOP);
  int fp_ops = mta_get_task_counter(RT_FLOAT_TOTAL);
  int traps  = mta_get_task_counter(RT_TRAP);
#endif

  mt_incr(search_color[0], 1);

  int start = 0;
  int end = 1;
  int q_index = 1;

  while ((end - start) != 0)
  {
    #pragma mta assert parallel
    #pragma mta loop future
    for (int i = start; i < end; i++)
    {
      vertex* v = g.vertices[Q[i]];
      bfs_slcounter<graph> slc(Q, search_color, v,
                               q_index, count);
      int deg = v->degree();
      if (deg >= parcutoff)
      {
        v->unsafe_visitedges(slc, true, false);
      }
      else
      {
        v->unsafe_visitedges(slc, false, false);
      }
    }

    start = end;
    end = q_index;
  }

  mttimer.stop();
  double seconds = mttimer.getElapsedSeconds();

#ifdef __MTA__
  issues = mta_get_task_counter(RT_ISSUES) - issues;
  memrefs = mta_get_task_counter(RT_MEMREFS) - memrefs;
  m_nops = mta_get_task_counter(RT_M_NOP)  - m_nops;
  a_nops = mta_get_task_counter(RT_A_NOP)  - a_nops;
  //a_nops = mta_get_task_counter(RT_FLOAT_TOTAL) - fp_ops;
  traps  = mta_get_task_counter(RT_TRAP);

  int size = num_edges(g);

  printf("memref/edge: (%lf)\n", memrefs / (double) size);
  printf("issues/edge: (%lf)\n", issues / (double) size);
  printf("traps/edge: (%lf)\n", traps / (double) size);
  printf("traps: (%d)\n", traps);
#endif

  printf("looping through vertices, applying visitor: %lf s\n", seconds);

  return count;
}

template <typename graph>
int test_allvisit9(graph& g, int parcutoff = 20)
{
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  int order = num_vertices(g);
  int count = 0;

  mt_timer mttimer;
  mttimer.start();

#ifdef __MTA__
  mta_resume_event_logging();

  int issues = mta_get_task_counter(RT_ISSUES);
  int memrefs = mta_get_task_counter(RT_MEMREFS);
  int m_nops = mta_get_task_counter(RT_M_NOP);
  int a_nops = mta_get_task_counter(RT_M_NOP);
  int fp_ops = mta_get_task_counter(RT_FLOAT_TOTAL);
  int traps  = mta_get_task_counter(RT_TRAP);
#endif

  bfs_slvisitor<graph> slc(count);
  bfs<graph, bfs_slvisitor<graph> > (g, slc, g.vertices[0]);

  mttimer.stop();
  double seconds = mttimer.getElapsedSeconds();

#ifdef __MTA__
  issues = mta_get_task_counter(RT_ISSUES) - issues;
  memrefs = mta_get_task_counter(RT_MEMREFS) - memrefs;
  m_nops = mta_get_task_counter(RT_M_NOP)  - m_nops;
  a_nops = mta_get_task_counter(RT_A_NOP)  - a_nops;
  //a_nops = mta_get_task_counter(RT_FLOAT_TOTAL) - fp_ops;
  traps  = mta_get_task_counter(RT_TRAP);

  int size = num_edges(g);

  printf("memref/edge: (%lf)\n", memrefs / (double) size);
  printf("issues/edge: (%lf)\n", issues / (double) size);
  printf("traps/edge: (%lf)\n", traps / (double) size);
  printf("traps: (%d)\n", traps);
#endif

  printf("looping through vertices, applying visitor: %lf s\n", seconds);

  return count;
}

template <typename graph>
class ev_slcounter {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  ev_slcounter(vertex* v, int& ctr) : src(v), count(ctr)  {}

  void operator()(edge* e)
  {
    vertex* v2 = e->other(this->src);
    bool v1_odd = (src->id << WORDSZ - 1) >> WORDSZ - 1;
    bool v2_odd = (v2->id << WORDSZ - 1) >> WORDSZ - 1;

    if (v1_odd && v2_odd) mt_incr(count, 1);
  }

  void set_data(vertex* v) { src = v; }

private:
  int& count;
  vertex* src;
};

template <typename graph, typename uservis>
void test_allvisit11(graph& g, uservis uv, int parcutoff = 20)
{
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  int order = num_vertices(g);
  int* search_color = (int*) malloc(sizeof(int) * order);
  int* Q            = (int*) malloc(sizeof(int) * order);

  #pragma mta assert nodep
  for (int i = 0; i < order; i++) search_color[i] = Q[i] = 0;

  mt_timer mttimer;
  mttimer.start();

#ifdef __MTA__
  mta_resume_event_logging();

  int issues = mta_get_task_counter(RT_ISSUES);
  int memrefs = mta_get_task_counter(RT_MEMREFS);
  int m_nops = mta_get_task_counter(RT_M_NOP);
  int a_nops = mta_get_task_counter(RT_M_NOP);
  int fp_ops = mta_get_task_counter(RT_FLOAT_TOTAL);
  int traps  = mta_get_task_counter(RT_TRAP);
#endif

  mt_incr(search_color[0], 1);

  int start = 0;
  int end = 1;
  int q_index = 1;

  while ((end - start) != 0)
  {
    #pragma mta assert parallel
    #pragma mta loop future
    for (int i = start; i < end; i++)
    {
      vertex* v = g.vertices[Q[i]];
      temp_bfs_visitor<graph, uservis >
      slc(uv, Q, search_color, v, q_index);

      int deg = v->degree();
      if (deg >= parcutoff)
      {
        v->unsafe_visitedges(slc, true, false);
      }
      else
      {
        v->unsafe_visitedges(slc, false, false);
      }
    }

    start = end;
    end = q_index;
  }

  mttimer.stop();
  double seconds = mttimer.getElapsedSeconds();

#ifdef __MTA__
  issues = mta_get_task_counter(RT_ISSUES) - issues;
  memrefs = mta_get_task_counter(RT_MEMREFS) - memrefs;
  m_nops = mta_get_task_counter(RT_M_NOP)  - m_nops;
  a_nops = mta_get_task_counter(RT_A_NOP)  - a_nops;
  //a_nops = mta_get_task_counter(RT_FLOAT_TOTAL) - fp_ops;
  traps  = mta_get_task_counter(RT_TRAP);

  int size = num_edges(g);

  printf("memref/edge: (%lf)\n", memrefs / (double) size);
  printf("issues/edge: (%lf)\n", issues / (double) size);
  printf("traps/edge: (%lf)\n", traps / (double) size);
  printf("traps: (%d)\n", traps);
#endif

  printf("looping through vertices, applying visitor: %lf s\n", seconds);
}

template <typename graph>
class adj_visitor {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  adj_visitor(int& ctr) : count(ctr) {}

  bool visit_test(vertex* v) { return true; }
  void pre_visit(vertex* v)  {}
  void post_visit(vertex* v)  {}

  void operator()(edge* e, vertex* src, vertex* dest)
  {
    bool v1_odd = (src->id << WORDSZ - 1) >> WORDSZ - 1;
    bool v2_odd = (dest->id << WORDSZ - 1) >> WORDSZ - 1;
    if (v1_odd && v2_odd) mt_incr(count, 1);
  }

private:
  int& count;
};

template <typename graph, typename visitor>
int test_allvisit12(graph& g, visitor user_vis, int parcutoff = 20)
{
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  int order = num_vertices(g);
  int count = 0;

  mt_timer mttimer;
  mttimer.start();

#ifdef __MTA__
  mta_resume_event_logging();

  int issues = mta_get_task_counter(RT_ISSUES);
  int memrefs = mta_get_task_counter(RT_MEMREFS);
  int m_nops = mta_get_task_counter(RT_M_NOP);
  int a_nops = mta_get_task_counter(RT_M_NOP);
  int fp_ops = mta_get_task_counter(RT_FLOAT_TOTAL);
  int traps  = mta_get_task_counter(RT_TRAP);
#endif

  #pragma mta assert parallel
  #pragma mta loop future
  for (int i = 0; i < order; i++)
  {
    vertex* v = g.vertices[i];
    internal_adj_visitor<graph, visitor> ivis(user_vis, v);

    int deg = v->degree();
    if (deg >= parcutoff)
    {
      v->unsafe_visitedges(ivis, true, false);
    }
    else
    {
      v->unsafe_visitedges(ivis, false, false);
    }
  }

  mttimer.stop();
  double seconds = mttimer.getElapsedSeconds();

#ifdef __MTA__
  issues = mta_get_task_counter(RT_ISSUES) - issues;
  memrefs = mta_get_task_counter(RT_MEMREFS) - memrefs;
  m_nops = mta_get_task_counter(RT_M_NOP)  - m_nops;
  a_nops = mta_get_task_counter(RT_A_NOP)  - a_nops;
  //a_nops = mta_get_task_counter(RT_FLOAT_TOTAL) - fp_ops;
  traps  = mta_get_task_counter(RT_TRAP);

  int size = num_edges(g);

  printf("memref/edge: (%lf)\n", memrefs / (double) size);
  printf("issues/edge: (%lf)\n", issues / (double) size);
  printf("traps/edge: (%lf)\n", traps / (double) size);
  printf("traps: (%d)\n", traps);
#endif

  printf("looping through vertices, applying visitor: %lf s\n", seconds);

  return count;
}

int main(int argc, char* argv[])
{
  typedef static_graph_adapter<directedS> Graph;
#ifdef __MTA__
  mta_suspend_event_logging();
#endif

  if (argc < 5)
  {
    fprintf(stderr,"usage: %s [-debug] "
           " --graph_type <dimacs|cray> "
           " --level <levels> --graph <Cray graph number> "
           " --filename <dimacs graph filename> "
           " [<0..15>]\n" , argv[0]);
    exit(1);
  }

  int alg = atoi(argv[1]);
  int parcut = 100;

  int alg = atoi(argv[1]);
  int parcut = 100;

  kernel_test_info kti;
  kti.process_args(argc, argv);
  Graph g;
  kti.gen_graph(g);

  int slcount;
  int count = 0;

  user_counter<Graph> uc(count);
  adj_visitor<Graph> av(count);

  switch (alg)
  {
    case 1:
      slcount = test_allvisit1(g, parcut);
      printf("slcount: %d\n", slcount);
      break;

    case 2:
      slcount = test_allvisit2(g, parcut);
      printf("slcount: %d\n", slcount);
      break;

    case 4:
      slcount = test_allvisit4(g, parcut);
      printf("slcount: %d\n", slcount);
      break;

    case 5:
      slcount = test_allvisit5(g);
      printf("slcount: %d\n", slcount);
      break;

    case 6:
      slcount = test_allvisit6(g);
      printf("slcount: %d\n", slcount);
      break;

    case 7:
      slcount = test_allvisit7(g);
      printf("slcount: %d\n", slcount);
      break;

    case 8:
      slcount = test_allvisit8(g, 20);
      printf("slcount: %d\n", slcount);
      break;

    case 9:
      slcount = test_allvisit9(g, 20);
      printf("slcount: %d\n", slcount);
      break;

    case 11:
      test_allvisit11(g, uc, 20);
      printf("slcount: %d\n", uc.count);
      break;

    case 12:
      mt_timer mttimer;
      mttimer.start();

#ifdef __MTA__
      mta_resume_event_logging();

      int issues = mta_get_task_counter(RT_ISSUES);
      int memrefs = mta_get_task_counter(RT_MEMREFS);
      int m_nops = mta_get_task_counter(RT_M_NOP);
      int a_nops = mta_get_task_counter(RT_M_NOP);
      int fp_ops = mta_get_task_counter(RT_FLOAT_TOTAL);
      int traps  = mta_get_task_counter(RT_TRAP);
#endif

      visit_adj<Graph, adj_visitor<Graph> >(g, av, 0, 0, parcut);

      printf("slcount: %d\n", count);

      mttimer.stop();
      double seconds = mttimer.getElapsedSeconds();

#ifdef __MTA__
      issues = mta_get_task_counter(RT_ISSUES) - issues;
      memrefs = mta_get_task_counter(RT_MEMREFS) - memrefs;
      m_nops = mta_get_task_counter(RT_M_NOP)  - m_nops;
      a_nops = mta_get_task_counter(RT_A_NOP)  - a_nops;
      //a_nops = mta_get_task_counter(RT_FLOAT_TOTAL) - fp_ops;
      traps  = mta_get_task_counter(RT_TRAP);

      int size = num_edges(g);

      printf("memref/edge: (%lf)\n", memrefs / (double) size);
      printf("issues/edge: (%lf)\n", issues / (double) size);
      printf("traps/edge: (%lf)\n", traps / (double) size);
      printf("traps: (%d)\n", traps);
#endif

      printf("looping through vertices, applying visitor: %lf s\n", seconds);
  }

  return 0;
}

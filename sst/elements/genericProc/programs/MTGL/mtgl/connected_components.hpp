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
/*! \file connected_components.hpp

    \brief This code contains the connectedComponents() (cc or "Kahan"),
           bullyComponents() (bu), Shiloach-Vishkin (sv), and
           checkComponents() (ch or "simple") connected components routines.
           These invoke searches using the visitor classes defined in
           components_visitor.hpp.

    \author Jon Berry (jberry@sandia.gov)

    \date 9/2005
*/
/****************************************************************************/

#ifndef MTGL_CONNECTED_COMPONENTS_HPP
#define MTGL_CONNECTED_COMPONENTS_HPP

#include <cstdio>
#include <climits>
#include <map>

#include <mtgl/util.hpp>
#include <mtgl/components_visitor.hpp>
#include <mtgl/copier.hpp>
#include <mtgl/bfs.hpp>
#include <mtgl/shiloach_vishkin.hpp>
#include <mtgl/xmt_hash_table.hpp>
#include <mtgl/xmt_hash_table_adapter.hpp>
#include <mtgl/edge_array_adapter.hpp>
#include <mtgl/mtgl_boost_property.hpp>

//#include "Random.h"
#include <sys/types.h>

#ifdef __MTA__
#include <sys/mta_task.h>
#include <machine/runtime.h>
#endif

#define SAMPLE_TYPE ((short) 4)

namespace mtgl {

#define GCC_CC_CHUNK 500

template <typename graph, typename component_map>
class find_biggest_connected_component {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::vertex_iterator vertex_iterator;

  find_biggest_connected_component(const graph& gg, const component_map cmps) :
    ran(0), biggest_id(0), biggest_size(0), g(gg), C(cmps) {}

  unsigned long run()
  {
    if (ran == 0)
    {
      ran = 1;
      unsigned long* rcount = (unsigned long*) calloc(num_vertices(g),
                                                      sizeof(unsigned long));

      vertex_id_map<graph> vid = get(_vertex_id_map, g);

      vertex_iterator verts = vertices(g);

      #pragma mta assert parallel
      for (size_type i = 0; i < num_vertices(g); i++)
      {
        vertex_t v = verts[i];
//        assert(get(vid, C[v]) >= 0);
        (void) mt_incr(rcount[get(vid, C[v])], 1);
      }

      #pragma mta assert parallel
      for (unsigned long i = 0; i < num_vertices(g); i++)
      {
        if (rcount[i] > biggest_size)
        {
          biggest_id = i;
          biggest_size = rcount[i];
        }
      }

      free(rcount);
    }

    return biggest_id;
  }

  unsigned long id()
  {
    if (ran)
    {
      return biggest_id;
    }
    else
    {
      return run();
    }
  }

  unsigned long size()
  {
    if (ran)
    {
      return biggest_size;
    }
    else
    {
      run();
      return biggest_size;
    }
  }

private:
  char ran;
  unsigned long biggest_id, biggest_size;
  const graph& g;
  const component_map C;
};

template <typename graph, typename component_map>
class count_connected_components {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::vertex_iterator vertex_iterator;

  count_connected_components(graph& gg, component_map cmps) : g(gg), C(cmps) {}

  unsigned long run()
  {
    unsigned long num_components = 0;
    unsigned long* rcount = (unsigned long*) calloc(num_vertices(g),
                                                    sizeof(unsigned long));

    vertex_id_map<graph> vid = get(_vertex_id_map, g);

    vertex_iterator verts = vertices(g);

    #pragma mta assert parallel
    for (size_type i = 0; i < num_vertices(g); i++)
    {
      vertex_t v = verts[i];
//      assert(get(vid, C[v]) >= 0);
      (void) mt_incr(rcount[get(vid, C[v])], 1);
    }

    unsigned long maxcomp = 0;

    #pragma mta assert parallel
    for (unsigned i = 0; i < num_vertices(g); i++)
    {
      if (rcount[i] > 0) (void) mt_incr(num_components, 1);

      if (rcount[i] > maxcomp) maxcomp = rcount[i];
    }

    printf("largest has %lu vertices\n", maxcomp);

    free(rcount);

    return num_components;
  }

private:
  graph& g;
  component_map C;
};

template <typename graph>
class default_filter {
public:
  default_filter() {}

  bool operator()(typename graph_traits<graph>::edge_descriptor e)
  {
    return true;
  }
};

#ifdef USING_QTHREADS
template <class graph_adapter, class component_map, class filter_t>
struct svcomp_s {
  graph_adapter& g;
  filter_t filter;
  int& graft;
  component_map D;

  svcomp_s(graph_adapter& gg, component_map comp, filter_t fil, int& cont) :
    g(gg), filter(fil), graft(cont), D(comp) {}
};

template <class graph_adapter, class component_map, class filter_t>
void sv_edge_loop(qthread_t* me, const size_t startat,
                  const size_t stopat, void* arg)
{
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_t;
  typedef typename graph_traits<graph_adapter>::edge_iterator edge_iter_t;
  typedef struct svcomp_s<graph_adapter, component_map, filter_t> sv_comp_t;

  graph_adapter& g(((sv_comp_t*) arg)->g);
  component_map D = (((sv_comp_t*) arg)->D);
  filter_t filter = (((sv_comp_t*) arg)->filter);
  int& graft = (((sv_comp_t*) arg)->graft);
  vertex_id_map<graph_adapter> vid = get(_vertex_id_map, g);

  edge_iter_t edgs = edges(g);

  for (size_t i = startat; i < stopat; i++)
  {
    edge_t e = edgs[i];

    if (filter(e))
    {
      vertex_t u = source(e, g);
      vertex_t v = target(e, g);

      if (get(vid, D[u]) < get(vid, D[v]) && D[v] == D[D[v]])
      {
        D[D[v]] = D[u];
        graft = 1;
      }

      if (get(vid, D[v]) < get(vid, D[u]) && D[u] == D[D[u]])
      {
        D[D[u]] = D[v];
        graft = 1;
      }
    }
  }
}

template <class graph_adapter, class component_map, class filter_t>
void sv_vertex_loop(qthread_t* me, const size_t startat,
                    const size_t stopat, void* arg)
{
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph_adapter>::vertex_iterator vertex_iter_t;
  typedef struct svcomp_s<graph_adapter, component_map, filter_t> sv_comp_t;

  graph_adapter& g(((sv_comp_t*) arg)->g);
  component_map D = (((sv_comp_t*) arg)->D);

  vertex_iter_t verts = vertices(g);

  for (size_t i = startat; i < stopat; i++)
  {
    vertex_t v = verts[i];

    while (D[v] != D[D[v]])
    {
      D[v] = D[D[v]];
    }
  }
}

#endif

/*! \brief A simple implementation of the Shiloach-Vishkin connected
           components algorithm.  This code scales through about 10 MTA
           processors, then scaling ends due to hot-spotting.  Before 10
           processors, it is faster then the Kahan and bully algorithms;
           beyond that, it is slower.

    The algorithm itself is described well in JaJa's "Introduction to
    Parallel Algorithms."
*/
template <typename graph, typename component_map,
          typename filter_t = default_filter<graph> >
class connected_components_sv {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;
  typedef typename graph_traits<graph>::edge_iterator edge_iter_t;
  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::vertex_iterator vertex_iterator;

  connected_components_sv(graph& gg, component_map res,
                          filter_t vis = default_filter<graph>()) :
    g(gg), filter(vis), D(res) {}

  void run()
  {
#ifdef USING_QTHREADS
    run_with_qthreads();
#else
    run_without_qthreads();
#endif
  }

  void run_without_qthreads()
  {
    size_type n = num_vertices(g);
    int graft = 1;
    edge_iter_t edgs = edges(g);
    vertex_id_map<graph> vid = get(_vertex_id_map, g);

    vertex_iterator verts = vertices(g);

    while (graft)
    {
      graft = 0;
      size_type sz = num_edges(g);

      #pragma mta assert parallel
      for (size_type i = 0; i < sz; i++)
      {
        #pragma mta trace "sv inner loop"
        edge_t e = edgs[i];

        if (filter(e))
        {
          vertex_t u = source(e, g);
          vertex_t v = target(e, g);

          if (get(vid, D[u]) < get(vid, D[v]) && D[v] == D[D[v]])
          {
            D[D[v]] = D[u];
            graft = 1;
          }

          if (get(vid, D[v]) < get(vid, D[u]) && D[u] == D[D[u]])
          {
            D[D[u]] = D[v];
            graft = 1;
          }
        }
      }

      #pragma mta assert parallel
      for (size_type i = 0; i < n; i++)
      {
        vertex_t v = verts[i];

        while (D[v] != D[D[v]])
        {
          D[v] = D[D[v]];
        }
      }
    }
  }

#ifdef USING_QTHREADS
  void run_with_qthreads()
  {
    typedef struct svcomp_s<graph, component_map, filter_t> sv_comp_t;

    int n = num_vertices(g);
    int m = num_edges(g);
    int graft = 1;
    sv_comp_t sv_comp_arg(g, D, filter, graft);

    while (graft)
    {
      graft = 0;

      qt_loop_balance(0, m, sv_edge_loop<graph, component_map, filter_t>,
                      &sv_comp_arg);

      qt_loop_balance(0, n, sv_vertex_loop<graph, component_map, filter_t>,
                      &sv_comp_arg);
    }
  }

#endif // USING_QTHREADS

private:
  graph& g;
  filter_t filter;
  component_map D;

  /*! \fn connected_components_sv(graph *g, int *result)
      \brief This constructor takes a graph and an array in which to write
             the component numbers and saves pointers to these.
      \param g A pointer to a graph.
      \param result A pointer to the array of component numbers.
  */
  /*! \fn run()
      \brief Invokes the algorithm.  In augmented versions, this will
             take a subproblem mask and the current subproblem.  It
             returns the number of connected components.
  */
};

template <typename graph_t, typename component_map,
          typename filter_t = default_filter<graph_t> >
class connected_components_foo_sv {
public:
  typedef typename graph_traits<graph_t>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph_t>::size_type size_type;

  connected_components_foo_sv(graph_t& gg, component_map res,
                              filter_t vis = default_filter<graph_t>()) :
    g(gg), filter(vis), D(res) {}

  connected_components_foo_sv() {}

#pragma mta no inline
  void run()
  {
#ifdef USING_QTHREADS
    run_with_qthreads();
#else
    run_without_qthreads();
#endif
  }

#pragma mta no inline
  void run_without_qthreads()
  {
    size_type n = num_vertices(g);
    size_type m = num_edges(g);
    int graft = 1;

    while (graft)
    {
      graft = 0;
      vertex_t* end_points = g.get_end_points();  // Fixed?
      size_type* C = D.get_data();  // Cheating! need to accomodate XMT.

      #pragma mta assert nodep
      #pragma mta assert noalias *C
      for (size_type u = 0; u < n; u++)
      {
        size_type begin = g[u];
        size_type end = g[u + 1];

        #pragma mta assert nodep
        #pragma mta assert noalias *C
        for (size_type j = begin; j < end; j++)
        {
          vertex_t v = end_points[j];

          if (C[u] < C[v] && C[v] == C[C[v]])
          {
            C[C[v]] = C[u];
            graft += 1;
          }

          if (C[v] < C[u] && C[u] == C[C[u]])
          {
            C[C[u]] = C[v];
            graft += 1;
          }
        }
      }

      #pragma mta assert noalias *C
      #pragma mta assert nodep
      for (size_type v = 0; v < n; v++)
      {
        while (C[v] != C[C[v]])
        {
          C[v] = C[C[v]];
        }
      }
    }
  }

#ifdef USING_QTHREADS
  void run_with_qthreads()
  {
    typedef struct svcomp_s<graph_t, component_map, filter_t> sv_comp_t;

    size_type n = num_vertices(g);
    size_type m = num_edges(g);
    int graft = 1;
    sv_comp_t sv_comp_arg(g, D, filter, graft);

    while (graft)
    {
      graft = 0;

      qt_loop_balance(0, m, sv_edge_loop<graph_t, component_map, filter_t>,
                      &sv_comp_arg);

      qt_loop_balance(0, n, sv_vertex_loop<graph_t, component_map, filter_t>,
                      &sv_comp_arg);
    }
  }

#endif // USING_QTHREADS

private:
  graph_t& g;
  filter_t filter;
  component_map D;

  /*! \fn connected_components_sv(graph *g, int *result)
      \brief This constructor takes a graph and an array in which to write
             the component numbers and saves pointers to these.
      \param g A pointer to a graph.
      \param result A pointer to the array of component numbers.
  */
  /*! \fn run()
      \brief Invokes the algorithm.  In augmented versions, this will
             take a subproblem mask and the current subproblem.  It
             returns the number of connected components.
  */

};

/*! \brief Kahan's three-phase connected components algorithm.  This is
           the most general of the connected components algorithms in this
           file.  The code scales through 40 processors on most inputs,
           exceptions being extremely pathological examples like a giant
           star graph.  Kahan's algorithm will scale even for strange
           inputs like very long paths of degree-2 vertices.

    The algorithm is described in "Software and Algorithms for graph Queries
    on Multithreaded Architectures." (Berry, Hendrickson, Kahan, Konecny,
    Cray User Group, 2006).
*/
template <typename graph, class inttype>
class connected_components_kahan {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge;
  typedef typename graph_traits<graph>::vertex_iterator vertex_iterator;
  typedef xmt_hash_table<u_int64_t, pair<u_int64_t, u_int64_t> > edgehash_table;

  connected_components_kahan(graph& gg, inttype* res, inttype hd = 0) :
    g(gg), result(res), high_degree(hd) {}

  int run()
  {
    int ticks;
    double freq, seconds;
    mt_timer timer;
    int concur, streams, issues, traps, memrefs;
    init_mta_counters(timer, issues, memrefs, concur, streams);
    edgehash_table small_graph(num_edges(g) / 2);

    inttype* searchColor = (inttype*) malloc(num_vertices(g) * sizeof(inttype));
    inttype* leader = (inttype*) malloc(num_vertices(g) * sizeof(inttype));

    #pragma mta assert parallel
    for (inttype i = 0; i < num_vertices(g); i++)
    {
      searchColor[i] = 0;
      leader[i]      = -1;
      mt_purge(result[i]);
    }

    //
    // Phase I: multiple asynchronous searches
    //

    inttype searches = num_vertices(g);
    inttype count = 0, leader_count = 0, edge_count;
    vertex_id_map<graph> vid_map = get(_vertex_id_map, g);
    edge_id_map<graph> eid_map = get(_edge_id_map, g);

    cc_psearch_visitor<graph, inttype*, inttype*, edgehash_table, inttype>
      ccpsearch_vis(num_vertices(g), leader, result, SAMPLE_TYPE, small_graph,
                    leader_count, vid_map, eid_map, g);

    psearch_high_low<graph, cc_psearch_visitor<graph, inttype*, inttype*,
                                               edgehash_table, inttype> >
      ps(g, ccpsearch_vis, high_degree);
    ps.run();

    //sample_mta_counters(timer, issues, memrefs, concur, streams);
    //printf("STATS FOR PHASE I \n");
    //print_mta_counters(timer, num_edges(g), issues,memrefs,concur, streams);

    //printf("THERE WERE %d leaders\n", leader_count);

    //
    // Phase I done
    //

    //init_mta_counters(timer, issues, memrefs, concur, streams);
    typedef xmt_hash_table_adapter::vertex_descriptor hvertex_t;
    typedef xmt_hash_table_adapter::vertex_iterator vertex_iterator_xht;
    u_int64_t* leaderIndices =
      (u_int64_t*) malloc(num_vertices(g) * sizeof(u_int64_t));

    std::map<hvertex_t, hvertex_t> sv_result;

    #pragma mta assert parallel
    for (inttype i = 0; i < num_vertices(g); i++)
    {
      if (leader[i] >= 0) leaderIndices[leader[i]] = i;
    }

    //
    // Preparing for Phase II; this uses an older version of
    // Shiloach-Vishkin; should adapt to use the class
    // shiloach_vishkin
    //

    //printf("leader_count: %d\n", leader_count);
    xmt_hash_table_adapter ga(&small_graph, leaderIndices, leader_count);
    print_edges(ga);
    init_identity_vert_map(ga, sv_result);

    vertex_id_map<xmt_hash_table_adapter> hvid_map = get(_vertex_id_map, ga);

    vertex_iterator_xht ga_verts = vertices(ga);

    for (xmt_hash_table_adapter::size_type i = 0; i < num_vertices(ga); i++)
    {
      printf("v[%lu]: %lu\n", (long unsigned) i,
             (long unsigned) get(hvid_map, ga_verts[i]));
    }

    //printf("about to call sv\n");
    //init_mta_counters(timer, issues, memrefs, concur, streams);
    typedef map_property_map<xmt_hash_table_adapter::vertex_descriptor,
                             vertex_id_map<xmt_hash_table_adapter> >
            component_map;

    component_map components(sv_result, hvid_map);
    connected_components_sv<xmt_hash_table_adapter, component_map>
      sv(ga, components);
    sv.run();

    //sample_mta_counters(timer, issues, memrefs, concur, streams);
    //printf("STATS FOR SV \n");
    //print_mta_counters(timer, num_edges(g), issues,memrefs,concur, streams);

    //
    // Phase II done
    //

    unsigned order = num_vertices(g);

    //init_mta_counters(timer, issues, memrefs, concur, streams);

    #pragma mta assert nodep
    for (inttype i = 0; i < order; i++) searchColor[i] = 0;

    #pragma mta assert parallel
    for (inttype i = 0; i < leader_count; i++)
    {
      result[leaderIndices[i]] = (int) get(hvid_map, sv_result[i]);
    }

    //sample_mta_counters(timer, issues, memrefs, concur, streams);
    //printf("STATS FOR INIT SCOLOR \n");
    //print_mta_counters(timer, num_edges(g), issues,memrefs,concur, streams);
    //init_mta_counters(timer, issues, memrefs, concur, streams);

    //
    // Phase III
    //

    post_psearch_visitor<graph, inttype*> postpsearch_vis(result, vid_map, g);
    psearch_high_low<graph, post_psearch_visitor<graph, inttype*> >
      psIII(g, postpsearch_vis, high_degree);
    psIII.run(leaderIndices, leader_count);

/*
    vertex_iterator g_verts = vertices(g);

    #pragma mta assert parallel
    #pragma mta loop future
    for (inttype i=0; i<leader_count; i++)
    {
      result[leaderIndices[i]] = (int) sv_result[i];
      psearch<graph, inttype*, post_psearch_visitor<graph,inttype*> >
        psrch(g, searchColor, postpsearch_vis);
      psrch.run(g_verts[leaderIndices[i]]);
    }
*/

//sample_mta_counters(timer, issues, memrefs, concur, streams);
//printf("STATS FOR PHASE III \n");
//print_mta_counters(timer, num_edges(g), issues,memrefs,concur, streams);

    //
    // Phase III done
    //

    sample_mta_counters(timer, issues, memrefs, concur, streams);
    printf("STATS FOR KAHAN \n");
    print_mta_counters(timer, num_edges(g), issues, memrefs, concur, streams);

    // ******************************
    //
    //  Component array filled; counting components.
    //
    unsigned long num_components = 0;
    unsigned long* rcount = (unsigned long*) calloc(num_vertices(g),
                                                    sizeof(unsigned long));

    #pragma mta assert parallel
    for (unsigned i = 0; i < num_vertices(g); i++)
    {
      (void) mt_incr(rcount[result[i]], 1);
    }

    #pragma mta assert parallel
    for (unsigned i = 0; i < num_vertices(g); i++)
    {
      if (rcount[i] > 0) (void) mt_incr(num_components, 1);
    }

    printf("there are %lu connected components\n", num_components);

    free(searchColor);
    free(leader);
    free(leaderIndices);
    free(rcount);

    return 0;                   // Need number of ticks.
  }

private:
  graph& g;
  inttype* result;
  inttype high_degree;

  /*! \fn connected_components_kahan(graph *g, int *result)
      \brief This constructor takes a graph and an array in which to write
             the component numbers and saves pointers to these.
      \param g A pointer to a graph.
      \param result A pointer to the array of component numbers.
  */
  /*! \fn run()
      \brief Invokes the algorithm.  In augmented versions, this will
             take a subproblem mask and the current subproblem.  It
             returns the number of connected components.
  */
};

/*! \brief The "bully algorithm" for connected components.  This algorithm
           is the fastest to date for the scale-free graph instances on
           which the MTGL has been tested.  It scales through 40 MTA
           processors.  However, we expect no speedup for pathological
           graphs like long paths, on which Kahan's algorithm still scales.

    The algorithm is described in "Software and Algorithms for graph Queries
    on Multithreaded Architectures." (Berry, Hendrickson, Kahan, Konecny,
    Cray User Group, 2006).
*/
template <typename graph>
class connected_components_bully {
public:
  typedef typename graph_traits<graph>::edge_descriptor edge;

  connected_components_bully(graph& gg, int* res, int hd = 0) :
    g(gg), result(res), high_degree(hd) {}

  int run()
  {
#ifdef __MTA__
    int issues = mta_get_task_counter(RT_ISSUES);
    int memrefs = mta_get_task_counter(RT_MEMREFS);
    int m_nops = mta_get_task_counter(RT_M_NOP);
    int a_nops = mta_get_task_counter(RT_M_NOP);
    int fp_ops = mta_get_task_counter(RT_FLOAT_TOTAL);
    mta_resume_event_logging();
#endif

    mt_timer mttimer;
    mttimer.start();
    vertex_id_map<graph> vid_map = get(_vertex_id_map, g);

    #pragma mta assert parallel
    for (unsigned int i = 0; i < num_vertices(g); i++) result[i] = i;

    // *******************************************************
    //  Bully search psearch
    // *******************************************************
    bully_comp_visitor<graph, int*> bullypsearch_vis(result, vid_map, g);
    psearch_high_low<graph, bully_comp_visitor<graph, int*>,
                     NO_FILTER, UNDIRECTED, 100> psrch(g, bullypsearch_vis,
                                                       high_degree);
    psrch.run();

/*
    printf("\n\nRESULTS IN BULL NON BFS\n");
    for (int i = 0; i < num_vertices(g); i ++)
    {
      printf("%d\t%d\n", i, result[i]);
    }
    printf("\n\n");
*/

    // *******************************************************
    //  Bully search done
    // *******************************************************

#ifdef __MTA__
    issues = mta_get_task_counter(RT_ISSUES) - issues;
    memrefs = mta_get_task_counter(RT_MEMREFS) - memrefs;
    m_nops = mta_get_task_counter(RT_M_NOP)  - m_nops;
    a_nops = mta_get_task_counter(RT_A_NOP)  - a_nops;
    a_nops = mta_get_task_counter(RT_FLOAT_TOTAL) - fp_ops;
    mta_suspend_event_logging();
#endif

    mttimer.stop();
    long postticks = mttimer.getElapsedTicks();

    //
    // Result filled in; now counting components;
    // this should be encapsulated into its own class.
    //

    unsigned int num_components = 0;
    unsigned* rcount = (unsigned*) calloc(num_vertices(g), sizeof(unsigned));

    #pragma mta assert parallel
    for (unsigned i = 0; i < num_vertices(g); i++)
    {
      assert(result[i] >= 0);
      (void) mt_incr<unsigned>(rcount[result[i]], 1);
    }

    #pragma mta assert parallel
    for (unsigned i = 0; i < num_vertices(g); i++)
    {
      if (rcount[i] > 0) (void) mt_incr<unsigned>(num_components, 1);
    }

    printf("there are %d connected components\n", num_components);

#ifdef __MTA__
    printf("BULLY memrefs/edge: %lf\n",  memrefs / (double) num_edges(g));
    static int total_issues, total_memrefs, total_fp_ops, total_int;
    total_issues += issues;
    total_memrefs += memrefs;
    total_fp_ops += fp_ops;
    total_int += (issues - a_nops - m_nops);
    printf("BU issues: %d, memrefs: %d, fp_ops: %d, ints+muls: %d"
           ",memratio:%6.2f\n",
           total_issues, total_memrefs, total_fp_ops, total_int,
           total_memrefs / (double) total_issues);
#endif

    free(rcount);

    return postticks;
  }

private:
  graph& g;
  int* result;
  int high_degree;

  /*! \fn connected_components_bully(graph *g, int *result)
      \brief This constructor takes a graph and an array in which to write
             the component numbers and saves pointers to these.
      \param g A pointer to a graph.
      \param result A pointer to the array of component numbers.
  */
  /*! \fn run()
      \brief Invokes the algorithm.  In augmented versions, this will
             take a subproblem mask and the current subproblem. It
             returns the number of connected components.
  */
};

// To test out the bfs_mtgl class to see how it performs.
/*
template <typename graph>
class connected_components_bully_bfs {
public:
  typedef typename graph_traits<graph>::edge_descriptor edge;
  typedef typename graph_traits<graph>::vertex_iterator vertex_iterator;

  connected_components_bully_bfs(graph& gg, int* res, int hd = 0) :
    g(gg), result(res), high_degree(hd) {}

  int run()
  {
#ifdef __MTA__
    int issues = mta_get_task_counter(RT_ISSUES);
    int memrefs = mta_get_task_counter(RT_MEMREFS);
    int m_nops = mta_get_task_counter(RT_M_NOP);
    int a_nops = mta_get_task_counter(RT_M_NOP);
    int fp_ops = mta_get_task_counter(RT_FLOAT_TOTAL);
    mta_resume_event_logging();
#endif

    mt_timer mttimer;
    mttimer.start();

    vertex_id_map<graph> vid_map = get(_vertex_id_map, g);

    #pragma mta assert parallel
    for (int i = 0; i < num_vertices(g); i++) result[i] = i;

    // *******************************************************
    //  Bully search bfs
    // *******************************************************
    bully_comp_visitor_bfs<graph, int*> bullybfs_vis(result, vid_map, g);
    int* colormap = new int[num_vertices(g)];

    #pragma mta assert parallel
    for (int i = 0; i < num_vertices(g); i++) colormap[i] = 0;

    //TODO: Look at psearch bully.
    breadth_first_search_mtgl<graph, int*, bully_comp_visitor_bfs<graph, int*> >
      bfs_mtgl(g, colormap, bullybfs_vis);

    vertex_iterator verts = vertices(g);

    // Maybe make this parallel?
    for (int i = 0; i < num_vertices(g); i++)
    {
      if (colormap[i] == 0) bfs_mtgl.run(verts[i], true);
    }

    // *******************************************************
    //  Bully search done
    // *******************************************************

#ifdef __MTA__
    issues = mta_get_task_counter(RT_ISSUES) - issues;
    memrefs = mta_get_task_counter(RT_MEMREFS) - memrefs;
    m_nops = mta_get_task_counter(RT_M_NOP)  - m_nops;
    a_nops = mta_get_task_counter(RT_A_NOP)  - a_nops;
    a_nops = mta_get_task_counter(RT_FLOAT_TOTAL) - fp_ops;
    mta_suspend_event_logging();
#endif
    mttimer.stop();
    long postticks = mttimer.getElapsedTicks();

    //
    // Result filled in; now counting components;
    // this should be encapsulated into its own class.
    //

    unsigned int num_components = 0;
    unsigned* rcount = (unsigned*) calloc(num_vertices(g), sizeof(unsigned));

    #pragma mta assert parallel
    for (unsigned i = 0; i < num_vertices(g); i++)
    {
      assert(result[i] >= 0);
      (void) mt_incr<unsigned>(rcount[result[i]], 1);
    }

    #pragma mta assert parallel
    for (unsigned i = 0; i < num_vertices(g); i++)
    {
      if (rcount[i] > 0) (void) mt_incr<unsigned>(num_components, 1);
    }

    printf("there are %d connected components\n", num_components);

#ifdef __MTA__
    printf("BULLY memrefs/edge: %lf\n",  memrefs / (double) num_edges(g));
    static int total_issues, total_memrefs, total_fp_ops, total_int;
    total_issues += issues;
    total_memrefs += memrefs;
    total_fp_ops += fp_ops;
    total_int += (issues - a_nops - m_nops);
    printf("BU issues: %d, memrefs: %d, fp_ops: %d, ints+muls: %d"
           ",memratio:%6.2f\n",
           total_issues, total_memrefs, total_fp_ops, total_int,
           total_memrefs / (double) total_issues);
#endif

    free(rcount);

    return postticks;
  }

private:
  graph& g;
  int* result;
  int high_degree;
};
*/

template <typename graph, typename ComponentMap>
class news_visitor {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  news_visitor(ComponentMap& rm, vertex_t to_writ) :
    component(rm), to_write(to_writ) {}

  news_visitor(const news_visitor& nv) :
    component(nv.component), to_write(nv.to_write) {}

  void operator()(vertex_t u, vertex_t v, graph &G) { component[v] = to_write; }

protected:
  ComponentMap component;
  vertex_t to_write;
};

template <typename graph, typename ComponentMap>
class gcc_cc_psearch_visitor : public default_psearch_visitor<graph> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  gcc_cc_psearch_visitor(ComponentMap& rm, vertex_id_map<graph>& vmap,
                         graph& _g) :
    component(rm), vid_map(vmap), g(_g) {}

  void tree_edge(edge& e, vertex& src) const
  {
    vertex dest = other(e, src, g);
    component[dest] = component[src];
  }

protected:
  ComponentMap component;
  vertex_id_map<graph>& vid_map;
  graph& g;
};

template <class graph, class component_map>
class gcc_cc_setup_visitor1 {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::size_type size_type;

  gcc_cc_setup_visitor1(component_map res, vertex_t hdv, size_type& oe) :
    orphan_edges(oe), high_deg_v(hdv), result(res) {}

  gcc_cc_setup_visitor1(const gcc_cc_setup_visitor1& ccv) :
    orphan_edges(ccv.orphan_edges),
    high_deg_v(ccv.high_deg_v), result(ccv.result) {}

  gcc_cc_setup_visitor1& operator=(const gcc_cc_setup_visitor1& ccv)
  {
    result     = ccv.result;
    high_deg_v = ccv.high_deg_v;
    orphan_edges = ccv.orphan_edges;
    return *this;
  }

  void operator()(vertex_t u, vertex_t v)
  {
    if (result[u] != high_deg_v) mt_incr(orphan_edges, 1);
  }

  void post_visit() {}

private:
  size_type& orphan_edges;
  vertex_t high_deg_v;
  component_map result;
};

template <class graph, class component_map>
class gcc_cc_setup_visitor2 {
  // Run this single threaded so that next_index isn't a hotspot.
  // Could be improved to use a strategy like bfs_chunked's.

public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::size_type size_type;

  gcc_cc_setup_visitor2(component_map res, vertex_t hdv, size_type& ni,
                        size_type* scs, size_type* dsts,
                        vertex_id_map<graph> vi) :
    high_deg_v(hdv), result(res), next_index(ni), srcs(scs),
    dests(dsts), vid(vi) {}

  gcc_cc_setup_visitor2(const gcc_cc_setup_visitor2& ccv) :
    high_deg_v(ccv.high_deg_v), result(ccv.result), next_index(ccv.next_index),
    srcs(ccv.srcs), dests(ccv.dests), vid(ccv.vid) {}

  gcc_cc_setup_visitor2& operator=(const gcc_cc_setup_visitor2& ccv)
  {
    high_deg_v = ccv.high_deg_v;
    result = ccv.result;
    next_index = ccv.next_index;
    srcs = ccv.srcs;
    dests = ccv.dests;
    vid = ccv.vid;

    return *this;
  }

  void operator()(vertex_t u, vertex_t v)
  {
    if (result[u] != high_deg_v)
    {
      size_type my_ind = mt_incr(next_index, 1);
      srcs[my_ind] = get(vid, u);
      dests[my_ind] = get(vid, v);
    }
  }

  void post_visit() {}

private:
  vertex_t high_deg_v;
  component_map result;
  size_type& next_index;
  size_type* srcs;
  size_type* dests;
  vertex_id_map<graph> vid;
};

template <class graph, class component_map>
class cc_setup2_chunked_visitor {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::size_type size_type;

  cc_setup2_chunked_visitor(component_map res, vertex_t hdv, size_type& ni,
                            size_type* scs, size_type* dsts,
                            vertex_id_map<graph> vi) :
    high_deg_v(hdv), result(res), next_index(ni), srcs(scs),
#ifdef USING_QTHREADS
      my_srcs(new size_type[GCC_CC_CHUNK]),
      my_dests(new size_type[GCC_CC_CHUNK]),
#endif
    dests(dsts), vid(vi), count(0) {}

  cc_setup2_chunked_visitor(const cc_setup2_chunked_visitor& ccv) :
    high_deg_v(ccv.high_deg_v), result(ccv.result),
    next_index(ccv.next_index), srcs(ccv.srcs), dests(ccv.dests),
#ifdef USING_QTHREADS
      my_srcs(new size_type[GCC_CC_CHUNK]),
      my_dests(new size_type[GCC_CC_CHUNK]),
#endif
    vid(ccv.vid), count(0) {}
#ifdef USING_QTHREADS
  ~cc_setup2_chunked_visitor() {
      delete my_srcs;
      delete my_dests;
  }
#endif

  cc_setup2_chunked_visitor& operator=(const cc_setup2_chunked_visitor& ccv)
  {
    high_deg_v = ccv.high_deg_v;
    result = ccv.result;
    next_index = ccv.next_index;
    srcs = ccv.srcs;
    dests = ccv.dests;
    vid = ccv.vid;
    count = 0;

    return *this;
  }

  void operator()(vertex_t u, vertex_t v)
  {
    if (result[u] != high_deg_v)
    {
      my_srcs[count] = get(vid, u);
      my_dests[count++] = get(vid, v);
    }
  }

  void post_visit()
  {
    size_type my_buffer_pos = mt_incr(next_index, count);
    size_type my_end = my_buffer_pos + count;
    size_type i, j;

    for (i = my_buffer_pos, j = 0; i < my_end; i++, j++)
    {
      srcs[i] = my_srcs[j];
      dests[i] = my_dests[j];
    }

    count = 0;
  }

private:
  vertex_t high_deg_v;
  component_map result;
  size_type& next_index;
  size_type* srcs;
  size_type* dests;
  vertex_id_map<graph> vid;
  size_type count;
#ifdef USING_QTHREADS
  size_type *my_srcs, *my_dests;
#else
  size_type my_srcs[GCC_CC_CHUNK];
  size_type my_dests[GCC_CC_CHUNK];
#endif
};

#ifdef USING_QTHREADS
template <class graph_adapter, class component_map, class filter_t,
          class uinttype = unsigned long>
struct gcc_sv_s {
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex_t;

  graph_adapter& g;
  filter_t filter;
  uinttype& orphan_edges;
  vertex_t hdv;
  component_map result;

  gcc_sv_s(graph_adapter& gg, component_map comp,
           filter_t fil, uinttype& orph, vertex_t hd) :
    g(gg), filter(fil), orphan_edges(orph), result(comp), hdv(hd) {}
};

template <class graph_adapter, class component_map, class filter_t,
          class uinttype>
void gcc_sv_setup_loop1(qthread_t* me, const size_t startat,
                        const size_t stopat, void* arg)
{
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_t;
  typedef typename graph_traits<graph_adapter>::edge_iterator edge_iter_t;
  typedef struct gcc_sv_s<graph_adapter, component_map, filter_t> sv_setup_t;

  sv_setup_t* s_arg = (sv_setup_t*) arg;
  graph_adapter& g(s_arg->g);
  component_map result = s_arg->result;
  uinttype& orphan_edges = s_arg->orphan_edges;
  vertex_t hdv = s_arg->hdv;
  uinttype my_orphan_edges = 0;

  edge_iter_t edgs = edges(g);

  for (int i = startat; i < stopat; i++)
  {
    edge_t e = edgs[i];
    //if (filter(e)) {
    vertex_t u = source(e, g);

    if (result[u] != hdv) my_orphan_edges++;
    //}
  }

  mt_incr(orphan_edges, my_orphan_edges);
}

#endif

/// Search the giant component, then Shiloach-Vishkin on the result.
template <typename graph, typename component_map,
          typename filter_t = default_filter<graph> >
class connected_components_gcc_sv {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge;
  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::vertex_iterator vertex_iterator;

  connected_components_gcc_sv(graph& gg, component_map res,
                              filter_t vis = default_filter<graph>()) :
    g(gg), result(res) {}

  void run()
  {
    typedef array_property_map<size_type,
                               vertex_id_map<edge_array_adapter <size_type> > >
            eaa_component_map;

    size_type order = num_vertices(g);
    size_type size = num_edges(g);

    size_type* searchColor = new size_type[order];

    vertex_iterator verts = vertices(g);

    #pragma mta assert parallel
    for (size_type i = 0; i < order; i++)
    {
      searchColor[i] = 0;
      vertex_t v = verts[i];
      result[v] = v;
    }

    //// find the highest degree vertex
    size_type* degrees = new size_type[order];

    #pragma mta assert parallel
    for (size_type i = 0; i < order; i++)
    {
      degrees[i] = out_degree(verts[i], g);
    }

    size_type ind_of_max = 0;
    size_type maxdeg = degrees[0];

    #pragma mta assert nodep
    for (size_type i = 0; i < order; i++)
    {
      if (degrees[i] > maxdeg)
      {
        maxdeg = degrees[i];
        ind_of_max = i;
      }
    }

    delete[] degrees;

    vertex_t hdv = verts[ind_of_max];

    //// \find the highest degree vertex

    //// Search from that node to label the giant component (GCC).
    vertex_id_map<graph> vid = get(_vertex_id_map, g);

#ifndef __MTA__
    news_visitor<graph, component_map> nvis(result, hdv);
    bfs_chunked(g, nvis, hdv);
#else
    size_type* color = new size_type[order];
    memset(color, 0, sizeof(size_type) * order);
    gcc_cc_psearch_visitor<graph, component_map>
    gcc_vis(result, vid, g);

    psearch<graph, size_type*, gcc_cc_psearch_visitor<graph, component_map> >
      psrch(g, searchColor, gcc_vis);

    mt_timer pstime;
    pstime.start();
    psrch.run(hdv);
    pstime.stop();
    printf("psearch: %f\n", pstime.getElapsedSeconds());
#endif

    mt_timer setupt;
    setupt.start();
    size_type orphan_edges = 0;
    size_type* srcs;
    size_type* dests;

#ifdef __MTA__
    vertex_t* end_points = g.get_end_points();  // Fixed?
    size_type* C = result.get_data();  // Cheating! need to accomodate XMT.

    #pragma mta assert nodep
    #pragma mta assert noalias *C
    for (size_type u = 0; u < order; u++)
    {
      if (C[u] != hdv)
      {
        size_type begin = g[u];
        size_type end = g[u + 1];
        int_fetch_add(&orphan_edges, end - begin);
      }
    }

    size_type next_index = 0;
    srcs = new size_type[orphan_edges];
    dests = new size_type[orphan_edges];

    #pragma mta assert nodep
    #pragma mta assert noalias *C
    for (size_type u = 0; u < order; u++)
    {
      size_type begin = g[u];
      size_type end = g[u + 1];

      #pragma mta assert nodep
      #pragma mta assert noalias *C
      for (size_type j = begin; j < end; j++)
      {
        vertex_t v = end_points[j];

        if (C[u] != hdv)
        {
          size_type my_ind = int_fetch_add(&next_index, 1);
          srcs[my_ind] = get(vid, u);
          dests[my_ind] = get(vid, v);
        }
      }
    }
#else
    // Find how many "orphan" edges remain.
    size_type* prefix_counts = 0;
    size_type* started_nodes = 0;
    size_type num_threads = 0;

    gcc_cc_setup_visitor1<graph, component_map> ccv(result, hdv, orphan_edges);
    visit_adj(g, ccv, prefix_counts, started_nodes, num_threads,
              (size_type) GCC_CC_CHUNK);

    free(prefix_counts);
    free(started_nodes);

    size_type next_index = 0;

    srcs = new size_type[orphan_edges];
    dests = new size_type[orphan_edges];

    // Build an array of all non-GCC edges.
    prefix_counts = 0;
    started_nodes = 0;
    num_threads = 0;

    cc_setup2_chunked_visitor<graph, component_map>
//    gcc_cc_setup_visitor2<graph,component_map>
      ccv2(result, hdv, next_index, srcs, dests, get(_vertex_id_map, g));

    visit_adj(g, ccv2, prefix_counts, started_nodes, num_threads,
              (size_type) GCC_CC_CHUNK);

    free(prefix_counts);
    free(started_nodes);
#endif

    edge_array_adapter<size_type> eaa(srcs, dests, order, orphan_edges);
    size_type* eaa_result = new size_type[order];

    #pragma mta assert parallel
    for (size_type i = 0; i < order; i++)
    {
      vertex_t v = result[verts[i]];
      eaa_result[i] = (size_type)       get(vid, v);
    }

    eaa_component_map eaa_components(eaa_result, get(_vertex_id_map, eaa));
    connected_components_sv<edge_array_adapter<size_type>, eaa_component_map>
      non_gcc_sv(eaa, eaa_components);
    non_gcc_sv.run();

    #pragma mta assert parallel
    for (size_type i = 0; i < order; i++)
    {
      vertex_t v = verts[i];
      vertex_t w = verts[eaa_result[i]];
      result[v] = w;
    }
  }

private:
  graph& g;
  component_map result;
  filter_t filter;
  int high_degree;
};

/*! \brief A simple algorithm that, in serial, does a parallel search,
           calls the result a component, and repeats.  MTGL developers
           found to their surprise that this algorithm was effective on
           the scale-free graphs on which the MTGL was tested initially.
           It scales through 40 processors for inputs without too many
           connected components.

    Its primary use is as a verification method for other algorithms.
*/
template <typename graph, class uinttype>
class connected_components_simple {
public:
  typedef typename graph_traits<graph>::edge_descriptor edge;
  typedef typename graph_traits<graph>::vertex_iterator vertex_iterator;

  connected_components_simple(graph& gg, uinttype* res) : g(gg), result(res) {}

  int run()
  {
    mt_timer mttimer;
    mttimer.start();
#ifdef __MTA__
    int issues = mta_get_task_counter(RT_ISSUES);
    int memrefs = mta_get_task_counter(RT_MEMREFS);
    int m_nops = mta_get_task_counter(RT_M_NOP);
    int a_nops = mta_get_task_counter(RT_M_NOP);
    int fp_ops = mta_get_task_counter(RT_FLOAT_TOTAL);
#endif

    uinttype* searchColor = new uinttype[num_vertices(g)];

    #pragma mta assert parallel
    for (uinttype i = 0; i < num_vertices(g); i++)
    {
      searchColor[i] = 0;
      result[i] = -1;
    }

    vertex_id_map<graph> vid_map = get(_vertex_id_map, g);
    post_psearch_visitor<graph, uinttype*>postpsearch_vis(result, vid_map, g);
    bool done = false;
    int num_components = 0;

    vertex_iterator verts = vertices(g);

    while (!done)
    {
      int next = -1;
      int num_left = 0;

      #pragma mta assert parallel
      for (uinttype i = 0; i < num_vertices(g); i++)
      {
        if (result[i] == -1)
        {
          next = i;
          num_left++;
        }
      }

      if (next == -1) break;

      result[next] = next;

      psearch<graph, uinttype*, post_psearch_visitor<graph, uinttype*> >
        psrch(g, searchColor, postpsearch_vis);
      psrch.run(verts[next]);

      num_components++;
    }

#ifdef __MTA__
    issues = mta_get_task_counter(RT_ISSUES) - issues;
    memrefs = mta_get_task_counter(RT_MEMREFS) - memrefs;
    m_nops = mta_get_task_counter(RT_M_NOP)  - m_nops;
    a_nops = mta_get_task_counter(RT_A_NOP)  - a_nops;
    a_nops = mta_get_task_counter(RT_FLOAT_TOTAL) - fp_ops;
#endif

    mttimer.stop();
    long postticks = mttimer.getElapsedTicks();
    printf("CH components: %d\n", num_components);

#ifdef __MTA__
    static int total_issues, total_memrefs, total_fp_ops, total_int;
    total_issues += issues;
    total_memrefs += memrefs;
    total_fp_ops += fp_ops;
    total_int += (issues - a_nops - m_nops);
    printf("CH issues: %d, memrefs: %d, fp_ops: %d, ints+muls: "
           "%d,memratio:%6.2f\n", total_issues, total_memrefs,
           total_fp_ops, total_int,
           total_memrefs / (double) total_issues);
#endif

    return postticks;
  }

private:
  graph& g;
  uinttype* result;
  int high_degree;

  /*! \fn connected_components_simple(graph *g, int *result)
      \brief This constructor takes a graph and an array in which to write
             the component numbers and saves pointers to these.
      \param g A pointer to a graph.
      \param result A pointer to the array of component numbers.
  */
  /*! \fn run()
      \brief Invokes the algorithm.  In augmented versions, this will
             take a subproblem mask and the current subproblem. It
             returns the number of connected components.
  */
};

}

#endif

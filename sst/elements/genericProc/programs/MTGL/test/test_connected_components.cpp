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
/*! \file test_connected_components.cpp

    \author Jon Berry (jberry@sandia.gov)

    \date 12/4/2007
*/
/****************************************************************************/

#include <list>

#include "mtgl_test.hpp"

#include <mtgl/util.hpp>
#include <mtgl/static_graph_adapter.hpp>
#include <mtgl/breadth_first_search.hpp>
#include <mtgl/connected_components.hpp>
#include <mtgl/edge_array_adapter.hpp>
#include <mtgl/mtgl_boost_property.hpp>
#include <mtgl/metrics.hpp>

using namespace mtgl;

template <class graph_adapter>
void print_component_dimacs(graph_adapter& ga, int* result);

double get_freq()
{
#ifdef __MTA__
  double freq = mta_clock_freq();
#else
  double freq = 1000000;
#endif

  return freq;
}

template <class T, class T2>
void init_array(T* a, T2 size)
{
  #pragma mta assert nodep
  for (T2 i = 0; i < size; i++) a[i] = 0;
}

template <class T, class T2>
void init_array2(T* a, T2 size)
{
  #pragma mta assert nodep
  for (T2 i = 0; i < size; i++) a[i] = (T) i;
}

template <typename graph, typename inttype>
void test_connected_components_kahan(graph& g, inttype* result)
{
  init_array(result, num_vertices(g));
  connected_components_kahan<graph, inttype> k1(g, result, 5000);
  int ticks1 = k1.run();

  init_array(result, num_vertices(g));
  connected_components_kahan<graph, inttype> k2(g, result, 5000);
  int ticks2 = k2.run();

  fprintf(stderr, "RESULT: kahan %lu (%6.2lf, %6.2lf)\n", num_edges(g),
          ticks1 / get_freq(), ticks2 / get_freq());
}

template <typename graph, class uinttype>
void test_connected_components_simple(graph& g, uinttype* result)
{
  init_array(result, num_vertices(g));
  connected_components_simple<graph, uinttype> s1(g, result);
  int ticks1 = s1.run();

  init_array(result, num_vertices(g));
  connected_components_simple<graph, uinttype> s2(g, result);
  int ticks2 = s2.run();

  fprintf(stderr, "RESULT: simple %lu (%6.2lf, %6.2lf)\n", num_edges(g),
          ticks1 / get_freq(), ticks2 / get_freq());
}

template <typename graph>
void
test_connected_components_sv(
  graph& g, typename graph_traits<graph>::vertex_descriptor* result)
{
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef array_property_map<vertex_t, vertex_id_map<graph> > component_map;

  init_identity_vert_array(g, result);
  component_map components(result, get(_vertex_id_map, g));
  connected_components_sv<graph, component_map> s1(g, components);

  mt_timer timer;
  long issues, memrefs, concur, streams, traps;
  init_mta_counters(timer, issues, memrefs, concur, streams, traps);

  s1.run();

  sample_mta_counters(timer, issues, memrefs, concur, streams, traps);
  print_mta_counters(timer, num_edges(g), issues, memrefs,
                     concur, streams, traps);

  count_connected_components<graph, component_map> ccc(g, components);
  int nc = ccc.run();

  printf("there are %d connected components\n", nc);

  init_identity_vert_array(g, result);
  connected_components_sv<graph, component_map> s2(g, components);

  init_mta_counters(timer, issues, memrefs, concur, streams, traps);

  s2.run();

  sample_mta_counters(timer, issues, memrefs, concur, streams, traps);
  print_mta_counters(timer, num_edges(g), issues, memrefs,
                     concur, streams, traps);

  count_connected_components<graph, component_map> ccc2(g, components);
  nc = ccc2.run();

  printf("there are %d connected components\n", nc);

  find_biggest_connected_component<graph, component_map> fbcc(g, components);

  unsigned long biggest_id = fbcc.run();
  unsigned long biggest_size = fbcc.size();

  printf("the biggest component's id is %lu, with %lu\n",
         biggest_id, biggest_size);
}

template <typename graph_adapter>
void
test_connected_components_foo_sv(
  graph_adapter& g,
  typename graph_traits<graph_adapter>::vertex_descriptor* result)
{
  typedef graph_adapter graph;
  typedef graph_traits<graph>::vertex_descriptor vertex_t;
  typedef array_property_map<vertex_t, vertex_id_map<graph> > component_map;

  mt_timer timer;
  long issues, memrefs, concur, streams, traps;

  init_identity_vert_array(g, result);

  component_map components(result, get(_vertex_id_map, g));

  connected_components_foo_sv<graph, component_map,
                              default_filter<graph_adapter> >
  s3(g, components);

  init_mta_counters(timer, issues, memrefs, concur, streams, traps);

  s3.run();

  sample_mta_counters(timer, issues, memrefs, concur, streams, traps);
  print_mta_counters(timer, num_edges(g), issues, memrefs,
                     concur, streams, traps);

  count_connected_components<graph, component_map> ccc3(g, components);
  int nc = ccc3.run();

  printf("there are %d connected components\n", nc);

  find_biggest_connected_component<graph, component_map> fbcc(g, components);

  unsigned long biggest_id = fbcc.run();
  unsigned long biggest_size = fbcc.size();

  printf("the biggest component's id is %lu\n", biggest_id);
}

template <class uinttype, typename graph_adapter>
void run_algorithm(kernel_test_info& kti, graph_adapter& ga)
{
  uinttype* result = (uinttype*) malloc(sizeof(uinttype) * num_vertices(ga));
  if (find(kti.algs, kti.algCount, "cc"))
  {
    test_connected_components_kahan<graph_adapter, uinttype>(ga, result);
    //test_connected_components_kahan_bfs<graph_adapter>(ga, result);
  }
/*
  if (find(kti.algs, kti.algCount, "bu"))
  {
    test_connected_components_bully<graph_adapter>(ga, result);
    //test_connected_components_bully_bfs<graph_adapter>(ga, result);
  }
*/
  if (find(kti.algs, kti.algCount, "ch"))
  {
    test_connected_components_simple<graph_adapter, uinttype>(ga, result);
  }
  if (find(kti.algs, kti.algCount, "sv"))
  {
    typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex_t;

    vertex_t* result_verts = new vertex_t[num_vertices(ga)];

    test_connected_components_sv<graph_adapter>(ga, result_verts);
    test_connected_components_foo_sv(ga, result_verts);

    delete [] result_verts;
  }
/*
  if (find(kti.algs, kti.algCount, "gcc_sv"))
  {
    typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex_t;

    vertex_t* result_verts = new vertex_t[num_vertices(ga)];

    test_connected_components_gcc_sv<graph_adapter, uinttype>(ga, result_verts);

    delete [] result_verts;
  }
*/
}

int main(int argc, char* argv[])
{
  if (argc < 5)
  {
    fprintf(stderr, "usage: %s [-debug] [-bu | -cc | -ch | -sv] "
            "--graph_type <dimacs|cray> --level <levels> "
            "--graph <Cray graph number> "
            "--filename <dimacs graph filename> "
            "[--threads <num_qthread_shepherds>]\n", argv[0]);
    exit(1);
  }

  kernel_test_info kti;
  kti.process_args(argc, argv);

#ifdef USING_QTHREADS
  int threads = kti.threads;
  qthread_init(threads);

  aligned_t* rets = new aligned_t[threads];
  int* args = new int[threads];

  for (int i = 0; i < threads; i++)
  {
    args[i] = i;
    qthread_fork_to(setaffin, args + i, rets + i, i);
  }

  for (int i = 0; i < threads; i++)
  {
    qthread_readFF(NULL, rets + i, rets + i);
  }

  delete rets;
  delete args;

  typedef aligned_t uinttype;
#else
  //typedef unsigned long uinttype;
  typedef int uinttype;
#endif

  static_graph_adapter<bidirectionalS> ga;
  kti.gen_graph(ga);

  run_algorithm<uinttype, static_graph_adapter<bidirectionalS> >(kti, ga);

  int* degdist = degree_distribution(ga);

  printf("order: %lu, size: %lu\n", num_vertices(ga), num_edges(ga));
  for (int i = 0; i < 32; i++) printf("2^%d:\t%d\n", i, degdist[i]);
}

#include <map>

using namespace std;

template <class graph>
int find_biggest_component(graph& g, int* result)
{
  map<int, int> freq;

  // Populate map.
  int ord = num_vertices(g);
  for (int i = 0; i < ord; i++) freq[result[i]] = 0;

  int mx = result[0];
  for (int i = 0; i < ord; i++)
  {
    freq[result[i]]++;

    if (freq[result[i]] > freq[mx]) mx = result[i];
  }

  printf("max comp size: %d\n", freq[mx]);

  return mx;
}

template <class graph_adapter>
void print_component_dimacs(graph_adapter& ga, int* result)
{
  typedef typename graph_traits<graph_adapter>::vertex_descriptor
  vertex_descriptor;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_descriptor;
  typedef typename graph_traits<graph_adapter>::vertex_iterator vertex_iterator;
  typedef typename graph_traits<graph_adapter>::out_edge_iterator
  out_edge_iterator;

  int cnum = find_biggest_component(ga, result);

  vertex_id_map<graph_adapter> vipm = get(_vertex_id_map, ga);
  edge_id_map<graph_adapter> eipm = get(_edge_id_map, ga);

  list<size_type> cc_ids;
  size_type cc_edges = 0;

  vertex_iterator verts = vertices(ga);
  size_type order = num_vertices(ga);
  for (size_type i = 0; i < order; i++)
  {
    vertex_t v = verts[i];
    size_type vid = get(vipm, v);

    out_edge_iterator inc_edgs = out_edges(v, ga);
    size_type out_deg = out_degree(v, ga);
    for (size_type j = 0; j < out_deg; j++)
    {
      edge_t e = inc_edgs[j];
      int eid = get(eipm, e);

      vertex_descriptor src = source(e, ga);
      vertex_descriptor trg = target(e, ga);

      int sid = get(vipm, src);
      int tid = get(vipm, trg);

      if ((result[sid] == cnum) && (result[sid] == result [tid]))
      {
        cc_ids.push_back(sid);
        cc_ids.push_back(tid);
        cc_edges++;
      }
    }
  }

  cc_ids.sort();
  cc_ids.unique();

  map<int, int> idmap;
  list<int>::iterator it;

  int i = 0;
  for (it = cc_ids.begin(); it != cc_ids.end(); it++)
  {
    idmap[*it] = i++;
  }

  printf("p sp %d %d\n", cc_ids.size(), cc_edges);

  vertex_iterator verts = vertices(ga);
  size_type order = num_vertices(ga);
  for (size_type i = 0; i < order; i++)
  {
    vertex_t v = verts[i];
    int vid = get(vipm, v);

    out_edge_iterator inc_edgs = out_edges(v, ga);
    size_type out_deg = out_degree(v, ga);
    for (size_type j = 0; j < out_deg; j++)
    {
      edge_descriptor e = inc_edgs[j];
      int eid = get(eipm, e);

      vertex_descriptor src = source(e, ga);
      vertex_descriptor trg = target(e, ga);

      int sid = get(vipm, src);
      int tid = get(vipm, trg);

      if ((result[sid] == cnum) && (result[sid] == result [tid]))
      {
        printf("a %d %d %d\n", idmap[sid] + 1, idmap[tid] + 1, 0);
      }
    }
  }
}

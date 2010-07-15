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
/*! \file test_community2.cpp

    \author Jon Berry (jberry@sandia.gov)

    \date 6/25/2008
*/
/****************************************************************************/

#include "mtgl_test.hpp"

#include <mtgl/ufl.h>
#include <mtgl/util.hpp>
#include <mtgl/metrics.hpp>
#include <mtgl/SMVkernel.h>
#include <mtgl/static_graph_adapter.hpp>
#include <mtgl/subgraph_adapter.hpp>
#include <mtgl/snl_community2.hpp>
#include <mtgl/connected_components.hpp>
#include <mtgl/contraction_adapter.hpp>
#include <mtgl/visit_adj.hpp>
#include <mtgl/xmt_hash_table.hpp>

using namespace mtgl;

template <typename graph_adapter>
class primal_filter {
public:
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_t;

  primal_filter(graph_adapter& gg, double* prml, double thresh) :
    primal(prml), eid_map(get(_edge_id_map, gg)),
    threshold(thresh), tolerance(1e-07) {}

  bool operator()(edge_t e)
  {
    int eid = get(eid_map, e);
    return (primal[eid] >= (threshold - tolerance));
  }

private:
  edge_id_map<graph_adapter> eid_map;
  double* primal;
  double threshold;
  double tolerance;
};

// ************************************************************************
// pad_with_dummy
//
//      We need to provide the community detection algorithm with a
//  graph containing no isolated vertices.  We'll do this by adding
//  a dummy vertex that is connected to all current isolated vertices.
//  This rather than inducing a subgraph with everything except the
//   isolated.
// ************************************************************************
template <typename graph_adapter, typename hash_table_t>
bool pad_with_dummy(graph_adapter& g, hash_table_t& eweights)
{
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph_adapter>::vertex_iterator v_iter_t;

  int order = num_vertices(g);
  vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, g);
  v_iter_t verts = vertices(g);
  int num_zero = 0;

  #pragma mta assert parallel
  for (int i = 0; i < order; i++)
  {
    vertex_t v = verts[i];

    if (out_degree(v, g) == 0) mt_incr(num_zero, 1);
  }

  if (num_zero == 0) return false;

  vertex_t new_v = add_vertex(g);
  order = num_vertices(g);
  verts = vertices(g);
  int new_v_id = get(vid_map, new_v);

  #pragma mta assert parallel
  for (int i = 0; i < order; i++)
  {
    vertex_t v = verts[i];

    if (out_degree(v, g) == 0)
    {
      add_edge(new_v, v, g);
      int v1id = new_v_id;
      int v2id = get(vid_map, v);
      order_pair(v1id, v2id);
      int64_t key = v1id * order + v2id;
      eweights[key] = 1;
      printf("pad: [%d](%d,%d): %d\n", (int) key, v1id, v2id,
             (int) eweights[key]);
    }
  }

  return true;
}

template <typename graph_adapter>
void run_algorithm(kernel_test_info& kti, graph_adapter& ga, double thresh)
{
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_t;
  typedef typename graph_traits<graph_adapter>::size_type size_type;
  typedef typename graph_traits<graph_adapter>::edge_iterator edge_iter_t;
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph_adapter>::vertex_iterator vertex_iterator;
  typedef xmt_hash_table<int64_t, int64_t> hash_table_t;

  int order = num_vertices(ga);
  int size = num_edges(ga);

  int* degseq = degree_distribution(ga);

  for (int i = 0; i < 32; i++) printf(" deg 2^%d: %d\n", i, degseq[i]);

  delete [] degseq;

  int* leader = new int[order];
  for (int i = 0; i < order; i++) leader[i] = i;

  int prev_num_components = order;
  graph_adapter* cur_g = &ga;
  int* contracted2orig = 0;
  int* server = new int[size];
  double* primal = new double[size + 2 * size];
  double* support = 0;

  hash_table_t eweight(int64_t(1.5 * size), true);
  hash_table_t contr_eweight(int64_t(1.5 * size / 2), true);
  vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, *cur_g);

  edge_iter_t edgs = edges(*cur_g);
  for (int i = 0; i < size; i++)
  {
    edge_t e = edgs[i];
    vertex_t v1 = source(e, *cur_g);
    vertex_t v2 = target(e, *cur_g);
    int v1id = get(vid_map, v1);
    int v2id = get(vid_map, v2);
    order_pair(v1id, v2id);
    int64_t key = v1id * order + v2id;
    eweight.insert(key, 1);
  }

  snl_community2<graph_adapter> sc(*cur_g, eweight, server, primal, support);
  sc.run();
  double total_support = 0.0;
  #pragma mta assert nodep
  for (int i = 0; i < size; i++) total_support += primal[i];

  double ave_support = total_support / size;

  if (order < 100)
  {
    for (int i = 0; i < order; i++) printf("server[%d]: %d\n", i, server[i]);

    for (int i = 0; i < 3 * size; i++)
    {
      printf("primal[%d]: %lf\n", i, primal[i]);
    }
  }

  size_type* component = (size_type*) malloc(sizeof(size_type) * order);
  for (int i = 0; i < order; i++) component[i] = i;

  primal_filter<graph_adapter> pf(*cur_g, primal, ave_support);

  connected_components_sv<graph_adapter, size_type*,
                          primal_filter<graph_adapter> >
  csv(*cur_g, component, pf);
  int num_components = csv.run();

  for (int i = 0; i < order; i++)
  {
    leader[i] = component[i];
    printf("leader[%d]: %d\n", i, leader[i]);
  }

  list<contraction_adapter<graph_adapter>*> contractions;
  int cur_order = num_vertices(*cur_g);
  int cur_size  = num_edges(*cur_g);

  do {
    vertex_t* v_component = (vertex_t*) malloc(sizeof(vertex_t) * cur_order);
    vertex_iterator verts_cur_g = vertices(*cur_g);

    for (int i = 0; i < cur_order; i++)
    {
      v_component[i] = verts_cur_g[component[i]];
      printf("comp[%d]: %d\n", i, component[i]);
    }

    contraction_adapter<graph_adapter>* pcga =
      new contraction_adapter<graph_adapter>(*cur_g);
    contractions.push_front(pcga);
    contraction_adapter<graph_adapter>& cga = *pcga;

    printf("about to print cur\n");
    fflush(stdout);
    print(cga.original_graph);
    fflush(stdout);
    printf("about to contract: orig order: %d\n", num_vertices(*cur_g));

    contract(cur_order, cur_size, &eweight, &contr_eweight, v_component, cga);
    bool padded = pad_with_dummy(cga.contracted_graph, contr_eweight);

    int ctr_order = num_vertices(cga.contracted_graph);
    int ctr_size  = num_edges(cga.contracted_graph);
    cur_g = &cga.contracted_graph;

    vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, *cur_g);

    edge_iter_t edgs = edges(*cur_g);
    for (int i = 0; i < ctr_size; i++)
    {
      edge_t e = edgs[i];
      vertex_t v1 = source(e, *cur_g);
      vertex_t v2 = target(e, *cur_g);
      int v1id = get(vid_map, v1);
      int v2id = get(vid_map, v2);
      order_pair(v1id, v2id);
      int64_t key = v1id * ctr_order + v2id;
      printf("contr_weight[%d](%d,%d): %d\n", (int) key, v1id, v2id,
             (int) contr_eweight[key]);
    }

    contracted2orig = (int*) malloc(sizeof(int) * ctr_order);
    int p_order  = padded ? ctr_order - 1 : ctr_order;

    bool* emask = (bool*) malloc(sizeof(bool) * ctr_size);
    print(cga.contracted_graph);
    memset(emask, 1, ctr_size);

    //subgraph_adapter<graph_adapter> sga(cga.contracted_graph);
    //init_edges(emask, sga);
    //printf("found %d isolated vertices\n",
    //num_vertices(cga.contracted_graph) - num_vertices(sga.subgraph));

    vertex_iterator verts_cga = vertices(cga.contracted_graph);

    for (int i = 0; i < p_order; i++)
    {
      vertex_t lv = verts_cga[i];
      vertex_t gv = cga.local_to_global(lv);

      vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, ga);
      vertex_id_map<graph_adapter> cvid_map = get(_vertex_id_map,
                                                  cga.contracted_graph);

      int lid = get(cvid_map, lv);
      int gid = get(vid_map, gv);

      contracted2orig[lid] = gid;
      printf("local %d -> global %d\n", lid, gid);
    }

    print(cga.contracted_graph);
    int* server = new int[ctr_size];
    double* primal = new double[ctr_size + 2 * ctr_size];
    double* support = 0;
    print(*cur_g);

    snl_community2<graph_adapter> sc(*cur_g, contr_eweight,
                                     server, primal, support);
    sc.run();

    double total_support = 0.0;
    #pragma mta assert nodep
    for (int i = 0; i < ctr_size; i++) total_support += primal[i];

    double ave_support = total_support / ctr_size;

    if (ctr_order < 100)
    {
      for (int i = 0; i < ctr_order; i++)
      {
        printf("server[%d]: %d\n", i, server[i]);
      }
      for (int i = 0; i < 3 * ctr_size; i++)
      {
        printf("primal[%d]: %lf\n", i, primal[i]);
      }
    }

    free(component);
    component = (int64_t*) malloc(sizeof(int64_t) * ctr_order);
    for (int i = 0; i < ctr_order; i++) component[i] = i;

    primal_filter<graph_adapter> pf(*cur_g, primal, ave_support);

    connected_components_sv<graph_adapter, int64_t,
                            primal_filter<graph_adapter> >
      csv(*cur_g, component, pf);

    int num_components = csv.run();
    if (contracted2orig)
    {
      printf("p_order: %d\n", p_order);

      for (int i = 0; i < p_order; i++)
      {
        printf("contra2orig[%d]: %d\n", i, contracted2orig[i]);
        leader[contracted2orig[i]] = leader[contracted2orig[component[i]]];
      }
    }

    // *******************************************************
    // ** contract the community edges
    // *******************************************************

    delete [] server;
    delete [] primal;
    free(v_component);

    if ((num_components == 1) || (num_components == prev_num_components)) break;

    cur_order = ctr_order;
    cur_size  = ctr_size;
    prev_num_components = num_components;
  } while (1);

  for (int i = 0; i < order; i++) printf("leader[%d]: %d\n", i, leader[i]);
}

int main(int argc, char* argv[])
{
  if (argc < 3)
  {
    fprintf(stderr, "usage: %s [-debug] "
            " [-assort] <types> "
            " --graph_type <dimacs|cray> "
            " --level <levels> --graph <Cray graph number> "
            " --filename <dimacs graph filename> "
            " [<0..15>]\n"
            , argv[0]);
    exit(1);
  }

  static_graph_adapter<directedS> ga;

  kernel_test_info kti;
  kti.process_args(argc, argv);
  kti.gen_graph(ga);

  run_algorithm(kti, ga, 0.05);

  return 0;
}

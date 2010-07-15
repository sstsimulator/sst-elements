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
/*! \file test_newmanbenchmark.cpp

    \brief Performs the Newman-Girvan benchmark.

    \author Jon Berry (jberry@sandia.gov)

    \date 8/28/2008
*/
/****************************************************************************/

#include <list>
#include <map>
#include <strstream>

#include "mtgl_test.hpp"

#include <mtgl/ufl.h>
#include <mtgl/util.hpp>
#include <mtgl/metrics.hpp>
#include <mtgl/SMVkernel.h>
#include <mtgl/static_graph_adapter.hpp>
#include <mtgl/subgraph_adapter.hpp>
#include <mtgl/snl_community3.hpp>
#include <mtgl/connected_components.hpp>
#include <mtgl/contraction_adapter.hpp>
#include <mtgl/visit_adj.hpp>
#include <mtgl/xmt_hash_table.hpp>

using namespace std;
using namespace mtgl;

struct quadrant_freq {
  quadrant_freq() : ne(0), nw(0), se(0), sw(0) {}
  quadrant_freq(int _ne, int _nw, int _se, int _sw) :
    ne(_ne), nw(_nw), se(_se), sw(_sw) {}

  int ne, nw, se, sw;
};

int goodness(list<int64_t*>& component_maps, list<int>& component_map_sizes,
             list<int*>& contraction_maps,
             int*& comp_rep, int*& final_component)
//             graph_adapter* ga, double* edge_supp)
{
  map<int, struct quadrant_freq> the_final_components;

  list<int64_t*>::iterator begin = component_maps.begin();
  list<int64_t*>::iterator end = component_maps.end();
  list<int>::iterator sbegin = component_map_sizes.begin();
  list<int>::iterator send = component_map_sizes.end();
  list<int*>::iterator cbegin = contraction_maps.begin();
  list<int*>::iterator cend = contraction_maps.end();

  int order = *sbegin;

  //  int * final_component = (int*) malloc(sizeof(int)*order);
  final_component = (int*) malloc(sizeof(int) * order);
  int level = 0;
  for ( ; begin != end; begin++, sbegin++)
  {
    int order = *sbegin;
    int64_t* map = *begin;
    int* contr = *cbegin;
    //for (int i=0; i<order; i++) {
    //  printf("L: %d map[%d]: %d\n", level, i,
    //      map[i]);
    //  fflush(stdout);
    //}
    level++;
  }
  level = 0;
  sbegin = component_map_sizes.begin();
  for ( ; cbegin != cend; cbegin++, sbegin++)
  {
    int order = *sbegin;
    int64_t* map = *begin;
    int* contr = *cbegin;
    //for (int i=0; i<order; i++) {
    //  printf("L: %d contr_map[%d]: %d\n", level, i,
    //      contr[i]);
    //  fflush(stdout);
    //}
    level++;
  }
  begin = component_maps.begin();
  sbegin = component_map_sizes.begin();
  cbegin = contraction_maps.begin();
  for ( ; cbegin != cend; begin++, sbegin++, cbegin++)
  {
    int order = *sbegin;
    int64_t* map = *begin;
    int* contr = *cbegin;
    //for (int i=0; i<order; i++) {
    //  printf("L: %d comp[contr[%d]=%d]: %d\n", level, i,
    //      contr[i], map[contr[i]]);
    //  fflush(stdout);
    //}
    level++;
  }
  for (int i = 0; i < order; i++)
  {
    begin = component_maps.begin();
    int64_t* map = *begin;
    int my_id = i, my_comp = map[i];
    sbegin = component_map_sizes.begin();
    cbegin = contraction_maps.begin();
    for (begin++; cbegin != cend; begin++, sbegin++, cbegin++)
    {
      int order = *sbegin;
      int64_t* map = *begin;
      int* contr = *cbegin;
      //printf("%d: comp[contr[%d]=%d (ord: %d)]: %d\n",
      //  i, my_comp, contr[my_comp],
      //  order, map[contr[my_comp]]);
      //fflush(stdout);
      my_comp = map[contr[my_comp]];
    }
    final_component[i] = my_comp;
    quadrant_freq qf = the_final_components[my_comp];
    the_final_components[my_comp] =
      quadrant_freq(qf.ne + (i / 32 == 0),
                    qf.nw + (i / 32 == 1),
                    qf.se + (i / 32 == 2),
                    qf.sw + (i / 32 == 3));
    //printf("incr: %d (v: %d)\n", my_comp, i);
  }
  // now, assign each component to its natural Newman component
  int num_final_comp = the_final_components.size();

  //int *comp_rep = (int*) malloc(sizeof(int)*order);
  comp_rep = (int*) malloc(sizeof(int) * order);

  map<int, struct quadrant_freq>::iterator mb = the_final_components.begin();
  int i = 0;
  for ( ; mb != the_final_components.end(); mb++, i++)
  {
    struct quadrant_freq qf = (*mb).second;
    int comp = (*mb).first;
    //printf("qf[%d]: {%d\t%d\t%d\t%d}\n", comp, qf.ne, qf.nw, qf.se,
    //        qf.sw);
    if (qf.ne >= qf.nw && qf.ne >= qf.se && qf.ne >= qf.sw)
    {
      comp_rep[comp] = 0;
    }
    else if (qf.nw >= qf.ne && qf.nw >= qf.se && qf.nw >= qf.sw)
    {
      comp_rep[comp] = 32;
    }
    else if (qf.se >= qf.ne && qf.se >= qf.nw && qf.se >= qf.sw)
    {
      comp_rep[comp] = 64;
    }
    else
    {
      comp_rep[comp] = 96;
    }
  }

  int good = 0;
  //fflush(stdout);
  //for (int i=0; i<order; i++) {
  //  printf("comp_rep[%d]: %d\n", final_component[i],
  //          comp_rep[final_component[i]]);
  //}
  for (int i = 0; i < order; i++)
  {
    int cr = comp_rep[final_component[i]];
    printf("final_component[%d]: %d ", i, cr);
    if (i / 32 == cr / 32)
    {
      good++;
      //  printf("*");
    }
    printf("\n");
  }

  printf("num good: %d\n", good);
  return good;
}

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
    if (out_degree(v, g) == 0)
    {
      mt_incr(num_zero, 1);
    }
  }
  if (num_zero == 0) return false;

  vertex_t new_v = add_vertex(g);
  order = num_vertices(g);
  vertices verts = vertices(g);
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
      int64_t key = v1id * (order - 1) + v2id;
      eweights[key] = 1;
      //printf("pad: [%d](%d,%d): %d\n", (int) key, v1id,v2id,
      //    (int) eweights[key]);
    }
  }
  return true;
}

template <typename graph_adapter>
int run_algorithm(kernel_test_info& kti, graph_adapter& ga, int v_out,
                  int& num_comm, double* edge_supp)
{
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_t;
  typedef typename graph_traits<graph_adapter>::edge_iterator edge_iter_t;
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex_t;

  typedef xmt_hash_table<int64_t, int64_t> hash_table_t;

  int order = num_vertices(ga);
  int size =  num_edges(ga);
  printf("order: %d, size: %d\n", order, size);

  int* degseq = degree_distribution(ga);
  for (int i = 0; i < 32; i++)
  {
    printf(" deg 2^%d: %d\n", i, degseq[i]);
  }
  delete [] degseq;

  int* leader = new int[order];
  for (int i = 0; i < order; i++)
  {
    leader[i] = i;
  }
  int prev_num_components = order;
  graph_adapter* cur_g = &ga;
  int* orig2contracted = 0;
  int* server = new int[size];
  double* primal = new double[size + 2 * size];
  double* support = 0;

  hash_table_t eweight(int64_t(10 * size), true);  // ouch! do better
  hash_table_t contr_eweight(int64_t(10 * size), true);
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

  snl_community3<graph_adapter> sc(*cur_g, eweight, server, primal, support);
  sc.run();
  double total_support = 0.0;
  double e_ts_sqr = 0.0;
  #pragma mta assert nodep
  for (int i = 0; i < size; i++)
  {
    total_support += primal[i];
    e_ts_sqr += (primal[i] * primal[i]);
  }
  double ave_support = total_support / size;
  double variance = e_ts_sqr / size - (ave_support * ave_support);
  double stddev = sqrt(variance);
  if (order < 100)
  {
    printf("First run (ave primal: %lf)\n", ave_support);
    for (int i = 0; i < size; i++)
    {
      if (primal[i] >= ave_support) printf("primal[%d]: %lf\n", i, primal[i]);
    }
    for (int i = 0; i < order; i++)
    {
      printf("server[%d]: %d\n", i, server[i]);
    }
  }

  int64_t* component = (int64_t*) malloc(sizeof(int64_t) * order);
  for (int i = 0; i < order; i++)
  {
    component[i] = i;
  }
  printf("ave: %lf, stddev: %lf\n", ave_support, stddev);
  primal_filter<graph_adapter> pf(*cur_g, primal, //1.0);
                                  0.25 * stddev + ave_support);

  connected_components_sv<graph_adapter, int64_t,
                          primal_filter<graph_adapter> >
  csv(*cur_g, component, pf);
  int num_components = csv.run();
  list<contraction_adapter<graph_adapter>*> contractions;
  list<int64_t*> component_maps;
  list<int>  component_map_sizes;
  list<int*>  contraction_maps;
  int cur_order = num_vertices(*cur_g);
  int cur_size  = num_edges(*cur_g);
  int num_good = 0;

  do {
    map<int, int> comp_map;
    for (int i = 0; i < cur_order; i++) comp_map[i] = component[i];
    component_maps.push_back(component);


    int* final_component;
    int* comp_rep;
    component_map_sizes.push_back(cur_order);
    num_good = goodness(component_maps, component_map_sizes,
                        contraction_maps, comp_rep, final_component);

    vertex_t* v_component = (vertex_t*) malloc(sizeof(vertex_t) * cur_order);

    vertex_iterator verts_cur_g = vertices(*cur_g);

    for (int i = 0; i < cur_order; i++)
    {
      v_component[i] = verts_cur_g[component[i]];
      //printf("comp[%d]: %d\n", i, component[i]);
    }

    contraction_adapter<graph_adapter>* pcga =
      new contraction_adapter<graph_adapter>(*cur_g);

    contractions.push_back(pcga);
    contraction_adapter<graph_adapter>& cga = *pcga;

    //printf("about to print cur\n");
    //fflush(stdout);
    //print(cga.original_graph);
    //fflush(stdout);
    printf("about to contract: orig order: %d\n", num_vertices(*cur_g));
    contract(cur_order, cur_size, &eweight, &contr_eweight,
             v_component, cga);
    int prepad_order = num_vertices(cga.contracted_graph);
    bool padded = pad_with_dummy(cga.contracted_graph,
                                 contr_eweight);
    int ctr_order = num_vertices(cga.contracted_graph);
    int ctr_size  = num_edges(cga.contracted_graph);
    cur_g = &cga.contracted_graph;

    // fix edge weight hash table keys if padding occurred

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
      int64_t key = v1id * prepad_order + v2id;
      int64_t value = contr_eweight[key];
      contr_eweight.erase(key);
      key = v1id * ctr_order + v2id;
      //printf("contr_weight[%d](%d,%d): %d\n", (int)key,
      //    v1id,v2id, value);
      contr_eweight[key] = value;
    }

    orig2contracted = (int*) malloc(sizeof(int) * cur_order);

    bool* emask = (bool*) malloc(sizeof(bool) * ctr_size);
    print(cga.contracted_graph);
    memset(emask, 1, ctr_size);

    //subgraph_adapter<graph_adapter> sga(cga.contracted_graph);
    //init_edges(emask, sga);
    //printf("found %d isolated vertices\n",
    //num_vertices(cga.contracted_graph) - num_vertices(sga.subgraph));

    vertex_iterator verts_cga = vertices(cga.original_graph);

    for (int i = 0; i < cur_order; i++)
    {
      vertex_t gv = verts_cga[i];
      vertex_t lv = cga.global_to_local_v(gv);
      vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, ga);
      vertex_id_map<graph_adapter> cvid_map =
        get(_vertex_id_map, cga.contracted_graph);

      int lid = get(cvid_map, lv);
      int gid = get(vid_map, gv);
      orig2contracted[gid] = lid;
      //printf("global %d -> local %d\n", gid, lid);
    }

    contraction_maps.push_back(orig2contracted);
    print(cga.contracted_graph);
    int* server = new int[ctr_size];
    double* primal = new double[ctr_size + 2 * ctr_size];
    double* support = 0;
    print(*cur_g);

    snl_community3<graph_adapter> sc(*cur_g, contr_eweight,
                                     server, primal, support);
    sc.run();
    double total_support = 0.0;
    #pragma mta assert nodep
    for (int i = 0; i < ctr_size; i++)
    {
      total_support += primal[i];
      e_ts_sqr += (primal[i] * primal[i]);
    }

    double ave_support = total_support / ctr_size;
    double variance = e_ts_sqr / ctr_size - (ave_support * ave_support);
    double stddev = sqrt(variance);

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

    component = (int64_t*) malloc(sizeof(int64_t) * ctr_order);
    for (int i = 0; i < ctr_order; i++) component[i] = i;

    printf("ave: %lf, stddev: %lf\n", ave_support, stddev);
    primal_filter<graph_adapter> pf(*cur_g, primal,
                                    0.25 * stddev + ave_support);

    connected_components_sv<graph_adapter, int64_t,
                            primal_filter<graph_adapter> >
    csv(*cur_g, component, pf);
    num_components = csv.run();
    // *******************************************************
    // ** contract the community edges
    // *******************************************************
    delete [] server;
    delete [] primal;
    free(v_component);

    if ((num_components == 1) || (num_components == prev_num_components))
    {
      typedef typename graph_traits<graph_adapter>::edge_iterator
              edge_iterator_t;
      typedef typename graph_traits<graph_adapter>::edge_descriptor edge_t;
      typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex_t;

      int size = num_edges(ga);
      vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, ga);
      edge_iterator_t edgs = edges(ga);
      for (int i = 0; i < size; i++)
      {
        edge_t e = edgs[i];
        vertex_t s = source(e, ga);
        vertex_t t = target(e, ga);
        int sid = get(vid_map, s);
        int tid = get(vid_map, t);

        int s_cr = final_component[sid];
        int t_cr = final_component[tid];

        printf("looking at edge: %d, %d, final comp: %d, %d\n", sid, tid,
               s_cr, t_cr);

        if (s_cr == t_cr)
        {
          edge_supp[i]++;
        }
      }

      break;
    }

    cur_order = ctr_order;
    cur_size  = ctr_size;
    prev_num_components = num_components;
    eweight = contr_eweight;
    printf("DONE ITER\n");
  } while (1);

  // ******** \pull this into a loop
  num_comm = num_components;
  return num_good;
}

int main(int argc, char* argv[])
{
  //mta_suspend_event_logging();
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

  typedef static_graph_adapter<directedS> Graph;

  kernel_test_info kti;
  kti.process_args(argc, argv);

  char* filenames[] = {"/home/jberry/tmp/newman_0.dimacs",
                       "/home/jberry/tmp/newman_1.dimacs",
                       "/home/jberry/tmp/newman_2.dimacs",
                       "/home/jberry/tmp/newman_3.dimacs",
                       "/home/jberry/tmp/newman_4.dimacs",
                       "/home/jberry/tmp/newman_5.dimacs",
                       "/home/jberry/tmp/newman_6.dimacs",
                       "/home/jberry/tmp/newman_7.dimacs",
                       "/home/jberry/tmp/newman_8.dimacs",
                       "/home/jberry/tmp/newman_9.dimacs",
                       "/home/jberry/tmp/newman_10.dimacs",
                       "/home/jberry/tmp/newman_11.dimacs"};

  char* out_filenames[] = {"newman_0_ours.csv",
                           "newman_1_ours.csv",
                           "newman_2_ours.csv",
                           "newman_3_ours.csv",
                           "newman_4_ours.csv",
                           "newman_5_ours.csv",
                           "newman_6_ours.csv",
                           "newman_7_ours.csv",
                           "newman_8_ours.csv",
                           "newman_9_ours.csv",
                           "newman_10_ours.csv",
                           "newman_11_ours.csv"};


//   FILE *fp = 0;
//   fp = fopen(out_filenames[newman_iter] , "w");

  for (int v_out = 0; v_out < 12; v_out++)
  {
    Graph ga;
    strcpy(kti.graph_filename, filenames[v_out]);
    kti.gen_graph(ga);

    typedef graph_traits<Graph>::edge_descriptor edge_t;
    typedef graph_traits<Graph>::vertex_descriptor vertex_t;
    typedef graph_traits<Graph>::edge_iterator edge_iterator_t;

    vertex_id_map<Graph> vid_map = get(_vertex_id_map, ga);

    int total_good = 0;
    int total_num_comm = 0;
    int num_runs = 10;

    int size = num_edges(ga);
    double* edge_supp = (double*) malloc(size * sizeof(double));
    memset(edge_supp, 0, size * sizeof(double));

    for (int i = 0; i < num_runs; i++)
    {
      int nc = 0;
      total_good += run_algorithm<Graph>(kti, ga, v_out, nc, edge_supp);
      total_num_comm += nc;
    }

    double ave_good = total_good / (double) num_runs;
    double ave_num_comm = total_num_comm / (double) num_runs;

    printf("AVE GOODNESS(%d): %lf/128 = %lf  [%lf communities]\n",
           v_out, ave_good, ave_good / 128, ave_num_comm);

    //print out edge_supp array
    FILE* fp = 0;
    fp = fopen(out_filenames[v_out], "w");

    if (!fp)
    {
      fprintf(stderr, "ERROR, CAN'T OPEN FILE %s FOR WRITING\n",
              out_filenames[v_out]);
    }

    fprintf(fp, "VERT, TO_VERT, INTER_INTRA, SUPPORT, AVG_SCORE, "
            "SQUARED_ERROR\n");

    double average = 0.0;

    //get average
    for (int i = 0; i < size; i++)
    {
//        printf("edge_support before: %lf, num_runs: %d, ", edge_supp[i],
//               num_runs);
      if (edge_supp[i] > num_runs) printf("OOOPS!\n");
      edge_supp[i] /= (double) num_runs;
      //printf("edge_support after: %lf\n", edge_supp[i]);
      average += edge_supp[i];
    }

    //printf("average before norm: %lf, ", average);
    average /= (double) size;
    //printf("after: %lf\n", average);
    edge_iterator_t edgs = edges(ga);

    for (int i = 0; i < size; i++)
    {
      edge_t e = edgs[i];
      vertex_t s = source(e, ga);
      vertex_t t = target(e, ga);
      int sid = get(vid_map, s);
      int tid = get(vid_map, t);

      int inter_intra = 0;
      if (sid / 32 == tid / 32) inter_intra = 1;


      int avg_score = 0;
      if ((inter_intra && edge_supp[i] >= average) ||
          (!inter_intra && edge_supp[i] <  average))
      {
        avg_score = 1;
      }

      double squared_error = inter_intra - edge_supp[i];
      squared_error *= squared_error;

      fprintf(fp, "%d, %d, %d, %lf, %d, %lf\n", sid, tid, inter_intra,
              edge_supp[i], avg_score, squared_error);

    }

    fclose(fp);
    fp = 0;

    free(edge_supp);
    edge_supp = 0;
  }

  return 0;
}

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
/*! \file test_static_graph3.cpp

    \brief This shows how to use the load-balanced "visit_adj" function to
           efficiently traverse the adjaceny lists of vertices in a power-law
           graph.

    \author Jon Berry (jberry@sandia.gov)

    \date 8/15/2009

    It is compared to an optimal C implmentation that achieves the compiler's
    "loop merge." We dont't expect anything to be able to beat that C
    implementation.

    However, note that the latter is very inflexible.  Adding any nontrivial
    code to the inner loop body tends to defeat the compiler's loop merge.
    The consequences of this are demonstrated in test_static_graph6.cpp.
*/
/****************************************************************************/

#include <cstdlib>
#include <climits>

#include <mtgl/static_graph_adapter.hpp>
#include <mtgl/visit_adj.hpp>
#include <mtgl/generate_rmat_graph.hpp>
#include <mtgl/metrics.hpp>
#include <mtgl/read_matrix_market.hpp>
#include <mtgl/read_dimacs.hpp>
#include <mtgl/mtgl_io.hpp>

using namespace std;
using namespace mtgl;

typedef static_graph_adapter<directedS> Graph;
typedef graph_traits<Graph>::size_type size_type;

// ********************************************************************
// ** compute_in_degree ***********************************************
// ********************************************************************
// ** Pure C traveral
// ********************************************************************
void compute_in_degree(static_graph<directedS>* g, size_type* in_degree)
{
  size_type i, j;
  size_type order = g->n;
  #pragma mta assert nodep
  #pragma mta assert noalias *in_degree
  for (i = 0; i < order; i++)
  {
    size_type begin = g->index[i];
    size_type end = g->index[i + 1];
    for (j = begin; j < end; j++)
    {
      mt_incr(in_degree[g->end_points[j]], 1);
    }
  }
}

//
// in_degree_computer:  a visitor to the load-balanced visit_adj function
//
template <typename graph>
class in_degree_computer {
public:
  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::vertex_descriptor vertex_descriptor;

  in_degree_computer(graph& gg, vertex_id_map<graph> vm, size_type *ind) :
    g(gg), vid_map(vm), in_degree(ind), order(num_vertices(gg)) {}

  void operator()(const vertex_descriptor& src, const vertex_descriptor& dest)
  {
    size_type v1id = get(vid_map, src);
    size_type v2id = get(vid_map, dest);
    mt_incr(in_degree[v2id],1);
  }

  void post_visit() {}

private:
  graph& g;
  size_type order;
  vertex_id_map<graph>& vid_map;
  size_type* in_degree;
};


int main(int argc, char* argv[])
{
  srand48(0);

  if (argc < 2)
  {
    fprintf(stderr, "Usage: %s {<p>|<filename>} \n", argv[0]);
    fprintf(stderr, "where specifying p requests a generated r-mat graph "
            "with 2^p vertices,\n");
    fprintf(stderr, "and specifying filename requests that a DIMACS or "
            "MatrixMarket file (or the .srcs file of a .srcs,.dests "
            "snapshot pair) be read to build the graph;\n");
    fprintf(stderr, "delta is the convergence tolerance of "
            "the power iteration.\n");
    fprintf(stderr, "DIMACS files must end with suffix .dimacs\n");
    fprintf(stderr, "MatrixMarket files must end with suffix .mtx\n");
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
      // String contains non-numeric characters; it must be a filename.
      use_rmat = 0;
      break;
    }
  }

  Graph g;

  if (use_rmat)
  {
    generate_rmat_graph(g, atoi(argv[1]), 8);
  }
  else if (argv[1][llen - 1] == 'x')
  {
    // Matrix-market input.
    dynamic_array<int> weights;

    read_matrix_market(g, argv[1], weights);
  }
  else if (strcmp(&argv[1][llen - 4], "srcs") == 0)
  {
    // Snapshot input: <file>.srcs, <file>.dests
    char* srcs_fname = argv[1];
    char dests_fname[256];

    strcpy(dests_fname, srcs_fname);
    strcpy(&dests_fname[llen - 4], "dests");

    read_binary(g, srcs_fname, dests_fname);
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

  size_type order = num_vertices(g);
  size_type size = num_edges(g);
  size_type* indeg = new size_type[order];
  printf("order: %d, size: %d\n", order, size);

  static_graph<directedS>* sg = g.get_graph();
//  compute_in_degree(sg, indeg);

  mt_timer timer;
  int issues, memrefs, concur, streams, traps;

  init_mta_counters(timer, issues, memrefs, concur, streams, traps);
//  timer.start();
  compute_in_degree(sg, indeg);
  sample_mta_counters(timer, issues, memrefs, concur, streams, traps);
//  timer.stop();
  printf("---------------------------------------------\n");
  printf("Pure C: \n");
  printf("---------------------------------------------\n");
  print_mta_counters(timer, num_edges(g), issues, memrefs, concur, streams,
                     traps);
//  printf("\n  Time: %f\n", timer.getElapsedSeconds());

  in_degree_computer<Graph> idc(g, get(_vertex_id_map, g), indeg);
  size_type *prefix_counts, *started_nodes, num_threads;
  init_mta_counters(timer, issues, memrefs, concur, streams, traps);
//  timer.start();
  visit_adj(g, idc, prefix_counts, started_nodes, num_threads);
  sample_mta_counters(timer, issues, memrefs, concur, streams, traps);
//  timer.stop();
  printf("---------------------------------------------\n");
  printf("visit_adj: \n");
  printf("---------------------------------------------\n");
  print_mta_counters(timer, num_edges(g), issues, memrefs, concur, streams,
                     traps);
//  printf("\n  Time: %f\n", timer.getElapsedSeconds());

  return 0;
}

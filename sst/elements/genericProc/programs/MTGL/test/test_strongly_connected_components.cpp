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
/*! \file test_strongly_connected_components.cpp

    \author Jon Berry (jberry@sandia.gov)

    \date 12/4/2007
*/
/****************************************************************************/

#include <cassert>

#include <mtgl/mtgl_adapter.hpp>
#include <mtgl/static_graph_adapter.hpp>
#include <mtgl/util.hpp>
#include <mtgl/strongly_connected_components.hpp>
#include <mtgl/metrics.hpp>
#include <mtgl/dynamic_array.hpp>

#include "mtgl_test.hpp"

using namespace mtgl;

double get_freq()
{
#ifdef __MTA__
  double freq = mta_clock_freq();
#else
  double freq = 1000000;
#endif

  return freq;
}

void print(int* A, int size)
{
  assert(A != NULL);
  printf("A:\t[");

  for(int i = 0; i < size; i++) printf(" %d", A[i]);

  printf(" ]\n");
}

template <typename graph_t>
void test_scc(graph_t& ga, int* result)
{
  strongly_connected_components<graph_t> scc(ga, result);
  int sccticks1 = scc.run();
  dynamic_array<int> ldrs;

  findClasses(result, num_vertices(ga), ldrs);

  printf("There are %d strongly connected components\n", ldrs.size());
  fprintf(stderr, "RESULT: scc %d (%6.2lf)\n", num_edges(ga),
          sccticks1 / get_freq());
}

template <typename graph_t>
void test_scc_dc_simple(graph_t& ga, int* result)
{
  strongly_connected_components_dc_simple<graph_t> scc(ga, result);
  int sccticks1 = scc.run();
  dynamic_array<int> ldrs;

  findClasses(result, num_vertices(ga), ldrs);

  printf("There are %d strongly connected components\n", ldrs.size());
  fprintf(stderr, "RESULT: scc %d (%6.2lf)\n", num_edges(ga),
          sccticks1 / get_freq());
}

template <typename graph_t>
void test_scc_dcsc(graph_t& ga, int* result)
{
  strongly_connected_components_dcsc<graph_t> scc(ga, result);
  int sccticks1 = scc.run();
  dynamic_array<int> ldrs;

  findClasses(result, num_vertices(ga), ldrs);

  printf("There are %d strongly connected components\n", ldrs.size());
  fprintf(stderr, "RESULT: scc %d (%6.2lf)\n", num_edges(ga),
          sccticks1 / get_freq());
}

template<typename graph_t>
void test_scc_recursive_dcsc(graph_t& ga, int* result)
{
  int numTriv = 0;
  int numNonTriv = 0;
  recursive_dcsc<graph_t> scc(ga, numTriv, numNonTriv, result);

  scc.run();

  printf("There are %d trivial SCC's\n", numTriv);
  printf("There are %d non-trivial SCC's\n", numNonTriv);
}

int main(int argc, char* argv[])
{
  if (argc < 5)
  {
    fprintf(stderr, "usage: %s [-debug] "
            " [-scc | -scc2 | -scc3] "
            " --graph_type <dimacs|rmat> "
            " --graph_struct <dyn|csr|static>"
            " --filename <dimacs graph filename> "
            " --graph <scale>"
            " --direction <directed|reversed|undirected>"
            "\n"
            , argv[0]);
    exit(1);
  }

  static_graph_adapter<directedS> ga;

  kernel_test_info kti;
  kti.process_args(argc, argv);
  kti.gen_graph(ga);

  printf("nVerts = %d\n", num_vertices(ga));
  printf("nEdges = %d\n", num_edges(ga));
  fflush(stdout);

  int* result = (int*) malloc(sizeof(int) * num_vertices(ga));

  if (find(kti.algs, kti.algCount, "scc"))
  {
    printf("running scc algorithm\n");
    test_scc<graph_t>(ga, result);
  }
  else if (find(kti.algs, kti.algCount, "scc2"))
  {
    printf("running scc dc-simple algorithm.\n");
    test_scc_dc_simple<graph_t>(ga, result);
  }
  else if (find(kti.algs, kti.algCount, "scc3"))
  {
    printf("running scc dcsc algorithm.\n");
    test_scc_dcsc<graph_t>(ga, result);
  }
  else if (find(kti.algs, kti.algCount, "scc4") && kti.graph_struct == BIDIR)
  {
    // note: this currently works only with bidirectional graph adapter until
    //       psearch / bfssearch works with reversible edges.
    printf("running my dcsc algorithm.\n");
    static_graph_adapter<bidirectionalS>* __ga =
      (static_graph_adapter<bidirectionalS>*) &ga;
    test_scc_recursive_dcsc<static_graph_adapter<bidirectionalS> >(*__ga,
                                                                   result);
  }

  free(result);

  return 0;
}

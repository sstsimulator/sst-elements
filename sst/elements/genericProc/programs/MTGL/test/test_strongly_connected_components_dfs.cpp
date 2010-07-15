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
/*! \file test_strongly_connected_components_dfs.cpp

    \author Jon Berry (jberry@sandia.gov)

    \date 12/4/2007
*/
/****************************************************************************/

#include "mtgl_test.hpp"

#include <mtgl/util.hpp>
#include <mtgl/dynamic_array.hpp>
#include <mtgl/strongly_connected_components_dfs.hpp>
#include <mtgl/static_graph_adapter.hpp>

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

template <typename graph>
void test_strongly_connected_components_dfs(graph& g, int* result)
{
  int sccticks1 = 0;
  int order = num_vertices(g);
  int size = num_edges(g);

  int* remainder = (int*) malloc(sizeof(int) * order);
  int* finish_time = (int*) malloc(sizeof(int) * order);

  strongly_connected_components_dfs<graph> scc(g, result, remainder);
  int sccticks2 = scc.run();

  dynamic_array<int> ldrs2;

  findClasses(result, order, order * 2, ldrs2);

  printf("There are %d strongly connected components\n", ldrs2.size());
  fprintf(stderr, "RESULT: scc_dfs %d (%6.2lf,%6.2lf)\n", size,
          sccticks1 / get_freq(), sccticks2 / get_freq());
}

int main(int argc, char* argv[])
{
  if (argc < 5)
  {
    fprintf(stderr, "usage: %s [-debug] "
            " [-scc_dfs] "
            " --graph_type <dimacs|cray> "
            " --level <levels> --graph <Cray graph number> "
            " --filename <dimacs graph filename> "
            " [<0..15>]\n" , argv[0]);
    exit(1);
  }

  static_graph_adapter<directedS> g;

  kernel_test_info kti;
  kti.process_args(argc, argv);
  kti.gen_graph(g);

  int* result = (int*) malloc(sizeof(int) * num_vertices(g));

  if (find(kti.algs, kti.algCount, "scc_dfs"))
  {
    test_strongly_connected_components_dfs<graph>(g, result);
  }

  free(result);
}

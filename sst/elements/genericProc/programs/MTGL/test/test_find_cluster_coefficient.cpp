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
/*! \file test_find_cluster_coefficient.cpp

    \author Vitus Leung (vjleung@sandia.gov)

    \date 3/5/2008
*/
/****************************************************************************/

#include "mtgl_test.hpp"

#include <mtgl/metrics.hpp>
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
void test_find_cluster_coefficient(graph& g)
{
  double clco;
  find_cluster_coefficient<graph> fcc(g, &clco);
  int ticks = fcc.run();

  fprintf(stdout, "found cluster coefficent %lf\n", clco);
  fprintf(stderr, "RESULT: find_cluster_coefficient %lu (%6.2lf, 0)\n",
  num_edges(g), ticks / get_freq());
}

int main(int argc, char *argv[])
{
  if (argc < 5)
  {
    fprintf(stderr, "usage: %s -debug "
           " -clco "
           " --graph_type rmat "
           " --graph_struct static\n"
           , argv[0]);

    exit(1);
  }

  static_graph_adapter<directedS> ga;

  kernel_test_info kti;
  kti.process_args(argc, argv);
  kti.gen_graph(ga);

  if (find(kti.algs, kti.algCount, "clco"))
  {
    test_find_cluster_coefficient<graph_adapter>(ga);
  }
}

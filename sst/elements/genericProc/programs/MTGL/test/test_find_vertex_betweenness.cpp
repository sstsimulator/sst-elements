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
/*! \file test_find_vertex_betweenness.cpp

    \author Vitus Leung (vjleung@sandia.gov)

    \date 7/10/2008
*/
/****************************************************************************/

#include "mtgl_test.hpp"

#include <mtgl/vertex_betweenness.hpp>
#include <mtgl/static_graph_adapter.hpp>

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
void test_find_vertex_betweenness(graph& g)
{
  int order = num_vertices(g);
  double* res = new double[order];

  // JWB: don't know what third parm is.
  find_vertex_betweenness<graph> fvb(g, res, order);

  int ticks = fvb.run();

  fprintf(stdout, "vertex betweenness centrality:\n");

  for(int i = 0; i < order; ++i) fprintf(stdout, "%d\t%9lf\n", i, res[i]);

  fprintf(stderr, "RESULT: find_vertex_betweenness %lu (%6.2lf,0)\n",
          num_edges(g), ticks/get_freq());

  delete [] res;
}

int main(int argc, char *argv[])
{
  if (argc < 5)
  {
    fprintf(stderr,"usage: %s -debug "
           " -vb "
           " --graph_type rmat "
           " --graph_struct static\n"
           , argv[0]);
    exit(1);
  }

  typedef static_graph_adapter<directedS> Graph;

  Graph ga;

  kernel_test_info kti;
  kti.process_args(argc, argv);
  kti.gen_graph(ga);

  if (find(kti.algs, kti.algCount, "vb"))
  {
    test_find_vertex_betweenness<Graph>(ga);
  }
}

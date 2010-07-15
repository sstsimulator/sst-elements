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
/*! \file test_mask_search.cpp

    \author Brad Mancke

    \date 12/12/2007
*/
/****************************************************************************/

#include "mtgl_test.hpp"

#include <mtgl/util.hpp>
#include <mtgl/metrics.hpp>
#include <mtgl/SMVkernel.h>
#include <mtgl/static_graph_adapter.hpp>
#include <mtgl/connected_components.hpp>
#include <mtgl/mask_search.hpp>

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

void init_array(int* a, int size)
{
  #pragma mta assert nodep
  for (int i = 0; i < size; i++) a[i] = 0;
}

template <typename graph>
void test_connected_components_bully(graph& g, int* result)
{
  init_array(result, num_vertices(g));
  connected_components_bully<graph> bu(g, result, 5000);
  int ticks1 = bu.run();
  init_array(result, num_vertices(g));
  connected_components_bully<graph> bu2(g, result, 5000);
  int ticks2 = bu2.run();
  fprintf(stderr, "RESULT: bully %d (%6.2lf,%6.2lf)\n", num_edges(g),
          ticks1 / get_freq(), ticks2 / get_freq());
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

  Graph ga;

  kernel_test_info kti;
  kti.process_args(argc, argv);
  kti.gen_graph(ga);

  int order = num_vertices(ga);
  int size = num_edges(ga);

  default_mask_search_visitor<Graph> dmsv;

  mask_search<Graph, default_mask_search_visitor<Graph> > ms(ga, dmsv);
  ms.run();
}

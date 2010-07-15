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
/*! \file test_dpmf.cpp

    \brief Tests the disjoint paths maxflow code.

    \author Brad Mancke

    \date 4/3/2008
*/
/****************************************************************************/

#include "mtgl_test.hpp"

#include <mtgl/disjoint_paths_max_flow.hpp>
#include <mtgl/util.hpp>
#include <mtgl/static_graph_adapter.hpp>

using namespace mtgl;

int main(int argc, char *argv[])
{
  if (argc < 3)
  {
    fprintf(stderr, "usage: %s [-debug] "
            " --graph_struct bidir "
            " --graph_type <rmat|snapshot> "
            " --graph <rmat scale> "
            " --filename <snapshot filename w/o .srcs|.dests> \n"
            , argv[0]);
    exit(1);
  }

  typedef static_graph_adapter<bidirectionalS> Graph;
  typedef graph_traits<Graph>::vertex_descriptor vertex_descriptor;

  Graph ga;

  kernel_test_info kti;
  kti.process_args(argc, argv);
  kti.gen_graph(ga);

  int order = num_vertices(ga);
  int size = num_edges(ga);

  int sid = kti.source;
  int tid = kti.sink;

  if (sid == tid)
  {
    if (!kti.weights)
    {
      sid = 0;
      // tid = order - 1;
      // Might want to set tid to 1 for large rmat graphs
      tid = 1;
    }
    else
    {
      printf("ERROR, SOURCE CAN'T EQUAL SINK\n");
    }
  }

  printf("sid: %d, tid: %d\n", sid, tid);
  printf("size: %d, order: %d\n", size, order);

  vertex_descriptor s = get_vertex(sid, ga);
  vertex_descriptor t = get_vertex(tid, ga);

  typedef int flow_t;

  disjoint_paths_max_flow<static_graph_adapter<bidirectionalS>, flow_t>
    dpmf(ga, s, t, kti.weights);
  flow_t flow = dpmf.run();

  std::cout << "Flow: " << flow << std::endl;

  dpmf.print_source_sink_edges();

  bool optimal = dpmf.check_to_see_if_optimal();
  bool constraints = dpmf.check_balance_constraints();

  return 0;
}

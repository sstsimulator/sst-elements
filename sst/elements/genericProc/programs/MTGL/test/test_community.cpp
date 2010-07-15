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
/*! \file test_community.cpp

    \author Jon Berry (jberry@sandia.gov)

    \date 2/22/2008
*/
/****************************************************************************/

#include "mtgl_test.hpp"

#include <mtgl/ufl.h>
#include <mtgl/util.hpp>
#include <mtgl/metrics.hpp>
#include <mtgl/SMVkernel.h>
#include <mtgl/static_graph_adapter.hpp>
#include <mtgl/snl_community.hpp>
#include <mtgl/connected_components.hpp>
#include <mtgl/visit_adj.hpp>

using namespace mtgl;

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

  static_graph_adapter<directedS> ga;

  kernel_test_info kti;
  kti.process_args(argc, argv);
  kti.gen_graph(ga);

  int order = num_vertices(ga);
  int size =  num_edges(ga);

  int* server = new int[order];
  double* primal = new double[order + 3 * size];
  double* support = 0;

  int* degseq = degree_distribution(ga);
  for (int i = 0; i < 32; i++)
  {
    printf(" deg 2^%d: %d\n", i, degseq[i]);
  }
  delete [] degseq;

  snl_community<graph_adapter> sga(ga, server, primal, support);
  sga.run();

  if (order < 100)
  {
    for (int i = 0; i < order; i++)
    {
      printf("server[%d]: %d\n", i, server[i]);
    }
  }

  if (size < 100 && support)
  {
    for (int i = 0; i < size; i++)
    {
      printf("support[%d]: %lf\n", i, support[i]);
    }
  }

  delete [] server;
  delete [] primal;
  delete [] support;

  return 0;
}

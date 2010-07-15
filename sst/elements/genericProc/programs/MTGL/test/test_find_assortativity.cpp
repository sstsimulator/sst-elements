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
/*! \file test_find_assortivity.cpp

    \author Jon Berry (jberry@sandia.gov)

    \date 12/4/2007
*/
/****************************************************************************/

#include "test_find_assortativity.h"
#include "mtgl_test.hpp"

#include <mtgl/static_graph_adapter.hpp>

int main(int argc, char* argv[])
{
  if (argc < 5)
  {
    fprintf(stderr,
            "usage: %s -assort <types>\n"
            "        --graph_struct static\n"
            "        -debug\n"
            "        --graph_type rmat\n", argv[0]);

    exit(1);
  }

  static_graph_adapter<directedS> ga;

  kernel_test_info kti;
  kti.process_args(argc, argv);
  kti.gen_graph(ga);

  if (find(kti.algs, kti.algCount, "assort"))
  {
    if (kti.graph_type != MMAP)
    {
      test_find_assortativity<graph_adapter>(ga, kti.assort_types);
    }
  }
}

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
/*! \file test_find_modularity.cpp

    \author Jon Berry (jberry@sandia.gov)

    \date 12/4/2007
*/
/****************************************************************************/

#include "mtgl_test.hpp"

#include <mtgl/util.hpp>
#include <mtgl/metrics.hpp>
#include <mtgl/graph_adapter.hpp>

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
void test_find_modularity(graph g, graph_traits<Graph>::size_type ntypes)
{
  typedef graph_traits<Graph>::size_type size_type;

  size_type order = num_vertices(g);
  size_type size = num_edges(g);

  int** result = (int**) malloc(ntypes * sizeof(int*));
  for (size_type i = 0; i < ntypes; ++i)
  {
    result[i] = (int*) calloc(ntypes, sizeof(int));
  }

  int* type = (int*) malloc(order * sizeof(int));

  for (size_type i = 0; i < order; ++i) type[i] = i % 2;

  for (size_type i = 0; i < 1; ++i) type[i] = 0;

  for (size_type i = 1; i < 4; i++) type[i] = 1;

  for (size_type i = 4; i < 7; i++) type[i] = 0;

  for (size_type i = 7; i < 8; i++) type[i] = 1;

  for (size_type i = 8; i < 9; i++) type[i] = 2;

  for (size_type i = 9; i < 10; i++) type[i] = 1;

  for (size_type i = 10; i < 12; i++) type[i] = 0;

  for (size_type i = 12; i < 14; i++) type[i] = 1;

  for (size_type i = 14; i < 16; i++) type[i] = 2;

  for (size_type i = 16; i < 17; i++) type[i] = 0;

  for (size_type i = 17; i < 18; i++) type[i] = 1;

  for (size_type i = 18; i < 19; i++) type[i] = 2;

  for (size_type i = 19; i < 20; i++) type[i] = 0;

  for (size_type i = 20; i < 21; i++) type[i] = 2;

  for (size_type i = 21; i < 22; i++) type[i] = 1;

  for (size_type i = 22; i < order; i++) type[i] = 2;

  find_type_mixing_matrix<graph> fa(g, result, ntypes, type);

//  mta_resume_event_logging();

  int ticks1 = fa.run();

//  mta_suspend_event_logging();

  int all = 2 * size;
  double ps, Q = 0, sq;

  for (size_type i = 0; i < ntypes; ++i)
  {
    ps = 0;

    for (int j = 0; j < ntypes; ++j) ps += result[i][j];

    ps /= all;
    sq = ps * ps;
    Q += (double) result[i][i] / all - sq;
  }

  fprintf(stdout, "modularity = %lf\n", Q);
  fprintf(stderr, "RESULT: find_modularity %d (%6.2lf,0)\n", size,
          ticks1 / get_freq());

  for (size_type i = 0; i < ntypes; ++i) free(result[i]);
  free(result);
  free(type);
}

int main(int argc, char* argv[])
{
  typedef graph_traits<graph_adapter>::size_type size_type;

  //mta_suspend_event_logging();
  if (argc < 5)
  {
    fprintf(stderr,
            "usage: %s -mod <types>\n"
            "        --graph_struct <dyn|csr>\n"
            "        -debug\n"
            "        --graph_type <cray|dimacs>\n"
            "        --graph <Cray graph number> --level <Cray graph levels>\n"
            "        --filename <dimacs>\n", argv[0]);
    exit(1);
  }

  kernel_test_info kti;
  kti.process_args(argc, argv);

  // 06/11/2009 GEM: It appears that this is using the types for something.
  //                 The type attributes for edges and vertices were removed
  //                 from the edge and vertex objects.  Thus, this needs to
  //                 be changed to use types as property maps.

  if (find(kti.algs, kti.algCount, "mod"))
  {
    if (kti.graph_type != MMAP)
    {
      graph* g = kti.g;
      size_type order = num_vertices(*g);

      for (size_type i = 0; i < order; i++)
      {
        get_vertex(i, *g)->set_type(i);
      }

      for (size_type i = 0; i < 1; i++)
      {
        get_vertex(i, *g)->set_type(0);
      }

      for (size_type i = 1; i < 4; i++)
      {
        get_vertex(i, *g)->set_type(1);
      }

      for (size_type i = 4; i < 7; i++)
      {
        get_vertex(i, *g)->set_type(0);
      }

      for (size_type i = 7; i < 8; i++)
      {
        get_vertex(i, *g)->set_type(1);
      }

      for (size_type i = 8; i < 9; i++)
      {
        get_vertex(i, *g)->set_type(2);
      }

      for (size_type i = 9; i < 10; i++)
      {
        get_vertex(i, *g)->set_type(1);
      }

      for (size_type i = 10; i < 12; i++)
      {
        get_vertex(i, *g)->set_type(0);
      }

      for (size_type i = 12; i < 14; i++)
      {
        get_vertex(i, *g)->set_type(1);
      }

      for (size_type i = 14; i < 16; i++)
      {
        get_vertex(i, *g)->set_type(2);
      }

      for (size_type i = 16; i < 17; i++)
      {
        get_vertex(i, *g)->set_type(0);
      }

      for (size_type i = 17; i < 18; i++)
      {
        get_vertex(i, *g)->set_type(1);
      }

      for (size_type i = 18; i < 19; i++)
      {
        get_vertex(i, *g)->set_type(2);
      }

      for (size_type i = 19; i < 20; i++)
      {
        get_vertex(i, *g)->set_type(0);
      }

      for (size_type i = 20; i < 21; i++)
      {
        get_vertex(i, *g)->set_type(2);
      }

      for (size_type i = 21; i < 22; i++)
      {
        get_vertex(i, *g)->set_type(1);
      }

      for (size_type i = 22; i < order; i++)
      {
        get_vertex(i, *g)->set_type(2);
      }

      graph_adapter ga(g);
      test_find_modularity<graph_adapter>(ga, kti.assort_types);

      free(g);
    }
  }
}

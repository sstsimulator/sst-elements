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
/*! \file test_find_degree_distribution.h

    \author Vitus Leung (vjleung@sandia.gov)

    \date 3/3/2008
*/
/****************************************************************************/

#include <cmath>

#include <mtgl/util.hpp>
#include <mtgl/metrics.hpp>

using namespace mtgl;

#ifndef GET_FREQ
#define GET_FREQ

double get_freq()
{
#ifdef __MTA__
  double freq = mta_clock_freq();
#else
  double freq = 1000000;
#endif
  return freq;
}
#endif

template <typename graph>
void test_find_degree_distribution(graph& g)
{
  int ord = num_vertices(g);
  int size = num_edges(g);
  int maxdegree = (int) ceil(log(2. * ord - 1) / log(2.)) + 1;
  double* result = (double*) malloc(maxdegree * sizeof(double));

  find_degree_distribution<graph> fdd(g, result, "dd_test.out");
  int ticks1 = fdd.run();
  fprintf(stdout, "degree distribution:\n");

  for (int i = 0; i < maxdegree; ++i)
  {
    fprintf(stdout, "%2d %lf\n", i, result[i]);
  }

  fprintf(stderr, "RESULT: find_degree_distrbution %lu (%6.2lf, 0)\n",
          size, ticks1/get_freq());

  free(result);
}

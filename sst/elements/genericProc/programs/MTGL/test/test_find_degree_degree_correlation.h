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
/*! \file test_find_degree_degree_correlation.h

    \author Vitus Leung (vjleung@sandia.gov)

    \date 3/4/2008
*/
/****************************************************************************/

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
void test_find_degree_degree_correlation(graph& g)
{
  int ord = num_vertices(g);
  int size = num_edges(g);
  int maxdegree = (int) ceil(log(2. * ord - 1) / log(2.)) + 1;

  double** result = (double**) malloc(maxdegree * sizeof(double*));

  #pragma mta assert parallel
  for (int i = 0; i < maxdegree; ++i)
  {
    result[i] = (double*) malloc(maxdegree * sizeof(double));
  }

  find_degree_degree_correlation<graph> fddc(g, result);
  int ticks1 = fddc.run();
  fprintf(stdout, "degree degree correlation:\n\t");

  for (int j = 1; j < maxdegree; ++j) fprintf(stdout, "%6d ", j);

  fprintf(stdout, "\n");

  for (int i = 1; i < maxdegree; ++i)
  {
    fprintf(stdout, "%7d ", i);

    for (int j = 1; j < maxdegree; ++j) fprintf(stdout, "%.4lf ", result[i][j]);

    fprintf(stdout, "\n");
  }

  for (int i = 0; i < maxdegree; ++i) free(result[i]);
  free(result);
}

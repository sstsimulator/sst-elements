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
/*! \file test_find_assortivity.h

    \author Jon Berry (jberry@sandia.gov)

    \date 12/4/2007
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
void test_find_assortativity(graph& g, int types)
{
  double result;

  find_assortativity<graph> fa(g, &result, types);
//  mta_resume_event_logging();
  int ticks1 = fa.run();
//  mta_suspend_event_logging();

  fprintf(stdout, "assortativity = %lf\n", result);
  fprintf(stderr, "RESULT: find_assortativity %lu (%6.2lf, 0)\n",
  num_edges(g), ticks1 / get_freq());
}

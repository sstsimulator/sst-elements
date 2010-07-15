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
/*! \file generate_plod_graph.hpp

    \brief The Power-law out-degree (PLOD) scale-free graph generator.  Based
           on the algorithm in C. Palmer, J. Steffan, "Generating Network
           Topologies That Obey Power Laws", Proc. GLOBECOM 2000.  Implemented
           in Boost also.

    \author Kamesh Madduri
    \author Greg Mackey (gemacke@sandia.gov)

    \date 12/4/2007
*/
/****************************************************************************/

#ifndef MTGL_GENERATE_PLOD_GRAPH_HPP
#define MTGL_GENERATE_PLOD_GRAPH_HPP

#include <cstdio>
#include <cmath>

#include <mtgl/util.hpp>
#include <mtgl/rand_fill.hpp>
#include <mtgl/mtgl_adapter.hpp>

namespace mtgl {

/****************************************************************************/
/*! \brief The Power-law out-degree (PLOD) scale-free graph generator.  Based
           on the algorithm in C. Palmer, J. Steffan, "Generating Network
           Topologies That Obey Power Laws", Proc. GLOBECOM 2000.  Implemented
           in Boost also.

    \author Kamesh Madduri
    \author Greg Mackey (gemacke@sandia.gov)

    Notes from the Boost PLOD generator documentation:
      http://www.boost.org/libs/graph/doc/plod_generator.html

    The Power Law Out Degree (PLOD) algorithm generates a scale-free graph
    from three parameters, n, alpha, and beta, by allocating a certain number
    of degree "credits" to each vertex, drawn from a power-law distribution.
    It then creates edges between vertices, deducting a credit from each
    involved vertex. The number of credits assigned to a vertex is:
    beta*x-alpha, where x is a random value between 0 and n-1.  The value of
    beta controls the y-intercept of the curve, so that increasing beta
    increases the average degree of vertices. The value of alpha controls how
    steeply the curve drops off, with larger values indicating a steeper
    curve.  The web graph, for instance, has alpha ~ 2.72.
*/
/****************************************************************************/
template <typename graph_adapter>
void
generate_plod_graph(graph_adapter& g,
                    typename graph_traits<graph_adapter>::size_type n,
                    double alpha, double beta)
{
  typedef typename graph_traits<graph_adapter>::size_type size_type;

  int* credits = (int*) malloc(n * sizeof(int));
  random_value* x = (random_value*) malloc(n * sizeof(random_value));

  rand_fill::generate(n, x);
  size_type numedges = 0;

  #pragma mta assert nodep
  for (size_type i = 0; i < n; i++)
  {
    if ((x[i] % n) == 0)
    {
      credits[i] = 0;
    }
    else
    {
      credits[i] = (int) ceil(beta * pow((double) (x[i] % n), -alpha));
    }

    mt_incr(numedges, credits[i]);
  }

  // numedges refers to the no. of undirected edges.
  numedges = numedges / 2;

  free(x);

  // Start adding edges.

  random_value* randVals1 =
    (random_value*) malloc(numedges * sizeof(random_value));
  random_value* randVals2 =
    (random_value*) malloc(numedges * sizeof(random_value));

  size_type m = 0;

  size_type* srcs = (size_type*) malloc(numedges * sizeof(size_type));
  size_type* dests = (size_type*) malloc(numedges * sizeof(size_type));

  size_type* oldVList = (size_type*) malloc(n * sizeof(size_type));
  size_type oldVListSize = n;

  for (size_type i = 0; i < n; i++) oldVList[i] = i;

  size_type* remainingVList = (size_type*) malloc(n * sizeof(size_type));

  while (1)
  {
    size_type numedgesToAdd = numedges - m;

    rand_fill::generate(numedgesToAdd, randVals1);
    rand_fill::generate(numedgesToAdd, randVals2);

    // Store the vertices with non-zero credits in a compact array.
    size_type remainingVertCount = 0;

    #pragma mta assert nodep
    for (size_type i = 0; i < oldVListSize; i++)
    {
      if (credits[oldVList[i]] != 0)
      {
        size_type k = mt_incr(remainingVertCount, 1);
        remainingVList[k] = oldVList[i];
      }
    }

    if (remainingVertCount == 1) break;

    #pragma mta assert parallel
    for (size_type i = 0; i < numedgesToAdd; i++)
    {
      size_type u = remainingVList[randVals1[i] % remainingVertCount];
      size_type v = remainingVList[randVals2[i] % remainingVertCount];

      // Disallow self loops.
      if (u == v) continue;

#ifdef DEBUG
      printf("adding (%lu, %lu)\n", u, v);
#endif

      int credits_u = mt_incr(credits[u], -1);
      int credits_v = mt_incr(credits[v], -1);

      if ((credits_u <= 0) || (credits_v <= 0))
      {
        mt_incr(credits[u], 1);
        mt_incr(credits[v], 1);
        continue;
      }

      size_type e = mt_incr(m, 1);
      srcs[e] = u;
      dests[e] = v;
    }

    if (m > numedges - 3)
    {
      break;
    }
    else
    {
      size_type* tmpVList = oldVList;
      oldVList = remainingVList;
      remainingVList = tmpVList;
      oldVListSize = remainingVertCount;
    }
  }

#ifdef DEBUG
  printf("No. of undirected edges added: %lu\n", m);
#endif

  init(n, m, srcs, dests, g);

  free(randVals1);
  free(randVals2);
  free(credits);
  free(oldVList);
  free(remainingVList);
  free(srcs);
  free(dests);
}

}

#endif

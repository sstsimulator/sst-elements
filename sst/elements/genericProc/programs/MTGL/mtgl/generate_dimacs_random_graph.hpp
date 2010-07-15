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
/*! \file generate_dimacs_random_graph.hpp

    \brief Generates a small world graph.

    \author Kamesh Madduri
    \author Greg Mackey (gemacke@sandia.gov)

    \date 12/4/2007
*/
/****************************************************************************/

#ifndef MTGL_GENERATE_DIMACS_RANDOM_GRAPH_HPP
#define MTGL_GENERATE_DIMACS_RANDOM_GRAPH_HPP

#pragma mta parallel on
#pragma mta parallel multiprocessor

#include <cassert>
#include <cstdlib>
#include <cstdio>

#include <mtgl/util.hpp>
#include <mtgl/rand_fill.hpp>
#include <mtgl/mtgl_adapter.hpp>
#include <mtgl/xmt_hash_set.hpp>

namespace mtgl {

/****************************************************************************/
/*! \brief Generates a small world graph.

    Generates a dimacs random graphs minus self loops and mult edges -
    hamiltonian cycle + m - n random edges.

    The number of vertices in the generated graph will be the maximum of the
    numbers of vertices in the level pairs.  The number of directed edges will
    be the sum of the numbers of edges in the level pairs.  This routine can
    create multiple edges and self loops.

    As an example, consider the following details: (10, 20, 100, 200).  The
    routine will distribute 20 edges among a randomly chosen set of 10
    vertices, then 200 more edges among the 100 total vertices.
*/
/****************************************************************************/
template <typename graph_adapter>
void
generate_dimacs_random_graph(
    graph_adapter& g,
    typename graph_traits<graph_adapter>::size_type scale,
    typename graph_traits<graph_adapter>::size_type average_degree)
{
  typedef typename graph_traits<graph_adapter>::size_type size_type;

  size_type n = 1 << scale;
  size_type m = n * average_degree;    // Adjacencies, really.
  size_type* srcs = (size_type*) malloc(m * sizeof(size_type));
  size_type* dests = (size_type*) malloc(m * sizeof(size_type));

  // Create Hamiltonian cycle part of graph.
  #pragma mta assert nodep
  for (size_type i = 0; i < n - 1; ++i)
  {
    srcs[i] = i;
    dests[i] = i + 1;
  }

  srcs[n - 1] = n - 1;
  dests[n - 1] = 0;

  // Put the Hamiltonian cycle edges in unique_edges.
  xmt_hash_set<size_type> unique_edges;

  #pragma mta assert nodep
  for (size_type i = 0; i < n - 1; ++i)
  {
    size_type key = i * m + i + 1;
    unique_edges.insert(key);
  }
  unique_edges.insert((n - 1) * m);

  // Create remainder of graph which is random edges.
  size_type nrvals = m - n;
  random_value* randVals1 =
    (random_value*) malloc(nrvals * sizeof(random_value));
  random_value* randVals2 =
    (random_value*) malloc(nrvals * sizeof(random_value));

  rand_fill::generate(nrvals, randVals1);
  rand_fill::generate(nrvals, randVals2);

  size_type num_unique_edges = n;

  #pragma mta assert parallel
  for (size_type i = n; i < m; ++i)
  {
    size_type v1 = randVals1[i - n] % n;
    size_type v2 = randVals2[i - n] % n;
    size_type key = v1 * m + v2;

    // Only add unique edges, and skip self loops.
    if (v1 != v2 && unique_edges.insert(key))
    {
      size_type ind = mt_incr(num_unique_edges, 1);
      srcs[ind] = v1;
      dests[ind] = v2;
    }
  }

  free(randVals1);
  free(randVals2);

#ifdef PERMUTE_NODES
  random_permutation(n, m, srcs, dests);
#endif

  init(n, num_unique_edges, srcs, dests, g);

  free(srcs);
  free(dests);
}

}

#endif

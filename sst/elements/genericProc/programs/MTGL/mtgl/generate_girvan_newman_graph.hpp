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
/*! \file generate_girvan_newman_graph.hpp

    \brief Generates the Girvan-Newman benchmark graph that has 4 clusters.

    \author Jon Berry (jberry@sandia.gov)
    \author Greg Mackey (gemacke@sandia.gov)

    \date 9/3/2008
*/
/****************************************************************************/

#ifndef MTGL_GENERATE_GIRVAN_NEWMAN_GRAPH_HPP
#define MTGL_GENERATE_GIRVAN_NEWMAN_GRAPH_HPP

#include <mtgl/util.hpp>
#include <mtgl/rand_fill.hpp>
#include <mtgl/mtgl_adapter.hpp>

namespace mtgl {

/****************************************************************************/
/*! \brief Generates the Girvan-Newman benchmark graph that has 4 clusters.

    \author Jon Berry (jberry@sandia.gov)
    \author Greg Mackey (gemacke@sandia.gov)

    Generates the 4 cluster computer generated graph described in the paper
    "Community structure in social and biological networks" by Girvan and
    Newman.
*/
/****************************************************************************/
template <class graph_adapter>
void generate_girvan_newman_graph(graph_adapter& g, int v_out)
{
  typedef typename graph_traits<graph_adapter>::size_type size_type;

  size_type n = 128;
  size_type max_edges = 128 * 128;

  double p_out = v_out / (double) 96;
  double p_in  = (16 - v_out) / (double) 31;

  random_value rmax = rand_fill::get_max();

  random_value* randVals =
    (random_value*) malloc(max_edges * sizeof(random_value));

  rand_fill::generate(max_edges, randVals);

  size_type num_internal = 0;
  size_type num_external = 0;
  size_type m = 0;

  size_type* srcs = (size_type*) malloc(max_edges * sizeof(size_type));
  size_type* dests = (size_type*) malloc(max_edges * sizeof(size_type));

  #pragma mta assert nodep
  for (size_type i = 0; i < 128; ++i)
  {
    for (size_type j = i + 1; j < 128; ++j)
    {
      size_type ind = i * 128 + j;
      double r = randVals[ind] / (double) rmax;

      if ((i / 32 == j / 32) && (r <= p_in))
      {
        size_type cur_ind = mt_incr(m, 1);
        srcs[cur_ind] = i;
        dests[cur_ind] = j;
        ++num_internal;
      }

      if (((i / 32 != j / 32) && (r <= p_out)) )
      {
        size_type cur_ind = mt_incr(m, 1);
        srcs[cur_ind] = i;
        dests[cur_ind] = j;
        ++num_external;
      }
    }
  }

  init(n, m, srcs, dests, g);

  free(randVals);
  free(srcs);
  free(dests);
}

}

#endif

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
/*! \file random_walk.hpp

    \brief Returns a random walk of specified length through the graph.

    \author Jon Berry (jberry@sandia.gov)
    \author Greg Mackey (gemacke@sandia.gov)

    \date 12/4/2007

    This algorithm visits edges based on DIRECTED travel.  From a vertex
    in a directed or bidirectional graph, all out edges are visited.  The
    result for a directed and a bidirectional graph should be the same when
    the algorithm is run in serial.  From a vertex in an undirected graph,
    all adjacent edges are visited which means that each edge is visited twice,
    once from each endpoint.

    For the algorithm to be able to visit all vertices in an undirected graph,
    the graph must be connected.  If the graph is not connected, the algorithm
    will still complete, but it will only visit the vertices in the component
    in which it starts.  However, if the walk is started on a vertex that
    has no neighbors, the algorithm will assert an error and die.

    For the algorithm to be able to visit all vertices in a directed or
    bidirectional graph, the graph must be strongly connected.  If the graph
    is not strongly connected, the algorithm will still complete as long as it
    doesn't encounter a sink.  The parts of the graph that the algorithm
    couldn't reach will just not be included in the result.  If the algorithm
    encounters a sink, however, it will assert an error and die.

    A weakly connected directed graph can be made strongly connected by
    creating a new graph that contains each edge from the original graph and
    its reverse.  In fact, the resulting graph is Eulerian, as well.  The
    duplicate_adapter is an easy way to do this.
*/
/****************************************************************************/

#ifndef MTGL_RANDOM_WALK_HPP
#define MTGL_RANDOM_WALK_HPP

#include <mtgl/util.hpp>
#include <mtgl/psearch.hpp>
#include <mtgl/rand_fill.hpp>
#include <mtgl/dynamic_array.hpp>

namespace mtgl {

template <class graph_adapter>
class random_walk_visitor : public default_psearch_visitor<graph_adapter> {
public:
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge;
  typedef typename graph_traits<graph_adapter>::size_type size_type;

  random_walk_visitor(size_type* ids, size_type target_plen,
                      size_type& cur_plen, random_value* rvals,
                      graph_adapter& g_) :
      g(g_), path_ids(ids), target_path_length(target_plen),
      path_length(cur_plen), randvals(rvals), rand_index(0),
      vid_map(get(_vertex_id_map, g)), eid_map(get(_edge_id_map, g)),
      tokens(num_vertices(g)) {}

  void pre_visit(vertex& v)
  {
    // Set the token for this vertex to 0 before it is visited.  Each outgoing
    // edge adjacent to this vertex will grab a different numbered token.  A
    // random number generator is used to choose a token and thus an edge to
    // visit.
    tokens[get(vid_map, v)] = 0;

    size_type ind = mt_incr(path_length, 1);
    path_ids[ind] = get(vid_map, v);
    size_type ind_next_rand = mt_incr(rand_index, 1);
    size_type deg = out_degree(v, g);

    // Uh-oh!  We hit a sink.  Need to die now.
    assert(deg != 0);

    // Choose the blessed token.
    randnum = randvals[ind_next_rand] % deg;
  }

  int visit_test(edge& e, vertex& v)
  {
    if (path_length >= target_path_length)  return false;

    // Grab my token from my source vertex and check if my token is the
    // blessed one.
    size_type my_token = mt_incr(tokens[get(vid_map, v)], 1);
    if (my_token == randnum)  return true;

    return false;
  }

  void tree_edge(edge& e, vertex& v) const
  {
    size_type ind = mt_incr(path_length, 1);

    path_ids[ind] = get(eid_map, e);
  }

private:
  graph_adapter& g;
  size_type* path_ids;
  size_type target_path_length;
  size_type& path_length;
  random_value* randvals;
  size_type rand_index;
  random_value randnum;
  vertex_id_map<graph_adapter> vid_map;
  edge_id_map<graph_adapter> eid_map;
  dynamic_array<size_type> tokens;
};

template <class graph_adapter>
void random_walk(graph_adapter g,
                 typename graph_traits<graph_adapter>::vertex_descriptor v,
                 typename graph_traits<graph_adapter>::size_type path_length,
                 typename graph_traits<graph_adapter>::size_type* path_ids)
{
  typedef typename graph_traits<graph_adapter>::size_type size_type;

  size_type order = num_vertices(g);

  int* search_color = (int*) malloc(order * sizeof(int));

  #pragma mta assert nodep
  for (size_type i = 0; i < order; ++i)  search_color[i] = 0;

  size_type cur_length = 0;
  size_type array_length = path_length / 2 + 1;

  random_value* rvals =
    (random_value*) malloc(array_length * sizeof(random_value));
  rand_fill::generate(array_length, rvals);

  random_walk_visitor<graph_adapter> rand_vis(path_ids, path_length,
                                              cur_length, rvals, g);

  psearch<graph_adapter, int*, random_walk_visitor<graph_adapter>,
          PURE_FILTER, DIRECTED> psrch(g, search_color, rand_vis);

  psrch.run(v);

  free(rvals);
  free(search_color);
}

}

#endif

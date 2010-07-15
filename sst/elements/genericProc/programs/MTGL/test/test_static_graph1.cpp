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
/*! \file test_static_graph1.cpp

    \author Jon Berry (jberry@sandia.gov)

    \date 1/4/2008
*/
/****************************************************************************/

#include <iostream>

#include <mtgl/static_graph_adapter.hpp>

using namespace std;
using namespace mtgl;

typedef directedS DIRECTION;

void print(static_graph_adapter<DIRECTION>& g)
{
  typedef static_graph_adapter<DIRECTION> Graph;
  typedef graph_traits<Graph>::vertex_descriptor vertex_descriptor_t;
  typedef graph_traits<Graph>::vertex_iterator vertex_iterator;
  typedef graph_traits<Graph>::adjacency_iterator adjacency_iterator;
  typedef graph_traits<Graph>::size_type size_type;

  size_type n = num_vertices(g);

  vertex_iterator verts = vertices(g);
  vertex_id_map<Graph> vid_map = get(_vertex_id_map, g);

  for (size_type i = 0; i < n; i++)
  {
    vertex_descriptor_t u = verts[i];
    size_type deg = out_degree(u, g);
    size_type uid = get(vid_map, u);

    cout << uid << " ";
    adjacency_iterator adjs = adjacent_vertices(u, g);

    for (size_type j = 0; j < deg; j++)
    {
      vertex_descriptor_t neighbor = adjs[j];
      size_type nid = get(vid_map, neighbor);

      cout << nid << " ";
    }

    cout << endl;
  }
}

int main()
{
  unsigned long srcs[] = { 0, 1, 2 };
  unsigned long dests[] = { 1, 2, 0 };
  unsigned long n = 3;

  static_graph_adapter<DIRECTION> ga;
  init(n, n, srcs, dests, ga);

  print(ga);

  return 0;
}

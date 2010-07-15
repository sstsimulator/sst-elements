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
/*! \file test_mutable_graph.cpp

    \brief Tests the mutable concepts included in adjacency_list_adapter.

    \author Greg Mackey (gemacke@sandia.gov)

    \date 6/15/2010
*/
/****************************************************************************/

#include <mtgl/adjacency_list_adapter.hpp>

#include <cstdlib>

using namespace mtgl;

typedef directedS DIRECTION;

template <typename Graph>
void print_my_graph(Graph& g)
{
  typedef typename graph_traits<Graph>::size_type size_type;
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;
  typedef typename graph_traits<Graph>::vertex_iterator vertex_iterator;
  typedef typename graph_traits<Graph>::adjacency_iterator adjacency_iterator;

  vertex_id_map<Graph> vid_map = get(_vertex_id_map, g);

  vertex_iterator verts = vertices(g);
  size_type order = num_vertices(g);
  for (size_type i = 0; i < order; i++)
  {
    vertex_descriptor u = verts[i];
    size_type uid = get(vid_map, u);
    adjacency_iterator adjs = adjacent_vertices(u, g);
    size_type end = out_degree(u, g);
    for (size_type j = 0; j < end; j++)
    {
      vertex_descriptor v = adjs[j];
      size_type vid = get(vid_map, v);
      printf("(%lu, %lu)\n", uid, vid);
    }
  }
}

int main(int argc, char *argv[])
{
  typedef adjacency_list_adapter<DIRECTION> Graph;

  typedef graph_traits<Graph>::size_type size_type;
  typedef graph_traits<Graph>::vertex_descriptor vertex_descriptor;
  typedef graph_traits<Graph>::edge_descriptor edge_descriptor;
  typedef graph_traits<Graph>::vertex_iterator vertex_iterator;
  typedef graph_traits<Graph>::edge_iterator edge_iterator;
  typedef graph_traits<Graph>::adjacency_iterator adjacency_iterator;
  typedef graph_traits<Graph>::out_edge_iterator out_edge_iterator;

  Graph g;

  // Initialize graph.

#if 0
  // Test case 1.
  const size_type numVerts = 8;
  const size_type numEdges = 9;

  size_type sources[numEdges] = { 0, 1, 2, 2, 3, 4, 5, 6, 6 };
  size_type targets[numEdges] = { 7, 7, 0, 2, 0, 3, 4, 0, 2 };

//  size_type sources[numEdges] = { 1, 2, 4, 6, 0, 2, 3, 5, 6 };
//  size_type targets[numEdges] = { 7, 2, 3, 0, 7, 0, 0, 4, 2 };
#endif

#if 0
  // Test case 2.
  const size_type numVerts = 12;
  const size_type numEdges = 11;

  size_type sources[numEdges] = { 0, 1, 2, 3, 5, 6, 6, 7, 7,  9, 10 };
  size_type targets[numEdges] = { 1, 2, 3, 4, 4, 3, 9, 6, 8, 10, 11 };
#endif

  // Test case 3.
  const size_type numVerts = 6;
  const size_type numEdges = 8;

  size_type sources[numEdges] = { 0, 0, 1, 1, 2, 3, 3, 4 };
  size_type targets[numEdges] = { 1, 2, 2, 3, 4, 4, 5, 5 };

  add_vertices(numVerts, g);
  add_edges(numEdges, sources, targets, g);

  size_type order = num_vertices(g);
  size_type size = num_edges(g);
  printf("order: %lu\n", order);
  printf(" size: %lu\n\n", size);

  vertex_id_map<Graph> vipm = get(_vertex_id_map, g);
  edge_id_map<Graph> eipm = get(_edge_id_map, g);

  printf("Testing get_vertex():\n");
  size_type largest_vid = 0;
  for (size_type i = 0; i < numVerts; ++i)
  {
    vertex_descriptor v = get_vertex(i, g);
    size_type vid = get(vipm, v);
    if (vid > largest_vid) largest_vid = vid;
  }
  printf("Largest vid: %lu\n\n", largest_vid);

  printf("Test vertex iterator and degree functions:\n");
  vertex_iterator vIter = vertices(g);
  largest_vid = 0;
  for (size_type i = 0; i < order; ++i)
  {
    vertex_descriptor v = vIter[i];
    size_type vid = get(vipm, v);
    if (vid > largest_vid) largest_vid = vid;
  }
  printf("Largest vid: %lu\n\n", largest_vid);

  printf("Test get_edge():\n");
  size_type largest_eid = 0;
  for (size_type i = 0; i < size; ++i)
  {
    edge_descriptor e = get_edge(i, g);
    size_type eid = get(eipm, e);
    if (eid > largest_eid) largest_eid = eid;
  }
  printf("Largest eid: %lu\n\n", largest_eid);

  printf("Test edge iterator:\n");
  edge_iterator eIter = edges(g);
  largest_eid = 0;
  for (size_type i = 0; i < size; ++i)
  {
    edge_descriptor e = eIter[i];
    size_type eid = get(eipm, e);
    if (eid > largest_eid) largest_eid = eid;
  }
  printf("Largest eid: %lu\n\n", largest_eid);

  printf("Test adjacent vertex iterator:\n");
  size_type adj_sum = 0;
  for (size_type i = 0; i < order; ++i)
  {
    vertex_descriptor v = vIter[i];

    size_type out_deg = out_degree(v, g);
    adjacency_iterator adjIter = adjacent_vertices(v, g);
    for (size_type j = 0; j < out_deg; ++j)
    {
      size_type vid2 = get(vipm, adjIter[j]);
      adj_sum += vid2;
    }
  }
  printf("adj_sum: %lu\n\n", adj_sum);

  printf("Test incident edge iterator:\n");
  size_type adj_edge_sum = 0;
  for (size_type i = 0; i < order; ++i)
  {
    vertex_descriptor v = vIter[i];

    size_type out_deg = out_degree(v, g);
    out_edge_iterator oeIter = out_edges(v, g);
    for (size_type j = 0; j < out_deg; ++j)
    {
      edge_descriptor e = oeIter[j];
      size_type eid = get(eipm, e);
      adj_edge_sum += eid;
    }
  }
  printf("adj_edge_sum: %lu\n\n", adj_edge_sum);

//  g.print();
  printf("\n");
  print(g);
  printf("\n");

  size_type rEdges[3] = { 2, 6, 5 };
  remove_edges(3, rEdges, g);

//  size_type rVerts[2] = { 2, 3 };
//  remove_vertices(2, rVerts, g);

/*
  vertex_descriptor rVerts[2];
  rVerts[0] = get_vertex(2, g);
  rVerts[1] = get_vertex(3, g);

  g.get_graph()->removeVertices(2, rVerts);
*/

  print(g);
  printf("\n");
  clear(g);

  return 0;
}

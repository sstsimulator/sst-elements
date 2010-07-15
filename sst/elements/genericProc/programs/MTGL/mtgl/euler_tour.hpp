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
/*! \file euler_tour.hpp

    \brief Creates an Euler tour through the graph using Hierholzer's
           algorithm.

    \author Greg Mackey (gemacke@sandia.gov)

    \date 4/2008

    This algorithm works for both directed and undirected graphs.  The
    algorithm assumes that the graph is Eulerian (i.e. an Euler tour exists).
    For undirected graphs this requires that the graph is connected and every
    vertex has even degree.  For directed graphs, this requires that the graph
    is strongly connected and the indegree of every vertex is equal to its
    outdegree.  The algorithm returns a boolean indicating if it was
    successful or not (i.e. if the graph is eulerian or not).  If the graph
    isn't eulerian, the returned path is invalid.

    The path returned by the algorithm is composed of a sequence of
    alternating vertex and edge ids representing the vertices and edges
    encountered in the path.

      (vertex id, edge id, vertex id, edge id, vertex id ...)

    A path of any length can be created by the algorithm.  It rounds the
    length of the requested path up to the length of the next whole Euler tour
    and calculates the whole Euler tour.  Then, only the necessary part of the
    last Euler tour is returned as the result.  For example, consider a graph
    with 8 edges.  We want a path with 10 edges, so the path length would be
    21.  The algorithm calculates an Euler tour and marks the first 17 entries
    in the path_ids (9 vertices and 8 edges).  The algorithm next calculates
    another Euler tour and uses the first two edge from the tour to fill out
    the remaining path_ids.  Thus, the returned path would have a whole Euler
    tour (8 edges) plus the first 2 edges from a second Euler tour.

    Any connected undirected graph can be made Eulerian by creating a new
    graph with two copies of each edge in the original graph.  Any weakly
    connected directed graph can be made Eulerian by creating a new graph
    that contains each edge from the original graph and its reverse.  The
    duplicate_adapter is an easy way to do this for both undirected and
    directed graphs.
*/
/****************************************************************************/

#ifndef MTGL_EULER_TOUR_HPP
#define MTGL_EULER_TOUR_HPP

#include <limits>
#include <mtgl/random.h>

namespace mtgl {

template <class graph_adapter>
bool euler_tour(graph_adapter& g,
                typename graph_traits<graph_adapter>::vertex_descriptor startVertex,
                typename graph_traits<graph_adapter>::size_type path_length,
                typename graph_traits<graph_adapter>::size_type* path_ids)
{
  typedef typename graph_traits<graph_adapter>::size_type size_type;
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge;
  typedef typename graph_traits<graph_adapter>::vertex_iterator vertex_iterator;
  typedef typename graph_traits<graph_adapter>::edge_iterator edge_iterator;
  typedef typename graph_traits<graph_adapter>::out_edge_iterator
          out_edge_iterator;

  vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, g);
  edge_id_map<graph_adapter> eid_map = get(_edge_id_map, g);

  size_type random_rank = 0;
  random rng;

  size_type order = num_vertices(g);
  size_type size = num_edges(g);
  size_type num_tours = (path_length / 2 + size - 1) / size;
  size_type tour_length = 2 * size;
  size_type last_tour_length = (path_length % tour_length) - 1;
  if (last_tour_length == 0) last_tour_length = tour_length;

  // Keeps track of the number of edges adjacent to a vertex that have not
  // been used yet in the partial Euler tour.  This lets us easily find a
  // vertex in the partial tour to start the next cycle from.
  size_type* numUnusedAdjEdges = new size_type[order];

  // Indexed by edge id, each entry in the vector contains an edge id that
  // points to the next edge in the euler tour.  A value of max_int
  // indicates the edge is not yet on the tour.
  size_type* visitedEdges = new size_type[size];

  // Unused edges is temporary storage to hold the unused edges adjacent to a
  // vertex.  The number of adjacent edges to a vertex is bounded by the
  // number of edges in the graph.
  size_type* unusedEdges = new size_type[size];
  size_type numUnusedEdges;

  edge_iterator edgs = edges(g);
  vertex_iterator verts = vertices(g);

  for (size_type tour_id = 0; tour_id < num_tours; ++tour_id)
  {
    // Reset numUnusedAdjEdges and visitedEdges.
    for (size_type i = 0; i < order; ++i)
    {
      numUnusedAdjEdges[i] = out_degree(verts[i], g);
    }
    for (size_type i = 0; i < size; ++i)
    {
      visitedEdges[i] = std::numeric_limits<size_type>::max();
    }

    // Add the first edge to the tour.
    vertex cycleStartVertex = startVertex;
    size_type cycleStartVertexId = get(vid_map, cycleStartVertex);
    --numUnusedAdjEdges[get(vid_map, startVertex)];

    out_edge_iterator eit = out_edges(startVertex, g);
    size_type deg = out_degree(startVertex, g);

    if (deg == 0) return false;

    // Get a random entry from the out edges of the starting vertex.
    size_type rank = mt_incr(random_rank, 1);
    size_type index = rng.nthValue(rank) % deg;
    size_type curEdgeId = get(eid_map, eit[index]);
    edge curEdge = edgs[curEdgeId];

    size_type startEdgeId = curEdgeId;
    size_type nextCycleEdgeId = startEdgeId;
    size_type numAddedEdges = 1;

    size_type prevEdgeId = startEdgeId;
    vertex curVert = other(curEdge, startVertex, g);
    size_type curVertId = get(vid_map, curVert);

    if (is_undirected(g)) --numUnusedAdjEdges[curVertId];

    // Now, add the remaining edges to the tour.
    while (numAddedEdges < size)
    {
      // Create a cycle.  (The first edge of the cycle has already been
      // added.)
      while (curVertId != cycleStartVertexId)
      {
        --numUnusedAdjEdges[curVertId];

        // Randomly pick an edge from the unvisited edges of the vertex.
        numUnusedEdges = 0;
        size_type deg = out_degree(curVert, g);
        out_edge_iterator eit2 = out_edges(curVert, g);

        // Put the unvisited edges of the vertex in an array.
        #pragma mta assert nodep
        for (size_type i = 0; i < deg; ++i)
        {
          edge e = eit2[i];
          size_type eid = get(eid_map, e);

          if (visitedEdges[eid] == std::numeric_limits<size_type>::max() &&
              eid != prevEdgeId)
          {
            size_type idx = mt_incr(numUnusedEdges, 1);
            unusedEdges[idx] = eid;
          }
        }
        if (numUnusedEdges == 0) return false;

        // Get a random entry into the unvisited edges array.
        rank = mt_incr(random_rank, 1);
        index = rng.nthValue(rank) % numUnusedEdges;
        curEdgeId = unusedEdges[index];
        curEdge = edgs[curEdgeId];

        visitedEdges[prevEdgeId] = curEdgeId;
        prevEdgeId = curEdgeId;
        curVert = other(curEdge, curVert, g);
        curVertId = get(vid_map, curVert);
        if (is_undirected(g)) --numUnusedAdjEdges[curVertId];
        ++numAddedEdges;
      }

      // Add this edge to close the loop of the cycle.
      visitedEdges[curEdgeId] = nextCycleEdgeId;

      // If you haven't traveled all the edges yet, find a vertex in the
      // cycle that has an adjacent edge that isn't in the cycle.
      if (numAddedEdges < size)
      {
        size_type edgeCount = 0;

        while (numUnusedAdjEdges[curVertId] == 0)
        {
          // Move the current edge.
          curEdgeId = visitedEdges[prevEdgeId];

          // TODO: Is this statement necessary?
          if (curEdgeId == std::numeric_limits<size_type>::max())
          {
            curEdgeId = startEdgeId;
          }

          curEdge = edgs[curEdgeId];

          // Move to the next vertex and update the previous edge.
          curVert = other(curEdge, curVert, g);
          curVertId = get(vid_map, curVert);
          prevEdgeId = curEdgeId;

          ++edgeCount;

          if (edgeCount > numAddedEdges + 1) return false;
        }

        nextCycleEdgeId = visitedEdges[curEdgeId];

        // Add the first edge of the new cycle.
        cycleStartVertex = curVert;
        cycleStartVertexId = get(vid_map, cycleStartVertex);
        --numUnusedAdjEdges[cycleStartVertexId];

        // Randomly pick an edge from the unvisited edges of the vertex.
        numUnusedEdges = 0;
        size_type deg = out_degree(curVert, g);
        out_edge_iterator eit2 = out_edges(curVert, g);

        // Put the unvisited edges of the vertex in an array.
        #pragma mta assert nodep
        for (size_type i = 0; i < deg; ++i)
        {
          edge e = eit2[i];
          size_type eid = get(eid_map, e);

          if (visitedEdges[eid] == std::numeric_limits<size_type>::max() &&
              eid != prevEdgeId)
          {
            size_type idx = mt_incr(numUnusedEdges, 1);
            unusedEdges[idx] = eid;
          }
        }
        if (numUnusedEdges == 0) return false;

        // Get a random entry into the unvisited edges array.
        rank = mt_incr(random_rank, 1);
        index = rng.nthValue(rank) % numUnusedEdges;
        curEdgeId = unusedEdges[index];
        curEdge = edgs[curEdgeId];

        visitedEdges[prevEdgeId] = curEdgeId;
        prevEdgeId = curEdgeId;
        ++numAddedEdges;
        curVert = other(curEdge, curVert, g);
        curVertId = get(vid_map, curVert);
        if (is_undirected(g)) --numUnusedAdjEdges[curVertId];
      }
    }

    // Travel the tour to fill the arrays that are returned with the result.

    // Update the first vertex values.
    path_ids[0 + tour_id * tour_length] = get(vid_map, startVertex);

    curEdgeId = startEdgeId;
    curEdge = edgs[curEdgeId];
    curVert = other(curEdge, startVertex, g);

    size_type this_tour_length = tour_id + 1 == num_tours ? last_tour_length :
                                 tour_length;

    for (size_type i = 2; i <= this_tour_length; i += 2)
    {
      curVertId = get(vid_map, curVert);

      // Update edge values.
      path_ids[i - 1 + tour_id * tour_length] = curEdgeId;

      // Update vertex values.
      path_ids[i + tour_id * tour_length] = curVertId;

      // Move edge and vertex forward.
      curEdgeId = visitedEdges[curEdgeId];

      // Check for the end of the last added cycle that wasn't closed.
      if (curEdgeId == std::numeric_limits<size_type>::max())
      {
        curEdgeId = nextCycleEdgeId;
      }

      if (curEdgeId == std::numeric_limits<size_type>::max()) return false;

      curEdge = edgs[curEdgeId];
      curVert = other(curEdge, curVert, g);
    }
  }

  delete [] numUnusedAdjEdges;
  delete [] visitedEdges;
  delete [] unusedEdges;

  return true;
}

}

#endif

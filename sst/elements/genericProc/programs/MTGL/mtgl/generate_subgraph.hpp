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
/*! \file generate_subgraph.hpp

    \brief Randomly generates a subgraph of a large graph.

    \author Greg Mackey (gemacke@sandia.gov)

    \date 9/11/2008

    This algorithm generates an edge induced subgraph that has no more than m
    edges.  The algorithm attempts to limit the number of vertices to n, but
    a slightly higher number of vertices could be included in the subgraph.

    The algorithm uses a psearch to create a list of edges and vertices
    included in the subgraph.  The number of vertices returned by the psearch
    will be n or less, and the number of edges returned by the psearch will be
    m or less.  An edge induced subgraph is created from the returned edges.
    The psearch allows for an edge to be included with one endpoint not
    included in the vertex list.  In this case, the extra vertex is added to
    the subgraph increasing the number of vertices.

    This algorithm visits edges based on DIRECTED travel.  From a vertex
    in a directed or bidirectional graph, all out edges are visited.  The
    result for a directed and a bidirectional graph should be the same when
    the algorithm is run in serial.  From a vertex in an undirected graph,
    all adjacent edges are visited which means that each edge is visited twice,
    once from each endpoint.
*/
/****************************************************************************/

#ifndef MTGL_GENERATE_SUBGRAPH_HPP
#define MTGL_GENERATE_SUBGRAPH_HPP

#include <mtgl/psearch.hpp>
#include <mtgl/subgraph_adapter.hpp>

namespace mtgl {

template <typename graph_adapter>
class generate_subgraph_visitor : public default_psearch_visitor<graph_adapter> {
public:
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge;
  typedef typename graph_traits<graph_adapter>::size_type size_type;

  generate_subgraph_visitor(size_type n, size_type m, bool *vmsk, bool *emsk,
                            graph_adapter& graph, size_type& cv,
                            size_type& ce) :
      desired_verts(n), desired_edges(m), vmask(vmsk), emask(emsk),
      created_verts(cv), created_edges(ce), vid_map(get(_vertex_id_map, graph)),
      eid_map(get(_edge_id_map, graph)), g(graph) {}

  void pre_visit(vertex& v)
  {
    size_type next = mt_incr(created_verts, 1);

    if (next < desired_verts)  vmask[get(vid_map, v)] = true;
  }

  int visit_test(edge& e, vertex& v)
  {
    size_type vert_count = created_verts +
                           (vmask[get(vid_map, other(e, v, g))] ? 0 : 1);

    return (vmask[get(vid_map, v)] && vert_count <= desired_verts &&
            created_edges < desired_edges);
  }

  void tree_edge(edge& e, vertex& v)
  {
    if (emask[get(eid_map, e)])  return;

    size_type next = mt_incr(created_edges, 1);

    if (next < desired_edges)  emask[get(eid_map, e)] = true;
  }

  void back_edge(edge& e, vertex& v)
  {
    if (emask[get(eid_map, e)])  return;

    size_type next = mt_incr(created_edges, 1);

    if (next < desired_edges)  emask[get(eid_map, e)] = true;
  }

private:
  size_type desired_verts;
  size_type desired_edges;
  bool* vmask;
  bool* emask;
  size_type& created_verts;
  size_type& created_edges;
  vertex_id_map<graph_adapter> vid_map;
  edge_id_map<graph_adapter> eid_map;
  graph_adapter& g;
};

/***/

template <class graph_adapter>
void generate_subgraph(
    typename graph_traits<graph_adapter>::size_type desired_verts,
    typename graph_traits<graph_adapter>::size_type desired_edges,
    graph_adapter& g, subgraph_adapter<graph_adapter>& subg)
{
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge;
  typedef typename graph_traits<graph_adapter>::size_type size_type;

  // The bitmap indicating which edges are in the subgraph.
  bool* subgraph_edges = new bool[num_edges(g)];
  for (size_type i = 0; i < num_edges(g); ++i)  subgraph_edges[i] = false;

  // The bitmap indicating which vertices are in the subgraph.
  bool* subgraph_verts = new bool[num_vertices(g)];
  for (size_type i = 0; i < num_vertices(g); ++i)  subgraph_verts[i] = false;

  // These are counters that are shared by all the copies of the visitors.
  size_type created_verts = 0;
  size_type created_edges = 0;

  generate_subgraph_visitor<graph_adapter>
      gen_sg_vis(desired_verts, desired_edges, subgraph_verts,
                 subgraph_edges, g, created_verts, created_edges);

  psearch_high_low<graph_adapter, generate_subgraph_visitor<graph_adapter>,
                   AND_FILTER, DIRECTED> psrch(g, gen_sg_vis, 1);
  psrch.run();

  init_edges(subgraph_edges, subg);

  delete [] subgraph_edges;
  delete [] subgraph_verts;
}

}

#endif

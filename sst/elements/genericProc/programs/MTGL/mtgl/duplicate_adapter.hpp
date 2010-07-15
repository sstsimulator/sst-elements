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
/*! \file contraction_adapter.hpp

    \brief This adapter provides an interface to take an existing graph
           adapter and create a new one that has two copies of every edge in
           the original graph.  It stores a reference to the original graph
           and an entire copy of the duplicate graph.

    \author Greg Mackey (gemacke@sandia.gov)

    \date 3/2008

    For undirected graphs each edge is just duplicated.  For directed graphs
    each edge is duplicated, but the duplicate's direction is the reverse of
    the original edge's direction.

    The duplicate graph keeps associations from both new edges back to the
    original single edge.  The vertex ids in the duplicate graph correspond to
    the vertex ids in the original graph.  An edge id of an edge from the
    original graph corresponds to the edge id of the first copy of the edge in
    the duplicate graph.  The edge id of the duplicate edge in the duplicate
    graph is equal to the edge id of the edge from the original graph plus the
    number of edges in the original graph.  For instance, an original graph
    has 100 edges.  For original edge with id 39, the first copy of the edge
    in the duplicate graph has id 39 while the duplicate copy has id 139.
*/
/****************************************************************************/

#ifndef MTGL_DUPLICATE_ADAPTER_HPP
#define MTGL_DUPLICATE_ADAPTER_HPP

#include <mtgl/mtgl_adapter.hpp>
#include <mtgl/static_graph_adapter.hpp>

namespace mtgl {

template <typename graph_adapter>
class duplicate_adapter {
public:
  typedef graph_traits<graph_adapter> base_traits;
  typedef typename base_traits::size_type base_size_type;
  typedef typename base_traits::edge_descriptor base_edge_descriptor;
  typedef typename base_traits::edge_iterator base_edge_iterator;
  typedef typename base_traits::directed_category base_directed_category;

  typedef static_graph_adapter<base_directed_category> wrapper_adapter;
  typedef graph_traits<wrapper_adapter> traits;
  typedef typename traits::size_type size_type;
  typedef typename traits::vertex_descriptor vertex_descriptor;
  typedef typename traits::edge_descriptor edge_descriptor;
  typedef typename traits::vertex_iterator vertex_iterator;
  typedef typename traits::adjacency_iterator adjacency_iterator;
  typedef typename traits::in_adjacency_iterator in_adjacency_iterator;
  typedef typename traits::edge_iterator edge_iterator;
  typedef typename traits::out_edge_iterator out_edge_iterator;
  typedef typename traits::in_edge_iterator in_edge_iterator;
  typedef typename traits::directed_category directed_category;
  typedef typename traits::graph_type graph_type;

  duplicate_adapter(graph_adapter& g) : original_graph(&g)
  {
    base_size_type order = num_vertices(*original_graph);
    base_size_type size = num_edges(*original_graph);

    base_edge_iterator edgs = edges(*original_graph);

    vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, *original_graph);

    size_type* e_srcs = new size_type[size * 2];
    size_type* e_dests = new size_type[size * 2];

    // Get edge source and target ids.
    #pragma mta assert parallel
    for (base_size_type i = 0; i < size; ++i)
    {
      base_edge_descriptor e = edgs[i];

      // Store the first copy of the original edge.
      e_srcs[i] = get(vid_map, source(e, *original_graph));
      e_dests[i] = get(vid_map, target(e, *original_graph));

      // Store the duplicate copy of the original edge.
      if (is_undirected(g))
      {
        e_srcs[i + size] = e_srcs[i];
        e_dests[i + size] = e_dests[i];
      }
      else
      {
        e_srcs[i + size] = e_dests[i];
        e_dests[i + size] = e_srcs[i];
      }
    }

    // Initialize the duplicate graph.  This is assuming that the init()
    // method of the underlying graph implements parallelization correctly
    // and efficiently.
    init(order, size * 2, e_srcs, e_dests, duplicate_graph);

    delete [] e_srcs;
    delete [] e_dests;
  }

  // The compiler-synthesized copy control works fine for this class, so we
  // don't implement it ourselves.  We assume that the graph adapter passed
  // as a template parameter has a correctly implemented deep copy which makes
  // the duplicate adapter perform a deep copy.

  graph_type* get_graph() const { return duplicate_graph.get_graph(); }
  const wrapper_adapter& get_adapter() const { return duplicate_graph; }
  const graph_adapter& get_original_adapter() const { return *original_graph; }

  void print() { duplicate_graph.print(); }

private:
  graph_adapter* original_graph;
  wrapper_adapter duplicate_graph;
};

/***/

template <typename G>
inline
typename duplicate_adapter<G>::vertex_descriptor
get_vertex(typename duplicate_adapter<G>::size_type i,
           const duplicate_adapter<G>& dg)
{
  return get_vertex(i, dg.get_adapter());
}

/***/

template <typename G>
inline
typename duplicate_adapter<G>::edge_descriptor
get_edge(typename duplicate_adapter<G>::size_type i,
         const duplicate_adapter<G>& dg)
{
  return get_edge(i, dg.get_adapter());
}

/***/

template <typename G>
inline
typename duplicate_adapter<G>::size_type
num_vertices(const duplicate_adapter<G>& dg)
{
  return num_vertices(dg.get_adapter());
}

/***/

template <typename G>
inline
typename duplicate_adapter<G>::size_type
num_edges(const duplicate_adapter<G>& dg)
{
  return num_edges(dg.get_adapter());
}

/***/

template <typename G>
inline
typename duplicate_adapter<G>::vertex_descriptor
source(const typename duplicate_adapter<G>::edge_descriptor& e,
       const duplicate_adapter<G>& dg)
{
  return source(e, dg.get_adapter());
}

/***/

template <typename G>
inline
typename duplicate_adapter<G>::vertex_descriptor
target(const typename duplicate_adapter<G>::edge_descriptor& e,
       const duplicate_adapter<G>& dg)
{
  return target(e, dg.get_adapter());
}

/***/

template <typename G>
inline
typename duplicate_adapter<G>::vertex_descriptor
other(const typename duplicate_adapter<G>::edge_descriptor& e,
      const typename duplicate_adapter<G>::vertex_descriptor& u_local,
      const duplicate_adapter<G>& dg)
{
  return other(e, u_local, dg.get_adapter());
}

/***/

template <typename G>
inline
typename duplicate_adapter<G>::size_type
degree(const typename duplicate_adapter<G>::vertex_descriptor& u_local,
       const duplicate_adapter<G>& dg)
{
  return degree(u_local, dg.get_adapter());
}

/***/

template <typename G>
inline
typename duplicate_adapter<G>::size_type
out_degree(const typename duplicate_adapter<G>::vertex_descriptor& u_local,
           const duplicate_adapter<G>& dg)
{
  return out_degree(u_local, dg.get_adapter());
}

/***/

template <typename G>
inline
typename duplicate_adapter<G>::size_type
in_degree(const typename duplicate_adapter<G>::vertex_descriptor& u_local,
          const duplicate_adapter<G>& dg)
{
  return in_degree(u_local, dg.get_adapter());
}

/***/

template <typename G>
inline
typename duplicate_adapter<G>::vertex_iterator
vertices(const duplicate_adapter<G>& dg)
{
  return vertices(dg.get_adapter());
}

/***/

template <typename G>
inline
typename duplicate_adapter<G>::edge_iterator
edges(const duplicate_adapter<G>& dg)
{
  return edges(dg.get_adapter());
}

/***/

template <typename G>
inline
typename duplicate_adapter<G>::adjacency_iterator
adjacent_vertices(const typename duplicate_adapter<G>::vertex_descriptor& v,
                  const duplicate_adapter<G>& dg)
{
  return adjacent_vertices(v, dg.get_adapter());
}

/***/

template <typename G>
inline
typename duplicate_adapter<G>::in_adjacency_iterator
in_adjacent_vertices(const typename duplicate_adapter<G>::vertex_descriptor& v,
                     const duplicate_adapter<G>& dg)
{
  return in_adjacent_vertices(v, dg.get_adapter());
}

/***/

template <typename G>
inline
typename duplicate_adapter<G>::out_edge_iterator
out_edges(const typename duplicate_adapter<G>::vertex_descriptor& v,
          const duplicate_adapter<G>& dg)
{
  return out_edges(v, dg.get_adapter());
}

/***/

template <typename G>
inline
typename duplicate_adapter<G>::in_edge_iterator
in_edges(const typename duplicate_adapter<G>::vertex_descriptor& v,
         const duplicate_adapter<G>& dg)
{
  return in_edges(v, dg.get_adapter());
}

/***/

template <typename G>
inline
bool
is_directed(const duplicate_adapter<G>& dg)
{
  return is_directed(dg.get_adapter());
}

/***/

template <typename G>
inline
bool
is_undirected(const duplicate_adapter<G>& dg)
{
  return is_undirected(dg.get_adapter());
}

/***/

template <typename G>
inline
bool
is_bidirectional(const duplicate_adapter<G>& dg)
{
  return is_bidirectional(dg.get_adapter());
}

/***/

template <typename G>
class vertex_id_map<duplicate_adapter<G> > :
  public vertex_id_map<typename duplicate_adapter<G>::wrapper_adapter> {
public:
  typedef typename duplicate_adapter<G>::wrapper_adapter DG;

  vertex_id_map() : vertex_id_map<DG>() {}
  vertex_id_map(const vertex_id_map& vm) : vertex_id_map<DG>(vm) {}
};

/***/

template <typename G>
class edge_id_map<duplicate_adapter<G> > :
  public edge_id_map<typename duplicate_adapter<G>::wrapper_adapter> {
public:
  typedef typename duplicate_adapter<G>::wrapper_adapter DG;

  edge_id_map() : edge_id_map<DG>() {}
  edge_id_map(const edge_id_map& em) : edge_id_map<DG>(em) {}
};

}

#endif

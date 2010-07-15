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
/*! \file transpose_adapter.hpp

    \brief This adapter provides an interface to access the transpose of a
           graph_adapter.  It stores a reference to the original graph and
           an entire copy of the transpose.  The transpose could possibly
           be stored more efficiently, but storing a full copy of the
           transpose allows this to work with any graph adapter that follows
           the standard interface.

    \author Greg Mackey (gemacke@sandia.gov)

    \date 6/27/2008

    The associations between the vertices in the original graph and the
    transposed graph are taken care of implicitly, such that the ids are the
    same between the original and transposed graphs.  The vertices are added
    to the transposed graph in such a way that vertex 82 in the original graph
    corresponds to vertex 82 in the transposed graph.  The same is true of the
    edges.  Thus, the associations don't have to be explicitly stored.

    Note that a transpose only has meaning for a directed graph.  The
    transpose of an undirected graph gives you back the original graph.  The
    adapter allows you to take the transpose of an undirected graph, but it
    just gives you a copy of the original graph.  Also, a transpose of a
    transpose of a graph simply gives you back the original graph.  Doing
    this is legal and amounts to an expensive way to make a copy of the
    original graph.
*/
/****************************************************************************/

#ifndef MTGL_TRANSPOSE_ADAPTER_HPP
#define MTGL_TRANSPOSE_ADAPTER_HPP

#include <mtgl/mtgl_adapter.hpp>
#include <mtgl/static_graph_adapter.hpp>

namespace mtgl {

template <typename graph_adapter>
class transpose_adapter {
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

  transpose_adapter(graph_adapter& g) : original_graph(&g)
  {
    base_size_type order = num_vertices(*original_graph);
    base_size_type size = num_edges(*original_graph);

    base_edge_iterator edgs = edges(*original_graph);

    vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, *original_graph);

    size_type* e_srcs = new size_type[size];
    size_type* e_dests = new size_type[size];

    // Get edge source and target ids.
    #pragma mta assert parallel
    for (base_size_type i = 0; i < size; ++i)
    {
      base_edge_descriptor e = edgs[i];

      e_srcs[i] = get(vid_map, source(e, *original_graph));
      e_dests[i] = get(vid_map, target(e, *original_graph));
    }

    // Initialize the transpose graph.  This is assuming that the init()
    // method of the underlying graph implements parallelization correctly
    // and efficiently.
    init(order, size, e_dests, e_srcs, transpose_graph);

    delete [] e_srcs;
    delete [] e_dests;
  }

  // The compiler-synthesized copy control works fine for this class, so we
  // don't implement it ourselves.  We assume that the graph adapter passed
  // as a template parameter has a correctly implemented deep copy which makes
  // the transpose adapter perform a deep copy.

  graph_type* get_graph() const { return transpose_graph.get_graph(); }
  const wrapper_adapter& get_adapter() const { return transpose_graph; }
  const graph_adapter& get_original_adapter() const { return *original_graph; }

  void print() { transpose_graph.print(); }

private:
  graph_adapter* original_graph;
  wrapper_adapter transpose_graph;
};

/***/

template <typename G>
inline
typename transpose_adapter<G>::vertex_descriptor
get_vertex(typename transpose_adapter<G>::size_type i,
           const transpose_adapter<G>& tg)
{
  return get_vertex(i, tg.get_adapter());
}

/***/

template <typename G>
inline
typename transpose_adapter<G>::edge_descriptor
get_edge(typename transpose_adapter<G>::size_type i,
         const transpose_adapter<G>& tg)
{
  return get_edge(i, tg.get_adapter());
}

/***/

template <typename G>
inline
typename transpose_adapter<G>::size_type
num_vertices(const transpose_adapter<G>& tg)
{
  return num_vertices(tg.get_adapter());
}

/***/

template <typename G>
inline
typename transpose_adapter<G>::size_type
num_edges(const transpose_adapter<G>& tg)
{
  return num_edges(tg.get_adapter());
}

/***/

template <typename G>
inline
typename transpose_adapter<G>::vertex_descriptor
source(const typename transpose_adapter<G>::edge_descriptor& e,
       const transpose_adapter<G>& tg)
{
  return source(e, tg.get_adapter());
}

/***/

template <typename G>
inline
typename transpose_adapter<G>::vertex_descriptor
target(const typename transpose_adapter<G>::edge_descriptor& e,
       const transpose_adapter<G>& tg)
{
  return target(e, tg.get_adapter());
}

/***/

template <typename G>
inline
typename transpose_adapter<G>::vertex_descriptor
other(const typename transpose_adapter<G>::edge_descriptor& e,
      const typename transpose_adapter<G>::vertex_descriptor& u_local,
      const transpose_adapter<G>& tg)
{
  return other(e, u_local, tg.get_adapter());
}

/***/

template <typename G>
inline
typename transpose_adapter<G>::size_type
degree(const typename transpose_adapter<G>::vertex_descriptor& u_local,
       const transpose_adapter<G>& tg)
{
  return degree(u_local, tg.get_adapter());
}

/***/

template <typename G>
inline
typename transpose_adapter<G>::size_type
out_degree(const typename transpose_adapter<G>::vertex_descriptor& u_local,
           const transpose_adapter<G>& tg)
{
  return out_degree(u_local, tg.get_adapter());
}

/***/

template <typename G>
inline
typename transpose_adapter<G>::size_type
in_degree(const typename transpose_adapter<G>::vertex_descriptor& u_local,
          const transpose_adapter<G>& tg)
{
  return in_degree(u_local, tg.get_adapter());
}

/***/

template <typename G>
inline
typename transpose_adapter<G>::vertex_iterator
vertices(const transpose_adapter<G>& tg)
{
  return vertices(tg.get_adapter());
}

/***/

template <typename G>
inline
typename transpose_adapter<G>::edge_iterator
edges(const transpose_adapter<G>& tg)
{
  return edges(tg.get_adapter());
}

/***/

template <typename G>
inline
typename transpose_adapter<G>::adjacency_iterator
adjacent_vertices(const typename transpose_adapter<G>::vertex_descriptor& v,
                  const transpose_adapter<G>& tg)
{
  return adjacent_vertices(v, tg.get_adapter());
}

/***/

template <typename G>
inline
typename transpose_adapter<G>::in_adjacency_iterator
in_adjacent_vertices(const typename transpose_adapter<G>::vertex_descriptor& v,
                     const transpose_adapter<G>& tg)
{
  return in_adjacent_vertices(v, tg.get_adapter());
}

/***/

template <typename G>
inline
typename transpose_adapter<G>::out_edge_iterator
out_edges(const typename transpose_adapter<G>::vertex_descriptor& v,
          const transpose_adapter<G>& tg)
{
  return out_edges(v, tg.get_adapter());
}

/***/

template <typename G>
inline
typename transpose_adapter<G>::in_edge_iterator
in_edges(const typename transpose_adapter<G>::vertex_descriptor& v,
         const transpose_adapter<G>& tg)
{
  return in_edges(v, tg.get_adapter());
}

/***/

template <typename G>
inline
bool
is_directed(const transpose_adapter<G>& tg)
{
  return is_directed(tg.get_adapter());
}

/***/

template <typename G>
inline
bool
is_undirected(const transpose_adapter<G>& tg)
{
  return is_undirected(tg.get_adapter());
}

/***/

template <typename G>
inline
bool
is_bidirectional(const transpose_adapter<G>& tg)
{
  return is_bidirectional(tg.get_adapter());
}

/***/

template <typename G>
class vertex_id_map<transpose_adapter<G> > :
  public vertex_id_map<typename transpose_adapter<G>::wrapper_adapter> {
public:
  typedef typename transpose_adapter<G>::wrapper_adapter TG;

  vertex_id_map() : vertex_id_map<TG>() {}
  vertex_id_map(const vertex_id_map& vm) : vertex_id_map<TG>(vm) {}
};

/***/

template <typename G>
class edge_id_map<transpose_adapter<G> > :
  public edge_id_map<typename transpose_adapter<G>::wrapper_adapter> {
public:
  typedef typename transpose_adapter<G>::wrapper_adapter TG;

  edge_id_map() : edge_id_map<TG>() {}
  edge_id_map(const edge_id_map& em) : edge_id_map<TG>(em) {}
};

}

#endif

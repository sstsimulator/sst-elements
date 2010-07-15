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
/*! \file adjacency_list_adapter.hpp

    \brief This adapter provides an interface to the adjacency_list class
           which is an adjacency list implementation of a graph.

    \author Greg Mackey (gemacke@sandia.gov)

    \date 3/17/2008
*/
/****************************************************************************/

#ifndef MTGL_ADJACENCY_LIST_ADAPTER_HPP
#define MTGL_ADJACENCY_LIST_ADAPTER_HPP

#include <mtgl/mtgl_adapter.hpp>
#include <mtgl/adjacency_list.hpp>

namespace mtgl {

/***/

template <typename graph_adapter>
class al_edge_adapter {
public:
  typedef typename graph_adapter::Vertex Vertex;
  typedef typename graph_adapter::Edge Edge;

  al_edge_adapter() : first(0), second(0) {}
  al_edge_adapter(Edge& e) : first(e.from), second(e.to), id(e.id) {}
  al_edge_adapter(Edge* e) : first(e->from), second(e->to), id(e->id) {}

  al_edge_adapter(Vertex* v1, Vertex* v2, unsigned long eid) :
    first(v1), second(v2), id(eid) {}

public:
  Vertex* first;
  Vertex* second;
  unsigned long id;
};

/***/

template <typename graph_adapter>
class al_adjacency_iterator {
public:
  typedef typename graph_adapter::Vertex Vertex;
  typedef typename graph_adapter::Edge Edge;

  al_adjacency_iterator() : v(0) {}
  al_adjacency_iterator(Vertex* vert) : v(vert) {}

  Vertex* operator[](unsigned long p)
  {
    Edge* e = v->edges[p];
    return v == e->from ? e->to : e->from;
  }

private:
  Vertex* v;
};

/***/

template <typename graph_adapter>
class al_out_edge_iterator {
public:
  typedef typename graph_adapter::Vertex Vertex;
  typedef typename graph_adapter::Edge Edge;

  al_out_edge_iterator() : v(0) {}
  al_out_edge_iterator(Vertex* vert) : v(vert) {}

  al_edge_adapter<graph_adapter> operator[](unsigned long p)
  {
    Edge* e = v->edges[p];
    Vertex* v2 = v == e->from ? e->to : e->from;
    return al_edge_adapter<graph_adapter>(v, v2, e->id);
  }

private:
  Vertex* v;
};

/***/

template <typename DIRECTION = directedS>
class adjacency_list_adapter {
public:
  typedef unsigned long size_type;
  typedef typename adjacency_list<DIRECTION>::Vertex* vertex_descriptor;
  typedef al_edge_adapter<adjacency_list<DIRECTION> > edge_descriptor;
  typedef typename adjacency_list<DIRECTION>::Vertex** vertex_iterator;
  typedef al_adjacency_iterator<adjacency_list<DIRECTION> > adjacency_iterator;
  typedef void in_adjacency_iterator;
  typedef typename adjacency_list<DIRECTION>::Edge** edge_iterator;
  typedef al_out_edge_iterator<adjacency_list<DIRECTION> > out_edge_iterator;
  typedef void in_edge_iterator;
  typedef DIRECTION directed_category;
  typedef adjacency_list<DIRECTION> graph_type;

  adjacency_list_adapter(adjacency_list<DIRECTION>* g) :
    the_graph(g), initialized_by_ptr(true) {}

  adjacency_list_adapter() : the_graph(new adjacency_list<DIRECTION>),
                       initialized_by_ptr(false) {}

  adjacency_list_adapter(const adjacency_list_adapter& g) :
    initialized_by_ptr(false)
  {
    the_graph = new adjacency_list<DIRECTION>(*(g.get_graph()));
  }

  ~adjacency_list_adapter()
  {
    if (!initialized_by_ptr) delete the_graph;
  }

  adjacency_list_adapter& operator=(const adjacency_list_adapter& rhs)
  {
    if (initialized_by_ptr)
    {
      initialized_by_ptr = false;
    }
    else
    {
      delete the_graph;
    }

    the_graph = new adjacency_list<DIRECTION>(*(rhs.get_graph()));

    return *this;
  }

  void clear()
  {
    the_graph->clear();
  }

  graph_type* get_graph() const { return the_graph; }

  void init(size_type numVerts, size_type numEdges, size_type* sources,
            size_type* targets)
  {
    the_graph->init(numVerts, numEdges, sources, targets);
  }

  vertex_iterator vertices() const { return the_graph->vertices.get_data(); }

  edge_iterator edges() const { return the_graph->edges.get_data(); }

  vertex_descriptor get_vertex(size_type i) const
  {
    return the_graph->vertices[i];
  }

  edge_descriptor get_edge(size_type i) const
  {
    return edge_descriptor(the_graph->edges[i]);
  }

  size_type get_order() const { return the_graph->nVertices; }
  size_type get_size() const { return the_graph->nEdges;  }
  size_type get_degree(vertex_descriptor v) const { return v->edges.size(); }

  size_type get_out_degree(vertex_descriptor v) const
  {
    return v->edges.size();
  }

  out_edge_iterator out_edges(const vertex_descriptor& v) const
  {
    return out_edge_iterator(v);
  }

  adjacency_iterator adjacent_vertices(const vertex_descriptor& v) const
  {
    return adjacency_iterator(v);
  }

  void print() { the_graph->print(); }

private:
  adjacency_list<DIRECTION>* the_graph;
  bool initialized_by_ptr;
};

/***/

template <typename DIRECTION>
inline
typename adjacency_list_adapter<DIRECTION>::vertex_descriptor
get_vertex(typename adjacency_list_adapter<DIRECTION>::size_type i,
           const adjacency_list_adapter<DIRECTION>& g)
{
  return g.get_vertex(i);
}

/***/

template <typename DIRECTION>
inline
typename adjacency_list_adapter<DIRECTION>::edge_descriptor
get_edge(typename adjacency_list_adapter<DIRECTION>::size_type i,
         const adjacency_list_adapter<DIRECTION>& g)
{
  return g.get_edge(i);
}

/***/

template <typename DIRECTION>
inline
typename adjacency_list_adapter<DIRECTION>::size_type
num_vertices(const adjacency_list_adapter<DIRECTION>& g)
{
  return g.get_order();
}

/***/

template <typename DIRECTION>
inline
typename adjacency_list_adapter<DIRECTION>::size_type
num_edges(const adjacency_list_adapter<DIRECTION>& g)
{
  return g.get_size();
}

/***/

template <typename DIRECTION>
inline
typename adjacency_list_adapter<DIRECTION>::vertex_descriptor
source(const typename adjacency_list_adapter<DIRECTION>::edge_descriptor& e,
       const adjacency_list_adapter<DIRECTION>& g)
{
	return e.first;
}

/***/

template <typename DIRECTION>
inline
typename adjacency_list_adapter<DIRECTION>::vertex_descriptor
target(const typename adjacency_list_adapter<DIRECTION>::edge_descriptor& e,
       const adjacency_list_adapter<DIRECTION>& g)
{
	return e.second;
}

/***/

template <typename DIRECTION>
inline
typename adjacency_list_adapter<DIRECTION>::vertex_descriptor
other(const typename adjacency_list_adapter<DIRECTION>::edge_descriptor& e,
      const typename adjacency_list_adapter<DIRECTION>::vertex_descriptor v,
      const adjacency_list_adapter<DIRECTION>& g)
{
  typename adjacency_list_adapter<DIRECTION>::vertex_descriptor rother;

  if (v == e.first)
  {
    rother = e.second;
  }
  else if (v == e.second)
  {
    rother = e.first;
  }
  else
  {
    assert(false);
  }

  return rother;
}

/***/

template <typename DIRECTION>
inline
typename adjacency_list_adapter<DIRECTION>::size_type
degree(const typename adjacency_list_adapter<DIRECTION>::vertex_descriptor& v,
       const adjacency_list_adapter<DIRECTION>& g)
{
  return v->edges.size();
}

/***/

template <typename DIRECTION>
inline
typename graph_traits<adjacency_list_adapter<DIRECTION> >::size_type
out_degree(const typename
             adjacency_list_adapter<DIRECTION>::vertex_descriptor& v,
           const adjacency_list_adapter<DIRECTION>& g)
{
  return v->edges.size();
}

/***/

template <typename DIRECTION>
inline
typename adjacency_list_adapter<DIRECTION>::vertex_iterator
vertices(const adjacency_list_adapter<DIRECTION>& g)
{
  return g.vertices();
}

/***/

template <typename DIRECTION>
inline
typename adjacency_list_adapter<DIRECTION>::edge_iterator
edges(const adjacency_list_adapter<DIRECTION>& g)
{
  return g.edges();
}

/***/

template <typename DIRECTION>
inline
typename adjacency_list_adapter<DIRECTION>::adjacency_iterator
adjacent_vertices(
    const typename adjacency_list_adapter<DIRECTION>::vertex_descriptor& v,
    const adjacency_list_adapter<DIRECTION>& g)
{
  return g.adjacent_vertices(v);
}

/***/

template <typename DIRECTION>
inline
typename adjacency_list_adapter<DIRECTION>::out_edge_iterator
out_edges(const typename
            adjacency_list_adapter<DIRECTION>::vertex_descriptor& v,
          const adjacency_list_adapter<DIRECTION>& g)
{
  return g.out_edges(v);
}

/***/

template <typename DIRECTION>
inline
bool
is_directed(const adjacency_list_adapter<DIRECTION>& g)
{
  return DIRECTION::is_directed();
}

/***/

template <typename DIRECTION>
inline
bool
is_undirected(const adjacency_list_adapter<DIRECTION>& g)
{
  return !is_directed(g);
}

/***/

template <typename DIRECTION>
inline
bool
is_bidirectional(const adjacency_list_adapter<DIRECTION>& g)
{
  return DIRECTION::is_bidirectional();
}

/***/

template <typename DIRECTION>
class vertex_id_map<adjacency_list_adapter<DIRECTION> > :
  public put_get_helper<typename adjacency_list_adapter<DIRECTION>::size_type,
                        vertex_id_map<adjacency_list_adapter<DIRECTION> > > {
public:
  typedef typename adjacency_list_adapter<DIRECTION>::vertex_descriptor
          key_type;
  typedef typename adjacency_list_adapter<DIRECTION>::size_type value_type;

  vertex_id_map() {}

  value_type operator[] (const key_type& k) const { return k->id; }
};

/***/

template <typename DIRECTION>
class edge_id_map<adjacency_list_adapter<DIRECTION> > :
  public put_get_helper<typename adjacency_list_adapter<DIRECTION>::size_type,
                        edge_id_map<adjacency_list_adapter<DIRECTION> > > {
public:
  typedef typename adjacency_list_adapter<DIRECTION>::edge_descriptor key_type;
  typedef typename adjacency_list_adapter<DIRECTION>::size_type value_type;

  edge_id_map() {}

  value_type operator[] (const key_type& k) const { return k.id; }
};

/***/

template <typename DIRECTION>
inline
void init(typename adjacency_list_adapter<DIRECTION>::size_type n,
          typename adjacency_list_adapter<DIRECTION>::size_type m,
          typename adjacency_list_adapter<DIRECTION>::size_type* srcs,
          typename adjacency_list_adapter<DIRECTION>::size_type* dests,
          adjacency_list_adapter<DIRECTION>& g)
{
  return g.init(n, m, srcs, dests);
}

/***/

template <typename DIRECTION>
inline
void clear(adjacency_list_adapter<DIRECTION>& g)
{
  return g.clear();
}

/***/

template <typename DIRECTION>
inline
typename adjacency_list_adapter<DIRECTION>::vertex_descriptor
add_vertex(const adjacency_list_adapter<DIRECTION>& g)
{
  return g.get_graph()->addVertex();
}

/***/

template <typename DIRECTION>
inline
void
add_vertices(typename adjacency_list_adapter<DIRECTION>::size_type num_verts,
             const adjacency_list_adapter<DIRECTION>& g)
{
  g.get_graph()->addVertices(num_verts);
}

/***/

template <typename DIRECTION>
inline
void
remove_vertices(typename adjacency_list_adapter<DIRECTION>::size_type num_verts,
                typename adjacency_list_adapter<DIRECTION>::size_type* v,
                const adjacency_list_adapter<DIRECTION> & g)
{
  g.get_graph()->removeVertices(num_verts, v);
}

/***/

template <typename DIRECTION>
inline
pair<typename adjacency_list_adapter<DIRECTION>::edge_descriptor, bool>
add_edge(typename adjacency_list_adapter<DIRECTION>::vertex_descriptor f,
         typename adjacency_list_adapter<DIRECTION>::vertex_descriptor t,
         const adjacency_list_adapter<DIRECTION>& g)
{
  return pair<typename adjacency_list_adapter<DIRECTION>::edge_descriptor,
              bool>(g.get_graph()->addEdge(f, t), true);
}

/***/

template <typename DIRECTION>
inline
pair<typename adjacency_list_adapter<DIRECTION>::edge_descriptor, bool>
add_edge(typename adjacency_list_adapter<DIRECTION>::size_type f,
         typename adjacency_list_adapter<DIRECTION>::size_type t,
         const adjacency_list_adapter<DIRECTION>& g)
{
  return pair<typename adjacency_list_adapter<DIRECTION>::edge_descriptor,
              bool>(g.get_graph()->addEdge(f, t), true);
}

/***/

template <typename DIRECTION>
inline
void
add_edges(typename adjacency_list_adapter<DIRECTION>::size_type num_edges,
          typename adjacency_list_adapter<DIRECTION>::vertex_descriptor* f,
          typename adjacency_list_adapter<DIRECTION>::vertex_descriptor* t,
          const adjacency_list_adapter<DIRECTION> & g)
{
  g.get_graph()->addEdges(num_edges, f, t);
}

/***/

template <typename DIRECTION>
inline
void
add_edges(typename adjacency_list_adapter<DIRECTION>::size_type num_edges,
          typename adjacency_list_adapter<DIRECTION>::size_type* f,
          typename adjacency_list_adapter<DIRECTION>::size_type* t,
          const adjacency_list_adapter<DIRECTION> & g)
{
  g.get_graph()->addEdges(num_edges, f, t);
}

/***/

template <typename DIRECTION>
inline
void
remove_edges(typename adjacency_list_adapter<DIRECTION>::size_type num_edges,
             typename adjacency_list_adapter<DIRECTION>::size_type* e,
             const adjacency_list_adapter<DIRECTION> & g)
{
  g.get_graph()->removeEdges(num_edges, e);
}

}

#endif

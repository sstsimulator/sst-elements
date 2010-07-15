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
/*! \file graph_adapter.hpp

    \brief This is an mtgl adapter over the sample "graph" data structure
           in mtgl/graph.h.

    \author Jon Berry (jberry@sandia.gov)
    \author Greg Mackey (gemacke@sandia.gov)

    \date 12/4/2007

    \deprecated This file is deprecated in favor of
                adjacency_list_adapter.hpp.  The class graph_adapter is
                deprecated in favor of adjacency_list_adapter.
*/
/****************************************************************************/

#ifndef MTGL_GRAPH_ADAPTER_HPP
#define MTGL_GRAPH_ADAPTER_HPP

#include <cassert>

#include <mtgl/mtgl_adapter.hpp>
#include <mtgl/graph.hpp>

namespace mtgl {

template <typename vertex_t, typename count_t>
class edge_adapter : public pair<vertex_t, vertex_t> {
public:
  edge_adapter() : pair<vertex_t, vertex_t>(0, 0) {}

  edge_adapter(edge& e) :
    pair<vertex_t, vertex_t>(e.vert1, e.vert2), id(e.id) {}

  edge_adapter(edge* e) :
    pair<vertex_t, vertex_t>(e->vert1, e->vert2), id(e->id) {}

  edge_adapter(vertex_t v1, vertex_t v2, count_t eid) :
    pair<vertex_t, vertex_t>(v1, v2), id(eid) {}

  count_t get_id() const { return id; }

private:
  count_t id;
};

template <typename vertex_t, typename count_t>
class g_vertex_iterator {
public:
  g_vertex_iterator() : the_vertices(0) {}
  g_vertex_iterator(vertex_t* vertices) : the_vertices(vertices) {}

  vertex_t operator[](count_t p) { return the_vertices[p]; }

private:
  vertex_t* the_vertices;
};

template <typename vertex_t, typename count_t>
class g_edge_iterator {
public:
  g_edge_iterator() : the_edges(0) {}
  g_edge_iterator(edge** edges) : the_edges(edges) {}

  edge_adapter<vertex_t, count_t> operator[](count_t p)
  {
    return edge_adapter<vertex_t, count_t>(the_edges[p]);
  }

private:
  edge** the_edges;
};

template <typename vertex_t, typename count_t>
class g_adjacency_iterator {
public:
  g_adjacency_iterator() : vid(0) {}
  g_adjacency_iterator(edge** adjs, count_t i) : vid(i), adjacencies(adjs) {}

  vertex_t operator[](count_t p)
  {
    edge* e = adjacencies[p];
    vertex_t oth = e->other(vid);
    return oth;
  }

public:
  count_t vid;
  edge** adjacencies;
};

template <typename vertex_t, typename count_t>
class g_out_edge_iterator {
public:
  g_out_edge_iterator() : v(0), adjacencies(0) {}

  g_out_edge_iterator(vertex* v_, edge** adj) :
    v(v_), adjacencies(adj) {}

  edge_adapter<vertex_t, count_t> operator[](count_t p)
  {
    edge* e = adjacencies[p];
    vertex_t v2 = v == e->get_vert1() ? e->get_vert2() : e->get_vert1();
    return edge_adapter<vertex_t, count_t>(v, v2, e->get_id());
  }

private:
  vertex_t v;
  edge** adjacencies;
};

template <typename DIRECTION = directedS>
class graph_adapter {
public:
  typedef unsigned long size_type;
  typedef vertex* vertex_descriptor;
  typedef edge_adapter<vertex_descriptor, size_type> edge_descriptor;
  typedef g_vertex_iterator<vertex_descriptor, size_type> vertex_iterator;
  typedef g_adjacency_iterator<vertex_descriptor, size_type>
          adjacency_iterator;
  typedef void in_adjacency_iterator;
  typedef g_edge_iterator<vertex_descriptor, size_type> edge_iterator;
  typedef g_out_edge_iterator<vertex_descriptor, size_type>
          out_edge_iterator;
  typedef void in_edge_iterator;
  typedef DIRECTION directed_category;
  typedef graph<DIRECTION> graph_type;

  graph_adapter(graph<DIRECTION>* g) : the_graph(g), initialized_by_ptr(true) {}
  graph_adapter() : the_graph(new graph<DIRECTION>),
                    initialized_by_ptr(false) {}

  graph_adapter(const graph_adapter& g) : initialized_by_ptr(false)
  {
    the_graph = new graph<DIRECTION>(*(g.get_graph()));
  }

  ~graph_adapter()
  {
    if (!initialized_by_ptr) delete the_graph;
  }

  graph_adapter& operator=(const graph_adapter& rhs)
  {
    if (initialized_by_ptr)
    {
      initialized_by_ptr = false;
    }
    else
    {
      delete the_graph;
    }

    the_graph = new graph<DIRECTION>(*(rhs.get_graph()));

    return *this;
  }

  void clear()
  {
    the_graph->clear();
  }

  void init(size_type n_verts, size_type n_edges, size_type* srcs,
            size_type* dests)
  {
    the_graph->init(n_verts, n_edges, srcs, dests);
  }

  graph<DIRECTION>* get_graph() const { return the_graph; }

  edge_iterator edges() const { return edge_iterator(the_graph->get_edges()); }

  vertex_descriptor get_vertex(size_type i) const
  {
    return the_graph->get_vertex(i);
  }

  edge_descriptor get_edge(size_type i) const
  {
    return edge_descriptor(the_graph->get_edge(i));
  }

  size_type get_degree(vertex_descriptor v) const { return v->degree(); }
  size_type get_out_degree(vertex_descriptor v) const { return v->degree(); }

  out_edge_iterator out_edges(const vertex_descriptor& v) const
  {
    return out_edge_iterator(the_graph->get_vertex(v->id),
                             the_graph->begin_incident_edges(v->id));
  }

  adjacency_iterator adjacent_vertices(const vertex_descriptor& v) const
  {
    return adjacency_iterator(the_graph->begin_incident_edges(v->id), v->id);
  }

  size_type get_order() const { return the_graph->get_order(); }
  size_type get_size() const { return the_graph->get_size(); }

  vertex_iterator vertices() const
  {
    return vertex_iterator(the_graph->get_vertices());
  }

private:
  graph<DIRECTION>* the_graph;
  bool initialized_by_ptr;
};

template <typename DIRECTION>
inline
typename graph_adapter<DIRECTION>::vertex_descriptor
get_vertex(typename graph_adapter<DIRECTION>::size_type i,
           const graph_adapter<DIRECTION>& g)
{
  return g.get_vertex(i);
}

template <typename DIRECTION>
inline
typename graph_adapter<DIRECTION>::edge_descriptor
get_edge(typename graph_adapter<DIRECTION>::size_type i,
         const graph_adapter<DIRECTION>& g)
{
  return g.get_edge(i);
}

template <typename DIRECTION>
inline
typename graph_adapter<DIRECTION>::size_type
num_vertices(const graph_adapter<DIRECTION>& g)
{
  return g.get_order();
}

template <typename DIRECTION>
inline
typename graph_adapter<DIRECTION>::size_type
num_edges(const graph_adapter<DIRECTION>& g)
{
  return g.get_size();
}

template <typename DIRECTION>
inline
typename graph_adapter<DIRECTION>::vertex_descriptor
source(const typename graph_adapter<DIRECTION>::edge_descriptor& e,
       const graph_adapter<DIRECTION>& g)
{
  return e.first;
}

template <typename DIRECTION>
inline
typename graph_adapter<DIRECTION>::vertex_descriptor
target(const typename graph_adapter<DIRECTION>::edge_descriptor& e,
       const graph_adapter<DIRECTION>& g)
{
  return e.second;
}

template <typename DIRECTION>
inline
typename graph_adapter<DIRECTION>::vertex_descriptor
other(const typename graph_adapter<DIRECTION>::edge_descriptor& e,
      const typename graph_adapter<DIRECTION>::vertex_descriptor& v,
      const graph_adapter<DIRECTION>& g)
{
  if (v->get_id() == e.first->get_id())
  {
    return e.second;
  }
  else if (v->get_id() == e.second->get_id())
  {
    return e.first;
  }
  else
  {
    printf("error: other((%lu, %lu), %lu)\n", e.first->get_id(),
           e.second->get_id(), v->get_id());
    return e.first;
  }
}

template <typename DIRECTION>
inline
typename graph_adapter<DIRECTION>::size_type
degree(const typename graph_adapter<DIRECTION>::vertex_descriptor& v,
       const graph_adapter<DIRECTION>& g)
{
  return v->degree();
}

template <typename DIRECTION>
inline
typename graph_adapter<DIRECTION>::size_type
out_degree(const typename graph_adapter<DIRECTION>::vertex_descriptor& v,
           const graph_adapter<DIRECTION>& g)
{
  return v->degree();
}

template <typename DIRECTION>
inline
typename graph_adapter<DIRECTION>::vertex_iterator
vertices(const graph_adapter<DIRECTION>& ga)
{
  return ga.vertices();
}

template <typename DIRECTION>
inline
typename graph_adapter<DIRECTION>::edge_iterator
edges(const graph_adapter<DIRECTION>& g)
{
  return g.edges();
}

template <typename DIRECTION>
inline
typename graph_adapter<DIRECTION>::adjacency_iterator
adjacent_vertices(const typename graph_adapter<DIRECTION>::vertex_descriptor& v,
                  const graph_adapter<DIRECTION>& ga)
{
  return ga.adjacent_vertices(v);
}

template <typename DIRECTION>
inline
typename graph_adapter<DIRECTION>::out_edge_iterator
out_edges(const typename graph_adapter<DIRECTION>::vertex_descriptor& v,
          const graph_adapter<DIRECTION>& g)
{
  return g.out_edges(v);
}

template <typename DIRECTION>
inline
bool
is_directed(const graph_adapter<DIRECTION>& g)
{
  return DIRECTION::is_directed();
}

template <typename DIRECTION>
inline
bool
is_undirected(const graph_adapter<DIRECTION>& g)
{
  return !is_directed(g);
}

template <typename DIRECTION>
inline
bool
is_bidirectional(const graph_adapter<DIRECTION>& g)
{
  return DIRECTION::is_bidirectional();
}

template <typename DIRECTION>
class vertex_id_map<graph_adapter<DIRECTION> > :
  public put_get_helper<typename graph_adapter<DIRECTION>::size_type,
                        vertex_id_map<graph_adapter<DIRECTION> > > {
public:
  typedef typename graph_adapter<DIRECTION>::vertex_descriptor key_type;
  typedef typename graph_adapter<DIRECTION>::size_type value_type;

  vertex_id_map() {}
  value_type operator[] (const key_type& k) const { return k->id; }
};

template <typename DIRECTION>
class edge_id_map<graph_adapter<DIRECTION> > :
  public put_get_helper<typename graph_adapter<DIRECTION>::size_type,
                        edge_id_map<graph_adapter<DIRECTION> > > {
public:
  typedef typename graph_adapter<DIRECTION>::edge_descriptor key_type;
  typedef typename graph_adapter<DIRECTION>::size_type value_type;

  edge_id_map() {}
  value_type operator[] (const key_type& k) const { return k.get_id(); }
};

template <typename DIRECTION>
inline
void init(typename graph_adapter<DIRECTION>::size_type n,
          typename graph_adapter<DIRECTION>::size_type m,
          typename graph_adapter<DIRECTION>::size_type* srcs,
          typename graph_adapter<DIRECTION>::size_type* dests,
          graph_adapter<DIRECTION>& g)
{
  g.init(n, m, srcs, dests);
}

template <typename DIRECTION>
inline
void clear(graph_adapter<DIRECTION>& g)
{
  g.clear();
}

template <typename DIRECTION>
inline
typename graph_adapter<DIRECTION>::vertex_descriptor
add_vertex(const graph_adapter<DIRECTION>& g)
{
  typename graph_adapter<DIRECTION>::size_type ord = g.get_order();
  g.get_graph()->addvertex(ord);
  return get_vertex(ord, g);
}

template <typename DIRECTION>
inline
void
add_vertices(typename graph_adapter<DIRECTION>::size_type num_verts,
             const graph_adapter<DIRECTION>& g)
{
  typename graph_adapter<DIRECTION>::vertex_descriptor* new_verts = 
    g.get_graph()->addVertices(0, num_verts - 1);

  free(new_verts);
}

template <typename DIRECTION>
inline
pair<typename graph_adapter<DIRECTION>::edge_descriptor, bool>
add_edge(typename graph_adapter<DIRECTION>::vertex_descriptor f,
         typename graph_adapter<DIRECTION>::vertex_descriptor t,
         const graph_adapter<DIRECTION>& g)
{
  typename graph_adapter<DIRECTION>::size_type size = g.get_size();
  return pair<typename graph_adapter<DIRECTION>::edge_descriptor, bool>(
           g.get_graph()->addedge(f, t, size), true);
}

template <typename DIRECTION>
inline
pair<typename graph_adapter<DIRECTION>::edge_descriptor, bool>
add_edge(typename graph_adapter<DIRECTION>::size_type f,
         typename graph_adapter<DIRECTION>::size_type t,
         const graph_adapter<DIRECTION>& g)
{
  typename graph_adapter<DIRECTION>::size_type size = g.get_size();
  return pair<typename graph_adapter<DIRECTION>::edge_descriptor, bool>(
           g.get_graph()->addedge(f, t, size), true);
}

template <typename DIRECTION>
inline
void
add_edges(typename graph_adapter<DIRECTION>::size_type num_edges,
          typename graph_adapter<DIRECTION>::vertex_descriptor* f,
          typename graph_adapter<DIRECTION>::vertex_descriptor* t,
          const graph_adapter<DIRECTION>& g)
{
  edge** new_edges = g.get_graph()->addedges(f, t, num_edges);

  free(new_edges);
}

template <typename DIRECTION>
inline
void
add_edges(typename graph_adapter<DIRECTION>::size_type num_edges,
          typename graph_adapter<DIRECTION>::size_type* f,
          typename graph_adapter<DIRECTION>::size_type* t,
          const graph_adapter<DIRECTION>& g)
{
  edge** new_edges = g.get_graph()->addedges(f, t, num_edges);

  free(new_edges);
}

}

#endif

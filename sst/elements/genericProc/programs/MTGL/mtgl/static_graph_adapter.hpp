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
/*! \file static_graph_adapter.hpp

    \brief The graph adapter wrapper for static_graph.hpp.

    \author Greg Mackey (gemacke@sandia.gov)
    \author Jon Berry (jberry@sandia.gov)
    \author Brad Mancke

    \date 1/3/2008
*/
/****************************************************************************/

#ifndef MTGL_STATIC_GRAPH_ADAPTER_HPP
#define MTGL_STATIC_GRAPH_ADAPTER_HPP

#include <cstdlib>

#include <mtgl/mtgl_adapter.hpp>
#include <mtgl/static_graph.hpp>
#include <mtgl/util.hpp>

namespace mtgl {

template <typename vertex_t, typename count_t>
class static_edge_adapter : public pair<vertex_t, vertex_t> {
public:
  static_edge_adapter() : first(0), second(0) {}
  static_edge_adapter(vertex_t v1, vertex_t v2, count_t eid) :
    first(v1), second(v2), id(eid) {}

public:
  vertex_t first;
  vertex_t second;
  count_t id;
};

template <typename vertex_t, typename count_t>
class static_vertex_iterator {
public:
  static_vertex_iterator() {}
  vertex_t operator[](count_t p) { return p; }
};

template <typename vertex_t, typename count_t>
class static_edge_iterator {
public:
  static_edge_iterator() : src_points(0), end_points(0), internal_ids(0) {}

  static_edge_iterator(vertex_t* sp, vertex_t* ep, count_t* ii) :
    src_points(sp), end_points(ep), internal_ids(ii) {}

  static_edge_adapter<vertex_t, count_t> operator[](count_t p)
  {
    return static_edge_adapter<vertex_t, count_t>(
               src_points[internal_ids[p]],
               end_points[internal_ids[p]],
               p);
  }

private:
  vertex_t* src_points;
  vertex_t* end_points;
  count_t* internal_ids;
};

template <typename vertex_t, typename count_t>
class static_adjacency_iterator {
public:
  static_adjacency_iterator() : adj(0) {}

  static_adjacency_iterator(count_t* ind, vertex_t* adj_, count_t i) :
    adj(adj_ + ind[i]) {}

  vertex_t operator[](count_t p)
  {
    vertex_t neigh = adj[p];
    return neigh;
  }

private:
  vertex_t* adj;
};

template <typename vertex_t, typename count_t>
class static_out_edge_iterator {
public:
  static_out_edge_iterator() : adj(0), original_ids(0), vid(0) {}

  static_out_edge_iterator(count_t* ind, vertex_t* adj_, count_t* oi,
                           count_t i) :
    adj(adj_ + ind[i]), original_ids(oi + ind[i]), vid(i) {}

  static_edge_adapter<vertex_t, count_t> operator[](count_t p)
  {
    return static_edge_adapter<vertex_t, count_t>(vid, adj[p],
                                                  original_ids[p]);
  }

private:
  vertex_t* adj;
  count_t* original_ids;
  count_t vid;
};

template <typename DIRECTION = directedS>
class static_graph_adapter {
public:
  typedef unsigned long size_type;
  typedef unsigned long vertex_descriptor;
  typedef static_edge_adapter<vertex_descriptor, size_type> edge_descriptor;
  typedef static_vertex_iterator<vertex_descriptor, size_type> vertex_iterator;
  typedef static_adjacency_iterator<vertex_descriptor, size_type>
          adjacency_iterator;
  typedef void in_adjacency_iterator;
  typedef static_edge_iterator<vertex_descriptor, size_type> edge_iterator;
  typedef static_out_edge_iterator<vertex_descriptor, size_type>
          out_edge_iterator;
  typedef void in_edge_iterator;
  typedef DIRECTION directed_category;
  typedef static_graph<DIRECTION> graph_type;

  static_graph_adapter(static_graph<DIRECTION>* g) :
    the_graph(g), initialized_by_ptr(true) {}

  static_graph_adapter() : the_graph(new static_graph<DIRECTION>),
                           initialized_by_ptr(false) {}

  static_graph_adapter(const static_graph_adapter& g) :
    initialized_by_ptr(false)
  {
    the_graph = new static_graph<DIRECTION>(*(g.get_graph()));
  }

  ~static_graph_adapter()
  {
    if (!initialized_by_ptr) delete the_graph;
  }

  static_graph_adapter& operator=(const static_graph_adapter& rhs)
  {
    if (initialized_by_ptr)
    {
      initialized_by_ptr = false;
    }
    else
    {
      delete the_graph;
    }

    the_graph = new static_graph<DIRECTION>(*(rhs.get_graph()));

    return *this;
  }

  void clear()
  {
    the_graph->clear();
  }

  static_graph<DIRECTION>* get_graph() const { return the_graph; }

  void init(size_type n, size_type m, size_type* srcs, size_type* dests)
  {
    the_graph->init(n, m, srcs, dests);
  }

  vertex_iterator vertices() const { return vertex_iterator(); }

  edge_iterator edges() const
  {
    return edge_iterator(the_graph->src_points, the_graph->end_points,
                         the_graph->internal_ids);
  }

  vertex_descriptor get_vertex(size_type i) const { return i; }

  edge_descriptor get_edge(size_type i) const
  {
    size_type int_id = the_graph->internal_ids[i];

    size_type v1 = the_graph->src_points[int_id];
    size_type v2 = the_graph->end_points[int_id];

    return edge_descriptor(v1, v2, i);
  }

  out_edge_iterator out_edges(const vertex_descriptor& v) const
  {
    return out_edge_iterator(the_graph->index, the_graph->end_points,
                             the_graph->original_ids, v);
  }

  adjacency_iterator adjacent_vertices(const vertex_descriptor& v) const
  {
    return adjacency_iterator(the_graph->index, the_graph->end_points, v);
  }

  size_type get_order() const { return the_graph->n; }
  size_type get_size() const { return the_graph->m; }

  size_type get_degree(size_type i) const { return get_out_degree(i); }

  size_type get_out_degree(size_type i) const
  {
    size_type* index = the_graph->index;
    size_type deg = index[i + 1] - index[i];
    return deg;
  }

  void print() const
  {
    printf("num Verts = %lu\n", the_graph->n);
    printf("num Edges = %lu\n", the_graph->m);

    for(size_type i = 0; i < the_graph->n; ++i)
    {
      size_type deg = the_graph->index[i + 1] - the_graph->index[i];

      printf("%lu\t: [%4lu] {", i, deg);

      for(size_type j = 0; j < deg; ++j)
      {
        size_type idx = the_graph->index[i];
        printf(" %lu", the_graph->end_points[idx + j]);
      }

      printf(" }\n");
    }
  }

  // The next two methods need to go away when mta_pe improves (see
  // pagerank.hpp - bidir case)
  size_type* get_index() const { return the_graph->index; }
  size_type* get_src_points() const { return the_graph->src_points; }
  size_type* get_end_points() const { return the_graph->end_points; }

  size_type operator[](size_type i) { return the_graph->index[i]; }

private:
  static_graph<DIRECTION>* the_graph;
  bool initialized_by_ptr;
};

template <typename DIRECTION>
inline
typename static_graph_adapter<DIRECTION>::vertex_descriptor
get_vertex(typename static_graph_adapter<DIRECTION>::size_type i,
           const static_graph_adapter<DIRECTION>& g)
{
  return g.get_vertex(i);
}

template <typename DIRECTION>
inline
typename static_graph_adapter<DIRECTION>::edge_descriptor
get_edge(typename static_graph_adapter<DIRECTION>::size_type i,
         const static_graph_adapter<DIRECTION>& g)
{
  return g.get_edge(i);
}

template <typename DIRECTION>
inline
typename static_graph_adapter<DIRECTION>::size_type
num_vertices(const static_graph_adapter<DIRECTION>& g)
{
  return g.get_order();
}

template <typename DIRECTION>
inline
typename static_graph_adapter<DIRECTION>::size_type
num_edges(const static_graph_adapter<DIRECTION>& g)
{
  return g.get_size();
}

template <typename DIRECTION>
inline
typename static_graph_adapter<DIRECTION>::vertex_descriptor
source(const typename static_graph_adapter<DIRECTION>::edge_descriptor& e,
       const static_graph_adapter<DIRECTION>& g)
{
  return e.first;
}

template <typename DIRECTION>
inline
typename static_graph_adapter<DIRECTION>::vertex_descriptor
target(const typename static_graph_adapter<DIRECTION>::edge_descriptor& e,
       const static_graph_adapter<DIRECTION>& g)
{
  return e.second;
}

template <typename DIRECTION>
inline
typename static_graph_adapter<DIRECTION>::vertex_descriptor
other(const typename static_graph_adapter<DIRECTION>::edge_descriptor& e,
      const typename static_graph_adapter<DIRECTION>::vertex_descriptor& v,
      const static_graph_adapter<DIRECTION>& g)
{
  if (v == e.first)
  {
    return e.second;
  }
  else if (v == e.second)
  {
    return e.first;
  }
  else
  {
    printf("error: other((%lu, %lu), %lu)\n", e.first, e.second, e.id);
    return e.first;
  }
}

template <typename DIRECTION>
inline
typename static_graph_adapter<DIRECTION>::size_type
degree(const typename static_graph_adapter<DIRECTION>::vertex_descriptor& v,
       const static_graph_adapter<DIRECTION>& g)
{
  return g.get_degree(v);
}

template <typename DIRECTION>
inline
typename static_graph_adapter<DIRECTION>::size_type
out_degree(const typename static_graph_adapter<DIRECTION>::vertex_descriptor& v,
           const static_graph_adapter<DIRECTION>& g)
{
  return g.get_out_degree(v);
}

template <typename DIRECTION>
inline
typename static_graph_adapter<DIRECTION>::vertex_iterator
vertices(const static_graph_adapter<DIRECTION>& g)
{
  return g.vertices();
}

template <typename DIRECTION>
inline
typename static_graph_adapter<DIRECTION>::edge_iterator
edges(const static_graph_adapter<DIRECTION>& g)
{
  return g.edges();
}

template <typename DIRECTION>
inline
typename static_graph_adapter<DIRECTION>::adjacency_iterator
adjacent_vertices(
    const typename static_graph_adapter<DIRECTION>::vertex_descriptor& v,
    const static_graph_adapter<DIRECTION>& g)
{
  return g.adjacent_vertices(v);
}

template <typename DIRECTION>
inline
typename static_graph_adapter<DIRECTION>::out_edge_iterator
out_edges(const typename static_graph_adapter<DIRECTION>::vertex_descriptor& v,
          const static_graph_adapter<DIRECTION>& g)
{
  return g.out_edges(v);
}

template <typename DIRECTION>
inline
bool
is_directed(const static_graph_adapter<DIRECTION>& g)
{
  return DIRECTION::is_directed();
}

template <typename DIRECTION>
inline
bool
is_undirected(const static_graph_adapter<DIRECTION>& g)
{
  return !is_directed(g);
}

template <typename DIRECTION>
inline
bool
is_bidirectional(const static_graph_adapter<DIRECTION>& g)
{
  return DIRECTION::is_bidirectional();
}

template <typename DIRECTION>
class vertex_id_map<static_graph_adapter<DIRECTION> > :
  public put_get_helper<typename static_graph_adapter<DIRECTION>::size_type,
                        vertex_id_map<static_graph_adapter<DIRECTION> > > {
public:
  typedef typename static_graph_adapter<DIRECTION>::vertex_descriptor key_type;
  typedef typename static_graph_adapter<DIRECTION>::size_type value_type;

  vertex_id_map() {}
  value_type operator[] (const key_type& k) const { return k; }
};

template <typename DIRECTION>
class edge_id_map<static_graph_adapter<DIRECTION> > :
  public put_get_helper<typename static_graph_adapter<DIRECTION>::size_type,
                        edge_id_map<static_graph_adapter<DIRECTION> > > {
public:
  typedef typename static_graph_adapter<DIRECTION>::edge_descriptor key_type;
  typedef typename static_graph_adapter<DIRECTION>::size_type value_type;

  edge_id_map() {}
  value_type operator[] (const key_type& k) const { return k.id; }
};

template <typename DIRECTION>
inline
void init(typename static_graph_adapter<DIRECTION>::size_type n,
          typename static_graph_adapter<DIRECTION>::size_type m,
          typename static_graph_adapter<DIRECTION>::size_type* srcs,
          typename static_graph_adapter<DIRECTION>::size_type* dests,
          static_graph_adapter<DIRECTION>& g)
{
  return g.init(n, m, srcs, dests);
}

template <typename DIRECTION>
inline
void clear(static_graph_adapter<DIRECTION>& g)
{
  return g.clear();
}

/***************************************/

template <typename vertex_t, typename count_t>
class static_in_adjacency_iterator {
public:
  static_in_adjacency_iterator() : adj(0) {}

  static_in_adjacency_iterator(count_t* ind, vertex_t* adj_, count_t i) :
    adj(adj_ + ind[i]) {}

  vertex_t operator[](count_t p)
  {
    vertex_t neigh = adj[p];
    return neigh;
  }

private:
  vertex_t* adj;
};

template <typename vertex_t, typename count_t>
class static_in_edge_iterator {
public:
  static_in_edge_iterator() : adj(0), original_ids(0), vid(0) {}

  static_in_edge_iterator(count_t* ind, vertex_t* adj_,
                          count_t* oi, count_t i) :
    adj(adj_ + ind[i]), original_ids(oi + ind[i]), vid(i) {}

  static_edge_adapter<vertex_t, count_t> operator[](count_t p)
  {
    return static_edge_adapter<vertex_t, count_t>(adj[p], vid, original_ids[p]);
  }

private:
  vertex_t* adj;
  count_t* original_ids;
  count_t vid;
};

template <>
class static_graph_adapter<bidirectionalS> {
public:
  typedef unsigned long size_type;
  typedef unsigned long vertex_descriptor;
  typedef static_edge_adapter<vertex_descriptor, size_type> edge_descriptor;
  typedef static_vertex_iterator<vertex_descriptor, size_type> vertex_iterator;
  typedef static_adjacency_iterator<vertex_descriptor, size_type>
          adjacency_iterator;
  typedef static_in_adjacency_iterator<vertex_descriptor, size_type>
          in_adjacency_iterator;
  typedef static_edge_iterator<vertex_descriptor, size_type> edge_iterator;
  typedef static_out_edge_iterator<vertex_descriptor, size_type>
          out_edge_iterator;
  typedef static_in_edge_iterator<vertex_descriptor, size_type>
          in_edge_iterator;
  typedef bidirectionalS directed_category;
  typedef static_graph<bidirectionalS> graph_type;

  static_graph_adapter(static_graph<bidirectionalS>* g) :
    the_graph(g), initialized_by_ptr(true) {}

  static_graph_adapter() : the_graph(new static_graph<bidirectionalS>),
                                  initialized_by_ptr(false) {}

  static_graph_adapter(const static_graph_adapter& g) :
    initialized_by_ptr(false)
  {
    the_graph = new static_graph<bidirectionalS>(*(g.get_graph()));
  }

  ~static_graph_adapter()
  {
    if (!initialized_by_ptr) delete the_graph;
  }

  static_graph_adapter& operator=(const static_graph_adapter& rhs)
  {
    if (initialized_by_ptr)
    {
      initialized_by_ptr = false;
    }
    else
    {
      delete the_graph;
    }

    the_graph = new static_graph<bidirectionalS>(*(rhs.get_graph()));

    return *this;
  }

  void clear()
  {
    the_graph->clear();
  }

  static_graph<bidirectionalS>* get_graph() const { return the_graph; }

  edge_iterator edges() const
  {
    return edge_iterator(the_graph->src_points, the_graph->end_points,
                         the_graph->internal_ids);
  }

  void init(size_type n, size_type m, vertex_descriptor* srcs,
            vertex_descriptor* dests)
  {
    the_graph->init(n, m, srcs, dests);
  }

  edge_descriptor get_edge(size_type i) const
  {
    size_type int_id = the_graph->internal_ids[i];

    vertex_descriptor v1 = the_graph->src_points[int_id];
    vertex_descriptor v2 = the_graph->end_points[int_id];

    return edge_descriptor(v1, v2, i);
  }

  vertex_descriptor get_vertex(size_type i) const
  {
    return (vertex_descriptor) i;
  }

  in_edge_iterator in_edges(const vertex_descriptor& v) const
  {
    return in_edge_iterator(the_graph->rev_index, the_graph->rev_end_points,
                            the_graph->rev_original_ids, v);
  }

  out_edge_iterator out_edges(const vertex_descriptor& v) const
  {
    return out_edge_iterator(the_graph->index, the_graph->end_points,
                             the_graph->original_ids, v);
  }

  adjacency_iterator adjacent_vertices(const vertex_descriptor& v) const
  {
    return adjacency_iterator(the_graph->index, the_graph->end_points, v);
  }

  in_adjacency_iterator in_adjacent_vertices(const vertex_descriptor& v) const
  {
    return in_adjacency_iterator(the_graph->rev_index,
                                 the_graph->rev_end_points, v);
  }

  vertex_descriptor operator[](size_type i)
  {
    return (vertex_descriptor) the_graph->rev_index[i];
  }

  size_type* get_index() const { return the_graph->index; }
  size_type* get_rev_index() const { return the_graph->rev_index; }

  vertex_descriptor* get_src_points() const { return the_graph->src_points; }
  vertex_descriptor* get_end_points() const { return the_graph->end_points; }
  vertex_descriptor* get_rev_src_points() { return the_graph->rev_src_points; }
  vertex_descriptor* get_rev_end_points() { return the_graph->rev_end_points; }

  size_type get_cross_index(vertex_descriptor eid) const
  {
    return the_graph->cross_index[eid];
  }

  size_type get_cross_cross_index(vertex_descriptor eid) const
  {
    return the_graph->cross_cross_index[eid];
  }

  size_type get_order() const { return the_graph->n; }
  size_type get_size() const { return the_graph->m; }

  size_type get_out_degree(vertex_descriptor i) const
  {
    size_type* index = the_graph->index;
    size_type deg = index[i + 1] - index[i];
    return deg;
  }

  size_type get_in_degree(vertex_descriptor i) const
  {
    size_type* index = the_graph->rev_index;
    size_type deg = index[i + 1] - index[i];
    return deg;
  }

  size_type get_degree(vertex_descriptor i) const
  {
    return get_in_degree(i) + get_out_degree(i);
  }

  vertex_iterator vertices() const { return vertex_iterator(); }

  void print() const
  {
    printf("num Verts = %lu\n", the_graph->n);
    printf("num Edges = %lu\n", the_graph->m);

    for (size_type i = 0; i < the_graph->n; ++i)
    {
      size_type deg = the_graph->index[i + 1] - the_graph->index[i];
      printf("%lu\t: [%4lu] {", i, deg);

      for (size_type j = 0; j < deg; ++j)
      {
        size_type idx = the_graph->index[i];
        printf(" %lu", the_graph->end_points[idx + j]);
      }

      printf(" }\n");
    }
  }

private:
  static_graph<bidirectionalS>* the_graph;
  bool initialized_by_ptr;
};

inline
static_graph_adapter<bidirectionalS>::size_type
in_degree(const static_graph_adapter<bidirectionalS>::vertex_descriptor& v,
          const static_graph_adapter<bidirectionalS>& g)
{
  return g.get_in_degree(v);
}

inline
static_graph_adapter<bidirectionalS>::in_adjacency_iterator
in_adjacent_vertices(
    const static_graph_adapter<bidirectionalS>::vertex_descriptor& v,
    const static_graph_adapter<bidirectionalS>& g)
{
  return g.in_adjacent_vertices(v);
}

inline
static_graph_adapter<bidirectionalS>::in_edge_iterator
in_edges(
    const static_graph_adapter<bidirectionalS>::vertex_descriptor& v,
    const static_graph_adapter<bidirectionalS>& g)
{
  return g.in_edges(v);
}

// Needed to implement to go over both the in and out going edges if we want
// UNDIRECTED.
template <typename visitor>
void visit_edges(static_graph_adapter<bidirectionalS>& g,
                 static_graph_adapter<bidirectionalS>::vertex_descriptor v,
                 visitor& vis, int par_cutoff, bool use_future, int directed)
{
  typedef static_graph_adapter<bidirectionalS>::size_type size_type;
  typedef static_graph_adapter<bidirectionalS>::edge_descriptor edge_descriptor;
  typedef static_graph_adapter<bidirectionalS>::out_edge_iterator
          out_edge_iterator;
  typedef static_graph_adapter<bidirectionalS>::in_edge_iterator
          in_edge_iterator;

  size_type deg;

  if (directed != REVERSED)
  {
    out_edge_iterator eit = out_edges(v, g);
    deg = out_degree(v, g);

    if (deg >= par_cutoff)
    {
      if (use_future)
      {
        #pragma mta assert parallel
        #pragma mta loop future
        for (size_type i = 0; i < deg; ++i)
        {
          edge_descriptor e = eit[i];
          vis(e);
        }
      }
      else
      {
        #pragma mta assert parallel
        for (size_type i = 0; i < deg; ++i)
        {
          edge_descriptor e = eit[i];
          vis(e);
        }
      }
    }
    else
    {
      for (size_type i = 0; i < deg; ++i)
      {
        edge_descriptor e = eit[i];
        vis(e);
      }
    }
  }

  if (directed != DIRECTED)
  {
    in_edge_iterator eit = in_edges(v, g);
    deg = in_degree(v, g);

    if (deg >= par_cutoff)
    {
      if (use_future)
      {
        #pragma mta assert parallel
        #pragma mta loop future
        for (size_type i = 0; i < deg; ++i)
        {
          edge_descriptor e = eit[i];
          vis.visit_in(e);
        }
      }
      else
      {
        #pragma mta assert parallel
        for (size_type i = 0; i < deg; ++i)
        {
          edge_descriptor e = eit[i];
          vis.visit_in(e);
        }
      }
    }
    else
    {
      for (size_type i = 0; i < deg; ++i)
      {
        edge_descriptor e = eit[i];
        vis.visit_in(e);
      }
    }
  }
}

}

#endif

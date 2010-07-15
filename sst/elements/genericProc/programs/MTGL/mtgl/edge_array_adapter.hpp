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
/*! \file edge_array_adapter.hpp

    \author Jon Berry (jberry@sandia.gov)

    \date 12/11/2008
*/
/****************************************************************************/

#ifndef MTGL_EDGE_ARRAY_ADAPTER_HPP
#define MTGL_EDGE_ARRAY_ADAPTER_HPP

#include <cstdlib>

#include <mtgl/mtgl_adapter.hpp>
#include <mtgl/util.hpp>

namespace mtgl {

template <typename uinttype> class edge_array_adapter;

template <typename uinttype>
class edge_array_edge_adapter : public pair<uinttype, uinttype> {
public:
  template <typename T>
  edge_array_edge_adapter(uinttype v1, uinttype v2, T eid) :
    pair<uinttype, uinttype>(v1, v2), id((T)eid) {}

  edge_array_edge_adapter(pair<uinttype, uinttype> p, uinttype eid) :
    pair<uinttype, uinttype>(p), id(eid) {}

  uinttype get_id() const { return id; }

private:
  uinttype id;
};

template <typename uinttype>
class edge_array_vertex_iterator {
public:
  // We assume that vertices are packed 0..n-1.
  edge_array_vertex_iterator() {}

  uinttype operator[](uinttype p) { return p; }
};

template <typename uinttype>
class edge_array_edge_iterator {
public:
  edge_array_edge_iterator() : srcs(0), dests(0) {}  // Doesn't work!
  edge_array_edge_iterator(uinttype* scs, uinttype* dsts) :
    srcs(scs), dests(dsts) {}

  edge_array_edge_adapter<uinttype> operator[](uinttype p)
  {
    return edge_array_edge_adapter<uinttype>(srcs[p], dests[p], p);
  }

private:
  uinttype* srcs;
  uinttype* dests;
};

template<typename uinttype>
class edge_array_adapter {
public:
  typedef uinttype size_type;
  typedef uinttype vertex_descriptor;
  typedef edge_array_edge_adapter<uinttype> edge_descriptor;
  typedef edge_array_vertex_iterator<uinttype> vertex_iterator;
  typedef void adjacency_iterator;  // none!
  typedef void in_adjacency_iterator;  // none!
  typedef edge_array_edge_iterator<uinttype> edge_iterator;
  typedef void out_edge_iterator;  // none!
  typedef void in_edge_iterator;  // none!
  typedef directedS directed_category;
  typedef void graph_type;  // none!

  // We assume that vertices are packed 0..n-1.
  edge_array_adapter(size_type* scs, size_type* dsts, size_type nn,
                     size_type mm) : srcs(scs), dests(dsts),  n(nn), m(mm) {}

  size_type get_order() const { return n; }
  size_type get_size() const { return m; }

  vertex_iterator vertices() const { return vertex_iterator(); }
  edge_iterator edges() const { return edge_iterator(srcs, dests); }

  vertex_descriptor get_vertex(size_type i) const { return i; }

private:
  size_type* srcs;
  size_type* dests;
  size_type n;
  size_type m;
};

template <typename uinttype>
inline
typename edge_array_adapter<uinttype>::vertex_descriptor
get_vertex(typename edge_array_adapter<uinttype>::size_type i,
           const edge_array_adapter<uinttype>& g)
{
  return g.get_vertex(i);
}

template <typename uinttype>
inline
uinttype
num_vertices(const edge_array_adapter<uinttype>& g)
{
  return g.get_order();
}

template <typename uinttype>
inline
uinttype
num_edges(const edge_array_adapter<uinttype>& g)
{
  return g.get_size();
}

template <typename uinttype>
inline
uinttype
source(const edge_array_edge_adapter<uinttype>& e,
       const edge_array_adapter<uinttype>& g)
{
  return e.first;
}

template <typename uinttype>
inline
uinttype
target(const edge_array_edge_adapter<uinttype>& e,
       const edge_array_adapter<uinttype>& g)
{
  return e.second;
}

template <typename uinttype>
inline
uinttype
other(const edge_array_edge_adapter<uinttype>& e, const uinttype& v,
      const edge_array_adapter<uinttype>& g)
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
    printf("error: other((%lu,%lu), %lu)\n", (long unsigned) e.first,
           (long unsigned) e.second, (long unsigned) e.get_id());

    return e.first;
  }
}

template <typename uinttype>
inline
edge_array_vertex_iterator<uinttype>
vertices(const edge_array_adapter<uinttype>& g)
{
  return g.vertices();
}

template <typename uinttype>
inline
edge_array_edge_iterator<uinttype>
edges(const edge_array_adapter<uinttype>& g)
{
  return g.edges();
}

template <typename uinttype>
inline
bool
is_directed(const edge_array_adapter<uinttype>& g)
{
  return true;
}

template <typename uinttype>
inline
bool
is_undirected(const edge_array_adapter<uinttype>& g)
{
  return false;
}

template <typename uinttype>
inline
bool
is_bidirectional(const edge_array_adapter<uinttype>& g)
{
  return false;
}

template <typename uinttype>
class vertex_id_map<edge_array_adapter<uinttype> > :
  public put_get_helper<uint64_t, vertex_id_map<edge_array_adapter
                                                <uinttype> > > {
public:
  typedef typename
    graph_traits<edge_array_adapter<uinttype> >::vertex_descriptor key_type;
  typedef uinttype value_type;

  vertex_id_map() {}
  value_type operator[] (const key_type& k) const { return k; }
};

template <typename uinttype>
class edge_id_map<edge_array_adapter<uinttype> > :
  public put_get_helper<uinttype, edge_id_map<edge_array_adapter
                                              <uinttype> > > {
public:
  typedef typename
    graph_traits<edge_array_adapter<uinttype> >::edge_descriptor key_type;
  typedef uinttype value_type;

  edge_id_map() {}
  value_type operator[] (const key_type& k) const { return k.get_id(); }
};

}

#endif

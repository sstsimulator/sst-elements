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
/*! \file breadth_first_search.hpp

    \brief Basic breadth first search class that expands only one level
           per call.

    \author Kamesh Madduri
    \author William McLendon (wcmclen@sandia.gov)

    \date 6/6/2005
    \date 7/10/2007
*/
/****************************************************************************/

#ifndef MTGL_BREADTH_FIRST_SEARCH_HPP
#define MTGL_BREADTH_FIRST_SEARCH_HPP

#include <climits>

#include <mtgl/mtgl_adapter.hpp>
#include <mtgl/graph_traits.hpp>
#include <mtgl/queue.hpp>
#include <mtgl/util.hpp>

#define BREADTH_FIRST_SEARCH_PARLIMIT 100

namespace mtgl {

template <typename graph>
struct default_bfs_visitor {
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;

  void bfs_root(vertex_t& v) const {}
  void discover_vertex(vertex_t& v) const {}
  void examine_vertex(vertex_t& v) const {}
  void tree_edge(edge_t& e, vertex_t& src, vertex_t& dest) const {}
  void back_edge(edge_t& e, vertex_t& src, vertex_t& dest) const {}
  void finish_vertex(vertex_t& v) const {}
};

template<typename graph, typename ColorMap, typename bfs_visitor>
class breadth_first_search {
public:
  typedef enum { white, gray, black } SearchColor;

  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;

  breadth_first_search(graph& g, ColorMap& c, bfs_visitor vis,
                       size_type _directed = (size_type)DIRECTED) :
    _g(g), visitor(vis), color(c), count(0), directed(_directed),
    Q(queue<size_type>(num_vertices(_g))) {}

  ~breadth_first_search() { Q.clear(); }

  SearchColor incrColor(size_type& color)
  {
    size_type clr = mt_incr(color, 1);
    if (clr == 0)
    {
      return breadth_first_search::white;
    }
    else if (clr < num_vertices(_g))
    {
      (void) mt_incr(color, num_vertices(_g));
      return breadth_first_search::gray;
    }
    else
    {
      return breadth_first_search::black;
    }
  }

  void bfsRoot(vertex_t& v) { visitor.bfs_root(v); }

  void run(size_type startIndex)
  {
    vertex_id_map<graph> vid_map = get(_vertex_id_map, _g);

    if (startIndex < 0 || startIndex >= num_vertices(_g)) return;

    vertex_t v = get_vertex(startIndex, _g);
    if (directed == (size_type)UNDIRECTED)
    {
      internal_bfs_undirected_visitor helper(_g, color, Q,
                                             count, visitor, vid_map);
      helper.bfsVisit(v);
    }
    else if (directed == (size_type)DIRECTED)
    {
      internal_bfs_directed_visitor helper(_g, color, Q,
                                           count, visitor, vid_map);
      helper.bfsVisit(v);
    }
    else if (directed == (size_type)REVERSED)
    {
      internal_bfs_reversed_visitor helper(_g, color, Q, count,
                                           visitor, vid_map);
      helper.bfsVisit(v);
    }
  };

  class internal_bfs_directed_visitor {
  public:
    internal_bfs_directed_visitor(graph& _ga, ColorMap& _cmap,
                                  queue<size_type>& _Q, size_type& _count,
                                  bfs_visitor& _vis,
                                  vertex_id_map<graph>& _vid_map) :
      ga(_ga), cmap(_cmap), Q(_Q), count(_count), visitor(_vis),
      vid_map(_vid_map)
    { /*printf("%d bfs: directed()\n",DIRECTED);*/ }

    void set_data(vertex_t& v) { src = v; }

    inline void operator()(edge_t e)
    {
      vertex_t first = source(e, ga);
      vertex_t second = target(e, ga);

      if (src != first) return;

      visitor.discover_vertex(second);
      size_type v2id = get(vid_map, second);
      size_type clr = mt_incr(cmap[v2id], 1);

      if (clr == breadth_first_search::white)
      {
        visitor.tree_edge(e, src, second);
        Q.push(v2id, mt_incr(count, 1));
      }
      else
      {
        visitor.back_edge(e, src, second);
      }
    }

    void bfsVisit(vertex_t v)
    {
      src = v;
      visit_edges(ga, v, *this, BREADTH_FIRST_SEARCH_PARLIMIT, false);
      visitor.finish_vertex(v);
    }

    graph& ga;
    ColorMap& cmap;
    queue<size_type>& Q;
    size_type& count;
    const bfs_visitor& visitor;
    vertex_id_map<graph>& vid_map;
    vertex_t src;
  };

  class internal_bfs_undirected_visitor {
  public:
    internal_bfs_undirected_visitor(graph& _ga, ColorMap& _cmap,
                                    queue<size_type>& _Q, size_type& _count,
                                    bfs_visitor& _vis,
                                    vertex_id_map<graph>& _vid_map) :
      ga(_ga), cmap(_cmap), Q(_Q), count(_count), visitor(_vis),
      vid_map(_vid_map)
    { /*printf("%d bfs: undirected()\n",UNDIRECTED);*/ }

    void set_data(vertex_t& v) { src = v; }

    inline void operator()(edge_t e)
    {
      vertex_t oth   = other(e, src, ga);
      visitor.discover_vertex(oth);
      size_type oid = get(vid_map, oth);
      size_type clr = mt_incr(cmap[oid], 1);

      if (clr == breadth_first_search::white)
      {
        visitor.tree_edge(e, src, oth);
        Q.push(oid, mt_incr(count, 1));
      }
      else
      {
        visitor.back_edge(e, src, oth);
      }
    }

    void bfsVisit(vertex_t v)
    {
      src = v;
      visit_edges(ga, v, *this, BREADTH_FIRST_SEARCH_PARLIMIT, false);
      visitor.finish_vertex(v);
    }

    graph& ga;
    ColorMap& cmap;
    queue<size_type>& Q;
    size_type& count;
    const bfs_visitor& visitor;
    vertex_id_map<graph>& vid_map;
    vertex_t src;
  };

  class internal_bfs_reversed_visitor {
  public:
    internal_bfs_reversed_visitor(graph& _ga, ColorMap& _cmap,
                                  queue<size_type>& _Q, size_type& _count,
                                  bfs_visitor& _vis,
                                  vertex_id_map<graph>& _vid_map) :
      ga(_ga), cmap(_cmap), Q(_Q), count(_count), visitor(_vis),
      vid_map(_vid_map)
    { /*printf("%d bfs: reversed()\n",REVERSED);*/ }

    void set_data(vertex_t& v) { src = v; }

    inline void operator()(edge_t e)
    {
      vertex_t first = source(e, ga);
      vertex_t second = target(e, ga);

      if (src != second) return;

      visitor.discover_vertex(first);
      size_type v2id = get(vid_map, first);
      size_type clr  = mt_incr(cmap[v2id], 1);

      if (clr == breadth_first_search::white)
      {
        visitor.tree_edge(e, src, first);
        Q.push(v2id, mt_incr(count, 1));
      }
      else
      {
        visitor.back_edge(e, src, first);
      }
    }

    void bfsVisit(vertex_t v)
    {
      src = v;
      visit_edges(ga, v, *this, BREADTH_FIRST_SEARCH_PARLIMIT, false);
      visitor.finish_vertex(v);
    }

    graph& ga;
    ColorMap& cmap;
    queue<size_type>& Q;
    size_type& count;
    const bfs_visitor& visitor;
    vertex_id_map<graph>& vid_map;
    vertex_t src;
  };

  graph& _g;
  bfs_visitor visitor;
  ColorMap& color;
  size_type count;
  size_type directed;
  queue<size_type> Q;
};

};

#endif

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
/*! \file mask_search.hpp

    \brief This class is to be used to iterate through a graph and visit the
           neighbors of the node based on the mask_array

    \author Brad Mancke

    \date 12/11/2007
*/
/****************************************************************************/

#ifndef MTGL_MASK_SEARCH_HPP
#define MTGL_MASK_SEARCH_HPP

#include <climits>

#include <mtgl/mtgl_adapter.hpp>
#include <mtgl/graph_traits.hpp>
#include <mtgl/util.hpp>

namespace mtgl {

template <typename graph>
struct default_mask_search_visitor {
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  //void mask_search_root(vertex& v) const{}
  void pre_visit(vertex& v) {}
  int start_test(vertex& v) { return true; }
  int visit_test(edge& e, vertex& v) { return true; }
  void tree_edge(edge& e, vertex& src) const {}
  //void back_edge(edge& e, vertex& src) const {}
  void finish_vertex(vertex& v) const {}
};

template <typename graph, typename user_visitor, int directed, int logic,
          int pcutoff>
class internal_mask_search_visitor;

template<typename graph, typename mask_search_visitor,
         int directed = UNDIRECTED, int filter = NO_FILTER, int pcutoff = 100>
class mask_search {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;
  typedef typename graph_traits<graph>::vertex_iterator vertex_iterator;

  mask_search(graph& g, mask_search_visitor vis) : _g(g), visitor(vis) {}
  mask_search(const mask_search& b) : _g(b._g), visitor(b.visitor) {}
  ~mask_search() { }

  void run();

  mask_search_visitor& visitor;
  graph& _g;
};

template <typename graph, typename user_visitor, int directed = UNDIRECTED,
          int logic = NO_FILTER, int pcutoff = 100>
class internal_mask_search_visitor {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  user_visitor visitor;
  vertex src;
  graph& _g;

  internal_mask_search_visitor(graph& g, user_visitor& vis) :
    visitor(vis), _g(g) {}

  internal_mask_search_visitor(const internal_mask_search_visitor& i) :
    visitor(i.visitor), _g(i._g), src(i.src) {}

  void set_data(vertex v) { src = v; }

  void operator()(edge e)
  {
    vertex v2 = other(e, src, _g);

    if(visitor.visit_test(e, src)) visitor.tree_edge(e, src);
  }

  void mask_search_visit(vertex v)
  {
    visitor.pre_visit(v);
    src = v;
    visit_edges(_g, v, *this, pcutoff, false);
    visitor.finish_vertex(v);
  }
};

// Directed, no filter.
template <typename graph, typename user_visitor, int pcutoff>
class internal_mask_search_visitor<graph, user_visitor, DIRECTED, NO_FILTER,
                                   pcutoff> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  user_visitor visitor;
  vertex src;
  graph& _g;

  internal_mask_search_visitor(graph& g, user_visitor& vis) :
    visitor(vis), _g(g) {}

  internal_mask_search_visitor(const internal_mask_search_visitor& i) :
    visitor(i.visitor), _g(i._g), src(i.src) {}

  void set_data(vertex v) { src = v; }

  void operator()(edge e)
  {
    vertex first = source(e, _g);

    if (first != src) return;

    vertex v2 = target(e, _g);

    if(visitor.visit_test(e, src)) visitor.tree_edge(e, src);
  }

  void mask_search_visit(vertex v)
  {
    visitor.pre_visit(v);
    src = v;
    visit_edges(_g, v, *this, pcutoff, true);
    visitor.finish_vertex(v);
  }
};

// Reverse no filter.
template <typename graph, typename user_visitor, int pcutoff>
class internal_mask_search_visitor<graph, user_visitor, REVERSED, NO_FILTER,
                                   pcutoff> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  user_visitor visitor;
  vertex src;
  graph& _g;

  internal_mask_search_visitor(graph& g, user_visitor& vis) :
    visitor(vis), _g(g) {}

  internal_mask_search_visitor(const internal_mask_search_visitor& i) :
    visitor(i.visitor), _g(i._g), src(i.src) {}

  void set_data(vertex v) { src = v; }

  void operator()(edge e)
  {
    vertex second = target(e, _g);

    if (second != src) return;

    vertex v2 = source(e, _g);

    if( visitor.visit_test(e, src)) visitor.tree_edge(e, src);
  }

  void mask_search_visit(vertex v)
  {
    visitor.pre_visit(v);
    src = v;
    visit_edges(_g, v, *this, pcutoff, true);
    visitor.finish_vertex(v);
  }
};

/*  The run function will go through each mask layer, and visit each */
template<typename graph, typename mask_search_visitor, int directed,
         int filter, int pcutoff>
void mask_search<graph, mask_search_visitor, directed, filter, pcutoff>::run()
{

  typedef internal_mask_search_visitor<graph, mask_search_visitor, directed,
                                       filter, pcutoff>
           internal_mask_search_visitor_t;

  internal_mask_search_visitor_t helper(_g, visitor);

  int** load_balance_masks = _g.get_load_balance_masks();
  int* load_balance_totals = _g.get_load_balance_totals();
  int** load_balance_parallelize_flags =
    _g.get_load_balance_parallelize_flags();

  vertex_iterator verts = vertices(_g);

  for (int i = 0; i < _g.get_num_load_balance_masks(); i++)
  {
    if (load_balance_parallelize_flags[i][0] == SERIAL)
    {
      for (int j = 0; j < load_balance_totals[i]; j++)
      {
        internal_mask_search_visitor_t helper2(helper);
        helper2.mask_search_visit(verts[load_balance_masks[i][j]]);
      }
    }
    else if (load_balance_parallelize_flags[i][0] == PARALLEL)
    {
      #pragma mta assert parallel
      for (int j = 0; j < load_balance_totals[i]; j++)
      {
        internal_mask_search_visitor_t helper2(helper);
        helper2.mask_search_visit(verts[load_balance_masks[i][j]]);
      }
    }
    else if (load_balance_parallelize_flags[i][0] == PARALLEL_FUTURE)
    {
      #pragma mta loop future
      for (int j = 0; j < load_balance_totals[i]; j++)
      {
        internal_mask_search_visitor_t helper2(helper);
        helper2.mask_search_visit(verts[load_balance_masks[i][j]]);
      }
    }
  }
}

}

#endif

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
/*! \file psearch.hpp

    \brief The "depth-first search" algorithm class.  This is a slight
           misnomer.  Although it is recursive like a psearch, this algorithm
           explores neighbors in parallel like a breadth-first search.

    \author Jon Berry (jberry@sandia.gov)

    \date 3/2/2005
*/
/****************************************************************************/

#ifndef MTGL_PSEARCH_HPP
#define MTGL_PSEARCH_HPP

#include <mtgl/util.hpp>
#include <mtgl/mtgl_adapter.hpp>
#include <mtgl/graph_traits.hpp>

namespace mtgl {

/// \brief This stub class defines the events at which users will have a
///        chance to respond.
template <typename graph>
struct default_psearch_visitor {
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  void psearch_root(vertex& v) {}
  void pre_visit(vertex& v) {}
  int start_test(vertex& v) { return false; }
  int visit_test(edge& e, vertex& v) { return false; }
  void tree_edge(edge& e, vertex& src) {}
  void back_edge(edge& e, vertex& src) {}
  void finish_vertex(vertex& v) {}

  /*! \fn void psearch_root(vertex& v)
      \brief This routine is called once at the search root, i.e., once per
             call to psearch::run().
      \param v The search root.
  */
  /*! \fn void pre_visit(vertex& v)
      \brief This routine is called before  visiting the edges of vertex v.
      \param v The vertex about to be visited.
  */
  /*! \fn int start_test(vertex& v)
      \brief This routine is called once at the search root, i.e., once per
             call to psearch::run().  It is used to determine whether the
             root is searched or not.
      \param v The vertex about to be visited.

      The default return value is false.  Therefore, the compiler can
      eliminate the call to this routine if the user does not override it.
  */
  /*! \fn int visit_test(edge& e, vertex& src)
      \brief When vertex v has been encountered, and its edge list is being
             traversed, each traversal of an incident edge is preceded by a
             call to this routine.  Depending on the template parameters to
             the psearch object, the decision about whether to traverse e
             will be made by noting the standard search criterion (has the
             destination vertex been encountered?), then using a boolean
             logic operation to combine this information with the result of
             visit_test.
      \param e The current edge being traversed.
      \param src The endpoint of e from which the traversal originated.

      The default return value is false, and the default boolean operation
      is "or".  Therefore, the compiler can eliminate the call to this
      routine if the user does not override it.
  */
  /*! \fn void tree_edge(edge& e, vertex& src)
      \brief This currently is called whenever a non-tree edge is
             encountered.  The latter may be a back, forward, or cross edge.
      \param e The current edge being traversed.
      \param src The endpoint of e from which the traversal originated.
  */
  /*! \fn void back_edge(edge& e, vertex& src)
      \brief This currently is called whenever a non-tree edge is
             encountered.  The latter may be a back, forward, or cross edge.
      \param e The current edge being traversed.
      \param src The endpoint of e from which the traversal originated.
  */
  /*! \fn void finish_vertex(vertex& v)
      \brief This routine is called after all of the incident edges of v
             have been explored.
      \param v A vertex whose incident edges have been explored.
  */
};

/***/

template <typename graph, typename ColorMap, typename user_visitor,
          int directed, int logic, int pcut>
class internal_psearch_visitor;

/***/

/*! \brief The psearch object takes from the application programmer a
           structure that will hold search colors, and a visitor object that
           will tailor the search.

    This class supports thread-safe concurrent searches from many sources,
    with the search color map shared among searches.

    Psearch visits only the parts of the graph that are reachable from the
    source vertex, not the whole graph.
*/
template <typename graph, typename ColorMap,
          typename psearch_visitor = default_psearch_visitor<graph>,
          int filter = NO_FILTER, int directed = UNDIRECTED,
          int parallelization_cutoff = 100>
class psearch {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  psearch(graph* g, ColorMap c, psearch_visitor& vis);
  psearch(graph& g, ColorMap c, psearch_visitor& vis);
  psearch(graph& g, psearch_visitor& v) : _g(g), visitor(v){}
  ~psearch();

  void run(vertex v);
  void serialRun(vertex* v);
  void mixedRun(vertex* v, bool* pmask);

  typedef enum { white, gray, black } SearchColor;

public:
  psearch_visitor& visitor;
  ColorMap color;
  graph& _g;

  /*! \fn psearch(graph* g, ColorMap c, psearch_visitor& vis)
      \brief This constructor initializes member data.
      \param g A pointer to the graph to search.
      \param c An indexed data structure of size equal to the number of
               vertices that will hold search colors (white, gray, black).
      \param vis A psearch visitor class that will hold application programmer
                 methods to tailor the search.
  */
  /*! \fn psearch(graph& g, ColorMap c, psearch_visitor& vis)
      \brief This constructor initializes member data.
      \param g The graph to search.
      \param c An indexed data structure of size equal to the number of
               vertices that will hold search colors (white, gray, black).
      \param vis A psearch visitor class that will hold application programmer
                 methods to tailor the search.
  */
  /*! \fn ~psearch()
      \brief The destructor.
  */
  /*! \fn void run(vertex v)
      \brief Initiate the search algorithm, with v as the search root.  This
             will cause visitor.psearchRoot(v) to be called.
  */
  /*! \var typedef enum { white, gray, black } SearchColor
      \brief The standard search marking colors: white means unvisited, gray
             means visited, but not finished, and black means finished.
  */
  /*! \var psearch_visitor& visitor
      \brief The application programmer instantiates each psearch object with
             an instance of a "visitor".  This will be an instance of
             default_psearch_visitor or a user-defined child class of
             default_psearch_visitor.
  */
  /*! \var ColorMap color
      \brief  The application programmer provides an indexed data structure of
              the template type "ColorMap" to each psearch instance. We
              remember this value in the member "color".

      It is very important that this member be declared as shown here;
      declaring it as a reference member can lead to a hot spot.
  */
  /*! \var graph& _g
      \brief The graph to be searched.
  */
};

/***/

template <typename graph, typename ColorMap, typename user_visitor,
          int directed = UNDIRECTED, int logic = NO_FILTER, int pcut = 100>
class internal_psearch_visitor {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;
  typedef psearch<graph, ColorMap, user_visitor, logic, directed, pcut>
          psearch_t;

  internal_psearch_visitor(graph& g, vertex v, ColorMap& col, user_visitor& vis,
                           vertex_id_map<graph>& vm) :
    visitor(vis), color(col), vid_map(vm), _g(g), src(v) {}

  void set_data(vertex v) { src = v; }
  void psearchRoot(vertex v) { visitor.psearch_root(v); }

  void operator()(edge e)
  {
    vertex v2 = other(e, src, _g);
    int clr = mt_incr(color[get(vid_map, v2)], 1);

    if ((clr == psearch_t::white) || visitor.visit_test(e, src))
    {
      visitor.tree_edge(e, src);
      internal_psearch_visitor next(*this);
      next.psearchVisit(v2);
    }
    else
    {
      // Call back edge for back, forward, cross.
      visitor.back_edge(e, src);
    }
  }

  void psearchVisit(vertex v)
  {
    visitor.pre_visit(v);
    src = v;
    visit_edges(_g, v, *this, pcut, true, directed);
    visitor.finish_vertex(v);
  }

public:
  user_visitor visitor;
  ColorMap color;
  vertex src;
  vertex_id_map<graph>& vid_map;
  graph& _g;

  /*! \fn internal_psearch_visitor(graph& g, vertex v, ColorMap& col,
                                   user_visitor& vis, vertex_id_map<graph>& vm)
      \brief This constructor initializes member data.
      \param g The graph to search.
      \param v The vertex that is currently being visited.
      \param col An indexed data structure of size equal to the number of
                 vertices that will hold search colors (white, gray, black).
      \param vis An internal visitor class that will hold application
                 programmer methods to tailor the search.
      \param vm The vertex id map for g.
  */
  /*! \fn void set_data(vertex v)
      \brief Set the vertex visited by this visitor to v.
  */
  /*! \fn void psearchRoot(vertex v)
      \brief Initiate a search from vertex v.
  */
  /*! \fn void operator()(edge e)
      \brief Visit edge e.
  */
  /*! \fn void psearchVisit(vertex* v)
      \brief Search from vertex v.
  */
  /*! \var user_visitor visitor
      \brief Stores the visitor class instance.
  */
  /*! \var ColorMap color
      \brief  The application programmer provides an indexed data structure of
              the template type "ColorMap" to each psearch instance. We
              remember this value in the member "color".

      It is very important that this member be declared as shown here;
      declaring it as a reference member can lead to a hot spot.
  */
  /*! \var vertex src
      \brief The vertex that is currently being visited.
  */
  /*! \var vertex_id_map<graph>& vid_map
      \brief The vertex id map for _g.
  */
  /*! \var graph& _g
      \brief The graph to be searched.
  */
};

/***/

template <typename graph, typename ColorMap, typename user_visitor, int pcut>
class internal_psearch_visitor<graph, ColorMap, user_visitor, DIRECTED,
                               PURE_FILTER, pcut> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;
  typedef typename graph_traits<graph>::size_type size_type;
  typedef psearch<graph, ColorMap, user_visitor, PURE_FILTER, DIRECTED, pcut>
          psearch_t;

  internal_psearch_visitor(graph& g, vertex v, ColorMap& col, user_visitor& vis,
                           vertex_id_map<graph>& vm) :
    visitor(vis), color(col), vid_map(vm), _g(g), src(v) {}

  void set_data(vertex v) { src = v; }
  void psearchRoot(vertex v) { visitor.psearch_root(v); }

  void operator()(edge e)
  {
    if (visitor.visit_test(e, src))
    {
      vertex v2 = target(e, _g);
      visitor.tree_edge(e, src);
      internal_psearch_visitor next(*this);
      next.psearchVisit(v2);
    }
    else
    {
      // Call back edge for back, forward, cross.
      visitor.back_edge(e, src);
    }
  }

  void visit_in(edge e) { fprintf(stderr, "ERROR: Shouldn't be here.\n"); }

  void psearchVisit(vertex v)
  {
    visitor.pre_visit(v);
    src = v;
    visit_edges(_g, v, *this, pcut, true, DIRECTED);
    visitor.finish_vertex(v);
  }

public:
  user_visitor visitor;
  ColorMap color;
  vertex src;
  vertex_id_map<graph>& vid_map;
  graph& _g;
};

/***/

template <typename graph, typename ColorMap, typename user_visitor, int pcut>
class internal_psearch_visitor<graph, ColorMap, user_visitor, REVERSED,
                               PURE_FILTER, pcut> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;
  typedef typename graph_traits<graph>::size_type size_type;
  typedef psearch<graph, ColorMap, user_visitor, PURE_FILTER, REVERSED, pcut>
          psearch_t;

  internal_psearch_visitor(graph& g, vertex v, ColorMap& col, user_visitor& vis,
                           vertex_id_map<graph>& vm) :
    visitor(vis), color(col), vid_map(vm), _g(g), src(v) {}

  void set_data(vertex v) { src = v; }
  void psearchRoot(vertex v) { visitor.psearch_root(v); }

  void operator()(edge e)
  {
    size_type second_id = get(vid_map, target(e, _g));
    if (second_id != get(vid_map, src))  return;

    if (visitor.visit_test(e, src))
    {
      vertex v2 = source(e, _g);
      visitor.tree_edge(e, src);
      internal_psearch_visitor next(*this);
      next.psearchVisit(v2);
    }
    else
    {
      // Call back edge for back, forward, cross.
      visitor.back_edge(e, src);
    }
  }

  void visit_in(edge e)
  {
    if (visitor.visit_test(e, src))
    {
      vertex v2 = source(e, _g);
      visitor.tree_edge(e, src);
      internal_psearch_visitor next(*this);
      next.psearchVisit(v2);
    }
    else
    {
      // Call back edge for back, forward, cross.
      visitor.back_edge(e, src);
    }
  }

  void psearchVisit(vertex v)
  {
    visitor.pre_visit(v);
    src = v;
    visit_edges(_g, v, *this, pcut, true, REVERSED);
    visitor.finish_vertex(v);
  }

public:
  user_visitor visitor;
  ColorMap color;
  vertex src;
  vertex_id_map<graph>& vid_map;
  graph& _g;
};

/***/

template <typename graph, typename ColorMap, typename user_visitor, int pcut>
class internal_psearch_visitor<graph, ColorMap, user_visitor, UNDIRECTED,
                               PURE_FILTER, pcut> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;
  typedef psearch<graph, ColorMap, user_visitor, PURE_FILTER, UNDIRECTED, pcut>
          psearch_t;

  internal_psearch_visitor(graph& g, vertex v, ColorMap& col, user_visitor& vis,
                           vertex_id_map<graph>& vm) :
    visitor(vis), color(col), vid_map(vm), _g(g), src(v) {}

  void set_data(vertex v) { src = v; }
  void psearchRoot(vertex v) { visitor.psearch_root(v); }

  void operator()(edge e)
  {
    if (visitor.visit_test(e, src))
    {
      vertex v2 = target(e, _g);
      visitor.tree_edge(e, src);
      internal_psearch_visitor next(*this);
      next.psearchVisit(v2);
    }
    else
    {
      // Call back edge for back, forward, cross.
      visitor.back_edge(e, src);
    }
  }

  void visit_in(edge e)
  {
    if (visitor.visit_test(e, src))
    {
      vertex v2 = source(e, _g);
      visitor.tree_edge(e, src);
      internal_psearch_visitor next(*this);
      next.psearchVisit(v2);
    }
    else
    {
      // Call back edge for back, forward, cross.
      visitor.back_edge(e, src);
    }
  }

  void psearchVisit(vertex v)
  {
    visitor.pre_visit(v);
    src = v;
    visit_edges(_g, v, *this, pcut, true, UNDIRECTED);
    visitor.finish_vertex(v);
  }

public:
  user_visitor visitor;
  ColorMap color;
  vertex src;
  vertex_id_map<graph>& vid_map;
  graph& _g;
};

/***/

template <typename graph, typename ColorMap, typename user_visitor, int pcut>
class internal_psearch_visitor<graph, ColorMap, user_visitor, DIRECTED,
                               AND_FILTER, pcut> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;
  typedef typename graph_traits<graph>::size_type size_type;
  typedef psearch<graph, ColorMap, user_visitor, AND_FILTER, DIRECTED, pcut>
          psearch_t;

  internal_psearch_visitor(graph& g, vertex v, ColorMap& col, user_visitor& vis,
                           vertex_id_map<graph>& vm) :
    visitor(vis), color(col), vid_map(vm), _g(g), src(v) {}

  void set_data(vertex v) { src = v; }
  void psearchRoot(vertex v) { visitor.psearch_root(v); }

  void operator()(edge e)
  {
    if (visitor.visit_test(e, src))
    {
      vertex v2 = target(e, _g);
      int clr = mt_incr(color[get(vid_map, v2)], 1);

      if (clr == psearch_t::white)
      {
        visitor.tree_edge(e, src);
        internal_psearch_visitor next(*this);
        next.psearchVisit(v2);
      }
      else
      {
        // Call back edge for back, forward, cross.
        visitor.back_edge(e, src);
      }
    }
  }

  void visit_in(edge e) { fprintf(stderr, "ERROR: Shouldn't be here.\n"); }

  void psearchVisit(vertex v)
  {
    visitor.pre_visit(v);
    src = v;
    visit_edges(_g, v, *this, pcut, true, DIRECTED);
    visitor.finish_vertex(v);
  }

public:
  user_visitor visitor;
  ColorMap color;
  vertex_id_map<graph>& vid_map;
  graph& _g;
  vertex src;
};

/***/

template <typename graph, typename ColorMap, typename user_visitor, int pcut>
class internal_psearch_visitor<graph, ColorMap, user_visitor, REVERSED,
                               AND_FILTER, pcut> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;
  typedef typename graph_traits<graph>::size_type size_type;
  typedef psearch<graph, ColorMap, user_visitor, AND_FILTER, REVERSED, pcut>
          psearch_t;

  internal_psearch_visitor(graph& g, vertex v, ColorMap& col, user_visitor& vis,
                           vertex_id_map<graph>& vm) :
    visitor(vis), color(col), vid_map(vm), _g(g), src(v) {}

  void set_data(vertex v) { src = v; }
  void psearchRoot(vertex v) { visitor.psearch_root(v); }

  void operator()(edge e)
  {
    size_type second_id = get(vid_map, target(e, _g));
    if (second_id != get(vid_map, src))  return;

    if (visitor.visit_test(e, src))
    {
      vertex v2 = source(e, _g);
      int clr = mt_incr(color[get(vid_map, v2)], 1);

      if (clr == psearch_t::white)
      {
        visitor.tree_edge(e, src);
        internal_psearch_visitor next(*this);
        next.psearchVisit(v2);
      }
      else
      {
        // Call back edge for back, forward, cross.
        visitor.back_edge(e, src);
      }
    }
  }

  void visit_in(edge e)
  {
    if (visitor.visit_test(e, src))
    {
      vertex v2 = source(e, _g);
      int clr = mt_incr(color[get(vid_map, v2)], 1);

      if (clr == psearch_t::white)
      {
        visitor.tree_edge(e, src);
        internal_psearch_visitor next(*this);
        next.psearchVisit(v2);
      }
      else
      {
        // Call back edge for back, forward, cross.
        visitor.back_edge(e, src);
      }
    }
  }

  void psearchVisit(vertex v)
  {
    visitor.pre_visit(v);
    src = v;
    visit_edges(_g, v, *this, pcut, true, REVERSED);
    visitor.finish_vertex(v);
  }

public:
  user_visitor visitor;
  ColorMap color;
  vertex src;
  vertex_id_map<graph>& vid_map;
  graph& _g;
};

/***/

template <typename graph, typename ColorMap, typename user_visitor, int pcut>
class internal_psearch_visitor<graph, ColorMap, user_visitor, UNDIRECTED,
                               AND_FILTER, pcut> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;
  typedef psearch<graph, ColorMap, user_visitor, AND_FILTER, UNDIRECTED, pcut>
          psearch_t;

  internal_psearch_visitor(graph& g, vertex v, ColorMap& col, user_visitor& vis,
                           vertex_id_map<graph>& vm) :
    visitor(vis), color(col), vid_map(vm), _g(g), src(v) {}

  void set_data(vertex v) { src = v; }
  void psearchRoot(vertex v) { visitor.psearch_root(v); }

  void operator()(edge e)
  {
    if (visitor.visit_test(e, src))
    {
      vertex v2 = target(e, _g);
      int clr = mt_incr(color[get(vid_map, v2)], 1);

      if (clr == psearch_t::white)
      {
        visitor.tree_edge(e, src);
        internal_psearch_visitor next(*this);
        next.psearchVisit(v2);
      }
      else
      {
        // Call back edge for back, forward, cross.
        visitor.back_edge(e, src);
      }
    }
  }

  void visit_in(edge e)
  {
    if (visitor.visit_test(e, src))
    {
      vertex v2 = source(e, _g);
      int clr = mt_incr(color[get(vid_map, v2)], 1);

      if (clr == psearch_t::white)
      {
        visitor.tree_edge(e, src);
        internal_psearch_visitor next(*this);
        next.psearchVisit(v2);
      }
      else
      {
        // Call back edge for back, forward, cross.
        visitor.back_edge(e, src);
      }
    }
  }

  void psearchVisit(vertex v)
  {
    visitor.pre_visit(v);
    src = v;
    visit_edges(_g, v, *this, pcut, true, UNDIRECTED);
    visitor.finish_vertex(v);
  }

public:
  user_visitor visitor;
  ColorMap color;
  vertex src;
  vertex_id_map<graph>& vid_map;
  graph& _g;
};

/***/

template <typename graph, typename ColorMap, typename user_visitor, int pcut>
class internal_psearch_visitor<graph, ColorMap, user_visitor, DIRECTED,
                               NO_FILTER, pcut> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;
  typedef typename graph_traits<graph>::size_type size_type;
  typedef psearch<graph, ColorMap, user_visitor, NO_FILTER, DIRECTED, pcut>
          psearch_t;

  internal_psearch_visitor(graph& g, vertex v, ColorMap& col, user_visitor& vis,
                           vertex_id_map<graph>& vm) :
    visitor(vis), color(col), vid_map(vm), _g(g), src(v) {}

  void set_data(vertex v) { src = v; }
  void psearchRoot(vertex v) { visitor.psearch_root(v); }

  void operator()(edge e)
  {
    vertex v2 = target(e, _g);
    int clr = mt_incr(color[get(vid_map, v2)], 1);

    if ((clr == psearch_t::white) || visitor.visit_test(e, src))
    {
      visitor.tree_edge(e, src);
      internal_psearch_visitor next(*this);
      next.psearchVisit(v2);
    }
    else
    {
      // Call back edge for back, forward, cross.
      visitor.back_edge(e, src);
    }
  }

  void visit_in(edge e) { fprintf(stderr, "ERROR: Shouldn't be here.\n"); }

  void psearchVisit(vertex v)
  {
    visitor.pre_visit(v);
    src = v;
    visit_edges(_g, v, *this, pcut, true, DIRECTED);
    visitor.finish_vertex(v);
  }

public:
  user_visitor visitor;
  ColorMap color;
  vertex src;
  vertex_id_map<graph>& vid_map;
  graph& _g;
};

/***/

template <typename graph, typename ColorMap, typename user_visitor, int pcut>
class internal_psearch_visitor<graph, ColorMap, user_visitor, REVERSED,
                               NO_FILTER, pcut> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;
  typedef typename graph_traits<graph>::size_type size_type;
  typedef psearch<graph, ColorMap, user_visitor, NO_FILTER, REVERSED, pcut>
          psearch_t;

  internal_psearch_visitor(graph& g, vertex v, ColorMap& col, user_visitor& vis,
                           vertex_id_map<graph>& vm) :
    visitor(vis), color(col), vid_map(vm), _g(g), src(v) {}

  void set_data(vertex v) { src = v; }
  void psearchRoot(vertex v) { visitor.psearch_root(v); }

  void operator()(edge e)
  {
    size_type second_id = get(vid_map, target(e, _g));
    if (second_id != get(vid_map, src))  return;

    vertex v2 = source(e, _g);
    int clr = mt_incr(color[get(vid_map, v2)], 1);

    if ((clr == psearch_t::white) || visitor.visit_test(e, src))
    {
      visitor.tree_edge(e, src);
      internal_psearch_visitor next(*this);
      next.psearchVisit(v2);
    }
    else
    {
      // Call back edge for back, forward, cross.
      visitor.back_edge(e, src);
    }
  }

  void visit_in(edge e)
  {
    vertex v2 = source(e, _g);
    int clr = mt_incr(color[get(vid_map, v2)], 1);

    if (clr == psearch_t::white || visitor.visit_test(e, src))
    {
      visitor.tree_edge(e, src);
      internal_psearch_visitor next(*this);
      next.psearchVisit(v2);
    }
    else
    {
      // Call back edge for back, forward, cross.
      visitor.back_edge(e, src);
    }
  }

  void psearchVisit(vertex v)
  {
    visitor.pre_visit(v);
    src = v;
    visit_edges(_g, v, *this, pcut, true, REVERSED);
    visitor.finish_vertex(v);
  }

public:
  user_visitor visitor;
  ColorMap color;
  vertex src;
  vertex_id_map<graph>& vid_map;
  graph& _g;
};

/***/

template <typename graph, typename ColorMap, typename user_visitor, int pcut>
class internal_psearch_visitor<graph, ColorMap, user_visitor, UNDIRECTED,
                               NO_FILTER, pcut> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;
  typedef psearch<graph, ColorMap, user_visitor, NO_FILTER, UNDIRECTED, pcut>
          psearch_t;

  internal_psearch_visitor(graph& g, vertex v, ColorMap& col, user_visitor& vis,
                           vertex_id_map<graph>& vm) :
    visitor(vis), color(col), src(v), vid_map(vm), _g(g) {}

  void set_data(vertex v) { src = v; }
  void psearchRoot(vertex v) { visitor.psearch_root(v); }

  void operator()(edge e)
  {
    vertex v2 = target(e, _g);
    int clr = mt_incr(color[get(vid_map, v2)], 1);

    if ((clr == psearch_t::white) || visitor.visit_test(e, src))
    {
      visitor.tree_edge(e, src);
      internal_psearch_visitor next(*this);
      next.psearchVisit(v2);
    }
    else
    {
      // Call back edge for back, forward, cross.
      visitor.back_edge(e, src);
    }
  }

  void visit_in(edge e)
  {
    vertex v2 = source(e, _g);
    int clr = mt_incr(color[get(vid_map, v2)], 1);

    if (clr == psearch_t::white || visitor.visit_test(e, src))
    {
      visitor.tree_edge(e, src);
      internal_psearch_visitor next(*this);
      next.psearchVisit(v2);
    }
    else
    {
      // Call back edge for back, forward, cross.
      visitor.back_edge(e, src);
    }
  }

  void psearchVisit(vertex v)
  {
    visitor.pre_visit(v);
    src = v;
    visit_edges(_g, v, *this, pcut, true, UNDIRECTED);
    visitor.finish_vertex(v);
  }

public:
  user_visitor visitor;
  ColorMap color;
  vertex src;
  vertex_id_map<graph>& vid_map;
  graph& _g;
};

/***/

template <typename graph, typename ColorMap, typename psearch_visitor,
          int filter, int directed, int pcutoff>
psearch<graph, ColorMap, psearch_visitor, filter, directed, pcutoff>::psearch(
    graph* g, ColorMap c, psearch_visitor& vis) : _g(*g), visitor(vis),
                                                  color(c)
{
}

/***/

template <typename graph, typename ColorMap, typename psearch_visitor,
          int filter, int directed, int pcutoff>
psearch<graph, ColorMap, psearch_visitor, filter, directed, pcutoff>::psearch(
    graph& g, ColorMap c, psearch_visitor& vis) : visitor(vis),
                                                  color(c), _g(g)
{
}

/***/

template <typename graph, typename ColorMap, typename psearch_visitor,
          int filter, int directed, int pcutoff>
psearch<graph, ColorMap, psearch_visitor, filter, directed, pcutoff>::~psearch()
{
}

/***/

template <typename graph, typename ColorMap, typename psearch_visitor,
          int filter, int directed, int pcutoff>
void psearch<graph, ColorMap, psearch_visitor, filter, directed, pcutoff>::run(
    vertex v)
{
  vertex_id_map<graph> vid_map = get(_vertex_id_map, _g);
  internal_psearch_visitor<graph, ColorMap, psearch_visitor, directed, filter,
                           pcutoff> helper(_g, v, color, visitor, vid_map);

  // Color might be a null pointer in the case of a PURE_FILTER.
  if (color != NULL)
  {
    unsigned long clr = mt_incr(color[get(vid_map, v)], 1);

    if (clr == psearch::white || visitor.start_test(v))
    {
      helper.psearchRoot(v);
      helper.psearchVisit(v);
    }
  }
  else
  {
    if (visitor.start_test(v))
    {
      helper.psearchRoot(v);
      helper.psearchVisit(v);
    }
  }
}

/***/

/*! \brief An algorithn to run concurrent parallel searches in the following
           way.  First, partition the vertices of the graph into high-degree
           and low-degree vertices.  Then run psearches from the high-degree
           vertices in serial.  When finished, run concurrent psearches from
           the low-degree vertices.

     This separation is critical to the performance of algorithms like Kahan's
     connected components algorithm and the bully algorithm.  Kahan's original
     implementation of his algorithm actually did this separation by degree
     recursively at each vertex visitation.  We stop with the simpler version
     here.

     The version of run() without arguments searches through the entire graph.
     The version of run() with arguments only searches through the parts of
     the graph reachable from the roots supplied.
*/
template <typename graph, typename visitor, int filter = NO_FILTER,
          int directed = UNDIRECTED, int parallelization_cutoff = 100>
class psearch_high_low {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::vertex_iterator vertex_iterator;

  psearch_high_low(graph& gg, visitor vs, int degThreshold = 0) :
      g(gg), vis(vs), degreeThreshold(degThreshold) {}

  void run()
  {
    size_type ord = num_vertices(g);
    vertex_iterator viter = vertices(g);

    if (degreeThreshold == 0)
    {
      printf("high_low: finding cutoff\n");
      size_type* degs = new size_type[ord];
      #pragma mta assert nodep
      for (size_type i = 0; i < ord; i++)
      {
        degs[i] = out_degree(viter[i], g);
      }

      size_type maxval = 0;
      #pragma mta assert nodep
      for (size_type i = 0; i < ord; i++)
      {
        if (degs[i] > maxval)  maxval = degs[i];
      }

      countingSort(degs, ord, maxval);
      //degreeThreshold = degs[ord-25];
      printf("threshold: %d\n", degreeThreshold);

      delete [] degs;
    }

    long* indicesOfBigs = (long*) malloc(sizeof(long)*ord);
    long* indicesOfSmalls = (long*) malloc(sizeof(long)*ord);
    size_type* searchColor = (size_type*) malloc(sizeof(size_type)*ord);

    size_type nextBig = 0;
    size_type nextSmall = 0;

    vertex_id_map<graph> vid_map = get(_vertex_id_map, g);

    #pragma mta assert parallel
    for (size_type i = 0; i < ord; i++)
    {
      vertex v = viter[i];
      int id = vid_map[v];
      searchColor[id] = 0;

      if (out_degree(v, g) > degreeThreshold)
      {
        size_type mine = mt_incr(nextBig, 1);
        indicesOfBigs[mine] = id;
      }
      else
      {
        size_type mine = mt_incr(nextSmall, 1);
        indicesOfSmalls[mine] = id;
      }
    }

//    printf("numbig: %lu\n", (long unsigned) nextBig);
//    printf("numsmall: %lu\n", (long unsigned) nextSmall);

    psearch<graph, size_type*, visitor, filter, directed,
            parallelization_cutoff> psearch(g, searchColor, vis);

    // In serial.
    for (size_type i = 0; i < nextBig; i++)
    {
      psearch.run(viter[indicesOfBigs[i]]);
    }

    #pragma mta assert parallel
    for (size_type i = 0; i < nextSmall; i++)
    {
      psearch.run(viter[indicesOfSmalls[i]]);
    }

    free(indicesOfBigs);
    free(indicesOfSmalls);
    free(searchColor);
  }

  template <typename T>
  void run(T* roots, int num_roots)
  {
    size_type ord = num_vertices(g);
    vertex_iterator viter = vertices(g);

    if (degreeThreshold == 0 && num_roots > 25)
    {
      printf("high_low: finding cutoff\n");
      size_type* degs = new size_type[num_roots];

      #pragma mta assert nodep
      for (int i = 0; i < num_roots; i++)
      {
        degs[i] = out_degree(viter[roots[i]], g);
      }

      size_type maxval = 0;
      #pragma mta assert nodep
      for (int i = 0; i < num_roots; i++)
      {
        if (degs[i] > maxval)  maxval = degs[i];
      }

      countingSort(degs, num_roots, maxval);
      degreeThreshold = degs[num_roots-25];
      printf("threshold: %d\n", degreeThreshold);

      delete [] degs;
    }

    long* indicesOfBigs = (long*) malloc(sizeof(long) * num_roots);
    long* indicesOfSmalls = (long*) malloc(sizeof(long) * num_roots);
    size_type* searchColor = (size_type*) malloc(sizeof(size_type) * ord);

    size_type nextBig   = 0;
    size_type nextSmall = 0;

    vertex_id_map<graph> vid_map = get(_vertex_id_map, g);

    #pragma mta assert nodep
    for (size_type i = 0; i < ord; i++)  searchColor[i] = 0;

    #pragma mta assert parallel
    for (int r = 0; r < num_roots; r++)
    {
      T i = roots[r];
      vertex v = viter[i];

      int id = vid_map[v];
      searchColor[id] = 0;

      if (out_degree(v, g) > degreeThreshold)
      {
        size_type mine = mt_incr(nextBig, 1);
        indicesOfBigs[mine] = id;
      }
      else
      {
        size_type mine = mt_incr(nextSmall, 1);
        indicesOfSmalls[mine] = id;
      }
    }

    psearch<graph, size_type*, visitor, filter, directed,
            parallelization_cutoff> psearch(g, searchColor, vis);

    // In serial.
    for (size_type i = 0; i < nextBig; i++)
    {
      printf("searching from %ld\n", indicesOfBigs[i]);
      psearch.run(viter[indicesOfBigs[i]]);
    }

    #pragma mta assert parallel
    for (size_type i = 0; i < nextSmall; i++)
    {
      psearch.run(viter[indicesOfSmalls[i]]);
    }

    free(indicesOfBigs);
    free(indicesOfSmalls);
    free(searchColor);
  }

private:
  graph& g;
  visitor vis;
  int degreeThreshold;

  /*! \fn psearch_high_low(graph& gg, visitor vs, int degThreshold = 0)
      \brief This constructor takes a graph and a visitor routine.  Note that
             a template parameter of the class allows the MTGL programmer to
             control the threshold for parallel loops through edge lists.
      \param gv The graph to be searched.
      \param vs A visitor object to tailor the searches.
      \param degThreshold A threshold to determine "high-degree vertices".
  */
  /*! \fn void run()
      \brief Invokes the algorithm.  Searches through the entire graph.

      In augmented versions, this will take a subproblem mask and the current
      subproblem.
  */
  /*! \fn template <typename T> void run(T* roots, int num_roots)
      \brief Invokes the algorithm.  Only searches through the parts of the
             graph reachable from the roots supplied.
      \param roots The ids of the vertices from which to start the search.
      \param num_roots Size of the roots array.

      In augmented versions, this will take a subproblem mask and the current
      subproblem.
  */
  /*! \var graph& g
      \brief The graph to be searched.
  */
  /*! \var visitor& vis
      \brief The application programmer instantiates each psearch_high_low
             object with an instance of a "visitor".  This will be an instance
             of default_psearch_visitor or a user-defined child class of
             default_psearch_visitor.
  */
  /*! \var int degreeThreshold
      \brief A threshold to determine "high-degree vertices".
  */
};

/***/

/*! \brief A generic implementation of the brat / bully search paradigm.  This
           is not yet used by the MTGL implementaion of the bully algorithm
           for connected components.  Rather, its use is being piloted in the
           "depth_first_search.hpp" code.  Note the subproblem and
           my_subproblem data members.  This mechanism allows bully-style
           searches through portions of the graph corresponding to
           subproblems, perhaps of recursive, divide and conquer algorithms.

    This visitor allows the MTGL programmer to retain the search forest,
    including leaders of each search, and search parents.

    The bratSearch algorithm below invokes a psearch using the "PURE_FILTER",
    which indicates that the visitor's visit_test method completely determines
    whether or not a search is to continue (as opposed to leaving the decision
    entirely or in part with the psearch object itself).  The reason for doing
    this is that the subproblem / my_subproblem filtering is an "AND"
    operation, but the brat search continuation is an "OR" operation.  These
    are combined under our control in the visit_test method of this
    brat_psearch_visitor class.
*/
template <typename graph, class T>
class brat_psearch_visitor : public default_psearch_visitor<graph> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  brat_psearch_visitor(vertex_id_map<graph>& vm, int* search_c, T* attr,
                       int* bldr, int* bp, graph& g, int* subp = 0,
                       int my_subp = 0) :
      search_color(search_c), attribute(attr), b_leader(bldr), b_parent(bp),
      subproblem(subp), my_subproblem(my_subp), vid_map(vm), _g(g) {}

  void psearch_root(vertex& v)
  {
    int id = vid_map[v];
    b_leader[id] = id;
    b_parent[id] = -1;
  }

  void pre_visit(vertex& v) {}

  bool visit_test(edge& e, vertex& src) const
  {
    vertex v2 = other(e, src, _g);
    int sid = vid_map[src];
    int tid = vid_map[v2];
    if (!subproblem || subproblem[tid] != my_subproblem)  return false;

    int clr = mt_incr(search_color[tid], 1);
    if (clr == 0 || (attribute[sid] < attribute[tid]))  return true;

    return false;
  }

  void tree_edge(edge& e, vertex& src)
  {
    vertex dest = other(e, src, _g);
    int sid = vid_map[src];
    int tid = vid_map[dest];
    int c = mt_readfe(attribute[tid]);

    if (c > attribute[sid])
    {
      mt_write(attribute[tid], attribute[sid]);
      b_leader[tid] = b_leader[sid];
      b_parent[tid] = sid;
    }
    else
    {
      mt_write(attribute[tid], c);
    }
  }

private:
  T* attribute;
  int* search_color;
  int* b_leader;
  int* b_parent;
  int* subproblem;
  int  my_subproblem;
  vertex_id_map<graph>& vid_map;
  graph& _g;

  /*! \fn brat_psearch_visitor(int* search_c, T* attr, int* bldr, int* bp,
                               int* subp = 0, int my_subp = 0)
      \brief This constructor initialized the visitor object.
      \param search_c The search color, which we manipulate as part of our
                      responisibility to be a "PURE_FILTER".
      \param attr The attribute used to determine domination (bullying).
      \param bldr For each vertex, store its "branching leader", i.e., the
                  search leader of the search that discovered it.
      \param bp For each vertex, store its "branching parent", i.e., the
                vertex that discovered it.
      \param subp An array of subproblem numbers, one per vertex.
      \param my_subp The active subproblem.   Search only in the subgraph
                     induced by this subproblem.
  */
  /*! \fn void psearch_root(vertex* v)
      \brief Note that v is a search leader.
  */
  /*! \fn void pre_visit(vertex* v)
      \brief Do nothing.
  */
  /*!
      \fn bool visit_test(edge* e, vertex* src) const
      \brief Determine whether or not to continue the search: if the
             destination is undiscovered or if the src dominates the
             destination in terms of the domination attribute.
  */
  /*!
      \fn bool tree_edge(edge* e, vertex* src) const
      \brief If discovering a new vertex or dominating a previously discovered
             vertex, overwrite the domination attribute of the destination.
  */
};

/*! \brief A generic implementation of the brat / bully search paradigm.  This
           is not yet used by the MTGL implementaion of the bully algorithm
           for connected components.  Rather, its use is being piloted in the
           "depth_first_search.hpp" code.  Note the subproblem and
           my_subproblem data members.  This mechanism allows bully-style
           searches through portions of the graph corresponding to
           subproblems, perhaps of recursive, divide and conquer algorithms.

    This visitor allows the MTGL programmer to retain the search forest,
    including leaders of each search, and search parents.

    The brat_search algorithm invokes a psearch using the "PURE_FILTER", which
    indicates that the visitor's visit_test method completely determines
    whether or not a search is to continue (as opposed to leaving the decision
    entirely or in part with the psearch object itself).  The reason for doing
    this is that the subproblem/my_subproblem filtering is an "AND" operation,
    but the brat search continuation is an "OR" operation.  These are combined
    under our control in the visit_test method of this brat_psearch_visitor
    class.
*/
template <typename graph, class T, int directed, class uinttype>
class brat_search {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;
  typedef typename graph_traits<graph>::vertex_iterator vertex_iterator;
  typedef typename graph_traits<graph>::size_type size_type;

  brat_search(graph* gg, T* att, int* b_ldr, int* b_prnt, int* subp = 0,
              int my_subp = 0, int degreeThresh = 10000) :
      g(gg), attr(att), b_leader(b_ldr), b_parent(b_prnt), subproblem(subp),
      my_subproblem(my_subp), degreeThreshold(degreeThresh) {}

  void run()
  {
    typedef brat_psearch_visitor<graph, T> visitor;
    size_type ord = num_vertices(*g);

    long* indicesOfBigs = (long*) malloc(sizeof(long)*ord);
    long* indicesOfSmalls = (long*) malloc(sizeof(long)*ord);
    uinttype* searchColor = (uinttype*) malloc(sizeof(uinttype)*ord);

    uinttype nextBig = 0;
    uinttype nextSmall = 0;

    vertex_id_map<graph> vid_map = get(_vertex_id_map, *g);
    vertex_iterator viter = vertices(*g);

    #pragma mta assert parallel
    for (size_type j = 0; j < ord; ++j)
    {
      vertex v = viter[j];
      int i = vid_map[v];
      if (subproblem[i] == my_subproblem)
      {
        searchColor[i] = 0;
      }
      else
      {
        searchColor[i] = INT_MAX;  // never visit
      }

      if (out_degree(v, *g) > degreeThreshold)
      {
        uinttype mine = mt_incr(nextBig, 1);
        indicesOfBigs[mine] = i;
      }
      else
      {
        uinttype mine = mt_incr(nextSmall, 1);
        indicesOfSmalls[mine] = i;
      }
    }

    visitor vis(vid_map, searchColor, attr, b_leader, b_parent, *g, subproblem,
                my_subproblem);

    psearch<graph, uinttype*, visitor, PURE_FILTER, directed>
        psearch(*g, searchColor, vis);

    // In serial.
    for (uinttype i = 0; i < nextBig; i++)
    {
      psearch.run(viter[indicesOfBigs[i]]);
    }

    #pragma mta assert parallel
    #pragma mta loop future
    for (uinttype i = 0; i < nextSmall; i++)
    {
      psearch.run(viter[indicesOfSmalls[i]]);
    }

    free(indicesOfBigs);
    free(indicesOfSmalls);
    free(searchColor);
  }

private:
  graph* g;
  T* attr;
  int* b_leader;
  int* b_parent;
  int* subproblem;
  int my_subproblem;
  int degreeThreshold;

  /*! \fn brat_search(graph* gg, T* att, int* b_ldr, int* b_prnt,
                      int* subp = 0, int my_subp = 0,
                      int degreeThresh = 10000)
      \brief This constructor initialized the algorithm object.
      \param gg A pointer to a graph.
      \param att The domination attribute.
      \param b_ldr For each vertex, store its "branching leader", i.e., the
                   search leader of the search that discovered it.
      \param b_prnt For each vertex, store its "branching parent", i.e., the
                    vertex that discovered it.
      \param subp An array of subproblem numbers, one per vertex.
      \param my_subp The active subproblem.   Search only in the subgraph
                     induced by this subproblem.
      \param degreeThresh A threshold to distingush "high" and "low" degree
                          vertices.
  */
  /*! \fn void run()
      \brief Invoke the algorithm
  */
};

}

#endif

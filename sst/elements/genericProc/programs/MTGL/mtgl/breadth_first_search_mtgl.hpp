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
/*! \file breadth_first_search_mtgl.hpp

    \brief Modification of breadth_first_search.hpp to complete the search
           and export the same mtgl interface as psearch.

    \deprecated See bfs.hpp.

    \author Brad Mancke
    \author William McLendon (wcmclen@sandia.gov)

    \date 2007
*/
/****************************************************************************/

#ifndef MTGL_BREADTH_FIRST_SEARCH_MTGL_HPP
#define MTGL_BREADTH_FIRST_SEARCH_MTGL_HPP

#include <climits>

#include <mtgl/mtgl_adapter.hpp>
#include <mtgl/graph_traits.hpp>
#include <mtgl/queue.hpp>
#include <mtgl/util.hpp>

namespace mtgl {

template <typename graph>
struct default_bfs_mtgl_visitor {
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  void bfs_root(vertex& v) const {}
  void pre_visit(vertex& v) {}
  int start_test(vertex& v) { return false; }
  int visit_test(edge& e, vertex& v) { return false; }
  void tree_edge(edge& e, vertex& src) const {}
  void back_edge(edge& e, vertex& src) const {}
  void finish_vertex(vertex& v) const {}
};

template <typename graph, typename ColorMap, typename user_visitor,
          int directed, int logic, int pcutoff>
class internal_bfs_mtgl_visitor;

template<typename graph, typename ColorMap, typename bfs_mtgl_visitor,
         int directed = UNDIRECTED, int filter = NO_FILTER, int pcutoff = 1>
class breadth_first_search_mtgl {
public:
  typedef enum { white, gray, black } SearchColor;
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;
  typedef typename graph_traits<graph>::vertex_iterator vertex_iterator;

  breadth_first_search_mtgl(graph& g, ColorMap c, bfs_mtgl_visitor vis) :
    _g(g), visitor(vis), color(c), count(0), Q(queue<int>(num_vertices(_g))) {}

  // Constructor adding a parameter to specify the max size of the queue.
  // This is used when the queue needs to be smaller or larger.  The default
  // queue size is the number of vertices in g.  When a BFS is run on a
  // sub-problem (i.e., in a recursive routine) and we know the max size
  // the queue might grow to, a smaller queue could give a significant
  // performance increase over the default size.  When running a BFS in which
  // the queue could grow larger than the number of vertices in g, a larger
  // queue size must be used.  There is no out of bounds checking.  If the
  // queue grows larger than it is initialized to, memory stomping happens.
  breadth_first_search_mtgl(graph& g, ColorMap c, bfs_mtgl_visitor vis,
                            int max_queue_size) :
    _g(g), visitor(vis), color(c), count(0), Q(queue<int>(max_queue_size)) {}

  breadth_first_search_mtgl(const breadth_first_search_mtgl& b) :
    _g(b._g), visitor(b.visitor), color(b.color), count(b.count), Q(b.Q) {}

  ~breadth_first_search_mtgl() {}

  void run(vertex v, bool recursive = false)
  {
    vertex_id_map<graph> vid_map = get(_vertex_id_map, _g);

    count = 0;

    typedef internal_bfs_mtgl_visitor<graph, ColorMap, bfs_mtgl_visitor,
                                      directed, filter, pcutoff>
            internal_bfs_mtgl_t;

    internal_bfs_mtgl_t helper(_g, color, Q, count, visitor, vid_map);

    vertex_iterator verts = vertices(_g);

    bool start_search = false;

    if (color != NULL)
    {
      int clr = mt_incr(color[get(vid_map, v)], 1);
      if (clr == breadth_first_search_mtgl::white || visitor.start_test(v))
      {
        start_search = true;
      }
    }
    else
    {
      if (visitor.start_test(v)) start_search = true;
    }

    if (start_search)
    {
      helper.bfs_mtgl_root(v);
      helper.bfs_mtgl_visit(v);

      if (recursive)
      {
        int q_start_indx = 0;
        int q_end_indx = helper.count;

        // Parallelize depending on how many to process.
        while (q_end_indx > q_start_indx)
        {
          //if (q_end_indx - q_start_indx >= pcutoff) {
          #pragma mta assert parallel
          //#pragma mta dynamic schedule
          #pragma mta interleave schedule
          for (int i = q_start_indx; i < q_end_indx; i++)
          {
            internal_bfs_mtgl_t helper2(helper);
            helper2.bfs_mtgl_visit(verts[helper2.Q.elem(i)]);
          }
          //} else {
          //for (int i = q_start_indx; i < q_end_indx; i++) {
          //  helper.bfs_mtgl_visit(verts[helper.Q.elem(i)]);
          //}
          //}

          q_start_indx = q_end_indx;
          q_end_indx = helper.count;
        }
      }
    }
  }

  graph& _g;
  bfs_mtgl_visitor& visitor;
  ColorMap color;
  int count;
  queue<int> Q;
};

template <typename graph, typename ColorMap, typename user_visitor,
          int directed = UNDIRECTED, int logic = NO_FILTER, int pcutoff = 1>
class internal_bfs_mtgl_visitor {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;
  typedef breadth_first_search_mtgl<graph, ColorMap, user_visitor, directed,
                                    logic, pcutoff>
          bfs_mtgl_t;

  internal_bfs_mtgl_visitor(graph& g, ColorMap& col, queue<int>& _Q,
                            int& _count, user_visitor& vis,
                            vertex_id_map<graph>& vm) :
    visitor(vis), color(col), Q(_Q), count(_count), vid_map(vm), _g(g) {}

  internal_bfs_mtgl_visitor(const internal_bfs_mtgl_visitor& i) :
    visitor(i.visitor), color(i.color), Q(i.Q), count(i.count),
    vid_map(i.vid_map), _g(i._g), src(i.src) {}

  void set_data(vertex v) { src = v; }
  void bfs_mtgl_root(vertex v) { visitor.bfs_root(v); }

  void operator()(edge e)
  {
    vertex v2 = other(e, src, _g);
    int oid = get(vid_map, v2);
    int clr = mt_incr(color[oid], 1);

    if (visitor.visit_test(e, src) || clr == bfs_mtgl_t::white)
    {
      visitor.tree_edge(e, src);
      Q.push(oid, mt_incr(count, 1));
    }
    else
    {
      visitor.back_edge(e, src);
    }
  }

  void bfs_mtgl_visit(vertex v)
  {
    visitor.pre_visit(v);
    src = v;
    visit_edges(_g, v, *this, pcutoff, false, directed);
    visitor.finish_vertex(v);
  }

  user_visitor visitor;
  ColorMap color;
  vertex src;
  vertex_id_map<graph>& vid_map;
  graph& _g;
  queue<int>& Q;
  int& count;
};

template <typename graph, typename ColorMap, typename user_visitor, int pcutoff>
class internal_bfs_mtgl_visitor<graph, ColorMap, user_visitor, UNDIRECTED,
                                NO_FILTER, pcutoff> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;
  typedef breadth_first_search_mtgl<graph, ColorMap, user_visitor, UNDIRECTED,
                                    NO_FILTER, pcutoff>
          bfs_mtgl_t;

  internal_bfs_mtgl_visitor(graph& g, ColorMap& col, queue<int>& _Q,
                            int& _count, user_visitor& vis,
                            vertex_id_map<graph>& vm) :
    visitor(vis), color(col), Q(_Q), count(_count), vid_map(vm), _g(g) {}

  internal_bfs_mtgl_visitor(const internal_bfs_mtgl_visitor& i) :
    visitor(i.visitor), color(i.color), Q(i.Q), count(i.count),
    vid_map(i.vid_map), _g(i._g), src(i.src) {}

  void set_data(vertex v) { src = v; }
  void bfs_mtgl_root(vertex v) { visitor.bfs_root(v); }

  void operator()(edge e)
  {
    vertex v2 = target(e, _g);
    int oid = get(vid_map, v2);
    int clr = mt_incr(color[oid], 1);

    if (visitor.visit_test(e, src) || clr == bfs_mtgl_t::white)
    {
      visitor.tree_edge(e, src);
      Q.push(oid, mt_incr(count, 1));
    }
    else
    {
      visitor.back_edge(e, src);
    }
  }

  void visit_in(edge e)
  {
    vertex first = source(e, _g);
    vertex v2 = target(e, _g);
    int oid = get(vid_map, first);
    int clr = mt_incr(color[oid], 1);

    if (visitor.visit_test(e, v2) || clr == bfs_mtgl_t::white)
    {
      visitor.tree_edge(e, v2);
      Q.push(oid, mt_incr(count, 1));
    }
    else
    {
      visitor.back_edge(e, v2);
    }
  }

  void bfs_mtgl_visit(vertex v)
  {
    visitor.pre_visit(v);
    src = v;
    visit_edges(_g, v, *this, pcutoff, true, UNDIRECTED);
    visitor.finish_vertex(v);
  }

  user_visitor visitor;
  ColorMap color;
  vertex src;
  vertex_id_map<graph>& vid_map;
  graph& _g;
  queue<int>& Q;
  int& count;
};

template <typename graph, typename ColorMap, typename user_visitor, int pcutoff>
class internal_bfs_mtgl_visitor<graph, ColorMap, user_visitor, UNDIRECTED,
                                AND_FILTER, pcutoff> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;
  typedef breadth_first_search_mtgl<graph, ColorMap, user_visitor, UNDIRECTED,
                                    AND_FILTER, pcutoff>
          bfs_mtgl_t;

  internal_bfs_mtgl_visitor(graph& g, ColorMap& col, queue<int>& _Q,
                            int& _count, user_visitor& vis,
                            vertex_id_map<graph>& vm) :
    visitor(vis), color(col), Q(_Q), count(_count), vid_map(vm), _g(g) {}

  internal_bfs_mtgl_visitor(const internal_bfs_mtgl_visitor& i) :
    visitor(i.visitor), color(i.color), Q(i.Q), count(i.count),
    vid_map(i.vid_map), _g(i._g), src(i.src) {}

  void set_data(vertex v) { src = v; }
  void bfs_mtgl_root(vertex v) { visitor.bfs_root(v); }

  void operator()(edge e)
  {
    if (visitor.visit_test(e, src))
    {
      vertex v2 = target(e, _g);
      int oid = get(vid_map, v2);
      int clr = mt_incr(color[oid], 1);

      if (clr == bfs_mtgl_t::white)
      {
        visitor.tree_edge(e, src);
        Q.push(oid, mt_incr(count, 1));
      }
      else
      {
        visitor.back_edge(e, src);
      }
    }
  }

  void visit_in(edge e)
  {
    vertex v2 = target(e, _g);

    if (visitor.visit_test(e, v2))
    {
      vertex first = source(e, _g);
      int oid = get(vid_map, first);
      int clr = mt_incr(color[oid], 1);

      if (clr == bfs_mtgl_t::white)
      {
        visitor.tree_edge(e, v2);
        Q.push(oid, mt_incr(count, 1));
      }
      else
      {
        visitor.back_edge(e, v2);
      }
    }
  }

  void bfs_mtgl_visit(vertex v)
  {
    visitor.pre_visit(v);
    src = v;
    visit_edges(_g, v, *this, pcutoff, true, UNDIRECTED);
    visitor.finish_vertex(v);
  }

  user_visitor visitor;
  ColorMap color;
  vertex src;
  vertex_id_map<graph>& vid_map;
  graph& _g;
  queue<int>& Q;
  int& count;
};

template <typename graph, typename ColorMap, typename user_visitor, int pcutoff>
class internal_bfs_mtgl_visitor<graph, ColorMap, user_visitor, UNDIRECTED,
                                PURE_FILTER, pcutoff> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;
  typedef breadth_first_search_mtgl<graph, ColorMap, user_visitor, UNDIRECTED,
                                    PURE_FILTER, pcutoff>
          bfs_mtgl_t;

  internal_bfs_mtgl_visitor(graph& g, ColorMap& col, queue<int>& _Q,
                            int& _count, user_visitor& vis,
                            vertex_id_map<graph>& vm) :
    visitor(vis), color(col), Q(_Q), count(_count), vid_map(vm), _g(g) {}

  internal_bfs_mtgl_visitor(const internal_bfs_mtgl_visitor& i) :
    visitor(i.visitor), color(i.color), Q(i.Q), count(i.count),
    vid_map(i.vid_map), _g(i._g), src(i.src) {}

  void set_data(vertex v) { src = v; }
  void bfs_mtgl_root(vertex v) { visitor.bfs_root(v); }

  void operator()(edge e)
  {
    if (visitor.visit_test(e, src))
    {
      vertex v2 = target(e, _g);
      int oid = get(vid_map, v2);
      visitor.tree_edge(e, src);
      Q.push(oid, mt_incr(count, 1));
    }
    else
    {
      visitor.back_edge(e, src);
    }
  }

  void visit_in(edge e)
  {
    vertex first = source(e, _g);
    vertex v2 = target(e, _g);
    int oid = get(vid_map, first);

    if (visitor.visit_test(e, v2))
    {
      visitor.tree_edge(e, v2);
      Q.push(oid, mt_incr(count, 1));
    }
    else
    {
      visitor.back_edge(e, v2);
    }
  }

  void bfs_mtgl_visit(vertex v)
  {
    visitor.pre_visit(v);
    src = v;
    visit_edges(_g, v, *this, pcutoff, true, UNDIRECTED);
    visitor.finish_vertex(v);
  }

  user_visitor visitor;
  ColorMap color;
  vertex src;
  vertex_id_map<graph>& vid_map;
  graph& _g;
  queue<int>& Q;
  int& count;
};

template <typename graph, typename ColorMap, typename user_visitor, int pcutoff>
class internal_bfs_mtgl_visitor<graph, ColorMap, user_visitor, DIRECTED,
                                NO_FILTER, pcutoff> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;
  typedef breadth_first_search_mtgl<graph, ColorMap, user_visitor, DIRECTED,
                                    NO_FILTER, pcutoff>
          bfs_mtgl_t;

  internal_bfs_mtgl_visitor(graph& g, ColorMap& col, queue<int>& _Q,
                            int& _count, user_visitor& vis,
                            vertex_id_map<graph>& vm) :
    visitor(vis), color(col), Q(_Q), count(_count), vid_map(vm), _g(g) {}

  internal_bfs_mtgl_visitor(const internal_bfs_mtgl_visitor& i) :
    visitor(i.visitor), color(i.color), Q(i.Q), count(i.count),
    vid_map(i.vid_map), _g(i._g), src(i.src) {}

  void set_data(vertex v) { src = v; }
  void bfs_mtgl_root(vertex v) { visitor.bfs_root(v); }

  void operator()(edge e)
  {
    vertex v2 = target(e, _g);
    int oid = get(vid_map, v2);
    int clr = mt_incr(color[oid], 1);

    if (visitor.visit_test(e, src) || clr == bfs_mtgl_t::white)
    {
      visitor.tree_edge(e, src);
      Q.push(oid, mt_incr(count, 1));
    }
    else
    {
      visitor.back_edge(e, src);
    }
  }

  void visit_in(edge e) { fprintf(stderr, "ERROR: Shouldn't be here.\n"); }

  void bfs_mtgl_visit(vertex v)
  {
    visitor.pre_visit(v);
    src = v;
    visit_edges(_g, v, *this, pcutoff, true, DIRECTED);
    visitor.finish_vertex(v);
  }

  user_visitor visitor;
  ColorMap color;
  queue<int>& Q;
  int& count;
  vertex_id_map<graph>& vid_map;
  graph& _g;
  vertex src;
};

template <typename graph, typename ColorMap, typename user_visitor, int pcutoff>
class internal_bfs_mtgl_visitor<graph, ColorMap, user_visitor, DIRECTED,
                                AND_FILTER, pcutoff> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;
  typedef breadth_first_search_mtgl<graph, ColorMap, user_visitor, DIRECTED,
                                    AND_FILTER, pcutoff>
          bfs_mtgl_t;

  internal_bfs_mtgl_visitor(graph& g, ColorMap& col, queue<int>& _Q,
                            int& _count, user_visitor& vis,
                            vertex_id_map<graph>& vm) :
    visitor(vis), color(col), Q(_Q), count(_count), vid_map(vm), _g(g) {}

  internal_bfs_mtgl_visitor(const internal_bfs_mtgl_visitor& i) :
    visitor(i.visitor), color(i.color), Q(i.Q), count(i.count),
    vid_map(i.vid_map), _g(i._g), src(i.src) {}

  void set_data(vertex v) { src = v; }
  void bfs_mtgl_root(vertex v) { visitor.bfs_root(v); }

  void operator()(edge e)
  {
    if (visitor.visit_test(e, src))
    {
      vertex v2 = target(e, _g);
      int oid = get(vid_map, v2);
      int clr = mt_incr(color[oid], 1);

      if (clr == bfs_mtgl_t::white)
      {
        visitor.tree_edge(e, src);
        Q.push(oid, mt_incr(count, 1));
      }
      else
      {
        visitor.back_edge(e, src);
      }
    }
  }

  void visit_in(edge e) { fprintf(stderr, "ERROR: Shouldn't be here.\n"); }

  void bfs_mtgl_visit(vertex v)
  {
    visitor.pre_visit(v);
    src = v;
    visit_edges(_g, v, *this, pcutoff, true, DIRECTED);
    visitor.finish_vertex(v);
  }

  user_visitor visitor;
  ColorMap color;
  queue<int>& Q;
  int& count;
  vertex_id_map<graph>& vid_map;
  graph& _g;
  vertex src;
};

template <typename graph, typename ColorMap, typename user_visitor, int pcutoff>
class internal_bfs_mtgl_visitor<graph, ColorMap, user_visitor, DIRECTED,
                                PURE_FILTER, pcutoff> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;
  typedef breadth_first_search_mtgl<graph, ColorMap, user_visitor, DIRECTED,
                                    PURE_FILTER, pcutoff>
          bfs_mtgl_t;

  internal_bfs_mtgl_visitor(graph& g, ColorMap& col, queue<int>& _Q,
                            int& _count, user_visitor& vis,
                            vertex_id_map<graph>& vm) :
    visitor(vis), color(col), Q(_Q), count(_count), vid_map(vm), _g(g) {}

  internal_bfs_mtgl_visitor(const internal_bfs_mtgl_visitor& i) :
    visitor(i.visitor), color(i.color), Q(i.Q), count(i.count),
    vid_map(i.vid_map), _g(i._g), src(i.src) {}

  void set_data(vertex v) { src = v; }
  void bfs_mtgl_root(vertex v) { visitor.bfs_root(v); }

  void operator()(edge e)
  {
    if (visitor.visit_test(e, src))
    {
      vertex v2 = target(e, _g);
      int oid = get(vid_map, v2);
      visitor.tree_edge(e, src);
      Q.push(oid, mt_incr(count, 1));
    }
    else
    {
      visitor.back_edge(e, src);
    }
  }

  void visit_in(edge e) { fprintf(stderr, "ERROR: Shouldn't be here.\n"); }

  void bfs_mtgl_visit(vertex v)
  {
    visitor.pre_visit(v);
    src = v;
    visit_edges(_g, v, *this, pcutoff, true, DIRECTED);
    visitor.finish_vertex(v);
  }

  user_visitor visitor;
  ColorMap color;
  queue<int>& Q;
  vertex src;
  int& count;
  vertex_id_map<graph>& vid_map;
  graph& _g;
};

template <typename graph, typename ColorMap, typename user_visitor, int pcutoff>
class internal_bfs_mtgl_visitor<graph, ColorMap, user_visitor, REVERSED,
                                NO_FILTER, pcutoff> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;
  typedef breadth_first_search_mtgl<graph, ColorMap, user_visitor, REVERSED,
                                    NO_FILTER, pcutoff>
          bfs_mtgl_t;

  internal_bfs_mtgl_visitor(graph& g, ColorMap& col, queue<int>& _Q,
                            int& _count, user_visitor& vis,
                            vertex_id_map<graph>& vm) :
    visitor(vis), color(col), Q(_Q), count(_count), vid_map(vm), _g(g) {}

  internal_bfs_mtgl_visitor(const internal_bfs_mtgl_visitor& i) :
    visitor(i.visitor), color(i.color), Q(i.Q), count(i.count),
    vid_map(i.vid_map), _g(i._g), src(i.src) {}

  void set_data(vertex v) { src = v; }
  void bfs_mtgl_root(vertex v) { visitor.bfs_root(v); }

  void operator()(edge e)
  {
    vertex second = target(e, _g);
    if (second != src)  return;

    vertex v2 = source(e, _g);
    int oid = get(vid_map, v2);
    int clr = mt_incr(color[oid], 1);

    if (visitor.visit_test(e, src) || clr == bfs_mtgl_t::white)
    {
      visitor.tree_edge(e, src);
      Q.push(oid, mt_incr(count, 1));
    }
    else
    {
      visitor.back_edge(e, src);
    }
  }

  void visit_in(edge e)
  {
    vertex first = source(e, _g);
    vertex v2 = target(e, _g);
    int oid = get(vid_map, first);
    int clr = mt_incr(color[oid], 1);

    if (visitor.visit_test(e, v2) || clr == bfs_mtgl_t::white)
    {
      visitor.tree_edge(e, v2);
      Q.push(oid, mt_incr(count, 1));
    }
    else
    {
      visitor.back_edge(e, v2);
    }
  }

  void bfs_mtgl_visit(vertex v)
  {
    visitor.pre_visit(v);
    src = v;
    visit_edges(_g, v, *this, pcutoff, true, REVERSED);
    visitor.finish_vertex(v);
  }

  user_visitor visitor;
  ColorMap color;
  queue<int>& Q;
  int& count;
  vertex src;
  vertex_id_map<graph>& vid_map;
  graph& _g;
};

template <typename graph, typename ColorMap, typename user_visitor, int pcutoff>
class internal_bfs_mtgl_visitor<graph, ColorMap, user_visitor, REVERSED,
                                AND_FILTER, pcutoff> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;
  typedef breadth_first_search_mtgl<graph, ColorMap, user_visitor, REVERSED,
                                    AND_FILTER, pcutoff>
          bfs_mtgl_t;

  internal_bfs_mtgl_visitor(graph& g, ColorMap& col, queue<int>& _Q,
                            int& _count, user_visitor& vis,
                            vertex_id_map<graph>& vm) :
    visitor(vis), color(col), Q(_Q), count(_count), vid_map(vm), _g(g) {}

  internal_bfs_mtgl_visitor(const internal_bfs_mtgl_visitor& i) :
    visitor(i.visitor), color(i.color), Q(i.Q), count(i.count),
    vid_map(i.vid_map), _g(i._g), src(i.src) {}

  void set_data(vertex v) { src = v; }
  void bfs_mtgl_root(vertex v) { visitor.bfs_root(v); }

  void operator()(edge e)
  {
    vertex second = target(e, _g);
    if (second != src)  return;

    if (visitor.visit_test(e, src))
    {
      vertex v2 = source(e, _g);
      int oid = get(vid_map, v2);
      int clr = mt_incr(color[oid], 1);

      if (clr == bfs_mtgl_t::white)
      {
        visitor.tree_edge(e, src);
        Q.push(oid, mt_incr(count, 1));
      }
      else
      {
        visitor.back_edge(e, src);
      }
    }
  }

  void visit_in(edge e)
  {
    vertex v2 = target(e, _g);

    if (visitor.visit_test(e, v2))
    {
      vertex first = source(e, _g);
      int oid = get(vid_map, first);
      int clr = mt_incr(color[oid], 1);

      if (clr == bfs_mtgl_t::white)
      {
        visitor.tree_edge(e, v2);
        Q.push(oid, mt_incr(count, 1));
      }
      else
      {
        visitor.back_edge(e, v2);
      }
    }
  }

  void bfs_mtgl_visit(vertex v)
  {
    visitor.pre_visit(v);
    src = v;
    visit_edges(_g, v, *this, pcutoff, true, REVERSED);
    visitor.finish_vertex(v);
  }

  user_visitor visitor;
  ColorMap color;
  queue<int>& Q;
  vertex src;
  int& count;
  vertex_id_map<graph>& vid_map;
  graph& _g;
};

template <typename graph, typename ColorMap, typename user_visitor, int pcutoff>
class internal_bfs_mtgl_visitor<graph, ColorMap, user_visitor, REVERSED,
                                PURE_FILTER, pcutoff> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;
  typedef breadth_first_search_mtgl<graph, ColorMap, user_visitor, REVERSED,
                                    PURE_FILTER, pcutoff>
          bfs_mtgl_t;

  internal_bfs_mtgl_visitor(graph& g, ColorMap& col, queue<int>& _Q,
                            int& _count, user_visitor& vis,
                            vertex_id_map<graph>& vm) :
    visitor(vis), color(col), Q(_Q), count(_count), vid_map(vm), _g(g) {}

  internal_bfs_mtgl_visitor(const internal_bfs_mtgl_visitor& i) :
    visitor(i.visitor), color(i.color), Q(i.Q), count(i.count),
    vid_map(i.vid_map), _g(i._g), src(i.src) {}

  void set_data(vertex v) { src = v; }
  void bfs_mtgl_root(vertex v) { visitor.bfs_root(v); }

  void operator()(edge e)
  {
    vertex second = target(e, _g);
    if (second != src)  return;

    if (visitor.visit_test(e, src))
    {
      vertex v2 = source(e, _g);
      int oid = get(vid_map, v2);
      visitor.tree_edge(e, src);
      Q.push(oid, mt_incr(count, 1));
    }
    else
    {
      visitor.back_edge(e, src);
    }
  }

  void visit_in(edge e)
  {
    vertex v2 = target(e, _g);

    if (visitor.visit_test(e, v2))
    {
      vertex first = source(e, _g);
      int oid = get(vid_map, first);
      visitor.tree_edge(e, v2);
      Q.push(oid, mt_incr(count, 1));
    }
    else
    {
      visitor.back_edge(e, v2);
    }
  }

  void bfs_mtgl_visit(vertex v)
  {
    visitor.pre_visit(v);
    src = v;
    visit_edges(_g, v, *this, pcutoff, true, REVERSED);
    visitor.finish_vertex(v);
  }

  user_visitor visitor;
  ColorMap color;
  queue<int>& Q;
  int& count;
  vertex src;
  vertex_id_map<graph>& vid_map;
  graph& _g;
};

template<typename graph, typename ColorMap, typename bfs_mtgl_visitor,
         int directed = UNDIRECTED, int filter = NO_FILTER, int pcutoff = 5000>
class breadth_first_search_mtgl_high_low {
public:
  typedef enum { white, gray, black } SearchColor;
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;
  typedef typename graph_traits<graph>::vertex_iterator vertex_iterator;

  breadth_first_search_mtgl_high_low(graph& g, ColorMap& c,
                                     bfs_mtgl_visitor vis) :
    _g(g), visitor(vis), color(c), count(0), Q(queue<int>(num_vertices(_g))) {}

  // Constructor adding a parameter to specify the max size of the queue.
  // This is used when the queue needs to be smaller or larger.  The default
  // queue size is the number of vertices in g.  When a BFS is run on a
  // sub-problem (i.e., in a recursive routine) and we know the max size
  // the queue might grow to, a smaller queue could give a significant
  // performance increase over the default size.  When running a BFS in which
  // the queue could grow larger than the number of vertices in g, a larger
  // queue size must be used.  There is no out of bounds checking.  If the
  // queue grows larger than it is initialized to, memory stomping happens.
  breadth_first_search_mtgl_high_low(graph& g, ColorMap& c,
                                     bfs_mtgl_visitor vis, int max_queue_size) :
    _g(g), visitor(vis), color(c), count(0), Q(queue<int>(max_queue_size)) {}

  breadth_first_search_mtgl_high_low(
      const breadth_first_search_mtgl_high_low& b) :
    _g(b._g), visitor(b.visitor), color(b.color), count(b.count), Q(b.Q) {}

  ~breadth_first_search_mtgl_high_low() {}

  // This isn't right, how can we do a high_low traversal from a bfs?
  // We can do it by going over neighbors, but this doesn't do it.
  void run(vertex v, bool recursive = false)
  {
    int* hd;
    int* ld;
    int n_hd;
    int n_ld;

    vertex_id_map<graph> vid_map = get(_vertex_id_map, _g);

    count = 0;

    typedef internal_bfs_mtgl_visitor<graph, ColorMap, bfs_mtgl_visitor,
                                      directed, filter, pcutoff>
            internal_bfs_mtgl_t;

    internal_bfs_mtgl_t helper(_g, color, Q, count, visitor, vid_map);

    vertex_iterator verts = vertices(_g);

    int clr = mt_incr(color[get(vid_map, v)], 1);
    if (clr == breadth_first_search_mtgl_high_low::white ||
        visitor.start_test(v))
    {
      helper.bfs_mtgl_root(v);
      helper.bfs_mtgl_visit(v);

      if (recursive)
      {
        int q_start_indx = 0;
        int q_end_indx = helper.count;

        // Parallelize depending on how many to process.
        while (q_end_indx > q_start_indx)
        {
          hd = new int[q_end_indx - q_start_indx];
          ld = new int[q_end_indx - q_start_indx];
          n_hd = 0;
          n_ld = 0;

          #pragma mta assert parallel
          for (int i = q_start_indx; i < q_end_indx; i++)
          {
            int vid = Q.elem(i);
            vertex vert = verts[vid];

            int tot_deg;
            if (directed == UNDIRECTED)
            {
              tot_deg = out_degree(vert, _g);
            }
            else if (directed == DIRECTED)
            {
              tot_deg = out_degree(vert, _g);
            }
            else if (directed == REVERSED)
            {
              tot_deg = in_degree(vert, _g);
            }

            if (tot_deg > 5000)
            {
              int ind = mt_incr(n_hd, 1);
              hd[ind] = vid;
            }
            else
            {
              int ind = mt_incr(n_ld, 1);
              ld[ind] = vid;
            }
          }

          #pragma mta assert parallel
          #pragma mta loop future
          for (int i = 0; i < n_hd; i++)
          {
            internal_bfs_mtgl_t helper2(helper);
            helper2.bfs_mtgl_visit(verts[hd[i]]);
          }

          #pragma mta assert parallel
          for (int i = 0; i < n_ld; i++)
          {
            internal_bfs_mtgl_t helper2(helper);
            helper2.bfs_mtgl_visit(verts[ld[i]]);
          }

          if (n_hd) delete [] hd;
          hd = 0;

          if (n_ld) delete [] ld;
          ld = 0;

          q_start_indx = q_end_indx;
          q_end_indx = helper.count;
        }
      }
    }
  }

  int count;
  bfs_mtgl_visitor& visitor;
  graph& _g;
  ColorMap color;
  queue<int> Q;
};

};

#endif

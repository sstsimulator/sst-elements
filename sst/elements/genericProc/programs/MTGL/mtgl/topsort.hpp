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
/*! \file topsort.hpp

    \brief This code contains the routines that find the "topsort prefix" and
           "topsort suffix" of a non-dag (or the while topsort of a dag).

    \author Greg Mackey (gemacke@sandia.gov)
    \author Jon Berry (jberry@sandia.gov)

    \date 8/2006
*/
/****************************************************************************/

#ifndef MTGL_TOPSORT_HPP
#define MTGL_TOPSORT_HPP

#include <cstdio>

#include <mtgl/graph_traits.hpp>
#include <mtgl/mtgl_adapter.hpp>
#include <mtgl/psearch.hpp>
#include <mtgl/util.hpp>
#include <mtgl/transpose_adapter.hpp>

#ifdef __MTA__
  #include <sys/mta_task.h>
  #include <machine/runtime.h>
#endif

namespace mtgl {

/// Visitor that computes the indegrees of the vertices in a directed graph.
template <typename graph_adapter>
class in_degree_visitor : public default_psearch_visitor<graph_adapter> {
public:
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge;

  in_degree_visitor(vertex_id_map<graph_adapter>& _vid_map, int* in_deg,
                    graph_adapter& _g, int* subprob = 0, int my_subp = 0) :
      vid_map(_vid_map), in_degree(in_deg), subproblem(subprob),
      my_subproblem(my_subp), g(_g) {}

  bool visit_test(edge& e, vertex& src) const
  {
    int dest_id = get(vid_map, other(e, src, g));
    return (!subproblem || subproblem[dest_id] == my_subproblem);
  }

  void tree_edge(edge& e, vertex& src) const
  {
    // This is an out-edge from src.  Increment in_degree[dest].
    int dest_id = get(vid_map, other(e, src, g));
    mt_incr(in_degree[dest_id], 1);
  }

  void back_edge(edge& e, vertex& src) const
  {
    // This is an out-edge from src.  Increment in_degree[dest].
    int dest_id = get(vid_map, other(e, src, g));
    mt_incr(in_degree[dest_id], 1);
  }

protected:
  vertex_id_map<graph_adapter>& vid_map;
  int* in_degree;
  int* subproblem;
  int my_subproblem;
  graph_adapter& g;
};

/***/

/// Visitor that computes the outdegrees of the vertices in a directed graph.
template <typename graph_adapter>
class out_degree_visitor : public default_psearch_visitor<graph_adapter> {
public:
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge;

  out_degree_visitor(vertex_id_map<graph_adapter>& _vid_map, int* out_deg,
                     graph_adapter& _g, int* subprob = 0, int my_subp = 0) :
      vid_map(_vid_map), out_degree(out_deg), subproblem(subprob),
      my_subproblem(my_subp), g(_g) {}

  bool visit_test(edge& e, vertex& src) const
  {
    int dest_id = get(vid_map, other(e, src, g));
    return (!subproblem || subproblem[dest_id] == my_subproblem);
  }

  void tree_edge(edge& e, vertex& src) const
  {
    // This is an out-edge from src.  Increment out_degree[src].
    int src_id = get(vid_map, src);
    mt_incr(out_degree[src_id], 1);
  }

  void back_edge(edge& e, vertex& src) const
  {
    // This is an out-edge from src.  Increment out_degree[src].
    int src_id = get(vid_map, src);
    mt_incr(out_degree[src_id], 1);
  }

protected:
  vertex_id_map<graph_adapter>& vid_map;
  int* out_degree;
  int* subproblem;
  int my_subproblem;
  graph_adapter& g;
};

/***/

/// Visitor that computes the degrees of the vertices in an undirected graph.
template <typename graph_adapter>
class degree_visitor : public default_psearch_visitor<graph_adapter> {
public:
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge;

  degree_visitor(vertex_id_map<graph_adapter>& _vid_map, int* deg,
                 graph_adapter& _g, int* subprob = 0, int my_subp = 0) :
      vid_map(_vid_map), degrees(deg), subproblem(subprob),
      my_subproblem(my_subp), g(_g) {}

  bool visit_test(edge& e, vertex& src) const
  {
    int dest_id = get(vid_map, other(e, src, g));
    return (!subproblem || subproblem[dest_id] == my_subproblem);
  }

  void tree_edge(edge& e, vertex& src) const
  {
    // This is an out-edge from src.  Increment degrees[dest].
    int dest_id = get(vid_map, other(e, src, g));
    int deg_incr = dest_id == get(vid_map, src) ? 2 : 1;
    mt_incr(degrees[dest_id], deg_incr);
  }

  void back_edge(edge& e, vertex& src) const
  {
    // This is an out-edge from src.  Increment degrees[dest].
    int dest_id = get(vid_map, other(e, src, g));
    int deg_incr = dest_id == get(vid_map, src) ? 2 : 1;
    mt_incr(degrees[dest_id], deg_incr);
  }

protected:
  vertex_id_map<graph_adapter>& vid_map;
  int* degrees;
  int* subproblem;
  int my_subproblem;
  graph_adapter& g;
};

/***/

template <typename graph_adapter>
class topsort_countdown_visitor :
    public default_psearch_visitor<graph_adapter> {
public:
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge;

  topsort_countdown_visitor(vertex_id_map<graph_adapter>& _vid_map,
                            int* in_or_out_deg, graph_adapter& _g,
                            int* subp = 0, int my_subp = 0) :
      vid_map(_vid_map), in_or_out_degree(in_or_out_deg), subproblem(subp),
      my_subproblem(my_subp), g(_g) {}

  bool visit_test(edge& e, vertex& src) const
  {
    int dest_id = get(vid_map, other(e, src, g));

    if (subproblem != 0 && (subproblem[dest_id] != my_subproblem)) return false;

    int remaining = mt_incr(in_or_out_degree[dest_id], -1);

    // Visit the edge if this is the last edge entering the vertex.
    if (remaining == 1)  return true;

    return false;
  }

protected:
  vertex_id_map<graph_adapter>& vid_map;
  int* in_or_out_degree;
  int* subproblem;
  int my_subproblem;
  graph_adapter& g;
};

/***/

template <typename graph_adapter>
class topsort_ordering_visitor : public default_psearch_visitor<graph_adapter> {
  // Positive top levels indicate that the vertex is settled while negative
  // top levels or 0 mean that the vertex is not settled.  After the algorithm
  // has finished, vertices with positive top levels have been removed while
  // vertices with negative top levels were touching removed vertices but
  // belong to a connected component and weren't removed.  Vertices with 0 top
  // level were in the interior of a connected component and were never
  // touched. When there is more than one path to a vertex, we assign the
  // largest top level possible (i.e. the one from following the longest path
  // to the vertex).

public:
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge;

  topsort_ordering_visitor(vertex_id_map<graph_adapter>& _vid_map,
                           int* in_or_out_deg, int* tl, graph_adapter& _g,
                           int* subp = 0, int my_subp = 0) :
      vid_map(_vid_map), in_or_out_degree(in_or_out_deg), top_level(tl),
      subproblem(subp), my_subproblem(my_subp), g(_g) {}

  void psearch_root(vertex& root)
  {
    // If the root hasn't been visited before, set its top level to -1.
    int root_id = get(vid_map, root);
    int root_tl = mt_readfe(top_level[root_id]);
    if (root_tl == 0)  --root_tl;
    mt_write(top_level[root_id], root_tl);
  }

  void pre_visit(vertex &v)
  {
    // This vertex is settled, so change its top level value to positive to
    // mark it finished.
    int v_id = get(vid_map, v);
    int v_tl = mt_readfe(top_level[v_id]);
    mt_write(top_level[v_id], -v_tl);
  }

  bool visit_test(edge& e, vertex& src) const
  {
    int src_id = get(vid_map, src);
    int dest_id = get(vid_map, other(e, src, g));

    if (subproblem != 0 && (subproblem[dest_id] != my_subproblem)) return false;

    // Increment the top level of the destination vertex if this path is
    // longer than any previous path traveled to the destination vertex.
    int dest_tl = mt_readfe(top_level[dest_id]);
    if (-top_level[src_id] <= dest_tl)
    {
      mt_write(top_level[dest_id], -top_level[src_id] - 1);
    }
    else
    {
      mt_write(top_level[dest_id], dest_tl);
    }

    int remaining = mt_incr(in_or_out_degree[dest_id], -1);

    // Visit the edge if this is the last edge entering the vertex.
    if (remaining == 1)  return true;

    return false;
  }

protected:
  vertex_id_map<graph_adapter>& vid_map;
  int* in_or_out_degree;
  int* top_level;
  int* subproblem;
  int my_subproblem;
  graph_adapter& g;
};

/***/

template <typename graph_adapter>
class topsort_labeling_visitor : public default_psearch_visitor<graph_adapter> {
public:
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge;

  topsort_labeling_visitor(vertex_id_map<graph_adapter>& _vid_map,
                           int* in_or_out_deg, int* st, int* ft, int& nst,
                           graph_adapter& _g, int* subp = 0, int my_subp = 0) :
      vid_map(_vid_map), in_or_out_degree(in_or_out_deg), start_time(st),
      finish_time(ft), next_start_time(nst), subproblem(subp),
      my_subproblem(my_subp), g(_g) {}

  bool visit_test(edge& e, vertex& src) const
  {
    int dest_id = get(vid_map, other(e, src, g));

    if (subproblem != 0 && (subproblem[dest_id] != my_subproblem)) return false;

    int remaining = mt_incr(in_or_out_degree[dest_id], -1);
    if (remaining == 1)  return true;

    return false;
  }

  void finish_vertex(vertex& v)
  {
    int v_id = get(vid_map, v);
    int my_start = mt_incr(next_start_time, 2);
    start_time[v_id] = my_start;
    finish_time[v_id] = my_start + 1;
  }

protected:
  vertex_id_map<graph_adapter>& vid_map;
  int* in_or_out_degree;
  int* subproblem;
  int my_subproblem;
  int* start_time;
  int* finish_time;
  int& next_start_time;
  graph_adapter& g;
};

/// \brief Computes the indegrees of the vertices in directed graph gg.
///
/// A malloced int array (indexed by vertex id) is returned by run() with the
/// result.  It is up to the user to free the memory.
template <typename graph_adapter>
class compute_in_degree {
public:
  typedef typename graph_traits<graph_adapter>::size_type size_type;

  compute_in_degree(graph_adapter& gg, int* subprob = 0, int my_subprb = 0) :
      ga(gg), subproblem(subprob), my_subproblem(my_subprb) {}

  int* run()
  {
    size_type order = num_vertices(ga);

    int* in_degrees = (int*) malloc(order * sizeof(int));
    #pragma mta assert nodep
    for (size_type i = 0; i < order; ++i)  in_degrees[i] = 0;

    vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, ga);

    in_degree_visitor<graph_adapter> indeg_vis(vid_map, in_degrees, ga,
                                               subproblem, my_subproblem);

    psearch_high_low<graph_adapter, in_degree_visitor<graph_adapter>,
                     AND_FILTER, DIRECTED, 4> psrch(ga, indeg_vis);
    psrch.run();

    return in_degrees;
  }

private:
  graph_adapter& ga;
  int* subproblem;
  int my_subproblem;
};

/***/

/// \brief Computes the outdegrees of the vertices in directed graph gg.
///
/// A malloced int array (indexed by vertex id) is returned by run() with the
/// result.  It is up to the user to free the memory.
template <typename graph_adapter>
class compute_out_degree {
public:
  typedef typename graph_traits<graph_adapter>::size_type size_type;

  compute_out_degree(graph_adapter& gg, int* subprb = 0, int my_subprb = 0) :
      ga(gg), subproblem(subprb), my_subproblem(my_subprb) {}

  int* run()
  {
    size_type order = num_vertices(ga);

    int* out_degrees = (int*) malloc(order * sizeof(int));
    #pragma mta assert nodep
    for (size_type i = 0; i < order; ++i)  out_degrees[i] = 0;

    vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, ga);

    out_degree_visitor<graph_adapter> outdeg_vis(vid_map, out_degrees);

    psearch_high_low<graph_adapter, out_degree_visitor<graph_adapter>,
                     AND_FILTER, DIRECTED, 4> psrch(ga, outdeg_vis);
    psrch.run();

    return out_degrees;
  }

private:
  graph_adapter& ga;
  int* subproblem;
  int my_subproblem;
};

/***/

/// \brief Computes the degrees of the vertices in undirected graph gg.
///
/// A malloced int array (indexed by vertex id) is returned by run() with the
/// result.  It is up to the user to free the memory.
template <typename graph_adapter>
class compute_degree {
public:
  typedef typename graph_traits<graph_adapter>::size_type size_type;

  compute_degree(graph_adapter& gg, int* subprob = 0, int my_subprb = 0) :
      ga(gg), subproblem(subprob), my_subproblem(my_subprb) {}

  int* run()
  {
    size_type order = num_vertices(ga);

    int* degrees = (int*) malloc(order * sizeof(int));
    #pragma mta assert nodep
    for (size_type i = 0; i < order; ++i)  degrees[i] = 0;

    vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, ga);

    degree_visitor<graph_adapter> deg_vis(vid_map, degrees, subproblem,
                                          my_subproblem);

    psearch_high_low<graph_adapter, degree_visitor<graph_adapter>,
                     AND_FILTER, UNDIRECTED, 4> psrch(ga, deg_vis);
    psrch.run();

    return degrees;
  }

private:
  graph_adapter& ga;
  int* subproblem;
  int my_subproblem;
};

/***/

/// \brief Finds all the prefix nodes in a graph.
///
/// The parameter array in_deg, which must be initialized to the indegrees of
/// the vertices, is modified.  The vertices with 0 indegree after the
/// algorithm has finished are the prefix vertices.
template <typename graph_adapter>
class topsort_prefix {
public:
  typedef typename graph_traits<graph_adapter>::size_type size_type;
  typedef typename graph_traits<graph_adapter>::vertex_iterator vertex_iterator;

  topsort_prefix(graph_adapter& gg, int* in_deg, int* subprob = 0,
                 int my_subprob = 0) :
      ga(gg), in_degrees(in_deg), subproblem(subprob),
      my_subproblem(my_subprob) {}

  void run()
  {
    vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, ga);

    size_type order = num_vertices(ga);
    vertex_iterator verts = vertices(ga);

    int* search_color = (int*) malloc(order * sizeof(int));
    #pragma mta assert nodep
    for (size_type i = 0; i < order; ++i)  search_color[i] = 0;

    #pragma mta assert parallel
    #pragma mta loop future
    for (size_type i = 0; i < order; ++i)
    {
      if (in_degrees[i] == 0)
      {
        topsort_countdown_visitor<graph_adapter>
            ts_in_vis(vid_map, in_degrees, subproblem, my_subproblem);

        psearch<graph_adapter, int*, topsort_countdown_visitor<graph_adapter>,
                AND_FILTER, DIRECTED> psrch(ga, search_color, ts_in_vis);
        psrch.run(verts[i]);
      }
    }

    free(search_color);
  }

private:
  graph_adapter& ga;
  int* in_degrees;
  int* subproblem;
  int my_subproblem;
};

/***/

/// \brief Finds all the suffix nodes in a graph.
///
/// The parameter array out_deg, which must be initialized to the outdegrees
/// of the vertices, is modified.  The vertices with 0 outdegree after the
/// algorithm has finished are the suffix vertices.
template <typename graph_adapter>
class topsort_suffix {
public:
  topsort_suffix(graph_adapter& gg, int* out_deg, int* subprob = 0,
                 int my_subprob = 0) :
      ga(gg), out_degrees(out_deg), subproblem(subprob),
      my_subproblem(my_subprob) {}

  void run()
  {
    // Run topsort_prefix on the transpose of the graph.
    transpose_adapter<graph_adapter> ta(ga);

    topsort_prefix<transpose_adapter<graph_adapter> >
        tp(ta, out_degrees, subproblem, my_subproblem);

    tp.run();
  }

private:
  graph_adapter& ga;
  int* out_degrees;
  int* subproblem;
  int my_subproblem;
};

/***/

/// \brief Finds all the "outer" nodes in a graph, those not belonging to a
///        conncected component.
///
/// The parameter array deg, which must be initialized to the degrees of the
/// vertices, is modified.  The vertices with 0 degree after the algorithm has
/// finished are the outer vertices.
///
/// TODO:  Need to make sure parallel is correct.
template <typename graph_adapter>
class topsort_undirected {
public:
  typedef typename graph_traits<graph_adapter>::size_type size_type;
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex;
  typedef typename graph_traits<graph_adapter>::adjacency_iterator adj_iter;
  typedef typename graph_traits<graph_adapter>::vertex_iterator vertex_iterator;

  topsort_undirected(graph_adapter& gg, int* deg, int* subprob = 0,
                     int my_subprob = 0) :
      ga(gg), degrees(deg), subproblem(subprob), my_subproblem(my_subprob) {}

  void run()
  {
    vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, ga);
    size_type order = num_vertices(ga);
    vertex_iterator verts = vertices(ga);

    int* search_color = (int*) malloc(order * sizeof(int));
    #pragma mta assert nodep
    for (size_type i = 0; i < order; ++i)  search_color[i] = 0;

    int* top_level = (int*) malloc(order * sizeof(int));
    #pragma mta assert nodep
    for (size_type i = 0; i < order; ++i)  top_level[i] = 0;

    bool settled_vertex = true;

    while (settled_vertex)
    {
      settled_vertex = false;

      #pragma mta assert parallel
      #pragma mta loop future
      for (size_type i = 0; i < order; ++i)
      {
        if (subproblem && subproblem[i] != my_subproblem)  continue;

        if (search_color[i] == 0 && (degrees[i] == 1 || degrees[i] == 0))
        {
          settled_vertex = true;

          search_color[i] = 1;
          top_level[i] = 1;

          vertex cur_vert = verts[i];
          adj_iter vit = adjacent_vertices(cur_vert, ga);
          int deg = out_degree(cur_vert, ga);
          for (int j = 0; j < deg; ++j)
          {
            vertex dest = vit[j];
            int dest_id = get(vid_map, dest);

            if (subproblem && subproblem[dest_id] != my_subproblem)  continue;

            if (degrees[dest_id] == 1 && search_color[dest_id] == 0)
            {
              top_level[dest_id] = 1;
            }

            search_color[dest_id] = 1;
            --degrees[dest_id];
          }
        }
      }

      #pragma mta assert parallel
      #pragma mta loop future
      for (size_type i = 0; i < order; ++i)
      {
        if (top_level[i] == 0)  search_color[i] = 0;
      }
    }

    #pragma mta assert parallel
    #pragma mta loop future
    for (size_type i = 0; i < order; ++i)
    {
      degrees[i] = top_level[i] ? 0 : 1;
    }

    free(search_color);
    free(top_level);
  }

private:
  graph_adapter ga;
  int* degrees;
  int* subproblem;
  int my_subproblem;
};

/***/

/// \brief Finds the topological level of the prefix nodes in a graph.
///
/// The parameter array in_deg must be initialized to the indegrees of the
/// vertices.  The function run() returns an integer array with the answer.
/// Positive top levels indicate that the vertex is settled while negative top
/// levels or 0 mean that the vertex is not settled.  After the algorithm has
/// finished, vertices with positive top levels have been removed while
/// vertices with negative top levels were touching removed vertices but
/// belong to a connected component and weren't removed.  Vertices with 0 top
/// level were in the interior of a connected component and were never
/// touched.  When there is more than one path to a vertex, we assign the
/// largest top level possible (i.e. the one from following the longest path
/// to the vertex).
///
/// The user is expected to delete the memory pointed to by the return value
/// using free().
template <typename graph_adapter>
class topsort_prefix_ordering {
public:
  typedef typename graph_traits<graph_adapter>::size_type size_type;
  typedef typename graph_traits<graph_adapter>::vertex_iterator vertex_iterator;

  topsort_prefix_ordering(graph_adapter& gg, int* in_deg, int* subprob = 0,
                          int my_subprob = 0) :
      ga(gg), in_degrees(in_deg), subproblem(subprob),
      my_subproblem(my_subprob) {}

  int* run()
  {
    vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, ga);
    size_type order = num_vertices(ga);
    vertex_iterator verts = vertices(ga);

    int* search_color = (int*) malloc(order * sizeof(int));
    #pragma mta assert nodep
    for (size_type i = 0; i < order; ++i)  search_color[i] = 0;

    int* top_level = (int*) malloc(order * sizeof(int));
    #pragma mta assert nodep
    for (size_type i = 0; i < order; ++i)  top_level[i] = 0;

    #pragma mta assert parallel
    #pragma mta loop future
    for (size_type i = 0; i < order; ++i)
    {
      if (in_degrees[i] == 0)
      {
        topsort_ordering_visitor<graph_adapter>
            ts_vis(vid_map, in_degrees, top_level, subproblem, my_subproblem);

        psearch<graph_adapter, int*, topsort_ordering_visitor<graph_adapter>,
                AND_FILTER, DIRECTED> psrch(ga, search_color, ts_vis);
        psrch.run(verts[i]);
      }
    }

    free(search_color);

    return top_level;
  }

private:
  graph_adapter ga;
  int* in_degrees;
  int* subproblem;
  int my_subproblem;
};

/***/

/// \brief Finds the topological level of the suffix nodes in a graph.
///
/// The parameter array out_deg must be initialized to the outdegrees of the
/// vertices.  The function run() returns an integer array with the answer.
/// Positive top levels indicate that the vertex is settled while negative top
/// levels or 0 mean that the vertex is not settled.  After the algorithm has
/// finished, vertices with positive top levels have been removed while
/// vertices with negative top levels were touching removed vertices but
/// belong to a connected component and weren't removed.  Vertices with 0 top
/// level were in the interior of a connected component and were never
/// touched.  When there is more than one path to a vertex, we assign the
/// largest top level possible (i.e. the one from following the longest path
/// to the vertex).
///
/// The user is expected to delete the memory pointed to by the return value
/// using free().
template <typename graph_adapter>
class topsort_suffix_ordering {
public:
  topsort_suffix_ordering(graph_adapter& gg, int* out_deg, int* subprob = 0,
                          int my_subprob = 0) :
      ga(gg), out_degrees(out_deg), subproblem(subprob),
      my_subproblem(my_subprob) {}

  int* run()
  {
    // Run topsort_prefix_ordering on the transpose of the graph.
    transpose_adapter<graph_adapter> ta(ga);

    topsort_prefix_ordering<transpose_adapter<graph_adapter> >
        tp(ta, out_degrees, subproblem, my_subproblem);

    return tp.run();
  }

private:
  graph_adapter ga;
  int* out_degrees;
  int* subproblem;
  int my_subproblem;
};

/***/

/// \brief Finds the topological level of the nodes in a graph.
///
/// The parameter array deg must be initialized to the degrees of the
/// vertices.  The function run() returns an integer array with the answer.
/// Positive top levels indicate that the vertex is settled while negative top
/// levels or 0 mean that the vertex is not settled.  After the algorithm has
/// finished, vertices with positive top levels have been removed while
/// vertices with negative top levels were touching removed vertices but
/// belong to a connected component and weren't removed.  Vertices with 0 top
/// level were in the interior of a connected component and were never
/// touched.  When there is more than one path to a vertex, we assign the
/// largest top level possible (i.e. the one from following the longest path
/// to the vertex).
///
/// The user is expected to delete the memory pointed to by the return value
/// using free().
///
/// TODO:  Need to make sure parallel is correct.
template <typename graph_adapter>
class topsort_undirected_ordering {
public:
  typedef typename graph_traits<graph_adapter>::size_type size_type;
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex;
  typedef typename graph_traits<graph_adapter>::adjacency_iterator adj_iter;
  typedef typename graph_traits<graph_adapter>::vertex_iterator vertex_iterator;

  topsort_undirected_ordering(graph_adapter& gg, int* deg, int* subprob = 0,
                              int my_subprob = 0) :
      ga(gg), degrees(deg), subproblem(subprob), my_subproblem(my_subprob) {}

  int* run()
  {
    vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, ga);
    size_type order = num_vertices(ga);
    vertex_iterator verts = vertices(ga);

    int* search_color = (int*) malloc(order * sizeof(int));
    #pragma mta assert nodep
    for (size_type i = 0; i < order; ++i)  search_color[i] = 0;

    int* top_level = (int*) malloc(order * sizeof(int));
    #pragma mta assert nodep
    for (size_type i = 0; i < order; ++i)  top_level[i] = 0;

    bool settled_vertex = true;
    int level = 1;

    while (settled_vertex)
    {
      settled_vertex = false;

      #pragma mta assert parallel
      #pragma mta loop future
      for (size_type i = 0; i < order; ++i)
      {
        if (subproblem && subproblem[i] != my_subproblem)  continue;

        if (search_color[i] == 0 && (degrees[i] == 1 || degrees[i] == 0))
        {
          settled_vertex = true;

          search_color[i] = 1;
          top_level[i] = level;

          vertex cur_vert = verts[i];
          adj_iter vit = adjacent_vertices(cur_vert, ga);
          int deg = out_degree(cur_vert, ga);
          for (int j = 0; j < deg; ++j)
          {
            vertex dest = vit[j];
            int dest_id = get(vid_map, dest);

            if (subproblem && subproblem[dest_id] != my_subproblem)  continue;

            if (degrees[dest_id] == 1 && search_color[dest_id] == 0)
            {
              top_level[dest_id] = level;
            }

            search_color[dest_id] = 1;
            --degrees[dest_id];
          }
        }
      }

      #pragma mta assert parallel
      #pragma mta loop future
      for (size_type i = 0; i < order; ++i)
      {
        if (top_level[i] == 0)  search_color[i] = 0;
      }

      ++level;
    }

    free(search_color);

    return top_level;
  }

private:
  graph_adapter ga;
  int* degrees;
  int* subproblem;
  int my_subproblem;
};

/***/

template <typename graph_adapter>
class topsort_prefix_labeling {
public:
  typedef typename graph_traits<graph_adapter>::size_type size_type;
  typedef typename graph_traits<graph_adapter>::vertex_iterator vertex_iterator;

  topsort_prefix_labeling(graph_adapter& gg, int* in_deg, int* st, int* ft,
                          int& next_st, int* subprob = 0, int my_subprob = 0) :
      ga(gg), in_degrees(in_deg), start_times(st), finish_times(ft),
      next_start_time(next_st), subproblem(subprob),
      my_subproblem(my_subprob) {}

  void run()
  {
    vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, ga);
    size_type order = num_vertices(ga);
    vertex_iterator verts = vertices(ga);

    int* search_color = (int*) malloc(order * sizeof(int));
    #pragma mta assert nodep
    for (size_type i = 0; i < order; ++i)  search_color[i] = 0;

    #pragma mta assert parallel
    #pragma mta loop future
    for (size_type i = 0; i < order; ++i)
    {
      if (in_degrees[i] == 0)
      {
        topsort_labeling_visitor<graph_adapter>
            ts_in_vis(vid_map, in_degrees, start_times, finish_times,
                      next_start_time, subproblem, my_subproblem);

        psearch<graph_adapter, int*, topsort_labeling_visitor<graph_adapter>,
                AND_FILTER, DIRECTED> psrch(ga, search_color, ts_in_vis);
        psrch.run(verts[i]);
      }
    }

    free(search_color);
  }

private:
  graph_adapter& ga;
  int* in_degrees;
  int* start_times;
  int* finish_times;
  int& next_start_time;
  int* subproblem;
  int my_subproblem;
};

/***/

template <typename graph_adapter>
class topsort_suffix_labeling {
public:
  topsort_suffix_labeling(graph_adapter& gg, int* out_deg, int* st, int* ft,
                          int& next_st, int* subprob = 0, int my_subprob = 0) :
      ga(gg), out_degrees(out_deg), start_times(st), finish_times(ft),
      next_start_time(next_st), subproblem(subprob),
      my_subproblem(my_subprob) {}

  void run()
  {
    transpose_adapter<graph_adapter> ta(ga);

    topsort_prefix_labeling<transpose_adapter<graph_adapter> >
        tp(ta, out_degrees, start_times, finish_times, next_start_time,
           subproblem, my_subproblem);

    return tp.run();
  }

private:
  graph_adapter& ga;
  int* out_degrees;
  int* start_times;
  int* finish_times;
  int& next_start_time;
  int* subproblem;
  int my_subproblem;
};

/***/

// TODO:  Add a topsort_undir_labeling class.

/***/

template <typename graph_adapter>
class topological_ordering {
public:
  typedef typename graph_traits<graph_adapter>::size_type size_type;

  topological_ordering(graph_adapter& gg, int* subprob = 0,
                       int my_subprob = 0) :
      ga(gg), subproblem(subprob), my_subproblem(my_subprob) {}

  int* run()
  {
    size_type order = num_vertices(ga);

    if (is_undirected(ga))
    {
      // Compute the degrees.
      compute_degree<graph_adapter> cd(ga);
      int* degrees = cd.run();

      // Compute the undirected topological ordering.
      topsort_undirected_ordering<graph_adapter> tp(ga, degrees);
      int* top_undir_level = tp.run();

      free(degrees);

      // Find the largest top level.
      int largest = 0;
      for (size_type i = 0; i < order; ++i)
      {
        if (top_undir_level[i] > largest)  largest = top_undir_level[i];
      }

      // Set all nodes who are in the connected component (have a negative or
      // 0 top level) to largest + 1.
      #pragma mta assert nodep
      for (size_type i = 0; i < order; ++i)
      {
        if (top_undir_level[i] <= 0)  top_undir_level[i] = largest + 1;
      }

      return top_undir_level;
    }
    else
    {
      int* top_ordering = (int*) malloc(order * sizeof(int));
      #pragma mta assert nodep
      for (size_type i = 0; i < order; ++i)  top_ordering[i] = 0;

      // Compute the in degrees.
      compute_in_degree<graph_adapter> cid(ga);
      int* in_degrees = cid.run();

      // Compute the out degrees.
      compute_out_degree<graph_adapter> cod(ga);
      int* out_degrees = cod.run();

      // Compute the topological prefix ordering.
      topsort_prefix_ordering<graph_adapter> tp(ga, in_degrees);
      int* top_prefix_level = tp.run();

      free(in_degrees);

      // Compute the topological suffix ordering.
      topsort_suffix_ordering<graph_adapter> ts(ga, out_degrees);
      int* top_suffix_level = ts.run();

      free(out_degrees);

      // Add the prefix orderings to the topological ordering.
      #pragma mta assert nodep
      for (size_type i = 0; i < order; ++i)
      {
        if (top_prefix_level[i] > 0)  top_ordering[i] = top_prefix_level[i];
      }

      // Find the largest prefix and suffix levels.  Only consider suffix
      // levels of vertices that weren't chosen as prefix vertices.
      int max_prefix_level = 0;
      int max_suffix_level = 0;
      for (size_type i = 0; i < order; ++i)
      {
        if (top_prefix_level[i] > max_prefix_level)
        {
          max_prefix_level = top_prefix_level[i];
        }

        if (top_ordering[i] == 0 && top_suffix_level[i] > max_suffix_level)
        {
          max_suffix_level = top_suffix_level[i];
        }
      }

      free(top_prefix_level);

      // Add the suffix orderings to the topological ordering.  If a vertex
      // was added as a prefix ordering, ignore it.
      #pragma mta assert nodep
      for (size_type i = 0; i < order; ++i)
      {
        if (top_suffix_level[i] > 0 && top_ordering[i] == 0)
        {
          top_ordering[i] = max_prefix_level + 1 + max_suffix_level + 1 -
                            top_suffix_level[i];
        }
      }

      free(top_suffix_level);

      // Set the topological ordering of the main central component.
      #pragma mta assert nodep
      for (size_type i = 0; i < order; ++i)
      {
        if (top_ordering[i] == 0)  top_ordering[i] = max_prefix_level + 1;
      }

      return top_ordering;
    }
  }

private:
  graph_adapter& ga;
  int* subproblem;
  int my_subproblem;
};

/***/

template <typename graph_adapter>
class topological_edge_ordering {
public:
  typedef typename graph_traits<graph_adapter>::size_type size_type;

  topological_edge_ordering(graph_adapter& gg, int* subprob = 0,
                            int my_subprob = 0) :
      ga(gg), subproblem(subprob), my_subproblem(my_subprob) {}

  class get_edge_topo_level {
  public:
    typedef typename graph_traits<graph_adapter>::edge_descriptor edge;
    typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex;

    get_edge_topo_level(graph_adapter& gg, int* tvl, int* tel) :
        ga(gg), vid_map(get(_vertex_id_map, gg)),
        eid_map(get(_edge_id_map, gg)), topo_vert_level(tvl),
        topo_edge_level(tel) {}

    void operator()(edge e)
    {
      vertex src = source(e, ga);
      vertex dest = target(e, ga);
      size_type src_id = get(vid_map, src);
      size_type dest_id = get(vid_map, dest);
      size_type eid = get(eid_map, e);

      if (topo_vert_level[dest_id] > topo_vert_level[src_id])
      {
        topo_edge_level[eid] = topo_vert_level[dest_id] -
                               topo_vert_level[src_id];
      }
      else
      {
        topo_edge_level[eid] = topo_vert_level[src_id] -
                               topo_vert_level[dest_id];
      }
    }

  private:
    graph_adapter& ga;
    vertex_id_map<graph_adapter> vid_map;
    edge_id_map<graph_adapter> eid_map;
    int* topo_vert_level;
    int* topo_edge_level;
  };

  int* run()
  {
    size_type order = num_vertices(ga);
    size_type size = num_edges(ga);

    topological_ordering<graph_adapter> to(ga);
    int* top_order = to.run();

    int* top_edge_order = (int*) malloc(size * sizeof(int));
    get_edge_topo_level getl(ga, top_order, top_edge_order);
    visit_edges(ga, getl);

    free(top_order);

    return top_edge_order;
  }

private:
  graph_adapter& ga;
  int* subproblem;
  int my_subproblem;
};

}

#endif

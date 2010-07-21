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
/*! \file static_graph.hpp

    \brief This graph class implements an enhanced compressed sparse row
           graph that can be directed, undirected, or bidirectional.

    \author Greg Mackey (gemacke@sandia.gov)
    \author Jon Berry (jberry@sandia.gov)
    \author Brad Mancke
    \author Kamesh Madduri

    The first enhancement is storing the source points of the adjacencies in
    addition to the end points.  This yields direct access to source vertices
    of an edge without the need for searching through the index array (aka it
    runs faster). The second enhancement is assigning edge ids based on the
    order the edges are passed during the call to init() instead of based on
    their indices.  These edge ids are kept as two arrays that allow direct
    access both directions between the edge id and the edge index.  This
    allows users to create array property maps for edges more easily as a user
    usually has the property array in the same order as they submitted the
    edges to init().

    \date 1/3/2008
*/
/****************************************************************************/

#ifndef MTGL_STATIC_GRAPH_HPP
#define MTGL_STATIC_GRAPH_HPP

#include <cstdlib>

#include <mtgl/graph_traits.hpp>
#include <mtgl/util.hpp>

namespace mtgl {

template <typename DIRECTION = directedS>
struct static_graph {
  typedef unsigned long size_type;

  static_graph() : m(0), n(0), index(0), src_points(0), end_points(0),
                   original_ids(0), internal_ids(0) {}

  ~static_graph() { clear(); }
  static_graph(const static_graph& g) { deep_copy(g); }

  static_graph& operator=(const static_graph& rhs)
  {
    clear();
    deep_copy(rhs);
    return *this;
  }

  bool is_directed() const { return DIRECTION::is_directed(); }
  bool is_undirected() const { return !is_directed(); }

  void init(size_type order, size_type size, size_type* srcs, size_type* dests)
  {
    printf("X\n");
    n = order;
    m = size;
    size_type a_size = is_undirected() ? 2 * m : m;

    size_type* degree = (size_type*) calloc(order, sizeof(size_type));
    size_type* numEdges = (size_type*) malloc((order + 1) * sizeof(size_type));

    // Count the out degree of each vertex.
    #pragma mta assert nodep
    #pragma mta block schedule
    for (size_type i = 0; i < size; ++i) {
      mt_incr(degree[srcs[i]], 1);
    }


    if (is_undirected())
    {
      #pragma mta assert nodep
      #pragma mta block schedule
      for (size_type i = 0; i < size; ++i) mt_incr(degree[dests[i]], 1);
    }

    // Find the starting index in the endpoints array for each vertex.
    numEdges[0] = 0;    

    #pragma mta block schedule
    for (size_type i = 1; i <= order; ++i)
    {
      numEdges[i] = numEdges[i - 1] + degree[i - 1];
    }

    // Reset degree to be all 0's.
    #pragma mta block schedule
    for (size_type i = 0; i < order; ++i) degree[i] = 0;

    size_type* srcV = (size_type*) malloc(a_size * sizeof(size_type));
    size_type* endV = (size_type*) malloc(a_size * sizeof(size_type));
    size_type* loc_original_ids =
      (size_type*) malloc(a_size * sizeof(size_type));
    size_type* loc_internal_ids = (size_type*) malloc(m * sizeof(size_type));

    #pragma mta assert nodep
    #pragma mta block schedule
    for (size_type i = 0; i < size; ++i)
    {
      size_type u = srcs[i];
      size_type vpos = numEdges[u] + mt_incr(degree[u], 1);
      srcV[vpos] = srcs[i];
      endV[vpos] = dests[i];
      loc_original_ids[vpos] = i;
      loc_internal_ids[i] = vpos;
    }

    if (is_undirected())
    {
      #pragma mta assert nodep
      #pragma mta block schedule
      for (size_type i = 0; i < size; ++i)
      {
        size_type u = dests[i];
        size_type vpos = numEdges[u] + mt_incr(degree[u], 1);
        srcV[vpos] = dests[i];
        endV[vpos] = srcs[i];
        loc_original_ids[vpos] = i;
      }
    }

    index = numEdges;
    src_points = srcV;
    end_points = endV;
    original_ids = loc_original_ids;
    internal_ids = loc_internal_ids;
    printf("X2 %p\n", degree);
    free(degree);
  }

  void clear()
  {
    if (index) free(index);
    if (end_points) free(end_points);
    if (src_points) free(src_points);
    if (original_ids) free(original_ids);
    if (internal_ids) free(internal_ids);
  }

private:
  void deep_copy(const static_graph& rhs)
  {
    m = rhs.m;
    n = rhs.n;

    size_type a_size = is_undirected() ? 2 * m : m;

    index = (size_type*) malloc((n + 1) * sizeof(size_type));
    src_points = (size_type*) malloc(a_size * sizeof(size_type));
    end_points = (size_type*) malloc(a_size * sizeof(size_type));
    original_ids = (size_type*) malloc(a_size * sizeof(size_type));
    internal_ids = (size_type*) malloc(m * sizeof(size_type));

    for (size_type i = 0; i < n + 1; ++i) index[i] = rhs.index[i];
    for (size_type i = 0; i < a_size; ++i) src_points[i] = rhs.src_points[i];
    for (size_type i = 0; i < a_size; ++i) end_points[i] = rhs.end_points[i];
    for (size_type i = 0; i < a_size; ++i)
    {
      original_ids[i] = rhs.original_ids[i];
    }
    for (size_type i = 0; i < m; ++i) internal_ids[i] = rhs.internal_ids[i];
  }

public:
  size_type m;      // # of edges
  size_type n;      // # of vertices

  // Indexes into the edge-arrays for the adjacencies.  Its size is |V|+1.  A
  // vertex, v, has neighbors in {src,end}_points from {src,end}_points[v] to
  // {src,end}_points[v+1].
  size_type* index;

  // src_points and end_points store the source and target vertices for each
  // edge, respectively.  These arrays are of size |E| if directed and 2*|E|
  // if undirected.
  size_type* src_points;
  size_type* end_points;

  // The order of the edges is rearranged from the passed in order during the
  // creation of the graph.  However, the ids of the edges should be assigned
  // based on the original order not the new order.  We need a way to map the
  // original order of the edges to the internal order and vice versa in order
  // to map the correct ids to the correct edges.
  size_type* original_ids;
  size_type* internal_ids;
};

/***************************************/

template <>
struct static_graph<bidirectionalS> {
  typedef unsigned long size_type;

  static_graph() : m(0), n(0), index(0), rev_index(0), cross_index(0),
                   cross_cross_index(0), src_points(0), end_points(0),
                   rev_src_points(0), rev_end_points(0), original_ids(0),
                   internal_ids(0), rev_original_ids(0) {}

  static_graph(const static_graph& g) { deep_copy(g); }
  ~static_graph() { clear(); }

  bool is_directed() const { return true; }
  bool is_undirected() const { return false; }

  static_graph& operator=(const static_graph& rhs)
  {
    clear();
    deep_copy(rhs);
    return *this;
  }

  void init(size_type order, size_type size, size_type* srcs, size_type* dests)
  {
    n = order;
    m = size;
    size_type a_size = m;

    // The out_degree is the default one in static_graph.
    size_type* out_degree = (size_type*) calloc(order, sizeof(size_type));
    size_type* in_degree = (size_type*) calloc(order, sizeof(size_type));
    size_type* out_numEdges =
      (size_type*) malloc((order + 1) * sizeof(size_type));
    size_type* in_numEdges =
      (size_type*) malloc((order + 1) * sizeof(size_type));

    #pragma mta assert nodep
    #pragma mta block schedule
    for (size_type i = 0; i < size; ++i)
    {
      mt_incr(out_degree[srcs[i]], 1);
      mt_incr(in_degree[dests[i]], 1);
    }

    in_numEdges[0] = 0;
    out_numEdges[0] = 0;

    #pragma mta block schedule
    for (size_type i = 1; i <= order; ++i)
    {
      out_numEdges[i] = out_numEdges[i - 1] + out_degree[i - 1];
    }

    #pragma mta block schedule
    for (size_type i = 1; i <= order; ++i)
    {
      in_numEdges[i] = in_numEdges[i - 1] + in_degree[i - 1];
    }

    // Reset out_degree and in_degree to be all 0's.
    #pragma mta block schedule
    for (size_type i = 0; i < order; ++i) out_degree[i] = 0;
    #pragma mta block schedule
    for (size_type i = 0; i < order; ++i) in_degree[i] = 0;

    size_type* out_endV = (size_type*) malloc(a_size * sizeof(size_type));
    size_type* out_srcV = (size_type*) malloc(a_size * sizeof(size_type));
    size_type* in_endV = (size_type*) malloc(a_size * sizeof(size_type));
    size_type* in_srcV = (size_type*) malloc(a_size * sizeof(size_type));
    size_type* loc_cross_index =
      (size_type*) malloc(a_size * sizeof(size_type));
    size_type* loc_cross_cross_index =
      (size_type*) malloc(a_size * sizeof(size_type));
    size_type* loc_original_ids =
      (size_type*) malloc(a_size * sizeof(size_type));
    size_type* loc_internal_ids = (size_type*) malloc(m * sizeof(size_type));

    #pragma mta assert nodep
    #pragma mta block schedule
    for (size_type i = 0; i < size; ++i)
    {
      size_type out_u = srcs[i];
      size_type out_vp = out_numEdges[out_u] + mt_incr(out_degree[out_u], 1);
      out_endV[out_vp] = dests[i];
      out_srcV[out_vp] = srcs[i];
      loc_original_ids[out_vp] = i;
      loc_internal_ids[i] = out_vp;

      // Put in code for in degrees.
      size_type in_u = dests[i];
      size_type in_vp = in_numEdges[in_u] + mt_incr(in_degree[in_u], 1);
      in_endV[in_vp] = srcs[i];
      in_srcV[in_vp] = dests[i];
      loc_cross_index[out_vp] = in_vp;
      loc_cross_cross_index[in_vp] = out_vp;
    }

    // Set the mapping from the reverse edges to the original ids.
    // TODO:  This extra mapping could be gotten rid of, but it would require
    //        doing an extra indirection (original_ids[cross_cross_index[i] in
    //        place of original_ids[i]).  Probably not much of a speed hit,
    //        but it would require having a separate iterator for in incident
    //        edges. BGL has this extra iterator, so we should probably move
    //        to it anyway.
    size_type* loc_rev_original_ids =
      (size_type*) malloc(a_size * sizeof(size_type));

    #pragma mta assert nodep
    #pragma mta block schedule
    for (size_type i = 0; i < a_size; ++i)
    {
      loc_rev_original_ids[i] = loc_original_ids[loc_cross_cross_index[i]];
    }

    index = out_numEdges;
    rev_index = in_numEdges;
    cross_index = loc_cross_index;
    cross_cross_index = loc_cross_cross_index;
    end_points = out_endV;
    src_points = out_srcV;
    rev_end_points = in_endV;
    rev_src_points = in_srcV;
    original_ids = loc_original_ids;
    internal_ids = loc_internal_ids;
    rev_original_ids = loc_rev_original_ids;

    free(out_degree);
    free(in_degree);
  }

  void clear()
  {
    if (index) free(index);
    if (cross_index) free(cross_index);
    if (src_points) free(src_points);
    if (end_points) free(end_points);
    if (original_ids) free(original_ids);
    if (internal_ids) free(internal_ids);
    if (rev_original_ids) free(rev_original_ids);
    if (rev_index) free(rev_index);
    if (cross_cross_index) free(cross_cross_index);
    if (rev_src_points) free(rev_src_points);
    if (rev_end_points) free(rev_end_points);
  }

private:
  void deep_copy(const static_graph& rhs)
  {
    m = rhs.m;
    n = rhs.n;

    size_type a_size = m;

    index = (size_type*) malloc((n + 1) * sizeof(size_type));
    cross_index = (size_type*) malloc(a_size * sizeof(size_type));
    src_points = (size_type*) malloc(a_size * sizeof(size_type));
    end_points = (size_type*) malloc(a_size * sizeof(size_type));
    original_ids = (size_type*) malloc(a_size * sizeof(size_type));
    internal_ids = (size_type*) malloc(m * sizeof(size_type));
    rev_original_ids = (size_type*) malloc(a_size * sizeof(size_type));

    for (size_type i = 0; i < n + 1; ++i) index[i] = rhs.index[i];
    for (size_type i = 0; i < a_size; ++i) cross_index[i] = rhs.cross_index[i];
    for (size_type i = 0; i < a_size; ++i) src_points[i] = rhs.src_points[i];
    for (size_type i = 0; i < a_size; ++i) end_points[i] = rhs.end_points[i];
    for (size_type i = 0; i < a_size; ++i)
    {
      original_ids[i] = rhs.original_ids[i];
    }
    for (size_type i = 0; i < a_size; ++i)
    {
      internal_ids[i] = rhs.internal_ids[i];
    }
    for (size_type i = 0; i < a_size; ++i)
    {
      rev_original_ids[i] = rhs.rev_original_ids[i];
    }

    rev_index = (size_type*) malloc((n + 1) * sizeof(size_type));
    cross_cross_index = (size_type*) malloc(a_size * sizeof(size_type));
    rev_src_points = (size_type*) malloc(a_size * sizeof(size_type));
    rev_end_points = (size_type*) malloc(a_size * sizeof(size_type));

    for (size_type i = 0; i < n + 1; ++i) rev_index[i] = rhs.rev_index[i];
    for (size_type i = 0; i < a_size; ++i)
    {
      cross_cross_index[i] = rhs.cross_cross_index[i];
    }
    for (size_type i = 0; i < a_size; ++i)
    {
      rev_src_points[i] = rhs.rev_src_points[i];
    }
    for (size_type i = 0; i < a_size; ++i)
    {
      rev_end_points[i] = rhs.rev_end_points[i];
    }
  }

public:
  size_type m;  // # of edges
  size_type n;  // # of vertices

  // Indexes into the edge-arrays for the adjacencies.  Its size is |V|+1.  A
  // vertex, v, has neighbors in {src,end}_points from {src,end}_points[v] to
  // {src,end}_points[v+1].  The same is true for rev_index with respect to
  // rev_{src,end}_points.
  size_type* index;
  size_type* rev_index;

  // How to map from end_points to rev_end_points (cross_index) and vice
  // versa (cross_cross_index).
  size_type* cross_index;
  size_type* cross_cross_index;

  // src_points and end_points store the source and target vertices for each
  // edge, respectively.  rev_src_points and rev_end_points store the source
  // and target vertices for each inedge, respectively.  These arrays are of
  // size |E|.
  size_type* src_points;
  size_type* end_points;
  size_type* rev_src_points;
  size_type* rev_end_points;

  // The order of the edges is rearranged from the passed in order during the
  // creation of the graph.  However, the ids of the edges should be assigned
  // based on the original order not the new order.  We need a way to map the
  // original order of the edges to the internal order (internal_ids) and
  // vice versa (original_ids) in order to map the correct ids to the
  // correct edges.  We also need a mapping from the internal order of the
  // reverse edges to the original order (rev_original_ids).
  size_type* original_ids;
  size_type* internal_ids;
  size_type* rev_original_ids;
};

}

#endif

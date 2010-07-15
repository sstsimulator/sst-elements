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
/*! \file subgraph_isomorphism.hpp

    \brief Searches for a subgraph in the given graph that is isomorphic to
           a target graph.

    \author Greg Mackey (gemacke@sandia.gov)
    \author Jon Berry (jberry@sandia.gov)

    \date 12/4/2007

    The big graph and target graphs are independent graph types that can be
    either a primary graph adapter or a wrapper adapter.

    The algorithm takes a function object that defines the comparison made
    for the isomorphic match.  A default comparator is provided
    (si_default_comparator), but a user can define their own.  The only
    restriction is that the () operator needs to take the same paramters as
    in si_default_comparator.  The algorithm can handle forward or reverse
    edges in the walk, and the user doesn't have to do anything special to
    make this work.
*/
/****************************************************************************/

#ifndef MTGL_SUBGRAPH_ISOMORPHISM_HPP
#define MTGL_SUBGRAPH_ISOMORPHISM_HPP

#include <cstdio>
#include <limits>

#include <mtgl/util.hpp>
#include <mtgl/psearch.hpp>
#include <mtgl/dynamic_array.hpp>
#include <mtgl/xmt_hash_table.hpp>
#include <mtgl/static_graph_adapter.hpp>
#include <mtgl/subgraph_adapter.hpp>
#include <mtgl/connected_components.hpp>

namespace mtgl {

/*! \brief Default comparison function object that determines if an edge
           from the big graph matches the given walk edge from the target
           graph.

    This function object checks if the type of the edge and the types of the
    source and target vertices from the big graph match those of the currently
    considered walk edge.  It also checkes if the out degrees of the source
    and target vertices in the big graph are at least as big as those from the
    currently considered walk edge from the target graph.  For directed and
    bidirectional graphs, the comparator is called once for each edge.  For
    undirected graphs the comparator is called twice for each edge where each
    endpoint gets a chance to be the "source".

    The comparator function object is passed as a parameter to
    subgraph_isomorphism().  Thus, the user can define their own comparator
    function object to customize the matching.  The only requirement is that
    the declaration of operator() must be identical to the one in the default
    comparator.
*/
template <typename graph_adapter, typename vertex_property_map,
          typename edge_property_map,
          typename graph_adapter_trg = graph_adapter,
          typename vertex_property_map_trg = vertex_property_map,
          typename edge_property_map_trg = edge_property_map>
class si_default_comparator {
public:
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge;
  typedef typename graph_traits<graph_adapter_trg>::vertex_descriptor
          vertex_trg;
  typedef typename graph_traits<graph_adapter_trg>::edge_descriptor edge_trg;

  si_default_comparator(vertex_property_map gvt, edge_property_map get,
                        vertex_property_map_trg trgvt,
                        edge_property_map_trg trget) :
    g_vtype(gvt), g_etype(get), trg_vtype(trgvt), trg_etype(trget) {}

  bool operator()(const edge& g_e, const edge_trg& trg_e,
                  graph_adapter& g, graph_adapter_trg& trg)
  {
    // Get vertices.
    vertex g_src = source(g_e, g);
    vertex g_dest = target(g_e, g);
    vertex_trg trg_src = source(trg_e, trg);
    vertex_trg trg_dest = target(trg_e, trg);

    // The edge is considered a match to the walk edge if its type triple
    // (src vertex type, edge type, dest vertex type) matches the walk edge's
    // corresponding triple, and if the edge's vertices have out degree at
    // least as big as the out degree of the corresponding edge's vertices
    // from the target graph.
    return (g_vtype[g_src] == trg_vtype[trg_src] &&
            g_etype[g_e] == trg_etype[trg_e] &&
            g_vtype[g_dest] == trg_vtype[trg_dest] &&
            out_degree(g_src, g) >= out_degree(trg_src, trg) &&
            out_degree(g_dest, g) >= out_degree(trg_dest, trg));
  }

private:
  vertex_property_map g_vtype;
  edge_property_map g_etype;
  vertex_property_map_trg trg_vtype;
  edge_property_map_trg trg_etype;

  /*!
    \fn bool operator()(const edge& g_e, const edge_trg& trg_e,
                        graph_adapter& g, graph_adapter_trg& trg)

    \brief Checks if edge g_e matches edge trg_e where a match is when the
           type triples (source vertex type, edge type, destination vertex
           type) match and the vertices of g_e have out degree at least as
           big as the out degree of the corresponding vertices of trg_e.

    \param g_e The edge from the big graph.
    \param trg_e The edge from the target graph.
    \param g The big graph.
    \param trg The target graph.
  */
};

template <typename graph_adapter>
class iso_bipartite_visitor : public default_psearch_visitor<graph_adapter> {
public:
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge;
  typedef typename graph_traits<graph_adapter>::size_type size_type;

  iso_bipartite_visitor(graph_adapter& bipartite_graph, size_type* ec) :
    edge_color(ec), vid_map(get(_vertex_id_map, bipartite_graph)),
    eid_map(get(_edge_id_map, bipartite_graph)), bg(bipartite_graph) {}

  // When the other vertex of an edge is visited during a search, the visitor
  // for the other vertex is initialized using the copy constructor.
  iso_bipartite_visitor(const iso_bipartite_visitor& vis) :
    edge_color(vis.edge_color), vid_map(vis.vid_map), eid_map(vis.eid_map),
    bg(vis.bg) {}

  int visit_test(edge& e, vertex& src)
  {
    size_type sid = get(vid_map, src);
    size_type tid = get(vid_map, other(e, src, bg));
    size_type eid = get(eid_map, e);
    edge_color[eid] = 1;

    return true;
  }

protected:
  size_type* edge_color;
  vertex_id_map<graph_adapter> vid_map;
  edge_id_map<graph_adapter> eid_map;
  graph_adapter& bg;
};

template<typename T>
class uniqueid_visitor {
public:
  uniqueid_visitor(T o, dynamic_array<T>& bvl) :
    count(0), order(o), bipartite_vertex_level(bvl) {}

  void operator()(const T& k, T& v)
  {
    v = mt_incr(count, 1);
    bipartite_vertex_level[v] = k / order;
  }

private:
  T count;
  T order;
  dynamic_array<T>& bipartite_vertex_level;
};

/*! \brief The walk-based algorithm of Berry, Hendrickson, Kahan, and Konecny,
           Cray User Group 2006.

    \param bigG The big graph that is to be searched.
    \param target The target or pattern graph that is to be matched in the
                  big graph.
    \param walk_ids The ids of the walk through the target graph.  The
                    sequence is (vertex id, edge id, vertex id, ...
                    vertex_id, edge id, vertex id).
    \param length Length of the array walk_ids.  This equals the number of
                  vertices in the walk + the number of edges in the walk.
                  This is equal to the number of verts * 2 - 1.
    \param result An array of the edges from bigG where each entry indicates
                  which component the edge belongs to. -1 means the edge
                  belongs to no component.  Edge with the same number belong
                  to the same component.
    \param si_compare The function object comparator used to customize the
                      matching.
    \param uvcf Indicates if the vert count filter should be turned on or off.
                The default is off.
*/
template <typename graph_adapter, typename si_comparator,
          typename graph_adapter_trg>
void subgraph_isomorphism(
  graph_adapter& bigG,
  graph_adapter_trg& targetG,
  typename graph_traits<graph_adapter_trg>::size_type* walk_ids,
  typename graph_traits<graph_adapter_trg>::size_type length,
  typename graph_traits<graph_adapter>::size_type* result,
  si_comparator& si_compare)
{
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge;
  typedef typename graph_traits<graph_adapter>::size_type size_type;
  typedef typename graph_traits<graph_adapter>::edge_iterator edge_iterator;

  typedef typename graph_traits<graph_adapter_trg>::vertex_descriptor
          vertex_trg;
  typedef typename graph_traits<graph_adapter_trg>::edge_descriptor edge_trg;
  typedef typename graph_traits<graph_adapter_trg>::size_type size_type_trg;
  typedef typename graph_traits<graph_adapter_trg>::vertex_iterator
          vertex_iterator_trg;
  typedef typename graph_traits<graph_adapter_trg>::edge_iterator
          edge_iterator_trg;

  typedef static_graph_adapter<directedS> bipartite_adapter;
  typedef typename graph_traits<bipartite_adapter>::size_type size_type_s;
  typedef array_property_map<size_type_s, vertex_id_map<bipartite_adapter> >
          vertex_property_map_s;
  typedef typename graph_traits<bipartite_adapter>::vertex_descriptor
          vertex_s;
  typedef typename graph_traits<bipartite_adapter>::edge_descriptor edge_s;

  size_type bigG_order = num_vertices(bigG);
  size_type bigG_size = num_edges(bigG);

  // Get memory for and initialize activeVerts which is a representation of
  // the vertices in the bipartite graph.
  bool** activeVerts = new bool*[(length + 1) / 2];
  #pragma mta assert nodep
  for (size_type_trg i = 0; i < (length + 1) / 2; ++i)
  {
    activeVerts[i] = new bool[bigG_order];
  }
  for (size_type_trg i = 0; i < (length + 1) / 2; ++i)
  {
    for (size_type j = 0; j < bigG_order; ++j)
    {
      activeVerts[i][j] = false;
    }
  }

  // Get the vertex and edge id maps.
  vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, bigG);
  vertex_id_map<graph_adapter_trg> vid_map_trg = get(_vertex_id_map, targetG);
  edge_id_map<graph_adapter> eid_map = get(_edge_id_map, bigG);
  edge_id_map<graph_adapter_trg> eid_map_trg = get(_edge_id_map, targetG);

  // 1. Initialize activeVerts and count the bipartite edges for the first
  //    walk edge.

  size_type b_size = 0;

  edge_iterator bg_edges = edges(bigG);
  edge_iterator_trg tg_edges = edges(targetG);
  vertex_iterator_trg tg_verts = vertices(targetG);

  edge_trg trg_e = tg_edges[walk_ids[1]];
  vertex_trg trg_walk_src = tg_verts[walk_ids[0]];

  // Determine if the target walk edge is in the forward or reversed
  // direction.  If the source vertex in the walk is the source of the
  // target edge, then the target edge is a forward edge.  Otherwise, the
  // target edge is a backward edge.
  size_type_trg trg_walk_src_id = get(vid_map_trg, trg_walk_src);
  size_type_trg trg_src_id = get(vid_map_trg, source(trg_e, targetG));
  bool trg_e_reversed = trg_walk_src_id != trg_src_id;

  // Add the unique vertices to the bipartite graph.
  #pragma mta assert parallel
  for (size_type i = 0; i < bigG_size; ++i)
  {
    edge e = bg_edges[i];

    // Get the "source" and "target" vertices for the big graph edge.  For
    // forward target edges, the "source" of the big graph edge is the edge's
    // actual source.  For backward target edges, the "source" of the big
    // graph edge is the edge's target.
    vertex bg_src = trg_e_reversed ? target(e, bigG) : source(e, bigG);
    vertex bg_dest = other(e, bg_src, bigG);

    // Get the vertex and edge ids.
    size_type bg_src_id = get(vid_map, bg_src);
    size_type bg_dest_id = get(vid_map, bg_dest);

    // Check if the first walk edge matches this edge from the big graph.
    if (si_compare(e, trg_e, bigG, targetG))
    {
      mt_incr(b_size, 1);

      activeVerts[0][bg_src_id] = true;
      activeVerts[1][bg_dest_id] = true;
    }

    // If the graph is undirected, also check if the reverse of this edge.
    if (is_undirected(bigG))
    {
      edge e_rev = edge(target(e, bigG), source(e, bigG), get(eid_map, e));

      if (si_compare(e_rev, trg_e, bigG, targetG))
      {
        mt_incr(b_size, 1);

        activeVerts[0][bg_dest_id] = true;
        activeVerts[1][bg_src_id] = true;
      }
    }
  }

  // 2. Initialize activeVerts and count the bipartite edges for the remaining
  //    walk edges.

  size_type activeLevel = 2;  // Level in bipartite graph.

  // Try to match the remainder of the walk through the target graph against
  // the filtered graph.
  for (size_type_trg j = 2; j < length - 2; j += 2)
  {
    edge_trg trg_e = tg_edges[walk_ids[j + 1]];
    vertex_trg trg_walk_src = tg_verts[walk_ids[j]];

    // Determine if the target walk edge is in the forward or reversed
    // direction.  If the source vertex in the walk is the source of the
    // target edge, then the target edge is a forward edge.  Otherwise, the
    // target edge is a backward edge.
    size_type_trg trg_walk_src_id = get(vid_map_trg, trg_walk_src);
    size_type_trg trg_src_id = get(vid_map_trg, source(trg_e, targetG));
    bool trg_e_reversed = trg_walk_src_id != trg_src_id;

    // Add the unique vertices to the bipartite graph.
    #pragma mta assert parallel
    for (size_type i = 0; i < bigG_size; ++i)
    {
      edge e = bg_edges[i];

      // Get the "source" and "target" vertices for the big graph edge.  For
      // forward target edges, the "source" of the big graph edge is the edge's
      // actual source.  For backward target edges, the "source" of the big
      // graph edge is the edge's target.
      vertex bg_src = trg_e_reversed ? target(e, bigG) : source(e, bigG);
      vertex bg_dest = other(e, bg_src, bigG);

      // Get the vertex and edge ids.
      size_type bg_src_id = get(vid_map, bg_src);
      size_type bg_dest_id = get(vid_map, bg_dest);

      // Check if the current walk edge matches this edge from the big graph.
      if (activeVerts[activeLevel - 1][bg_src_id] &&
          si_compare(e, trg_e, bigG, targetG))
      {
        mt_incr(b_size, 1);

        activeVerts[activeLevel][bg_dest_id] = true;
      }

      // If the graph is undirected, also check the reverse of this edge.
      if (is_undirected(bigG))
      {
        edge e_rev = edge(target(e, bigG), source(e, bigG), get(eid_map, e));

        if (activeVerts[activeLevel - 1][bg_dest_id] &&
            si_compare(e_rev, trg_e, bigG, targetG))
        {
          mt_incr(b_size, 1);

          activeVerts[activeLevel][bg_src_id] = true;
        }
      }
    }

    ++activeLevel;
  }

  // Create the data structures that will hold the bipartite vertex and edge
  // information.  We know the exact number of edges and estimate the number
  // of vertices as 2.2 * # edges.  There can't be more than 2 * # edges, and
  // the .2 guarantees the hash table has some empty space.
  xmt_hash_table<size_type, size_type>
    bipartite_vertex_id(static_cast<int>(2.2 * b_size));

  dynamic_array<size_type> b_sources;
  dynamic_array<size_type> b_dests;
  dynamic_array<size_type> b_eid_map;
  b_sources.resize(b_size);
  b_dests.resize(b_size);
  b_eid_map.resize(b_size);

  // 3. Keep track of the unique vertices and get the bipartite edges for the
  //    first walk edge.

  b_size = 0;

  trg_e = tg_edges[walk_ids[1]];
  trg_walk_src = tg_verts[walk_ids[0]];

  // Determine if the target walk edge is in the forward or reversed
  // direction.  If the source vertex in the walk is the source of the
  // target edge, then the target edge is a forward edge.  Otherwise, the
  // target edge is a backward edge.
  trg_walk_src_id = get(vid_map_trg, trg_walk_src);
  trg_src_id = get(vid_map_trg, source(trg_e, targetG));
  trg_e_reversed = trg_walk_src_id != trg_src_id;

  // Add the unique vertices to the bipartite graph.
  #pragma mta assert parallel
  #pragma mta assert nodep
  for (size_type i = 0; i < bigG_size; ++i)
  {
    edge e = bg_edges[i];

    // Get the "source" and "target" vertices for the big graph edge.  For
    // forward target edges, the "source" of the big graph edge is the edge's
    // actual source.  For backward target edges, the "source" of the big
    // graph edge is the edge's target.
    vertex bg_src = trg_e_reversed ? target(e, bigG) : source(e, bigG);
    vertex bg_dest = other(e, bg_src, bigG);

    // Get the vertex and edge ids.
    size_type bg_src_id = get(vid_map, bg_src);
    size_type bg_dest_id = get(vid_map, bg_dest);
    size_type e_id = get(eid_map, e);

    // Check if the first walk edge matches this edge from the big graph.
    if (si_compare(e, trg_e, bigG, targetG))
    {
      size_type b_eid = mt_incr(b_size, 1);

      b_sources[b_eid] = bg_src_id;
      b_dests[b_eid] = bigG_order + bg_dest_id;
      b_eid_map[b_eid] = e_id;

      bipartite_vertex_id.insert(bg_src_id, 0);
      bipartite_vertex_id.insert(bigG_order + bg_dest_id, 0);
    }

    // If the graph is undirected, also check if the reverse of this edge.
    if (is_undirected(bigG))
    {
      edge e_rev = edge(target(e, bigG), source(e, bigG), get(eid_map, e));

      if (si_compare(e_rev, trg_e, bigG, targetG))
      {
        size_type b_eid = mt_incr(b_size, 1);

        b_sources[b_eid] = bg_dest_id;
        b_dests[b_eid] = bigG_order + bg_src_id;
        b_eid_map[b_eid] = e_id;

        bipartite_vertex_id.insert(bg_dest_id, 0);
        bipartite_vertex_id.insert(bigG_order + bg_src_id, 0);
      }
    }
  }

  // 4. Keep track of the unique vertices and get the bipartite edges for the
  //    remaining walk edges.

  activeLevel = 2;  // Level in bipartite graph.

  // Try to match the remainder of the walk through the target graph against
  // the filtered graph.
  for (size_type_trg j = 2; j < length - 2; j += 2)
  {
    edge_trg trg_e = tg_edges[walk_ids[j + 1]];
    vertex_trg trg_walk_src = tg_verts[walk_ids[j]];

    // Determine if the target walk edge is in the forward or reversed
    // direction.  If the source vertex in the walk is the source of the
    // target edge, then the target edge is a forward edge.  Otherwise, the
    // target edge is a backward edge.
    size_type_trg trg_walk_src_id = get(vid_map_trg, trg_walk_src);
    size_type_trg trg_src_id = get(vid_map_trg, source(trg_e, targetG));
    bool trg_e_reversed = trg_walk_src_id != trg_src_id;

    // Add the unique vertices to the bipartite graph.
    #pragma mta assert parallel
    #pragma mta assert nodep
    for (size_type i = 0; i < bigG_size; ++i)
    {
      edge e = bg_edges[i];

      // Get the "source" and "target" vertices for the big graph edge.  For
      // forward target edges, the "source" of the big graph edge is the edge's
      // actual source.  For backward target edges, the "source" of the big
      // graph edge is the edge's target.
      vertex bg_src = trg_e_reversed ? target(e, bigG) : source(e, bigG);
      vertex bg_dest = other(e, bg_src, bigG);

      // Get the vertex and edge ids.
      size_type bg_src_id = get(vid_map, bg_src);
      size_type bg_dest_id = get(vid_map, bg_dest);
      size_type e_id = get(eid_map, e);

      // Check if the current walk edge matches this edge from the big graph.
      if (activeVerts[activeLevel - 1][bg_src_id] &&
          si_compare(e, trg_e, bigG, targetG))
      {
        size_type b_eid = mt_incr(b_size, 1);

        b_sources[b_eid] = (activeLevel - 1) * bigG_order + bg_src_id;
        b_dests[b_eid] = activeLevel * bigG_order + bg_dest_id;
        b_eid_map[b_eid] = e_id;

        bipartite_vertex_id.insert(activeLevel * bigG_order + bg_dest_id, 0);
      }

      // If the graph is undirected, also check the reverse of this edge.
      if (is_undirected(bigG))
      {
        edge e_rev = edge(target(e, bigG), source(e, bigG), get(eid_map, e));

        if (activeVerts[activeLevel - 1][bg_dest_id] &&
            si_compare(e_rev, trg_e, bigG, targetG))
        {
          size_type b_eid = mt_incr(b_size, 1);

          b_sources[b_eid] = (activeLevel - 1) * bigG_order + bg_dest_id;
          b_dests[b_eid] = activeLevel * bigG_order + bg_src_id;
          b_eid_map[b_eid] = e_id;

          bipartite_vertex_id.insert(activeLevel * bigG_order + bg_src_id, 0);
        }
      }
    }

    ++activeLevel;
  }

  size_type b_order = bipartite_vertex_id.size();
  dynamic_array<size_type> bipartite_vertex_level(b_order + 1);

  // Create the compressed unique ids for the bipartite vertices; set their
  // mappings back to the vertices in bigG; and set their levels.
  uniqueid_visitor<size_type> uidvis(bigG_order, bipartite_vertex_level);
  bipartite_vertex_id.visit(uidvis);

  // Replace the spaced out ids of b_sources and b_dests with the compressed
  // ids.
  #pragma mta assert nodep
  for (size_type i = 0; i < b_size; ++i)
  {
    size_type bsi;
    size_type bdi;

    bipartite_vertex_id.lookup(b_sources[i], bsi);
    bipartite_vertex_id.lookup(b_dests[i], bdi);

    b_sources[i] = bsi;
    b_dests[i] = bdi;
  }

  #pragma mta assert nodep
  for (size_type_trg i = 0; i < (length + 1) / 2; ++i)
  {
    delete [] activeVerts[i];
  }
  delete [] activeVerts;

  // Add entries for the new vertex in the enhanced bipartite graph.
  bipartite_vertex_level[b_order] = activeLevel;

  // Find the number of max level vertices.
  size_type num_max_level_verts = 0;
  for (size_type i = 0; i < b_order; ++i)
  {
    if (bipartite_vertex_level[i] == activeLevel - 1) num_max_level_verts += 1;
  }

  // Initialize result such that all edges belong to no component.
  for (size_type i = 0; i < bigG_size; ++i)
  {
    result[i] = std::numeric_limits<size_type>::max();
  }

  if (num_max_level_verts == 0) return;

  size_type* max_level_vertices =
    (size_type*) malloc(num_max_level_verts * sizeof(size_type));

  num_max_level_verts = 0;
  #pragma mta assert nodep
  for (size_type i = 0; i < b_order; ++i)
  {
    if (bipartite_vertex_level[i] == activeLevel - 1)
    {
      size_type pos = mt_incr(num_max_level_verts, 1);
      max_level_vertices[pos] = i;
    }
  }

  // Get the number of vertices and edges for the enhanced biparatite graph.
  size_type b_order2 = b_order + 1;
  size_type b_size2 = b_size + num_max_level_verts;

  b_sources.resize(b_size2);
  b_dests.resize(b_size2);
  b_eid_map.resize(b_size2);

  #pragma mta assert nodep
  for (size_type i = 0; i < num_max_level_verts; ++i)
  {
    b_sources[b_size + i] = max_level_vertices[i];
    b_dests[b_size + i] = b_order;
    b_eid_map[b_size + i] = std::numeric_limits<size_type>::max();
  }

  // Flip the sources and destinations because we want the directed edges
  // going from the end of the match to the beginning.  That way we don't have
  // to worry about levels.  Moving forward an edge in the graph moves forward
  // one level.
  bipartite_adapter bipartite_graph;
  init(b_order2, b_size2, b_dests.get_data(), b_sources.get_data(),
       bipartite_graph);

  size_type* b_edge_color = (size_type*) malloc(b_size2 * sizeof(size_type));
  #pragma mta assert nodep
  for (unsigned int i = 0; i < b_size2; ++i)  b_edge_color[i] = 0;

  int* search_color = (int*) malloc(b_order2 * sizeof(int));
  #pragma mta assert nodep
  for (unsigned int i = 0; i < b_order2; ++i)  search_color[i] = 0;

  iso_bipartite_visitor<bipartite_adapter> ibv(bipartite_graph, b_edge_color);

  psearch<bipartite_adapter, int*, iso_bipartite_visitor<bipartite_adapter>,
          AND_FILTER, DIRECTED> psrch(bipartite_graph, search_color, ibv);
  psrch.run(get_vertex(b_order, bipartite_graph));

  // Count the number of colored edges.
  size_type num_subgraph_edges = 0;
  for (size_type i = 0; i < b_size; ++i) num_subgraph_edges += b_edge_color[i];

  // Create list of colored edges but converted back into their bigG
  // counterparts.  This will probably include many duplicate edges from bigG,
  // but subgraph::init_edges() can handle duplicates.
  edge* subgraph_edges = (edge*) malloc(num_subgraph_edges * sizeof(edge));
  num_subgraph_edges = 0;
  #pragma mta assert nodep
  for (size_type i = 0; i < b_size; ++i)
  {
    if (b_edge_color[i])
    {
      size_type j = mt_incr(num_subgraph_edges, 1);

      subgraph_edges[j] = bg_edges[b_eid_map[i]];
    }
  }

  typedef subgraph_adapter<graph_adapter> Subgraph;
  typedef typename graph_traits<Subgraph>::edge_descriptor edge_sg;
  typedef typename graph_traits<Subgraph>::size_type size_type_sg;
  typedef typename graph_traits<Subgraph>::vertex_descriptor
          vertex_descriptor_sg;
  typedef typename graph_traits<Subgraph>::vertex_iterator vertex_iterator_sg;
  typedef typename graph_traits<Subgraph>::edge_iterator edge_iterator_sg;

  typedef array_property_map<vertex_descriptor_sg, vertex_id_map<Subgraph> >
          component_map;

  // Create the subgraph of bigG induced by the colored edges.
  Subgraph component_graph(bigG);
  init_edges(subgraph_edges, num_subgraph_edges, component_graph);
  size_type cg_order = num_vertices(component_graph);
  size_type cg_size = num_edges(component_graph);

  vertex_id_map<Subgraph> vid_map_sg = get(_vertex_id_map, component_graph);

  vertex_iterator_sg cg_verts = vertices(component_graph);
  edge_iterator_sg cg_edges = edges(component_graph);

  vertex_descriptor_sg* subgraph_result =
    (vertex_descriptor_sg*) malloc(cg_order * sizeof(vertex_descriptor_sg));

  for (size_type_sg i = 0; i < cg_order; ++i)
  {
    subgraph_result[i] = cg_verts[i];
  }
  component_map components(subgraph_result, vid_map_sg);

  connected_components_sv<Subgraph, component_map>
    cc(component_graph, components);
  cc.run();

  // Copy the components from the subgraph to the result.
  #pragma mta assert nodep
  #pragma mta assert noalias *result, *subgraph_result
  for (size_type i = 0; i < cg_size; ++i)
  {
    edge_sg e = cg_edges[i];
    size_type_sg sid = get(vid_map_sg, source(e, component_graph));
    size_type bigG_eid = get(eid_map, component_graph.local_to_global(e));

    result[bigG_eid] = get(vid_map_sg, subgraph_result[sid]);
  }

  free(max_level_vertices);
  free(b_edge_color);
  free(search_color);
  free(subgraph_edges);
  free(subgraph_result);
}

}

#endif

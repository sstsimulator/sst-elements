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
/*! \file subgraph_adapter.hpp

    \brief This adapter provides an interface to create a subgraph of
           an existing graph adapter.  It stores a reference to the original
           graph and an entire copy of the subgraph.  The subgraph could
           possibly be stored more efficiently, but storing a full copy of the
           subgraph allows this to work with any graph adapter that follows
           the standard interface.

    \author Greg Mackey (gemacke@sandia.gov)

    \date 3/17/2008

    The associations between the vertices and edges in the original graph and
    the vertices and edges in the subgraph are stored explicitly.  Subgraph to
    original graph associations are stored using dynamic arrays.  Original
    graph to subgraph associations are stored using hash tables to reduce the
    memory usage while keeping almost constant access times.

    This adapter produces deterministic subgraphs in structure, but the ids
    of the local edges may change between parallel runnings.

    The base_adapter_type is expected to correctly implement a deep copy
    for both the copy constructor and the assignment operator.
*/
/****************************************************************************/

#ifndef MTGL_SUBGRAPH_ADAPTER_HPP
#define MTGL_SUBGRAPH_ADAPTER_HPP

#include <cassert>

#include <mtgl/mtgl_adapter.hpp>
#include <mtgl/dynamic_array.hpp>
#include <mtgl/xmt_hash_table.hpp>
#include <mtgl/static_graph_adapter.hpp>

namespace mtgl {

template <typename graph_adapter>
class subgraph_adapter {
public:
  typedef graph_traits<graph_adapter> base_traits;
  typedef typename base_traits::size_type base_size_type;
  typedef typename base_traits::vertex_descriptor base_vertex_descriptor;
  typedef typename base_traits::edge_descriptor base_edge_descriptor;
  typedef typename base_traits::vertex_iterator base_vertex_iterator;
  typedef typename base_traits::adjacency_iterator base_adjacency_iterator;
  typedef typename base_traits::edge_iterator base_edge_iterator;
  typedef typename base_traits::out_edge_iterator base_out_edge_iterator;
  typedef typename base_traits::directed_category base_directed_category;

  typedef static_graph_adapter<base_directed_category> wrapper_adapter;
  typedef graph_traits<wrapper_adapter> traits;
  typedef typename traits::size_type size_type;
  typedef typename traits::vertex_descriptor vertex_descriptor;
  typedef typename traits::edge_descriptor edge_descriptor;
  typedef typename traits::vertex_iterator vertex_iterator;
  typedef typename traits::adjacency_iterator adjacency_iterator;
  typedef typename traits::in_adjacency_iterator in_adjacency_iterator;
  typedef typename traits::edge_iterator edge_iterator;
  typedef typename traits::out_edge_iterator out_edge_iterator;
  typedef typename traits::in_edge_iterator in_edge_iterator;
  typedef typename traits::directed_category directed_category;
  typedef typename traits::graph_type graph_type;

  subgraph_adapter(graph_adapter& g) : original_graph(&g) {}
  subgraph_adapter(const subgraph_adapter& sg) { deep_copy(sg); }

  subgraph_adapter& operator=(const subgraph_adapter& rhs)
  {
    deep_copy(rhs);

    return *this;
  }

  graph_type* get_graph() const { return subgraph.get_graph(); }
  const wrapper_adapter& get_adapter() const { return subgraph; }
  graph_adapter& get_original_adapter() const { return *original_graph; }

  vertex_descriptor global_to_local(base_vertex_descriptor u_global) const
  {
    size_type u_local_id;

    bool in_subgraph = m_local_vertex.lookup(
           get(get(_vertex_id_map, *original_graph), u_global), u_local_id);

    assert(in_subgraph == true);

    return get_vertex(u_local_id, subgraph);
  }

  base_vertex_descriptor local_to_global(vertex_descriptor u_local) const
  {
    base_size_type u_global_id =
      m_global_vertex[get(get(_vertex_id_map, subgraph), u_local)];

    return get_vertex(u_global_id, *original_graph);
  }

  edge_descriptor global_to_local(base_edge_descriptor e_global) const
  {
    size_type e_local_id;

    bool in_subgraph = m_local_edge.lookup(
        get(get(_edge_id_map, *original_graph), e_global), e_local_id);

    assert(in_subgraph == true);

    return get_edge(e_local_id, subgraph);
  }

  base_edge_descriptor local_to_global(edge_descriptor e_local)
  {
    base_size_type e_global_id =
      m_global_edge[get(get(_edge_id_map, subgraph), e_local)];

    return get_edge(e_global_id, *original_graph);
  }

  void print()
  {
    mtgl::print(subgraph);

    printf("\n");
    printf("m_global_vertex: %d\n", m_global_vertex.size());
    for (int i = 0; i < m_global_vertex.size(); ++i)
    {
      printf("  %d -> %d\n", i, m_global_vertex[i]);
    }

    printf("\n");
    printf("m_local_vertex: %d\n", m_local_vertex.size());
    for (int i = 0; i < m_local_vertex.size(); ++i)
    {
      size_type local;
      int global_id = m_global_vertex[i];
      m_local_vertex.lookup(global_id, local);

      printf("  %d -> %d\n", global_id, local);
    }

    printf("\n");
    printf("m_global_edge: %d\n", m_global_edge.size());
    for (int i = 0; i < m_global_edge.size(); ++i)
    {
      printf("  %d -> %d\n", i, m_global_edge[i]);
    }

    printf("\n");
    printf("m_local_edge: %d\n", m_local_edge.size());
    for (int i = 0; i < m_local_edge.size(); ++i)
    {
      size_type local;
      int global_id = m_global_edge[i];
      m_local_edge.lookup(global_id, local);

      printf("  %d -> %d\n", global_id, local);
    }
  }

  class uniqueid_visitor {
  public:
    uniqueid_visitor(dynamic_array<base_size_type>& g) :
      count(0), global(g) {}

    void operator()(const base_size_type& k, size_type& v)
    {
      v = mt_incr(count, 1);
      global[v] = k;
    }

  private:
    size_type count;
    dynamic_array<base_size_type>& global;
  };

  // Creates an edge induced subgraph.  We assume that emask is the size of
  // original_graph.
  void internal_init_edges(bool* emask)
  {
    // Get the vertex and edge id maps.
    vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, *original_graph);
    edge_id_map<graph_adapter> eid_map = get(_edge_id_map, *original_graph);

    base_edge_iterator edgs = edges(*original_graph);
    base_size_type num_global_edges = num_edges(*original_graph);

    // Count the number of edges.
    size_type num_local_edges = 0;
    for (size_type i = 0; i < num_global_edges; ++i)
    {
      num_local_edges += emask[i];
    }

    m_local_vertex.resize(2 * num_local_edges);
    m_local_edge.resize((size_type) (1.7 * num_local_edges));

    // Create the unique sets of vertices and edges and set them as the
    // local vertices and edges.
    #pragma mta assert parallel
    for (size_type i = 0; i < num_global_edges; ++i)
    {
      if (emask[i])
      {
        base_edge_descriptor e_global = edgs[i];
        base_size_type sid = get(vid_map, source(e_global, *original_graph));
        base_size_type did = get(vid_map, target(e_global, *original_graph));

        // Add the source and destination vertex ids to m_local_vertex.
        m_local_vertex.insert(sid,0);
        m_local_vertex.insert(did,0);

        // Add the edge ids to m_local_edge.
        m_local_edge.insert(i,0);
      }
    }

    size_type num_unique_verts = m_local_vertex.size();
    size_type num_unique_edges = m_local_edge.size();

    // Create unique ids for the local vertices, and initialize
    // m_global_vertex.
    m_global_vertex.resize(num_unique_verts);
    uniqueid_visitor v_uidvis(m_global_vertex);
    m_local_vertex.visit(v_uidvis);

    // Create unique ids for the local edges, and initialize m_global_edge.
    m_global_edge.resize(num_unique_edges);
    uniqueid_visitor e_uidvis(m_global_edge);
    m_local_edge.visit(e_uidvis);

    // Create the arrays to hold the local edge sources and dests.  These will
    // be passed to subgraph's init().
    size_type* sources = new size_type[num_unique_edges];
    size_type* dests = new size_type[num_unique_edges];

    #pragma mta assert parallel
    for (size_type i = 0; i < num_unique_edges; ++i)
    {
      base_edge_descriptor e_global = edgs[m_global_edge[i]];

      base_size_type sid = get(vid_map, source(e_global, *original_graph));
      base_size_type did = get(vid_map, target(e_global, *original_graph));

      m_local_vertex.lookup(sid,sources[i]);
      m_local_vertex.lookup(did,dests[i]);
    }

    init(num_unique_verts, num_unique_edges, sources, dests, subgraph);

    delete [] sources;
    delete [] dests;
  }

  // Creates an edge induced subgraph.
  void internal_init_edges(base_size_type numEdges,
                           base_edge_descriptor* e_globals)
  {
    // Get the vertex and edge id maps.
    vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, *original_graph);
    edge_id_map<graph_adapter> eid_map = get(_edge_id_map, *original_graph);

    m_local_vertex.resize(2 * numEdges);
    m_local_edge.resize((size_type) (1.7 * numEdges));

    // Create the unique sets of vertices and edges and set them as the
    // local vertices and edges.
    #pragma mta assert parallel
    for (base_size_type i = 0; i < numEdges; ++i)
    {
      base_size_type sid = get(vid_map, source(e_globals[i], *original_graph));
      base_size_type did = get(vid_map, target(e_globals[i], *original_graph));
      base_size_type eid = get(eid_map, e_globals[i]);

      // Add the source and destination vertex ids to m_local_vertex.
      m_local_vertex.insert(sid,0);
      m_local_vertex.insert(did,0);

      // Add the edge ids to m_local_edge.
      m_local_edge.insert(eid,0);
    }

    size_type num_unique_verts = m_local_vertex.size();
    size_type num_unique_edges = m_local_edge.size();

    // Create unique ids for the local vertices, and initialize
    // m_global_vertex.
    m_global_vertex.resize(num_unique_verts);
    uniqueid_visitor v_uidvis(m_global_vertex);
    m_local_vertex.visit(v_uidvis);

    // Create unique ids for the local edges, and initialize m_global_edge.
    m_global_edge.resize(num_unique_edges);
    uniqueid_visitor e_uidvis(m_global_edge);
    m_local_edge.visit(e_uidvis);

    // Create the arrays to hold the local edge sources and dests.  These will
    // be passed to subgraph's init().
    size_type* sources = new size_type[num_unique_edges];
    size_type* dests = new size_type[num_unique_edges];

    base_edge_iterator edgs = edges(*original_graph);

    #pragma mta assert parallel
    for (size_type i = 0; i < num_unique_edges; ++i)
    {
      base_edge_descriptor e = edgs[m_global_edge[i]];

      base_size_type sid = get(vid_map, source(e, *original_graph));
      base_size_type did = get(vid_map, target(e, *original_graph));

      m_local_vertex.lookup(sid,sources[i]);
      m_local_vertex.lookup(did,dests[i]);
    }

    init(num_unique_verts, num_unique_edges, sources, dests, subgraph);

    delete [] sources;
    delete [] dests;
  }

  // Creates a vertex induced subgraph.
  void internal_init_vertices(bool* vmask)
  {
    // Get the vertex and edge id maps.
    vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, *original_graph);
    edge_id_map<graph_adapter> eid_map = get(_edge_id_map, *original_graph);

    base_vertex_iterator verts = vertices(*original_graph);
    base_size_type num_global_vertices = num_vertices(*original_graph);

    // Count the number of vertices.
    size_type num_local_vertices = 0;
    for (base_size_type i = 0; i < num_global_vertices; ++i)
    {
      num_local_vertices += vmask[i];
    }

    m_local_vertex.resize((size_type) (1.7 * num_local_vertices));

    // Create the unique set of vertices and set them as the local vertices.
    #pragma mta assert parallel
    for (base_size_type i = 0; i < num_global_vertices; ++i)
    {
      if (vmask[i])
      {
        base_vertex_descriptor v = verts[i];
        base_size_type vid = get(vid_map, v);

        // Add the vertex id to m_local_vertex.
        m_local_vertex.insert(vid, 0);
      }
    }

    // Count the number of unique edges.  Only count edges that have both
    // endpoints in v_globals.
    size_type num_local_edges = 0;
    for (base_size_type i = 0; i < num_global_vertices; ++i)
    {
      if (vmask[i])
      {
        base_vertex_descriptor u = verts[i];

        base_adjacency_iterator adj_iter =
          adjacent_vertices(u, *original_graph);
        base_size_type out_deg = out_degree(u, *original_graph);
        for (base_size_type j = 0; j < out_deg; ++j)
        {
          base_vertex_descriptor v = adj_iter[j];
          base_size_type vid = get(vid_map, v);

          num_local_edges += vmask[vid];
        }
      }
    }

    m_local_edge.resize((size_type) (1.7 * num_local_edges));

    // Create the unique set of edges and set them as the local edges.  Only
    // add edges that have both endpoints in v_globals.
    #pragma mta assert parallel
    for (base_size_type i = 0; i < num_global_vertices; ++i)
    {
      if (vmask[i])
      {
        base_vertex_descriptor u = verts[i];

        base_out_edge_iterator oe_iter = out_edges(u, *original_graph);
        base_size_type out_deg = out_degree(u, *original_graph);
        #pragma mta assert parallel
        for (size_type j = 0; j < out_deg; ++j)
        {
          base_edge_descriptor e = oe_iter[j];
          base_size_type vid = get(vid_map, target(e, *original_graph));

          // Only add this edge if its target is in vmask.  We already know
          // that the source is in vmask.
          if (vmask[vid])
          {
            base_size_type eid = get(eid_map, e);
            m_local_edge.insert(eid, 0);
          }
        }
      }
    }

    size_type num_unique_verts = m_local_vertex.size();
    size_type num_unique_edges = m_local_edge.size();

    // Create unique ids for the local vertices, and initialize
    // m_global_vertex.
    m_global_vertex.resize(num_unique_verts);
    uniqueid_visitor v_uidvis(m_global_vertex);
    m_local_vertex.visit(v_uidvis);

    // Create unique ids for the local edges, and initialize m_global_edge.
    m_global_edge.resize(num_unique_edges);
    uniqueid_visitor e_uidvis(m_global_edge);
    m_local_edge.visit(e_uidvis);

    // Create the arrays to hold the local edge sources and dests.  These will
    // be passed to subgraph's init().
    size_type* sources = new size_type[num_unique_edges];
    size_type* dests = new size_type[num_unique_edges];

    base_edge_iterator edgs = edges(*original_graph);

    #pragma mta assert parallel
    for (size_type i = 0; i < num_unique_edges; ++i)
    {
      base_edge_descriptor e = edgs[m_global_edge[i]];

      base_size_type sid = get(vid_map, source(e, *original_graph));
      base_size_type did = get(vid_map, target(e, *original_graph));

      assert(m_local_vertex.lookup(sid, sources[i]));
      assert(m_local_vertex.lookup(did, dests[i]));
    }

    init(num_unique_verts, num_unique_edges, sources, dests, subgraph);

    delete [] sources;
    delete [] dests;
  }

  // Creates a vertex induced subgraph.
  void internal_init_vertices(base_size_type numVertices,
                              base_vertex_descriptor* v_globals)
  {
    // Get the vertex and edge id maps.
    vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, *original_graph);
    edge_id_map<graph_adapter> eid_map = get(_edge_id_map, *original_graph);

    m_local_vertex.resize((size_type) (1.7 * numVertices));

    // Create the unique set of vertices and set them as the local vertices.
    #pragma mta assert parallel
    for (base_size_type i = 0; i < numVertices; ++i)
    {
      base_size_type vid = get(vid_map, v_globals[i]);

      // Add the vertex id to m_local_vertex.
      m_local_vertex.insert(vid,0);
    }

    // Count the number of unique edges.  Only count edges that have both
    // endpoints in v_globals.
    size_type num_local_edges = 0;
    #pragma mta assert parallel
    for (size_type i = 0; i < numVertices; ++i)
    {
      base_adjacency_iterator adj_iter = adjacent_vertices(v_globals[i],
                                                           *original_graph);
      base_size_type out_deg = out_degree(v_globals[i], *original_graph);
      #pragma mta assert parallel
      for (base_size_type j = 0; j < out_deg; ++j)
      {
        base_vertex_descriptor v = adj_iter[j];
        base_size_type vid = get(vid_map, v);

        num_local_edges += m_local_vertex.member(vid);
      }
    }

    m_local_edge.resize((size_type) (1.7 * num_local_edges));

    // Create the unique set of edges and set them as the local edges.  Only
    // add edges that have both endpoints in v_globals.
    #pragma mta assert parallel
    for (base_size_type i = 0; i < numVertices; ++i)
    {
      base_out_edge_iterator oe_iter = out_edges(v_globals[i], *original_graph);
      base_size_type out_deg = out_degree(v_globals[i], *original_graph);
      #pragma mta assert parallel
      for (base_size_type j = 0; j < out_deg; ++j)
      {
        base_edge_descriptor e = oe_iter[j];
        base_size_type vid = get(vid_map, target(e, *original_graph));

        // Only add this edge if its target is in v_globals.  We already know
        // that the source is in v_globals.
        if (m_local_vertex.member(vid))
        {
          base_size_type eid = get(eid_map, e);
          m_local_edge.insert(eid,0);
        }
      }
    }

    size_type num_unique_verts = m_local_vertex.size();
    size_type num_unique_edges = m_local_edge.size();

    // Create unique ids for the local vertices, and initialize
    // m_global_vertex.
    m_global_vertex.resize(num_unique_verts);
    uniqueid_visitor v_uidvis(m_global_vertex);
    m_local_vertex.visit(v_uidvis);

    // Create unique ids for the local edges, and initialize m_global_edge.
    m_global_edge.resize(num_unique_edges);
    uniqueid_visitor e_uidvis(m_global_edge);
    m_local_edge.visit(e_uidvis);

    // Create the arrays to hold the local edge sources and dests.  These will
    // be passed to subgraph's init().
    size_type* sources = new size_type[num_unique_edges];
    size_type* dests = new size_type[num_unique_edges];

    base_edge_iterator edgs = edges(*original_graph);

    #pragma mta assert parallel
    for (size_type i = 0; i < num_unique_edges; ++i)
    {
      base_edge_descriptor e = edgs[m_global_edge[i]];

      base_size_type sid = get(vid_map, source(e, *original_graph));
      base_size_type did = get(vid_map, target(e, *original_graph));

      m_local_vertex.lookup(sid, sources[i]);
      m_local_vertex.lookup(did, dests[i]);
    }

    init(num_unique_verts, num_unique_edges, sources, dests, subgraph);

    delete [] sources;
    delete [] dests;
  }

private:
  void deep_copy(const subgraph_adapter& rhs)
  {
    original_graph = rhs.original_graph;
    subgraph = rhs.subgraph;

    // Since the local <-> global links use ids, simply copying the data
    // structures will work.
    m_global_vertex = rhs.m_global_vertex;
    m_global_edge = rhs.m_global_edge;
    m_local_vertex = rhs.m_local_vertex;
    m_local_edge = rhs.m_local_edge;
  }

private:
  graph_adapter* original_graph;
  wrapper_adapter subgraph;
  dynamic_array<base_size_type> m_global_vertex;  // local -> global
  xmt_hash_table<base_size_type, size_type> m_local_vertex;  // global -> local
  dynamic_array<base_size_type> m_global_edge;  // local -> global
  xmt_hash_table<base_size_type, size_type> m_local_edge;  // global -> local
};

/***/

template <typename G>
inline
typename subgraph_adapter<G>::vertex_descriptor
get_vertex(typename subgraph_adapter<G>::size_type i,
           const subgraph_adapter<G>& sg)
{
  return get_vertex(i, sg.get_adapter());
}

/***/

template <typename G>
inline
typename subgraph_adapter<G>::edge_descriptor
get_edge(typename subgraph_adapter<G>::size_type i,
         const subgraph_adapter<G>& sg)
{
  return get_edge(i, sg.get_adapter());
}

/***/

template <typename G>
inline
typename subgraph_adapter<G>::size_type
num_vertices(const subgraph_adapter<G>& sg)
{
  return num_vertices(sg.get_adapter());
}

/***/

template <typename G>
inline
typename subgraph_adapter<G>::size_type
num_edges(const subgraph_adapter<G>& sg)
{
  return num_edges(sg.get_adapter());
}

/***/

template <typename G>
inline
typename subgraph_adapter<G>::vertex_descriptor
source(const typename subgraph_adapter<G>::edge_descriptor& e,
       const subgraph_adapter<G>& sg)
{
  return source(e, sg.get_adapter());
}

/***/

template <typename G>
inline
typename subgraph_adapter<G>::vertex_descriptor
target(const typename subgraph_adapter<G>::edge_descriptor& e,
       const subgraph_adapter<G>& sg)
{
  return target(e, sg.get_adapter());
}

/***/

template <typename G>
inline
typename subgraph_adapter<G>::vertex_descriptor
other(const typename subgraph_adapter<G>::edge_descriptor& e,
      const typename subgraph_adapter<G>::vertex_descriptor& u_local,
      const subgraph_adapter<G>& sg)
{
  return other(e, u_local, sg.get_adapter());
}

/***/

template <typename G>
inline
typename subgraph_adapter<G>::size_type
degree(const typename subgraph_adapter<G>::vertex_descriptor& u_local,
       const subgraph_adapter<G>& sg)
{
  return degree(u_local, sg.get_adapter());
}

/***/

template <typename G>
inline
typename subgraph_adapter<G>::size_type
out_degree(const typename subgraph_adapter<G>::vertex_descriptor& u_local,
           const subgraph_adapter<G>& sg)
{
  return out_degree(u_local, sg.get_adapter());
}

/***/

template <typename G>
inline
typename subgraph_adapter<G>::size_type
in_degree(const typename subgraph_adapter<G>::vertex_descriptor& u_local,
          const subgraph_adapter<G>& sg)
{
  return in_degree(u_local, sg.get_adapter());
}

/***/

template <typename G>
inline
typename subgraph_adapter<G>::vertex_iterator
vertices(const subgraph_adapter<G>& sg)
{
  return vertices(sg.get_adapter());
}

/***/

template <typename G>
inline
typename subgraph_adapter<G>::edge_iterator
edges(const subgraph_adapter<G>& sg)
{
  return edges(sg.get_adapter());
}

/***/

template <typename G>
inline
typename subgraph_adapter<G>::adjacency_iterator
adjacent_vertices(const typename subgraph_adapter<G>::vertex_descriptor& v,
                  const subgraph_adapter<G>& sg)
{
  return adjacent_vertices(v, sg.get_adapter());
}

/***/

template <typename G>
inline
typename subgraph_adapter<G>::in_adjacency_iterator
in_adjacent_vertices(const typename subgraph_adapter<G>::vertex_descriptor& v,
                     const subgraph_adapter<G>& sg)
{
  return in_adjacent_vertices(v, sg.get_adapter());
}

/***/

template <typename G>
inline
typename subgraph_adapter<G>::out_edge_iterator
out_edges(const typename subgraph_adapter<G>::vertex_descriptor& v,
          const subgraph_adapter<G>& sg)
{
  return out_edges(v, sg.get_adapter());
}

/***/

template <typename G>
inline
typename subgraph_adapter<G>::in_edge_iterator
in_edges(const typename subgraph_adapter<G>::vertex_descriptor& v,
         const subgraph_adapter<G>& sg)
{
  return in_edges(v, sg.get_adapter());
}

/***/

template <typename G>
inline
bool
is_directed(const subgraph_adapter<G>& sg)
{
  return is_directed(sg.get_adapter());
}

/***/

template <typename G>
inline
bool
is_undirected(const subgraph_adapter<G>& sg)
{
  return is_undirected(sg.get_adapter());
}

/***/

template <typename G>
inline
bool
is_bidirectional(const subgraph_adapter<G>& sg)
{
  return is_bidirectional(sg.get_adapter());
}

/***/

template <typename G>
class vertex_id_map<subgraph_adapter<G> > :
  public vertex_id_map<typename subgraph_adapter<G>::wrapper_adapter> {
public:
  typedef typename subgraph_adapter<G>::wrapper_adapter SG;

  vertex_id_map() : vertex_id_map<SG>() {}
  vertex_id_map(const vertex_id_map& vm) : vertex_id_map<SG>(vm) {}
};

/***/

template <typename G>
class edge_id_map<subgraph_adapter<G> > :
  public edge_id_map<typename subgraph_adapter<G>::wrapper_adapter> {
public:
  typedef typename subgraph_adapter<G>::wrapper_adapter SG;

  edge_id_map() : edge_id_map<SG>() {}
  edge_id_map(const edge_id_map& em) : edge_id_map<SG>(em) {}
};

/***/

template <typename G>
inline
void
init_edges(bool* emask, subgraph_adapter<G>& sg)
{
  sg.internal_init_edges(emask);
}

/***/

template <typename G>
inline
void
init_edges(typename subgraph_adapter<G>::edge_descriptor* e_globals,
           typename subgraph_adapter<G>::size_type numEdges,
           subgraph_adapter<G>& sg)
{
  sg.internal_init_edges(numEdges, e_globals);
}

/***/

template <typename G>
inline
void
init_vertices(bool* vmask, subgraph_adapter<G>& sg)
{
  sg.internal_init_vertices(vmask);
}

/***/

template <typename G>
inline
void
init_vertices(typename subgraph_adapter<G>::vertex_descriptor* v_globals,
              typename subgraph_adapter<G>::size_type numVertices,
              subgraph_adapter<G>& sg)
{
  sg.internal_init_vertices(numVertices, v_globals);
}

}

#endif

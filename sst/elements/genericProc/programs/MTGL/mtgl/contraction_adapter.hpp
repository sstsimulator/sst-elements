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
/*! \file contraction_adapter.hpp

    \author Greg Mackey (gemacke@sandia.gov)
    \author Jon Berry (jberry@sandia.gov)

    \date 5/1/2008
*/
/****************************************************************************/

#ifndef MTGL_CONTRACTION_ADAPTER_HPP
#define MTGL_CONTRACTION_ADAPTER_HPP

#include <cassert>
#include <map>
#include <set>

#include <mtgl/mtgl_adapter.hpp>
#include <mtgl/mtgl_boost_property.hpp>
#include <mtgl/dynamic_array.hpp>
#include <mtgl/xmt_hash_table.hpp>
#include <mtgl/xmt_hash_set.hpp>
#include <mtgl/static_graph_adapter.hpp>

namespace mtgl {

template <typename graph_adapter>
class contraction_adapter {
public:
  typedef graph_traits<graph_adapter> base_traits;
  typedef typename base_traits::size_type base_size_type;
  typedef typename base_traits::vertex_descriptor base_vertex_descriptor;
  typedef typename base_traits::edge_descriptor base_edge_descriptor;
  typedef typename base_traits::vertex_iterator base_vertex_iterator;
  typedef typename base_traits::adjacency_iterator base_adjacency_iterator;
  typedef typename base_traits::in_adjacency_iterator
          base_in_adjacency_iterator;
  typedef typename base_traits::edge_iterator base_edge_iterator;
  typedef typename base_traits::out_edge_iterator base_out_edge_iterator;
  typedef typename base_traits::in_edge_iterator base_in_edge_iterator;
  typedef typename base_traits::directed_category base_directed_category;
  typedef typename base_traits::graph_type base_graph_type;

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

  contraction_adapter(graph_adapter& g) :
    original_graph(&g), m_global_vertex(num_vertices(g)), m_edge_counter(0) {}

  // GM: Copy control is NOT implemented correctly for this class.  I am
  //     disallowing the copy contructor and assignment operator until
  //     someone implements it correctly.
private:
  contraction_adapter(const contraction_adapter& g) {}
  contraction_adapter& operator=(const contraction_adapter& rhs) {}

public:
  graph_type* get_graph() const { return contracted_graph.get_graph(); }
  const wrapper_adapter& get_adapter() const { return contracted_graph; }
  graph_adapter& get_original_adapter() const { return *original_graph; }

  bool node_sets_disjoint(
         dynamic_array<dynamic_array<vertex_descriptor> >& to_contract)
  {
    int total_count = 0;
    int sz = to_contract.size();

    #pragma mta assert parallel
    for (int i = 0; i < sz; i++)
    {
      mt_incr(total_count, to_contract[i].size());
    }

    size_type* buffer = new size_type[total_count];
    int index = 0;
    size_type max = 0;
    vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, *this);

    #pragma mta assert parallel
    for (int i = 0; i < sz; i++)
    {
      int sub_size = to_contract[i].size();
      for (int j = 0; j < sub_size; j++)
      {
        int ind = mt_incr(index, 1);
        buffer[ind] = get(vid_map, to_contract[i][j]);

        if (buffer[ind] > max) max = buffer[ind];
      }
    }

    countingSort(buffer, total_count, max);
    bool retval = true;

    #pragma mta assert nodep
    for (int i = 0; i < total_count - 1; i++)
    {
      if (buffer[i] == buffer[i + 1]) retval = false;
    }

    delete[] buffer;

    return (retval);
  }

  vertex_descriptor global_to_local(base_vertex_descriptor u_global) const
  {
    vertex_descriptor u_local;
    bool in_contracted_graph;
    tie(u_local, in_contracted_graph) = find_vertex(u_global);
    assert(in_contracted_graph == true);

    return u_local;
  }

  base_vertex_descriptor local_to_global(vertex_descriptor u_local) const
  {
    return m_global_vertex[get(get(_vertex_id_map, contracted_graph), u_local)];
  }

  pair<vertex_descriptor, bool>
  find_vertex(base_vertex_descriptor u_global) const;

// private:

  // For contraction, we will not check for duplicates here.  We don't keep
  // edge mappings.  As we build the graph, we'll check for duplicates before
  // calling this.
  edge_descriptor local_add_edge(vertex_descriptor u_local,
                                 vertex_descriptor v_local)
  {
    edge_descriptor e_local;
    bool inserted;

    vertex_id_map<graph_adapter> c_vid_map = get(_vertex_id_map,
                                                 contracted_graph);

    tie(e_local, inserted) = add_edge(u_local, v_local, contracted_graph);

    return e_local;
  }

  graph_adapter* original_graph;
  wrapper_adapter contracted_graph;
  int m_edge_counter;

  // This is a mapping from a local vertex ID to the vertex descriptor
  // of the leader of the group in the old world.  We can use the
  // vertex descriptor of the leader in the old (global) graph to look
  // up the set of vertices in the old graph.  CAP: TODO.  The set rep
  // hash table should probably be class data here.
  dynamic_array<base_vertex_descriptor> m_global_vertex;  // local -> global

  // CAP: TODO.  For contraction, this could be a vector.
  // global -> local
  std::map<base_vertex_descriptor, vertex_descriptor> m_local_vertex;
};

template <typename G>
inline
typename contraction_adapter<G>::vertex_descriptor
get_vertex(typename contraction_adapter<G>::size_type i,
           const contraction_adapter<G>& cg)
{
  return get_vertex(i, cg.get_adapter());
}

template <typename G>
inline
typename contraction_adapter<G>::edge_descriptor
get_edge(typename contraction_adapter<G>::size_type i,
         const contraction_adapter<G>& cg)
{
  return get_edge(i, cg.get_adapter());
}

template <typename G>
inline
typename contraction_adapter<G>::size_type
num_vertices(const contraction_adapter<G>& cg)
{
  return num_vertices(cg.get_adapter());
}

template <typename G>
inline
typename contraction_adapter<G>::size_type
num_edges(const contraction_adapter<G>& cg)
{
  return num_edges(cg.get_adapter());
}

template <typename G>
typename contraction_adapter<G>::vertex_descriptor
source(typename contraction_adapter<G>::edge_descriptor e,
       const contraction_adapter<G>& cg)
{
  return source(e, cg.get_adapter());
}

template <typename G>
typename contraction_adapter<G>::vertex_descriptor
target(typename contraction_adapter<G>::edge_descriptor e,
       const contraction_adapter<G>& cg)
{
  return target(e, cg.get_adapter());
}

template <typename G>
typename contraction_adapter<G>::vertex_descriptor
other(typename contraction_adapter<G>::edge_descriptor e,
      typename contraction_adapter<G>::vertex_descriptor u_local,
      const contraction_adapter<G>& cg)
{
  return other(e, u_local, cg.get_adapter());
}

template <typename G>
typename contraction_adapter<G>::size_type
degree(typename contraction_adapter<G>::vertex_descriptor u_local,
       const contraction_adapter<G>& cg)
{
  return degree(u_local, cg.contracted_graph);
}

template <typename G>
typename contraction_adapter<G>::size_type
out_degree(typename contraction_adapter<G>::vertex_descriptor u_local,
           const contraction_adapter<G>& cg)
{
  return out_degree(u_local, cg.contracted_graph);
}

template <typename G>
typename contraction_adapter<G>::size_type
in_degree(typename contraction_adapter<G>::vertex_descriptor u_local,
          const contraction_adapter<G>& cg)
{
  return in_degree(u_local, cg.contracted_graph);
}

template <typename G>
inline
typename contraction_adapter<G>::vertex_iterator
vertices(const contraction_adapter<G>& cg)
{
  return vertices(cg.get_adapter());
}

template <typename G>
inline
typename contraction_adapter<G>::edge_iterator
edges(const contraction_adapter<G>& cg)
{
  return edges(cg.get_adapter());
}

template <typename G>
inline
typename contraction_adapter<G>::adjacency_iterator
adjacent_vertices(const typename contraction_adapter<G>::vertex_descriptor& v,
                  const contraction_adapter<G>& cg)
{
  return adjacent_vertices(v, cg.get_adapter());
}

template <typename G>
inline
typename contraction_adapter<G>::in_adjacency_iterator
in_adjacent_vertices(
    const typename contraction_adapter<G>::vertex_descriptor& v,
    const contraction_adapter<G>& cg)
{
  return in_adjacent_vertices(v, cg.get_adapter());
}

template <typename G>
inline
typename contraction_adapter<G>::out_edge_iterator
out_edges(const typename contraction_adapter<G>::vertex_descriptor& v,
          const contraction_adapter<G>& cg)
{
  return out_edges(v, cg.get_adapter());
}

template <typename G>
inline
typename contraction_adapter<G>::in_edge_iterator
in_edges(const typename contraction_adapter<G>::vertex_descriptor& v,
         const contraction_adapter<G>& cg)
{
  return in_edges(v, cg.get_adapter());
}

template <typename G>
inline
bool
is_directed(const contraction_adapter<G>& cg)
{
  return is_directed(cg.get_adapter());
}

template <typename G>
inline
bool
is_undirected(const contraction_adapter<G>& cg)
{
  return is_undirected(cg.get_adapter());
}

template <typename G>
inline
bool
is_bidirectional(const contraction_adapter<G>& cg)
{
  return is_bidirectional(cg.get_adapter());
}

template <typename G>
class vertex_id_map<contraction_adapter<G> > :
  public vertex_id_map<typename contraction_adapter<G>::wrapper_adapter> {
public:
  typedef typename contraction_adapter<G>::wrapper_adapter CG;

  vertex_id_map() : vertex_id_map<CG>() {}
  vertex_id_map(const vertex_id_map& vm) : vertex_id_map<CG>(vm) {}
};

template <typename G>
class edge_id_map<contraction_adapter<G> > :
  public edge_id_map<typename contraction_adapter<G>::wrapper_adapter> {
public:
  typedef typename contraction_adapter<G>::wrapper_adapter CG;

  edge_id_map() : edge_id_map<CG>() {}
  edge_id_map(const edge_id_map& em) : edge_id_map<CG>(em) {}
};

template <typename G>
pair<typename contraction_adapter<G>::vertex_descriptor, bool>
contraction_adapter<G>::find_vertex(
  typename contraction_adapter<G>::base_vertex_descriptor u_global) const
{
  typename std::map<base_vertex_descriptor,
                    vertex_descriptor>::const_iterator i;
  i = m_local_vertex.find(u_global);

  bool valid = i != m_local_vertex.end();
  vertex_descriptor null;

  return pair<vertex_descriptor, bool>(valid ? (*i).second : null, valid);
}

// Greg's version
// This function add the edges in e_globals to the subgraph and then adds
// all vertex endpoints to the subgraph as well.
template <typename G>
void
add_edges(typename contraction_adapter<G>::size_type numEdges,
          typename contraction_adapter<G>::edge_descriptor* e_globals,
          contraction_adapter<G>& g)
{
  typedef typename contraction_adapter<G>::vertex_descriptor vertex_descriptor;
  typedef typename contraction_adapter<G>::size_type size_type;
  typedef typename contraction_adapter<G>::vertex_iterator vertex_iterator;

  vertex_id_map<G> vid_map = get(_vertex_id_map, *g.original_graph);

  // Get the unigue vertices to be added.
  std::map<size_type,int> unique_vertices;
  for (size_type i = 0; i < numEdges; ++i)
  {
    // Insert from and to vertices.
    unique_vertices[get(vid_map, source(e_globals[i],
                                               *g.original_graph))] = 1;
    unique_vertices[get(vid_map, target(e_globals[i],
                                               *g.original_graph))] = 1;
  }

  vertex_iterator verts = vertices(*g.original_graph);

  typename std::set<size_type,int>::iterator vIter = unique_vertices.begin();
  typename std::map<size_type,int>::iterator vIterEnd = unique_vertices.end();
  for ( ; vIter != vIterEnd; ++vIter)
  {
    // Add the vertex to the subgraph.
    vertex_descriptor u_global = verts[(*vIter).first];
    vertex_descriptor u_local = add_vertex(g.get_adapter());

    // Store the links between the subgraph vertex and the original vertex.
    g.m_global_vertex.push_back(u_global);
    g.m_local_vertex[u_global] = u_local;
  }

  // Add all the edges.
  for (size_type i = 0; i < numEdges; ++i)
  {
    // (local from vertex descriptor, local to vertex descriptor,
    //  global edge type)
    g.local_add_edge(g.global_to_local(e_globals[i].first),
                     g.global_to_local(e_globals[i].second),
                     e_globals[i]);
  }
}

// Greg's version
// This function add the vertices in u_globals to the subgraph and then adds
// all edges touching those vertices to the subgraph as well.
template <typename G>
void
add_vertices(typename contraction_adapter<G>::size_type numVerts,
             typename contraction_adapter<G>::vertex_descriptor* u_globals,
             contraction_adapter<G>& g)
{
  typedef typename contraction_adapter<G>::vertex_descriptor vertex_descriptor;
  typedef typename contraction_adapter<G>::edge_descriptor edge_descriptor;
  typedef typename contraction_adapter<G>::size_type size_type;
  typedef typename contraction_adapter<G>::out_edge_iterator out_edge_iterator;

  // Add all the vertices.
  for (size_type i = 0; i < numVerts; ++i)
  {
    // Add the vertex to the subgraph.
    vertex_descriptor u_local = add_vertex(g.get_adapter());

    // Store the links between the subgraph vertex and the original vertex.
    g.m_global_vertex.push_back(u_globals[i]);
    g.m_local_vertex[u_globals[i]] = u_local;
  }

  // Now, add all the edges.
  typename std::map<vertex_descriptor, vertex_descriptor>::iterator vIter =
    g.m_local_vertex.begin();
  typename std::map<vertex_descriptor, vertex_descriptor>::iterator vIterEnd =
    g.m_local_vertex.end();
  for ( ; vIter != vIterEnd; ++vIter)
  {
    size_type deg = out_degree(vIter->first, *g.original_graph);

    out_edge_iterator ei = out_edges(vIter->first, *g.original_graph);

    // Loop over all the incident edges of this vertex from the original
    // graph.
    for (size_type i = 0; i < deg; i++)
    {
      edge_descriptor e_global = ei[i];

      vertex_descriptor v_global = other(e_global, vIter->first, g);
      if (g.find_vertex(v_global).second == true)
      {
        g.local_add_edge(vIter->second, g.global_to_local(v_global),
                         e_global);
      }
    }
  }
}

// Our version.
// numVerts is the number of vertices in the current (old) graph.
// We'll take two representations of the contractions we wish to perform.
// The leader array is indexed by ID in the old graph (0 to numVerts - 1).
// leaders[i] gives the vertex descriptor for vertex i's leader (i will
// contract into a vertex represented by leaders[i] in the new graph).
// There's also an optional set representation.  This maps (via hashing) a
// leader in the old graph to an array containing all the vertices that will
// contract into that leader.

// Assume for now that the weight map key is an int64_t (low*order+high).

template <typename G, typename weight_map_t>
void
contract(typename contraction_adapter<G>::size_type numVerts,
         typename contraction_adapter<G>::size_type numEdges,
         weight_map_t* orig_ewghts, weight_map_t* contr_ewghts,
         typename graph_traits<G>::vertex_descriptor* leaders,
         contraction_adapter<G>& g,
         xmt_hash_table<typename contraction_adapter<G>::size_type,
                        dynamic_array<
           typename graph_traits<G>::vertex_descriptor>* >* set_rep = NULL)
{
  typedef typename contraction_adapter<G>::vertex_descriptor vertex_descriptor;
  typedef typename contraction_adapter<G>::edge_descriptor edge_descriptor;
  typedef typename contraction_adapter<G>::size_type size_type;
  typedef typename graph_traits<G>::vertex_iterator vertex_iterator;
  typedef typename graph_traits<G>::edge_iterator edge_iterator;

  weight_map_t& orig_eweights = *orig_ewghts;
  weight_map_t& contr_eweights = *contr_ewghts;

  bool we_own_set_rep = false;

  size_type* buffer;    // Sorted array of leader id's.
  int num_leaders;      // The number of contracted groups.
                        // Both of these two are used after the following
                        // if-then-else statement, yet only set in the
                        // if clause - need to fix if set rep is provided.

  vertex_id_map<G> vid_map = get(_vertex_id_map, *g.original_graph);

  vertex_iterator verts = vertices(*g.original_graph);

  if (set_rep == NULL)
  {
    buffer = new size_type[numVerts];
    size_type max = 0;

    // Find the leaders.  We sort the leader IDs (which are ints).
    // Whenever an element of the sorted array changes, we have found
    // a new leader.
    #pragma mta assert parallel
    for (size_type i = 0; i < numVerts; i++)
    {
      buffer[i] = get(vid_map, leaders[i]);

      // Need the max for the upcoming sort
      if (buffer[i] > max) max = buffer[i];
    }

    countingSort(buffer, numVerts, max);

    num_leaders = 1;

    // We don't yet know how many vertices the contracted graph will have,
    // so we will allocate space for this map incrementally.  This reserves
    // space for the first leader (in position 0 of the buffer array)
    g.m_global_vertex.buildIncr(); // Find correct alloc.

    we_own_set_rep = true;

    #pragma mta assert parallel
    for (size_type i = 0; i < numVerts - 1; i++)
    {
      if (buffer[i] != buffer[i + 1])
      {
        mt_incr(num_leaders, 1);
        g.m_global_vertex.buildIncr(); // Find correct alloc.
      }
    }

    g.m_global_vertex.endBuild(); // Do the allocation.

    set_rep = new xmt_hash_table<size_type, dynamic_array<vertex_descriptor>*>
                  (num_leaders);

    vertex_descriptor v = verts[buffer[0]];
    size_type vvid = get(vid_map, v);
    set_rep->insert(vvid, new dynamic_array<vertex_descriptor>);

    for (size_type i = 0; i < numVerts - 1; i++)
    {
//      printf("buffer[%d]: %d (%x)\n", i, buffer[i], verts[buffer[i]]);
      if (buffer[i] != buffer[i + 1])
      {
        vertex_descriptor v = verts[buffer[i + 1]];
        size_type vid = get(vid_map, v);
        set_rep->insert(vid, new dynamic_array<vertex_descriptor>);
      }
    }
  }
  else     // We're given the set representation hash table
  {
    // Consistency check and set up m_global_vertex based on the
    // number of sets passed in.
  }

  // Logic error: if set_rep given.  buffer is uninitialized.

  // Create a hash table that associates leaders from the old graph
  // with the list of (old) vertices to be contracted into the leader.

  // Here, we assume that id is going to be an index.
  // JWB:  Need to make this explicit in the MTGL or change the assumption.
  vertex_descriptor first_leader_global = verts[buffer[0]];
  vertex_descriptor first_local = add_vertex(g.get_adapter());

  vertex_id_map<G> c_vid_map = get(_vertex_id_map, g.get_adapter());
  bool* is_leader = (bool*) malloc(sizeof(bool) * numVerts);

  #pragma mta assert nodep
  for (size_type i = 0; i < numVerts; i++) is_leader[i] = false;

  g.m_global_vertex[0] = first_leader_global;
  g.m_local_vertex[first_leader_global] = first_local;
  size_type flg_id = get(vid_map, first_leader_global);

//  size_type fl_id  = get(c_vid_map, g.m_local_vertex[first_leader_global]);
//  printf("m_local_vertex[%d(%x)]: (%x) %d\n", flg_id, first_leader_global,
//         first_local, fl_id);

  is_leader[flg_id] = true;

  # pragma mta assert parallel
  for (size_type i = 0; i < numVerts - 1; ++i)
  {
    //printf("leaders[%d]: %x (%d)\n", i, leaders[i],get(vid_map,leaders[i]));

    if (we_own_set_rep)
    {
      if (buffer[i] != buffer[i + 1])
      {
        vertex_descriptor this_leader = verts[buffer[i + 1]];
        vertex_descriptor u_local = add_vertex(g.get_adapter());

        size_type local_id = get(c_vid_map, u_local);

        g.m_global_vertex[local_id] = this_leader;
        g.m_local_vertex[this_leader] = u_local;

        size_type flg_id = get(vid_map, this_leader);

        is_leader[flg_id] = true;

        //size_type fl_id  = get(c_vid_map, g.m_local_vertex[this_leader]);
        //printf("m_local_vertex[%d (%x)]: (%x) %d\n", flg_id,
        //       this_leader, u_local, fl_id);
      }
    }
    else
    {
      // We are in trouble -- we need to take the existing
      // set rep and complete our global vs. local maps.
    }
  }

  if (we_own_set_rep)
  {
    // Now populate the dynamic arrays in the hash table.
    #pragma mta assert parallel
    for (size_type i = 0; i < numVerts; i++)
    {
      vertex_descriptor this_vertex = verts[i];
      vertex_descriptor this_leader = leaders[i];
      vertex_descriptor local_v = g.m_local_vertex[this_leader];

      dynamic_array<vertex_descriptor>* this_group;

      size_type vid = get(vid_map, this_leader);

      bool success = set_rep->lookup(vid, this_group);
      assert(success);

      this_group->push_back(this_vertex);

      if (!is_leader[i])
      {
        g.m_local_vertex[this_vertex] = g.m_local_vertex[this_leader];
      }
    }
  }

  // Now, add all the edges.

  size_type order = num_vertices(*g.original_graph);
  size_type size = num_edges(*g.original_graph);

  edge_iterator iter = edges(*g.original_graph);

  // Keep the set of edges we've added to the contracted graph (as pairs
  // of vertices).  Need this to make sure we add a edge only once.
  xmt_hash_set<u_int64_t> current_edges;

  #pragma mta assert parallel
  #pragma mta interleave schedule
  for (size_type i = 0; i < size; i++)
  {
    edge_descriptor e = iter[i];

    vertex_descriptor src = source(e, *g.original_graph);
    vertex_descriptor tgt = target(e, *g.original_graph);
    vertex_descriptor new_src = g.m_local_vertex[src];
    vertex_descriptor new_tgt = g.m_local_vertex[tgt];

    size_type g_sid = get(vid_map, src);
    size_type g_tid = get(vid_map, tgt);
    size_type sid = get(c_vid_map, new_src);
    size_type tid = get(c_vid_map, new_tgt);

    order_pair(g_sid, g_tid);

    int64_t g_key = g_sid * (int64_t) order + g_tid;
    int64_t wgt = orig_eweights[g_key];

    //printf("weight(%d,%d): %d\n", g_sid, g_tid, (int) wgt);
    if (sid != tid)
    {
      // Check to see if this edge is already there.
      order_pair(sid, tid);
      int64_t key = sid * (int64_t) num_leaders + tid;
      bool already_there = current_edges.member(key);

      if (!already_there)
      {
        edge_descriptor local_edge = g.local_add_edge(new_src, new_tgt);
        current_edges.insert(key);
        contr_eweights[key] = 1;
      }
      else
      {
        //mt_incr((int64_t&) contr_eweights[key], (int64_t) 1);
        contr_eweights.update(key, contr_eweights[key] + (int64_t) 1);
      }
    }
  }

  delete [] buffer;
}

template <typename G>
typename contraction_adapter<G>::vertex_descriptor
add_vertex(typename contraction_adapter<G>::vertex_descriptor u_global,
           contraction_adapter<G>& g)
{
  typedef typename contraction_adapter<G>::vertex_descriptor vertex_descriptor;
  typedef typename contraction_adapter<G>::edge_descriptor edge_descriptor;
  typedef typename contraction_adapter<G>::size_type size_type;
  typedef typename contraction_adapter<G>::out_edge_iterator out_edge_iterator;

  vertex_descriptor u_local = add_vertex(g.get_adapter());

  g.m_global_vertex.push_back(u_global);
  g.m_local_vertex[u_global] = u_local;

  // Remember edge global and local maps.
  {
    out_edge_iterator ei = out_edges(u_global, *g.original_graph);
    size_type deg = out_degree(u_global, *g.original_graph);

    for (size_type i = 0; i < deg; ++i)
    {
      edge_descriptor e_global = ei[i];

      vertex_descriptor v_global = other(e_global, u_global, g);

      if (g.find_vertex(v_global).second == true)
      {
        g.local_add_edge(u_local, g.global_to_local(v_global), e_global);
      }
    }
  }

  // if (is_directed(g))  //  there's more in BGL's subgraph.hpp
}

}

#endif

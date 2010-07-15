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
/*! \file ull_state.hpp

    \author William McLendon (wcmclen@sandia.gov)

    \date 10/2/2009
*/
/****************************************************************************/

#ifndef MTGL_ULL_STATE_HPP
#define MTGL_ULL_STATE_HPP

#include <cstdio>
#include <limits>
#include <iomanip>

#include <mtgl/util.hpp>
#include <mtgl/psearch.hpp>
#include <mtgl/vf_isomorphism_base.hpp>

#ifdef __MTA__
#  include <sys/mta_task.h>
#  include <machine/runtime.h>
#endif

namespace mtgl {

#define DEBUG_is_feasible_pair 0
#define DEBUG_trace            0
#define DEBUG_print_graphs     0

/// \brief vf2 state class.  This class is used internally by the
///        vf2_isomorphism algorithm to maintain result states and
///        store results.
template <typename Graph>
class ull_state : public vf_default_state<Graph>{
public:
  typedef typename graph_traits<Graph>::size_type size_t;
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<Graph>::vertex_iterator vertex_iterator;

  ull_state(void);

  ull_state(Graph& g1, Graph& g2,
            vertex_id_map<Graph>& vm1, vertex_id_map<Graph>& vm2,
            edge_id_map<Graph>& em1,   edge_id_map<Graph>& em2,
            int search_type = VF2_SUBG_MONOMORPHISM,
            int search_termination = VF2_FIND_ONE);

  ull_state(ull_state<Graph>& state, bool deep_copy = false);

  ~ull_state();

  Graph& get_graph1()   { return graph1;  }
  Graph& get_graph2()   { return graph2;  }
  bool is_goal()        { return core_len == n1; }
  size_t get_core_len() { return core_len; }

  bool is_dead()
  {
    bool rval = false;

    if (n1 > n2) return true;

    #pragma mta assert parallel
    for (int i = core_len; i < n1; i++)
    {
      if (rval) continue;

      for (int j = 0; j < n2; j++)
      {
        if ( M[i][j] != 0 ) continue;
      }

      rval = true;
    }

    return rval;
  }

  void get_core_set(size_t c1[], size_t c2[]);

  ull_state<Graph>* ShallowCopy();
  ull_state<Graph>* DeepCopy();

  void back_track();

  bool next_pair(size_t* pn1, size_t* pn2, size_t prev_n1, size_t prev_n2);

  bool is_feasible_pair(size_t node1, size_t node2);

  void add_pair(size_t node1, size_t node2);

  void print_self()
  {
    cout << "vf2-state:" << endl;
    this->print_array(core_1, n1, "\tcore_1");
    this->print_array(core_2, n2, "\tcore_2");
  }

  int get_search_type() { return (search_type);        }
  int get_search_term() { return (search_termination); }

private:
  size_t null_node;
  int search_type;
  int search_termination;

  Graph& graph1;
  Graph& graph2;
  size_t core_len;
  size_t* core_1;
  size_t* core_2;
  size_t n1, n2;

  int** M;    // Matrix encoding the compatibility of nodes (byte array?).

  vertex_id_map<Graph>& vid_map1;
  vertex_id_map<Graph>& vid_map2;

  edge_id_map<Graph>& eid_map1;
  edge_id_map<Graph>& eid_map2;

  size_t* share_count;
  bool original_copy;
};



/// \brief Constructor - note, here g1 is the graph we're searching for and
///        g2 is the target graph.
template <typename Graph>
ull_state<Graph>::
ull_state(Graph& g1, Graph& g2,
          vertex_id_map<Graph>& vm1, vertex_id_map<Graph>& vm2,
          edge_id_map<Graph>& em1, edge_id_map<Graph>& em2, int search_type_,
          int search_termination_) :
  graph1(g1),    graph2(g2),
  vid_map1(vm1), vid_map2(vm2),
  eid_map1(em1), eid_map2(em2),
  search_type(search_type_), search_termination(search_termination_)

{
  original_copy = true;
  null_node = std::numeric_limits<size_t>::max();

  n1 = num_vertices(g1);
  n2 = num_vertices(g2);

  core_len = 0;

  core_1 = new size_t[n1];
  core_2 = new size_t[n2];
  M      = new int*[n1];
  assert(core_1);
  assert(core_2);
  assert(M);

  #pragma mta assert parallel
  for (int i = 0; i < n1; i++)
  {
    M[i] = new int[n2];
    assert(M[i]);
  }

  #pragma mta assert parallel
  for (int i = 0; i < n1; i++) core_1[i] = null_node;

  #pragma mta assert parallel
  for (int i = 0; i < n2; i++) core_2[i] = null_node;

  vertex_iterator verts1 = vertices(graph1);
  vertex_iterator verts2 = vertices(graph2);

  #pragma mta assert parallel
  for (int i = 0; i < n1; i++)
  {
    vertex_t v1_vertex = verts1[i];
    size_t v1_in_deg   = in_degree(v1_vertex, graph1);
    size_t v1_out_deg  = out_degree(v1_vertex, graph1);

    #pragma mta assert parallel
    for (int j = 0; j < n2; j++)
    {
      vertex_t v2_vertex = verts2[j];
      size_t v2_in_deg   = in_degree(v2_vertex, graph2);
      size_t v2_out_deg  = out_degree(v2_vertex, graph2);

      bool termIN  = v1_in_deg  <= v2_in_deg;
      bool termOUT = v1_out_deg <= v2_out_deg;

      M[i][j] = (termIN && termOUT) ? 1 : 0;
    }
  }
}

/// \brief Copy constructor.
template <typename Graph>
ull_state<Graph>::
ull_state(ull_state<Graph>& state, bool deep_copy) :
  graph1(state.graph1),     graph2(state.graph2),
  vid_map1(state.vid_map1), vid_map2(state.vid_map2),
  eid_map1(state.eid_map1), eid_map2(state.eid_map2),
  n1(state.n1), n2(state.n2),
  core_len(state.core_len)
{
  core_1 = new size_t[n1];
  core_2 = new size_t[n2];
  M = new int*[n1];

  assert(core_1);
  assert(core_2);
  assert(M);

  #pragma mta assert parallel
  for (int i = 0; i < core_len; i++) M[i] = NULL;

  #pragma mta assert parallel
  for (int i = core_len; i < n1; i++)
  {
    M[i] = new int[n2];
    assert(M[i]);
  }

  #pragma mta assert parallel
  for (int i = 0; i < n1; i++) core_1[i] = state.core_1[i];

  #pragma mta assert parallel
  for (int i = 0; i < n2; i++) core_2[i] = state.core_2[i];

  #pragma mta assert parallel
  for (int i = core_len; i < n1; i++)
  {
    #pragma mta assert parallel
    for (int j = 0; j < n2; j++) M[i][j] = state.M[i][j];
  }
}

/// \brief Destructor.
template <typename Graph>
ull_state<Graph>::~ull_state()
{
  size_t new_share_count;

  new_share_count = mt_readfe(*share_count); // lock by FE bit

  if (--new_share_count == 0)
  {
    delete[] core_1;
    delete[] core_2;
    delete share_count;

    for (int i = 0; i < n1; i++)
    {
      if (M[i]) delete [] M[i];
    }

    delete [] M;
  }

  mt_write(*share_count, new_share_count); // release lock on FE bit
}

/// \brief Puts in *pn1  and *pn2 the next pair of nodes to be tried.
///
/// prev_n1, prev_n2 must be last nodes or NULL_NODE (default) to start the
/// first pair.  Returns false if no more pairs are avaliable.
template <typename Graph>
bool
ull_state<Graph>::
next_pair(size_t* pn1, size_t* pn2, size_t prev_n1, size_t prev_n2)
{
  // TODO
}

/// \brief Returns true if (node1, node2) can be added to the state.
template <typename Graph>
bool ull_state<Graph>::
is_feasible_pair(size_t node1, size_t node2)
{
  // TODO
}

template <typename Graph>
ull_state<Graph>*
ull_state<Graph>::ShallowCopy()
{
  return (new ull_state<Graph> (*this, false));
}

template <typename Graph>
ull_state<Graph>*
ull_state<Graph>::DeepCopy()
{
  return (new ull_state<Graph> (*this, true));
}

template <typename Graph>
void
ull_state<Graph>::add_pair(size_t node1, size_t node2)
{
  // TODO
}

template <typename Graph>
void
ull_state<Graph>::back_track()
{
  // TODO
}

template <typename Graph>
void
ull_state<Graph>::get_core_set(size_t c1[], size_t c2[])
{
  // TODO
}

}

#endif

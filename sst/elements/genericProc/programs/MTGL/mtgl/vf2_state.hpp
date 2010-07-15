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
/*! \file vf2_state.hpp

    \author William McLendon (wcmclen@sandia.gov)

    \date 10/1/2009
*/
/****************************************************************************/

#ifndef MTGL_VF2_STATE_HPP
#define MTGL_VF2_STATE_HPP

#include <cstdio>
#include <limits>
#include <iomanip>

#include <mtgl/util.hpp>
#include <mtgl/psearch.hpp>
#include <mtgl/vf_isomorphism_base.hpp>

#ifdef __MTA__
#include <sys/mta_task.h>
#include <machine/runtime.h>
#endif

namespace mtgl {

#define DEBUG_is_feasible_pair 0
#define DEBUG_trace            0
#define DEBUG_print_graphs     0

/// \brief This class is used internally by the vf2_isomorphism algorithm to
///        maintain result states and store results.
template <typename Graph>
class vf2_state : public vf_default_state<Graph>{
public:
  typedef typename graph_traits<Graph>::size_type size_t;
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<Graph>::edge_descriptor edge_t;
  typedef typename graph_traits<Graph>::out_edge_iterator eiter_t;
  typedef typename graph_traits<Graph>::in_edge_iterator in_eiter_t;

  vf2_state(void);

  vf2_state(Graph& g1, Graph& g2,
            vertex_id_map<Graph>& vm1, vertex_id_map<Graph>& vm2,
            edge_id_map<Graph>& em1,   edge_id_map<Graph>& em2,
            int search_type = VF2_SUBG_MONOMORPHISM,
            int search_termination = VF2_FIND_ONE);

  vf2_state(vf2_state<Graph>& state, bool deep_copy = false);

  ~vf2_state();

  Graph& get_graph1()   { return graph1;  }
  Graph& get_graph2()   { return graph2;  }
  bool is_goal()        { return core_len == n1; }
  size_t get_core_len() { return core_len; }

  bool is_dead()
  {
    return (n1 > n2 || len_t1both > len_t2both
            || len_t1out > len_t2out || len_t1in > len_t2in);
  }


  void get_core_set(size_t c1[], size_t c2[]);

  vf2_state<Graph>* ShallowCopy();
  vf2_state<Graph>* DeepCopy();

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

  size_t core_len, orig_core_len;
  size_t added_node1;

  size_t len_t1both, len_t1in, len_t1out;
  size_t len_t2both, len_t2in, len_t2out;

  size_t* core_1;
  size_t* core_2;
  size_t* in_1;
  size_t* in_2;
  size_t* out_1;
  size_t* out_2;
  //size_t * order;
  size_t n1, n2;

  size_t* share_count;

  vertex_id_map<Graph>& vid_map1;
  vertex_id_map<Graph>& vid_map2;

  edge_id_map<Graph>& eid_map1;
  edge_id_map<Graph>& eid_map2;

  bool original_copy;
};

/// \brief Constructor - note, here g1 is the graph we're searching for and
///        g2 is the target graph.
template <typename Graph>
vf2_state<Graph>::
vf2_state(Graph& g1, Graph& g2,
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

  n1 = num_vertices(graph1);
  n2 = num_vertices(graph2);

  //order = NULL;     // only used if we're sorting nodes, which we're not currently.

  core_len = orig_core_len = 0;
  len_t1both = len_t1in = len_t1out = 0;
  len_t2both = len_t2in = len_t2out = 0;

  added_node1 = null_node;

  core_1 = new size_t[n1];
  in_1   = new size_t[n1];
  out_1  = new size_t[n1];
  core_2 = new size_t[n2];
  in_2   = new size_t[n2];
  out_2  = new size_t[n2];
  share_count = new size_t;

  if (!core_1 || !core_2 || !in_1 || !in_2 || !out_1 || !out_2
      || !share_count)
  {
    cerr << "Out of memory" << endl;
  }

  #pragma mta assert parallel
  for (int i = 0; i < n1; i++)
  {
    core_1[i] = null_node;
    in_1[i]   = 0;
    out_1[i]  = 0;
  }

  #pragma mta assert parallel
  for (int i = 0; i < n2; i++)
  {
    core_2[i] = null_node;
    in_2[i]   = 0;
    out_2[i]  = 0;
  }
  *share_count = 1;
}

/// Copy Constructor
template <typename Graph>
vf2_state<Graph>::
vf2_state(vf2_state<Graph>& state, bool deep_copy) :
  graph1(state.graph1),     graph2(state.graph2),
  vid_map1(state.vid_map1), vid_map2(state.vid_map2),
  eid_map1(state.eid_map1), eid_map2(state.eid_map2)
{
  original_copy = deep_copy;

  // lock share count while incrementing it...
  mt_readfe(*state.share_count);

  n1 = state.n1;
  n2 = state.n2;

  null_node = state.null_node;

  orig_core_len = state.core_len;
  core_len = state.core_len;

  len_t1both = state.len_t1both;
  len_t2both = state.len_t2both;
  len_t1in = state.len_t1in;
  len_t2in = state.len_t2in;
  len_t1out = state.len_t1out;
  len_t2out = state.len_t2out;

  added_node1 = null_node;
  search_type = state.search_type;
  search_termination = state.search_termination;

  // for a deep copy, we need to create new memory, etc.
  // share count will need to be set appropriately.
  if (deep_copy)
  {
    cout << "DeepCopy" << endl;

    *share_count = 0;

    core_1 = new size_t[n1];
    in_1 = new size_t[n1];
    out_1 = new size_t[n1];
    core_2 = new size_t[n2];
    in_2 = new size_t[n2];
    out_2 = new size_t[n2];

    if (!core_1 || !core_2 || !in_1 || !in_2 || !out_1 || !out_2 ||
        !share_count)
    {
      cerr << "Deep copy failed, out of memory." << endl;
    }

    #pragma mta assert parallel
    for (size_t i = 0; i < n1; i++)
    {
      core_1[i] = state.core_1[i];
      in_1[i] = state.in_1[i];
      out_1[i] = state.out_1[i];
    }

    #pragma mta assert parallel
    for (size_t i = 0; i < n2; i++)
    {
      core_2[i] = state.core_2[i];
      in_2[i] = state.in_2[i];
      out_2[i] = state.out_2[i];
    }
  }
  // For a shallow copy, we just copy the pointers.
  else
  {
    core_1 = state.core_1;
    core_2 = state.core_2;
    in_1   = state.in_1;
    in_2   = state.in_2;
    out_1  = state.out_1;
    out_2  = state.out_2;
    share_count = state.share_count;
  }

  mt_incr((*share_count), 1);
}

/// Destructor
template <typename Graph>
vf2_state<Graph>::~vf2_state()
{
  size_t new_share_count;

  new_share_count = mt_readfe(*share_count); // lock by FE bit

  if (--new_share_count == 0)
  {
    delete[] core_1;
    delete[] core_2;
    delete[] in_1;
    delete[] in_2;
    delete[] out_1;
    delete[] out_2;
    delete share_count;
    //delete [] order;
  }

  mt_write(*share_count, new_share_count); // release lock on FE bit
}

/// \brief Puts in *pn1  and *pn2 the next pair of nodes to be tried.
///
/// prev_n1, prev_n2 must be last nodes or NULL_NODE (default) to start the
/// first pair.  Returns false if no more pairs are avaliable.
template <typename Graph>
bool
vf2_state<Graph>::
next_pair(size_t* pn1, size_t* pn2, size_t prev_n1, size_t prev_n2)
{
#if DEBUG_trace
  cout << endl;
  cout << "+++\tnext_pair(" << *pn1 << ", " << *pn2 << ", "
       << prev_n1 << ", " << prev_n2 << ") " << endl;
  cout << "core_len = " << core_len << endl;
  cout << "t1 = { in=" << len_t1in << ", out=" << len_t1out << ", both="
       << len_t1both << "}" << endl;
  cout << "t2 = { in=" << len_t2in << ", out=" << len_t2out << ", both="
       << len_t2both << "}" << endl;

  print_array(in_1, n1, "  in_1", null_node);
  print_array(out_1, n1, " out_1", null_node);
  print_array(in_2, n2, "  in_2", null_node);
  print_array(out_2, n2, " out_2", null_node);
  print_array(core_1, n1, "core_1", null_node);
  print_array(core_2, n2, "core_2", null_node);
#endif

  if (prev_n1 == null_node) prev_n1 = 0;

  if (prev_n2 == null_node)
  {
    prev_n2 = 0;
  }
  else
  {
    mt_incr(prev_n2, 1);
  }

  if (len_t1both > core_len && len_t2both > core_len)
  {
    while (prev_n1 < n1 && (core_1[prev_n1] != null_node ||
                            out_1[prev_n1] == 0 || in_1[prev_n1] == 0))
    {
      mt_incr(prev_n1, 1);
      prev_n2 = 0;
    }
  }
  else if (len_t1out > core_len && len_t2out > core_len)
  {
    while (prev_n1 < n1 && (core_1[prev_n1] != null_node ||
           out_1[prev_n1] == 0))
    {
      mt_incr(prev_n1, 1);
      prev_n2 = 0;
    }
  }
  else if (len_t1in > core_len && len_t2in > core_len)
  {
    while (prev_n1 < n1 && (core_1[prev_n1] != null_node || in_1[prev_n1] == 0))
    {
      mt_incr(prev_n1, 1);
      prev_n2 = 0;
    }
  }
  else
  {
    while (prev_n1 < n1 && core_1[prev_n1] != null_node)
    {
      mt_incr(prev_n1, 1);
      prev_n2 = 0;
    }
  }

  if (len_t1both > core_len && len_t2both > core_len)
  {
    while (prev_n2 < n2 && (core_2[prev_n2] != null_node ||
                            out_2[prev_n2] == 0 || in_2[prev_n2] == 0))
    {
      mt_incr(prev_n2, 1);
    }
  }
  else if (len_t1out > core_len && len_t2out > core_len)
  {
    while (prev_n2 < n2 && (core_2[prev_n2] != null_node ||
           out_2[prev_n2] == 0))
    {
      mt_incr(prev_n2, 1);
    }
  }
  else if (len_t1in > core_len && len_t2in > core_len)
  {
    while (prev_n2 < n2 && (core_2[prev_n2] != null_node || in_2[prev_n2] == 0))
    {
      mt_incr(prev_n2, 1);
    }
  }
  else
  {
    while (prev_n2 < n2 && core_2[prev_n2] != null_node)
    {
      mt_incr(prev_n2, 1);
    }
  }

  if (prev_n1 < n1 && prev_n2 < n2)
  {
    *pn1 = prev_n1;
    *pn2 = prev_n2;
    return true;
  }

  return false;
}

/// Returns true if (node1, node2) can be added to the state.
template <typename Graph>
bool vf2_state<Graph>::
is_feasible_pair(size_t node1, size_t node2)
{
  assert(node1 < n1);
  assert(node2 < n2);
  assert(core_1[node1] == null_node);
  assert(core_2[node2] == null_node);

  //if( !g1->CompatibleNode(g1->GetNodeAttr(node1), g2->GetNodeAttr(node2)))
  //  return false;

  size_t other1, other2;
  void* attr1 = NULL;
  int termout1 = 0;
  int termout2 = 0;
  int termin1 = 0;
  int termin2 = 0;
  int new1 = 0;
  int new2 = 0;

  // Check the 'out' edges of node1

  vertex_t v1_vertex = get_vertex(node1, graph1);
  vertex_t v2_vertex = get_vertex(node2, graph2);

  size_t v1_out_degree = out_degree(v1_vertex, graph1);
  size_t v1_in_degree = in_degree(v1_vertex, graph1);

  size_t v2_out_degree = out_degree(v2_vertex, graph2);
  size_t v2_in_degree = in_degree(v2_vertex, graph2);

  int do_exit = 0;

  #pragma mta interleave schedule
  #pragma mta assert parallel
  for (size_t phase = 0; phase < 4; phase++)
  {
    if (phase == 0)
    {
      eiter_t adj_edges = out_edges(v1_vertex, graph1);

      // Check the 'out' edges of node1
      if (v1_out_degree > 10)
      {
        #pragma mta assert parallel
        for (size_t i = 0; i < v1_out_degree; i++)
        {
          if (do_exit == 0)
          {
            edge_t e = adj_edges[i];
            other1 = get(vid_map1, target(e, graph1));

            if (core_1[other1] != null_node)
            {
              other2 = core_1[other1];
              // TODO: Add attribute check here as well!
              if (!this->has_edge(graph2, node2, other2, vid_map2))
              {
                mt_incr(do_exit, 1);
              }
            }
            else
            {
              if (in_1[other1]) mt_incr(termin1, 1);
              if (out_1[other1]) mt_incr(termout1, 1);
              if (!in_1[other1] && !out_1[other1]) mt_incr(new1, 1);
            }
          }
        }
      }
      else
      {
        // degree is small, use serial version
        for (size_t i = 0; i < v1_out_degree; i++)
        {
          edge_t e = adj_edges[i];
          other1 = get(vid_map1, target(e, graph1));

          if (core_1[other1] != null_node)
          {
            other2 = core_1[other1];
            // TODO: Add attribute check here as well!
            if ( !has_edge(graph2, node2, other2, vid_map2))
            {
              mt_incr(do_exit, 1);
              break;
            }
          }
          else
          {
            if (in_1[other1]) mt_incr(termin1, 1);
            if (out_1[other1]) mt_incr(termout1, 1);
            if (!in_1[other1] && !out_1[other1]) mt_incr(new1, 1);
          }
        }
      }
    }
    else if (phase == 1)
    {
      in_eiter_t in_adj_edges = in_edges(v1_vertex, graph1);

      // Check the 'in' edges of node 1
      if (v1_in_degree > 10)
      {
        #pragma mta assert parallel
        for (size_t i = 0; i < v1_in_degree; i++)
        {
          if (0 == do_exit)
          {
            edge_t e = in_adj_edges[i];
            other1 = get(vid_map1, source(e, graph1)); // double check to make sure this does the right thing.

            if (core_1[other1] != null_node)
            {
              other2 = core_1[other1];

              if (!has_edge(graph2, other2, node2, vid_map2)) // TODO: Add attribute check here as well!
              {
                mt_incr(do_exit, 1);
              }
            }
            else
            {
              if (in_1[other1]) mt_incr(termin1, 1);
              if (out_1[other1]) mt_incr(termout1, 1);
              if (!in_1[other1] && !out_1[other1]) mt_incr(new1, 1);
            }
          }
        }
      }
      else
      {
        for (size_t i = 0; i < v1_in_degree; i++)
        {
          if (0 == do_exit)
          {
            edge_t e = in_adj_edges[i];
            other1 = get(vid_map1, source(e, graph1)); // double check to make sure this does the right thing.

            if (core_1[other1] != null_node)
            {
              other2 = core_1[other1];

              if (!has_edge(graph2, other2, node2, vid_map2)) // TODO: Add attribute check here as well!
              {
                mt_incr(do_exit, 1);
                break;
              }
            }
            else
            {
              if (in_1[other1]) mt_incr(termin1, 1);
              if (out_1[other1]) mt_incr(termout1, 1);
              if (!in_1[other1] && !out_1[other1]) mt_incr(new1, 1);
            }
          }
        }
      }
    }
    else if (phase == 2)
    {
      eiter_t adj_edges = out_edges(v2_vertex, graph2);

      if (v2_out_degree > 10)
      {
        // Check the 'out' edges of node2
        #pragma mta assert parallel
        for (size_t i = 0; i < v2_out_degree; i++)
        {
          if (0 == do_exit)
          {
            edge_t e = adj_edges[i];
            other2 = get(vid_map2, target(e, graph2));

            if (search_type != VF2_SUBG_MONOMORPHISM &&
                core_2[other2] != null_node)
            {
              other1 = core_2[other2];

              if (!has_edge(graph1, node1, other1, vid_map1))
              {
                mt_incr(do_exit, 1);
              }
            }
            else
            {
              if (in_2[other2]) mt_incr(termin2, 1);
              if (out_2[other2]) mt_incr(termout2, 1);
              if (!in_2[other2] && !out_2[other2]) mt_incr(new2, 1);
            }
          }
        }
      }
      else
      {
        // Check the 'out' edges of node2
        for (size_t i = 0; i < v2_out_degree; i++)
        {
          if (0 == do_exit)
          {
            edge_t e = adj_edges[i];
            other2 = get(vid_map2, target(e, graph2));

            if (search_type != VF2_SUBG_MONOMORPHISM &&
                core_2[other2] != null_node)
            {
              other1 = core_2[other2];

              if (!has_edge(graph1, node1, other1, vid_map1))
              {
                mt_incr(do_exit, 1);
                break;
              }
            }
            else
            {
              if (in_2[other2]) mt_incr(termin2, 1);
              if (out_2[other2]) mt_incr(termout2, 1);
              if (!in_2[other2] && !out_2[other2]) mt_incr(new2, 1);
            }
          }
        }
      }
    }
    else if (phase == 3)
    {
      in_eiter_t in_adj_edges = in_edges(v2_vertex, graph2);

      if (v2_in_degree > 10)
      {
        // Check the 'in' edges of node2.
        #pragma mta assert parallel
        for (size_t i = 0; i < v2_in_degree; i++)
        {
          if (0 == do_exit)
          {
            edge_t e = in_adj_edges[i];
            other2 = get(vid_map2, source(e, graph2));

            if (search_type != VF2_SUBG_MONOMORPHISM &&
                core_2[other2] != null_node)
            {
              other1 = core_2[other2];

              if (!has_edge(graph1, other1, node1, vid_map1))
              {
                mt_incr(do_exit, 1);
              }
            }
            else
            {
              if (in_2[other2]) mt_incr(termin2, 1);
              if (out_2[other2]) mt_incr(termout2, 1);
              if (!in_2[other2] && !out_2[other2]) mt_incr(new2, 1);
            }
          }
        }
      }
      else
      {
        // Check the 'in' edges of node2.
        for (size_t i = 0; i < v2_in_degree; i++)
        {
          if (0 == do_exit)
          {
            edge_t e = in_adj_edges[i];
            other2 = get(vid_map2, source(e, graph2));

            if (search_type != VF2_SUBG_MONOMORPHISM &&
                core_2[other2] != null_node)
            {
              other1 = core_2[other2];

              if (!has_edge(graph1, other1, node1, vid_map1))
              {
                mt_incr(do_exit, 1);
                break;
              }
            }
            else
            {
              if (in_2[other2]) mt_incr(termin2, 1);
              if (out_2[other2]) mt_incr(termout2, 1);
              if (!in_2[other2] && !out_2[other2]) mt_incr(new2, 1);
            }
          }
        }
      }
    }
  }

  // If an exit condition was flagged, then we should exit.
  if (do_exit != 0) return false;

  bool ret_val = false;
  if (search_type == VF2_FULL_ISOMORPHISM)
  {
    ret_val = (termin1 == termin2 && termout1 == termout2 && new1 == new2);
  }
  else if (search_type == VF2_SUBG_MONOMORPHISM)
  {
    ret_val = (termin1 <= termin2 &&
               termout1 <= termout2 &&
               (termin1 + termout1 + new1) <= (termin2 + termout2 + new2));
  }
  else
  {
    ret_val = (termin1 <= termin2 && termout1 <= termout2 && new1 <= new2);
  }

  return ret_val;
}

/// \brief Copies the state but allocates no new memory (i.e., just copies
///        pointers and increments share counter)
template <typename Graph>
vf2_state<Graph>*
vf2_state<Graph>::ShallowCopy()
{
  return (new vf2_state<Graph> (*this, false));
}

template <typename Graph>
vf2_state<Graph>*
vf2_state<Graph>::DeepCopy()
{
  return (new vf2_state<Graph> (*this, true));
}

template <typename Graph>
void
vf2_state<Graph>::add_pair(size_t node1, size_t node2)
{
  assert(node1 < n1);
  assert(node2 < n2);
  assert(core_len < n1);
  assert(core_len < n2);

  mt_incr(core_len, 1);
  added_node1 = node1;

  if (!in_1[node1])
  {
    in_1[node1] = core_len;
    mt_incr(len_t1in, 1);
    if (out_1[node1]) mt_incr(len_t1both, 1);
  }

  if (!out_1[node1])
  {
    out_1[node1] = core_len;
    mt_incr(len_t1out, 1);
    if (in_1[node1]) mt_incr(len_t1both, 1);
  }

  if (!in_2[node2])
  {
    in_2[node2] = core_len;
    mt_incr(len_t2in, 1);
    if (out_2[node2]) mt_incr(len_t2both, 1);
  }

  if (!out_2[node2])
  {
    out_2[node2] = core_len;
    mt_incr(len_t2out, 1);
    if (in_2[node2]) mt_incr(len_t2both, 1);
  }

  core_1[node1] = node2;
  core_2[node2] = node1;

  vertex_t v1 = get_vertex(node1, graph1);
  vertex_t v2 = get_vertex(node2, graph2);

  size_t v1_in_degree = in_degree(v1, graph1);
  size_t v1_out_degree = out_degree(v1, graph1);

  size_t v2_in_degree = in_degree(v2, graph2);
  size_t v2_out_degree = out_degree(v2, graph2);

  #pragma mta interleave schedule
  #pragma mta assert parallel
  for (int phase = 0; phase < 4; phase++)
  {
    if (0 == phase)
    {
      in_eiter_t in_adj_edges = in_edges(v1, graph1);

      #pragma mta assert parallel
      for (size_t i = 0; i < v1_in_degree; i++)
      {
        edge_t e = in_adj_edges[i];
        size_t other = get(vid_map1, source(e, graph1));

        if (!in_1[other])
        {
          in_1[other] = core_len;
          mt_incr(len_t1in, 1);
          if (out_1[other]) mt_incr(len_t1both, 1);
        }
      }
    }
    else if (1 == phase)
    {
      eiter_t adj_edges = out_edges(v1, graph1);

      #pragma mta assert parallel
      for (size_t i = 0; i < v1_out_degree; i++)
      {
        edge_t e = adj_edges[i];
        size_t other = get(vid_map1, target(e, graph1));

        if (!out_1[other])
        {
          out_1[other] = core_len;
          mt_incr(len_t1out, 1);
          if (in_1[other]) mt_incr(len_t1both, 1);
        }
      }
    }
    else if (2 == phase)
    {
      in_eiter_t in_adj_edges = in_edges(v2, graph2);

      #pragma mta assert parallel
      for (size_t i = 0; i < v2_in_degree; i++)
      {
        edge_t e = in_adj_edges[i];
        size_t other = get(vid_map2, source(e, graph2));

        if (!in_2[other])
        {
          in_2[other] = core_len;
          mt_incr(len_t2in, 1);
          if (out_2[other])
          {
            mt_incr(len_t2both, 1);
          }
        }
      }
    }
    else if (3 == phase)
    {
      eiter_t adj_edges = out_edges(v2, graph2);

      #pragma mta assert parallel
      for (size_t i = 0; i < v2_out_degree; i++)
      {
        edge_t e = adj_edges[i];
        size_t other = get(vid_map2, target(e, graph2));

        if (!out_2[other])
        {
          out_2[other] = core_len;
          mt_incr(len_t2out, 1);
          if (in_2[other])
          {
            mt_incr(len_t2both, 1);
          }
        }
      }
    }
  }
}

template <typename Graph>
void
vf2_state<Graph>::back_track()
{
  assert(core_len - orig_core_len <= 1);
  assert(added_node1 != null_node);

  if (orig_core_len < core_len)
  {
    size_t node2 = core_1[added_node1];

    vertex_t v1 = get_vertex(added_node1, graph1);
    vertex_t v2 = get_vertex(node2, graph2);

    size_t v1_in_degree = in_degree(v1, graph1);
    size_t v1_out_degree = out_degree(v1, graph1);

    size_t v2_in_degree = in_degree(v2, graph2);
    size_t v2_out_degree = out_degree(v2, graph2);

    if (in_1[added_node1] == core_len) in_1[added_node1] = 0;

    if (out_1[added_node1] == core_len) out_1[added_node1] = 0;

    if (in_2[node2] == core_len) in_2[node2] = 0;

    if (out_2[node2] == core_len) out_2[node2] = 0;

    #pragma mta assert parallel
    #pragma mta interleave schedule
    for (int phase = 0; phase < 4; phase++)
    {
      if (0 == phase)
      {
        in_eiter_t in_adj_edges = in_edges(v1, graph1);

        #pragma mta assert parallel
        for (size_t i = 0; i < v1_in_degree; i++)
        {
          edge_t e = in_adj_edges[i];
          size_t other = get(vid_map1, source(e, graph1));

          if (in_1[other] == core_len) in_1[other] = 0;
        }
      }
      else if (1 == phase)
      {
        eiter_t adj_edges = out_edges(v1, graph1);

        #pragma mta assert parallel
        for (size_t i = 0; i < v1_out_degree; i++)
        {
          edge_t e = adj_edges[i];
          size_t other = get(vid_map1, target(e, graph1));

          if (out_1[other] == core_len) out_1[other] = 0;
        }
      }
      else if (2 == phase)
      {
        in_eiter_t in_adj_edges = in_edges(v2, graph2);

        #pragma mta assert parallel
        for (size_t i = 0; i < v2_in_degree; i++)
        {
          edge_t e = in_adj_edges[i];
          size_t other = get(vid_map2, source(e, graph2));

          if (in_2[other] == core_len) in_2[other] = 0;
        }
      }
      else if (3 == phase)
      {
        eiter_t adj_edges = out_edges(v2, graph2);

        #pragma mta assert parallel
        for (size_t i = 0; i < v2_out_degree; i++)
        {
          edge_t e = adj_edges[i];
          size_t other = get(vid_map2, target(e, graph2));

          if (out_2[other] == core_len) out_2[other] = 0;
        }
      }
    }

    core_1[added_node1] = null_node;
    core_2[node2] = null_node;

    core_len = orig_core_len;
    added_node1 = null_node;
  }
}

template <typename Graph>
void
vf2_state<Graph>::get_core_set(size_t c1[], size_t c2[])
{
  size_t i, j;

  #pragma mta assert parallel
  for (i = 0, j = 0; i < n1; i++)
  {
    if (core_1[i] != null_node)
    {
      size_t j_current = mt_incr(j, 1);
      c1[j_current] = i;
      c2[j_current] = core_1[i];
    }
  }
}

}

#endif

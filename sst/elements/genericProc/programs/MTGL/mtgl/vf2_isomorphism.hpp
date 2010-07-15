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
/*! \file vf2_isomorphism.hpp

    \brief Searches for a subgraph in the given graph that is isomorphic to
           a target graph.

    \author William McLendon (wcmclen@sandia.gov)

    \date 7/23/2009

    Implements the VF2 subgraph isomorphism from Cordella et. al. into MTGL
    with modification to exploit the multithreaded XMT architecture.

    "A (Sub)Graph Isomorphism Algorithm for Matching Large Graphs"
    Luigi P. Cordella, Pasquale Foggia, Carlo Sansone, and Mario Vento
    IEEE Transactions on Pattern Analysis and Machine Intelligence,
    Vol. 26, No. 10, October 2004.
*/
/****************************************************************************/

#ifndef MTGL_VF2_ISOMORPHISM_HPP
#define MTGL_VF2_ISOMORPHISM_HPP

#include <cstdio>
#include <limits>
#include <iomanip>
#include <sys/time.h>

#include <mtgl/util.hpp>
#include <mtgl/psearch.hpp>
#include <mtgl/vf2_state.hpp>
#include <mtgl/ull_state.hpp>

#ifdef __MTA__
#include <sys/mta_task.h>
#include <machine/runtime.h>
#endif

#define DEBUG_is_feasible_pair 0
#define DEBUG_trace            0
#define DEBUG_print_graphs     0

namespace mtgl {

#define VF2_SUBG_ISOMORPHISM  1
#define VF2_FULL_ISOMORPHISM  2
#define VF2_SUBG_MONOMORPHISM 3

#define VF2_FIND_ALL 1
#define VF2_FIND_ONE 2

/*! \brief Default VF2 isomorphism search visitor class.  operator() is called
           to handle isomorphic matches.
           If the mode of VF2 allows for finding multiple matches, returning
           TRUE from this will terminate the VF2 search, FALSE allows VF2 to
           search for all matches.
 */
class default_vf2_visitor {
public:
  virtual bool
  operator()(int n, size_t ni1[], size_t ni2[]) { return false; }
};

/*! \brief This is the main vf2 (sub)graph isomorphism class.  Implements the
           VF2 graph / subgraph isomorphism algorithm by Cordella, Foggia,
           Sansone, and Vento by adapting the reference implementation
           of vflib into the MTGL framework.

    This algorithm can compute graph or subgraph isomorphism returning a
    mapping of vertex indices from one graph to the other.

    Execution modes include full graph isomorphism and sub-graph isomorphism
    returning all matched sub-graphs or the first sub-graph.

    The class default_vf2_visitor can be used as a base class visitor to
    customize handling of matches found during execution.

    vflib can be found at the following URL:
     http://amalfi.dis.unina.it/graph/db/vflib-2.0
 */
template <typename Graph, typename match_visitor = default_vf2_visitor>
class vf2_isomorphism {
public:
  typedef typename graph_traits<Graph>::size_type size_t;
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<Graph>::edge_descriptor edge_t;
  typedef typename graph_traits<Graph>::out_edge_iterator eiter_t;
  typedef typename graph_traits<Graph>::in_edge_iterator in_eiter_t;

  vf2_isomorphism(Graph& graph, Graph& g_pattern, match_visitor& vis);
  ~vf2_isomorphism();

  int run(int search_type, int search_termination);

  int match(vf2_state<Graph>* state);

  int match(size_t c1[], size_t c2[], vf2_state<Graph>* state,
            size_t * pcount, size_t rdepth = 0);

  bool match(size_t * pn, size_t c1[], size_t c2[],
             vf2_state<Graph>* state, size_t rdepth = 0);

private:
  typedef struct nodeset
  {
    size_t n1;
    size_t n2;
    bool is_feasible;
  } nodeset;

  void print_graph(Graph& g);

  Graph& graph1;  // Graph
  Graph& graph2;  // Pattern (looking for this one!)

  match_visitor& match_vis;  // Visitor class

  size_t core_alloc_size;

  size_t null_node;

  int num_state_objs;
  int max_state_objs;
  int match_id;
};

template <typename Graph, typename match_visitor>
vf2_isomorphism<Graph, match_visitor>::
vf2_isomorphism(Graph& graph, Graph& g_pattern, match_visitor& vis) :
  graph1(graph), graph2(g_pattern), match_vis(vis)
{
  null_node = std::numeric_limits<size_t>::max();
  num_state_objs = 0;
  max_state_objs = 2;

#if DEBUG_print_graphs
  cout << "Target Graph:" << endl;
  print_graph(graph);
  cout << "Pattern graph:" << endl;
  print_graph(g_pattern);
#endif
}

template <typename Graph, typename match_visitor>
vf2_isomorphism<Graph, match_visitor>::~vf2_isomorphism() {}

/*! \brief This routine starts the VF2 isomorphism algorithm running.

    \parm search_type The type of search to conduct.  Valid options are
        VF2_SUBG_ISOMORPHISM : find isomorphic subgraphs.
        VF2_FULL_ISOMORPHISM : full graph isomorphism.
        VF2_SUBG_MONOMORPHISM: subgraph monomorphism (default)

    \param search_termination Stop after first match found or find all
                              matches?  Valid options are
         VF2_FIND_ONE : Stop after first match found (default)
         VF2_FIND_ALL : Keep going until all matches are found.
 */
template <typename Graph, typename match_visitor>
int
vf2_isomorphism<Graph, match_visitor>::
run(int search_type = VF2_SUBG_MONOMORPHISM,
    int search_termination = VF2_FIND_ONE)
{
  vertex_id_map<Graph> vid_map1 = get(_vertex_id_map, graph1);
  vertex_id_map<Graph> vid_map2 = get(_vertex_id_map, graph2);

  edge_id_map<Graph> eid_map1 = get(_edge_id_map, graph1);
  edge_id_map<Graph> eid_map2 = get(_edge_id_map, graph2);

  int r_val = 0;
  match_id = -1;

  vf2_state<Graph> s0(graph2,      graph1,
                      vid_map2,    vid_map1,
                      eid_map2,    eid_map1,
                      search_type, search_termination);

  mt_incr(num_state_objs, 1);

  //vf2_state<Graph> * s1 = new vf2_state<Graph>();
  //s1->DeepCopy( &s0 );

  r_val = match(&s0);

  return r_val;
}



// Initial state call--run() invokes this method.
template <typename Graph, typename match_visitor>
int
vf2_isomorphism<Graph, match_visitor>::
match(vf2_state<Graph>* state)
{
#if DEBUG_trace
  cout << "+\tmatch()" << endl;

  switch ( state->get_search_type())
  {
    case VF2_SUBG_ISOMORPHISM:
      cout << "\t-\tsearch_type = SUBG_ISOMORPHISM" << endl;
      break;

    case VF2_FULL_ISOMORPHISM:
      cout << "\t-\tsearch_type = FULL_ISOMORPHISM" << endl;
      break;

    case VF2_SUBG_MONOMORPHISM:
      cout << "\t-\tsearch_type = SUBG_MONOMORPHISM" << endl;
      break;

    default:
      cerr << "\t-\tUNRECOGNIZED SEARCH TYPE!" << endl;
  }

  switch ( state->get_search_term())
  {
    case VF2_FIND_ONE:
      cout << "\t-\ttermination = FIND_ONE match" << endl;
      break;

    case VF2_FIND_ALL:
      cout << "\t-\ttermination = FIND_ALL matches" << endl;
      break;

    default:
      cerr << "\t-\tUNRECOGNIZED TERMINATION TYPE!" << endl;
  }
#endif

  size_t num_verts_g1 = num_vertices(graph1);
  size_t num_verts_g2 = num_vertices(graph2);

  size_t num_edges_g1 = num_edges(graph1);
  size_t num_edges_g2 = num_edges(graph2);

  // Choose conservative dimension for our arrays
  size_t n = num_verts_g1 < num_verts_g2 ? num_verts_g1 : num_verts_g2;

  core_alloc_size = n;

  size_t* c1 = new size_t[core_alloc_size];
  size_t* c2 = new size_t[core_alloc_size];

  if (!c1 || !c2)
  {
    cerr << "Out of memory" << endl;
    return 0;
  }

  #pragma mta assert no dependence
  for (size_t i = 0; i < core_alloc_size; i++)
  {
    c1[i] = null_node;
    c2[i] = null_node;
  }

  size_t count = 0;

  // One match
  if (state->get_search_term() == VF2_FIND_ONE ||
      state->get_search_type() == VF2_FULL_ISOMORPHISM)
  {
    match(&count, c1, c2, state);

    if (count > 0) match_vis(count, c1, c2);
  }
  // All matches (subgraphs only)
  else if (state->get_search_term() == VF2_FIND_ALL)
  {
    match(c1, c2, state, &count);
  }
  else
  {
    cerr << "MTGL VF2 Isomorphism, unrecognized option provided." << endl;
  }

  delete[] c1;
  delete[] c2;

  return count;
}

/// \brief Visits all the matchings between two graphs, starting from state
///        'state'.
///
/// Returns true if the caller must stop the visit.  Stops when there are no
/// more matches, or the visitor vis returns true.
template <typename Graph, typename match_visitor>
int
vf2_isomorphism<Graph, match_visitor>::
match(size_t c1[], size_t c2[], vf2_state<Graph>* state,
      size_t* pcount, size_t rdepth)
{
  mt_incr(match_id, 1);

  if (state->is_goal())
  {
    mt_incr((*pcount), 1); // ++*pcount; (check this that it's doing the right thing!)

    size_t n = state->get_core_len();

    state->get_core_set(c1, c2);

    // in ref implementation there's a user_data argument here as well
    int ret = match_vis(n, c1, c2);
    return ret;
  }

  if (state->is_dead()) return false;

  size_t n1 = null_node;
  size_t n2 = null_node;

  while (state->next_pair(&n1, &n2, n1, n2))
  {
    if (state->is_feasible_pair(n1, n2))
    {
      vf2_state<Graph>* s1 = state->ShallowCopy();
      s1->add_pair(n1, n2);

      if (match(c1, c2, s1, pcount, rdepth + 1))
      {
        s1->back_track();
        delete s1;
        return true;
      }
      else
      {
        s1->back_track();
        delete s1;
      }
    }
  }

  return false;
}

/// \brief Finds a matching between two graphs, if it exists, starting from
///        the initial state.
///
/// Returns true if a match is found.  *pn is the number of matched nodes.
/// c1 and c2 will contain the ids of the corresponding nodes in the two
/// graphs.
template <typename Graph, typename match_visitor>
bool
vf2_isomorphism<Graph, match_visitor>::
match(size_t* pn, size_t c1[], size_t c2[],
      vf2_state<Graph>* state, size_t rdepth)
{
  bool found = false;

  if (state->is_goal())
  {
    *pn = state->get_core_len();
    state->get_core_set(c1, c2);
    return true;
  }

  if (state->is_dead()) return false;

  vector<nodeset> node_pairs;
  size_t t1 = null_node;
  size_t t2 = null_node;

  while (state->next_pair(&t1, &t2, t1, t2))
  {
    nodeset node;
    node.n1 = t1;
    node.n2 = t2;
    node_pairs.push_back(node);
  }

  for (size_t i; !found && i < node_pairs.size(); i++)
  {
    size_t n1 = node_pairs[i].n1;
    size_t n2 = node_pairs[i].n2;

    if (state->is_feasible_pair(n1, n2))
    {
      vf2_state<Graph>* s1 = state->ShallowCopy();
      s1->add_pair(n1, n2);
      found = match(pn, c1, c2, s1, rdepth + 1);
      s1->back_track();
      delete s1;
    }
  }

  return found;
}

/// Helper method to print out a graph.
template <typename Graph, typename match_visitor>
void
vf2_isomorphism<Graph, match_visitor>::print_graph(Graph& g)
{
  typedef typename graph_traits<Graph>::vertex_iterator vertex_iterator;

  vertex_id_map<Graph> vid_map = get(_vertex_id_map, g);

  size_t num_verts = num_vertices(g);
  size_t num_edges = num_edges(g);

  vertex_iterator verts = vertices(g);

  for (size_t vidx = 0; vidx < num_verts; vidx++)
  {
    vertex_t vi = verts[vidx];
    int deg = out_degree(vi, g);
    eiter_t adj_edges = out_edges(vi, g);

    cout << vidx << "\tout [" << setw(2) << deg << "] { ";

    for (size_t adji = 0; adji < deg; adji++)
    {
      edge_t e = adj_edges[adji];
      size_t other = get(vid_map, target(e, g));
      cout << other << " ";
    }

    cout << "}" << endl;

    deg = in_degree(vi, g);
    eiter_t in_adj_edges = in_edges(vi, g);

    cout << "\tin  [" << setw(2) << deg << "] { ";

    for (size_t adji = 0; adji < deg; adji++)
    {
      edge_t e = in_adj_edges[adji];
      size_t other = get(vid_map, source(e, g));
      cout << other << " ";
    }

    cout << "}" << endl;
  }
}

}

#endif

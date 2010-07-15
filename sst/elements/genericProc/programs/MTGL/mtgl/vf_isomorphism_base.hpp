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
/*! \file vf2_isomorphism_base.hpp

    \author William McLendon (wcmclen@sandia.gov)

    \date 10/2/2009
*/
/****************************************************************************/

#ifndef MTGL_VF_ISOMORPHISM_BASE_HPP
#define MTGL_VF_ISOMORPHISM_BASE_HPP

namespace mtgl {

#define VF2_SUBG_ISOMORPHISM  1
#define VF2_FULL_ISOMORPHISM  2
#define VF2_SUBG_MONOMORPHISM 3

#define VF2_FIND_ALL 1
#define VF2_FIND_ONE 2

template <typename Graph>
class vf_default_state {
public:
  typedef typename graph_traits<Graph>::size_type size_t;
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<Graph>::edge_descriptor edge_t;
  typedef typename graph_traits<Graph>::out_edge_iterator eiter_t;

  virtual ~vf_default_state() {}
  virtual Graph& get_graph1() = 0;
  virtual Graph& get_graph2() = 0;

  virtual bool   is_goal() = 0;
  virtual bool   is_dead() = 0;
  virtual size_t get_core_len() = 0;
  virtual void get_core_set(size_t c1[], size_t c2[]) = 0;

  virtual vf_default_state<Graph>* ShallowCopy() = 0;
  virtual vf_default_state<Graph>* DeepCopy() = 0;

  virtual void back_track() {};

  virtual bool is_feasible_pair(size_t node1, size_t node2) = 0;

  virtual void add_pair(size_t node1, size_t node2) = 0;

  virtual bool next_pair(size_t* pn1, size_t* pn2, size_t prev_n1,
                         size_t prev_n2) = 0;

  //int get_search_type() { return (search_type);        }
  //int get_search_term() { return (search_termination); }

  bool has_edge(Graph& g1, size_t node1, size_t node2,
                vertex_id_map<Graph>& vid_map);

  virtual void print_array(size_t* A, size_t size, char* tag,
                           size_t null_value = -1);

  // size_t null_node;
  // int    search_type;
  // int    search_termination;
};

/// Helper utility, mainly for debugging and such.
template <typename Graph>
void
vf_default_state<Graph>::print_array(size_t* A, size_t size, char* tag,
                                     size_t null_value)
{
  if (tag) cout << tag << "\t";

  cout << "[ ";
  for (int i = 0; i < size; i++)
  {
    if (A[i] != null_value)
    {
      cout << setw(2) << A[i] << " ";
    }
    else
    {
      cout << "<> ";
    }
  }

  cout << "]" << endl;
}

/// \brief Determines if an edge (u,v) exists in the graph given u and v
///        specified as vertex indices.
template <typename Graph>
bool
vf_default_state<Graph>::
has_edge(Graph& ga, size_t node1, size_t node2, vertex_id_map<Graph>& vid_map)
{
  int ret = 0;

  if (node1 >= 0 && node2 >= 0 && node1 < num_vertices(ga) &&
      node2 < num_vertices(ga))
  {
    const vertex_t u = get_vertex(node1, ga);
    const int deg = out_degree(u, ga);
    eiter_t adj_edges = out_edges(u, ga);

    int found = 0;

    if (deg > 20)
    {
      #pragma mta assert parallel
      for (int i = 0; i < deg; i++)
      {
        if (0 == found)
        {
          edge_t e = adj_edges[i];
          const size_t vidx = get(vid_map, target(e, ga));

          if (vidx == node2)
          {
            mt_incr(found, 1); // set flag so rest of xmt loops basically do nothing.
            mt_incr(ret, 1); // (atomic increment, set to true)
          }
        }
      }
    }
    else
    {
      for (int i = 0; (i < deg) && (0 == ret); i++)
      {
        edge_t e = adj_edges[i];
        const size_t vidx = get(vid_map, target(e, ga));

        if (vidx == node2)
        {
          mt_incr(ret, 1); // (atomic increment, set to true)
        }
      }
    }
  }

  return (0 != ret);
}

}

#endif

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
/*! \file mtgl_adapter.hpp

    \brief This file contains common code used by adapters over graph
           data structures.

    \author Jon Berry (jberry@sandia.gov)

    \date 2007
*/
/****************************************************************************/

#ifndef MTGL_MTGL_ADAPTER_HPP
#define MTGL_MTGL_ADAPTER_HPP

#include <cstdio>

#include <mtgl/graph_traits.hpp>
#include <mtgl/mtgl_boost_property.hpp>
#include <mtgl/util.hpp>

// There are pragmas in this code specifically for the MTA
// The following line will disable the warning
#ifndef __MTA__
#pragma warning( disable: 4068 )
#endif

namespace mtgl {

enum vertex_id_map_t { _vertex_id_map = 0 };
enum edge_id_map_t { _edge_id_map = 1 };

template <class graph>
class vertex_id_map :
  public put_get_helper<int, vertex_id_map<graph> > {
public:
  typedef typename graph_traits<graph>::vertex_descriptor key_type;
  typedef typename graph_traits<graph>::size_type value_type;
  vertex_id_map() {}

  value_type operator[] (const key_type& k) const { return k.get_id();  }
  value_type operator[] (const key_type* k) const { return k->get_id(); }
};

template <class graph>
class edge_id_map :
  public put_get_helper<int, edge_id_map<graph> > {
public:
  typedef typename graph_traits<graph>::edge_descriptor key_type;
  typedef typename graph_traits<graph>::size_type value_type;
  edge_id_map() {}

  value_type operator[] (const key_type& k) const { return k.get_id();  }
  value_type operator[] (const key_type* k) const { return k->get_id(); }
};

template <class graph_adapter>
inline vertex_id_map<graph_adapter> get(vertex_id_map_t, graph_adapter& ga)
{
  return vertex_id_map<graph_adapter>();
}

template <class graph_adapter>
inline vertex_id_map<graph_adapter>get(vertex_id_map_t, const graph_adapter& ga)
{
  return vertex_id_map<graph_adapter>();
}

template <class graph_adapter>
inline edge_id_map<graph_adapter> get(edge_id_map_t, graph_adapter& ga)
{
  return edge_id_map<graph_adapter>();
}

template <class graph_adapter, class visitor>
void
visit_edges(graph_adapter& ga,
            typename graph_traits<graph_adapter>::vertex_descriptor v,
            visitor& vis,
            typename graph_traits<graph_adapter>::size_type par_cutoff = 5000,
            bool use_future = true,
            int directed = DIRECTED)
{
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_t;
  typedef typename graph_traits<graph_adapter>::out_edge_iterator e_iter_t;
  typedef typename graph_traits<graph_adapter>::size_type size_type;

  e_iter_t eit = out_edges(v, ga);
  size_type deg = out_degree(v, ga);

  if (deg >= par_cutoff)
  {
    if (use_future)
    {
      #pragma mta assert parallel
      #pragma mta loop future
      for (size_type i = 0; i < deg; i++)
      {
        edge_t e = eit[i];
        vis(e);
      }
    }
    else
    {
      #pragma mta assert parallel
      for (size_type i = 0; i < deg; i++)
      {
        edge_t e = eit[i];
        vis(e);
      }
    }
  }
  else
  {
    for (size_type i = 0; i < deg; i++)
    {
      edge_t e = eit[i];
      vis(e);
    }
  }
}

template <class graph_adapter, class visitor>
void visit_edges_filtered(
    graph_adapter& ga,
    typename graph_traits<graph_adapter>::vertex_descriptor v,
    visitor& vis, int par_cutoff = 5000,
    bool use_future = true)
{
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_t;
  typedef typename graph_traits<graph_adapter>::size_type size_type;
  typedef typename graph_traits<graph_adapter>::out_edge_iterator inc_iter_t;

  inc_iter_t eit = out_edges(v, ga);
  size_type deg = out_degree(v, ga);

  if (deg >= par_cutoff)
  {
    if (use_future)
    {
      #pragma mta assert parallel
      #pragma mta loop future
      for (size_type i = 0; i < deg; i++)
      {
        edge_t e = eit[i];

        if (vis.visit_test(e)) vis(e);
      }
    }
    else
    {
      #pragma mta assert parallel
      for (size_type i = 0; i < deg; i++)
      {
        edge_t e = eit[i];

        if (vis.visit_test(e)) vis(e);
      }
    }
  }
  else
  {
    for (size_type i = 0; i < deg; i++)
    {
      edge_t e = eit[i];

      if (vis.visit_test(e)) vis(e);
    }
  }
}

template <class graph_adapter, class visitor>
void visit_edges(graph_adapter& ga,
                 visitor vis, bool use_future = true)
{
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_t;
  typedef typename graph_traits<graph_adapter>::edge_iterator edge_iter_t;
  typedef typename graph_traits<graph_adapter>::size_type size_type;

  edge_iter_t edgs = edges(ga);
  size_type size = num_edges(ga);

  if (use_future)
  {
    #pragma mta assert parallel
    #pragma mta loop future
    for (size_type i = 0; i < size; i++)
    {
      edge_t e = edgs[i];
      vis(e);
    }
  }
  else
  {
    #pragma mta assert parallel
    for (size_type i = 0; i < size; i++)
    {
      edge_t e = edgs[i];
      vis(e);
    }
  }
}

template <class graph_adapter, class visitor>
void visit_edges_filtered(graph_adapter& ga,
                          visitor vis, bool use_future = true)
{
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_t;
  typedef typename graph_traits<graph_adapter>::edge_iterator edge_iter_t;
  typedef typename graph_traits<graph_adapter>::size_type size_type;

  edge_iter_t edgs = edges(ga);
  size_type size = num_edges(ga);

  if (use_future)
  {
    #pragma mta assert parallel
    #pragma mta loop future
    for (size_type i = 0; i < size; i++)
    {
      edge_t e = edgs[i];

      if (vis.visit_test(e)) vis(e);
    }
  }
  else
  {
    #pragma mta assert parallel
    for (size_type i = 0; i < size; i++)
    {
      edge_t e = edgs[i];

      if (vis.visit_test(e)) vis(e);
    }
  }
}

template <class graph_adapter>
void print(graph_adapter& ga)
{
  typedef typename graph_traits<graph_adapter>::size_type size_type;
  typedef typename graph_traits<graph_adapter>::vertex_descriptor
          vertex_descriptor;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_descriptor;
  typedef typename graph_traits<graph_adapter>::vertex_iterator vertex_iterator;
  typedef typename graph_traits<graph_adapter>::out_edge_iterator
          out_edge_iterator;

  vertex_id_map<graph_adapter> vipm = get(_vertex_id_map, ga);
  edge_id_map<graph_adapter> eipm = get(_edge_id_map, ga);

  vertex_iterator verts = vertices(ga);
  size_type order = num_vertices(ga);
  for (size_type i = 0; i < order; ++i)
  {
    vertex_descriptor v = verts[i];
    int vid = get(vipm, v);

    printf("%d : { ", vid);

    out_edge_iterator inc_edgs = out_edges(v, ga);
    size_type out_deg = out_degree(v, ga);
    for (size_type j = 0; j < out_deg; ++j)
    {
      edge_descriptor e = inc_edgs[j];
      int eid = get(eipm, e);

      vertex_descriptor src = source(e, ga);
      vertex_descriptor trg = target(e, ga);

      int sid = get(vipm, src);
      int tid = get(vipm, trg);

      printf("{%d}(%d, %d) ", eid, sid, tid);
    }

    printf("}\n");
  }
}

template <class graph_adapter>
void print_transpose(graph_adapter& ga)
{
  typedef typename graph_traits<graph_adapter>::size_type size_type;
  typedef typename graph_traits<graph_adapter>::vertex_descriptor
          vertex_descriptor;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_descriptor;
  typedef typename graph_traits<graph_adapter>::vertex_iterator vertex_iterator;
  typedef typename graph_traits<graph_adapter>::out_edge_iterator
          out_edge_iterator;

  vertex_id_map<graph_adapter> vipm = get(_vertex_id_map, ga);
  edge_id_map<graph_adapter> eipm = get(_edge_id_map, ga);

  vertex_iterator verts = vertices(ga);
  size_type order = num_vertices(ga);
  for (size_type i = 0; i < order; ++i)
  {
    vertex_descriptor v = verts[i];
    int vid = get(vipm, v);

    printf("%d : { ", vid);

    out_edge_iterator inc_edgs = out_edges(v, ga);
    size_type out_deg = out_degree(v, ga);
    for (size_type j = 0; j < out_deg; ++j)
    {
      edge_descriptor e = inc_edgs[j];
      int eid = get(eipm, e);

      vertex_descriptor src = source(*inc_edgs, ga);
      vertex_descriptor trg = target(*inc_edgs, ga);

      int sid = get(vipm, src);
      int tid = get(vipm, trg);

      printf("{%d}(%d, %d) ", eid, sid, tid);
    }

    printf("}\n");
  }
}

template <class graph_adapter>
void print_edges(graph_adapter& ga)
{
  typedef typename graph_traits<graph_adapter>::vertex_descriptor
          vertex_descriptor;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_descriptor;
  typedef typename graph_traits<graph_adapter>::edge_iterator edge_iterator;
  typedef typename graph_traits<graph_adapter>::size_type size_type;

  vertex_id_map<graph_adapter> vipm = get(_vertex_id_map, ga);
  edge_id_map<graph_adapter> eipm = get(_edge_id_map, ga);

  edge_iterator edgs = edges(ga);
  size_type num_edges = num_edges(ga);
  for (size_type i = 0; i < num_edges; ++i)
  {
    edge_descriptor e = edgs[i];
    size_type eid = get(eipm, *edgs);

    vertex_descriptor v = source(*edgs, ga);
    vertex_descriptor w = target(*edgs, ga);

    size_type vid = get(vipm, v);
    size_type wid = get(vipm, w);

    printf("(%d, %d)\n",  vid, wid);
  }
}

template <class graph_adapter>
void print_adj(graph_adapter& ga)
{
  typedef typename graph_traits<graph_adapter>::size_type size_type;
  typedef typename graph_traits<graph_adapter>::vertex_iterator vertex_iterator;
  typedef typename graph_traits<graph_adapter>::vertex_descriptor
          vertex_descriptor;
  typedef typename graph_traits<graph_adapter>::adjacency_iterator
          adjacency_iterator;

  vertex_id_map<graph_adapter> vipm = get(_vertex_id_map, ga);

  vertex_iterator verts = vertices(ga);
  size_type num_verts = num_vertices(ga);
  for (size_type i = 0; i < num_verts; ++i)
  {
    vertex_descriptor src = verts[i];
    size_type vid = get(vipm, src);

    printf("%d: { ", vid);

    adjacency_iterator adj_verts = adjacent_vertices(src, ga);
    size_type out_deg = out_degree(src, ga);
    for (size_type j = 0; j < out_deg; ++j)
    {
      vertex_descriptor trg = adj_verts[j];
      size_type tid = get(vipm, trg);

      printf("(%d, %d) ", vid, tid);
    }

    printf("}\n");
  }
}

template <class graph>
void separate_by_degree(graph& g, int*& indicesOfBigs, int& num_big,
                        int*& indicesOfSmalls, int& num_small,
                        int deg_threshold = 5000)
{
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;
  typedef typename graph_traits<graph>::vertex_iterator vertex_iter_t;
  typedef typename graph_traits<graph>::size_type size_type;

  num_big   = 0;
  num_small = 0;
  int max_deg = 0;

  vertex_id_map<graph> vid_map = get(_vertex_id_map, g);

  vertex_iter_t viter = vertices(g);
  size_type ord = num_vertices(g);

  #pragma mta assert parallel
  for (size_type i = 0; i < ord; ++i)
  {
    vertex_t v = viter[i];

    size_type deg = out_degree(v, g);

    if (deg > max_deg) max_deg = deg;

    if (deg > deg_threshold)
    {
      mt_incr(num_big, 1);
    }
    else
    {
      mt_incr(num_small, 1);
    }
  }

  indicesOfBigs = (int*) malloc(sizeof(int) * num_big);
  indicesOfSmalls = (int*) malloc(sizeof(int) * num_small);

  num_big = num_small = 0;

  #pragma mta assert parallel
  for (size_type i = 0; i < ord; ++i)
  {
    vertex_t v = viter[i];
    int id = get(vid_map, v);

    if (out_degree(v, g) > deg_threshold)
    {
      int mine = mt_incr(num_big, 1);
      indicesOfBigs[mine] = id;
    }
    else
    {
      int mine = mt_incr(num_small, 1);
      indicesOfSmalls[mine] = id;
    }
  }

  printf("sep by deg: %d, %d (max: %d)\n", num_big, num_small, max_deg);
}

template <class graph>
void separate_by_in_degree(graph& g, int*& indicesOfBigs, int& num_big,
                           int*& indicesOfSmalls, int& num_small,
                           int deg_threshold = 5000)
{
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;
  typedef typename graph_traits<graph>::vertex_iterator vertex_iter_t;
  typedef typename graph_traits<graph>::size_type size_type;

  num_big   = 0;
  num_small = 0;
  int max_deg = 0;

  vertex_id_map<graph> vid_map = get(_vertex_id_map, g);

  vertex_iter_t viter = vertices(g);
  size_type ord = num_vertices(g);

  #pragma mta assert parallel
  for (size_type i = 0; i < ord; ++i)
  {
    vertex_t v = viter[i];

    size_type deg = in_degree(v, g);

    if (deg > max_deg) max_deg = deg;

    if (deg > deg_threshold)
    {
      mt_incr(num_big, 1);
    }
    else
    {
      mt_incr(num_small, 1);
    }
  }

  indicesOfBigs = (int*) malloc(sizeof(int) * num_big);
  indicesOfSmalls = (int*) malloc(sizeof(int) * num_small);
  num_big = num_small = 0;

  #pragma mta assert parallel
  for (size_type i = 0; i < ord; ++i)
  {
    vertex_t v = viter[i];
    int id = get(vid_map, v);

    if (in_degree(v, g) > deg_threshold)
    {
      int mine = mt_incr(num_big, 1);
      indicesOfBigs[mine] = id;
    }
    else
    {
      int mine = mt_incr(num_small, 1);
      indicesOfSmalls[mine] = id;
    }
  }

  printf("sep by in: %d, %d (max: %d)\n", num_big, num_small, max_deg);
}

}

#endif

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
/*! \file connectivity_visitor.hpp

    \brief This code contains the visitor classes used by the s-t connectivity
           algorithm(s) in st_connectivity.hpp.

    \author Kamesh Madduri

    \date 8/2005
*/
/****************************************************************************/

#ifndef MTGL_CONNECTIVITY_VISITOR_HPP
#define MTGL_CONNECTIVITY_VISITOR_HPP

#include <cstdio>
#include <climits>

#include <mtgl/util.hpp>
#include <mtgl/breadth_first_search.hpp>
#include <mtgl/mtgl_adapter.hpp>
#include <mtgl/graph_traits.hpp>

namespace mtgl {

template <typename graph_adapter>
class sp_visitor : public default_bfs_visitor<graph_adapter>{
public:
  typedef typename graph_traits<graph_adapter>::size_type size_type;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_t;
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex_t;


  sp_visitor(size_type s, size_type t, size_type* col, size_type* done,
             vertex_id_map<graph_adapter>& vid_map_) :
    s(s), t(t), done(done), color(col), vid_map(vid_map_)
  {
    mt_write(color[s], 1);
    *done = 0;
  }

  void bfs_root(vertex_t& v) const {}

  void tree_edge(edge_t& e, vertex_t& src, vertex_t& dest) const
  {
    size_type destId = get(vid_map, dest);
    size_type srcId  = get(vid_map, src);
    mt_write(color[destId], color[srcId]);
  }

  void back_edge(edge_t& e, vertex_t& src, vertex_t& dest) const
  {
    size_type srcId = get(vid_map, src);
    size_type destId = get(vid_map, dest);
    size_type clr = mt_readff(color[destId]);

    if (color[srcId] != clr) mt_incr(*done, 1);
  }

  void discover_vertex(vertex_t& v) const {}
  void examine_vertex(vertex_t& v) const {}
  void finish_vertex(vertex_t& v) const {}

  size_type s, t;
  size_type* done;
  size_type* color;
  vertex_id_map<graph_adapter>& vid_map;
};

}

#endif

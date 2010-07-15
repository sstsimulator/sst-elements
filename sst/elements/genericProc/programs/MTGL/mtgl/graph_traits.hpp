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
/*! \file graph_traits.hpp

    \brief This simple class implements the Boost Graph Library's graph_traits
           idiom.

    \author Jon Berry (jberry@sandia.gov)
    \author Greg Mackey (gemacke@sandia.gov)

    \date 12/4/2007
*/
/****************************************************************************/

#ifndef MTGL_GRAPH_TRAITS_HPP
#define MTGL_GRAPH_TRAITS_HPP

namespace mtgl {

template <class graph>
class graph_traits {
public:
  typedef typename graph::size_type               size_type;
  typedef typename graph::vertex_descriptor       vertex_descriptor;
  typedef typename graph::edge_descriptor         edge_descriptor;
  typedef typename graph::vertex_iterator         vertex_iterator;
  typedef typename graph::adjacency_iterator      adjacency_iterator;
  typedef typename graph::in_adjacency_iterator   in_adjacency_iterator;
  typedef typename graph::edge_iterator           edge_iterator;
  typedef typename graph::out_edge_iterator       out_edge_iterator;
  typedef typename graph::in_edge_iterator        in_edge_iterator;
  typedef typename graph::directed_category       directed_category;
  typedef typename graph::graph_type              graph_type;
};

struct undirectedS {
  static bool is_directed() { return false; }
  static bool is_bidirectional() { return false; }
};

struct directedS {
  static bool is_directed() { return true; }
  static bool is_bidirectional() { return false; }
};

struct bidirectionalS {
  static bool is_directed() { return true; }
  static bool is_bidirectional() { return true; }
};

}

#endif

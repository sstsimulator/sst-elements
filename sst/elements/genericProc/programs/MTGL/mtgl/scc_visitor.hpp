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
/*! \file scc_visitor.hpp

    \brief This code contains the visitor classes used by the strongly
           connected components algorithm(s) defined in
           strongly_connected_components.hpp and
           strongly_connected_components_dfs.hpp.

    \author Jon Berry (jberry@sandia.gov)

    \date 2/2006
*/
/****************************************************************************/

#ifndef MTGL_SCC_VISITOR_HPP
#define MTGL_SCC_VISITOR_HPP

#include <cstdio>
#include <climits>

#include <mtgl/graph_traits.hpp>
#include <mtgl/mtgl_adapter.hpp>
#include <mtgl/psearch.hpp>
#include <mtgl/util.hpp>

#define NO_SCC -1
#define FWD_MARK 1
#define BWD_MARK 2
#define NO_MARK  0

namespace mtgl {

template <class graph_adapter>
class forward_scc_visitor : public default_psearch_visitor<graph_adapter> {

public:
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_t;

  forward_scc_visitor(graph_adapter& _ga, short* fmark, int* scc,
                      vertex_id_map<graph_adapter>& _vid_map) :
    ga(_ga), forward_mark(fmark), scc_num(scc), vid_map(_vid_map) {}

  void psearch_root(vertex_t& v) { forward_mark[get(vid_map, v)] |= FWD_MARK; }

  bool visit_test(edge_t& e, vertex_t& src) const
  {
    vertex_t tv = target(e, ga);
    int tvid = get(vid_map, tv);

    return scc_num[tvid] == NO_SCC ? true : false;
  }

  void pre_visit(vertex_t& v) { forward_mark[get(vid_map, v)] |= FWD_MARK; }

protected:
  graph_adapter& ga;
  short* forward_mark;
  int* scc_num;
  vertex_id_map<graph_adapter>& vid_map;
};

template <typename graph_adapter>
class backward_scc_visitor : public default_psearch_visitor<graph_adapter> {
public:
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_t;

  backward_scc_visitor(graph_adapter& _ga, short* bmark, int* scc,
                       vertex_id_map<graph_adapter>& _vid_map) :
    ga(_ga), backward_mark(bmark), scc_num(scc), vid_map(_vid_map) {}

  void psearch_root(vertex_t& v) { backward_mark[get(vid_map, v)] |= BWD_MARK; }

  bool visit_test(edge_t& e, vertex_t& src) const
  {
    vertex_t sv = source(e, ga);
    int svid = get(vid_map, sv);

    return scc_num[svid] == NO_SCC ? true : false;
  }

  void pre_visit(vertex_t& v) { backward_mark[get(vid_map, v)] |= BWD_MARK; }

protected:
  graph_adapter& ga;
  short* backward_mark;
  int* scc_num;
  vertex_id_map<graph_adapter>& vid_map;
};

template <typename graph_adapter>
class forward_scc_visitor2 : public default_psearch_visitor<graph_adapter> {
public:
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_t;

  forward_scc_visitor2(bool* fmark, int* subp, int mysubp, int* scc,
                       vertex_id_map<graph_adapter>& _vid_map) :
    forward_mark(fmark), subproblem(subp), my_subproblem(mysubp),
    scc_num(scc), vid_map(_vid_map) {}

  void psearch_root(vertex_t& v)
  {
    int vid = get(vid_map, v);
    forward_mark[vid] = true;
  }

  bool visit_test(edge_t& e, vertex_t& src) const
  {
    int sid = get(vid_map, source(e, src));
    int tid = get(vid_map, target(e, src));

    return subproblem[tid] == my_subproblem ? true : false;
  }

  void pre_visit(vertex_t& v)
  {
    int vid = get(vid_map, v);
    forward_mark[vid] = true;
  }

protected:
  bool* forward_mark;
  int* subproblem;
  int my_subproblem;
  int* scc_num;
  vertex_id_map<graph_adapter>& vid_map;
};

template <typename graph_adapter>
class backward_scc_visitor2 : public default_psearch_visitor<graph_adapter> {
public:
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_t;

  backward_scc_visitor2(bool* fmark, int* subp, int mysubp, int* scc,
                        vertex_id_map<graph_adapter>& _vid_map) :
    backward_mark(fmark), subproblem(subp), scc_num(scc),
    my_subproblem(mysubp), vid_map(_vid_map) {}

  void psearch_root(vertex_t& v)
  {
    int vid = get(vid_map, v);
    backward_mark[vid] = true;
  }

  void pre_visit(vertex_t& v)
  {
    int vid = get(vid_map, v);
    backward_mark[vid] = true;
  }

  bool visit_test(edge_t& e, vertex_t& src) const
  {
    int sid = get(vid_map, source(e, src));
    int tid = get(vid_map, target(e, src));

    return subproblem[sid] == my_subproblem ? true : false;
  }

protected:
  bool* backward_mark;
  int* subproblem;
  int my_subproblem;
  int* scc_num;
  vertex_id_map<graph_adapter>& vid_map;
};

}

#undef BWD_MARK
#undef NO_MARK

#endif

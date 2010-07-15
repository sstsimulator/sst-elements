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
/*! \file components_visitor.hpp

    \brief This code contains the visitor classes used by the connected
           components algorithm(s) defined in connected_components.hpp.

    \author Jon Berry (jberry@sandia.gov)

    \date 10/2005
*/
/****************************************************************************/

#ifndef MTGL_COMPONENTS_VISITOR_HPP
#define MTGL_COMPONENTS_VISITOR_HPP

#include <cstdio>
#include <climits>
#include <algorithm>

#include <mtgl/util.hpp>
#include <mtgl/bfs.hpp>
#include <mtgl/shiloach_vishkin.hpp>
#include <mtgl/copier.hpp>
#include <mtgl/psearch.hpp>

namespace mtgl {

/*! \brief Tailors the psearch of Phase I of Kahan's three-phase
           connected components algorithm.  The output is a hash table
           associating the leaders of component searches that touched
           each other.

           It's debatable whether the template arguments should be
           retained.  They were intended to provide flexibility that
           might never be used.  As is, they cause no harm.
*/
template <typename graph, typename ComponentMap, typename LeaderMap,
          typename SmallgraphMap, class uinttype>
class cc_psearch_visitor : public default_psearch_visitor<graph> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  cc_psearch_visitor(uinttype g_ord, LeaderMap& ldrs, ComponentMap& rm,
                     short valid_typ, SmallgraphMap& sm,
                     uinttype& lc, vertex_id_map<graph>& vmap,
                     edge_id_map<graph>& emap, graph& _g) :
    vid_map(vmap), eid_map(emap), component(rm), small_graph(sm),
    leaders(ldrs), valid_type(valid_typ), lcount(lc), g_order(g_ord), g(_g) {}

  void psearch_root(vertex& v) const
  {
    uinttype next = mt_incr(lcount, 1);   // psearch root count
    uinttype id = get(vid_map, v);
    mt_write(component[id], id);
    leaders[id] = next;
  }

  void tree_edge(edge& e, vertex& src) const
  {
    #pragma mta trace "cc_psearch:tree_edge"

    vertex dest = other(e, src, g);
    uinttype sid = get(vid_map, src);
    uinttype did = get(vid_map, dest);
    mt_write(component[did], component[sid]);
  }

  void back_edge(edge& e, vertex& src) const
  {
    vertex dest = other(e, src, g);
    uinttype v1 = get(vid_map, src), v2 = get(vid_map, dest);
    uinttype c1 = mt_readff(component[v1]);
    uinttype c2 = mt_readff(component[v2]);

    if (c1 != c2)
    {
      u_int64_t key1 = c1;
      key1 = key1 * g_order + c2;
      u_int64_t key2 = c2;
      key2 = key2 * g_order + c1;
      pair<u_int64_t, u_int64_t> p(leaders[c1], leaders[c2]);

      if (!small_graph.member(key1))
      {
        small_graph.insert(key1, p);
        pair<u_int64_t, u_int64_t> p2(leaders[c1], leaders[c2]);
        small_graph.insert(key2, p2);
      }
    }
  }

private:
  vertex_id_map<graph>& vid_map;
  edge_id_map<graph>& eid_map;
  ComponentMap component;
  SmallgraphMap& small_graph;
  LeaderMap leaders;
  short valid_type;
  uinttype& lcount;                // search root count
  uinttype g_order;                // the order of the graph
  graph& g;

  /*! \fn cc_psearch_visitor(int g_ord, LeaderMap& ldrs, ComponentMap& rm,
                             short valid_typ, SmallgraphMap& sm,
                             int& c, int& lc)
      \brief This constructor stores references to user data structures that
             store the component leaders and the component numbers of all
             vertices.
      \param g_ord The number of vertices in the graph
      \param ldrs The ids of component leaders from which searches originated
      \param rm The result map - component numbers for all vertices
      \param valid_typ  deprecated
      \param sm The hash table associating component leaders
      \param c The count of all node visits (deprecated)
      \param lc The number of search leaders

      \fn void psearch_root(vertex *v) const
      \brief Assign a unique leader number to v.
      \param v A search leader

      \fn void tree_edge(edge *e, vertex *src) const
      \brief Searching from src via e, the destination has not been discovered
             yet.  Assign the component number of src to dest.
      \param e An edge being traversed in order to find a vertex for the
               first time.
      \param src The vertex from which the traversal of e is initiated

      \fn void back_edge(edge *e, vertex *src) const
      \brief Searching from src via e, the destination has been discovered
             before.  If this was by a search from a different leader, make a
             hash table entry associating the two leaders.
      \param e An edge being traversed in order to find a vertex that has
               been discovered previously.
      \param src The vertex from which the traversal of e is initiated.
  */
};

template <typename LeaderMap, typename elem, typename TargetMap>
class sv_copy_filter : public default_copy_filter<elem> {
public:
  sv_copy_filter(LeaderMap& lm, TargetMap& tm, int lsz, int& r) :
    leader(lm), dest(tm), num_leaders(lsz), rank(r)
  {
    rank = r;                   // shouldn't be necessary, but is
    printf("svcf: rank: %d\n", rank);
  }

  inline void copy_element(elem& e)  const
  {
    int r = mt_incr(rank, 1);

    dest[r] = e;
    dest[r].x = r;
    dest[r].y = leader[e.y];
    dest[r].z = leader[e.z];
  }

private:
  TargetMap dest;
  LeaderMap leader;
  int num_leaders;
  int& rank;
};

/*! \brief Tailors the run of ShiloachVishkin from ShiloachVishkin.h.

    Note that this is deprecated. We should use the connected_components_sv
    algorithm in connected_components.hpp.  However, the sv phase of Kahan's
    algorithm is such a tiny percentage of the run-time that it hasn't been
    a priority to do this.  

    When graft and hook operations occur in Shiloach-Vishkin, we update
    component numbers here.
*/
template <typename ComponentLeaderType>
class sv_visitor : public default_sv_visitor {
public:
  sv_visitor(ComponentLeaderType& cl) : comp_leaders(cl) {}

  void graft(int grafted, int new_leader) const { comp_leaders[grafted] = 0; }
  void hook(int parent, int gparent) const { comp_leaders[parent] = 0; }

private:
  ComponentLeaderType comp_leaders;
};

/*! \brief Tailors the psearch of Phase III of Kahan's three-phase
           connected components algorithm.  This search simply "spreads
           the news."  Prior to running this search, all true connected
           component leaders have been identified.  It remains only to
           relabel all vertices with their leader's id.  This is done
           through the tree_edge method of this class.

*/
template <typename graph, typename ComponentMap,
          typename uinttype = unsigned long>
class post_psearch_visitor : public default_psearch_visitor<graph> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  post_psearch_visitor(ComponentMap& rm, vertex_id_map<graph>& vmap,
                       graph& _g, int* vm = 0) :
    component(rm), vid_map(vmap), g(_g) {}

  void tree_edge(edge& e, vertex& src) const
  {
    vertex dest = other(e, src, g);
    component[get(vid_map, dest)] = component[get(vid_map, src)];
  }

protected:
  ComponentMap component;
  vertex_id_map<graph>& vid_map;
  graph& g;

  /*! \fn post_psearch_visitor(ComponentMap& rm, int *vm=0)
      \brief This constructor stores references to user data structures that
             store the component component numbers of all vertices.
       \param rm The result map - component numbers for all vertices
       \param vm The vertex mask - future feature indicating what vertices are
                 of interest for this computation.  T

       \fn void psearch_root(vertex *v) const
       \brief Assign a unique leader number to v.
       \param v A search leader

       \fn void tree_edge(edge *e, vertex *src) const
       \brief Searching from src via e, the destination has not been
              discovered yet.  Assign the component number of src to dest.
       \param e An edge being traversed in order to find a vertex for the
                first time.
       \param src The vertex from which the traversal of e is initiated
  */
};

/*! \brief Tailors the bfs of Phase III of Kahan's three-phase
           connected components algorithm.  This search simply "spreads
           the news."  Prior to running this search, all true connected
           component leaders have been identified.  It remains only to
           relabel all vertices with their leader's id.  This is done
           through the tree_edge method of this class.

*/
template <typename graph, typename ComponentMap>
class post_bfs_visitor : public default_bfs2_visitor<graph> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  post_bfs_visitor(ComponentMap& rm, vertex_id_map<graph>& vmap,
                   graph& _g, int* vm = 0) :
    component(rm), vid_map(vmap), g(_g) {}

  post_bfs_visitor(const post_bfs_visitor& v) :
    component(v.component), vid_map(v.vid_map), g(v.g) {}

  void tree_edge(edge& e, vertex& src) const
  {
    vertex dest = other(e, src, g);
    component[get(vid_map, dest)] = component[get(vid_map, src)];
  }

protected:
  ComponentMap component;
  vertex_id_map<graph>& vid_map;
  graph& g;
};

template <typename graph, typename ComponentMap>
class filtered_psearch_visitor : public default_psearch_visitor<graph> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  filtered_psearch_visitor(ComponentMap& rm, graph& _g, int* vm = 0) :
    component(rm), vert_mask(vm), g(_g) {}

  bool visit_test(edge e, vertex src) const
  {
    vertex dest = other(e, src, g);
    return (!vert_mask || vert_mask[dest->get_id()] == 1);
  }

  void tree_edge(edge e, vertex src) const
  {
    vertex dest = other(e, src, g);
    component[dest->get_id()] = component[src->get_id()];
  }

protected:
  ComponentMap component;
  int* vert_mask;
  graph& g;
};

template <typename graph>
class find_t_visitor : public default_psearch_visitor<graph> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;

  find_t_visitor(int tt, bool& flg) : t(tt), flag(flg) {}

  void pre_visit(vertex src) const { if (src->get_id() == t) flag = true; }

private:
  bool& flag;
  int t;
};

template <typename graph>
class scc_labeling_visitor : public default_psearch_visitor<graph> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  // Given an scc leader, traverse its scc, blanking
  // out the finishing times and filling in the result

  scc_labeling_visitor(int* ft, int* mask, int* scc, graph& _g) :
    finish_time(ft), vert_mask(mask), result(scc), g(_g) {}

  bool visit_test(edge e, vertex src)
  {
    vertex dest = other(e, src, g);
    return (!vert_mask || vert_mask[dest->get_id()]);
  }

  void tree_edge(edge e, vertex src) const
  {
    vertex dest = other(e, src, g);
    result[dest->get_id()] = result[src->get_id()];
    finish_time[dest->get_id()] = 0;
  }

protected:
  int* finish_time;
  int* result;
  int* vert_mask;
  graph& g;
};

/*! \brief Tailors the psearch of the bully algorithm for connected
           components.  This class inherits from post_psearch_visitor
           to demonstrate that flexibility, but it could easily exist
           as its own class.

    The search will continue if an undiscovered vertex is encountered, or if
    a discovered vertex has a higher component leader id than the vertex
    discovering it.

    Note that the visit_test and the tree_edge routines cannot be executed
    atomically.  The algorithm is correct since an actual relabeling will
    occur only if it should, as determined by the tree_edge routine.

    Note also that certain vertices will never have an opportunity to become
    search roots.  In other versions of this code, we have explored parameters
    that allow every vertex a chance to initiate a search, if requested.  For
    the bully algorithm, however, this is not necessary.
*/
template <typename graph, typename ComponentMap>
class bully_comp_visitor : public post_psearch_visitor<graph, ComponentMap> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  bully_comp_visitor(ComponentMap& rm, vertex_id_map<graph>& vmap, graph& _g) :
    post_psearch_visitor<graph, ComponentMap>(rm, vmap, _g), src_c(0), g(_g) {}

  void pre_visit(vertex& v)
  {
    src_c = post_psearch_visitor<graph, ComponentMap>
            ::component[get(this->vid_map, v)];
  }

  void psearch_root(vertex& v) const {}

  bool visit_test(edge& e, vertex& src) const
  {
    vertex v2 = other(e, src, g);

    int c2 = post_psearch_visitor<graph, ComponentMap>
             ::component[get(this->vid_map, v2)];

    if (src_c < c2)
    {
      return true;
    }
    else
    {
      return false;
    }
  }

  void tree_edge(edge& e, vertex& src) const
  {
    vertex dest = other(e, src, g);
    int c = mt_readfe(post_psearch_visitor<graph,
                                           ComponentMap>::
                      component[get(this->vid_map, dest)]);

    if (c > src_c)
    {
      post_psearch_visitor<graph, ComponentMap>
      ::component[get(this->vid_map, dest)] = src_c;
    }
    else
    {
      mt_write(post_psearch_visitor<graph, ComponentMap>
               ::component[get(this->vid_map, dest)], c);
    }
  }

private:
  int src_c;
  graph& g;

  /*! \fn bully_psearch_visitor(ComponentMap& rm)
      \brief This constructor stores references to user data structures
               that store the component leaders and the component numbers
               of all vertices.
      \param rm The result map - component numbers for all vertices

      \fn void psearch_root(vertex *v) const
      \brief Assign a unique leader number to v.
      \param v A search leader

      \fn void visit_test(edge *e, vertex *src) const
      \brief Give a search another chance to continue: if the destination's
             component leader id is greater than that of the vertex
             disovering that destination (perhaps not the first discoverer).
      \param e An edge that may or may not be traversed to discover a vertex.
      \param src The vertex from which the traversal of e is initiated

      \fn void tree_edge(edge *e, vertex *src) const
      \brief If the component leader id of src is less than that of the
             destination vertex, rewrite the component leader of the latter.
      \param e An edge being traversed either because the destination has
               not yet been discovered, or because its component leader id
               is dominated by that of the src vertex.
  */
};

/*! \brief Tailors the bfs of the bully algorithm for connected
           components.  This class inherits from post_bfs_visitor
           to demonstrate that flexibility, but it could easily exist
           as its own class.

    The search will continue if an undiscovered vertex is encountered, or if
    a discovered vertex has a higher component leader id than the vertex
    discovering it.

    Note that the visit_test and the tree_edge routines cannot be executed
    atomically.  The algorithm is correct since an actual relabeling will
    occur only if it should, as determined by the tree_edge routine.

    Note also that certain vertices will never have an opportunity to become
    search roots.  In other versions of this code, we have explored parameters
    that allow every vertex a chance to initiate a search, if requested.  For
    the bully algorithm, however, this is not necessary.
*/
template <typename graph, typename ComponentMap>
class bully_comp_visitor_bfs : public post_bfs_visitor<graph, ComponentMap> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  bully_comp_visitor_bfs(ComponentMap& rm, vertex_id_map<graph>& vmap,
                         graph& _g) :
    post_bfs_visitor<graph, ComponentMap>(rm, vmap, _g), src_c(0), g(_g) {}

  void pre_visit(vertex& v)
  {
    src_c = post_bfs_visitor<graph, ComponentMap>
            ::component[get(this->vid_map, v)];
  }

  void bfs_root(vertex& v) const {}

  bool visit_test(edge& e, vertex& src) const
  {
    vertex v2 = other(e, src, g);

    int c2 = post_bfs_visitor<graph, ComponentMap>
             ::component[get(this->vid_map, v2)];

    if (src_c < c2)
    {
      return true;
    }
    else
    {
      return false;
    }
  }

  void tree_edge(edge& e, vertex& src) const
  {
    vertex dest = other(e, src, g);
    int c = mt_readfe(post_bfs_visitor<graph,
                                       ComponentMap>::
                      component[get(this->vid_map, dest)]);

    if (c > src_c)
    {
      post_bfs_visitor<graph, ComponentMap>
      ::component[get(this->vid_map, dest)] = src_c;
    }
    else
    {
      mt_write(post_bfs_visitor<graph, ComponentMap>
               ::component[get(this->vid_map, dest)], c);
    }
  }

private:
  int src_c;
  graph& g;

  /*! \fn bully_bfs_visitor(ComponentMap& rm)
      \brief This constructor stores references to user data structures
             that store the component leaders and the component numbers
             of all vertices.
      \param rm The result map - component numbers for all vertices

      \fn void bfs_root(vertex *v) const
      \brief Assign a unique leader number to v.
      \param v A search leader

      \fn void visit_test(edge *e, vertex *src) const
      \brief Give a search another chance to continue: if the destination's
             component leader id is greater than that of the vertex
             disovering that destination (perhaps not the first discoverer).
      \param e An edge that may or may not be traversed to discover a vertex.
      \param src The vertex from which the traversal of e is initiated

      \fn void tree_edge(edge *e, vertex *src) const
      \brief If the component leader id of src is less than that of the
             destination vertex, rewrite the component leader of the latter.
      \param e An edge being traversed either because the destination has
               not yet been discovered, or because its component leader id
               is dominated by that of the src vertex.
  */
};

}

#endif

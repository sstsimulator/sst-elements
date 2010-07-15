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
/*! \file mtgl_search_edgetime.hpp

    \brief This code contains the mtgl_search_edgetime() routine.  This
           invokes an edge-time based search of a graph using the psearch()
           routine defined in psearch.hpp.

    \author William McLendon (wcmclen@sandia.gov)

    \date 1/23/2008
*/
/****************************************************************************/

#ifndef MTGL_MTGL_SEARCH_EDGETIME_HPP
#define MTGL_MTGL_SEARCH_EDGETIME_HPP

#include <climits>

#include <mtgl/mtgl_adapter.hpp>
#include <mtgl/psearch.hpp>
#include <mtgl/util.hpp>

namespace mtgl {

/*! \brief Implements a rooted search of a graph where edges are given a time
           stamp.  A root edge, r=(u,v), is provided which sets the starting
           time to be time(r).  Vertex u will be returned as the
           starting vertex of the search and both u and v will have a time
           set to be time(r).  An edge e=(u,v) is traversed iff
           time(e) > time(u); if traversed we set time(v) = time(e).
           The result of this search is essentially a rooted tree starting from
           src(r) in which vertex timestamps are monotonically increasing with
           distance (hops) from src(r).
 */
template<class GRAPH_T, typename TIMESTAMP_T>
class mtgl_search_edgetime {
  typedef typename graph_traits<GRAPH_T>::vertex_descriptor VERTEX_T;
  typedef typename graph_traits<GRAPH_T>::edge_descriptor EDGE_T;
  typedef typename graph_traits<GRAPH_T>::out_edge_iterator EITER_T;
  typedef typename graph_traits<GRAPH_T>::vertex_iterator VITER_T;

public:
  /* time-equivalent of infinity, purposely a public item so it can
     be easily accessed. */
  TIMESTAMP_T INFTIME;

  /*! \brief Default constructor, takes a graph adapter and a list of
             edge time stamps.
      \param _G        (IN) Reference to a graph adapter.
      \param _edgeTime (IN) Array of edge time stamps.  Must be |E| in
                            size.
   */
  mtgl_search_edgetime(GRAPH_T& _G, TIMESTAMP_T* _edgeTime = NULL) :
    G(_G), edgeTime(_edgeTime)
  {
    INFTIME  = INT_MAX;
    max_time = INFTIME;
  }

  /*! \brief Searches the graph starting from a given edge id, returns
             a result in vTimes and incEdgeID.
      \param srcEdgeID (IN)  Edge index of source edge (u->v).
      \param vTimes    (OUT) Array to store timestamps of vertices.
                             Must be allocated to size V.
      \param incEdgeID (OUT) Array storing the incident edge id for
                             each vertex that was visited in search.
                             For a vertex v, the edge id is an edge
                             u->v.  Will be -99 if v was not visited,
                             and -1 if v is the root vertex (i.e, no
                             incident edges.
      \param maxTime   (OUT) Sets a maximum valid timestamp for edges to
                             be traversed.  Can serve to create a window
                             allowing only edges with a timestamp from
                             time(srcEdgeID) to maxTime to be included in
                             the result.
   */
  int run(int srcEdgeID,
          TIMESTAMP_T* vTimes,
          int* incEdgeID,
          TIMESTAMP_T* maxTime)
  {
    max_time = maxTime;
    return(run(srcEdgeID, vTimes, incEdgeID));
  }

  /*! \brief Searches the graph starting from a given edge id, returns
             a result in vTimes and incEdgeID.
      \param srcEdgeID (IN)  Edge index of source edge (u->v).
      \param vTimes    (OUT) Array to store timestamps of vertices.
                             Must be allocated to size V.
      \param incEdgeID (OUT) Array storing the incident edge id for
                             each vertex that was visited in search.
                             For a vertex v, the edge id is an edge
                             u->v.  Will be -99 if v was not visited,
                             and -1 if v is the root vertex (i.e, no
                             incident edges.
   */
  int run(int srcEdgeID,
          TIMESTAMP_T* vTimes,
          int* incEdgeID)
  {
    TIMESTAMP_T startTime;
    int edgeSrcVertID = 0;
    int edgeTgtVertID = 0;

    assert(vTimes    != NULL);
    assert(incEdgeID != NULL);

    unsigned int n = num_vertices(G);
    int m = num_edges(G);

    vertex_id_map<GRAPH_T> vid_map = get(_vertex_id_map, G);
    edge_id_map<GRAPH_T>   eid_map = get(_edge_id_map,   G);

    EDGE_T srcEdge   = get_edge(srcEdgeID, G);

    edgeTgtVertID = get(vid_map, target(srcEdge, G));
    edgeSrcVertID = get(vid_map, source(srcEdge, G));

    startTime = edgeTime[srcEdgeID];

    int* vLock = (int*) malloc(sizeof(int) * n);

    vis_t vis(G, vLock, incEdgeID, edgeTime, vTimes, max_time,
              vid_map, eid_map);

    #pragma mta assert no dependence
    for (int i = 0; i < n; ++i)
    {
      vLock[i]     = 0;
      incEdgeID[i] = -99;
      vTimes[i]    = INFTIME;
    }

    vTimes[edgeTgtVertID]    = startTime;
    vTimes[edgeSrcVertID]    = startTime;
    incEdgeID[edgeSrcVertID] = -1;
    incEdgeID[edgeTgtVertID] = srcEdgeID;

    psearch<GRAPH_T, int*, vis_t, PURE_FILTER, DIRECTED, 10>
    psrch(G, NULL, vis);

    psrch.run(get_vertex(edgeTgtVertID, G));

    free(vLock);  vLock = NULL;

    return(1);
  }

protected:
  //-----[ psearch visitor_t ]-------------------------------------------
  class vis_t : default_psearch_visitor<GRAPH_T>{
public:
    vis_t(GRAPH_T& __G,
          int* __vLock,
          int* __incEdgeID,
          TIMESTAMP_T* __eTime,
          TIMESTAMP_T* __vTimes,
          TIMESTAMP_T __max_time,
          vertex_id_map<GRAPH_T>& __vid_map,
          edge_id_map<GRAPH_T>& __eid_map) :
      G(__G), vLock(__vLock), incEdgeID(__incEdgeID),
      eTime(__eTime), vTimes(__vTimes), max_time(__max_time),
      vid_map(__vid_map), eid_map(__eid_map) {}

    bool start_test(VERTEX_T& v) { return(true); }
    void pre_visit(VERTEX_T& v) {}
    void psearch_root(VERTEX_T& v) {}
    void finish_vertex(VERTEX_T& v) {}

    bool visit_test(EDGE_T e, VERTEX_T v)
    {
      int eid = get(eid_map, e);
      int sid = get(vid_map, source(e, v));
      int tid = get(vid_map, target(e, v));
      TIMESTAMP_T et = eTime[eid];

      if (sid != tid && et <= max_time)
      {
        if (et > vTimes[sid] && et < vTimes[tid])
        {
          return(true);
        }
      }

      return(false);
    }

    void tree_edge(EDGE_T e, VERTEX_T src)
    {
      int eid = get(eid_map, e);
      int sid = get(vid_map, source(e, src));
      int tid = get(vid_map, target(e, src));
      TIMESTAMP_T et = eTime[eid];

      if (et < vTimes[tid])
      {
        int vL = mt_readfe(vLock[tid]);    // lock target vert
        if ( et < vTimes[tid] )
        {
          vTimes[tid]    = et;
          incEdgeID[tid] = eid;
        }
        mt_write(vLock[tid], vL + 1);      // unlock target vert
      }
    }

    void back_edge(EDGE_T e, VERTEX_T src) {}

private:
    GRAPH_T& G;
    int* vLock;
    int* incEdgeID;
    TIMESTAMP_T* vTimes;
    TIMESTAMP_T* eTime;
    TIMESTAMP_T max_time;
    vertex_id_map<GRAPH_T>& vid_map;
    edge_id_map<GRAPH_T>& eid_map;
  };
  //-----[ psearch visitor_t ]-------------------------------------------

  void printGraph()
  {
    vertex_id_map<GRAPH_T> vid_map = get(_vertex_id_map, G);
    edge_id_map<GRAPH_T>   eid_mpa = get(_edge_id_map,   G);

    VITER_T verts = vertices(G);

    for (unsigned int vi = 0; vi < num_vertices(G); ++vi)
    {
      VERTEX_T vu = verts[vi];
      EITER_T inc_edges = out_edges(vu, G);
      int deg = G.get_degree(vu);
      printf("%u\t{ ", vi);

      for (int ineigh = 0; ineigh < deg; ++ineigh)
      {
        EDGE_T e = inc_edges[ineigh];
        int tgt = get(vid_map, target(e, G));
        printf("%d ", tgt);
      }

      printf("}\n");
    }
  }

private:
  GRAPH_T& G;
  TIMESTAMP_T* edgeTime;
  TIMESTAMP_T max_time;
};

}

#endif

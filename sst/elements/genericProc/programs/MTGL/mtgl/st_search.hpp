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
/*! \file st_search.hpp

    \brief Shortest path S to T search class.

    \author William McLendon (wcmclen@sandia.gov)

    \date 1/5/2007
*/
/****************************************************************************/

#ifndef MTGL_ST_SEARCH_HPP
#define MTGL_ST_SEARCH_HPP

#include <cstdio>
#include <climits>

#include <mtgl/breadth_first_search.hpp>
#include <mtgl/mtgl_adapter.hpp>
#include <mtgl/graph_traits.hpp>
#include <mtgl/psearch.hpp>
#include <mtgl/dynamic_array.hpp>
#include <mtgl/util.hpp>

// Definitons and Macros
#define DIR_FWD  1
#define DIR_BWD -1
#define UNKNOWN -1

#define COLOR_WHITE 0
#define COLOR_GREY  1
#define COLOR_BLACK 2

// #define DEBUGST 1

/* Some useful macros */

#define TIMER_RESTART(timer)                   \
{                                              \
  timer.stop();                                \
  timer.start();                               \
}

#define TIMER_SECT(timer, label, cond)         \
{                                              \
  if (cond)                                    \
  {                                            \
    timer.stop();                              \
    printf("TIME: %35s = %7.4lf\n",            \
           label, timer.getElapsedSeconds());  \
    timer.start();                             \
  }                                            \
}

#define TIMEIT 0

namespace mtgl {

#if defined(DEBUGST)

/// Convenient function for printing out arrays of integers.
inline void print_arr(int* A, int sz, char* lbl = NULL)
{
  assert(A != NULL);

  if (lbl)
  {
    printf("% 15s [ ", lbl);
  }
  else
  {
    printf("% 15s [ ", "");
  }

  for (int i = 0; i < sz; ++i)
  {
    if (A[i] != -1)
    {
      printf("%1d ", A[i]);
    }
    else
    {
      printf(". ");
    }
  }

  printf("]\n");
};

/// Convenient function for printing out arrays of bools.
inline void print_arr(bool* A, int sz, char* lbl = NULL)
{
  assert(A != NULL);

  if (lbl)
  {
    printf("% 15s [ ", lbl);
  }
  else
  {
    printf("% 15s [ ", "");
  }

  for (int i = 0; i < sz; ++i)
  {
    if (A[i] != 0)
    {
      printf("%1d ", A[i]);
    }
    else
    {
      printf(". ");
    }
  }

  printf("]\n");
};
#endif

/// \breif The print_edges function is helpful for printing out a dynamic
///        array of integers containing the edge-ids of a list of edges.
///        This just prints then out in an asthetically pleasing manner.
///        Needs to be put somewhere more appropriate than in st_search.hpp
template <typename graph_adapter>
void print_edges(dynamic_array<int>& eids, graph_adapter& ga)
{
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_t;
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph_adapter>::edge_iterator edge_iterator;

  vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, ga);

  edge_iterator edgs = edges(ga);

  for (int i = 0; i < eids.size(); ++i)
  {
    edge_t e = edgs[eids[i]];
    vertex_t sv = source(e, ga);
    vertex_t tv = target(e, ga);
    int svid = get(vid_map, sv);
    int tvid = get(vid_map, tv);

    printf("edge : %9d -> %-9d\n", svid, tvid);
  }
}

/// \brief The "S-T" shortest path algorithm searches for the shortest path
///        between two vertices in g from V_s to V_t.
template <typename graph_adapter, int dir = DIRECTED>
class st_search {
public:
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_t;
  typedef typename graph_traits<graph_adapter>::vertex_iterator
          vertex_iterator;

  // Constructor
  st_search(graph_adapter& _ga, int _vs, int _vt,
            dynamic_array<int>& _short_path_ids) :
    ga(_ga), vs(_vs), vt(_vt), short_path_ids(_short_path_ids)
  {
    distance = 0;

    if (dir == DIRECTED)
    {
      dirFwd = DIRECTED;
      dirBwd = REVERSED;
    }
    else if (dir == UNDIRECTED)
    {
      dirFwd = UNDIRECTED;
      dirBwd = UNDIRECTED;
    }
    else if (dir == REVERSED)
    {
      dirFwd = REVERSED;
      dirBwd = DIRECTED;
    }
  }

  int run(const int retDataType = EDGES)
  {
    unsigned int nVerts = num_vertices(ga);
    int nEdges = num_edges(ga);

    mt_timer mttimer;
    mttimer.start();

    int* rec_colors = (int*) malloc(sizeof(int) * nVerts);

//    printf("ST RETURN TYPE = %d\n", retDataType);

    // -------------------------------------------------------------
    // --- Start Discovery Phase ---
    // -------------------------------------------------------------
    bool found = false;
    int* dist_from_vs = (int*) malloc(sizeof(int) * nVerts);
    int* dist_from_vt = (int*) malloc(sizeof(int) * nVerts);
    bool* meetpt_mask = (bool*) malloc(sizeof(bool) * nVerts);

    if (vs == vt)
    {
      printf("vs and vt are the same vertex\n");
      return(0);
    }

    // Initialize for discovery.
    #pragma mta assert no dependence
    #pragma mta block schedule
    for (int i = 0; i < nVerts; ++i)
    {
      dist_from_vs[i] = UNKNOWN;
      dist_from_vt[i] = UNKNOWN;
      rec_colors[i]= COLOR_BLACK;
      meetpt_mask[i] = false;
    }

    dist_from_vs[vs] = 0;
    dist_from_vt[vt] = 0;
    rec_colors[vs] = COLOR_WHITE;
    rec_colors[vt] = COLOR_WHITE;

    vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, ga);
    edge_id_map<graph_adapter> eid_map = get(_edge_id_map, ga);

    vis_discover fwd_discoverer(vs, vt, nVerts, &found, &distance, DIR_FWD,
                                dist_from_vs, dist_from_vt, rec_colors,
                                meetpt_mask, vid_map);

    vis_discover bwd_discoverer(vs, vt, nVerts, &found, &distance, DIR_BWD,
                                dist_from_vs, dist_from_vt, rec_colors,
                                meetpt_mask, vid_map);

    bfs_bidir<vis_discover>(ga, get_vertex(vs, ga), fwd_discoverer,
                            get_vertex(vt, ga), bwd_discoverer,
                            &found, vid_map);

#ifdef DEBUGST
    printf("=== finished discovery phase ===\n");
    print_arr(dist_from_vs, nVerts, "DVS");
    print_arr(dist_from_vt, nVerts, "DVT");
    print_arr(meetpt_mask, nVerts, "meetpt_mask");
#endif

#if TIMEIT
    TIMER_SECT(mttimer, "STSEARCH\tDISC", TIMEIT);
#endif

    // -----------------------------------------------------------
    // --- Start Recording Phase ---
    // -----------------------------------------------------------
    if (found)
    {
#ifdef DEBUGST
      printf("=== start recording phase ===\n");
#endif

      // Only allocate mask for vids or eids based on parameter.
      bool* ret_id_mask = NULL;
      unsigned int ret_id_mask_sz = 0;

      if (retDataType == VERTICES)
      {
        ret_id_mask_sz = num_vertices(ga);
      }
      else if (retDataType == EDGES)
      {
        ret_id_mask_sz = num_edges(ga);
      }
      else
      {
        fprintf(stderr, "ERROR: st-search reports invalid"
                " return type provided.\n");
        exit(1);
      }

      ret_id_mask = (bool*) malloc(sizeof(bool) * ret_id_mask_sz);
      #pragma mta assert parallel
      for (int i = 0; i < ret_id_mask_sz; ++i) ret_id_mask[i] = false;

      vis_recorder rvis(dist_from_vs, dist_from_vt, distance, rec_colors,
                        ret_id_mask, retDataType, vid_map, eid_map, ga);

      psearch<graph_adapter, int*, vis_recorder, AND_FILTER, UNDIRECTED>
      psrch_D(ga, rec_colors, rvis);

      vertex_iterator verts = vertices(ga);

      #pragma mta assert parallel
      #pragma mta interleave schedule
      for (int i = 0; i < nVerts; ++i)
      {
        if (meetpt_mask[i])
        {
          vertex_t vert = verts[i];
          psrch_D.run(vert);
        }
      }

      #pragma mta assert parallel
      for (int i = 0; i < ret_id_mask_sz; ++i)
      {
        if (ret_id_mask[i]) short_path_ids.push_back(i);
      }

      freeit(ret_id_mask);

#ifdef DEBUGST
      print_arr(dist_from_vs, nVerts, "DVS");
      print_arr(dist_from_vt, nVerts, "DVT");
      print_arr(meetpt_mask, nVerts, "meetpt_mask");
#endif

    }
    else
    {
      // No path.
      distance = -1;

#ifdef DEBUGST
      printf("st_search reports no path found.\n");
#endif
    }

    freeit(rec_colors);
    freeit(dist_from_vs);
    freeit(dist_from_vt);
    freeit(meetpt_mask);

    return(distance);
  }

private:
  #pragma mta inline
  template <typename PTR>
  void freeit(PTR*& _ptr)
  {
    free(_ptr);
    _ptr = NULL;
  }

  template <typename visitor>
  void bfs_bidir(graph_adapter& ga, vertex_t v1, visitor& vis1,
                 vertex_t v2, visitor& vis2, bool* stop,
                 vertex_id_map<graph_adapter>& vid_map)
  {
    int v1id = get(vid_map, v1);
    int v2id = get(vid_map, v2);
    unsigned int nVerts = num_vertices(ga);

    int* disc_colormap = (int*) malloc(sizeof(int) * nVerts);

#ifdef DEBUGST
    printf("st_search::bfs_bidir()\n");
#endif

    #pragma mta assert no dependence
    #pragma mta block schedule
    for (int i = 0; i < nVerts; i++) disc_colormap[i] = 0;

    breadth_first_search<graph_adapter, int*, visitor>
    bfs_fwd(ga, disc_colormap, vis1, dirFwd);

    breadth_first_search<graph_adapter, int*, visitor>
    bfs_bwd(ga, disc_colormap, vis2, dirBwd);

    bfs_fwd.run(v1id);

    int qstart, qend;
    int fwd_frontier_sz = 0;
    int bwd_frontier_sz = 0;
    bool expand_fwd = true;

    if (!(*stop))
    {
      bfs_bwd.run(v2id);

      if (bfs_fwd.count <= bfs_bwd.count || bfs_bwd.count == 0)
      {
        // Do forward search.
        expand_fwd = true;
        qstart = 0;
        qend = bfs_fwd.count;
        bwd_frontier_sz = bfs_bwd.count;
      }
      else if (bfs_fwd.count > 0)
      {
        // Do backward search.
        expand_fwd = false;
        qstart = 0;
        qend = bfs_bwd.count;
        fwd_frontier_sz = bfs_fwd.count;
      }
    }

#ifdef DEBUGST
    cout << "*stop = " << *stop << endl;
    cout << "qend  = " << qend << endl;
    cout << "qstart= " << qstart << endl;
    cout << "fwd_frontier_size = " << fwd_frontier_sz << endl;
    cout << "bwd_frontier_size = " << bwd_frontier_sz << endl;
    cout << "expand_fwd        = " << expand_fwd << endl;
#endif

    while (!(*stop) && (qend - qstart) > 0)
    {
#ifdef DEBUGST
      printf("----------------- Expand Frontier -------------------\n");
#endif

      if (expand_fwd)
      {
        int* ptr_queue = bfs_fwd.Q.Q;

        #pragma mta assert parallel
        #pragma mta interleave schedule
        #pragma mta loop future
        for (int vi = qstart; vi < qend; vi++) bfs_fwd.run(ptr_queue[vi]);

        fwd_frontier_sz = bfs_fwd.count - qend;
      }
      else
      {
        int* ptr_queue = bfs_bwd.Q.Q;

        #pragma mta assert parallel
        #pragma mta interleave schedule
        #pragma mta loop future
        for (int vi = qstart; vi < qend; vi++) bfs_bwd.run(ptr_queue[vi]);

        bwd_frontier_sz = bfs_bwd.count - qend;
      }

      if ((fwd_frontier_sz <= bwd_frontier_sz || bwd_frontier_sz == 0) &&
          fwd_frontier_sz > 0)
      {
        // Do forward search.
        expand_fwd = true;
        qstart = bfs_fwd.count - fwd_frontier_sz;
        qend = bfs_fwd.count;
      }
      else if (bwd_frontier_sz > 0)
      {
        // Do backward search.
        expand_fwd = false;
        qstart = bfs_bwd.count - bwd_frontier_sz;
        qend = bfs_bwd.count;
      }
      else
      {
        *stop = 1;
      }
    }

    freeit(disc_colormap);
  }

  class vis_discover : public default_bfs_visitor<graph_adapter> {
  public:
    vis_discover(const int _vs, const int _vt, const int nVerts, bool* found,
                 int* length, const int direction, int* dist_from_vs,
                 int* dist_from_vt, int* rec_colors, bool* meetpt_mask,
                 vertex_id_map<graph_adapter>& vid_map) :
      vs(_vs), vt(_vt), _nVerts(nVerts), _direction(direction), _found(found),
      _length(length), _dist_from_vs(dist_from_vs), _dist_from_vt(dist_from_vt),
      _rec_colors(rec_colors), _meetpt_mask(meetpt_mask), _vid_map(vid_map)
    {
#ifdef DEBUGST
      printf("vis_discover::vis_discover()\n");
#endif

      *_found = false;
    }

    void tree_edge(edge_t& e, vertex_t& src, vertex_t& dest) const
    {
      int destID = get(_vid_map, dest);
      int srcID  = get(_vid_map, src);

#ifdef DEBUGST
      printf("vis_discover::tree_edge(%d->%d)\n", srcID, destID);
#endif

      if (_direction == DIR_FWD && _dist_from_vs[destID] == UNKNOWN)
      {
        _dist_from_vs[destID] = _dist_from_vs[srcID] + 1;
        _rec_colors[destID] = COLOR_WHITE;

        if (destID == vt)
        {
          *_found = true;
          *_length = _dist_from_vs[destID] + _dist_from_vt[destID];
          _meetpt_mask[destID] = true;
          _dist_from_vt[srcID] = _dist_from_vt[destID] + 1;
        }
      }
      else if (_direction == DIR_BWD && _dist_from_vt[destID] == UNKNOWN)
      {
        _dist_from_vt[destID] = _dist_from_vt[srcID] + 1;
        _rec_colors[destID] = COLOR_WHITE;

        if (destID == vs)
        {
          *_found = true;
          *_length = _dist_from_vs[destID] + _dist_from_vt[destID];
          _meetpt_mask[destID] = true;
          _dist_from_vs[srcID] = _dist_from_vt[destID] + 1;
        }
      }

#ifdef DEBUGST
      print_arr(_dist_from_vs, _nVerts, "DVS");
      print_arr(_dist_from_vt, _nVerts, "DVT");
      print_arr(_meetpt_mask, _nVerts, "meetpt_mask");
#endif
    }

    void back_edge(edge_t& e, vertex_t& src, vertex_t& dest) const
    {
      int destID = get(_vid_map, dest);
      int srcID  = get(_vid_map, src);

#ifdef DEBUGST
      printf("vis_discover::back_edge(%d->%d)\n", srcID, destID);
#endif

      if (_direction == DIR_FWD && _dist_from_vs[destID] == UNKNOWN)
      {
        _dist_from_vs[destID] = _dist_from_vs[srcID] + 1;
      }
      else if (_direction == DIR_BWD && _dist_from_vt[destID] == UNKNOWN)
      {
        _dist_from_vt[destID] = _dist_from_vt[srcID] + 1;
      }

      if ((_dist_from_vs[destID] != UNKNOWN) &&
          (_dist_from_vt[destID] != UNKNOWN))
      {
        *_found = true;
        _meetpt_mask[destID] = true;
        *_length = _dist_from_vs[destID] + _dist_from_vt[destID];
      }

#ifdef DEBUGST
      print_arr(_dist_from_vs, _nVerts, "DVS");
      print_arr(_dist_from_vt, _nVerts, "DVT");
      print_arr(_meetpt_mask, _nVerts, "meetpt_mask");
#endif
    }

private:
    const int vs, vt, _nVerts;
    bool* _found;
    int* _length;
    int* _dist_from_vs;
    int* _dist_from_vt;
    int* _rec_colors;
    bool* _meetpt_mask;
    const int _direction;
    vertex_id_map<graph_adapter>& _vid_map;
  };

  class vis_recorder : public default_psearch_visitor<graph_adapter> {
  public:
    vis_recorder(int* _dvs, int* _dvt, int _splen, int* _cmask,
                 bool* _ret_id_mask, const int _ret_data_type,
                 vertex_id_map<graph_adapter>& _vid_map,
                 edge_id_map<graph_adapter>& _eid_map, graph_adapter& _g) :
      dvs(_dvs), dvt(_dvt), splen(_splen), cmask(_cmask),
      ret_id_mask(_ret_id_mask), ret_data_type(_ret_data_type),
      vid_map(_vid_map), eid_map(_eid_map), g(_g) {}

    void pre_visit(vertex_t& v)
    {
      vid = get(vid_map, v);

#ifdef DEBUGST
      printf("vis_recorder::pre_visit(%d)\n", vid);
#endif
    }

    bool visit_test(edge_t& e, vertex_t& v)
    {
      bool ret = false;
      int sid = get(vid_map, source(e, g));
      int tid = get(vid_map, target(e, g));

#ifdef DEBUGST
      int oid = get(vid_map, other(e, v, g));

      printf("vis_recorder::visit_test(%d->%d)\n", sid, tid);
      printf("\tDV[u] = %d %d\n", dvs[sid], dvt[sid]);
      printf("\tDV[v] = %d %d\n", dvs[tid], dvt[tid]);
#endif

      if (dir == DIRECTED || dir == UNDIRECTED)
      {
        if (vid == sid && dvt[sid] == dvt[tid] + 1 && dvt[tid] != UNKNOWN)
        {
          ret = true;
        }

        if (vid == tid && dvs[tid] == dvs[sid] + 1 && dvs[sid] != UNKNOWN)
        {
          ret = true;
        }
      }

      if (dir == REVERSED || dir == UNDIRECTED)
      {
        if (vid == tid && dvt[tid] == dvt[sid] + 1 && dvt[sid] != UNKNOWN)
        {
          ret = true;
        }

        if (vid == sid && dvs[sid] == dvs[tid] + 1 && dvs[tid] != UNKNOWN)
        {
          ret = true;
        }
      }

      return(ret);
    }

    void tree_edge(edge_t& e, vertex_t& v)
    {
#ifdef DEBUGST
      int sid = get(vid_map, source(e, g));
      int tid = get(vid_map, target(e, g));
      int oth = get(vid_map, other(e, v, g));

      printf("vis_recorder::tree_edge(%d->%d)\n", sid, tid);
      printf("\tDV[u] = %d %d\n", dvs[sid], dvt[sid]);
      printf("\tDV[v] = %d %d\n", dvs[tid], dvt[tid]);
      printf("\tADD\n");
#endif

      if (ret_data_type == VERTICES)
      {
        ret_id_mask[ get(vid_map, source(e, g)) ] = true;
        ret_id_mask[ get(vid_map, target(e, g)) ] = true;
      }
      else if (ret_data_type == EDGES)
      {
        ret_id_mask[ get(eid_map, e) ] = true;
      }
    }

    void back_edge(edge_t& e, vertex_t& v)
    {
#ifdef DEBUGST
      int sid = get(vid_map, source(e, g));
      int tid = get(vid_map, target(e, g));
      int oth = get(vid_map, other(e, v, g));

      printf("vis_recorder::back_edge(%d->%d)\n", sid, tid);
      printf("\tDV[u] = %d %d\n", dvs[sid], dvt[sid]);
      printf("\tDV[v] = %d %d\n", dvs[tid], dvt[tid]);
      printf("ADD\n");
#endif

      if (ret_data_type == VERTICES)
      {
        ret_id_mask[ get(vid_map, source(e, g)) ] = true;
        ret_id_mask[ get(vid_map, target(e, g)) ] = true;
      }
      else if (ret_data_type == EDGES)
      {
        ret_id_mask[ get(eid_map, e) ] = true;
      }
    }

private:
    int vid;
    int* dvs;
    int* dvt;
    int splen;
    int* cmask;
    bool* ret_id_mask;
    int ret_data_type;
    vertex_id_map<graph_adapter>& vid_map;
    edge_id_map<graph_adapter>& eid_map;
    graph_adapter& g;
  };

  graph_adapter& ga;
  int vs;
  int vt;
  int distance;
  dynamic_array<int>& short_path_ids;
  int dirFwd;
  int dirBwd;

  /*! \fn st_search(graph_adapter& ga, int vs, int vt,
                    dynamic_array<int>& short_path_ids)
      \brief The constructor for the short path search. Takes a pointer
             to a graph and a source and target vertex id.  Returns the
             distance from S to T and saves a list of either edge or
             vertex IDs to the short_path_ids dynamic array.

      \param g   A reference to a graph_adapter.
      \param vs  The index # of the source vertex.
      \param vt  The index # of the target vertex.
      \param short_path_ids A dynamic array into which result ids are
                              saved (vertex ids or edge ids).
   */
  /*! \fn run(int retDataType=EDGES)
      \brief Executes the S-T Shortest Path search.
             The default mode is to return the shortest path as a list
             of edge-ids.  Changing the parameter retDataType to VERTICES
             will cause only the vertices on the short path to be returned.
             NOTE: returning only vertices may lose some information about
             the actual edge-wise path.

      \param retDataType Specifies what data is returned, EDGES (default)
                         causes short_path_ids to be filled with edge ids,
                         VERTICES will result in short_path_ids being filled
                         with vertex ids.
   */
};

}

#undef TIMER_SECT
#undef TIMER_RESTART
#undef TIMEIT

#endif

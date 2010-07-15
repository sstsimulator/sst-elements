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
/*! \file k_distance_neighborhood.hpp

    \brief The "k-neighbors" class.  This class takes a set of vertices,
           V, and a parameter, k, and returns V plus all vertices reachable
           from any vertices in V via "k" or fewer links.

    \author William McLendon (wcmclen@sandia.gov)

    \date 6/27/2007
*/
/****************************************************************************/

#ifndef MTGL_K_DISTANCE_NEIGHBORHOOD_HPP
#define MTGL_K_DISTANCE_NEIGHBORHOOD_HPP

#include <cstdio>
#include <climits>
#include <cassert>

#include <mtgl/mtgl_adapter.hpp>
#include <mtgl/graph_traits.hpp>
#include <mtgl/breadth_first_search.hpp>
#include <mtgl/util.hpp>
#include <mtgl/dynamic_array.hpp>

namespace mtgl {

// ---------------- begin class k_distance_neighborhood ------------------
/*! \brief This class implements the k-distance neighborhood algorithm.
           This algorithm takes a set of vertices, V, and a parameter, k,
           and returns a set of vertices containing V plus all
           vertices reachable from any vertices in V via "k" or
           fewer hops.
 */
template<typename graph_adapter, int dir = DIRECTED>
class k_distance_neighborhood {
public:
  typedef graph_traits<graph_adapter> graph_traits_t;
  typedef typename graph_traits_t::vertex_descriptor vertex_t;
  typedef typename graph_traits_t::edge_descriptor edge_t;
  typedef typename graph_traits_t::size_type size_type;

  /*! \param ga Reference to a graph adapter.
      \param k Provides the number of hops from src_vids to search.
      \param out_vids Reference to a dynamic_array into which the result
                      is saved.
   */
  k_distance_neighborhood(graph_adapter& _ga, int _k,
                          dynamic_array<int>& _out_vids) :
    ga(_ga), k_param(_k), out_vids(_out_vids)
  {
    size_type order = num_vertices(ga);
    cmap = (int*) malloc(sizeof(int) * order);

    #pragma mta assert no dependence
    for (size_type i = 0; i < order; i++) cmap[i] = 0;
  }

  ~k_distance_neighborhood() { free(cmap); cmap = NULL; }

  // ----------------- begin method run() --------------------
  /*! \param src_vids A dynamic array listing the vertex ids from
                      which we start our search.
                      NOTE: src_vids should never reference the SAME
                            array as out_vids.
      \param save_srcids [OPTIONAL] Defaults to true, this parameter
                         tells the search if we should include the
                         vertices in src_vids in the result as well.
   */
  void run(dynamic_array<int>& src_vids, bool save_srcids = true)
  {
    assert(&src_vids != NULL);
    assert(&out_vids != NULL);
    assert(&src_vids != &out_vids);

    if (k_param <= 0)
    {
      for (int i = 0; i < src_vids.size(); i++)
      {
        out_vids.push_back(src_vids[i]);
      }
    }
    else
    {
      vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, ga);

      #pragma mta assert no dependence
      for (int i = 0; i < src_vids.size(); i++)
      {
        cmap[ src_vids[i] ] = 1;

        if (save_srcids)
        {
          out_vids.push_back( src_vids[i] );
        }
      }

      visitor_t vis(out_vids, cmap, vid_map);

      breadth_first_search<graph_adapter, int*, visitor_t>
      bfs_expand(ga, cmap, vis, dir);

      for (int i = 0; i < src_vids.size(); i++)
      {
        bfs_expand.run(src_vids[i]);
      }

      if (k_param > 1)
      {
        int frontier_size = bfs_expand.count;
        int qstart = 0;
        int qend   = bfs_expand.count;
        int k_prime = k_param - 1;
        while (k_prime > 0 && qend - qstart > 0)
        {
          #pragma mta assert parallel
          #pragma mta interleave schedule
          #pragma mta loop future
          for (int i = qstart; i < qend; i++)
          {
            bfs_expand.run( bfs_expand.Q.Q[i] );
          }

          frontier_size = bfs_expand.count - qend;
          qstart = bfs_expand.count - frontier_size;
          qend   = bfs_expand.count;
          k_prime--;
        }
      }
    }
  }

  // ------------------- end method run() --------------------

  void set_k_param(int _k) { k_param = _k;    }
  int  get_k_param(void)   { return(k_param); }

protected:
  // --------------- begin class visitor_t -------------------
  class visitor_t : public default_bfs_visitor<graph_adapter>{
  public:
    visitor_t(dynamic_array<int>& _out_vids, int* _cmap,
              vertex_id_map<graph_adapter>& _vid_map) :
      out_vids(_out_vids), cmap(_cmap), vid_map(_vid_map) {}

    void tree_edge(edge_t& e, vertex_t& src, vertex_t& dest) const
    {
      int srcID = get(vid_map, src);
      int destID = get(vid_map, dest);
      out_vids.push_back(destID);
    }

  private:
    dynamic_array<int>& out_vids;
    int* cmap;
    vertex_id_map<graph_adapter>& vid_map;
  };
  // ----------------- end class visitor_t -------------------

private:
  int* cmap;
  graph_adapter& ga;
  int k_param;
  dynamic_array<int>& out_vids;
};
// ----------------- end class k_distance_neighborhood -------------------

};
// ------------------------ end namespace mtgl ---------------------------
#endif

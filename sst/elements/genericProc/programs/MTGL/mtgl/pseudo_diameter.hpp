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
/*! \file pseudo_diameter.hpp

    \brief This will be the class that will hold the methods and structures
           to do pseudo diameter.

    \author Vitus Leung (vjleung@sandia.gov)

    \date 6/9/2008
*/
/****************************************************************************/

#ifndef MTGL_PSEUDO_DIAMETER_HPP
#define MTGL_PSEUDO_DIAMETER_HPP

#include <mtgl/breadth_first_search_mtgl.hpp>

#ifdef __MTA__
#include <sys/mta_task.h>
#include <machine/runtime.h>
#endif

namespace mtgl {

template <typename graph_adapter>
class pd_visitor : public default_bfs_mtgl_visitor<graph_adapter> {
public:
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_t;

  pd_visitor(graph_adapter&gg, vertex_id_map<graph_adapter> vm, unsigned* nl) :
    ga(gg), vid_map(vm), node_level(nl) {}

  pd_visitor(const pd_visitor& pdv) :
    ga(pdv.ga), vid_map(pdv.vid_map), node_level(pdv.node_level) {}

  void bfs_root(vertex_t& v) const { node_level[get(vid_map, v)] = 0; }

  void tree_edge(edge_t& e, vertex_t& src) const
  {
    node_level[get(vid_map, target(e, ga))] = node_level[get(vid_map, src)] + 1;
  }

  const graph_adapter& ga;
  const vertex_id_map<graph_adapter> vid_map;
  unsigned* node_level;
};

// ====================== begin class pseudo_diameter =======================
template <typename graph_adapter>
class find_pseudo_diameter {
public:
  typedef typename graph_traits<graph_adapter>::vertex_iterator vertex_iterator;

  find_pseudo_diameter(graph_adapter& gg, unsigned* pseudo_diameter) :
    ga(gg), pd(pseudo_diameter), vid_map(get(_vertex_id_map, ga)) {}

  int run()
  {
    mt_timer timer1;
    int i, issues1, memrefs1, concur1, streams1;

    unsigned int order = num_vertices(ga);

    node_level = new unsigned[order];

    init_mta_counters(timer1, issues1, memrefs1, concur1, streams1);

    int* visited = new int[order];

    pd_visitor<graph_adapter> pdv(ga, vid_map, node_level);
    breadth_first_search_mtgl<graph_adapter, int*, pd_visitor<graph_adapter>,
                              DIRECTED, NO_FILTER, 100 > bfs(ga, visited,
                                                             pdv);

    random_value* rv = (random_value*) malloc(order * sizeof(random_value));
    rand_fill::generate(order, rv);

    int start_vertex = (int) rint((double) rv[order - 1] /
                                  rand_fill::get_max() * (order - 1));

    free(rv);

    vertex_iterator verts = vertices(ga);

    bfs.run(verts[start_vertex], true);

    int j = 0;

    for (i = 1; i < order; ++i)
    {
      if (visited[i])
      {
        if (node_level[i] > node_level[j]) j = i;
      }
      else
      {
        printf("DIRECTED GRAPH NOT STRONGLY CONNECTED!\n");
        exit(1);
      }
    }

    *pd = 0;

    while(*pd < node_level[j])
    {
      *pd = node_level[j];
      #pragma mta assert nodep
      for (i = 0; i < order; ++i) visited[i] = 0;

      bfs.run(verts[j], true);
      j = 0;

      for (i = 1; i < order; ++i)
      {
        if (visited[i])
        {
          if (node_level[i] > node_level[j]) j = i;
        }
        else
        {
          printf("DIRECTED GRAPH NOT STRONGLY CONNECTED!\n");
          exit(1);
        }
      }
    }

    sample_mta_counters(timer1, issues1, memrefs1, concur1, streams1);
    printf("\npseudo diameter took:\n");
    print_mta_counters(timer1, num_edges(ga), issues1, memrefs1, concur1,
                 streams1);
    long postticks = timer1.getElapsedTicks();
    return postticks;
  }

private:
  graph_adapter& ga;
  unsigned* node_level;
  unsigned* pd;
  vertex_id_map<graph_adapter> vid_map;
};

}

#endif

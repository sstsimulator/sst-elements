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
/*! \file vertex_betweenness.hpp

    \brief This will be the class that will hold the methods and structures
           to do vertex betweenness.

    \author Vitus Leung (vjleung@sandia.gov)

    \date 7/9/2008
*/
/****************************************************************************/

#ifndef MTGL_VERTEX_BETWEENNESS_HPP
#define MTGL_VERTEX_BETWEENNESS_HPP

#include <mtgl/breadth_first_search_mtgl.hpp>

#ifdef __MTA__
#include <sys/mta_task.h>
#include <machine/runtime.h>
#endif

using namespace mtgl;

namespace mtgl {

template <typename graph_adapter>
class vb_visitor : public default_bfs_mtgl_visitor<graph_adapter> {
public:
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_t;
  typedef typename graph_traits<graph_adapter>::size_type size_type;

  vb_visitor(graph_adapter& gg, vertex_id_map<graph_adapter>vm, size_type* d,
             size_type* sigma, size_type** Succ, size_type* Succ_count) :
    ga(gg), vid_map(vm), distance(d), path_count(sigma),
    successors(Succ), Succ_count(Succ_count) {}

  vb_visitor(const vb_visitor& vbv) :
    ga(vbv.ga), vid_map(vbv.vid_map), distance(vbv.distance),
    path_count(vbv.path_count), successors(vbv.successors),
    Succ_count(vbv.Succ_count) {}

  void tree_edge(edge_t& e, vertex_t& src) const
  {
    size_type w = get(vid_map, target(e, ga));
    size_type v = get(vid_map, src);
    mt_write(distance[w], distance[v] + 1);

    /* Add w to v's successor list */
    mt_incr(path_count[w], path_count[v]);
    successors[v][mt_incr(Succ_count[v], 1)] = w;
  }

  void back_edge(edge_t& e, vertex_t& src) const
  {
    size_type w = get(vid_map, target(e, ga));
    size_type v = get(vid_map, src);

    size_type dist = mt_readff(distance[w]);

    /* Add w to v's successor list */
    if (dist == distance[v] + 1)
    {
      mt_incr(path_count[w], path_count[v]);
      successors[v][mt_incr(Succ_count[v], 1)] = w;
    }
  }

  const graph_adapter& ga;
  const vertex_id_map<graph_adapter> vid_map;

  size_type* distance;
  size_type* path_count;
  size_type** successors;
  size_type* Succ_count;
};


/// An implementation of vertex betweenness centrality for unweighted graphs.
template <typename graph_adapter>
class find_vertex_betweenness {
public:
  typedef typename graph_traits<graph_adapter>::vertex_iterator vertex_iterator;
  typedef typename graph_traits<graph_adapter>::size_type size_type;

  /// \brief This constructor takes a graph adapter and an array in which to
  ///        write the betweenness centrality values to, and saves pointers
  ///        to these.
  ///
  /// \param ga A pointer to a graph adapter.
  /// \param BC A pointer to the array of vertex betweenness centrality
  ///           values.
  find_vertex_betweenness(graph_adapter& gg, double* res, size_type vb_par) :
    ga(gg), vBC(res), par(vb_par), vid_map(get(_vertex_id_map, ga)) {}

  /// Invokes the vertex betweenness centrality algorithm.
  size_type run()
  {
    typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex_t;

    mt_timer timer1;
    size_type issues1, memrefs1, concur1, streams1;

    /* Memory allocation and initialization */

    size_type n = num_vertices(ga);

    init_mta_counters(timer1, issues1, memrefs1, concur1, streams1);

    size_type** visited = (size_type**) malloc(par * sizeof(size_type*));

    /* d: distance from source vertex */
    size_type** d = (size_type**) malloc(par * sizeof(size_type*));

    /* sigma: no. of shortest paths from source to a given vertex */
    size_type** sigma = (size_type**) malloc(par * sizeof(size_type*));

    /* Succ: list of successors for a vertex. The successor list size
       is bounded by the out-degree
       (degree in the case of undirected graphs) */
    size_type*** Succ = (size_type***) malloc(par * sizeof(size_type**));

    /* Succ_count is needed to index the successor lists */
    size_type** Succ_count = (size_type**) malloc(par * sizeof(size_type*));

    /* Partial dependencies */
    double** delta = (double**) malloc(par * sizeof(double*));

    vertex_iterator verts = vertices(ga);
    #pragma mta assert nodep
    for (size_type i=0; i<n; i++) {
            vBC[i] = 0.0;
    }

    #pragma mta assert parallel
    for (size_type h = 0; h < par; h++)
    {
      visited[h] = (size_type*) malloc(n * sizeof(size_type));
      d[h] = (size_type*) malloc(n * sizeof(size_type));
      sigma[h] = (size_type*) malloc(n * sizeof(size_type));
      Succ[h] = (size_type**) malloc(n * sizeof(size_type*));

      #pragma mta assert parallel
      for (size_type i = 0; i < n; i++)
      {
        vertex_t v = verts[i];
        Succ[h][i] = (size_type*) malloc(out_degree(v, ga) * sizeof(size_type));
      }

      Succ_count[h] = (size_type*) malloc(n * sizeof(size_type));
      delta[h] = (double*) malloc(n * sizeof(double));
    }

    /* The betweenness centrality algorithm begins here */
    size_type* lock_vBC = (size_type*) malloc(n * sizeof(size_type));

    /* Loop: n iterations */
    for (size_type s = 0; s < n; s += par)

      #pragma mta assert parallel
      for (size_type h = 0; h < par; h++)
      {
        if (s + h < n)
        {
          vb_visitor<graph_adapter>vbv(ga, vid_map, d[h], sigma[h], Succ[h],
                                       Succ_count[h]);
          breadth_first_search_mtgl<graph_adapter, size_type*,
                                    vb_visitor<graph_adapter>, DIRECTED,
                                    NO_FILTER, 100 >
          bfs(ga, visited[h], vbv);

          /* s is the source vertex */
          vertex_t v = verts[s + h];

          /* BC of a degree-0 and degree-1 vertex is zero */
          /* Don't run BFS from degree-0 and 1 vertices */
          //if (degree(v, ga) < 2) continue;   // this perhaps for XMT, big data
          if (degree(v, ga) < 1) continue;

          /* initialize all internal variables for iteration */
          printf("blowing vBC away!\n");
          #pragma mta assert parallel
          for (size_type i = 0; i < n; i++)
          {
            visited[h][i] = 0;
            mt_purge(d[h][i]);
            sigma[h][i] = 0;
            Succ_count[h][i] = 0;
            delta[h][i] = 0;
          }

          /* Run BFS from source vertex s */
          mt_write(d[h][s + h], 0);
          sigma[h][s + h] = 1;

          bfs.run(v, true);
          if (bfs.count <= 0) continue;

          /* backward pass: accumulate dependencies */
          for (size_type i = d[h][bfs.Q.elem(bfs.count - 1)] - 1; i > 0; i--)
          {
            #pragma mta assert parallel
            for (size_type v = 0; v < n; v++)
            {
              if (d[h][v] == i)
              {
                double sigma_v = (double) sigma[h][v];
                #pragma mta assert parallel
                for (size_type j = 0; j < Succ_count[h][v]; j++)
                {
                  size_type w = Succ[h][v][j];
                  delta[h][v] += sigma_v / sigma[h][w] * (1 + delta[h][w]);
                }

                // s+h not in Q
                /* Using lock_vBC[v] for locking */
                size_type lock_v = mt_readfe(lock_vBC[v]);
                printf("before delta[%lu][%lu]: %f, vBC[%lu]: %f (%x)\n", h, v,
                       delta[h][v],v, vBC[v], vBC);
                vBC[v] += delta[h][v];
                printf("after delta[%lu][%lu]: %f, vBC[%lu]: %f (%x)\n", h, v,
                       delta[h][v],v, vBC[v], vBC);
                mt_write(lock_vBC[v], lock_v);
              }
            }
          }
        }
      }

    #pragma mta assert parallel
    for (size_type h = 0; h < par; h++)
    {
      free(visited[h]); free(d[h]);
      free(sigma[h]); free(delta[h]); free(Succ_count[h]);

      #pragma mta assert parallel
      for (size_type i = 0; i < n; i++) free(Succ[h][i]);

      free(Succ[h]);
    }

    free(Succ); free(visited); free(d);
    free(sigma); free(delta); free(Succ_count);
    free(lock_vBC);
    //sample_mta_counters(timer1, issues1, memrefs1, concur1, streams1);
    //print_mta_counters(timer1, num_edges(ga), issues1, memrefs1, concur1,
    //                   streams1);
    long postticks = timer1.getElapsedTicks();
    return postticks;
  }

private:
  graph_adapter& ga;
  double* vBC;
  size_type par;
  vertex_id_map<graph_adapter> vid_map;
};

}

#endif

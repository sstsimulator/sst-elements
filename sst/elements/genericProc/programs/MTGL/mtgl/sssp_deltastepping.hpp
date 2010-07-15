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
/*! \file array2d.hpp

    \brief Deltastepping SSSP code.

    \author William McLendon (wcmclen@sandia.gov)

    \date 1/7/2008
*/
/****************************************************************************/

#ifndef MTGL_SSSP_DELTASTEPPING_HPP
#define MTGL_SSSP_DELTASTEPPING_HPP

#include <cmath>

#include <mtgl/mtgl_adapter.hpp>
#include <mtgl/visit_adj.hpp>
#include <mtgl/util.hpp>

namespace mtgl {

#define MTGL_USE_VISITADJ   0    // enable the visit_adj() use via MTGL
#define MEASURE_TRAPS       0

#define DEBUG       0
#define FREEMEM     1

#define SSSP_INIT_SIZE 100

// sort weights ==> reordering edges in the underlying graph, which is a
//                  side-effect.  FWIW, these algorithms should be side-effect
//                  free thus this option should be disabled.

// Some useful tools for debugging / printing messages
//#define PRINTDEBUG  0
//#if PRINTDEBUG
//  #define INFO(...) {printf("INFO :\t");printf(__VA_ARGS__);fflush(stdout);}
//#else
//  #define INFO(...) ;
//#endif

#if DEBUG
        #define mt_assert(x) if (!x) {\
    printf("%s:%i: assertion failed: ", __FILE__, __LINE__);\
    assert(x);\
}
#endif

#define CREATE_TIMER(t)  mt_timer t;
#define START_TIMER(t)   t.start();
#define STOP_TIMER(t, tag)    {\
    t.stop();\
    printf("%25s\t%9.4f\n", tag, t.getElapsedSeconds()); fflush(stdout);\
}

#if defined(__MTA__) && MEASURE_TRAPS
        #define START_TRAP(t) t = mta_get_task_counter(RT_TRAP);
        #define STOP_TRAP(t, tag) {\
    t = mta_get_task_counter(RT_TRAP) - t;\
    printf("%25s\t%9d\n", tag, t); fflush(stdout);\
}
#else
        #define START_TRAP(t)     ;
        #define STOP_TRAP(t, tag) ;
#endif

template<class graph_t, class timestamp_t>
class sssp_deltastepping {

  typedef typename graph_traits<graph_t>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph_t>::edge_descriptor edge_t;
  typedef typename graph_traits<graph_t>::out_edge_iterator EITER_T;
  typedef typename graph_traits<graph_t>::vertex_iterator VITER_T;

public:
  // G        [ IN] - reference to graph adapter.
  // s        [ IN] - source vertex id
  // realWt   [ IN] - edge weights (|E|)
  // cs       [OUT] - pointer to checksum value (scalar)
  // result   [OUT] - vertex weights (|V|)
  // vertTime [ IN] - vertex time stamps (optional) (|V|)
  sssp_deltastepping(graph_t& _G, int _s, double* _realWt,
                     double* _cs, double* _result,
                     timestamp_t* _vertTime = NULL)
    :   G(_G), s(_s), realWt(_realWt), cs(_cs), result(_result),
    vertTime(_vertTime)
  {
    //printAdjacency(s);
    //printGraph();
  }

  double run(void)
  {
    double delta, INFTY;

    unsigned int m, n;
    int* numEdges, * endV;
    double* Wt, * d, * dR;
    int** Buckets, numBuckets, * Bcount, * Bsize, * Braise;
    int* Bdel, * Bt, * Btemp;
    int Btemp_count, Bdel_t, Bcount_t;
    int currBucketNum, lastBucketNum, * vBucketMap, * vBucketLoc;
    int* S, * SLoc, Scount, * R, * RLoc, Rcount;
    int* memBlock;
    double delta_l, INFTY_l;
    int sum, not_connected;
    double Wsum, checksum;

    int __traps;

    vertex_id_map<graph_t> vid_map = get(_vertex_id_map, G);
    edge_id_map<graph_t>   eid_map = get(_edge_id_map,   G);

    mt_timer mttimer;

#if DEBUG
    mt_assert(realWt != NULL);
    mt_assert(result != NULL);

    int numPhases, * numVertsInPhase, numRelaxations,
        numRelaxationsHeavy, maxBucketNum;
#endif

    //s = 1;
    m = num_edges(G);
    n = num_vertices(G);

    VITER_T verts = vertices(G);

    int Gn = n;

    Wt = realWt;

    double maxWt = 0.0;
    for (int i = 0; i < m; i++)
    {
      if (Wt[i] > maxWt)
      {
        maxWt = Wt[i];
      }
    }

    INFTY = maxWt * 20;              // 20 is a surrogate for diameter.
    delta = ((double) n) / ((double) m);
    delta_l = delta;
    INFTY_l = INFTY;
    numBuckets = (int) (INFTY / delta + 1);    //(int)(1.0/delta) * (n/2);

#if DEBUG
    fprintf(stderr, "source: %d\n", s);
    fprintf(stderr, "delta: %lf\n", delta);
    fprintf(stderr, "No. of buckets: %d\n", numBuckets);

    /* Assuming a max of n phases
       Storing the no. of verts visited in each phase
       for statistics */
    numVertsInPhase = (int*) calloc(n, sizeof(int));
#endif

    mttimer.start();

    /* Memory allocation */
    #pragma mta trace "malloc start"
    Buckets = (int**) malloc(numBuckets * sizeof(int*));
    Bsize   = (int*) calloc(numBuckets,   sizeof(int));
    Bcount  = (int*) calloc(numBuckets,   sizeof(int));
    Braise  = (int*) calloc(4 * numBuckets, sizeof(int));
    Bdel    = (int*) calloc(numBuckets,   sizeof(int));

    #pragma mta trace "memblock malloc start"
#ifdef __MTA__
    memBlock = (int*) malloc((9 * n + 1) * sizeof(int));
#else
    if (sizeof(int) == 4) memBlock = (int*) malloc((11 * n + 2) * sizeof(int));
    if (sizeof(int) == 8) memBlock = (int*) malloc((9 * n + 1) * sizeof(int));
#endif

    #pragma mta trace "memblock malloc end"
    S          = memBlock;
    R          = memBlock + n;
    SLoc       = memBlock + 2 * n;
    RLoc       = memBlock + 3 * n;
    Btemp      = memBlock + 4 * n;
    vBucketMap = memBlock + 5 * n;
    vBucketLoc = memBlock + 6 * n;
    dR = (double*)(memBlock + 7 * n);

#ifdef __MTA__
    d = (double*)(memBlock + 8 * n);
#else
    if (sizeof(int) == 4) d = (double*)(memBlock + 9 * n);
    if (sizeof(int) == 8) d = (double*)(memBlock + 8 * n);
#endif

    /* Initializing the bucket data structure */
    #pragma mta assert nodep
    for (int i = 0; i < n; i++)
    {
      vBucketMap[i] = -1;
      d[i]    = INFTY_l;
      RLoc[i] = -1;
      SLoc[i] = -1;
    }
    d[n] = INFTY_l;
    #pragma mta trace "malloc end"

    R[0]   = s;
    dR[0]  = 0;
    Rcount = 0;
    Scount = 0;
//CREATE_TIMER(__t1);
//START_TIMER(__t1);
    #pragma mta trace "relax s start"
    lastBucketNum = relax(R, dR, RLoc, 1, G, d, Buckets, 0,
                          numBuckets, Bsize, Bcount, Braise,
                          Bdel, vBucketMap, vBucketLoc, delta_l,
                          INFTY_l);
    #pragma mta trace "relax s end"
//STOP_TIMER(__t1, "relax s");

#if DEBUG
    numRelaxations = 1;
    numRelaxationsHeavy = 0;
    numPhases = 0;
    maxBucketNum = 0;
#endif

    currBucketNum = 0;
    while (currBucketNum <= lastBucketNum)
    {
      if (Bcount[currBucketNum] == 0)
      {
        currBucketNum++;
        continue;
      }

      /* Handle light edges */
      while (Bcount[currBucketNum] != 0)
      {
        Bcount_t = Bcount[currBucketNum];
        Bdel_t = Bdel[currBucketNum];
        Bt = Buckets[currBucketNum];
        Btemp_count = 0;
        Rcount = 0;

//CREATE_TIMER(__t0);
//START_TIMER(__t0);

        if (Bdel_t == Bcount_t)
        {
          Btemp_count = 0;

          /* The bucket doesn't have a lot of empty spots */
        }
        else if (Bdel_t < Bcount_t / 3 + 2)
        {
//          INFO("\tlight nodup\n");
          START_TRAP(__traps);
//CREATE_TIMER(__t2);
//START_TIMER(__t2);
          Btemp_count = Bcount_t;
          if (Bcount_t > 30)
          {
            #pragma mta trace "light nodup start"
#if MTGL_USE_VISITADJ
            #pragma mta assert parallel
            #pragma mta block dynamic schedule
            for (int i = 0; i < Bcount_t; i++)
            {
              const int u = Bt[i];                                              // source vert
              if (u == Gn) continue;
              const double du = d[u];
              light_collect_visadj_t lnodup_vis(G, delta_l, Wt,
                                                d, du, dR, Rcount, R, RLoc,
                                                vertTime, vid_map, eid_map);
              visit_adj<graph_t, light_collect_visadj_t>(G, u, lnodup_vis, 1);
            }
#else
            #pragma mta assert parallel
//            #pragma mta block dynamic schedule
            #pragma mta loop future
            for ( int i = 0; i < Bcount_t; i++)
            {
              const int u = Bt[i];                                                      // source vert
              if (u == Gn) continue;
              const double du = d[u];

              const vertex_t vu = verts[u];
              const int deg = out_degree(vu, G);
              EITER_T inc_edges = out_edges(vu, G);

              #pragma mta assert parallel
              for (int ineigh = 0; ineigh < deg; ineigh++)
              {
                edge_t e = inc_edges[ineigh];
                const int j = get(eid_map, e);
                const int v = get(vid_map, target(e, G));
                //INFO("\tP light nodup: e[%d]  %d->%d\n",j,u,v);
                if (vertTime == NULL || vertTime[u] <= vertTime[v])
                {
                  if ( du + Wt[j] < d[v] )
                  {
                    if (delta_l > Wt[j])
                    {
                      int rlv = mt_readfe(RLoc[v]);
                      if (rlv == -1)
                      {
                        int pos = mt_incr(Rcount, 1);
                        R[pos]  = v;
                        dR[pos] = du + Wt[j];
                        mt_write(RLoc[v], pos);
                      }
                      else
                      {
                        if (du + Wt[j] < dR[rlv])
                        {
                          dR[rlv] = du + Wt[j];
                        }
                        mt_write(RLoc[v], rlv);
                      }
                    }
                  }
                }
              }
            }
#endif
            #pragma mta trace "light nodup end"
          }
          else
          {
#if MTGL_USE_VISITADJ
            for (int i = 0; i < Bcount_t; i++)
            {
              int u = Bt[i];
              if (u == Gn) continue;
              double du = d[u];
              light_collect_visadj_t light_vis(G, delta_l, Wt,
                                               d, du, dR, Rcount, R, RLoc,
                                               vertTime, vid_map, eid_map);
              visit_adj<graph_t, light_collect_visadj_t>(G, u, light_vis, 1);
            }
#else
            for (int i = 0; i < Bcount_t; i++)
            {
              const int u  = Bt[i];
              if (u == Gn) continue;
              const double du  = d[u];

              vertex_t vu  = verts[u];
              const int deg  = out_degree(vu, G);
              EITER_T inc_edges = out_edges(vu, G);

              #pragma mta assert parallel
              for (int ineigh = 0; ineigh < deg; ineigh++)
              {
                edge_t e = inc_edges[ineigh];
                const int j = get(eid_map, e);
                const int v = get(vid_map, target(e, G));
                //INFO("\tS light nodup: e[%3d]  %2d->%-2d\tj u v\n",j,u,v);fflush(stdout);
                if (vertTime == NULL || vertTime[u] <= vertTime[v])
                {
                  if ( du + Wt[j] < d[v] )
                  {
                    if (delta_l > Wt[j])
                    {
                      int rlv = mt_readfe(RLoc[v]);
                      if (rlv == -1)
                      {
                        int pos = mt_incr(Rcount, 1);
                        R[pos] = v;
                        dR[pos] = du + Wt[j];
                        mt_write(RLoc[v], pos);
                      }
                      else
                      {
                        if (du + Wt[j] < dR[rlv])
                        {
                          dR[rlv] = du + Wt[j];
                        }
                        mt_write(RLoc[v], rlv);
                      }
                    }
                  }
                }
              }
            }
#endif
          }

//STOP_TIMER(__t2, "light nodup");
          STOP_TRAP(__traps, "light nodup traps");
//CREATE_TIMER(__t3);
//START_TIMER(__t3);

          /* Add to S */
          if (Bcount_t > 30)
          {
            #pragma mta trace "light nodup S start"
            #pragma mta assert parallel
            for (int i = 0; i < Bcount_t; i++)
            {
              int slv;
              int Gn = n;
              int u = Bt[i];

              if (u == Gn)
              {
                continue;
              }

              slv = mt_readfe(SLoc[u]);

              /* Add the vertex to S */
              if (slv == -1)
              {
                int pos = mt_incr(Scount, 1);
                S[pos] = u;
                mt_write(SLoc[u], pos);
              }
              else
              {
                mt_write(SLoc[u], slv);
              }

              vBucketMap[u] = -1;
              vBucketLoc[u] = -1;
            }

            #pragma mta trace "light nodup S end"
          }
          else
          {
            for (int i = 0; i < Bcount_t; i++)
            {
              int slv;
              int Gn = n;
              int u = Bt[i];

              if (u == Gn)
              {
                continue;
              }

              slv = mt_readfe(SLoc[u]);

              /* Add the vertex to S */
              if (slv == -1)
              {
                int pos = mt_incr(Scount, 1);
                S[pos] = u;
                mt_write(SLoc[u], pos);
              }
              else
              {
                mt_write(SLoc[u], slv);
              }

              vBucketMap[u] = -1;
              vBucketLoc[u] = -1;
            }
          }
//STOP_TIMER(__t3, "light nodup S");
        }
        else
        {
          /* Bdel_t > Bcount_t/3  */
          /* There are a significant number of empty spots in the bucket.
           * So we get the non-empty vertices and store them in a compact
           * array */

          int Gn = n;

//CREATE_TIMER(__t4);
//START_TIMER(__t4);
          if (Bcount_t > 30)
          {
            #pragma mta trace "light dup filter start"
            #pragma mta assert nodep
            #pragma mta interleave schedule
            // #pragma mta use 60 streams
            for (int i = 0; i < Bcount_t; i++)
            {
              int u = Bt[i];
              if (u != Gn)
              {
                int pos = mt_incr(Btemp_count, 1);
                Btemp[pos] = u;
              }
            }

            #pragma mta trace "light dup filter end"
          }
          else
          {
            for (int i = 0; i < Bcount_t; i++)
            {
              int u = Bt[i];
              if (u != Gn)
              {
                int pos = mt_incr(Btemp_count, 1);
                Btemp[pos] = u;
              }
            }
          }
//STOP_TIMER(__t4, "light dup filter");

          /* The nested loop can be collapsed, but this results
           * in a lot of hotspots */

//CREATE_TIMER(__t5);
//START_TIMER(__t5);

//          INFO("\tlight dup\n");
          if (Btemp_count > 30)
          {
#if MTGL_USE_VISITADJ
            #pragma mta trace "light dup start"
            #pragma mta assert parallel
            for (int i = 0; i < Btemp_count; i++)
            {
              int u = Btemp[i];
              double du = d[u];
              light_collect_visadj_t light_vis(G, delta_l, Wt,
                                               d, du, dR, Rcount, R, RLoc,
                                               vertTime, vid_map, eid_map);
              // visitadj in serial (note parcutoff = m = |E|).
              visit_adj<graph_t, light_collect_visadj_t>(G, u, light_vis, m);
            }
#else
            #pragma mta trace "light dup start"
            #pragma mta assert parallel
            for (int i = 0; i < Btemp_count; i++)
            {
              const int u = Btemp[i];
              const double du = d[u];

              vertex_t vu = verts[u];
              const int deg = out_degree(vu, G);
              EITER_T inc_edges = out_edges(vu, G);

              for (int ineigh = 0; ineigh < deg; ineigh++)
              {
                edge_t e = inc_edges[ineigh];
                const int j = get(eid_map, e);
                const int v = get(vid_map, target(e, G));
                //INFO("\tP light dup: e[%2d]  %2d->%-2d\tj u v\n",j,u,v);
                if (vertTime == NULL || vertTime[u] <= vertTime[v])
                {
                  if ( du + Wt[j] < d[v] )
                  {
                    if (delta_l > Wt[j])
                    {
                      int rlv = mt_readfe(RLoc[v]);
                      if (rlv == -1)
                      {
                        int pos = mt_incr(Rcount, 1);
                        R[pos] = v;
                        dR[pos] = du + Wt[j];
                        mt_write(RLoc[v], pos);
                      }
                      else
                      {
                        if (du + Wt[j] < dR[rlv])
                        {
                          dR[rlv] = du + Wt[j];
                        }
                        mt_write(RLoc[v], rlv);
                      }
                    }
                  }
                }
              }
            }

            #pragma mta trace "light dup end"
#endif                                                  // MTGL_USE_VISITADJ
          }
          else
          {
#if MTGL_USE_VISITADJ
            for (int i = 0; i < Btemp_count; i++)
            {
              int u     = Btemp[i];
              double du = d[u];
              light_collect_visadj_t light_vis(G, delta_l, Wt,
                                               d, du, dR, Rcount, R, RLoc,
                                               vertTime, vid_map, eid_map);
              // visitadj in serial (note parcutoff = m = |E|).
              visit_adj<graph_t, light_collect_visadj_t>(G, u, light_vis, m);
            }

#else
            for (int i = 0; i < Btemp_count; i++)
            {
              const int u = Btemp[i];
              const double du = d[u];

              vertex_t vu = verts[u];
              const int deg = out_degree(vu, G);
              EITER_T inc_edges = out_edges(vu, G);

              for (int ineigh = 0; ineigh < deg; ineigh++)
              {
                edge_t e = inc_edges[ineigh];
                const int j = get(eid_map, e);
                const int v = get(vid_map, target(e, G));
                //INFO("\tP light dup: e[%2d]  %2d->%-2d\tj u v\n",j,u,v);
                if (vertTime == NULL || vertTime[u] <= vertTime[v])
                {
                  if ( du + Wt[j] < d[v] )
                  {
                    if (delta_l > Wt[j])
                    {
                      int rlv = mt_readfe(RLoc[v]);
                      if (rlv == -1)
                      {
                        int pos = mt_incr(Rcount, 1);
                        R[pos] = v;
                        dR[pos] = du + Wt[j];
                        mt_write(RLoc[v], pos);
                      }
                      else
                      {
                        if (du + Wt[j] < dR[rlv])
                        {
                          dR[rlv] = du + Wt[j];
                        }
                        mt_write(RLoc[v], rlv);
                      }
                    }
                  }
                }
              }
            }
#endif                                                  // MTGL_USE_VISITADJ
          }

//STOP_TIMER(__t5, "light dup");
//CREATE_TIMER(__t6);
//START_TIMER(__t6);
//          INFO("\tlight dup S\n");
          if (Btemp_count > 30)
          {
            #pragma mta trace "light dup S start"
            #pragma mta assert parallel
            for (int i = 0; i < Btemp_count; i++)
            {
              int u   = Btemp[i];
              int slv = mt_readfe(SLoc[u]);

              /* Add the vertex to S */

              if (slv == -1)
              {
                int pos = mt_incr(Scount, 1);
                S[pos] = u;
                mt_write(SLoc[u], pos);
              }
              else
              {
                mt_write(SLoc[u], slv);
              }
              vBucketMap[u] = -1;
              vBucketLoc[u] = -1;
            }

            #pragma mta trace "light dup S end"
          }
          else
          {
            for (int i = 0; i < Btemp_count; i++)
            {
              int u   = Btemp[i];
              int slv = mt_readfe(SLoc[u]);

              /* Add the vertex to S */

              if (slv == -1)
              {
                int pos = mt_incr(Scount, 1);
                S[pos] = u;
                mt_write(SLoc[u], pos);
              }
              else
              {
                mt_write(SLoc[u], slv);
              }
              vBucketMap[u] = -1;
              vBucketLoc[u] = -1;
            }
          }
//STOP_TIMER(__t6, "light dup S");
        }        /* end of if .. then .. else */
                 /* We have collected all the light edges in R */

//STOP_TIMER(__t0, "collect edges");

#if DEBUG
        if (Btemp_count != 0) { maxBucketNum = currBucketNum; }
#endif

        Bcount[currBucketNum] = 0;
        Bdel[currBucketNum] = 0;

#if DEBUG
        numVertsInPhase[numPhases++] = Rcount;
        numRelaxations += Rcount;
#endif

        /* Relax all light edges */
        if (Rcount != 0)
        {
          lastBucketNum = relax(R, dR, RLoc, Rcount, G, d, Buckets,
                                currBucketNum, numBuckets, Bsize,
                                Bcount, Braise, Bdel, vBucketMap,
                                vBucketLoc, delta_l, INFTY_l);
        }

      }                         /* inner-most while end */

      Rcount = 0;

//CREATE_TIMER(__hvy);
//START_TIMER(__hvy);

      /* Collect heavy edges into R */
//      INFO("\theavy collect\n");
      if (Scount > 10)
      {
#if MTGL_USE_VISITADJ
        #pragma mta trace "heavy collect start"
        #pragma mta assert parallel
        #pragma mta block dynamic schedule
        for (int i = 0; i < Scount; i++)
        {
          int u     = S[i];
          SLoc[u]   =   -1;
          double du = d[u];
          heavy_collect_visadj_t hc_vis(G, delta_l, Wt, d, du, dR,
                                        Rcount, R, RLoc, vertTime,
                                        vid_map, eid_map);
          visit_adj<graph_t, heavy_collect_visadj_t>(G, u, hc_vis, m);
        }
#else

        #pragma mta trace "heavy collect start"
        #pragma mta assert parallel
        #pragma mta block dynamic schedule
        for (int i = 0; i < Scount; i++)
        {
          const int u = S[i];
          SLoc[u]         = -1;
          const double du = d[u];

          vertex_t vu = verts[u];
          const int deg = out_degree(vu, G);
          EITER_T inc_edges = out_edges(vu, G);

          for (int ineigh = 0; ineigh < deg; ineigh++)
          {
            edge_t e = inc_edges[ineigh];
            const int j = get(eid_map, e);
            const int v = get(vid_map, target(e, G));
            //INFO("\tP heavy collect: e[%2d] %2d->%-2d  j u v\n",j,u,v);
            if (vertTime == NULL || vertTime[u] <= vertTime[v])
            {
              if (Wt[j] + du < d[v])
              {
                if (delta_l <= Wt[j])                                                           // SORTW
                {
                  const int rlv = mt_readfe(RLoc[v]);
                  if (rlv == -1)
                  {
                    int pos = mt_incr(Rcount, 1);
                    R[pos]  = v;
                    dR[pos] = d[u] + Wt[j];
                    mt_write(RLoc[v], pos);
                  }
                  else
                  {
                    if (du + Wt[j] < dR[rlv])
                    {
                      dR[rlv] = du + Wt[j];
                    }
                    mt_write(RLoc[v], rlv);
                  }
                }
              }
            }
          }
        }

        #pragma mta trace "heavy collect end"
#endif

      }
      else
      {
#if MTGL_USE_VISITADJ
        for (int i = 0; i < Scount; i++)
        {
          int u     = S[i];
          SLoc[u]   = -1;
          double du = d[u];
          heavy_collect_visadj_t hc_vis(G, delta_l, Wt, d, du, dR,
                                        Rcount, R, RLoc, vertTime,
                                        vid_map, eid_map);
          visit_adj<graph_t, heavy_collect_visadj_t>(G, u, hc_vis, m);
        }
#else
        for (int i = 0; i < Scount; i++)
        {
          const int u = S[i];
          SLoc[u]         = -1;
          const double du = d[u];

          vertex_t vu = verts[u];
          const int deg = out_degree(vu, G);
          EITER_T inc_edges = out_edges(vu, G);

          for (int ineigh = 0; ineigh < deg; ineigh++)
          {
            edge_t e = inc_edges[ineigh];
            const int j = get(eid_map, e);
            const int v = get(vid_map, target(e, G));
            //INFO("\tS heavy collect: e[%2d] %2d->%-2d  j u v\n",j,u,v);
            if ( vertTime == NULL || vertTime[u] <= vertTime[v])
            {
              if (Wt[j] + du < d[v])
              {
                if (delta_l <= Wt[j])                                                           // SORTW
                {
                  const int rlv = mt_readfe(RLoc[v]);
                  if (rlv == -1)
                  {
                    const int pos = mt_incr(Rcount, 1);
                    R[pos]  = v;
                    dR[pos] = d[u] + Wt[j];
                    mt_write(RLoc[v], pos);
                  }
                  else
                  {
                    if (du + Wt[j] < dR[rlv])
                    {
                      dR[rlv] = du + Wt[j];
                    }
                    mt_write(RLoc[v], rlv);
                  }
                }
              }
            }
          }
        }
#endif
      }
//STOP_TIMER(__hvy, "heavy collect");
      Scount = 0;

      /* Relax heavy edges */

      #pragma mta trace "heavy relax start"
      if (Rcount != 0)
      {
//CREATE_TIMER(__tmr);
//START_TIMER(__tmr);
        lastBucketNum = relax(R, dR, RLoc, Rcount, G, d, Buckets,
                              currBucketNum, numBuckets, Bsize, Bcount,
                              Braise, Bdel, vBucketMap, vBucketLoc,
                              delta_l, INFTY_l);
//STOP_TIMER(__tmr, "heavy relax");
      }

      #pragma mta trace "heavy relax end"

#if DEBUG
      numRelaxationsHeavy += Rcount;
#endif
    }

    mttimer.stop();
    double running_time = mttimer.getElapsedSeconds();

    /* Compute d checksum  AND save vert weights into result */
    checksum = 0;
    not_connected = 0;

    #pragma mta assert parallel
    for (int i = 0; i < n; i++)
    {
      result[i] = d[i];   // Save Result.
      if (d[i] < INFTY_l)
      {
        checksum = checksum + d[i];
      }
      else
      {
        mt_incr(not_connected, 1);
      }
    }
    *cs = checksum;

#if DEBUG
    /* Compute W checksum */
    Wsum = 0;
    for (int i = 0; i < m; i++)
    {
      Wsum = Wsum + Wt[i];
    }

    *cs = checksum;
    fprintf(stderr, "d checksum: %lf, W checksum: %lf, Avg. distance %lf\n",
            checksum, Wsum, checksum / (n - not_connected));
    fprintf(stderr, "Last non-empty bucket: %d\n", maxBucketNum);
    fprintf(stderr, "No. of phases: %d\n", numPhases);
    fprintf(stderr, "No. of light relaxations: %d\n", numRelaxations);
    fprintf(stderr, "No. of heavy relaxations: %d\n", numRelaxationsHeavy);
    fprintf(stderr, "Avg. no. of light edges relaxed in a phase: %d\n",
            numRelaxations / numPhases);

    if (maxBucketNum != 0)
    {
      fprintf(stderr, "Avg. no. of heavy edges relaxed per bucket: %d\n",
              numRelaxationsHeavy / maxBucketNum);
    }

    fprintf(stderr, "Total no. of relaxations: %d\n\n",
            numRelaxations + numRelaxationsHeavy);

    fflush(stderr);
#endif

#if FREEMEM
    /* Free memory */
    #pragma mta assert parallel
    for (int i = 0; i < numBuckets; i++)
    {
      if (Bsize[i] != 0) free(Buckets[i]);
    }

    free(Buckets);
    free(Bsize);
    free(Bcount);
    free(Braise);
    free(Bdel);
    free(memBlock);
#if DEBUG
    free(numVertsInPhase);
#endif
#endif
//    INFO("\tChecksum = %6.1lf\n", checksum);
//    INFO("<<<\tsssp_deltastepping()\n");
    return running_time;
  }

  //-----[ end run() ]--------------------------------------------------

  //-----[ relax() ]----------------------------------------------------
  #pragma mta inline
  int relax(int* R,
            double* dR,
            int* RLoc,
            int Rcount,
            graph_t& G,
            double* d,
            int** Buckets,
            int currBucketNum,
            int numBuckets,
            int* Bsize,
            int* Bcount,
            int* Braise,
            int* Bdel,
            int* vBucketMap,
            int* vBucketLoc,
            double delta,
            double INFTY)
  {
    int i, offset;
    double delta_l = delta;
    double INFTY_l = INFTY;
    unsigned int Gn = num_vertices(G);
    int lastBucketNum = -1;

    //INFO(">>>\trelax()\n");
    if (Rcount > 30)
    {
      #pragma mta trace "loop 1 start"
      #pragma mta assert nodep
      #pragma mta interleave schedule
      for (i = 0; i < Rcount; i++)
      {
        int v      = R[i];
        int bn     = (int) floor(dR[i] / delta_l);
        int bn_old = vBucketMap[v];
        int offset = numBuckets * (i & 3);

        if (bn > lastBucketNum)
        {
          lastBucketNum = bn;
        }
        if (bn >= numBuckets)
        {
          bn = numBuckets - 1;
        }
        /*
        if (bn >= numBuckets) {
                fprintf(stderr, "Error: relaxation failed, "
                            "bn: %d, numBuckets: %d\n",
                            bn, numBuckets);
                exit(1);
        }
        */
        RLoc[v] = (i & 3) * Gn + mt_incr(Braise[bn + offset], 1);

        if ((d[v] < INFTY_l) && (bn_old != -1))
        {
          Buckets[bn_old][vBucketLoc[v]] = Gn;
          Bdel[bn_old]++;
        }
      }

      #pragma mta trace "loop 1 end"
    }
    else
    {
      for (i = 0; i < Rcount; i++)
      {
        int v = R[i];
        int bn = (int) floor(dR[i] / delta_l);
        int bn_old = vBucketMap[v];
        int offset = numBuckets * (i & 3);

        if (bn > lastBucketNum)
        {
          lastBucketNum = bn;
        }
        if (bn >= numBuckets)
        {
          bn = numBuckets - 1;
        }
        RLoc[v] = (i & 3) * Gn + mt_incr(Braise[bn + offset], 1);

        if ((d[v] < INFTY_l) && (bn_old != -1))
        {
          Buckets[bn_old][vBucketLoc[v]] = Gn;
          Bdel[bn_old]++;
        }
      }
    }

    lastBucketNum++;
    // fprintf(stderr, "[%d %d] ", currBucketNum, lastBucketNum);

    #pragma mta trace "loop 2 start"
    for (i = currBucketNum; i < lastBucketNum; i++)
    {
      int* Bi = Buckets[i];
      int Bsize_i = Bsize[i];
      int size_incr = Braise[i] +
                      Braise[i + 1 * numBuckets] +
                      Braise[i + 2 * numBuckets] +
                      Braise[i + 3 * numBuckets];

      if ((size_incr > 0) && (Bcount[i] + size_incr >= Bsize[i]))
      {
        int Bsize_tmp = Bcount[i] + size_incr + SSSP_INIT_SIZE;
        int* Bt = (int*) malloc(Bsize_tmp * sizeof(int));

        if (Bsize_i != 0)
        {
          if (Bsize_i > 30)
          {
            #pragma mta assert nodep
            #pragma mta interleave schedule
            // #pragma mta use 60 streams
            for (int j = 0; j < Bsize_i; j++)
            {
              Bt[j] = Bi[j];
            }
          }
          else
          {
            for (int j = 0; j < Bsize_i; j++)
            {
              Bt[j] = Bi[j];
            }
          }
          free(Bi);
        }
        Buckets[i] = Bt;
        Bsize[i] = Bsize_tmp;
      }
    }

    #pragma mta trace "loop 2 end"

    if (Rcount > 30)
    {
      #pragma mta trace "loop 3 start"
      #pragma mta assert nodep
      #pragma mta interleave schedule
      for (i = 0; i < Rcount; i++)
      {
        int v = R[i];
        double x = dR[i];
        int loc = RLoc[v];
        int locDiv = loc / Gn;
        int locMod = loc % Gn;
        int bn = (int) floor(x / delta_l);
        int pos = Bcount[bn] + locMod;
        pos += (locDiv >= 1) * Braise[bn];
        pos += (locDiv >= 2) * Braise[bn + numBuckets];
        pos += (locDiv >= 3) * Braise[bn + 2 * numBuckets];

        Buckets[bn][pos] = v;
        vBucketLoc[v] = pos;
        vBucketMap[v] = bn;

        //if (d[v] != x)
        //  printf("update D[%5d] <= %2.8f  (%-2.8f)\n",v,x,d[v]);
        d[v] = x;
        RLoc[v] = -1;
      }

      #pragma mta trace "loop 3 end"
    }
    else
    {
      for (i = 0; i < Rcount; i++)
      {
        int v = R[i];
        double x = dR[i];
        int loc = RLoc[v];
        int locDiv = loc / Gn;
        int locMod = loc % Gn;
        int bn = (int) floor(x / delta_l);
        int pos = Bcount[bn] + locMod;
        pos += (locDiv >= 1) * Braise[bn];
        pos += (locDiv >= 2) * Braise[bn + numBuckets];
        pos += (locDiv >= 3) * Braise[bn + 2 * numBuckets];

        Buckets[bn][pos] = v;
        vBucketLoc[v] = pos;
        vBucketMap[v] = bn;

        //if (d[v] != x)
        //  printf("update D[%5d] <= %2.8f  (%-2.8f)\n", v,x,d[v]);
        d[v] = x;
        RLoc[v] = -1;
      }
    }

    if (lastBucketNum - currBucketNum > 30)
    {
      #pragma mta trace "loop 4 start"
      #pragma mta parallel single processor
      #pragma mta assert nodep
      #pragma mta interleave schedule
      for (i = currBucketNum; i < lastBucketNum; i++)
      {
        Bcount[i] += Braise[i] + Braise[i + numBuckets] +
                     Braise[i + 2 * numBuckets] + Braise[i + 3 * numBuckets];
        Braise[i] = 0;
        Braise[i + numBuckets]   = 0;
        Braise[i + 2 * numBuckets] = 0;
        Braise[i + 3 * numBuckets] = 0;
      }

      #pragma mta trace "loop 4 end"
    }
    else
    {
      for (i = currBucketNum; i < lastBucketNum; i++)
      {
        Bcount[i] += Braise[i] + Braise[i + numBuckets] +
                     Braise[i + 2 * numBuckets] + Braise[i + 3 * numBuckets];
        Braise[i] = 0;
        Braise[i + numBuckets]   = 0;
        Braise[i + 2 * numBuckets] = 0;
        Braise[i + 3 * numBuckets] = 0;
      }
    }

    //INFO("<<<\trelax()\n");
    return lastBucketNum;
  }

  //-----[ end relax() ]------------------------------------------------

protected:
  void printGraph()
  {
    for (unsigned int vi = 0; vi < num_vertices(G); vi++)
    {
      printAdjacency(vi);
    }
  }

  void printAdjacency(int vid)
  {
    vertex_id_map<graph_t> vid_map = get(_vertex_id_map, G);
    vertex_t vu = get_vertex(vid, G);
    int deg = out_degree(vu, G);
    printf("%d [%d]\t{ ", vid, deg); fflush(stdout);
    EITER_T inc_edges = out_edges(vu, G);

    for (int ineigh = 0; ineigh < deg; ineigh++)
    {
      edge_t e = inc_edges[ineigh];
      int tgt = get(vid_map, target(e, G));
      printf("%d ", tgt); fflush(stdout);
    }
    printf("}\n"); fflush(stdout);
  }

  void print_double_array(double* A, int N, char* TAG) {
    int c = 0;
    for (int i = 0; i < N; i++)
    {
      c++;
      printf("%6.3f ", A[i]);
      if (c == 10)
      {
        printf("\n"); c = 0;
      }
    }
    printf("\n");
  }

  //-----[ heavy_collect_visadj_t ]-------------------------------------
  class heavy_collect_visadj_t {
public:
    heavy_collect_visadj_t(graph_t& __ga,
                           double __delta_l,
                           double* __Wt,
                           double* __d,
                           double __du,
                           double* __dR,
                           int& __Rcount,
                           int* __R,
                           int* __RLoc,
                           timestamp_t* __vertTime,
                           vertex_id_map<graph_t>& __vid_map,
                           edge_id_map<graph_t>& __eid_map)
      : ga(__ga), delta_l(__delta_l), Wt(__Wt),
      d(__d), du(__du), dR(__dR),
      Rcount(__Rcount), R(__R), RLoc(__RLoc),
      vertTime(__vertTime),
      vid_map(__vid_map), eid_map(__eid_map)
    {}

    bool visit_test(vertex_t v) {
      return(true);
    }

    void operator()(edge_t& e, vertex_t& src, vertex_t& dest)
    {
      vertex_t first = source(e, ga);
      if (src == first)
      {
        int u = get(vid_map, src);      // source-ID
        int v = get(vid_map, dest);     // dest-ID
        int j = get(eid_map, e);        // edge-ID
        //INFO("\t- heavy collect: e[%2d] %2d->%-2d   j u v\n", j,u,v);
        if (vertTime == NULL || vertTime[u] <= vertTime[v])
        {
          if (Wt[j] + du < d[v])
          {
            if (delta_l <= Wt[j])                                                        // SORTW
            {
              int rlv = mt_readfe( RLoc[v] );
              if (rlv == -1)
              {
                int pos = mt_incr( Rcount, 1 );
                R[pos]  = v;
                dR[pos] = d[u] + Wt[j];
                mt_write( RLoc[v], pos );
              }
              else
              {
                if ( du + Wt[j] < dR[rlv] )
                {
                  dR[rlv] = du + Wt[j];
                }
                mt_write( RLoc[v], rlv);
              }
            }                                                                           // SORTW
          }
        }
      }
    }

private:
    graph_t& ga;
    timestamp_t* vertTime;
    double delta_l;
    double* Wt;
    double* d;
    double du;
    double* dR;
    int& Rcount;
    int* R;
    int* RLoc;
    vertex_id_map<graph_t>& vid_map;
    edge_id_map<graph_t>& eid_map;
  };
  //-----[ heavy_collect_visadj_t ]-------------------------------------

  //-----[ light_collect_visadj_t ]-------------------------------------
  class light_collect_visadj_t {
public:
    light_collect_visadj_t(graph_t& __ga,
                           double __delta_l,
                           double* __Wt,
                           double* __d,
                           double __du,
                           double* __dR,
                           int& __Rcount,
                           int* __R,
                           int* __RLoc,
                           timestamp_t* __vertTime,
                           vertex_id_map<graph_t>& __vid_map,
                           edge_id_map<graph_t>& __eid_map)
      : ga(__ga), delta_l(__delta_l), Wt(__Wt),
      d(__d), du(__du), dR(__dR),
      Rcount(__Rcount), R(__R), RLoc(__RLoc),
      vertTime(__vertTime),
      vid_map(__vid_map), eid_map(__eid_map)
    {}

    bool visit_test(vertex_t v) {
      return(true);
    }

    void operator()(edge_t& e, vertex_t& src, vertex_t& dest)
    {
      vertex_t first = source(e, ga);
      if (src == first)
      {
        int u = get(vid_map, src);    // source-ID
        int v = get(vid_map, dest);   // dest-ID
        int j = get(eid_map, e);      // edge-ID
        //INFO("\tlight collect: e[%d]  %d->%d\n",j,u,v);
        if (vertTime == NULL || vertTime[u] <= vertTime[v])
        {
          if ( du + Wt[j] < d[v] )
          {
            if ( delta_l > Wt[j] )                                               // SORTW
            {
              int rlv = mt_readfe( RLoc[v] );
              if (rlv == -1)
              {
                int pos = mt_incr(Rcount, 1);
                R[pos]  = v;
                dR[pos] = du + Wt[j];
                mt_write( RLoc[v], pos );
              }
              else
              {
                if ( du + Wt[j] < dR[rlv] )
                {
                  dR[rlv] = du + Wt[j];
                }
                mt_write( RLoc[v], rlv );
              }
            }                                                                   // SORTW
          }
        }
      }
    }

private:
    graph_t& ga;
    timestamp_t* vertTime;
    double delta_l;
    double* Wt;
    double* d;
    double du;
    double* dR;
    int& Rcount;
    int* R;
    int* RLoc;
    vertex_id_map<graph_t>& vid_map;
    edge_id_map<graph_t>& eid_map;
  };
  //-----[ light_collect_visadj_t ]-------------------------------------

private:
  graph_t& G;
  int s;
  double* realWt;
  double* cs;
  double* result;
  timestamp_t* vertTime;
};

// clean up #defined values that should be local to this header file.

#undef SSSP_INIT_SIZE
#undef FREEMEM
//#undef INFO
#undef DEBUG
//#undef PRINTDEBUG
#undef MTGL_USE_VISITADJ

}

#endif

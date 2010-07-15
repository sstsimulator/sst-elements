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
/*! \file depth_first_search.hpp

    \brief This experimental code contains utilities to compute values
           associated with serial depth-first searches.

    \author Jon Berry (jberry@sandia.gov)

    \date 11/2006

    It is assumed that a parallel search (psearch) has already been run.
    Given, this, there are algorithms here to compute numbers of descendants
    of each vertex, (with respect to the search forest found by the psearch)
    and to relabel the vertices with "discovery" and "finish" times as if a
    serial depth-first search had been run.  This gives access to traditional
    versions of strongly connected components, topological sorts, and
    biconnected components.

    However, this code won't scale if the graph has a giant strongly-connected
    component.

    This code contains a complex usage of visitor classes, including cases in
    which a search may trigger other searches.
*/
/****************************************************************************/

#ifndef MTGL_DEPTH_FIRST_SEARCH_HPP
#define MTGL_DEPTH_FIRST_SEARCH_HPP

#include <cstdio>
#include <climits>

#include <mtgl/util.hpp>
#include <mtgl/copier.hpp>
#include <mtgl/psearch.hpp>
#include <mtgl/depth_first_search_visitor.hpp>
#include <mtgl/topsort.hpp>
#include <mtgl/dynamic_array.hpp>

#ifdef __MTA__
#include <sys/mta_task.h>
#include <machine/runtime.h>
#endif

#define SAMPLE_TYPE ((short) 4)

namespace mtgl {

template <typename graph>
class count_descendants {
public:
  typedef typename graph_traits<graph>::edge_descriptor edge;
  typedef typename graph_traits<graph>::vertex_iterator vertex_iterator;
  typedef typename graph_traits<graph>::size_type size_type;

  count_descendants(graph* gg, int* ldrs, int num_ldrs, int* prnt,
                    int* subp = 0) :
    g(gg), leaders(ldrs), num_leaders(num_ldrs),
    psearch_parent(prnt), subproblem(subp) {}

  int* run()
  {
    printf("countDescendants\n");
    size_type order = num_vertices(*g);
    int* searchColor = (int*) malloc(sizeof(int) * order);
    int* dcount = (int*) malloc(sizeof(int) * order);

    vertex_iterator verts = vertices(*g);

    #pragma mta assert nodep
    for (size_type i = 0; i < order; i++)
    {
      searchColor[i] = 0;
      dcount[i] = 0;
    }
    #pragma mta assert parallel
    #pragma mta loop future
    for (int i = 0; i < num_leaders; i++)
    {
      count_descendants_visitor<graph>
      cdv(dcount, psearch_parent, subproblem, *g);

      psearch<graph, int*, count_descendants_visitor<graph>,
              AND_FILTER, DIRECTED, 1>
      psearch(*g, searchColor, cdv);

      psearch.run(verts[leaders[i]]);
    }

    int* dc2 = (int*) malloc(sizeof(int) * order);

    #pragma mta assert nodep
    for (size_type i = 0; i < order; i++)
    {
      searchColor[i] = 0;
      dc2[i] = 0;
    }

    printf("debugging descendant count\n");

    #pragma mta assert parallel
    #pragma mta loop future
    for (int i = 0; i < num_leaders; i++)
    {
      count_descendants_debugger<graph> cdb(dcount, dc2,
                                            psearch_parent, subproblem, *g);

      psearch<graph, int*, count_descendants_debugger<graph>,
              AND_FILTER, DIRECTED, 2>
      psearch2(*g, searchColor, cdb);

      psearch2.run(verts[leaders[i]]);
    }

    printf("done debugging descendant count\n");

    return dcount;
  }

private:
  graph* g;
  int* leaders;
  int num_leaders;
  int* psearch_parent;
  int* subproblem;
};

template <typename graph>
class relabel_psearch {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;
  typedef typename graph_traits<graph>::vertex_iterator vertex_iterator;
  typedef typename graph_traits<graph>::adjacency_iterator adjacency_iterator;
  typedef typename graph_traits<graph>::size_type size_type;

  relabel_psearch(graph* gg, int* ldrs, int num_ldrs, int* b_ldr,
                  int* prnt, int* dcnt, int* b_st, int* b_ft,
                  int* lw, int* hgh, int* st, int* ft, int& next_st,
                  bool* vlid, int serial_psrch_max = 1, int* subprob = 0) :
    g(gg), leaders(ldrs), num_leaders(num_ldrs), b_leader(b_ldr),
    psearch_parent(prnt), dcount(dcnt), b_start_time(b_st),
    b_finish_time(b_ft), low(lw), high(hgh), start_time(st),
    finish_time(ft), next_start_time(next_st), valid(vlid),
    serial_psearch_max(serial_psrch_max), subproblem(subprob) {}

  int run()
  {
    printf("relabelpsearch\n");

    size_type order = num_vertices(*g);
    int* searchColor = (int*) malloc(sizeof(int) * order);
    int* searchColor2 = (int*) malloc(sizeof(int) * order);
    int* rdv_fintime = (int*) malloc(sizeof(int) * order);

    vertex_iterator verts = vertices(*g);
    vertex_id_map<graph> vid_map = get(_vertex_id_map, *g);

    #pragma mta assert nodep
    for (size_type i = 0; i < order; i++)
    {
      searchColor[i] = 0;
      searchColor2[i] = 0;
      low[i] = b_start_time[i];
      high[i] = b_finish_time[i];
      valid[i] = true;
      rdv_fintime[i] = 0;
    }

    //#pragma mta assert parallel  // 11/28/2006
    //#pragma mta loop future
    for (int i = 0; i < num_leaders; i++)
    {
      int id = leaders[i];
      if (subproblem[id])
      {
        relabel_psearch_visitor<graph> rdv(g, dcount,
                                           b_leader, psearch_parent,
                                           b_start_time, b_finish_time,
                                           start_time, finish_time, low, high,
                                           serial_psearch_max, valid,
                                           searchColor2, next_start_time,
                                           rdv_fintime, subproblem);

        psearch<graph, int*,
                relabel_psearch_visitor<graph>,
                AND_FILTER, DIRECTED, 1>
        psearch(g, searchColor, rdv);

        psearch.run(verts[id]);
      }
    }

    printf("FINISHING TOP LEVEL\n");

    int num_invalid = 0;

    #pragma mta assert nodep
    for (size_type i = 0; i < order; i++)
    {
      if (!valid[i]) num_invalid++;
    }

    printf("AT TOP LEVEL\n");
    printf("num_invalid: %d\n", num_invalid);

    if (num_invalid < serial_psearch_max)
    {
      for (int i = 0; i < num_leaders; i++)
      {
        int id = leaders[i];

        if (subproblem[id] && !valid[id])
        {
          start_time[id]  = next_start_time;
          finish_time[id] = -1;        // can't rely on dcount if invalid
          bool print = false;
          relabel_event_visitor<graph> srv(psearch_parent, dcount,
                                           start_time, finish_time, subproblem,
                                           low, high, b_start_time,
                                           b_finish_time, next_start_time,
                                           valid, print, b_leader,
                                           rdv_fintime, *g);

          psearch<graph, int*, relabel_event_visitor<graph>,
                  AND_FILTER, DIRECTED>
          psearch(g, searchColor2, srv);

          psearch.mixedRun(verts[id], valid);

          vertex v = verts[id];
          size_type deg = degree(v, *g);
          adjacency_iterator adj_verts = adjacent_vertices(v, *g);
          int vfin = 0;

          if (deg > 100)
          {
            #pragma mta assert parallel
            for (int j = 0; j < deg; j++)
            {
              vertex dest = adj_verts[j];
              int fin = finish_time[get(vid_map, dest)];
              if (fin > vfin) vfin = fin;
            }
          }
          else
          {
            for (int j = 0; j < deg; j++)
            {
              vertex dest = adj_verts[j];
              int fin = finish_time[get(vid_map, dest)];
              if (fin > vfin) vfin = fin;
            }
          }

          if (start_time[id] > vfin) vfin = start_time[id];

          finish_time[id] = vfin + 1;
          next_start_time = vfin + 2;
        }
      }

      num_invalid = 0;
    }

    free(searchColor);
    free(searchColor2);
    free(rdv_fintime);

    return num_invalid;
  } // run()

private:
  graph* g;
  int* leaders;
  int num_leaders;
  int* b_leader;
  int* psearch_parent;
  int* dcount;
  int* b_start_time;
  int* b_finish_time;
  int* start_time;
  int* finish_time;
  int* low, *high;
  int& next_start_time;
  bool* valid;
  int serial_psearch_max;
  int* subproblem;
}; // class relabel_psearch

template <typename graph>
class dfs_event_times {
public:
  typedef typename graph_traits<graph>::edge_descriptor edge;
  typedef typename graph_traits<graph>::vertex_iterator vertex_iterator;
  typedef typename graph_traits<graph>::size_type size_type;

  dfs_event_times(graph* gg, int* st, int* ft,
                  int serial_psrch_max = 1000, int* subprob = 0,
                  int my_subprob = 0, int* brat_att = 0, bool* vlid = 0) :
    g(gg), start_time(st), finish_time(ft),
    serial_psearch_max(serial_psrch_max), subproblem(subprob),
    my_subproblem(my_subprob), brat_attr(brat_att), valid(vlid) {}

  void run()
  {
    printf("dfs_event_times\n");

    size_type order = num_vertices(*g);
    vertex_iterator verts = vertices(*g);

    int next_b_start_time = 0;
    int next_real_start_time = 0;
    int* b_leader = (int*) malloc(sizeof(int) * order);
    int* b_parent = (int*) malloc(sizeof(int) * order);
    int* b_start_time = (int*) malloc(sizeof(int) * order);
    int* b_finish_time = (int*) malloc(sizeof(int) * order);
    int* in_degree;
    int* in_degree_copy = (int*) malloc(sizeof(int) * order);
    int* out_degree;
    int* low = (int*) malloc(sizeof(int) * order);
    int* high = (int*) malloc(sizeof(int) * order);
    bool allocd_brat_attr = false;
    bool allocd_valid = false;

    compute_in_degree<graph> cid(*g, subproblem, my_subproblem);
    in_degree = cid.run();

    compute_out_degree<graph> cod(*g, subproblem, my_subproblem);
    out_degree = cod.run();

    #pragma mta assert nodep
    for (size_type i = 0; i < order; i++)
    {
      b_leader[i] = i;
      b_parent[i] = i;
      in_degree_copy[i] = in_degree[i];
    }

    if (!brat_attr)
    {
      brat_attr  = (int*) malloc(sizeof(int) * order);
      allocd_brat_attr = true;

      #pragma mta assert nodep
      for (size_type i = 0; i < order; i++) brat_attr[i] = i;
    }

    if (!valid)
    {
      valid = (bool*) malloc(sizeof(bool) * order);
      allocd_valid = true;

      #pragma mta assert nodep
      for (size_type i = 0; i < order; i++) valid[i] = true;
    }

    // The suffix nodes, are good candidates for early finish times.

    topsort_suffix_with_event_labeling<graph>
    ts(*g, out_degree, start_time, finish_time,
       next_real_start_time, subproblem, my_subproblem);

    ts.run();

    // Mark the prefix nodes, but don't give them event times yet.

    topsort_prefix<graph> tp(*g, in_degree);
    tp.run();

    #pragma mta assert nodep
    for (size_type i = 0; i < order; i++)
    {
      if (in_degree[i] == 0 || out_degree[i] == 0) subproblem[i] = 0;
    }

    brat_search<graph, int, DIRECTED> bs(g, brat_attr, b_leader, b_parent,
                                         subproblem, my_subproblem);
    bs.run();

    dynamic_array<int> classes;

    findClasses(b_leader, order, classes);

    int num_leaders = classes.size();

    printf("num_leaders after bratSearch %d\n", num_leaders);

    int* leaders = (int*) malloc(sizeof(int) * num_leaders);
    copy(leaders, classes);

    printf("bratSearch done\n");

// ---------------------------------------------------------------

    int* leaders2 = (int*) malloc(sizeof(int) * num_leaders);
    for (unsigned int i = 0; i < num_leaders; ++i) leaders2[i] = leaders[i];

    countingSort(leaders2, num_leaders, order);
    printf("num_leaders after sort %d\n", num_leaders);

    int* psearch_leader = (int*) malloc(sizeof(int) * order);
    int* searchColor2 = (int*) malloc(sizeof(int) * order);

    #pragma mta assert nodep
    for (size_type i = 0; i < order; i++)
    {
      if (subproblem[i])
      {
        searchColor2[i] = 0;
      }
      else
      {
        searchColor2[i] = INT_MAX;
      }

      psearch_leader[i] = i;
    }

    printf("num_leaders after psearch trees %d\n", num_leaders);

// ---------------------------------------------------------------

    count_descendants<graph> cd(g, leaders, num_leaders, b_parent, subproblem);

    int* numdesc = cd.run();

    printf("num_leaders after desc count %d\n", num_leaders);

    int* searchColor = (int*) malloc(sizeof(int) * order);

    #pragma mta assert nodep
    for (size_type i = 0; i < order; i++) searchColor[i] = 0;

    printf("num_descendants 0: %d\n", numdesc[0]);
    printf("num_leaders 0: %d\n", num_leaders);

    printf("assigning branching event times\n");

    #pragma mta assert parallel
    #pragma mta loop future
    for (int i = 0; i < num_leaders; i++)
    {
      if (subproblem[leaders2[i]])
      {
        b_event_visitor<graph> dev(b_parent, numdesc,
                                   b_start_time, b_finish_time, subproblem, *g);

        psearch<graph, int*, b_event_visitor<graph>, AND_FILTER, DIRECTED, 1>
        psrch(g, searchColor, dev);

        int num_events = 2 * numdesc[leaders2[i]];
        int my_start = mt_incr(next_b_start_time, num_events);

        b_start_time [leaders2[i]] = my_start;
        b_finish_time[leaders2[i]] = my_start + num_events - 1;

        assert(b_finish_time[leaders2[i]] < 2 * order);

        psrch.run(verts[leaders2[i]]);
      }
    }
/*
    printf("bratSearch: new branching parents :\n");

    for (size_type i=0; i<order; i++)
    {
      if (subproblem[i] == 1) printf("%d: bp:%d\n",i,b_parent[i]);
    }

    printf("bratSearch: new descendant counts:\n");

    for (size_type i=0; i<order; i++)
    {
      if (subproblem[i] == 1) printf("%d: nd:%d\n", i, numdesc[i]);
    }

    printf("bratSearch: new branching event times:\n");

    for (size_type i=0; i<order; i++)
    {
      if (subproblem[i] == 1)
      {
        printf("%d: bst:%d\tbft: %d\n", i, b_start_time[i], b_finish_time[i]);
      }
*/

/*
      for (size_type i=0; i<order; i++)
      {
        if (subproblem[i])
        {
          printf("%d: bst: %d, bft: %d\n", i, b_start_time[i],
                 b_finish_time[i]);
        }
      }
*/
// ---------debug event times--------------------------------------------
/*
      #pragma mta assert nodep
      for (size_type i=0; i<order; i++) searchColor[i] = 0;

      #pragma mta assert parallel
      #pragma mta loop future
      for (int i=0; i<num_leaders; i++)
      {
        psearch_event_debugger dev(b_parent,b_start_time,
                                   b_finish_time, order, subproblem, *g);

        psearch<int*, psearch_event_debugger, AND_FILTER, DIRECTED,1>
        psearch(g, searchColor, dev);

        psearch.run(verts[leaders[i]]);
      }
*/
// ----------------------------------------------------------------------

    relabel_psearch<graph> rlp(g, leaders2, num_leaders, b_leader,
                               b_parent, numdesc, b_start_time, b_finish_time,
                               low, high, start_time, finish_time,
                               next_real_start_time, valid,
                               serial_psearch_max, subproblem);

    int num_invalid = rlp.run();

    topsort_prefix_with_event_labeling<graph>

    tpw(*g, in_degree_copy, start_time, finish_time, next_real_start_time,
        subproblem, my_subproblem);
    tpw.run();

/*
    for (size_type i=0; i<order; i++)
    {
      printf("%d: st: %d\tft: %d\n", i, start_time[i], finish_time[i]);
    }
*/
    if (num_invalid > 0)
    {
      #pragma mta assert nodep
      for (size_type i = 0; i < order; i++)
      {
        brat_attr[i] = abs(high[i] - low[i]);
      }

      printf("ABOUT TO RECURSE: old bst,bft:\n");

      free(numdesc);
      free(searchColor);
      free(b_leader);
      free(low);
      free(high);
      free(b_parent);
      free(b_start_time);
      free(b_finish_time);
      free(in_degree);
      free(in_degree_copy);
      free(out_degree);

      if (serial_psearch_max < order) serial_psearch_max *= 2;

      dfs_event_times<graph> psfet(g, start_time, finish_time,
                                   serial_psearch_max, subproblem,
                                   my_subproblem, brat_attr, valid);
      psfet.run();
    }
    else
    {
      free(numdesc);
      free(searchColor);
      free(b_leader);
      free(b_parent);
      free(low);
      free(high);
      free(b_start_time);
      free(b_finish_time);
      free(in_degree);
      free(in_degree_copy);
      free(out_degree);
      free(valid);
    }

    free(leaders2);
  }

private:
  graph* g;
  int* start_time;
  int* finish_time;
  int serial_psearch_max;
  int* subproblem;
  int my_subproblem;
  int* brat_attr;
  bool* valid;
};

}

#endif

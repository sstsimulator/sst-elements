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
/*! \file depth_first_search_visitor.hpp

    \brief Visitor classes used by algorithm that computes serial dfs values
           such as discovery and finish times.

    \author Jon Berry (jberry@sandia.gov)

    \date 11/2006
*/
/****************************************************************************/

#ifndef MTGL_DEPTH_FIRST_SEARCH_VISITOR_HPP
#define MTGL_DEPTH_FIRST_SEARCH_VISITOR_HPP

#include <cstdio>
#include <climits>

#include <mtgl/util.hpp>
#include <mtgl/copier.hpp>
#include <mtgl/psearch.hpp>

namespace mtgl {

template <typename graph>
class count_descendants_visitor : public default_psearch_visitor<graph> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  count_descendants_visitor(int* dc, int* psearchp, int* vm, graph& _g) :
    descendant_count(dc), parent(0), psearch_parent(psearchp),
    num_descendants(0), my_vert(-1), vert_mask(vm), g(_g) {}

  count_descendants_visitor(const count_descendants_visitor& ctv) :
    parent(&ctv), descendant_count(ctv.descendant_count), num_descendants(0),
    psearch_parent(ctv.psearch_parent), my_vert(ctv.my_vert),
    vert_mask(ctv.vert_mask), g(ctv.g) {}

  void pre_visit(vertex v) { my_vert = v->id; }

  bool visit_test(edge e, vertex src)
  {
    vertex dest = other(e, src, g);
    return ((!vert_mask || vert_mask[dest->id]) &&
            (psearch_parent[dest->id] == src->id));
  }

  void finish_vertex(vertex v)
  {
    assert(v->id >= 0);
    assert(v->id == my_vert);

    (void) mt_incr(descendant_count[v->id], 1);             // count myself

    if (parent && parent->my_vert >= 0)
    {
      assert(parent->my_vert != my_vert);
      (void) mt_incr(descendant_count[parent->my_vert],
                     descendant_count[v->id]);
    }
  }

private:
  const count_descendants_visitor* parent;
  int num_descendants;
  int* descendant_count;
  int my_vert;
  int* psearch_parent;
  int* vert_mask;
  graph& g;
};

template <typename graph>
class count_descendants_debugger : public default_psearch_visitor<graph> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  count_descendants_debugger(int* dc, int* dc2, int* psearchp, int* vm,
                             graph& _g) :
    descendant_count(dc), descendant_count2(dc2),
    psearch_parent(psearchp), vert_mask(vm), g(_g) {}

  count_descendants_debugger(const count_descendants_debugger& ctv) :
    descendant_count(ctv.descendant_count),
    descendant_count2(ctv.descendant_count2),
    psearch_parent(ctv.psearch_parent), vert_mask(ctv.vert_mask), g(ctv.g) {}

  void pre_visit(vertex v) { descendant_count2[v->id] = 1; }

  bool visit_test(edge e, vertex src)
  {
    vertex dest = other(e, src, g);
    return ((!vert_mask || vert_mask[dest->id]) &&
            (psearch_parent[dest->id] == src->id));
  }

  void tree_edge(edge e, vertex src)
  {
    vertex dest = other(e, src, g);
    //if(my_psearchtree != psearchtree[dest->id])
    //foo();
    (void) mt_incr(descendant_count2[src->id],
                   descendant_count[dest->id]);
    //if (descendant_count2[src->id] >= 524288)
    //foo();
  }

  void finish_vertex(vertex src)
  {
    if (descendant_count2[src->id] != descendant_count[src->id])
    {
      printf("bad kid count: %d (%d vs orig %d)\n", src->id,
             descendant_count2[src->id],
             descendant_count[src->id]);
      //foo();
    }
  }

private:
  int* descendant_count;
  int* descendant_count2;
  int* psearch_parent;
  int* vert_mask;
  graph& g;
};

template <typename graph>
class b_event_visitor : public default_psearch_visitor<graph> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  b_event_visitor(int* bp, int* dc, int* st, int* ft, int* vm, graph& _g) :
    descendant_count(dc), start_time(st), finish_time(ft),
    next_start_time(0), b_parent(bp), vert_mask(vm), g(_g) {}

  void pre_visit(vertex v)
  {
    // start time[v->id] has already been set
    next_start_time = start_time[v->id] + 1;
  }

  bool visit_test(edge e, vertex src)
  {
    vertex dest = other(e, src, g);
    return ((!vert_mask || vert_mask[dest->id]) &&
            (b_parent[dest->id] == src->id));
  }

  void tree_edge(edge* e, vertex src)
  {
    vertex dest = other(e, src, g);

    if (descendant_count[src->id] < descendant_count[dest->id])
    {
      printf("dc[%d]: %d < dc[%d]: %d\n", src->id,
             descendant_count[src->id], dest->id,
             descendant_count[dest->id]);
    }

    int num_events = 2 * descendant_count[dest->id];
    int my_start = mt_incr(next_start_time, num_events);
    start_time[dest->id]  = my_start;
    finish_time[dest->id] = my_start + num_events - 1;
/*
    if (finish_time[dest->id] >= 2*order)
    {
      printf("vertex %d fault: st: %d, ft: %d (detected(%d,%d)\n",
             iest->id, start_time[dest->id], finish_time[dest->id],
             e->vert1->id, e->vert2->id);
    }
*/
  }

private:
  int next_start_time;
  int* b_parent;
  int* descendant_count;
  int* start_time;
  int* finish_time;
  int* vert_mask;
  graph& g;
};

template <typename graph>
class serial_event_visitor : public default_psearch_visitor<graph> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  serial_event_visitor(int* psearchp, int* dc, int* st, int* ft, int* vm,
                       int* l, int* h, int* bst, int* bft, graph& _g) :
    descendant_count(dc), start_time(st), finish_time(ft), next_start_time(0),
    psearch_parent(psearchp), vert_mask(vm), low(l), high(h),
    b_start_time(bst), b_finish_time(bft), g(_g) {}

  void psearch_root(vertex v) { my_leader = v->id; }

  void pre_visit(vertex v)
  {
    // start time[v->id] has already been set
    next_start_time = start_time[v->id] + 1;
  }

  bool visit_test(edge e, vertex src)
  {
    vertex dest = other(e, src, g);
    return (!vert_mask || vert_mask[dest->id]);
  }

  void tree_edge(edge e, vertex src)
  {
    vertex dest = other(e, src, g);
    int num_events = 2 * descendant_count[dest->id];
    int my_start = mt_incr(next_start_time, num_events);
    start_time[dest->id]  = my_start;
    finish_time[dest->id] = my_start + num_events - 1;
    psearch_parent[dest->id] = src->id;
  }

private:
  int next_start_time;
  int* psearch_parent;
  int* descendant_count;
  int* start_time;
  int* finish_time;
  int* vert_mask;
  int* low;                     // for debugging
  int* high;                    // for debugging
  int* b_start_time;            // for debugging
  int* b_finish_time;           // for debugging
  int my_leader;
  graph& g;
};

template <typename graph>
class relabel_event_visitor : public default_psearch_visitor<graph> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  relabel_event_visitor(int* psearchp, int* dc, int* st, int* ft, int* vm,
                        int* l, int* h, int* bst, int* bft, int& nst,
                        bool* vld, bool prnt, int* b_ldr, int* rdvfin,
                        graph& _g) :
    descendant_count(dc), start_time(st), finish_time(ft),
    next_start_time(nst), psearch_parent(psearchp), vert_mask(vm), low(l),
    high(h), b_start_time(bst), b_finish_time(bft), valid(vld),
    print(prnt), b_leader(b_ldr), rdv_fintime(rdvfin), g(_g) {}

  void psearch_root(vertex v) { my_leader = v->id; }

  void pre_visit(vertex v)
  {
    // start time[v->id] has already been set
    next_start_time = start_time[v->id] + 1;

    if (rdv_fintime[v->id] == 0)
    {
      printf("error: rev::pv(%d) (ldr: %d) visiting unfinished vert.\n",
             v->id, my_leader);
    }

    if (rdv_fintime[v->id] > next_start_time)
    {
      printf("error: rev::pv(%d) visiting future vert.\n", v->id);
    }

    if (v->id == 42212 || v->id == 296693 || v->id == 689561 ||
        v->id == 522156 || v->id == 4592086 || v->id == 4545236)
    {
      printf("rev::pv(%d) (ldr: %d) l[%d]: %d, h[%d]: %d "
             " l[%d]: %d, h[%d]: %d\n",
             v->id, my_leader, v->id, low[v->id], v->id, high[v->id],
             my_leader, low[my_leader], my_leader, high[my_leader]);
    }
/*
    if (!((low[v->id] >= low[my_leader]) &&
        (high[v->id] <= high[my_leader])))
    {
      printf("error: rev: %d: l: %d, h: %d, relab_root: %d "
             " l[%d]: %d, h[%d]: %d\n", v->id, low[v->id],
             high[v->id], my_leader, my_leader,
             low[my_leader], my_leader, high[my_leader]);

      printf("error: rev: %d: l: %d, h: %d, b_ldr: %d "
             " l[%d]: %d, h[%d]: %d\n", v->id, low[v->id],
             high[v->id], b_leader[v->id], b_leader[v->id],
             low[b_leader[v->id]], b_leader[v->id], high[b_leader[v->id]]);

      printf("error: rev: %d: l: %d, h: %d, b_ldr[mldr]: %d "
             " l[%d]: %d, h[%d]: %d\n", v->id, low[v->id],
             high[v->id], b_leader[my_leader],
             b_leader[my_leader], low[b_leader[my_leader]],
             b_leader[my_leader],high[b_leader[my_leader]]);
    }
*/
  }

  bool visit_test(edge e, vertex src)
  {
    vertex dest = other(e, src, g);
    return ((!vert_mask || vert_mask[dest->id]) &&
            (!valid[src->id] || psearch_parent[dest->id] == src->id));
  }

  void tree_edge(edge e, vertex src)
  {
    vertex dest = other(e, src, g);

    if (valid[dest->id])
    {
      int num_events = 2 * descendant_count[dest->id];
      int my_start = mt_incr(next_start_time, num_events);
      start_time[dest->id] = my_start;
      finish_time[dest->id] = my_start + num_events - 1;

/*
      printf("rev::te(%d,%d) v(ldr: %d) l[%d]: %d, h[%d]: %d "
             " l[%d]: %d, h[%d]: %d, st: %d, ft: %d\n",
             src->id, dest->id, my_leader, src->id, low[src->id], src->id,
             high[src->id], my_leader, low[my_leader], my_leader,
             high[my_leader], start_time[dest->id], finish_time[dest->id]);
*/
    }
    else
    {
      int my_start = mt_incr(next_start_time, 1);
      start_time[dest->id] = my_start;

/*
      printf("rev::te(%d,%d)iv(ldr: %d) l[%d]: %d, h[%d]: %d "
             " l[%d]: %d, h[%d]: %d, st:%d, ft:%d\n",
             src->id, dest->id, my_leader, src->id, low[src->id], src->id,
             high[src->id], my_leader, low[my_leader], my_leader,
             high[my_leader], start_time[dest->id], finish_time[dest->id]);
*/
    }

    psearch_parent[dest->id] = src->id;

/*
    printf("mixed relabel::te:[%d] (%d, %d) %d:low:%d,high:%d "
           " bst: %d, bft: %d, st: %d, ft: %d\n",
           my_leader, src->id, dest->id, dest->id, low[dest->id],
           high[dest->id], b_start_time[dest->id], b_finish_time[dest->id],
           start_time[dest->id], finish_time[dest->id]);
*/
  }

  void finish_vertex(vertex v)
  {
    if (!valid[v->id])
    {
      int my_finish = mt_incr(next_start_time, 1);
      finish_time[v->id] = my_finish;

/*
      printf("rev::fv(%d) (ldr: %d) l[%d]: %d, h[%d]: %d "
             " l[%d]: %d, h[%d]: %d, st:%d, ft:%d\n",
             v->id,  my_leader, v->id, low[v->id], v->id, high[v->id],
             my_leader, low[my_leader], my_leader,
             high[my_leader], start_time[v->id], finish_time[v->id]);
*/
    }

    valid[v->id] = 1;
  }

private:
  int& next_start_time;
  int* psearch_parent;
  int* descendant_count;
  int* start_time;
  int* finish_time;
  int* vert_mask;
  bool* valid;
  int* low;                     // for debugging
  int* high;                    // for debugging
  int* b_start_time;            // for debugging
  int* b_finish_time;           // for debugging
  int my_leader;                // for debugging
  int* b_leader;                // for debugging
  int* rdv_fintime;             // for debugging
  bool print;
  graph& g;
};

template <typename graph>
class relabel_psearch_visitor : public default_psearch_visitor<graph> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  relabel_psearch_visitor(graph* gr, int* dcount, int* bldr, int* psearchp,
                          int* bst, int* bft, int* dst, int* dft, int* l,
                          int* h, int sdm, bool* vld, int* sc2,
                          int& nst, int* rdvfin, int* vm = 0) :
    g(gr), descendant_count(dcount), b_leader(bldr), psearch_parent(psearchp),
    b_start_time(bst), b_finish_time(bft), start_time(dst), finish_time(dft),
    low(l), high(h), serial_psearch_max(sdm), next_start_time(nst),
    vert_mask(vm), searchColor2(sc2), valid(vld), rdv_fintime(rdvfin) {}

  relabel_psearch_visitor(const relabel_psearch_visitor& rdv) :
    g(rdv.g), descendant_count(rdv.descendant_count), b_leader(rdv.b_leader),
    psearch_parent(rdv.psearch_parent), b_start_time(rdv.b_start_time),
    b_finish_time(rdv.b_finish_time), start_time(rdv.start_time),
    finish_time(rdv.finish_time), low(rdv.low), high(rdv.high),
    next_start_time(rdv.next_start_time),
    serial_psearch_max(rdv.serial_psearch_max),
    vert_mask(rdv.vert_mask), searchColor2(rdv.searchColor2),
    valid(rdv.valid), rdv_fintime(rdv.rdv_fintime) {}

  void pre_visit(vertex v) { orig_descendant_count = descendant_count[v->id]; }

  bool visit_test(edge e, vertex src)
  {
    vertex dest = other(e, src, *g);

    // here's our only chance to update our low and high
    // values based upon contact with other branchings
    // or branches of the same branching
    if (psearch_parent[dest->id] != src->id)
    {
      int lowsrc = mt_readfe(low[src->id]);

      if (b_start_time[dest->id] < lowsrc)
      {
        lowsrc = b_start_time[dest->id];
      }

      mt_write(low[src->id], lowsrc);

      if (src->id == 42212 || src->id == 296693 ||
          src->id == 689561 ||
          src->id == 522156 || src->id == 4592086 ||
          src->id == 4545236)
      {
        printf("rdv::vt(%d,%d) update low[%d]: %d\n",
               src->id, dest->id, src->id, lowsrc);
      }

      if (lowsrc < b_start_time[src->id]) valid[src->id] = false;

      int highsrc = mt_readfe(high[src->id]);

      if (b_finish_time[dest->id] > highsrc)
      {
        highsrc = b_finish_time[dest->id];
      }

      mt_write(high[src->id], highsrc);

      if (src->id == 42212 || src->id == 296693 ||
          src->id == 689561 ||
          src->id == 522156 || src->id == 4592086 ||
          src->id == 4545236)
      {
        printf("rdv::vt(%d,%d) update high[%d]: %d\n",
               src->id, dest->id, src->id, highsrc);
      }

      if (highsrc > b_finish_time[src->id]) valid[src->id] = false;
    }

    return ((!vert_mask || vert_mask[dest->id]) &&
            (psearch_parent[dest->id] == src->id));
    //return ((!vert_mask || vert_mask[dest->id]) &&
    //        (b_leader[src->id] == b_leader[dest->id]));
  }

  void tree_edge(edge e, vertex src)
  {
    // This method is needed because of a strange
    // property of MTGL.  The mechanism for specifying the
    // way in which the user's 'visit_test' is combined with
    // the standard white/grey/black visit tests forces a
    // decision between logical 'and', logical 'or', and
    // replacement.  This particular search uses logical 'and'.
    // In this variety of search, the psearch class will first
    // evaluate the visit_test method, and do nothing if it
    // fails.  This is typically the course of action that
    // makes sense.  However, in our case, we really would
    // like to have the visit_test method succeed only if the
    // src is the psearch_parent of the dest. We'd like the back_edge
    // method called if the user's
    // visit_test fails.  Since that won't happen as things
    // stand, we have the strange situation in which we
    // have weakened the 'visit_test' to make sure it succeeds
    // for all possible cases of interest (branching leaders of
    // src and dest are the same), and now we're relying on
    // the resulting 'tree_edge' call to update the low and
    // high values, even though this edge may go between two
    // different branches of the branching (i.e., it's not
    // conceptually a 'tree_edge.')

    vertex dest = other(e, src, *g);
    int lowsrc = mt_readfe(low[src->id]);

    if (b_start_time[dest->id] < lowsrc)
    {
      printf("error: rdv::te(%d,%d): low[%d]:%d,low[%d]:%d\n",
             src->id, dest->id, src->id, low[src->id], dest->id, low[dest->id]);

      lowsrc = b_start_time[dest->id];
    }

    mt_write(low[src->id], lowsrc);

    if (src->id == 42212 || src->id == 296693 ||
        src->id == 689561 ||
        src->id == 522156 || src->id == 4592086 ||
        src->id == 4545236)
    {
      printf("rdv::te(%d) update low[%d]: %d\n", src->id, src->id, lowsrc);
    }

    int highsrc = mt_readfe(high[src->id]);

    if (b_finish_time[dest->id] > highsrc)
    {
      printf("error: rdv::te(%d,%d): high[%d]:%d,high[%d]:%d\n",
             src->id, dest->id, src->id, high[src->id],
             dest->id, high[dest->id]);

      highsrc = b_finish_time[dest->id];
    }

    mt_write(high[src->id], highsrc);

    if (src->id == 42212 || src->id == 296693 ||
        src->id == 689561 ||
        src->id == 522156 || src->id == 4592086 ||
        src->id == 4545236)
    {
      printf("rdv::te(%d) update high[%d]: %d\n", src->id, src->id, highsrc);
    }

    if ((low[src->id]  < b_start_time[src->id]) ||
        (high[src->id] > b_finish_time[src->id]))
    {
      valid[src->id] = false;
    }
  }

  void back_edge(edge e, vertex src)
  {
    vertex dest = other(e, src, *g);
    int lowsrc = mt_readfe(low[src->id]);

    if (b_start_time[dest->id] < lowsrc) lowsrc = b_start_time[dest->id];

    mt_write(low[src->id], lowsrc);

    if (src->id == 42212 || src->id == 296693 ||
        src->id == 689561 ||
        src->id == 522156 || src->id == 4592086 ||
        src->id == 4545236)
    {
      printf("rdv::be(%d) update low[%d]: %d\n", src->id, src->id, lowsrc);
    }

    int highsrc = mt_readfe(high[src->id]);

    if (b_finish_time[dest->id] > highsrc) highsrc = b_finish_time[dest->id];

    mt_write(high[src->id], highsrc);

    if (src->id == 42212 || src->id == 296693 ||
        src->id == 689561 ||
        src->id == 522156 || src->id == 4592086 ||
        src->id == 4545236)
    {
      printf("rdv::be(%d) update high[%d]: %d\n", src->id, src->id, highsrc);
    }

    if ((low[src->id]  < b_start_time[src->id]) ||
        (high[src->id] > b_finish_time[src->id]))
    {
      valid[src->id] = false;
    }
  }

  void finish_vertex(vertex v)
  {
    int time = mt_incr(next_start_time, 0);
    rdv_fintime[v->id] = 1;
    if (v->id == 42212 || v->id == 296693 || v->id == 689561 ||
        v->id == 522156 || v->id == 4592086 || v->id == 4545236)
    {
      printf("rdv::fv(%d) finishing %d at time: %d\n",
             v->id, v->id, time);
    }

    bool new_psearch_tree = false;

    if ((low[v->id] == b_start_time[v->id]) &&
        (high[v->id] == b_finish_time[v->id]))
    {
      //int ok_before_337 = (start_time[3372706]==0);
      //int ok_before_459 = (start_time[4592806]==0);
      //bool invalid_before_337 = (valid[3372706]==0);
      //bool invalid_before_459 = (valid[4592086]==0);
      new_psearch_tree = true;
      int num_events = 2 * descendant_count[v->id];
      int true_psearch_start = mt_incr(next_start_time, num_events);
      start_time[v->id] = true_psearch_start;
      finish_time[v->id] = true_psearch_start + num_events - 1;
      bool print = false;

      if ((serial_psearch_max > descendant_count[v->id]) &&
          (descendant_count[v->id] > 1))
      {
        relabel_event_visitor<graph> srv(psearch_parent,
                                         descendant_count,
                                         start_time, finish_time, vert_mask,
                                         low, high, b_start_time, b_finish_time,
                                         next_start_time,
                                         valid, print, b_leader, rdv_fintime);

        psearch<graph, int*, relabel_event_visitor<graph>,
                AND_FILTER, DIRECTED>
        psearch(*g, searchColor2, srv);

        psearch.mixedRun(v, valid);
      }

      if (psearch_parent[v->id] != -1)
      {
        int discount = descendant_count[v->id];
        mt_incr(descendant_count[psearch_parent[v->id]], -discount);
      }

      valid[v->id] = true;
    }

    if (psearch_parent[v->id] >= 0)
    {
      int psearchp = psearch_parent[v->id];

      if (!valid[v->id]) valid[psearchp] = false;

      int lofp = mt_readfe(low[psearchp]);

      if (low[v->id] < lofp) lofp = low[v->id];

      mt_write(low[psearchp], lofp);

      time = mt_incr(next_start_time, 0);

      if (v->id == 42212 || v->id == 296693 || v->id == 689561 ||
          v->id == 522156 || v->id == 4592086 || v->id == 4545236 ||
          psearchp  == 42212 || psearchp  == 296693 || psearchp  == 689561 ||
          psearchp  == 522156 || psearchp  == 4592086 || psearchp  == 4545236)
      {
        printf("rdv::fv(%d) update low[%d]: %d at time: %d\n",
               v->id, psearchp, lofp, time);
      }

      int hofp = mt_readfe(high[psearchp]);

      if (high[v->id] > hofp) hofp = high[v->id];

      mt_write(high[psearchp], hofp);

      if (v->id == 42212 || v->id == 296693 || v->id == 689561 ||
          v->id == 522156 || v->id == 4592086 || v->id == 4545236 ||
          psearchp  == 42212 || psearchp  == 296693 || psearchp  == 689561 ||
          psearchp  == 522156 || psearchp  == 4592086 || psearchp  == 4545236)
      {
        printf("rdv::fv(%d) update high[%d]: %d at time: %d\n",
               v->id, psearchp, hofp, time);
      }

      if (new_psearch_tree) psearch_parent[v->id] = -1;
    }
  }

private:
  graph* g;
  int orig_descendant_count;
  int* descendant_count;
  int* b_start_time;
  int* b_finish_time;
  int* b_leader;
  int* psearch_parent;         // starts as branching parent; relabeled
  int* start_time;
  int* finish_time;
  int* low;
  int* high;
  int* searchColor2;
  int* rdv_fintime;
  int& next_start_time;
  int* vert_mask;
  bool* valid;
  int serial_psearch_max;
};

// verify the parentheses property
template <typename graph>
class psearch_event_debugger : public default_psearch_visitor<graph> {
public:
  typedef typename graph_traits<graph>::vertex_descritor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  psearch_event_debugger(int* psearchp, int* st, int* ft, int ordr, int* vm,
                         graph& _g) :
    start_time(st), finish_time(ft), psearch_parent(psearchp),
    order(ordr), vert_mask(vm), g(_g) {}

  void pre_visit(vertex v)
  {
    printf("ev_debug: %d: st[%d], ft[%d]\n", v->id,
           start_time[v->id], finish_time[v->id]);
    assert(start_time[v->id] < finish_time[v->id]);
  }

  bool visit_test(edge e, vertex src)
  {
    vertex dest = other(e, src, g);
    return ((!vert_mask || vert_mask[dest->id]) &&
            (psearch_parent[dest->id] == src->id));
  }

  void tree_edge(edge e, vertex src)
  {
    vertex dest = other(e, src, g);
    assert(start_time[src->id] < start_time[dest->id]);
    assert(finish_time[src->id] > finish_time[dest->id]);
  }

private:
  int order;
  int* psearch_parent;
  int* start_time;
  int* finish_time;
  int* vert_mask;
  graph& g;
};

template <typename graph>
class error_finding_visitor : public default_psearch_visitor<graph> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  error_finding_visitor(int* fintime, int myfin, graph& _g) :
    scc(fintime), my_scc(myfin), g(_g) {}

  void psearch_root(vertex v)
  {
    if (scc[v->id] != my_scc)
    {
      printf("efv: fintime: %d, scc: %d\n", scc[v->id], my_scc);
    }
  }

  bool visit_test(edge e, vertex v)
  {
    vertex dest = other(e, src, g);

    if (scc[dest->id] != my_scc)
    {
      printf("efv(%d,%d): fintime: %d, scc: %d\n",
             v->id, dest->id, scc[v->id], my_scc);
    }

    return true;
  }

private:
  int* scc, my_scc;
  graph& g;
};

}

#endif

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
/*! \file strongly_connected_components.hpp

    \brief This code contains the scc() routine that invokes visitors defined
           in scc_visitor.hpp.

    \author Jon Berry (jberry@sandia.gov)

    \date 3/2/2005

    Strongly connected components algorithms:
      * strongly_connected_components_dcsc:
          divide and conquer a algorithm of Coppersmith, Fleischer,
          Hendrickson, and Pinar.
      * strongly_connected_components_dc_simple:
          simpler version of the same, in which there is no preprocessing to
          remove topsort prefix/suffix.
*/
/****************************************************************************/

#ifndef MTGL_STRONGLY_CONNECTED_COMPONENTS_HPP
#define MTGL_STRONGLY_CONNECTED_COMPONENTS_HPP

#include <cstdio>
#include <climits>
#include <cassert>

#include <mtgl/scc_visitor.hpp>
#include <mtgl/graph_traits.hpp>
#include <mtgl/mtgl_adapter.hpp>
#include <mtgl/recursive_dcsc.hpp>
#include <mtgl/topsort.hpp>
#include <mtgl/util.hpp>

#ifdef __MTA__
#include <sys/mta_task.h>
#include <machine/runtime.h>
#endif

namespace mtgl {

#define FORWARD  0
#define BACKWARD 1
#define OTHER    2

#define FWD_MARK 1    // marked in fwd dir = 1
#define BWD_MARK 2    // marked in bwd dir = 2
#define NO_MARK  0    // not visited

template <typename graph_adapter>
class strongly_connected_components {
public:
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_t;
  typedef typename graph_traits<graph_adapter>::vertex_iterator vertex_iterator;

  strongly_connected_components(graph_adapter& gg, int* res) :
    ga(gg), result(res) {}

  int run()
  {
    mt_timer mttimer;
    mttimer.start();
#ifdef __MTA__
    int issues = mta_get_task_counter(RT_ISSUES);
    int memrefs = mta_get_task_counter(RT_MEMREFS);
    int m_nops = mta_get_task_counter(RT_M_NOP);
    int a_nops = mta_get_task_counter(RT_M_NOP);
    int fp_ops = mta_get_task_counter(RT_FLOAT_TOTAL);
#endif

    bool done = false;
    int num_components = 0;
    unsigned int order = num_vertices(ga);
    int* color = (int*) malloc(order * sizeof(int));
    short* scc_mark = (short*) malloc(sizeof(short) * order);

    vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, ga);
    vertex_iterator verts = vertices(ga);

    #pragma mta assert nodep
    for (unsigned int i = 0; i < order; i++) result[i] = NO_SCC;

    while (!done)
    {
      int next = -1;

      #pragma mta assert parallel
      for (unsigned int i = 0; i < order; i++)
      {
        if (result[i] == -1) next = i;
      }

      if (next == -1) break;

      #pragma mta assert nodep
      for (unsigned int i = 0; i < order; i++)
      {
        color[i] = 0;
        scc_mark[i] = NO_MARK;
      }

      forward_scc_visitor<graph_adapter>
      fvis(ga, scc_mark, result, vid_map);

      backward_scc_visitor<graph_adapter>
      bvis(ga, scc_mark, result, vid_map);

      result[next] = next;

      psearch<graph_adapter, int*, forward_scc_visitor<graph_adapter>,
              AND_FILTER, DIRECTED> psrch(ga, color, fvis);

      psrch.run(verts[next]);

      #pragma mta assert nodep
      for (unsigned int i = 0; i < order; i++) color[i] = 0;

      psearch<graph_adapter, int*, backward_scc_visitor<graph_adapter>,
              AND_FILTER, REVERSED> psrch2(ga, color, bvis);

      psrch2.run(verts[next]);

      #pragma mta assert nodep
      for (unsigned int i = 0; i < order; i++)
      {
        if (scc_mark[i] == (FWD_MARK | BWD_MARK)) result[i] = next;
      }

      num_components++;
    }

#ifdef __MTA__
    issues = mta_get_task_counter(RT_ISSUES) - issues;
    memrefs = mta_get_task_counter(RT_MEMREFS) - memrefs;
    m_nops = mta_get_task_counter(RT_M_NOP)  - m_nops;
    a_nops = mta_get_task_counter(RT_A_NOP)  - a_nops;
    a_nops = mta_get_task_counter(RT_FLOAT_TOTAL) - fp_ops;
#endif

    mttimer.stop();
    long postticks = mttimer.getElapsedTicks();

#ifdef __MTA__
    static int total_issues, total_memrefs, total_fp_ops, total_int;
    total_issues += issues;
    total_memrefs += memrefs;
    total_fp_ops += fp_ops;
    total_int += (issues - a_nops - m_nops);
    printf("SCC issues: %d,memrefs: %d, fp_ops: %d,"
           " ints+muls: %d,memratio:%6.2f\n",
           total_issues, total_memrefs, total_fp_ops, total_int,
           total_memrefs / (double) total_issues);
#endif

    printf("there are %d strongly connected components\n", num_components);
    return postticks;
  }

private:
  template <typename U>
  void print_array(U* A, int size, char* tag = NULL)
  {
    assert(A != NULL);

    if (tag == NULL)
    {
      printf("%-14s:\t[", "A");
    }
    else
    {
      printf("%-14s:\t[", tag);
    }

    for (int i = 0; i < size; i++) printf(" %2d", A[i]);

    printf(" ]\n");
  }

  graph_adapter& ga;
  int* result;
};

template <typename graph_adapter>
class strongly_connected_components_dc_simple {
public:
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_t;

  strongly_connected_components_dc_simple(graph_adapter& _ga, int* res) :
    ga(_ga), result(res) {}

  int run()
  {
    mt_timer mttimer;
    mttimer.start();

#ifdef __MTA__
    int issues = mta_get_task_counter(RT_ISSUES);
    int memrefs = mta_get_task_counter(RT_MEMREFS);
    int m_nops = mta_get_task_counter(RT_M_NOP);
    int a_nops = mta_get_task_counter(RT_M_NOP);
    int fp_ops = mta_get_task_counter(RT_FLOAT_TOTAL);
#endif

    bool done = false;
    int numSCC = 0, numNontrivSCC = 0;
    unsigned int order = num_vertices(ga);

    vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, ga);

    int my_subproblem = 0;
    int next_subproblem = 1;
    int subproblem_src = 0;
    int* subproblem = (int*) malloc(num_vertices(ga) * sizeof(int));
    bool* forward_mark = (bool*) malloc(num_vertices(ga) * sizeof(bool));
    bool* backward_mark = (bool*) malloc(num_vertices(ga) * sizeof(bool));

    #pragma mta assert nodep
    for (unsigned int i = 0; i < order; i++)
    {
      result[i] = NO_SCC;
      subproblem[i] = subproblem_src;
    }

    scc_aux(ga, subproblem_src, my_subproblem, &next_subproblem,
            forward_mark, backward_mark, subproblem, result, &numSCC,
            &numNontrivSCC, vid_map);

    free(forward_mark);
    free(backward_mark);
    free(subproblem);

#ifdef __MTA__
    issues = mta_get_task_counter(RT_ISSUES) - issues;
    memrefs = mta_get_task_counter(RT_MEMREFS) - memrefs;
    m_nops = mta_get_task_counter(RT_M_NOP)  - m_nops;
    a_nops = mta_get_task_counter(RT_A_NOP)  - a_nops;
    a_nops = mta_get_task_counter(RT_FLOAT_TOTAL) - fp_ops;
#endif

    mttimer.stop();
    long postticks = mttimer.getElapsedTicks();

#ifdef __MTA__
    static int total_issues, total_memrefs, total_fp_ops, total_int;
    total_issues += issues;
    total_memrefs += memrefs;
    total_fp_ops += fp_ops;
    total_int += (issues - a_nops - m_nops);
    printf("SCC issues: %d,memrefs: %d, fp_ops: %d, "
           "ints+muls: %d,memratio:%6.2f\n",
           total_issues, total_memrefs, total_fp_ops, total_int,
           total_memrefs / (double) total_issues);
#endif

    printf("there are %d strongly connected components\n", numSCC);
    printf("there are %d non-triv strongly connected components\n",
           numNontrivSCC);
    return postticks;
  }

  void scc_aux(graph_adapter& ga, int src, int my_subprob, int* next_subprob,
               bool* forward_mark, bool* backward_mark, int* subprob,
               int* result, int* numSCC, int* numNontrivSCC,
               vertex_id_map<graph_adapter>& vid_map)
  {
    (void) mt_incr(*numSCC, 1);
    bool done = false;
    unsigned int order = num_vertices(ga);
    int* color = (int*) malloc(num_vertices(ga) * sizeof(int));

    #pragma mta assert nodep
    for (unsigned int i = 0; i < order; i++)
    {
      color[i] = 0;
      forward_mark[i]  = false;
      backward_mark[i] = false;
    }

    forward_scc_visitor2<graph_adapter>
    fvis(forward_mark, subprob, my_subprob, result, vid_map);

    backward_scc_visitor2<graph_adapter>
    bvis(backward_mark, subprob, my_subprob,  result, vid_map);

    psearch<graph_adapter, int*, forward_scc_visitor2<graph_adapter>,
            AND_FILTER, DIRECTED> psrch(ga, color, fvis);

    psrch.run(get_vertex(src, ga));

    #pragma mta assert nodep
    for (unsigned int i = 0; i < order; i++) color[i] = 0;

    psearch<graph_adapter, int*, backward_scc_visitor2<graph_adapter>,
            AND_FILTER, REVERSED> psrch2(ga, color, bvis);

    psrch2.run(get_vertex(src, ga));

    int subprob_ids[3], problem_src[3];

    subprob_ids[FORWARD]  = mt_incr(*next_subprob, 1);
    subprob_ids[BACKWARD] = mt_incr(*next_subprob, 1);
    subprob_ids[OTHER]    = mt_incr(*next_subprob, 1);

    problem_src[FORWARD]  = -1;
    problem_src[BACKWARD] = -1;
    problem_src[OTHER]    = -1;
    int fsubct = 0;
    int bsubct = 0;
    int osubct = 0;
    int sccsz  = 0;

    #pragma mta assert parallel
    for (unsigned int i = 0; i < order; i++)
    {
      if (subprob[i] != my_subprob) continue;

      if (forward_mark[i] && !backward_mark[i])
      {
        problem_src[FORWARD] = i;
        subprob[i] = subprob_ids[FORWARD];
        mt_incr(fsubct, 1);
      }
      else if (!forward_mark[i] && backward_mark[i])
      {
        problem_src[BACKWARD] = i;
        subprob[i] = subprob_ids[BACKWARD];
        mt_incr(bsubct, 1);
      }
      else if (!forward_mark[i] && !backward_mark[i])
      {
        problem_src[OTHER] = i;
        subprob[i] = subprob_ids[OTHER];
        mt_incr(osubct, 1);
      }
      else if (forward_mark[i] && backward_mark[i])
      {
        result[i] = src;
        subprob[i] = NO_SCC;
        mt_incr(sccsz, 1);
      }
    }

    if (sccsz > 1) (void) mt_incr(*numNontrivSCC, 1);

    free(color);

    #pragma mta assert parallel
    #pragma mta loop future
    for (int i = 0; i < 3; i++)
    {
      if (problem_src[i] >= 0)
      {
        int ps = problem_src[i];
        int sp = subprob_ids[i];
        bool* forward_mark = (bool*) malloc(num_vertices(ga) * sizeof(bool));
        bool* backward_mark = (bool*) malloc(num_vertices(ga) * sizeof(bool));

        scc_aux(ga, ps, sp, next_subprob, forward_mark, backward_mark, subprob,
                result, numSCC, numNontrivSCC, vid_map);

        free(forward_mark);
        free(backward_mark);
      }
    }
  }

private:
  template <typename U>
  void print_array(U* A, int size, char* tag = NULL)
  {
    assert(A != NULL);

    if (tag == NULL)
    {
      printf("%-14s:\t[", "A");
    }
    else
    {
      printf("%-14s:\t[", tag);
    }

    for (int i = 0; i < size; i++) printf(" %2d", A[i]);

    printf(" ]\n");
  }

  graph_adapter& ga;
  int* result;
};

#if 1
template <typename graph_adapter>
class strongly_connected_components_dcsc {
public:
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_t;

  strongly_connected_components_dcsc(graph_adapter& gg, int* res) :
    ga(gg), result(res) {}

  int run()
  {
    mt_timer mttimer;
    mttimer.start();

#ifdef __MTA__
    int issues = mta_get_task_counter(RT_ISSUES);
    int memrefs = mta_get_task_counter(RT_MEMREFS);
    int m_nops = mta_get_task_counter(RT_M_NOP);
    int a_nops = mta_get_task_counter(RT_M_NOP);
    int fp_ops = mta_get_task_counter(RT_FLOAT_TOTAL);
#endif

    bool done = false;
    int numSCC = 0, numNontrivSCC = 0;
    unsigned int order = num_vertices(ga);

    int my_subproblem = 0;
    int next_subproblem = 1;
    int subproblem_src = 0;
    int* subproblem = (int*) malloc(order * sizeof(int));
    int* in_degree;
    int* out_degree;
    int num_in_deg_0 = 0;
    int num_out_deg_0 = 0;

    vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, ga);

    compute_in_degree<graph_adapter> cid(ga);
    in_degree = cid.run();

    compute_out_degree<graph_adapter> cod(ga);
    out_degree = cod.run();

    #pragma mta assert nodep
    for (unsigned int i = 0; i < order; i++)
    {
      result[i] = NO_SCC;
      subproblem[i] = subproblem_src;

      if (in_degree[i] == 0) num_in_deg_0++;
      if (out_degree[i] == 0) num_out_deg_0++;
    }

    scc_aux3(ga, subproblem_src, my_subproblem, &next_subproblem,
             in_degree, out_degree, subproblem, result, &numSCC,
             &numNontrivSCC, vid_map);

    free(subproblem);

#ifdef __MTA__
    issues = mta_get_task_counter(RT_ISSUES) - issues;
    memrefs = mta_get_task_counter(RT_MEMREFS) - memrefs;
    m_nops = mta_get_task_counter(RT_M_NOP) - m_nops;
    a_nops = mta_get_task_counter(RT_A_NOP) - a_nops;
    a_nops = mta_get_task_counter(RT_FLOAT_TOTAL) - fp_ops;
#endif

    mttimer.stop();
    long postticks = mttimer.getElapsedTicks();

#ifdef __MTA__
    static int total_issues, total_memrefs, total_fp_ops, total_int;
    total_issues  += issues;
    total_memrefs += memrefs;
    total_fp_ops  += fp_ops;
    total_int     += (issues - a_nops - m_nops);
    printf("SCC issues: %d,memrefs: %d, fp_ops: %d, "
           "ints+muls: %d,memratio:%6.2f\n",
           total_issues, total_memrefs, total_fp_ops, total_int,
           total_memrefs / (double) total_issues);
#endif

    printf("there are %d strongly connected components\n", numSCC);
    printf("there are %d non-triv strongly connected components\n",
           numNontrivSCC);

    return postticks;
  }


  void scc_aux3(graph_adapter& ga, int src, int my_subprob,
                int* next_subprob, int* in_degree, int* out_degree,
                int* subprob, int* result, int* numSCC,
                int* numNontrivSCC,
                vertex_id_map<graph_adapter>& vid_map)
  {
    //printf("scc_aux %d\n",src);
    (void) mt_incr(*numSCC, 1);
    bool done = false;
    unsigned int order = num_vertices(ga);
    int* color = (int*) malloc(order * sizeof(int));
    int* tmp_in_degree  = (int*) malloc(order * sizeof(int));
    int* tmp_out_degree = (int*) malloc(order * sizeof(int));
    bool* forward_mark   = (bool*) malloc(order * sizeof(bool));
    bool* backward_mark  = (bool*) malloc(order * sizeof(bool));

    #pragma mta assert nodep
    for (unsigned int i = 0; i < order; i++)
    {
      color[i] = 0;
      forward_mark[i] = false;
      backward_mark[i] = false;
      tmp_in_degree[i]  = in_degree[i];
      tmp_out_degree[i] = out_degree[i];
    }

    topsort_prefix<graph_adapter> tp(ga, tmp_in_degree, subprob, my_subprob);
    topsort_suffix<graph_adapter> ts(ga, tmp_out_degree, subprob, my_subprob);
    tp.run();
    ts.run();

    #pragma mta assert nodep
    for (unsigned int i = 0; i < order; i++)
    {
      if ((src == i) || (subprob[i] != my_subprob)) continue;

      if (tmp_in_degree[i] == 0)
      {
        result[i] = i;
        subprob[i] = NO_SCC;
        (void) mt_incr(*numSCC, 1);
      }
      else if (tmp_out_degree[i] == 0)
      {
        result[i] = i;
        subprob[i] = NO_SCC;
        (void) mt_incr(*numSCC, 1);
      }
    }

    free(tmp_in_degree);
    free(tmp_out_degree);

    forward_scc_visitor2<graph_adapter>
    fvis(forward_mark, subprob, my_subprob, result, vid_map);

    backward_scc_visitor2<graph_adapter>
    bvis(backward_mark, subprob, my_subprob, result, vid_map);

    psearch<graph_adapter, int*, forward_scc_visitor2<graph_adapter>,
            AND_FILTER, DIRECTED, 1> psrch(ga, color, fvis);

    psrch.run(get_vertex(src, ga));

    #pragma mta assert nodep
    for (unsigned int i = 0; i < order; i++) color[i] = 0;

    psearch<graph_adapter, int*, backward_scc_visitor2<graph_adapter>,
            AND_FILTER, REVERSED, 1> psrch2(ga, color, bvis);

    psrch2.run(get_vertex(src, ga));

    int subprob_ids[3], problem_src[3];

    subprob_ids[FORWARD]  = mt_incr(*next_subprob, 1);
    subprob_ids[BACKWARD] = mt_incr(*next_subprob, 1);
    subprob_ids[OTHER]    = mt_incr(*next_subprob, 1);

    problem_src[FORWARD] = -1;
    problem_src[BACKWARD] = -1;
    problem_src[OTHER] = -1;
    int fsubct = 0;
    int bsubct = 0;
    int osubct = 0;
    int sccsz = 0;

    #pragma mta assert parallel
    for (unsigned int i = 0; i < order; i++)
    {
      if (subprob[i] != my_subprob) continue;

      if (forward_mark[i] && !backward_mark[i])
      {
        problem_src[FORWARD] = i;
        subprob[i] = subprob_ids[FORWARD];
        mt_incr(fsubct, 1);
      }
      else if (!forward_mark[i] && backward_mark[i])
      {
        problem_src[BACKWARD] = i;
        subprob[i] = subprob_ids[BACKWARD];
        mt_incr(bsubct, 1);
      }
      else if (!forward_mark[i] && !backward_mark[i])
      {
        problem_src[OTHER] = i;
        subprob[i] = subprob_ids[OTHER];
        mt_incr(osubct, 1);
      }
      else if (forward_mark[i] && backward_mark[i])
      {
        result[i] = src;
        subprob[i] = NO_SCC;
        mt_incr(sccsz, 1);
      }
    }

    free(forward_mark);
    free(backward_mark);

    if (sccsz > 1) (void) mt_incr(*numNontrivSCC, 1);

    free(color);

    #pragma mta assert parallel
    #pragma mta loop future
    for (int i = 0; i < 3; i++)
    {
      if (problem_src[i] >= 0)
      {
        int ps = problem_src[i];
        int sp  = subprob_ids[i];
        scc_aux3(ga, ps, sp, next_subprob, in_degree, out_degree, subprob,
                 result, numSCC, numNontrivSCC, vid_map);
      }
    }
  }

private:
  graph_adapter& ga;
  int* result;
};
#endif

#undef FORWARD
#undef BACKWARD
#undef OTHER

#undef FWD_MARK
#undef BWD_MARK
#undef NO_MARK

}

#endif

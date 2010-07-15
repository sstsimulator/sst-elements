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
/*! \file test_indset.cpp

    \author Jon Berry (jberry@sandia.gov)

    \date 4/26/2009
*/
/****************************************************************************/

#include <cstdlib>

#include <mtgl/static_graph_adapter.hpp>
#include <mtgl/quicksort.hpp>
#include <mtgl/generate_rmat_graph.hpp>
#include <mtgl/breadth_first_search_mtgl.hpp>
#include <mtgl/visit_adj.hpp>
#include <mtgl/metrics.hpp>
#include <mtgl/read_matrix_market.hpp>
#include <mtgl/read_dimacs.hpp>
#include <mtgl/mtgl_io.hpp>

using namespace std;
using namespace mtgl;

#define ERRMAX 10
template <class graph>
void valid_max_ind_set(graph& g, bool* indset)
{
  typedef typename graph_traits<graph>::vertex_descriptor vertex_descriptor;
  typedef typename graph_traits<graph>::size_type size_type;

  size_type order = num_vertices(g);
  vertex_descriptor* end_points = g.get_end_points();

  mtgl::pair<int, int>* conflicts = new mtgl::pair<int, int>[ERRMAX];

  int* isolated = new int[ERRMAX];
  int num_conflicts = 0;
  int num_isolated = 0;

  #pragma mta assert nodep
  for (int i = 0; i < order; i++)
  {
    int begin = g[i];
    int end = g[i + 1];
    bool not_maximal = true;
    bool conflict = false;

    #pragma mta assert nodep
    for (int j = begin; j < end; j++)
    {
      int dest = end_points[j];

      if (i == dest) continue;

      if (indset[dest]) not_maximal = false;

      if (indset[i] && indset[dest])
      {
        if (num_conflicts < ERRMAX)
        {
          int pos = mt_incr(num_conflicts, 1);
          conflicts[pos] = mtgl::pair<int, int>(i, dest);
        }
      }
    }

    if (!indset[i] && not_maximal)
    {
      if (num_isolated < ERRMAX)
      {
        int pos = mt_incr(num_isolated, 1);
        isolated[pos] = i;
      }

    }
  }

  for (int i = 0; i < num_conflicts; i++)
  {
    printf(" conflict: %d\t%d\n", conflicts[i].first, conflicts[i].second);
  }

  for (int i = 0; i < num_isolated; i++)
  {
    printf(" isolated: %d\n", isolated[i]);
  }
}

#if 0
template <class graph>
class qt_luby_game_args {
public:
  qt_luby_game_args(graph& gg, int* actind, bool* act,
                    bool* indst, double* val) :
    g(gg), active_ind(actind), active(act), indset(indst), value(val) {}

  graph& g;
  int* active_ind;
  bool* indset;
  bool* active;
  double* value;
};

template <class graph>
void luby_game_outer(qthread_t* me, const size_t startat,
                     const size_t stopat, void* arg)
{
  typedef typename graph_traits<graph>::vertex_descriptor vertex_descriptor;

  qt_luby_game_args<graph>* qt_linfo = (qt_luby_game_args<graph>*)arg;

  for (size_t k = startat; k < stopat; k++)
  {
    int* my_active_ind = qt_linfo->active_ind;
    int i = my_active_ind[k];
    int begin = qt_linfo->g[i];
    int end = qt_linfo->g[i + 1];
    bool* my_indset = qt_linfo->indset;
    bool* my_active = qt_linfo->active;
    double* my_value = qt_linfo->value;
    vertex_descriptor* my_end_points = qt_linfo->g.get_end_points();

    for (int j = begin; j < end; j++)
    {
      int dest = my_end_points[j];
      if (my_active[dest])
      {
        double cs = my_value[i];
        double cd = my_value[dest];

        if (!(my_indset[i] && my_indset[dest])) continue;

        if (cs < cd)
        {
          my_indset[i] = false;
        }
        else if (cs > cd)
        {
          my_indset[dest] = false;
        }
        else if (i < dest)
        {
          my_indset[dest] = false;
        }
        else if (i > dest)
        {
          my_indset[i] = false;
        }
      }
    }
  }
}

template <class graph>
class qt_luby_deactivate_args {
public:
  qt_luby_deactivate_args(graph& gg, int* actind, bool* act, bool* indst,
                          int& new_indset_size) :
    g(gg), active_ind(actind), active(act), indset(indst),
    new_ind_set_size(new_indset_size) {}

  graph& g;
  int* active_ind;
  bool* indset;
  bool* active;
  int& new_ind_set_size;
};

template <class graph>
void luby_deactivate_outer(qthread_t* me,
                           const size_t startat, const size_t stopat, void* arg)
{
  typedef typename graph_traits<graph>::vertex_descriptor vertex_descriptor;

  qt_luby_deactivate_args<graph>* qt_linfo =
    (qt_luby_deactivate_args<graph>*)arg;

  for (size_t k = startat; k < stopat; k++)
  {
    int* my_active_ind = qt_linfo->active_ind;
    int i = my_active_ind[k];
    int begin = qt_linfo->g[i];
    int end = qt_linfo->g[i + 1];
    bool* my_indset = qt_linfo->indset;
    bool* my_active = qt_linfo->active;
    int& new_ind_set_size = qt_linfo->new_ind_set_size;
    vertex_descriptor* my_end_points = qt_linfo->g.get_end_points();

    if (my_indset[i])
    {
      mt_incr(new_ind_set_size, 1);
      if (my_active[i]) my_active[i] = false;
    }

    for (int j = begin; j < end; j++)
    {
      int dest = my_end_points[j];

      if (my_indset[i] && my_active[dest])
      {
        my_active[dest] = false;
      }
    }
  }
}

template <class graph>
class qt_luby_setup_args {
public:
  qt_luby_setup_args(graph& gg, int* actind, int* next_actind,
                     bool* act, bool* indst, int& new_numactive) :
    g(gg), active_ind(actind), next_active_ind(next_actind),
    active(act), indset(indst), new_num_active(new_numactive) {}

  graph& g;
  int* active_ind;
  int* next_active_ind;
  bool* indset;
  bool* active;
  int& new_num_active;
};

template <class graph>
void luby_setup_outer(qthread_t* me, const size_t startat,
                      const size_t stopat, void* arg)
{
  typedef typename graph_traits<graph>::vertex_descriptor vertex_descriptor;

  qt_luby_setup_args<graph>* qt_linfo = (qt_luby_setup_args<graph>*)arg;

  for (size_t k = startat; k < stopat; k++)
  {
    int* my_active_ind = qt_linfo->active_ind;
    int* my_next_active_ind = qt_linfo->next_active_ind;
    int i = my_active_ind[k];
    bool* my_indset = qt_linfo->indset;
    bool* my_active = qt_linfo->active;
    int& new_num_active = qt_linfo->new_num_active;

    if (my_active[i])
    {
      int pos = mt_incr(new_num_active, 1);
      my_next_active_ind[pos] = i;
      my_indset[i] = true;
    }
  }
}
#endif

int main(int argc, char* argv[])
{
#ifndef __MTA__
  srand48(0);
#endif

  typedef static_graph_adapter<directedS> Graph;
  typedef graph_traits<Graph>::vertex_descriptor vertex_descriptor;
  typedef graph_traits<Graph>::size_type size_type;

  if (argc < 2)
  {
    fprintf(stderr, "Usage: %s {<p>|<filename>}", argv[0]);
    fprintf(stderr, "where specifying p requests a generated r-mat graph "
                    "with 2^p vertices,\n");
    fprintf(stderr, "and specifying filename requests that a DIMACS or "
                    "MatrixMarket file be read to build the graph;\n");
    fprintf(stderr, "DIMACS files must end with suffix .dimacs\n");
    fprintf(stderr, "MatrixMarket files must end with suffix .mtx\n");

    exit(1);
  }

#ifdef USING_QTHREADS
  if (argc < 3)
  {
    fprintf(stderr, "usage: %s <p> <threads>\n", argv[0]);
    fprintf(stderr, "(for 2^p vertex r-mat)\n");

    exit(1);
  }

  int threads = atoi(argv[2]);
  qthread_init(threads);
  aligned_t rets[threads];
  int args[threads];

  for (int i = 0; i < threads; i++)
  {
    args[i] = i;
    qthread_fork_to(setaffin, args + i, rets + i, i);
  }

  for (int i = 0; i < threads; i++)
  {
    qthread_readFF(NULL, rets + i, rets + i);
  }

  typedef aligned_t uinttype;
#else
  typedef unsigned long uinttype;
  //typedef int uinttype;
#endif

  // Determine which input format is used:  automatically generated rmat
  // or file-based input.
  int use_rmat = 1;
  int llen = strlen(argv[1]);

  for (int i = 0; i < llen; i++)
  {
    if (!(argv[1][i] >= '0' && argv[1][i] <= '9'))
    {
      // String contains non-numeric characters; it must be a filename.
      use_rmat = 0;
      break;
    }
  }

  Graph g;

  if (use_rmat)
  {
    generate_rmat_graph(g, atoi(argv[1]), 8);
  }
  else if (argv[1][llen - 1] == 'x')
  {
    // Matrix-market input
    dynamic_array<int> weights;

    read_matrix_market(g, argv[1], weights);
  }
  else if (strcmp(&argv[1][llen - 4], "srcs") == 0)
  {
    // Snapshot input: <file>.srcs, <file>.dests
    char* srcs_fname = argv[1];
    char dests_fname[256];

    strcpy(dests_fname, srcs_fname);
    strcpy(&dests_fname[llen - 4], "dests");

    read_binary(g, srcs_fname, dests_fname);
  }
  else if (argv[1][llen - 1] == 's')
  {
    // DIMACS input.
    dynamic_array<int> weights;

    read_dimacs(g, argv[1], weights);
  }
  else
  {
    fprintf(stderr, "Invalid filename %s\n", argv[1]);
  }

  size_type order = num_vertices(g);
  size_type size = num_edges(g);

  printf("ORDER: %d, SIZE: %d\n", (int)order, (int)size);

  int i, j;
  mt_timer timer;
  double* value = new double[order];
  bool* indset = new bool[order];
  bool* active = new bool[order];
  size_type ind_set_size = 0;
  size_type* active_ind = new size_type[order];
  size_type* next_active_ind = new size_type[order];

  memset(value, 0, order * sizeof(double));
  memset(indset, 1, order * sizeof(bool));
  memset(active, 1, order * sizeof(bool));

  //timer.start();
  //maximal_independent_set<Graph,bool*> mis(g, active);
  //mis.run(indset, ind_set_size);
  //timer.stop();
  //printf("mis time: %f\n", timer.getElapsedSeconds());

  timer.start();
  luby_maximal_independent_set<Graph> lmis(g, active);
  lmis.run(indset, ind_set_size);
  timer.stop();
  printf("luby time: %f\n", timer.getElapsedSeconds());

#if 0
  int prev_ind_set_size = -1;
  ind_set_size = 0;

  #pragma mta fence
  int issues, memrefs, concur, streams;
  init_mta_counters(timer, issues, memrefs, concur, streams);
  vertex_id_map<Graph> vid_map = get(_vertex_id_map, g);
  vertex_descriptor* end_points = g.get_end_points();

  if (order < 100) print(g);

  luby_maximal_independent_set<Graph> luby(g, active);
  size_type luby_result_size = 0;
  luby.run(indset, luby_result_size);
  printf("luby result size: %d\n", luby_result_size);

  #pragma mta assert nodep
  for (int i = 0; i < order; i++) active_ind[i] = i;

  int num_active = order;
  memset(active, 1, order * sizeof(bool));

  while (prev_ind_set_size < ind_set_size)
  {
    prev_ind_set_size = ind_set_size;

#ifdef USING_QTHREADS
    qt_luby_game_args<Graph> la(g, active_ind, active, indset, value);
    qt_loop_balance(0, num_active, luby_game_outer<Graph>, &la);
#else
    #pragma mta trace "about to run contest"
    #pragma mta assert nodep
    for (int a = 0; a < num_active; a++)
    {
      int i = active_ind[a];
      int begin = g[i];
      int end = g[i + 1];
      bool* my_indset = indset;
      double* my_value = value;
      vertex_descriptor* my_end_points = end_points;

      #pragma mta assert nodep
      for (int j = begin; j < end; j++)
      {
        int dest = my_end_points[j];
        if (active[dest])
        {
          double cs = my_value[i];
          double cd = my_value[dest];

          if (!(my_indset[i] && my_indset[dest])) continue;

          if (cs < cd)
          {
            my_indset[i] = false;
          }
          else if (cs > cd)
          {
            my_indset[dest] = false;
          }
          else if (i < dest)
          {
            my_indset[dest] = false;
          }
          else if (i > dest)
          {
            my_indset[i] = false;
          }
        }
      }
    }
#endif

    #pragma mta trace "about to deactivate losers"
    int new_ind_set_size = 0;

#ifdef USING_QTHREADS
    qt_luby_deactivate_args<Graph> da(g, active_ind, active, indset,
                                      new_ind_set_size);
    qt_loop_balance(0, num_active, luby_deactivate_outer<Graph>, &da);
#else
    #pragma mta assert nodep
    for (int a = 0; a < num_active; a++)
    {
      int i = active_ind[a];
      int begin = g[i];
      int end = g[i + 1];

      if (indset[i])
      {
        new_ind_set_size++;
        if (active[i]) active[i] = false;
      }

      #pragma mta assert nodep
      for (int j = begin; j < end; j++)
      {
        int dest = end_points[j];
        if (indset[i] && active[dest])
        {
          active[dest] = false;
        }
      }
    }
#endif

    int new_num_active = 0;
    ind_set_size += new_ind_set_size;

    #pragma mta trace "about to setup next round"
    #pragma mta fence
    #pragma mta assert nodep
    for (int a = 0; a < num_active; a++)
    {
      int i = active_ind[a];
      if (active[i])
      {
        int pos = mt_incr(new_num_active, 1);
        next_active_ind[pos] = i;
        indset[i] = true;
      }
    }

    size_type* tmp = next_active_ind;
    next_active_ind = active_ind;
    active_ind = tmp;
    num_active = new_num_active;

    printf("size: %d, active: %d\n", ind_set_size, num_active);
  }

  #pragma mta fence

  sample_mta_counters(timer, issues, memrefs, concur, streams);
  printf("indset rank performance stats: \n");
  printf("---------------------------------------------\n");
  print_mta_counters(timer, num_edges(g), issues, memrefs, concur, streams);

  printf("size: %d\n", ind_set_size);
  valid_max_ind_set(g, indset);
#endif

#ifdef USING_QTHREADS
  qthread_finalize();
#endif

  return 0;
}

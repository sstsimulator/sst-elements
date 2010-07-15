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
/*! \file test_st_search.cpp

    \author Jon Berry (jberry@sandia.gov)

    \date 12/4/2007
*/
/****************************************************************************/

#include "mtgl_test.hpp"

#include <mtgl/util.hpp>
#include <mtgl/static_graph_adapter.hpp>
#include <mtgl/sssp_deltastepping.hpp>
#include <mtgl/st_search.hpp>

using namespace mtgl;

double get_freq()
{
#ifdef __MTA__
  double freq = mta_clock_freq();
#else
  double freq = 1000000;
#endif

  return freq;
}

void init_array(int* a, int size)
{
  #pragma mta assert nodep
  for (int i = 0; i < size; i++) a[i] = 0;
}


template <typename graph>
void test_st_search(graph& ga, int vs, int vt, int dir)
{
  mt_timer mttimer;

  if (vs == -1)
  {
    vs = rand() % num_vertices(ga);
    printf("vs = %d\n", vs);
  }

  if (vt == -1)
  {
    do
    {
      vt = rand() % num_vertices(ga);
    } while (vt == vs && num_vertices(ga) > 1);

    printf("vt = %d\n", vt);
  }

  dynamic_array<int> short_path_ids;

  int ret_data_type = VERTICES;                 // EDGES or VERTICES
  int short_path_length;

  mttimer.start();

  if (dir == DIRECTED)
  {
    st_search<graph, DIRECTED>   sts(ga, vs, vt, short_path_ids);
    short_path_length = sts.run(ret_data_type);
  }
  else if (dir == UNDIRECTED)
  {
    st_search<graph, UNDIRECTED> sts(ga, vs, vt, short_path_ids);
    short_path_length = sts.run(ret_data_type);
  }
  else if (dir == REVERSED)
  {
    st_search<graph, REVERSED>   sts(ga, vs, vt, short_path_ids);
    short_path_length = sts.run(ret_data_type);
  }

  mttimer.stop();

  printf("RESULT: st: %d %d (%6.3lf)\n",
         short_path_length, short_path_ids.size(),
         mttimer.getElapsedSeconds());
}


template <typename graph>
void test_sssp_deltastepping(graph& ga, int vs)
{
  mt_timer mttimer;
  double cs;
  double* realWt = (double*) malloc(sizeof(double) * num_edges(ga));
  double* result = (double*) malloc(sizeof(double) * num_vertices(ga));

  // fill edge weights with ramdom data.
  #pragma mta assert no dependence
  for (int i = 0; i < num_edges(ga); i++)
  {
    realWt[i] = (double) (rand() % 99) / 100.00 + .01;
  }

  mttimer.start();

  sssp_deltastepping<graph, int>
  sssp_ds(ga, vs, realWt, &cs, result);
  sssp_ds.run();

  mttimer.stop();

  printf("RESULT: ds: %6.3lf (%6.3lf)\n", cs, mttimer.getElapsedSeconds());

  free(realWt);  realWt = NULL;
  free(result);  result = NULL;
}

int main(int argc, char* argv[])
{
  if (argc < 5)
  {
    fprintf(stderr, "usage: %s [-debug] "
            " [-ds ] "
            " --graph_type <dimacs|rmat> "
            " --level <levels> --graph <SCALE> "
            " --filename <dimacs graph filename> "
            " --vs <vsource>\n"
            , argv[0]);
    exit(1);
  }

  static_graph_adapter<directedS> ga;

  kernel_test_info kti;
  kti.process_args(argc, argv);
  kti.gen_graph(ga);

  if (num_vertices(ga) < 100)
  {
    print(ga);              // debugging!
  }

  printf("nVerts = %d\n", num_vertices(ga));
  printf("nEdges = %d\n", num_edges(ga));
  fflush(stdout);

  if (find(kti.algs, kti.algCount, "ds"))
  {
    test_sssp_deltastepping<graph_adapter>(ga, kti.vs);
  }
  else if ( find(kti.algs, kti.algCount, "st"))
  {
    test_st_search<graph_adapter>(ga, kti.vs, kti.vt, kti.direction);
  }
}

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
/*! \file test_sssp_deltastepping.cpp

    \author William McLendon (wcmclen@sandia.gov)

    \date 1/7/2008
*/
/****************************************************************************/

#include <cmath>

#include <mtgl/util.hpp>
#include <mtgl/static_graph_adapter.hpp>
#include <mtgl/read_matrix_market.hpp>
#include <mtgl/read_dimacs.hpp>
#include <mtgl/generate_rmat_graph.hpp>
#include <mtgl/metrics.hpp>
#include <mtgl/sssp_deltastepping.hpp>
#include <mtgl/rand_fill.hpp>
#include <mtgl/quicksort.hpp>
#include <mtgl/mtgl_io.hpp>

using namespace mtgl;

#define ASSERT 1

#ifdef __MTA__

#define BEGIN_LOGGING() \
  mta_resume_event_logging(); \
  int issues      = mta_get_task_counter(RT_ISSUES); \
  int streams     = mta_get_task_counter(RT_STREAMS); \
  int concurrency = mta_get_task_counter(RT_CONCURRENCY); \
  int traps       = mta_get_task_counter(RT_TRAP);

#define END_LOGGING() { \
  issues      = mta_get_task_counter(RT_ISSUES) - issues; \
  streams     = mta_get_task_counter(RT_STREAMS) - streams; \
  concurrency = mta_get_task_counter(RT_CONCURRENCY) - concurrency; \
  traps       = mta_get_task_counter(RT_TRAP) - traps; \
  mta_suspend_event_logging(); \
  printf("issues: %d, streams: %lf, " \
         "concurrency: %lf, " \
         "traps: %d\n", \
         issues, \
         streams / (double) issues, \
         concurrency / (double) issues, \
         traps); \
}

#else
  #define BEGIN_LOGGING() ;
  #define END_LOGGING()   ;
#endif

template <typename T>
void generate_edge_weights(T min, T max, T** realWt, int sz,
                           int max_weight = 100)
{
  T maxWt = 0;
  T* wgts  = (T*) malloc(sizeof(T) * sz);
  random_value* rvals = (random_value*) malloc(sizeof(random_value) * sz);

  rand_fill::generate(sz, rvals);

#if 1
  #pragma mta assert parallel
  for (int i = 0; i < sz; i++) wgts[i] = rvals[i] / 4294967295.0;
#endif

#if 0
  #pragma mta assert parallel
  for (int i = 0; i < sz; i++)
  {
    int max_pow = (int) (log(max_weight) / 0.301);
    assert(max_pow > 0);
    int pow = rvals[i] % max_pow;
    wgts[i] = ((T) (1 << (pow + 1))) / (1 << max_pow);
  }
#endif

  free(rvals);
  rvals = NULL;
  *realWt = wgts;
}

template <typename T>
void generate_range(T min, T max, T** Wt, int sz)
{
  assert(max > min);
  random_value maxWt = 0;
  T* wgts  = (T*) malloc(sizeof(T) * sz);
  random_value* rvals = (random_value*) malloc(sizeof(random_value) * sz);

  rand_fill::generate(sz, rvals);

  #pragma mta assert nodep
  for (int i = 0; i < sz; i++)
  {
    if (min == 0)
    {
      wgts[i] = rvals[i] % (max + 1);
    }
    else
    {
      wgts[i] = rvals[i] % (max - min) + min;
    }

//    if (i<100)
//    {
//      printf("%d ::: %d", i, (rvals[i]%(max-min))+min);  // debug stuff

//      if (rvals[i]%(max-min)+min <0)
//      {
//        printf(" ***\n");
//      }
//      else
//      {
//        printf("\n");
//      }
//    }
  }

  free(rvals);
  rvals = NULL;
  *Wt = wgts;
}

// ***************************************************************************
//
// DDDDD  DDDDDD DD    DDDDDD  DDDD     SSSSSS SSSSSS SSSSSS SSSSS
// DD  DD DD     DD      DD   DD  DD    SS       SS   SS     SS  SS
// DD  DD DDDDD  DD      DD   DDDDDD    SSSSSS   SS   SSSSS  SSSSS
// DD  DD DD     DD      DD   DD  DD        SS   SS   SS     SS
// DDDDD  DDDDDD DDDDDD  DD   DD  DD    SSSSSS   SS   SSSSSS SS
///
// ***************************************************************************
template <typename graph>
double* run_sssp_deltastepping(graph& ga, int vs, double* realWt,
                               double& checksum, double& time)
{
  double* result = (double*) malloc(sizeof(double) * num_vertices(ga));
  mt_timer timer;
  sssp_deltastepping<graph, int> sssp_ds(ga, vs, realWt, &checksum, result);

  timer.start();
  sssp_ds.run();
  timer.stop();
  time = timer.getElapsedSeconds();

  return result;
}

template <typename graph>
void test_sssp_deltastepping(graph& ga, int vs, int nSrcs, int nTrials = 1)
{
  double the_time = 0.0;
  double* realWt = NULL;
  int* srcs   = NULL;
  double checksum = 0.0;
  bool started = false;
  double prev_checksum = 0.0;

  generate_edge_weights<double>(0.00, 1.00, &realWt, num_edges(ga));

  srand48(3324545);
  generate_range<int>(0, num_vertices(ga) - 1, &srcs, nSrcs);

  if (vs != -1)
  {
    nSrcs   = 1;
    srcs[0] = vs;
  }

  double* result = 0;

  for (int i = 0; i < nSrcs; i++)
  {
    printf("srcs[i]: %d\n", srcs[i]);

    for (int j = 0; j < nTrials; j++)
    {
      double time = 0.0;
      double* result = run_sssp_deltastepping<graph>(ga, srcs[i], realWt,
                                                     checksum, time);

      the_time += time;

      if (started && checksum != prev_checksum)
      {
        fprintf(stderr, "sssp checksum error\n");
      }

      if (nTrials > 1 && j < nTrials)
      {
        free(result);
        result = 0;
      }

      started = true;
      prev_checksum = checksum;
    }

    printf("\n");

    if (nTrials == 1)
    {
      free(result);
      result = 0;
    }
  }

  printf("Avg time: %8.3lf\n", the_time / (nTrials * nSrcs));

  free(realWt);
  realWt = NULL;
  free(srcs);
  srcs   = NULL;
}

int main(int argc, char* argv[])
{
#ifndef __MTA__
  srand48(0);
#endif

  typedef static_graph_adapter<directedS> Graph;
  typedef graph_traits<Graph>::size_type size_type;

  if (argc < 2)
  {
    fprintf(stderr, "Usage: %s {<p>|<filename>} <src | -num_srcs>", argv[0]);
    fprintf(stderr, "where specifying p requests a generated r-mat graph "
            "with 2^p vertices,\n");
    fprintf(stderr, "and specifying filename requests that a DIMACS or "
            "MatrixMarket file be read to build the graph;\n");
    fprintf(stderr, "DIMACS files must end with suffix .dimacs\n");
    fprintf(stderr, "MatrixMarket files must end with suffix .mtx\n");
    fprintf(stderr, "<src> is the source vertex id.  if negative, \n");
    fprintf(stderr, "this is interpreted as the number of randomly-\n");
    fprintf(stderr, "generated sources to try.\n");

    exit(1);
  }
#ifdef USING_QTHREADS
  if (argc < 3)
  {
    fprintf(stderr, "usage: %s {<p>|<filename>} <src | -num_srcs> <threads>\n",
            argv[0]);
    fprintf(stderr, "where specifying p requests a generated r-mat graph "
            "with 2^p vertices,\n");
    fprintf(stderr, "and specifying filename requests that a DIMACS or "
            "MatrixMarket file be read to build the graph;\n");
    fprintf(stderr, "DIMACS files must end with suffix .dimacs\n");
    fprintf(stderr, "MatrixMarket files must end with suffix .mtx\n");
    fprintf(stderr, "<src> is the source vertex id.  if negative, \n");
    fprintf(stderr, "this is interpreted as the number of randomly-\n");
    fprintf(stderr, "generated sources to try.\n");

    exit(1);
  }

  int threads = atoi(argv[3]);
  printf("qthread_init(%d)\n", threads);
  fflush(stdout);
  qthread_init(threads);
  printf("qthread_init done(%d)\n", threads);
  fflush(stdout);
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
#endif

  // Determine which input format is used:  automatically generated rmat
  // or file-based input.
  int use_rmat = 1;
  int llen = strlen(argv[1]);
  for (int i = 0; i < llen; i++)
  {
    if (!(argv[1][i] >= '0' && argv[1][i] <= '9'))
    {
      // String contains non-numeric characters;
      // it must be a filename.
      use_rmat = 0;
      break;
    }
  }

  Graph ga;

  if (use_rmat)
  {
    generate_rmat_graph(ga, atoi(argv[1]), 8);
  }
  else if (argv[1][llen - 1] == 'x')
  {
    // Matrix-market input.
    dynamic_array<int> weights;

    read_matrix_market(ga, argv[1], weights);
  }
  else if (strcmp(&argv[1][llen - 4], "srcs") == 0)
  {
    // Snapshot input: <file>.srcs, <file>.dests
    char* srcs_fname = argv[1];
    char dests_fname[256];

    strcpy(dests_fname, srcs_fname);
    strcpy(&dests_fname[llen - 4], "dests");

    read_binary(ga, srcs_fname, dests_fname);
  }
  else if (argv[1][llen - 1] == 's')
  {
    // DIMACS input.
    dynamic_array<int> weights;

    read_dimacs(ga, argv[1], weights);
  }
  else
  {
    fprintf(stderr, "Invalid filename %s\n", argv[1]);
  }

  int s_arg = atoi(argv[2]);
  int vs = (s_arg >= 0) ? s_arg : -1;
  int nSrcs = (s_arg < 0) ? -s_arg : 0;

  printf("degree(%d,g) = %d\n", 0, (int) out_degree(get_vertex(0, ga), ga));
  printf("order(g) = %d\n", (int) num_vertices(ga));
  printf("size(g) = %d\n", (int) num_edges(ga));

  size_type order = num_vertices(ga);
  size_type size  = num_edges(ga);
  size_type trials = 1;
  vertex_id_map<Graph> vid_map = get(_vertex_id_map, ga);

  mt_timer timer;
  timer.start();
  test_sssp_deltastepping(ga, vs, nSrcs, trials);
  timer.stop();
  printf("sssp secs: %f\n", timer.getElapsedSeconds());

  return 0;
}

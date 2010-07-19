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
/*! \file test_pagerank.cpp

    \author Jon Berry (jberry@sandia.gov)

    \date 12/20/2007
*/
/****************************************************************************/

#include <cstdlib>

#include <mtgl/static_graph_adapter.hpp>
//#include <mtgl/adjacency_list_adapter.hpp>
#include <mtgl/pagerank.hpp>
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

int main(int argc, char* argv[])
{
  char * nargv[5];
#ifndef __MTA__
  srand48(0);
#endif

  typedef static_graph_adapter<bidirectionalS> Graph;
//  typedef adjacency_list_adapter<directedS> Graph;

  if (argc < 3)
  {
      argc = 4;
      nargv[0] = argv[0]; /* executable */
      nargv[1] = "8"; /* p */
      nargv[2] = "0.001"; /* delta */
      nargv[3] = "4"; /* # threads */
      nargv[4] = NULL;
      argv = nargv;
      /*
    fprintf(stderr, "Usage: %s {<p>|<filename>} <delta>\n", argv[0]);
    fprintf(stderr, "where specifying p requests a generated r-mat graph "
            "with 2^p vertices,\n");
    fprintf(stderr, "and specifying filename requests that a DIMACS or "
            "MatrixMarket file (or the .srcs file of a .srcs,.dests "
            "snapshot pair) be read to build the graph;\n");
    fprintf(stderr, "delta is the convergence tolerance of "
            "the power iteration.\n");
    fprintf(stderr, "DIMACS files must end with suffix .dimacs\n");
    fprintf(stderr, "MatrixMarket files must end with suffix .mtx\n");
    exit(1);
    */
  }

#ifdef USING_QTHREADS
  if (argc < 4)
  {
      qthread_initialize();
  } else {
      int threads = atoi(argv[3]);
      qthread_init(threads);
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
    if (!(argv[1][i] >= '0' && argv[1][i] <= '9'))
    {
      // String contains non-numeric characters; it must be a filename.
      use_rmat = 0;
      break;
    }

  Graph g;

  if (use_rmat)
  {
    generate_rmat_graph(g, atoi(argv[1]), 8);
  }
  else if (argv[1][llen - 1] == 'x')
  {
    // Matrix-market input.
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

  int order = num_vertices(g);
  int size = num_edges(g);
  printf("ORDER: %d, SIZE: %d\n", order, size);

  PIM_quickPrint(0,0,0);
  pagerank<Graph> pr(g);
  rank_info* rinfo = pr.run(atof(argv[2]));
  exit(1);

  mt_timer timer;
  int issues, memrefs, concur, streams;
  init_mta_counters(timer, issues, memrefs, concur, streams);

  rinfo = pr.run(atof(argv[2]));

  sample_mta_counters(timer, issues, memrefs, concur, streams);
  printf("Page rank performance stats: \n");
  printf("---------------------------------------------\n");
  print_mta_counters(timer, num_edges(g), issues, memrefs, concur, streams);

  // Print at most 20 top page ranks.
  //printf("Number of Iterations %d\n", iter_cnt);
  int maxprint = (order > 20 ? 20 : order);
  printf("---------------------------------------------\n");
  printf("Top %d page ranks: \n", maxprint);
  printf("---------------------------------------------\n");

  double* ranks = new double[order];
  int normalize = 1;     // normalization to a probability distribution is
                         // optional but it allows comparisons between results
                         // from different codes.

  int* ranksidx = new int[order];
  double scale = 0.;

  #pragma mta assert nodep
  for (int i = 0; i < order; i++)
  {
    if (i < 10) printf("rinfo[%d].rank: %f\n", i, rinfo[i].rank);

    ranks[i] = rinfo[i].rank;
    scale += ranks[i];
    ranksidx[i] = i;
  }

  quicksort<double> qs(ranks, order, ranksidx);
  qs.run();

  for (int i = order - maxprint; i < order; i++)
  {
    printf("%d %lf\n", ranksidx[i], (normalize ? ranks[i] / scale : ranks[i]));
  }

  printf("Page Rank Stats:\n");
  printf("      Max Value:  %g\n",
         (normalize ? ranks[order - 1] / scale : ranks[order - 1]));
  printf("      Min Value:  %g\n", (normalize ? ranks[0] / scale : ranks[0]));

  delete[] ranksidx;
  delete[] ranks;

#ifdef USING_QTHREADS
  qthread_finalize();
#endif

  return 0;
}

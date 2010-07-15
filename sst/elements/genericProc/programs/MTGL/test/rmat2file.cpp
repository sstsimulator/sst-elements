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
/*! \file rmat2file.cpp

    \author Jon Berry (jberry@sandia.gov)

    \date 12/20/2008
*/
/****************************************************************************/

#include <cstdlib>

#include <mtgl/static_graph_adapter.hpp>
#include <mtgl/generate_rmat_graph.hpp>
#include <mtgl/write_dimacs.hpp>
#include <mtgl/write_matrix_market.hpp>
#include <mtgl/mtgl_io.hpp>

using namespace mtgl;

int main(int argc, char* argv[])
{
  typedef static_graph_adapter<directedS> Graph;

  if (argc < 9)
  {
    fprintf(stderr, "usage: %s <scale> <deg> <a> <b> <c> <d> <file_root> "
            "<dimacs|matrixmarket|snapshot>\n", argv[0]);
    exit(1);
  }

  fprintf(stderr, "rmat2file...\n");

  char fname[256];
  char ftype[256];
  strcpy(fname, argv[7]);
  strcpy(ftype, argv[8]);

  int scale = atoi(argv[1]);            // 2^scale vertices
  int deg = atoi(argv[2]);              // ave degree <deg>
  double a = atof(argv[3]);
  double b = atof(argv[4]);
  double c = atof(argv[5]);
  double d = atof(argv[6]);

  srand48(0);

#ifdef USING_QTHREADS
  if (argc < 10)
  {
    fprintf(stderr, "usage: %s <scale> <deg> <a> <b> <c> <d> <file_root> "
            "<dimacs|matrixmarket|snapshot> <num shepherds>\n");
    exit(1);
  }

  int threads = atoi(argv[9]);
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
#endif

  Graph g;
  printf("Generating graph...\n");
  generate_rmat_graph(g, scale, deg, a, b, c, d);

  if (strcmp(ftype, "dimacs") == 0)
  {
    printf("Writing DIMACS file...\n");
    strcat(fname, ".dimacs");
    write_dimacs(g, fname);
  }
  else if (strcmp(ftype, "matrixmarket") == 0)
  {
    printf("Writing MTX file...\n");
    strcat(fname, ".mtx");
    write_matrix_market(g, fname);
  }
  else if (strcmp(ftype, "snapshot") == 0)
  {
    printf("Writing SNAPSHOT files...\n");
    char* srcs_f = fname;
    char dests_f[256];
    strcpy(dests_f, srcs_f);
    strcat(srcs_f, ".srcs");
    strcat(dests_f, ".dests");
    write_binary(g, srcs_f, dests_f);
  }

  return 0;
}

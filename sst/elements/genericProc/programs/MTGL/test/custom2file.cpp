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
/*! \file custom2file.cpp

    \brief Writes out a custom graph file format as described below.

    \author Jon Berry (jberry@sandia.gov)

    \date 3/19/2009

    Let the user define a custom graph in a file of 

    src dest
    src dest
    ...

    then snapshot that out to be read in as the other files are.
*/
/****************************************************************************/

#include <cstdlib>

#include <mtgl/static_graph_adapter.hpp>
#include <mtgl/write_dimacs.hpp>
#include <mtgl/write_matrix_market.hpp>
#include <mtgl/mtgl_io.hpp>

using namespace mtgl;

int main(int argc, char* argv[])
{
  typedef static_graph_adapter<directedS> Graph;

  if (argc < 3)
  {
    fprintf(stderr,
            "usage: %s <outfname> <dimacs|matrixmarket|snapshot> < "
            "<filename>\n", argv[0]);
    fprintf(stderr, "      were <filename> is a file of\n");
    fprintf(stderr, "      <src> <dest>\n");
    fprintf(stderr, "      <src> <dest>\n");
    fprintf(stderr, "      ..... ......\n");
    fprintf(stderr, "with no more than 100 edges\n");
    exit(1);
  }

  fprintf(stderr, "custom2file...\n");

  char fname[256];
  char ftype[256];
  strcpy(fname, argv[1]);
  strcpy(ftype, argv[2]);

  unsigned long n = 100, i = 0;
  unsigned long m = 100;
  unsigned long* srcs = (unsigned long*) malloc(m * sizeof(unsigned long));
  unsigned long* dests = (unsigned long*) malloc(m * sizeof(unsigned long));

  while (std::cin >> srcs[i] >> dests[i]) i++;

  m = i;
  int maxv = 0;

  for (int i = 0; i < m; ++i)
  {
    printf("%lu %lu\n", srcs[i], dests[i]);

    if (srcs[i] > maxv) maxv = srcs[i];
    if (dests[i] > maxv) maxv = dests[i];
  }

  n = maxv + 1;

  Graph g;
  init(n, m, srcs, dests, g);

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

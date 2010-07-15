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
/*! \file test_tri.cpp

    \author Jon Berry (jberry@sandia.gov)

    \date 1/19/2009
*/
/****************************************************************************/

#include <mtgl/util.hpp>
#include <mtgl/triangles.hpp>
#include <mtgl/metrics.hpp>
#include <mtgl/generate_rmat_graph.hpp>
#include <mtgl/read_dimacs.hpp>
#include <mtgl/read_matrix_market.hpp>
#include <mtgl/static_graph_adapter.hpp>
#include <mtgl/bfs.hpp>
#include <mtgl/connected_components.hpp>
#include <mtgl/mtgl_io.hpp>
#include <mtgl/dynamic_array.hpp>

using namespace std;
using namespace mtgl;

template <typename graph>
void test_find_triangles(graph& g)
{
  typedef typename graph_traits<graph>::size_type size_type;
  size_type count = 0;

  dynamic_array<triple<size_type, size_type, size_type> > result;
  count_triangles<graph> ctv(g, result, count);
  find_triangles<graph, count_triangles<graph> > ft(g, ctv);

  ft.run();

  fprintf(stdout, "found %d triangles\n", (int) count);
  fprintf(stderr, "RESULT: find_triangles %lu \n", count);
}

int main(int argc, char* argv[])
{
#ifndef __MTA__
  srand48(0);
#endif

  typedef static_graph_adapter<undirectedS> Graph;
  typedef graph_traits<Graph>::vertex_descriptor vertex_descriptor;
  typedef graph_traits<Graph>::size_type size_type;

  if (argc < 2)
  {
    fprintf(stderr, "Usage: %s {<p>|<filename>} <src>", argv[0]);
    fprintf(stderr, "where specifying p requests a "
            "generated r-mat graph "
            "with 2^p vertices,\n");
    fprintf(stderr, "and specifying filename requests that a "
            "DIMACS or MatrixMarket "
            "file be read to build the graph;\n");
    fprintf(stderr, "src is the index of the bfs source\n");
    fprintf(stderr, "DIMACS files must end with suffix .dimacs\n");
    fprintf(stderr, "MatrixMarket files must end with "
            "suffix .mtx\n");

    exit(1);
  }

#ifdef USING_QTHREADS
  if (argc < 4)
  {
    fprintf(stderr, "usage: %s {<p>|<filename>} <src> <threads>\n", argv[0]);
    fprintf(stderr, "(for 2^p vertex r-mat)\n");
    exit(1);
  }

  int threads = atoi(argv[3]);
  printf("qthread_init(%d)\n", (int)threads);
  fflush(stdout);
  qthread_init(threads);
  printf("qthread_init done(%d)\n", (int)threads);
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

  Graph gr;

  if (use_rmat)
  {
    printf("Generating rmat %s\n", argv[1]);
    mt_timer rmat_time;
    rmat_time.start();
    generate_rmat_graph(gr, atoi(argv[1]), 8);
    rmat_time.stop();
    printf("rmat time: %f\n", rmat_time.getElapsedSeconds());
  }
  else if (argv[1][llen - 1] == 'x')
  {
    // Matrix-market input.
    dynamic_array<int> weights;

    read_matrix_market(gr, argv[1], weights);
  }
  else if (strcmp(&argv[1][llen - 4], "srcs") == 0)
  {
    // Snapshot input: <file>.srcs, <file>.dests
    char* srcs_fname = argv[1];
    char dests_fname[256];

    strcpy(dests_fname, srcs_fname);
    strcpy(&dests_fname[llen - 4], "dests");

    read_binary(gr, srcs_fname, dests_fname);
  }
  else if (strcmp(argv[1], "custom") == 0)
  {
    size_type num_verts = 5;
    size_type num_edges = 7;

    size_type* srcs = (size_type*) malloc(sizeof(size_type) * num_edges);
    size_type* dests = (size_type*) malloc(sizeof(size_type) * num_edges);

    srcs[0] = 0; dests[0] = 1;
    srcs[1] = 1; dests[1] = 2;
    srcs[2] = 2; dests[2] = 0;
    srcs[3] = 2; dests[3] = 3;
    srcs[4] = 3; dests[4] = 4;
    srcs[5] = 4; dests[5] = 2;
    srcs[6] = 0; dests[6] = 3;

    init(num_verts, num_edges, srcs, dests, gr);
  }
  else if (argv[1][llen - 1] == 's')
  {
    // DIMACS input.
    dynamic_array<int> weights;

    read_dimacs(gr, argv[1], weights);
  }
  else
  {
    fprintf(stderr, "Invalid filename %s\n", argv[1]);
  }

  printf("Graph done\n");
//  print(gr);
  size_type order = num_vertices(gr);
  size_type size  = num_edges(gr);
  vertex_id_map<Graph> vid_map = get(_vertex_id_map, gr);
  vertex_descriptor* result =
    (vertex_descriptor*) malloc(sizeof(vertex_descriptor) * order);
  size_type nc = 0;
  printf("Starting test on %d vertices, %d edges\n", order, size);
  mt_timer tri_time;
  tri_time.start();
  test_find_triangles<Graph>(gr);
  tri_time.stop();
  printf("tri time: %f\n", tri_time.getElapsedSeconds());

  return 0;
}

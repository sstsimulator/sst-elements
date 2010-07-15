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
/*! \file test_cc.cpp

    \brief Tests the connected component algorithm.

    \author Jon Berry (jberry@sandia.gov)

    \date 3/3/2009
*/
/****************************************************************************/

#include <mtgl/util.hpp>
#include <mtgl/metrics.hpp>
#include <mtgl/read_dimacs.hpp>
#include <mtgl/read_matrix_market.hpp>
#include <mtgl/generate_rmat_graph.hpp>
#include <mtgl/static_graph_adapter.hpp>
#include <mtgl/bfs.hpp>
#include <mtgl/connected_components.hpp>
#include <mtgl/mtgl_io.hpp>

using namespace std;
using namespace mtgl;

template <class Graph>
class dummy_bfs_vis {
public:
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;

  dummy_bfs_vis(unsigned int& nv) : num_visited(nv) {}
  dummy_bfs_vis(const dummy_bfs_vis& d) : num_visited(d.num_visited) {}

  dummy_bfs_vis& operator=(const dummy_bfs_vis& d)
  {
    num_visited = d.num_visited;
    return *this;
  }

  void operator()(vertex_descriptor u, vertex_descriptor v, Graph &g)
  {
#ifdef DEBUG
    mt_incr(num_visited, 1);
#endif
  }

private:
  unsigned int& num_visited;
};

int main(int argc, char* argv[])
{
  char * nargv[5];
#ifndef __MTA__
  srand48(0);
#endif

  // The connected components algorithm used here is INVALID over
  // non-undirected graphs.
  typedef static_graph_adapter<undirectedS> Graph;
  typedef graph_traits<Graph>::vertex_descriptor vertex_descriptor;
  typedef graph_traits<Graph>::size_type size_type;
  typedef graph_traits<Graph>::vertex_iterator vertex_iterator;

  if (argc < 2)
  {
#if 1
      argc = 3;
      nargv[0] = argv[0];
      nargv[1] = "5"; /* p */
      nargv[2] = "4"; /* threads */
      nargv[3] = NULL;
      argv = nargv;
#else
    fprintf(stderr, "Usage: %s {<p>|<filename>}", argv[0]);
    fprintf(stderr, "where specifying p requests a generated r-mat graph "
                    "with 2^p vertices,\n");
    fprintf(stderr, "and specifying filename requests that a DIMACS or "
                    "MatrixMarket file be read to build the graph;\n");
    fprintf(stderr, "DIMACS files must end with suffix .dimacs\n");
    fprintf(stderr, "MatrixMarket files must end with suffix .mtx\n");

    exit(1);
#endif
  }

#ifdef USING_QTHREADS
  int threads;
  if (argc < 3)
  {
      qthread_initialize();
      threads = qthread_num_shepherds();
  } else {
      threads = atoi(argv[2]);
      printf("qthread_init(%d)\n", threads);
      fflush(stdout);
      qthread_init(threads);
  }
  printf("qthread_init done(%d)\n", threads);
  fflush(stdout);

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

  size_type order = num_vertices(ga);
  if (order < 100) print(ga);
  vertex_id_map<Graph> vid_map = get(_vertex_id_map, ga);

  vertex_descriptor* result = new vertex_descriptor[order];
  int nc = 0;
  mt_timer cc_time;
  typedef array_property_map<vertex_descriptor, vertex_id_map<Graph> >
  component_map;
  init_identity_vert_array(ga, result);
  component_map components(result, get(_vertex_id_map, ga));
  connected_components_gcc_sv<Graph, component_map> s1(ga, components);
  cc_time.start();
  s1.run();
  cc_time.stop();
  printf("gcc_sv time: %f\n", cc_time.getElapsedSeconds());
  count_connected_components<Graph, component_map> gcc(ga, components);
  nc = gcc.run();
  printf("there are %d connected components\n", nc);

  init_identity_vert_array(ga, result);
  connected_components_sv<Graph, component_map> s2(ga, components);
  cc_time.start();
  s2.run();
  cc_time.stop();
  printf("sv time (going through edges): %f\n", cc_time.getElapsedSeconds());
  count_connected_components<Graph, component_map> ccc(ga, components);
  nc = ccc.run();
  printf("there are %d connected components\n", nc);

  init_identity_vert_array(ga, result);
  connected_components_foo_sv<Graph, component_map> s3(ga, components);
  cc_time.start();
  s3.run();
  cc_time.stop();
  printf("sv time (going through adjs (non-qthread)): %f\n",
         cc_time.getElapsedSeconds());
  count_connected_components<Graph, component_map> ccc2(ga, components);
  nc = ccc2.run();
  printf("there are %d connected components\n", nc);

#ifdef DEBUG
  find_biggest_connected_component<Graph, component_map> fbcc(ga, components);
  unsigned long cs_id = fbcc.run();
  printf("the biggest has leader %d\n", cs_id);
  list<vertex_descriptor> the_component;
  vertex_iterator verts = vertices(ga);

  for (int i = 0; i < order; i++)
  {
    if (result[i] == verts[cs_id])
    {
      the_component.push_back(verts[i]);
    }
  }

  the_component.sort();
  the_component.unique();
  printf("comp.order: %d\n", the_component.size());
  //for (list<vertex_descriptor>::iterator it=the_component.begin();
  //     it != the_component.end(); it++) {
  //  printf("%d\n", get(vid_map,*it));
  //}
#endif

  delete[] result;

  return 0;
}

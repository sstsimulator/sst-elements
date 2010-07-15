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
/*! \file test_bfs.cpp

    \brief Tests the bfs code.

    \author Jon Berry (jberry@sandia.gov)

    \date 1/25/2008
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

template <class graph>
class dummy_bfs_vis {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;

  dummy_bfs_vis(unsigned int& nv) : num_visited(nv) {}
  dummy_bfs_vis(const dummy_bfs_vis& d) : num_visited(d.num_visited) {}

  dummy_bfs_vis& operator=(const dummy_bfs_vis& d)
  {
    num_visited = d.num_visited;
    return *this;
  }

  void operator()(typename graph_traits<graph>::vertex_descriptor u,
                  typename graph_traits<graph>::vertex_descriptor v,
                  graph &g)
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

  typedef static_graph_adapter<directedS> Graph;
  typedef graph_traits<Graph>::vertex_descriptor vertex_t;
  typedef graph_traits<Graph>::size_type size_type;
  typedef graph_traits<Graph>::vertex_iterator vertex_iterator_t;

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
  size_type size  = num_edges(ga);
  vertex_id_map<Graph> vid_map = get(_vertex_id_map, ga);

  //printf("degree(%d,g) = %d\n", 0, (int)out_degree(get_vertex(0, ga), (int)ga));
  printf("order(g) = %d\n", (int)order);
  printf("size(g) = %d\n", (int)size);

  unsigned long cs_id = 0;
  vertex_t* result = new vertex_t[order];
  typedef array_property_map<vertex_t, vertex_id_map<Graph> >
  component_map;
  init_identity_vert_array(ga, result);
  component_map components(result, get(_vertex_id_map, ga));
  connected_components_sv<Graph, component_map> s1(ga, components);
  s1.run();
  count_connected_components<Graph, component_map> ccc(ga, components);
  int nc = ccc.run();
  printf("there are %d connected components\n", nc);
  find_biggest_connected_component<Graph, component_map> fbcc(ga, components);
  cs_id = fbcc.run();
  printf("the biggest has leader %lu\n", cs_id);

#if 0
  list<vertex_t> the_component;
  for (int i = 0; i < order; i++)
  {
    if (result[i] == get_vertex(cs_id, ga))
    {
      the_component.push_back(get_vertex(i, ga));
    }
  }
  the_component.sort();
  the_component.unique();
  printf("comp.order: %d\n", the_component.size());
  //for (list<vertex_t>::iterator it=the_component.begin();
  //     it != the_component.end(); it++) {
  //  printf("%d\n", get(vid_map,*it));
  //}
#endif

  unsigned int num_visited = 0;
  dummy_bfs_vis<Graph> dbv(num_visited);
  size_type maxdeg = 0;
  vertex_iterator_t verts = vertices(ga);

  #pragma mta assert parallel
  for (size_t i = 0; i < order; i++)
  {
    size_type deg = out_degree(verts[i], ga);
    if (deg > maxdeg)
    {
      maxdeg = deg;
    }
  }

  graph_traits<Graph>::vertex_descriptor v = get_vertex(cs_id, ga);
  printf("searching from %lu (deg: %lu)\n", cs_id, out_degree(v, ga));
  size_type* parent = (size_type*) malloc(sizeof(size_type) * order);
  mt_timer timer;
  timer.start();
  int levels = bfs_chunked(ga, dbv, v, parent);
  //bfs(ga, dbv, v);
  timer.stop();
  printf("bfs secs: %f\n", timer.getElapsedSeconds());

  size_type visited = 0;
  for(size_type i = 0; i < order; i++) if (parent[i] != ULONG_MAX) visited++;
  printf("Levels        = %d\n\n", levels);
  printf("Visited nodes = %d\n\n", (int)visited);

  treeCheck(v, levels, ga, parent);

#ifdef DEBUG
  size_type largest_deg_remaining = 0;
  vertex_iterator_t verts = vertices(ga);
  for (int i = 0; i < order; i++)
  {
    vertex_t w = verts[i];
    size_type deg = out_degree(w, ga);
    if (result[w] != v && deg > largest_deg_remaining)
    {
      largest_deg_remaining = deg;
    }
  }
  printf("largest remaining degree: %d\n", largest_deg_remaining);
#endif

  return 0;
}

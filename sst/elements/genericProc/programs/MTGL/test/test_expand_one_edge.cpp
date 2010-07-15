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

#include <mtgl/util.hpp>
#include <mtgl/metrics.hpp>
#include <mtgl/read_dimacs.hpp>
#include <mtgl/read_matrix_market.hpp>
#include <mtgl/generate_rmat_graph.hpp>
#include <mtgl/static_graph_adapter.hpp>
#include <mtgl/expand_one_edge.hpp>
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
#ifndef __MTA__
  srand48(0);
#endif

  typedef static_graph_adapter<directedS> Graph;
  typedef graph_traits<Graph>::vertex_descriptor vertex_t;
  typedef graph_traits<Graph>::size_type size_type;
  typedef graph_traits<Graph>::vertex_iterator vertex_iterator_t;

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

  unsigned int num_visited = 0;
  dummy_bfs_vis<Graph> dbv(num_visited);
  vertex_iterator_t verts = vertices(ga);
  const size_type level_size = 2;

  // this is both the set of vertices to visit and the Q space for
  // their neighbors
  vertex_t *to_visit = (vertex_t*) malloc(sizeof(vertex_t)*order);

  #pragma mta assert parallel
  for (size_t i = 0; i < level_size; i++)
  {
       to_visit[i] = verts[i];
  }

  // The data for the property map (doesn't have to be an array in general)
  size_type* parent = (size_type*) malloc(sizeof(size_type) * order);
 #pragma mta assert nodep
  for (size_type i=0; i<order; i++) {
       parent[i] = std::numeric_limits<size_type>::max();
  }
  // make a property map.  This won't have to be an array map in general
  array_property_map<size_type, vertex_id_map<Graph> > colors = 
       make_array_property_map(parent, get(_vertex_id_map,ga));

  size_type head = 0;
  size_type tail = level_size;

  expand_one_edge(ga, dbv, to_visit, head, tail, colors);
  // head and tail now delimit the next level;

  printf("next level        = (%lu,%lu)\n\n", head, tail);

  return 0;
}

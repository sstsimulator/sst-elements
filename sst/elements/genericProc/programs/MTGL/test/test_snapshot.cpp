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
/*! \file test_snapshot.cpp

    \author Jon Berry (jberry@sandia.gov)

    \date 12/18/2008
*/
/****************************************************************************/

#include <cstdlib>
#include <cstring>

#include <mtgl/static_graph_adapter.hpp>
#include <mtgl/generate_rmat_graph.hpp>
#include <mtgl/mtgl_io.hpp>

using namespace mtgl;

typedef static_graph_adapter<undirectedS> Graph;

int edge_lists_differ(Graph& g, Graph& restored)
{
  graph_traits<Graph>::edge_iterator edgs = edges(g);
  int mg = num_edges(g);
  vertex_id_map<Graph> vid_mapg = get(_vertex_id_map, g);

  graph_traits<Graph>::edge_iterator edgs_er = edges(restored);
  int mr = num_edges(restored);
  vertex_id_map<Graph> vid_mapr = get(_vertex_id_map, restored);

  if (mg != mr)
  {
    printf("sizes differ %d != %d\n", mg, mr);
    return 1;
  }

#ifdef KDDKDD_WORKING_ONLY_IN_SERIAL
  int differ = 0;

  for (int i = 0; i < mr; i++)
  {
    graph_traits<Graph>::edge_descriptor eg = edgs[i];
    graph_traits<Graph>::vertex_descriptor srcg = source(eg, g);
    graph_traits<Graph>::vertex_descriptor trgg = target(eg, g);
    int sidg = get(vid_mapg, srcg);
    int tidg = get(vid_mapg, trgg);

    graph_traits<Graph>::edge_descriptor er = edgs_er[i];
    graph_traits<Graph>::vertex_descriptor srcr = source(er, restored);
    graph_traits<Graph>::vertex_descriptor trgr = target(er, restored);
    int sidr = get(vid_mapr, srcr);
    int tidr = get(vid_mapr, trgr);

    if (sidr != sidg || tidr != tidg) differ++;
  }

  if (differ)
  {
    printf("%d edges differ\n", differ);
    return 1;
  }
#endif

  return 0;
}

int main(int argc, char* argv[])
{
  if (argc < 3)
  {
    fprintf(stderr, "usage: %s <vtx exp> <srcfile> <destfile>\n", argv[0]);
    exit(1);
  }

  char sfname[256];
  char dfname[256];
  strcpy(sfname, argv[2]);
  strcpy(dfname, argv[3]);

  Graph g;
  generate_rmat_graph(g, atoi(argv[1]), 8);
  write_binary(g, sfname, dfname);

  printf("original order: %lu\n", num_vertices(g));
  printf("original size: %lu\n", num_edges(g));

  if (num_edges(g) <= 200) g.print();

  Graph restored;
  read_binary(restored, sfname, dfname);

  printf("restored order: %lu\n", num_vertices(restored));
  printf("restored size: %lu\n", num_edges(restored));

  if (num_edges(g) <= 200) restored.print();

  if (edge_lists_differ(g, restored))
  {
    printf("test_snapshot FAILED.\n");
  }
  else
  {
    printf("test_snapshot PASSED.\n");
  }
}

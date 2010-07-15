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

#include <mtgl/static_graph.hpp>

using namespace mtgl;

int main(int argc, char* argv[])
{
  unsigned long n;
  unsigned long m;

  // Read in the number of vertices and edges.
  std::cin >> n;
  std::cin >> m;

  unsigned long* srcs = new unsigned long[m];
  unsigned long* dests = new unsigned long[m];

  // Read in the ids of each edge's vertices.
  for (unsigned long i = 0; i < m; ++i)
  {
    std::cin >> srcs[i] >> dests[i];
  }

  // Initialize the graph.
  static_graph<directedS> g;
  g.init(n, m, srcs, dests);

  delete [] srcs;
  delete [] dests;

  // Print the graph.
  printf("Graph:\n");

  for (unsigned long u = 0; u < n; ++u)
  {
    unsigned long begin = g.index[u];
    unsigned long end = g.index[u+1];

    for (unsigned long j = begin; j < end; ++j)
    {
      unsigned long v = g.end_points[j];
      printf("(%lu, %lu)\n", u, v);
    }
  }

  printf("\n");

  // Find the sum of the ids of the adjacencies.
  unsigned long id_sum = 0;

  for (unsigned long u = 0; u < n; ++u)
  {
    unsigned long begin = g.index[u];
    unsigned long end = g.index[u+1];

    for (unsigned long j = begin; j < end; ++j) id_sum += g.end_points[j];
  }

  printf("sum of endpoint id's: %lu\n", id_sum);

  return 0;
}

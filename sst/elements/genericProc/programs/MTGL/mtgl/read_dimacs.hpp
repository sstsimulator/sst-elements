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
/*! \file read_dimacs.hpp

    \brief Parses a DIMACS graph file and creates an MTGL graph.

    \author Joe Crobak
    \author Eric Goodman (elgoodm@sandia.gov)
    \author Greg Mackey (gemacke@sandia.gov)

    \date 12/4/2007
*/
/****************************************************************************/

#ifndef MTGL_READ_DIMACS_HPP
#define MTGL_READ_DIMACS_HPP

#include <cstdio>
#include <cstdlib>

#include <mtgl/util.hpp>
#include <mtgl/mtgl_io.hpp>
#include <mtgl/merge_sort.hpp>
#include <mtgl/dynamic_array.hpp>

namespace mtgl {

/// Parses a DIMACS graph file and creates an MTGL graph.
template <typename graph_adapter, typename WT>
void read_dimacs(graph_adapter& g, char* filename, dynamic_array<WT>& weights)
{
  typedef typename graph_traits<graph_adapter>::size_type size_type;

#ifdef DEBUG
  mt_timer timer;
  mt_timer timer2;
  timer.start();
#endif

  int buflen;
  char* buf = read_array<char>(filename, buflen);

#ifdef DEBUG
  timer.stop();
  printf("File read time: %10.6f\n", timer.getElapsedSeconds());

  timer.start();
  timer2.start();
#endif

  // Find the problem line and get the problem size.
  // Problem line expected to be in the format of:
  //    p sp num_nodes num_edges

  // Find the starting position in buf for the problem line.
  int num_intro_lines = 1;
  int pls = 0;

  if (buf[0] != 'p')
  {
    for ( ; pls < buflen - 1 && !(buf[pls] == '\n' && buf[pls+1] == 'p');
         ++pls)
    {
      if (buf[pls] == '\n') ++num_intro_lines;
    }

    ++num_intro_lines;
    ++pls;
  }

  if (pls == buflen)
  {
    fprintf(stderr, "\nError: Did not find problem line.\n");
    exit(1);
  }

  // Find the length of the problem line and put a null character at the
  // end of the problem line.  This is necessary because sscanf expects it.
  int pll;
  for (pll = 0; pll < buflen - 1 - pls && buf[pll + pls] != '\n'; ++pll);
  buf[pll + pls] = '\0';

  size_type num_vertices;
  size_type num_edges;

  int ret = sscanf(&buf[pls], "%*c %*s %lu %lu", &num_vertices,
                   &num_edges);

  if (ret != 2)
  {
    fprintf(stderr, "\nERROR: Problem line misformatted (expected p FORMAT "
            "NODES EDGES).\n");
    exit(1);
  }

  // Put the '\n' back at the end of the problem line.  The remainder of the
  // code expects it.
  buf[pll + pls] = '\n';

  // Find the starting position for the first edge line.
  pls += pll + 1;
  if (buf[pls] != 'a')
  {
    for ( ; pls < buflen - 1 && !(buf[pls] == '\n' && buf[pls+1] == 'a');
         ++pls)
    {
      if (buf[pls] == '\n') ++num_intro_lines;
    }

    ++num_intro_lines;
    ++pls;
  }

#ifdef DEBUG
  timer2.stop();
  printf("Problem line find and read time: %10.6f\n",
         timer2.getElapsedSeconds());

  timer2.start();
#endif

  // Find start of each edge line.
  int num_lines = 0;
  int* start_positions = (int*) malloc(sizeof(int) * num_edges);

  #pragma mta assert nodep
  for (int i = pls - 1; i < buflen - 1; ++i)
  {
    if (buf[i] == '\n' && buf[i+1] == 'a')
    {
      int pos = mt_incr(num_lines, 1);
      start_positions[pos] = i + 1;
    }
  }

  if (num_lines != num_edges)
  {
    fprintf(stderr, "\nERROR: Number of edges in file doesn't match number "
            "from problem description\n       line.\n");
    exit(1);
  }

#ifdef DEBUG
  timer2.stop();
  printf("Start positions find time: %10.6f\n", timer2.getElapsedSeconds());
#endif

#ifdef __MTA__
#ifdef DEBUG
  timer2.start();
#endif

  merge_sort(start_positions, num_edges);

#ifdef DEBUG
  timer2.stop();
  printf("Start positions sort time: %10.6f\n", timer2.getElapsedSeconds());
#endif
#endif

#ifdef DEBUG
  timer2.start();
#endif

  // Allocate storage for the sources, destinations, and weights.
  size_type* edge_heads = (size_type*) malloc(sizeof(size_type) * num_edges);
  size_type* edge_tails = (size_type*) malloc(sizeof(size_type) * num_edges);
  weights.resize(num_edges);

  // Find all the edges.  Expected format:
  //   a SRC DST CAP
  // This will be the bulk of the lines in the graph and so is parallelized
  // where possible.  The computation of line starts is so that we can have
  // no dependencies between lines in this loop.  This, of course, implies
  // that we are being slightly inefficient for the serial case, but the
  // relative speed of the single previous pass through the buffer to find
  // the line starts appears very small for relatively large sized graphs
  // (~128MB files).

  #pragma mta assert nodep
  for (size_type i = 0; i < num_edges; ++i)
  {
    long from;
    long to;
    long weight;

    char* a = &buf[start_positions[i] + 1];
    char* b = NULL;

    from = strtol(a, &b, 10);
    to = strtol(b, &a, 10);
    weight = strtol(a, &b, 10);

    if (a == b)
    {
      fprintf(stderr, "\nError on line %i: Too few parameters when "
              "describing edge.\n", i + num_intro_lines + 1);
      exit(1);
    }

    // Dimacs vertex ids are 1-based.  We need them to be 0-based.
    --from;
    --to;

    if (from < 0 || static_cast<size_type>(from) >= num_vertices)
    {
      fprintf(stderr, "\nError on line %i: First vertex id is invalid.\n",
              i + num_intro_lines + 1);
      exit(1);
    }
    else if (to < 0 || static_cast<size_type>(to) >= num_vertices)
    {
      fprintf(stderr, "\nError on line %i: Second vertex id is invalid.\n",
              i + num_intro_lines + 1);
      exit(1);
    }
    else if (weight < 0)
    {
      fprintf(stderr, "\nError on line %i: Negative weight supplied.\n",
              i + num_intro_lines + 1);
      exit(1);
    }

    edge_heads[i] = static_cast<size_type>(from);
    edge_tails[i] = static_cast<size_type>(to);
    weights[i] = static_cast<WT>(weight);
  }

#ifdef DEBUG
  timer2.stop();
  printf("Edge read time: %10.6f\n", timer2.getElapsedSeconds());

  timer.stop();
  printf("File parse time: %10.6f\n", timer.getElapsedSeconds());

  timer.start();
#endif

  init(num_vertices, num_edges, edge_heads, edge_tails, g);

#ifdef DEBUG
  timer.stop();
  printf("Graph creation time: %10.6f\n", timer.getElapsedSeconds());
#endif

  free(start_positions);
  free(edge_heads);
  free(edge_tails);
  free(buf);
}

template <typename graph_adapter, typename WT>
void read_dimacs(graph_adapter& g, const char* filename,
                 dynamic_array<WT>& weights)
{
  return read_dimacs(g, const_cast<char*>(filename), weights);
}

}

#endif

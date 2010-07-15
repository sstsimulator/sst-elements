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
/*! \file write_dimacs.hpp

    \brief Writes a graph to a dimacs formatted file.

    \author Greg Mackey (gemacke@sandia.gov)

    \date 5/14/2010
*/
/****************************************************************************/

#ifndef MTGL_WRITE_DIMACS_HPP
#define MTGL_WRITE_DIMACS_HPP

#include <cstdio>
#include <vector>

#include <mtgl/mtgl_adapter.hpp>

namespace mtgl {

/// Writes a graph to a dimacs formatted file.
template <class graph_adapter>
void write_dimacs(graph_adapter& ga, char* fname)
{
  typedef typename graph_traits<graph_adapter>::vertex_descriptor
          vertex_descriptor;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_descriptor;
  typedef typename graph_traits<graph_adapter>::edge_iterator edge_iterator;
  typedef typename graph_traits<graph_adapter>::size_type size_type;

  vertex_id_map<graph_adapter> vipm = get(_vertex_id_map, ga);

  FILE* fp = fopen(fname, "w");
  if (!fp) return;

  fprintf(fp, "p sp %lu %lu\n", num_vertices(ga), num_edges(ga));

  size_type sz = num_edges(ga);
  edge_iterator edgs = edges(ga);

  for (size_type i = 0; i < sz; ++i)
  {
    edge_descriptor e = edgs[i];

    vertex_descriptor src = source(e, ga);
    vertex_descriptor trg = target(e, ga);

    int sid = get(vipm, src);
    int tid = get(vipm, trg);

    // Dimacs files are 1-based.
    fprintf(fp, "a %d %d 0\n", sid + 1, tid + 1);
  }

  printf("sz: %d\n", (int) sz);

  fclose(fp);
}

template <class graph_adapter>
void write_dimacs(graph_adapter& ga, const char* fname)
{
  write_dimacs(ga, const_cast<char*>(fname));
}

int comp_pair(const pair<int, int>& a, const pair<int, int>& b)
{
  if (a.first == b.first)
  {
    return a.second < b.second;
  }
  else
  {
    return a.first < b.first;
  }
}

/// Writes a graph to a dimacs formatted file where the edges are sorted.
template <class graph_adapter>
void write_dimacs_sorted(graph_adapter& ga, char* fname)
{
  typedef typename graph_traits<graph_adapter>::vertex_descriptor
                   vertex_descriptor;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_descriptor;
  typedef typename graph_traits<graph_adapter>::edge_iterator edge_iterator;
  typedef typename graph_traits<graph_adapter>::size_type size_type;

  vertex_id_map<graph_adapter> vipm = get(_vertex_id_map, ga);

  FILE* fp = fopen(fname, "w");
  if (!fp) return;

  fprintf(fp, "p sp %d %d\n", (int)num_vertices(ga), (int)num_edges(ga));

  std::vector<pair<int, int> > output;

  size_type sz = num_edges(ga);
  edge_iterator edgs = edges(ga);

  for (size_type i = 0; i < sz; ++i)
  {
    edge_descriptor e = edgs[i];

    vertex_descriptor src = source(e, ga);
    vertex_descriptor trg = target(e, ga);

    int sid = get(vipm, src);
    int tid = get(vipm, trg);

    // Dimacs files are 1-based for some reason.
    output.push_back(pair<int, int>(sid + 1, tid + 1));
  }

  std::sort(output.begin(), output.end(), comp_pair);

  for (int i = 0; i < output.size(); i++)
  {
    pair<int, int> p = output[i];
    fprintf(fp, "a %d %d 0\n", p.first, p.second);
  }

  printf("sz : %d\n", (int) sz);
  printf("osz : %lu\n", (long unsigned) output.size());

  fclose(fp);
}

template <class graph_adapter>
void write_dimacs_sorted(graph_adapter& ga, const char* fname)
{
  write_dimacs_sorted(ga, const_cast<char*>(fname));
}

}

#endif

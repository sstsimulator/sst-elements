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
/*! \file write_matrix_market.hpp

    \brief Writes a graph to a matrix-market formatted file.

    \author Greg Mackey (gemacke@sandia.gov)

    \date 5/14/2010
*/
/****************************************************************************/

#ifndef MTGL_WRITE_MATRIX_MARKET_HPP
#define MTGL_WRITE_MATRIX_MARKET_HPP

#include <cstdio>

#include <mtgl/mtgl_adapter.hpp>

namespace mtgl {

/// Writes a graph to a matrix-market formatted file.
template <class graph_adapter>
void write_matrix_market(graph_adapter& ga, char* fname)
{
  typedef typename graph_traits<graph_adapter>::size_type size_type;
  typedef typename graph_traits<graph_adapter>::vertex_iterator vertex_iterator;
  typedef typename graph_traits<graph_adapter>::vertex_descriptor
          vertex_descriptor;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_descriptor;
  typedef typename graph_traits<graph_adapter>::out_edge_iterator
          out_edge_iterator;

  vertex_id_map<graph_adapter> vipm = get(_vertex_id_map, ga);
  edge_id_map<graph_adapter> eipm = get(_edge_id_map, ga);

  FILE* fp = fopen(fname, "w");
  if (!fp) return;

  fprintf(fp, "%%%%MatrixMarket matrix coordinate pattern general\n");

  size_type order = num_vertices(ga);
  size_type size = num_edges(ga);

  fprintf(fp, "%d %d %d\n", order, order, size);

  vertex_iterator verts = vertices(ga);
  for (size_type i = 0; i < order; ++i)
  {
    vertex_descriptor src = verts[i];
    int sid = get(vipm, src);

    out_edge_iterator inc_edgs = out_edges(src, ga);
    size_type out_deg = out_degree(src, ga);
    for (size_type j = 0; j < out_deg; ++j)
    {
      edge_descriptor e = inc_edgs[j];

      vertex_descriptor trg = target(e, ga);

      int tid = get(vipm, trg);

      float prob = 1. / out_deg;

      // Matrix-market files are one-based.
      fprintf(fp, "%d %d %f\n", sid + 1, tid + 1, prob);
    }
  }

  fclose(fp);
}

template <class graph_adapter>
void write_matrix_market(graph_adapter& ga, const char* fname)
{
  write_matrix_market(ga, const_cast<char*>(fname));
}

}

#endif

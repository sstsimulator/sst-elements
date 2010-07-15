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
/*! \file write_binary.hpp

    \brief Writes a graph to the srcs / dests format.

    \author Greg Mackey (gemacke@sandia.gov)
    \author Jon Berry (jberry@sandia.gov)

    \date 5/14/2010
*/
/****************************************************************************/

#ifndef MTGL_WRITE_BINARY_HPP
#define MTGL_WRITE_BINARY_HPP

#include <cstdlib>
#include <cstdio>

#include <mtgl/snap_util.h>
#include <mtgl/mtgl_adapter.hpp>

namespace mtgl {

template <typename graph_adapter>
bool write_binary(graph_adapter& g, char* src_filename, char* dest_filename)
{
  typedef typename graph_traits<graph_adapter>::size_type size_type;
  typedef typename graph_traits<graph_adapter>::edge_iterator edge_iterator;
  typedef typename graph_traits<graph_adapter>::edge_descriptor
          edge_descriptor;

  size_type m = num_edges(g);
  vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, g);

  size_type* tmpbuf = (size_type*) malloc(m * sizeof(size_type));

  if (snap_init() != SNAP_ERR_OK)
  {
    perror("Can't initialize libsnapshot.\n");
    return false;
  }

  edge_iterator eIter = edges(g);
  #pragma mta assert nodep
  for (size_type i = 0; i < m; ++i)
  {
    edge_descriptor e = eIter[i];
    tmpbuf[i] = get(vid_map, source(e, g));
  }

  if (snap_snapshot(src_filename, tmpbuf,
                    m * sizeof(size_type), NULL) != SNAP_ERR_OK)
  {
    return false;
  }

  #pragma mta assert nodep
  for (size_type i = 0; i < m; ++i)
  {
    edge_descriptor e = eIter[i];
    tmpbuf[i] = get(vid_map, target(e, g));
  }

  if (snap_snapshot(dest_filename, tmpbuf,
                    m * sizeof(size_type), NULL) != SNAP_ERR_OK)
  {
    return false;
  }

  free(tmpbuf);

  return true;
}

template <typename graph_adapter>
bool write_binary(graph_adapter& g, const char* src_filename,
                  const char* dest_filename)
{
  return write_binary(g, const_cast<char*>(src_filename),
                      const_cast<char*>(dest_filename));
}

template <typename graph_adapter, typename int_type>
bool write_binary(graph_adapter& g, char* src_filename, char* dest_filename,
                  int_type val)
{
  typedef typename graph_traits<graph_adapter>::size_type size_type;
  typedef typename graph_traits<graph_adapter>::edge_iterator edge_iterator;
  typedef typename graph_traits<graph_adapter>::edge_descriptor
          edge_descriptor;

  size_type m = num_edges(g);
  vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, g);

  int_type* tmpbuf = (int_type*) malloc(m * sizeof(int_type));

  if (snap_init() != SNAP_ERR_OK)
  {
    perror("Can't initialize libsnapshot.\n");
    return false;
  }

  edge_iterator eIter = edges(g);
  #pragma mta assert nodep
  for (size_type i = 0; i < m; ++i)
  {
    edge_descriptor e = eIter[i];
    tmpbuf[i] = static_cast<int_type>(get(vid_map, source(e, g)));
  }

  if (snap_snapshot(src_filename, tmpbuf,
                    m * sizeof(int_type), NULL) != SNAP_ERR_OK)
  {
    return false;
  }

  #pragma mta assert nodep
  for (size_type i = 0; i < m; ++i)
  {
    edge_descriptor e = eIter[i];
    tmpbuf[i] = static_cast<int_type>(get(vid_map, target(e, g)));
  }

  if (snap_snapshot(dest_filename, tmpbuf,
                    m * sizeof(int_type), NULL) != SNAP_ERR_OK)
  {
    return false;
  }

  free(tmpbuf);

  return true;
}

template <typename graph_adapter, typename int_type>
bool write_binary(graph_adapter& g, const char* src_filename,
                  const char* dest_filename, int_type val)
{
  return write_binary(g, const_cast<char*>(src_filename),
                      const_cast<char*>(dest_filename), val);
}

}

#endif

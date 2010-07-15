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
/*! \file induced_subgraph.hpp

    \brief Routines for extracting subgraphs -- THESE DON'T YET WORK WITH
           ADAPTERS (2007).

    \author Joseph Crobak
    \author Jon Berry (jberry@sandia.gov)

    \date 1/6/2005
*/
/****************************************************************************/

#ifndef MTGL_INDUCED_SUBGRAPH_HPP
#define MTGL_INDUCED_SUBGRAPH_HPP

#include <mtgl/util.hpp>
#include <mtgl/psearch.hpp>

namespace mtgl {

template <typename graph>
class vertex_typing_visitor : public default_psearch_visitor<graph> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  vertex_typing_visitor(bool* emask, bool* vmask, int* oldToNewVID,
                        bool* bignd, int* handle_v,
                        vertex_id_map<graph>& vimap, graph& _g) :
    edge_mask(emask), vert_mask(vmask), oldToNewVertID(oldToNewVID),
    bigend(bignd), srcid(-1), handle_vertex(handle_v), vid_map(vimap), g(_g) {}

  void pre_visit(vertex src)
  {
    int sid = get(src, vid_map);
    srcid = src->get_id();
  }

  bool tree_edge(edge e, vertex src)
  {
    if (edge_mask[e->get_id()])
    {
      if (bigend && bigend[e->get_id()])
      {
        vertex dest = other(e, src, g);
        int destid = dest->get_id();
        vert_mask[destid] = true;
        int ticket = mt_incr(handle_vertex[srcid], 1);

        if (ticket == 0)
        {
          vert_mask[srcid] = true;
        }
      }
      else
      {
        vert_mask[srcid] = true;
        vertex dest = other(e, src, g);
        int destid = dest->get_id();
        int ticket = mt_incr(handle_vertex[dest->get_id()], 1);

        if (ticket == 0)
        {
          vert_mask[destid] = true;
        }
      }
    }

    return false;
  }

  bool back_edge(edge e, vertex src)
  {
    if (edge_mask[e->get_id()])
    {
      if (bigend && bigend[e->get_id()])
      {
        vertex dest = other(e, src, g);
        int destid = dest->get_id();
        vert_mask[destid] = true;
        int ticket = mt_incr(handle_vertex[srcid], 1);

        if (ticket == 0)
        {
          vert_mask[srcid] = true;
        }
      }
      else
      {
        vert_mask[srcid] = true;
        vertex dest = other(e, src, g);
        int destid = dest->get_id();
        int ticket = mt_incr(handle_vertex[destid], 1);

        if (ticket == 0)
        {
          vert_mask[destid] = true;
        }
      }
    }

    return false;
  }

protected:
  vertex_id_map<graph>& vid_map;
  bool* edge_mask;
  bool* vert_mask;
  int* oldToNewVertID;
  bool* bigend;
  int* handle_vertex;
  int srcid;
  graph& g;
};

#pragma mta debug level 0

template <typename T>
void prefixSumsFromMask(bool* mask, T* result, int size)
{
  result[0] = mask[0];
  for (int i = 1; i < size; i++) result[i] = result[i - 1] + (mask[i] == true);
}

#pragma mta debug level default

template <class graph, class T>
graph* induced_subgraph(graph* g, bool* edge_mask,
                        T*& newToOldVertID, T*& eid_map)
{
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;
  typedef typename graph_traits<graph>::edge_iterator edge_iterator;

  graph* newgraph = new graph();
  unsigned int orig_order = num_vertices(*g);
  int orig_size = num_edges(*g);

  bool* vert_mask      = (bool*) calloc(orig_order, sizeof(bool));
  int* verts_maskPSum  = (int*) calloc(orig_order, sizeof(int));
  int* oldToNewVertID  = (int*) calloc(orig_order, sizeof(int));
  int* edge_maskPSum   = (int*) calloc(orig_size, sizeof(int));
  int* handle_vert     = (int*) calloc(orig_order, sizeof(int));
  int newNumVerts;
  int newNumedges;
  vertex** sources;
  vertex** dests;

  newToOldVertID = (T*) calloc(orig_order, sizeof(T));

  vertex_id_map<graph> vidmap = get(_vertex_id_map, g);

  vertex_typing_visitor<graph> tv(edge_mask, vert_mask, oldToNewVertID,
                                  g->get_bigend(), handle_vert, vidmap, *g);

  //psearchHighLowDirected(g, tv);
  psearch_high_low<graph, vertex_typing_visitor<graph>, NO_FILTER, DIRECTED>
    psrch(g, tv);
  psrch.run();

  prefixSumsFromMask(vert_mask, verts_maskPSum, orig_order);

  #pragma mta assert nodep
  for (int j = 0; j < orig_order; j++)
  {
    if (vert_mask[j] == true)
    {
      oldToNewVertID[j] = 0;
      break;
    }
  }

  #pragma mta assert parallel
  for (int i = 1; i < orig_order; i++)
  {
    if (verts_maskPSum[i - 1] != verts_maskPSum[i])
    {
      oldToNewVertID[i] = verts_maskPSum[i - 1];
      newToOldVertID[verts_maskPSum[i - 1]] = i;
    }
    else
    {
      oldToNewVertID[i] = -1;
    }
  }

  newNumVerts = verts_maskPSum[orig_order - 1];

  if (newNumVerts > 0)
  {
    vertex** newgraphVerts = newgraph->addVertices(0, newNumVerts - 1,
                                                   BUILD_STATE);

    #pragma mta assert nodep
    for (int i = 0; i < newNumVerts; i++) newgraphVerts[i]->get_id() = i;

    // End Add Vertices

    //     Add edges

    prefixSumsFromMask(edge_mask, edge_maskPSum, orig_size);

    newNumedges = edge_maskPSum[orig_size - 1];

    if (newNumedges > 0)
    {
      sources = (vertex**) malloc(sizeof(vertex*) * newNumedges);
      dests   = (vertex**) malloc(sizeof(vertex*) * newNumedges);
      int* ids = (int*)  malloc(sizeof(int) * newNumedges);

      edge_iterator edgs = edges(*g);

      #pragma mta assert parallel
      for (int i = 0; i < orig_size; i++)
      {
        if (edge_mask[i] == true)
        {
          int newVert1 = oldToNewVertID[edgs[i]->get_vert1()->get_id()];
          int newVert2 = oldToNewVertID[edgs[i]->get_vert2()->get_id()];

          // if id is -1 then vert was removed
          if (newVert1 >= 0 && newVert2 >= 0)
          {
            int index = edge_maskPSum[i] - 1;
            sources[index] = newgraphVerts[newVert1];
            dests[index]   = newgraphVerts[newVert2];
            ids[index]     = edgs[i]->get_id();
          }
        }
      }

      edge** newgraphedges = newgraph->addedges(sources, dests, newNumedges);

      eid_map = (T*) malloc(sizeof(T) * newNumedges);

      #pragma mta assert nodep
      for (int i = 0; i < newNumedges; i++)
      {
        newgraphedges[i]->get_id() = i;
        eid_map[i] = ids[i];
      }

      free(sources);
      free(dests);
      free(ids);
    }

    free(newgraphVerts);
  }
  // End Add edges

  free(vert_mask);
  free(verts_maskPSum);
  free(oldToNewVertID);
  free(edge_maskPSum);

  return newgraph;
}

// subgraph_generator template constants.
const int EMASK = 1;
const int VMASK = 2;

/*!  \brief Class to generate a subgraph, given either a vertex mask or an
            edge mask.

     Template parameter can be either EMASK or VMASK to specify that the
     _mask argument given to the constructor is an edge-mask or a vertex-mask,
     respectively.
 */
template<typename graph, int masktype = EMASK>
class subgraph_generator {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;
  typedef typename graph_traits<graph>::edge_iterator edge_iterator;
  typedef typename graph_traits<graph>::size_type size_type;

  /*! \brief Constructor.
      \param _subg Subgraph pointer reference containing the
                   returned induced subgraph.
      \param _g    Pointer to the graph from which the subgraph will
                   be extracted.
      \param _mask Boolean mask array.  Contains either an edge
                   mask or a vertex mask, depending on the template
                   parameter.
      \param _svid_to_gvid subgraph vid to graph vid map.
                           Allocated within routine.
  */
  subgraph_generator(graph*& _subg, graph* _g, bool* _mask,
                     int*& _svid_to_gvid) :
    subg(_subg), g(_g), mask(_mask), svid_to_gvid(_svid_to_gvid)
  {
    assert(g != NULL);
    assert(mask != NULL);
    vmask = NULL;
    emask = NULL;
    eid_map = NULL;

    switch(masktype)
    {
      case VMASK: vmask = mask; break;
      case EMASK: emask = mask; break;
      default:
        fprintf(stderr, "ERROR: invalid masktype given"
                " to subgraph_generator\n");
        exit(1);
    };
  }

  // ---------------------------------------------
  ~subgraph_generator()
  {
    if (eid_map != NULL) free(eid_map);
    eid_map = NULL;
  }

  /// Runs the subgraph generator routine.
  void run()
  {
    int num_edges_marked = 0;
    size_type nEdges = num_edges(*g);

    if (masktype == VMASK)
    {
      emask = (bool*) malloc(sizeof(bool) * nEdges);
      generate_emask();

      /* Count the # of edges to include in the induced subg. */
      /* for now this is a little kludgy.  We want to ensure that an induced
       * subgraph will contain all degree-zero vertices as well... currently
       * they are ignored since induced subgraph works based on an edge-mask
       * For the sake of the connection-subgraphs routine, we can check to
       * see if the emask generated has no labeled edges... if so, add the
       * vertices marked in vmask.  In the end, we need a true vertex-mask
       * based induced subgraph generator.
       */
      int num_edges_marked = 0;
      #pragma mta assert parallel
      for (int ei = 0; ei < nEdges; ei++)
      {
        if (emask[ei]) num_edges_marked += 1;
      }
      /* TODO: for now */
    }
    subg = induced_subgraph(g, emask, svid_to_gvid, eid_map);
    if (masktype == VMASK)
    {
      free(emask);
      emask = NULL;
    }
  }

protected:
  // Extracts an edge-mask given a mask of vertices.
  // Adds an edge if both endpoints are in the vertex-mask
  #pragma mta inline
  void generate_emask()
  {
    size_type nEdges = num_edges(*g);
    edge_iterator edgs = edges(*g);
    vertex_id_map<graph> vid_map = get(_vertex_id_map, *g);

    #pragma mta assert parallel
    for (int ei = 0; ei < nEdges; ei++)
    {
      emask[ei] = false;
      edge eptr = edgs[ei];
      vertex v1 = source(eptr, *g);
      vertex v2 = target(eptr, *g);

      if (vmask[get(vid_map, v1)] && vmask[get(vid_map, v2)]) emask[ei] = true;
    }
  }

private:
  graph*& subg;
  graph*  g;
  bool*   mask;
  bool*   vmask;
  bool*   emask;
  int*&   svid_to_gvid;
  int*    eid_map;
};    /* end: class induced_subgraph */

}

#endif

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

#include <mtgl/subgraph_isomorphism.hpp>
#include <mtgl/static_graph_adapter.hpp>

using namespace mtgl;

template <typename Graph, typename vertex_property_map,
          typename edge_property_map>
void print_my_graph(Graph& g, vertex_property_map& vTypes,
                    edge_property_map& eTypes)
{
  typedef typename graph_traits<Graph>::size_type size_type;
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;
  typedef typename graph_traits<Graph>::edge_descriptor edge_descriptor;
  typedef typename graph_traits<Graph>::edge_iterator edge_iterator;

  vertex_id_map<Graph> vid_map = get(_vertex_id_map, g);
  edge_id_map<Graph> eid_map = get(_edge_id_map, g);

  edge_iterator edgs = edges(g);
  size_type size = num_edges(g);
  for (size_type i = 0; i < size; i++)
  {
    edge_descriptor e = edgs[i];
    size_type eid = get(eid_map, e);

    vertex_descriptor u = source(e, g);
    size_type uid = get(vid_map, u);

    vertex_descriptor v = target(e, g);
    size_type vid = get(vid_map, v);

    printf("%lu: (%lu, %lu)   [%lu, %lu, %lu]\n", eid, uid, vid, vTypes[u],
           eTypes[e], vTypes[v]);
  }
}

int main(int argc, char *argv[])
{
  typedef static_graph_adapter<directedS> Graph;
  typedef graph_traits<Graph>::size_type size_type;

  // Initialize the original graph.
  Graph g;

  const size_type numVerts = 6;
  const size_type numEdges = 7;

  size_type sources[numEdges] = { 1, 2, 1, 2, 3, 3, 4 };
  size_type targets[numEdges] = { 0, 0, 2, 3, 4, 5, 5 };

  init(numVerts, numEdges, sources, targets, g);

  // Create the big graph vertex and edge type property maps.
  typedef array_property_map<int, vertex_id_map<Graph> > vertex_property_map;
  typedef array_property_map<int, edge_id_map<Graph> > edge_property_map;

  int vTypes[numVerts] = { 1, 2, 3, 2, 3, 1 };
  int eTypes[numEdges] = { 1, 2, 3, 3, 3, 1, 2 };

  vertex_id_map<Graph> g_vid_map = get(_vertex_id_map, g);
  edge_id_map<Graph> g_eid_map = get(_edge_id_map, g);
  vertex_property_map vTypemap(vTypes, g_vid_map);
  edge_property_map eTypemap(eTypes, g_eid_map);

  // Print the original graph with its vertex and edge types.
  printf("Big graph (%d, %d)\n", num_vertices(g), num_edges(g));
  print_my_graph(g, vTypemap, eTypemap);

  // Initialize target graph.
  Graph target;

  const size_type numVertsTarget = 3;
  const size_type numEdgesTarget = 3;

  size_type sourcesTarget[numEdgesTarget] = { 1, 2, 1 };
  size_type targetsTarget[numEdgesTarget] = { 0, 0, 2 };

  init(numVertsTarget, numEdgesTarget, sourcesTarget, targetsTarget, target);

  // Create the target graph vertex and edge type property maps.
  int vTypesTarget[numVertsTarget] = { 1, 2, 3 };
  int eTypesTarget[numEdgesTarget]  = { 1, 2, 3 };

  vertex_id_map<Graph> trg_vid_map = get(_vertex_id_map, target);
  edge_id_map<Graph> trg_eid_map = get(_edge_id_map, target);
  vertex_property_map vTypemapTarget(vTypesTarget, trg_vid_map);
  edge_property_map eTypemapTarget(eTypesTarget, trg_eid_map);

  // Print the target graph with its vertex and edge types.
  printf("\nTarget graph (%d, %d)\n", numVertsTarget, numEdgesTarget);
  print_my_graph(target, vTypemapTarget, eTypemapTarget);

  // Set up walk through target graph.
  const size_type walk_length = 7;
  size_type walk_ids[walk_length] = { 0, 0, 1, 2, 2, 1, 0 };

  // Declare the results array and the comparator and run subgraph
  // isomorphism.
  size_type* result = new size_type[numEdges];

  si_default_comparator<Graph, vertex_property_map, edge_property_map>
    si_comp(vTypemap, eTypemap, vTypemapTarget, eTypemapTarget);

  subgraph_isomorphism(g, target, walk_ids, walk_length, result, si_comp);

  // Print the results.
  printf("\nResult\n");
  for (size_type i = 0; i < numEdges; ++i) printf("%lu: %lu\n", i, result[i]);

  delete [] result;

  return 0;
}

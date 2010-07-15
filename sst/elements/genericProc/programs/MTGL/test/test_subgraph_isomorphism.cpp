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
/*! \file test_subgraph_isomorphism.cpp

    \brief This is a driver to test the subgraph isomorphism code.

    \author Greg Mackey (gemacke@sandia.gov)

    \date 3/17/2008
*/
/****************************************************************************/

#include <mtgl/subgraph_isomorphism.hpp>
#include <mtgl/duplicate_adapter.hpp>
#include <mtgl/random_walk.hpp>
#include <mtgl/generate_subgraph.hpp>
#include <mtgl/filter_graph.hpp>

//#include <mtgl/adjacency_list_adapter.hpp>
//#include <mtgl/graph_adapter.hpp>
#include <mtgl/static_graph_adapter.hpp>

using namespace mtgl;

//#define FILTERED
//#define RANDWALK
//#define TEST1

/***/

int main(int argc, char *argv[])
{
  typedef directedS DIRECTION;
//  typedef adjacency_list_adapter<DIRECTION> Graph;
//  typedef graph_adapter<DIRECTION> Graph;
  typedef static_graph_adapter<DIRECTION> Graph;

  typedef graph_traits<Graph>::vertex_descriptor vertex_descriptor;
  typedef graph_traits<Graph>::edge_descriptor edge;
  typedef graph_traits<Graph>::size_type size_type;
  typedef array_property_map<int, vertex_id_map<Graph> > vertex_property_map;
  typedef array_property_map<int, edge_id_map<Graph> > edge_property_map;

  typedef subgraph_adapter<Graph> SubGraph;
  typedef graph_traits<SubGraph>::vertex_descriptor vertex_descriptor_sg;
  typedef graph_traits<SubGraph>::edge_descriptor edge_sg;
  typedef graph_traits<SubGraph>::size_type size_type_sg;
  typedef array_property_map<int, vertex_id_map<SubGraph> >
          vertex_property_map_sg;
  typedef array_property_map<int, edge_id_map<SubGraph> >
          edge_property_map_sg;

  // Initialize original graph.
  Graph g;

#ifdef TEST1
  // Test 1.
  const size_type numVerts = 8;
  const size_type numEdges = 10;

  int vTypes[numVerts] = { 0, 0, 0, 0, 0, 0, 0, 0 };

  size_type sources[numEdges] = { 0, 0, 1, 2, 2, 3, 4, 4, 5, 6 };
  size_type targets[numEdges] = { 1, 2, 3, 3, 5, 5, 5, 6, 7, 7 };
  int eTypes[numEdges] = { 0, 1, 1, 0, 1, 2, 0, 0, 1, 1 };
#else
  // Test 2.
  const size_type numVerts = 6;
  const size_type numEdges = 7;

  int vTypes[numVerts] = { 1, 2, 3, 2, 3, 1 };

  size_type sources[numEdges] = { 1, 2, 1, 2, 3, 3, 4 };
  size_type targets[numEdges] = { 0, 0, 2, 3, 4, 5, 5 };
  int eTypes[numEdges] = { 1, 2, 3, 3, 3, 1, 2 };
#endif

  init(numVerts, numEdges, sources, targets, g);

  // Create the big graph vertex and edge type property maps.
  vertex_id_map<Graph> g_vid_map = get(_vertex_id_map, g);
  edge_id_map<Graph> g_eid_map = get(_edge_id_map, g);
  vertex_property_map vTypemap(vTypes, g_vid_map);
  edge_property_map eTypemap(eTypes, g_eid_map);

  printf("\nThe graph (%d, %d)\n", num_vertices(g), num_edges(g));
  print(g);

  printf("\nEdge types:\n");
  for (size_type i = 0; i < numEdges; ++i)
  {
    printf("%d: (%d)\n", i, eTypes[i]);
  }

#ifdef RANDWALK
  // Create target graph.
  typedef SubGraph TargetGraph;
  typedef size_type_sg size_type_trg;
  typedef vertex_property_map_sg vertex_property_map_trg;
  typedef edge_property_map_sg edge_property_map_trg;
  typedef graph_traits<TargetGraph>::vertex_iterator vertex_iterator_trg;
  typedef graph_traits<TargetGraph>::edge_iterator edge_iterator_trg;

  size_type_trg numVertsTarget = 4;
  size_type_trg numEdgesTarget = 4;

  TargetGraph target(g);
  generate_subgraph(numVertsTarget, numEdgesTarget, g, target);

  numVertsTarget = num_vertices(target);
  numEdgesTarget = num_edges(target);

  int* vTypesTarget = new int[numVertsTarget];
  int* eTypesTarget = new int[numEdgesTarget];

  // Get the types for the target vertices from the corresponding vertices in
  // the big graph.
  vertex_iterator_trg verts_trg = vertices(target);
  for (size_type_trg i = 0; i < numVertsTarget; ++i)
  {
    size_type_trg g_vid = get(g_vid_map, target.local_to_global(verts_trg[i]));
    vTypesTarget[i] = vTypes[g_vid];
  }

  // Get the types for the target edges from the corresponding edges in the
  // big graph.
  edge_iterator_trg edgs_trg = edges(target);
  for (size_type_trg i = 0; i < numEdgesTarget; ++i)
  {
    size_type_trg g_eid = get(g_eid_map, target.local_to_global(edgs_trg[i]));
    eTypesTarget[i] = eTypes[g_eid];
  }

#else
  typedef Graph TargetGraph;
  typedef size_type size_type_trg;
  typedef vertex_property_map vertex_property_map_trg;
  typedef edge_property_map edge_property_map_trg;

  // Initialize target graph.
  TargetGraph target;

#ifdef TEST1
  // Test 1.
  const size_type_trg numVertsTarget = 4;
  const size_type_trg numEdgesTarget = 4;

  int vTypesTarget[numVerts] = { 0, 0, 0, 0 };

  size_type_trg sourcesTarget[numEdgesTarget] = { 0, 0, 1, 2 };
  size_type_trg targetsTarget[numEdgesTarget] = { 1, 2, 3, 3 };
  int eTypesTarget[numEdgesTarget]  = { 0, 1, 1, 0 };
#else
  // Test 2.
  const size_type_trg numVertsTarget = 3;
  const size_type_trg numEdgesTarget = 3;

  int vTypesTarget[numVerts] = { 1, 2, 3 };

  size_type_trg sourcesTarget[numEdgesTarget] = { 1, 2, 1 };
  size_type_trg targetsTarget[numEdgesTarget] = { 0, 0, 2 };
  int eTypesTarget[numEdgesTarget]  = { 1, 2, 3 };
#endif

  init(numVertsTarget, numEdgesTarget, sourcesTarget, targetsTarget, target);
#endif

  // Create the target graph vertex and edge type property maps.
  vertex_id_map<TargetGraph> trg_vid_map = get(_vertex_id_map, target);
  edge_id_map<TargetGraph> trg_eid_map = get(_edge_id_map, target);
  vertex_property_map_trg vTypemapTarget(vTypesTarget, trg_vid_map);
  edge_property_map_trg eTypemapTarget(eTypesTarget, trg_eid_map);

  printf("\nThe target (%d, %d)\n", numVertsTarget, numEdgesTarget);
  print(target);

  printf("\nEdge types:\n");
  for (size_type_trg i = 0; i < numEdgesTarget; ++i)
  {
    printf("%d: (%d)\n", i, eTypesTarget[i]);
  }

#ifdef RANDWALK
  // Create duplicate of target.
  duplicate_adapter<TargetGraph> dtarget(target);

  printf("\nThe target duplicate (%d, %d)\n", num_vertices(dtarget),
         num_edges(dtarget));
  print(dtarget);
  printf("\n");

  size_type_trg num_walk_edges = num_edges(dtarget) * 2;
  size_type_trg walk_length = 2 * num_walk_edges + 1;
  size_type_trg* walk_ids = new size_type_trg[walk_length];

  // Find random walk through duplicate.
  random_walk(dtarget, get_vertex(0, dtarget), walk_length, walk_ids);

  printf("Found random walk.\n");
  for (size_type_trg i = 1; i < walk_length; i += 2)
  {
    printf("%d: (%d, %d)\n", walk_ids[i], walk_ids[i-1], walk_ids[i+1]);
  }

  // Replace the duplicate edge ids with the original ids.
  for (size_type_trg i = 1; i < walk_length; i += 2)
  {
    if (walk_ids[i] >= numEdgesTarget)  walk_ids[i] -= numEdgesTarget;
  }

  printf("\nReplaced walk ids.\n");

  for (size_type_trg i = 1; i < walk_length; i += 2)
  {
    printf("%d: (%d, %d)\n", walk_ids[i], walk_ids[i-1], walk_ids[i+1]);
  }

#else
  // Set up walk through target graph.
#ifdef TEST1
  // Test 1.
  const size_type_trg walk_length = 9;
  const size_type_trg walk_edges = walk_length / 2 + 1;
  size_type_trg walk_ids[walk_length] = { 0, 0, 1, 2, 3, 3, 2, 1, 0 };
#else
  // Test 2.
  const size_type_trg walk_length = 7;
  const size_type_trg walk_edges = walk_length / 2 + 1;
  size_type_trg walk_ids[walk_length] = { 0, 0, 1, 2, 2, 1, 0 };
#endif
#endif

#ifdef FILTERED
  // Filter large graph to only edges matching edges in target graph.
  typedef SubGraph FiltGraph;
  typedef size_type_sg size_type_filt;
  typedef edge_sg edge_filt;
  typedef vertex_property_map_sg vertex_property_map_filt;
  typedef edge_property_map_sg edge_property_map_filt;
  typedef graph_traits<FiltGraph>::vertex_iterator vertex_iterator_filt;
  typedef graph_traits<FiltGraph>::edge_iterator edge_iterator_filt;

  FiltGraph filteredG(g);

  f_default_comparator<Graph, vertex_property_map, edge_property_map,
                       TargetGraph, vertex_property_map_trg,
                       edge_property_map_trg>
    f_comp(vTypemap, eTypemap, vTypemapTarget, eTypemapTarget);

  filter_graph_by_edges(g, target, f_comp, filteredG);

  size_type_filt numVertsFiltered = num_vertices(filteredG);
  size_type_filt numEdgesFiltered = num_edges(filteredG);

  printf("\n");
  printf("The filtered graph (%d, %d)\n", numVertsFiltered, numEdgesFiltered);
  print(filteredG);

  int* vTypesFiltered = new int[numVertsFiltered];
  int* eTypesFiltered = new int[numEdgesFiltered];

  vertex_iterator_filt verts_filt = vertices(filteredG);
  for (size_type_filt i = 0; i < numVertsFiltered; ++i)
  {
    size_type_filt g_vid = get(g_vid_map,
                               filteredG.local_to_global(verts_filt[i]));

    vTypesFiltered[i] = vTypes[g_vid];
  }

  edge_iterator_filt edgs_filt = edges(filteredG);
  for (size_type_filt i = 0; i < numEdgesFiltered; ++i)
  {
    size_type_filt g_eid = get(g_eid_map,
                               filteredG.local_to_global(edgs_filt[i]));
    eTypesFiltered[i] = eTypes[g_eid];
  }

  printf("\nEdge types:\n");
  for (size_type_filt i = 0; i < numEdgesFiltered; ++i)
  {
    printf("%d: (%d)\n", i, eTypesFiltered[i]);
  }

  vertex_id_map<FiltGraph> filteredG_vid_map = get(_vertex_id_map, filteredG);
  edge_id_map<FiltGraph> filteredG_eid_map = get(_edge_id_map, filteredG);
  vertex_property_map_filt vTypemapFiltered(vTypesFiltered, filteredG_vid_map);
  edge_property_map_filt eTypemapFiltered(eTypesFiltered, filteredG_eid_map);
#else
  typedef Graph FiltGraph;
  typedef size_type size_type_filt;
  typedef edge edge_filt;
  typedef vertex_property_map vertex_property_map_filt;
  typedef edge_property_map edge_property_map_filt;

  vertex_property_map_filt& vTypemapFiltered = vTypemap;
  edge_property_map_filt& eTypemapFiltered = eTypemap;
  FiltGraph& filteredG = g;
#endif

#ifdef FILTERED
  size_type_filt* result = new size_type_filt[numEdgesFiltered];
#else
  size_type* result = new size_type[numEdges];
#endif

  typedef si_default_comparator<FiltGraph, vertex_property_map_filt,
                                edge_property_map_filt, TargetGraph,
                                vertex_property_map_trg,
                                edge_property_map_trg> si_comparator;

  si_comparator si_comp(vTypemapFiltered, eTypemapFiltered,
                        vTypemapTarget, eTypemapTarget);

  subgraph_isomorphism(filteredG, target, walk_ids, walk_length, result,
                       si_comp);

  printf("\nResult\n");
#ifdef FILTERED
  size_type* result_filtered = new size_type[numEdges];

  for (size_type i = 0; i < numEdges; ++i)
  {
    result_filtered[i] = std::numeric_limits<size_type>::max();
  }

  for (size_type_filt i = 0; i < numEdgesFiltered; ++i)
  {
    size_type_filt g_eid = get(g_eid_map,
                               filteredG.local_to_global(edgs_filt[i]));
    result_filtered[g_eid] = result[i];
  }

  if (numEdges < 100)
  {
    for (size_type i = 0; i < numEdges; ++i)
    {
      printf("%d: %d\n", i, result_filtered[i]);
    }
  }
#else
  if (numEdges < 100)
  {
    for (size_type i = 0; i < numEdges; ++i) printf("%d: %d\n", i, result[i]);
  }
#endif

#ifdef RANDWALK
  delete [] vTypesTarget;
  delete [] eTypesTarget;
  delete [] walk_ids;
#endif

#ifdef FILTERED
  delete [] vTypesFiltered;
  delete [] eTypesFiltered;
  delete [] result_filtered;
#endif

  delete [] result;

  return 0;
}

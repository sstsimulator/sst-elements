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
#include <mtgl/filter_graph.hpp>
#include <mtgl/subgraph_adapter.hpp>
#include <mtgl/duplicate_adapter.hpp>
#include <mtgl/random_walk.hpp>

using namespace mtgl;

template <typename graph_adapter, typename vertex_property_map,
          typename edge_property_map,
          typename graph_adapter_trg = graph_adapter,
          typename vertex_property_map_trg = vertex_property_map,
          typename edge_property_map_trg = edge_property_map>
class type_flow_comparator {
public:
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge;
  typedef typename graph_traits<graph_adapter_trg>::vertex_descriptor
          vertex_trg;
  typedef typename graph_traits<graph_adapter_trg>::edge_descriptor edge_trg;

  type_flow_comparator(vertex_property_map gvt, edge_property_map get,
                       edge_property_map gef, vertex_property_map_trg trgvt,
                       edge_property_map_trg trget,
                       edge_property_map_trg trgef) :
    g_vtype(gvt), g_etype(get), g_eflow(gef), trg_vtype(trgvt),
    trg_etype(trget), trg_eflow(trgef) {}

  bool operator()(const edge& g_e, const edge_trg& trg_e,
                  graph_adapter& g, graph_adapter_trg& trg)
  {
    // Get vertices.
    vertex g_src = source(g_e, g);
    vertex g_dest = target(g_e, g);
    vertex_trg trg_src = source(trg_e, trg);
    vertex_trg trg_dest = target(trg_e, trg);

    // The edge is considered a match to the target edge if its type triple
    // (src vertex type, edge type, dest vertex type) matches the target
    // edge's corresponding triple.
    return (g_vtype[g_src] == trg_vtype[trg_src] &&
            g_etype[g_e] == trg_etype[trg_e] &&
            g_eflow[g_e] >= trg_eflow[trg_e] &&
            g_vtype[g_dest] == trg_vtype[trg_dest]);
  }

private:
  vertex_property_map g_vtype;
  edge_property_map g_etype;
  edge_property_map g_eflow;
  vertex_property_map_trg trg_vtype;
  edge_property_map_trg trg_etype;
  edge_property_map_trg trg_eflow;
};

template <typename Graph, typename vertex_property_map,
          typename edge_property_map>
void print_my_graph(Graph& g, vertex_property_map& vTypes,
                    edge_property_map& eTypes, edge_property_map& eFlows)
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

    printf("%lu: (%lu, %lu)   [%lu, %lu, %lu, %lu]\n", eid, uid, vid,
           vTypes[u], eTypes[e], eFlows[e], vTypes[v]);
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

  // Create the big graph vertex and edge type property maps and edge flow
  // property map.
  typedef array_property_map<int, vertex_id_map<Graph> > vertex_property_map;
  typedef array_property_map<int, edge_id_map<Graph> > edge_property_map;

  int vTypes[numVerts] = { 1, 2, 3, 2, 3, 1 };
  int eTypes[numEdges] = { 1, 2, 3, 3, 3, 1, 2 };
  int eFlows[numEdges] = { 4, 5, 9, 2, 9, 4, 6 };

  vertex_id_map<Graph> g_vid_map = get(_vertex_id_map, g);
  edge_id_map<Graph> g_eid_map = get(_edge_id_map, g);
  vertex_property_map vTypemap(vTypes, g_vid_map);
  edge_property_map eTypemap(eTypes, g_eid_map);
  edge_property_map eFlowmap(eFlows, g_eid_map);

  // Print the original graph with its vertex and edge types and edge flows.
  printf("Big graph (%d, %d)\n", num_vertices(g), num_edges(g));
  print_my_graph(g, vTypemap, eTypemap, eFlowmap);

  // Initialize target graph.
  Graph target;

  const size_type numVertsTarget = 3;
  const size_type numEdgesTarget = 3;

  size_type sourcesTarget[numEdgesTarget] = { 1, 2, 1 };
  size_type targetsTarget[numEdgesTarget] = { 0, 0, 2 };

  init(numVertsTarget, numEdgesTarget, sourcesTarget, targetsTarget, target);

  // Create the target graph vertex and edge type property maps and edge flow
  // property map.
  int vTypesTarget[numVertsTarget] = { 1, 2, 3 };
  int eTypesTarget[numEdgesTarget]  = { 1, 2, 3 };
  int eFlowsTarget[numEdgesTarget]  = { 4, 6, 8 };

  vertex_id_map<Graph> trg_vid_map = get(_vertex_id_map, target);
  edge_id_map<Graph> trg_eid_map = get(_edge_id_map, target);
  vertex_property_map vTypemapTarget(vTypesTarget, trg_vid_map);
  edge_property_map eTypemapTarget(eTypesTarget, trg_eid_map);
  edge_property_map eFlowmapTarget(eFlowsTarget, trg_eid_map);

  // Print the target graph with its vertex and edge types and edge flows.
  printf("\nTarget graph (%d, %d)\n", numVertsTarget, numEdgesTarget);
  print_my_graph(target, vTypemapTarget, eTypemapTarget, eFlowmapTarget);

  // Create duplicate of target.
  duplicate_adapter<Graph> duplicate_target(target);

  // Create the duplicate target graph vertex and edge type property maps and
  // edge flow property map.
  //
  // NOTE: We only do this to be able to print the duplicate target graph.
  //       The property maps aren't needed for the algorithm.  We can reuse
  //       vTypesTarget as the vertices and their ids are the same.
  int eTypesTargetDup[2 * numEdgesTarget];
  int eFlowsTargetDup[2 * numEdgesTarget];

  vertex_id_map<duplicate_adapter<Graph> >
    dup_trg_vid_map = get(_vertex_id_map, duplicate_target);
  edge_id_map<duplicate_adapter<Graph> >
    dup_trg_eid_map = get(_edge_id_map, duplicate_target);

  typedef array_property_map<int, vertex_id_map<duplicate_adapter<Graph> > >
    vertex_property_map_dup;
  typedef array_property_map<int, edge_id_map<duplicate_adapter<Graph> > >
    edge_property_map_dup;
  vertex_property_map_dup vTypemapTargetDup(vTypesTarget, dup_trg_vid_map);
  edge_property_map_dup eTypemapTargetDup(eTypesTargetDup, dup_trg_eid_map);
  edge_property_map_dup eFlowmapTargetDup(eFlowsTargetDup, dup_trg_eid_map);

  // Copy the types and flows from the target edges to the duplicate target
  // edges.
  for (size_type i = 0; i < numEdgesTarget; ++i)
  {
    eTypesTargetDup[i] = eTypesTarget[i];
    eTypesTargetDup[i + numEdgesTarget] = eTypesTarget[i];
    eFlowsTargetDup[i] = eFlowsTarget[i];
    eFlowsTargetDup[i + numEdgesTarget] = eFlowsTarget[i];
  }

  printf("\nDuplicate target graph (%d, %d)\n", num_vertices(duplicate_target),
         num_edges(duplicate_target));

  print_my_graph(duplicate_target, vTypemapTargetDup, eTypemapTargetDup,
                 eFlowmapTargetDup);

  size_type num_walk_edges = 2 * num_edges(duplicate_target);
  size_type walk_length = 2 * num_walk_edges + 1;
  size_type* walk_ids = new size_type[walk_length];

  // Find random walk through duplicate.
  random_walk(duplicate_target, get_vertex(0, duplicate_target),
              walk_length, walk_ids);

  printf("\nFound random walk.\n");
  for (size_type i = 1; i < walk_length; i += 2)
  {
    printf("%d: (%d, %d)\n", walk_ids[i], walk_ids[i-1], walk_ids[i+1]);
  }

  // Replace the duplicate edge ids with the original ids.
  for (size_type i = 1; i < walk_length; i += 2)
  {
    if (walk_ids[i] >= numEdgesTarget)  walk_ids[i] -= numEdgesTarget;
  }

  printf("\nReplaced walk ids.\n");

  for (size_type i = 1; i < walk_length; i += 2)
  {
    printf("%d: (%d, %d)\n", walk_ids[i], walk_ids[i-1], walk_ids[i+1]);
  }

  // Filter large graph to only edges matching edges in target graph.
  typedef subgraph_adapter<Graph> FiltGraph;
  typedef graph_traits<FiltGraph>::size_type size_type_filt;

  FiltGraph filteredG(g);

  type_flow_comparator<Graph, vertex_property_map, edge_property_map>
    tf_comp(vTypemap, eTypemap, eFlowmap, vTypemapTarget,
            eTypemapTarget, eFlowmapTarget);

  filter_graph_by_edges(g, target, tf_comp, filteredG);

  // Create the filtered graph vertex and edge type property maps and edge
  // flow property map.
  typedef array_property_map<int, vertex_id_map<FiltGraph> >
          vertex_property_map_filt;
  typedef array_property_map<int, edge_id_map<FiltGraph> >
          edge_property_map_filt;

  size_type_filt numVertsFiltered = num_vertices(filteredG);
  size_type_filt numEdgesFiltered = num_edges(filteredG);

  int* vTypesFiltered = new int[numVertsFiltered];
  int* eTypesFiltered = new int[numEdgesFiltered];
  int* eFlowsFiltered = new int[numEdgesFiltered];

  vertex_id_map<FiltGraph> filteredG_vid_map = get(_vertex_id_map, filteredG);
  edge_id_map<FiltGraph> filteredG_eid_map = get(_edge_id_map, filteredG);
  vertex_property_map_filt vTypemapFiltered(vTypesFiltered, filteredG_vid_map);
  edge_property_map_filt eTypemapFiltered(eTypesFiltered, filteredG_eid_map);
  edge_property_map_filt eFlowmapFiltered(eFlowsFiltered, filteredG_eid_map);

  // Copy the vertex types for the filtered graph.
  graph_traits<FiltGraph>::vertex_iterator verts_filt = vertices(filteredG);
  for (size_type_filt i = 0; i < numVertsFiltered; ++i)
  {
    graph_traits<FiltGraph>::vertex_descriptor fg_v = verts_filt[i];

    graph_traits<Graph>::vertex_descriptor g_v =
      filteredG.local_to_global(fg_v);

    vTypemapFiltered[fg_v] = vTypemap[g_v];
  }

  // Copy the edge types and flows for the filtered graph.
  graph_traits<FiltGraph>::edge_iterator edgs_filt = edges(filteredG);
  for (size_type_filt i = 0; i < numEdgesFiltered; ++i)
  {
    graph_traits<FiltGraph>::edge_descriptor fg_e = edgs_filt[i];

    graph_traits<Graph>::edge_descriptor g_e = filteredG.local_to_global(fg_e);

    eTypemapFiltered[fg_e] = eTypemap[g_e];
    eFlowmapFiltered[fg_e] = eFlowmap[g_e];
  }

  // Print the filtered graph with its vertex and edge types and edge flows.
  printf("\nFiltered graph (%d, %d)\n", numVertsFiltered, numEdgesFiltered);
  print_my_graph(filteredG, vTypemapFiltered, eTypemapFiltered,
                 eFlowmapFiltered);

  // Declare the results array and the comparator and run subgraph
  // isomorphism.
  size_type_filt* result_filtered = new size_type_filt[numEdgesFiltered];

  type_flow_comparator<FiltGraph, vertex_property_map_filt,
                       edge_property_map_filt,
                       Graph, vertex_property_map, edge_property_map>
    si_tf_comp(vTypemapFiltered, eTypemapFiltered, eFlowmapFiltered,
               vTypemapTarget, eTypemapTarget, eFlowmapTarget);

  subgraph_isomorphism(filteredG, target, walk_ids, walk_length,
                       result_filtered, si_tf_comp);

  size_type* result = new size_type[numEdges];

  // Initialize result such that all edges belong to no component.
  for (size_type i = 0; i < numEdges; ++i)
  {
    result[i] = std::numeric_limits<size_type>::max();
  }

  // Copy the result from the filtered graph.
  for (size_type_filt i = 0; i < numEdgesFiltered; ++i)
  {
    size_type_filt g_eid = get(g_eid_map,
                               filteredG.local_to_global(edgs_filt[i]));
    result[g_eid] = result_filtered[i];
  }

  // Print the results.
  printf("\nResult\n");
  for (size_type i = 0; i < numEdges; ++i) printf("%lu: %lu\n", i, result[i]);

  delete [] walk_ids;
  delete [] vTypesFiltered;
  delete [] eTypesFiltered;
  delete [] eFlowsFiltered;
  delete [] result_filtered;
  delete [] result;

  return 0;
}

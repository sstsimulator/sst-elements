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
/*! \file test_contraction.cpp

    \author Jon Berry (jberry@sandia.gov)

    \date 5/1/2008
*/
/****************************************************************************/

#include <mtgl/graph_adapter.hpp>
#include <mtgl/contraction_adapter.hpp>
#include <mtgl/snl_community2.hpp>
#include <mtgl/connected_components.hpp>
#include <mtgl/util.hpp>

using namespace mtgl;

/***/

typedef graph_adapter<undirectedS> Graph;

template <typename graph_adapter>
class primal_filter {
public:
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_t;
  typedef typename graph_traits<graph_adapter>::size_type size_type;
  primal_filter(graph_adapter& gg, double* prml, double thresh)
    : primal(prml), eid_map(get(_edge_id_map, gg)), threshold(thresh) {}

  bool operator()(edge_t e)
  {
    size_type eid = get(eid_map, e);
    return (primal[eid] >= threshold);
  }

private:
  edge_id_map<graph_adapter> eid_map;
  double* primal;
  double threshold;
};

int main()
{
  //typedef contraction_adapter<graph_adapter> CGraph;
  typedef graph_traits<Graph>::vertex_descriptor vertex_t;
  typedef graph_traits<Graph>::edge_descriptor edge_t;
  typedef graph_traits<Graph>::size_type size_type;
  typedef graph_traits<Graph>::vertex_iterator vertex_iterator;

  // Initialize graph.
  Graph ga;

  //const int numVertices = 8;
  //const int numEdges = 9;
  const size_type numVertices = 6;
  const size_type numEdges = 7;

  //int sources[numEdges] = { 0, 1, 2, 3, 4, 5};
  //int targets[numEdges] = { 1, 2, 3, 4, 5, 1};
  size_type sources[numEdges] = { 0, 1, 2, 2, 3, 4, 2};
  size_type targets[numEdges] = { 1, 2, 0, 3, 4, 2, 5};
  //int sources[numEdges] = { 0, 1, 2, 3, 2, 4,5,6,7};
  //int targets[numEdges] = { 1, 2, 3, 0, 4, 5,6,7,4};

  init(numVertices, numEdges, sources, targets, ga);

  print(ga);
  size_type order = num_vertices(ga);
  size_type size =  num_edges(ga);
  printf("ord: %d, sz: %d\n", order, size);
  int* server = new int[order];
  int64_t* component = new int64_t[order];
  double* primal = new double[2 * order + 2 * size];
  double* support = new double[size];

  for (size_type i = 0; i < order; i++) component[i] = i;

  snl_community2<Graph> sga(ga, server, primal, support);
  sga.run();

  primal_filter<Graph> pf(ga, primal, order / (double) (2 * size));
  connected_components_sv<Graph, int64_t, primal_filter<Graph> >
  csv(ga, component, pf);
  csv.run();
  vertex_t* v_component = (vertex_t*) malloc(sizeof(vertex_t) * order);

  vertex_iterator verts = vertices(ga);

  for (size_type i = 0; i < order; i++)
  {
    v_component[i] = verts[component[i]];
    printf("comp[%d]: %d\n", i, component[i]);
  }

  // Test subgraph code.
  contraction_adapter<Graph> cga(ga);

  contract<Graph>(numVertices, v_component, cga);

  print(cga);
  printf("done printing\n");
  fflush(stdout);
  {
    size_type c_order = num_vertices(cga);
    size_type c_size =  num_edges(cga);
    int* c_server = new int[c_order];
    double* primal = new double[2 * c_order + 2 * c_size];
    double* support = new double[c_size];

    snl_community2<Graph> sga(cga.contracted_graph, server,
                                      primal, support);
    sga.run();

    for (size_type i = 0; i < c_order; i++)
    {
      printf("server[%d]: %d \n", i, server[i]);
    }
    for (size_type i = 0; i < c_size; i++)
    {
      printf("primal[%d]: %lf\n", i, primal[i]);
    }
  }

  return 0;
}

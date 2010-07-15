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
/*! \file filter_graph.hpp

    \brief Revised version of the graph filter algorithm that filters a graph
           based on the edge properties of a target graph.

    \author Greg Mackey (gemacke@sandia.gov)

    \date 7/2008

    Each edge in the original graph is compared against the edges in the
    target graph.  If the original edge matches any edge in the target graph
    based on the match described in the filter function object, then that
    edge is kept.  Otherwise, it is thrown away.  Then, an edge induced
    subgraph of the original graph, created from the matching edges, is
    returned.

    The algorithm takes a function object that defines the comparison made
    for the filter.  A default comparator is provided (f_default_comparator),
    but a user can define their own.  The only restriction is that the
    () operator needs to take the same paramters as in f_default_comparator.

    This algorithm visits edges based on DIRECTED travel.  From a vertex
    in a directed or bidirectional graph, all out edges are visited.  The
    result for a directed and a bidirectional graph should be the same when
    the algorithm is run in serial.  From a vertex in an undirected graph,
    all adjacent edges are visited which means that the comparator is called
    twice for each edge where each endpoint gets a chance to be the "source".
*/
/****************************************************************************/

// TODO:  Need to add a filter_graph_by_vertices class.

#ifndef MTGL_FILTER_GRAPH_HPP
#define MTGL_FILTER_GRAPH_HPP

#include <mtgl/psearch.hpp>
#include <mtgl/subgraph_adapter.hpp>

namespace mtgl {

/*! \brief Default comparison function object that determines if an edge from
           the big graph matches the given edge from the target graph.

  This function object checks if the type of the edge and the types of the
  source and target vertices from the big graph match those of the currently
  considered target graph edge.  For directed and bidirectional graphs, the
  comparator is called once for each edge.  For undirected graphs the
  comparator is called twice for each edge where each endpoint gets a chance
  to be the "source".

  The comparator function object is passed as a parameter to
  filter_graph_by_edges().  Thus, the user can define their own comparator
  function object to customize the matching.  The only requirement is that
  the definition of operator() must be identical to the one in the default
  comparator.
*/
template <typename graph_adapter, typename vertex_property_map,
          typename edge_property_map,
          typename graph_adapter_trg = graph_adapter,
          typename vertex_property_map_trg = vertex_property_map,
          typename edge_property_map_trg = edge_property_map>
class f_default_comparator {
public:
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge;
  typedef typename graph_traits<graph_adapter_trg>::vertex_descriptor
          vertex_trg;
  typedef typename graph_traits<graph_adapter_trg>::edge_descriptor edge_trg;

  f_default_comparator(vertex_property_map gvt, edge_property_map get,
                       vertex_property_map_trg trgvt,
                       edge_property_map_trg trget) :
    g_vtype(gvt), g_etype(get), trg_vtype(trgvt), trg_etype(trget) {}

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
            g_vtype[g_dest] == trg_vtype[trg_dest]);
  }

private:
  vertex_property_map g_vtype;
  edge_property_map g_etype;
  vertex_property_map_trg trg_vtype;
  edge_property_map_trg trg_etype;

  /*!
    \fn bool operator()(const edge& g_e, const edge_trg& trg_e,
                        graph_adapter& g, graph_adapter_trg& trg)
    \brief Checks if edge g_e matches edge trg_e where a match is when the
           type triples (source vertex type, edge type, destination vertex
           type) match.

    \param g_e The edge from the big graph.
    \param trg_e The edge from the target graph.
    \param g The big graph.
    \param trg The target graph.
  */
};

/***/

/*! \brief Filters edges and vertices in a graph by types using a target graph
           as the template.

  Given type information about edges and vertices in a target graph, this
  visitor class tailors the search of a large graph to mask out edges that
  don't match one of the target edges.  An edge is considered a match if its
  type triple matches.
*/
template <typename graph_adapter, typename filter_comparator,
          typename graph_adapter_trg>
class iso_edge_filter : public default_psearch_visitor<graph_adapter> {
public:
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge;
  typedef typename graph_traits<graph_adapter>::edge_iterator edge_iterator;

  iso_edge_filter(graph_adapter& graph, graph_adapter_trg& trg,
                  filter_comparator& f_comp, int* e_mask) :
      g(graph), targetG(trg), filter_compare(f_comp),
      edge_mask(e_mask),
      eid_map(get(_edge_id_map, graph)),
      tgsize(num_edges(trg)) {}

  void tree_edge(edge& e, vertex& src) const
  {
    // "Randomize" the start of the search through target edges so that all
    // incident edge threads don't access the same sequence.
    int eid = get(eid_map, e);
    int start = eid;
    edge_iterator tg_edges = edges(targetG);

    for (int i = 0; i < tgsize; ++i)
    {
      int j = start++ % tgsize;
      if (filter_compare(e, tg_edges[j], g, targetG))
      {
        edge_mask[eid] = 1;
        break;
      }
    }
  }

  void back_edge(edge& e, vertex& src) const
  {
    // "Randomize" the start of the search through target edges so that all
    // incident edge threads don't access the same sequence.
    int eid = get(eid_map, e);
    int start = eid;
    edge_iterator tg_edges = edges(targetG);

    for (int i = 0; i < tgsize; ++i)
    {
      int j = start++ % tgsize;
      if (filter_compare(e, tg_edges[j], g, targetG))
      {
        edge_mask[eid] = 1;
        break;
      }
    }
  }

protected:
  graph_adapter& g;
  graph_adapter_trg& targetG;
  filter_comparator& filter_compare;
  int* edge_mask;
  edge_id_map<graph_adapter> eid_map;
  int tgsize;

  /*!
    \fn iso_edge_filter(graph_adapter& graph, graph_adapter& trg,
                        filter_comparator& f_comp, int* e_mask) :
    \brief This constructor initializes member data.

    \param graph The search graph.
    \param trg The target graph.
    \param f_comp Comparator to determine which edges match the edges in the
                  target graph.
    \param e_mask The result of the searches: a boolean array indicating
                  edge matches in the search graph.
  */
  /*!
    \fn void tree_edge(edge *e, vertex *src) const
    \brief Check if this edge from the big graph matches the edge from the
           target graph.  The match is based on the operation defined in the
           comparator.

    \param e The edge being traversed for the first time.
    \param src The endpoint from which the traversal originated.

    The access to target graph edge properties is dispersed with modular
    arithmetic, i.e., starting with eid % target_size.
  */
  /*!
    \fn void back_edge(edge *e, vertex *src) const
    \brief This method is identical to the tree_edge method, as it is
           irrelevant to this visitor how an edge has been encountered.

    \param e The edge being traversed for the first time.
    \param src The endpoint from which the traversal originated.

    The access to target graph edge properties is dispersed with modular
    arithmetic, i.e., starting with eid % target_size.
  */
  /*!
    \var graph_adapter& g
    \brief The search graph.
  */
  /*!
    \var graph_adapter& target
    \brief The target graph.
  */
  /*!
    \var filter_compare Comparator to determine which edges match the edges in
                        the target graph.
  */
  /*!
    \var int *edge_mask
    \brief The result of the searches -- a mask allowing through only edges
           that match one of the target graph edges.
  */
  /*!
    \var eidm edge_id_map<graph_adapter> eid_map;
    \brief Property map to edge ids.
  */
  /*!
    \var int tgsize
    \brief The number of edges in the target graph.
  */
};

/***/

template <class graph_adapter, class filter_comparator,
          typename graph_adapter_trg>
void filter_graph_by_edges(graph_adapter& g, graph_adapter_trg& target,
                           filter_comparator& filter_compare,
                           subgraph_adapter<graph_adapter>& filteredG)
{
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_descriptor;
  typedef typename graph_traits<graph_adapter>::size_type size_type;
  typedef typename graph_traits<graph_adapter>::edge_iterator edge_iterator;

  size_type n_edges = num_edges(g);

  // The bitmap indicating which edges to keep.
  int* possible_edges = (int*) calloc(n_edges, sizeof(int));

  iso_edge_filter<graph_adapter, filter_comparator, graph_adapter_trg>
      edgeTypeFilter(g, target, filter_compare, possible_edges);

  // Do a psearch_high_low<NO_FILTER, DIRECTED> with a visitor that doesn't
  // define a visit_test().  The default visit_test() always returns false.
  // Thus, each vertex will be visited once, and each out edge of every
  // vertex will be visited.  For directed and bidirectional graphs, each
  // edge will be visited once.  For undirected graphs, each edge will be
  // visited twice  where each endpoint gets a chance to be the "source".
  psearch_high_low<graph_adapter,
                   iso_edge_filter<graph_adapter, filter_comparator,
                                   graph_adapter_trg>,
                   NO_FILTER, DIRECTED> psrch(g, edgeTypeFilter, 100);
  psrch.run();

  // Count the number of possible edges.
  size_type edge_count = 0;
  for (size_type i = 0; i < n_edges; ++i)
  {
    edge_count += possible_edges[i] > 0;
  }

  int* possible_edges2 = (int*) malloc(n_edges * sizeof(int));

  // Change possible edges where each entry is the sum of all the previous
  // entries.
  possible_edges2[0] = possible_edges[0];
  for (size_type i = 1; i < n_edges; ++i)
  {
    possible_edges2[i] = possible_edges2[i - 1] + possible_edges[i];
  }

  // This sets the entries in possible_edges to be either -1 (representing
  // an edge that isn't possible) or 0..edge_count-1 (representing the new
  // edge id of a possible edge).
  possible_edges[0] = possible_edges2[0] == 1 ? 0 : -1;
  for (size_type i = 1; i < n_edges; ++i)
  {
    possible_edges[i] = possible_edges2[i] == possible_edges2[i - 1] ?
                        -1 : possible_edges2[i] - 1;
  }

  free(possible_edges2);

  // Create an array of the possible edges then create a subgraph out of the
  // possible edges.
  edge_descriptor* filtered_edges =
    (edge_descriptor*) malloc(edge_count * sizeof(edge_descriptor));

  edge_iterator edgs = edges(g);

  #pragma mta assert nodep
  for (size_type i = 0; i < n_edges; ++i)
  {
    if (possible_edges[i] >= 0)
    {
      filtered_edges[possible_edges[i]] = edgs[i];
    }
  }

  init_edges(filtered_edges, edge_count, filteredG);

  free(filtered_edges);
  free(possible_edges);
}

}

#endif

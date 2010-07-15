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
/*! \file adjacency_list.hpp

    \brief An adjacency list graph implementation.

    \author Greg Mackey (gemacke@sandia.gov)

    \date 3/17/2008
*/
/****************************************************************************/

#ifndef MTGL_ADJACENCY_LIST_HPP
#define MTGL_ADJACENCY_LIST_HPP

#include <iostream>

#include <mtgl/dynamic_array.hpp>
#include <mtgl/xmt_hash_set.hpp>
#include <mtgl/graph_traits.hpp>

namespace mtgl {

template <typename DIRECTION = directedS>
struct adjacency_list {
  struct Vertex;
  struct Edge;

  adjacency_list() : nVertices(0), nEdges(0) {}
  adjacency_list(const adjacency_list& g) { deep_copy(g); }
  ~adjacency_list() { clear(); }

  adjacency_list& operator=(const adjacency_list& rhs)
  {
    clear();
    deep_copy(rhs);
    return *this;
  }

  inline Vertex* addVertex();
  inline void addVertices(unsigned long num_verts);
  inline void removeVertices(unsigned long num_verts, Vertex** v);
  inline void removeVertices(unsigned long num_verts, unsigned long* v);
  inline Edge* addEdge(Vertex* f, Vertex* t);
  Edge* addEdge(unsigned long f_id, unsigned long t_id)
  { return addEdge(vertices[f_id], vertices[t_id]); }

  inline void addEdges(unsigned long num_edges, Vertex** f, Vertex** t);

  inline void
  addEdges(unsigned long num_edges, unsigned long* f, unsigned long* t);

  inline void removeEdges(unsigned long num_edges, Edge** e);
  inline void removeEdges(unsigned long num_edges, unsigned long* e);

  bool is_directed() const { return DIRECTION::is_directed(); }
  bool is_undirected() const { return !is_directed(); }

  inline void init(unsigned long num_verts, unsigned long num_edges,
                   unsigned long* sources, unsigned long* targets);

  void print() const;
  void printDotfile(const std::string& filename) const;

  inline void clear();

private:
  inline void deep_copy(const adjacency_list& rhs);

public:
  unsigned long nVertices;
  unsigned long nEdges;

  dynamic_array<Vertex*> vertices;
  dynamic_array<Edge*> edges;
};

/***/

template <>
class adjacency_list<bidirectionalS> {
public:
  adjacency_list()
  {
    fprintf(stderr, "** Error: Bidirectional direction not yet supported "
            "for adjacency_list. **\n");
    exit(1);
  }
};

/***/

template <typename DIRECTION>
struct adjacency_list<DIRECTION>::Edge {
  Edge(Vertex* f, Vertex* t, unsigned long i) : id(i), from(f), to(t) {}

  unsigned long id;
  Vertex* from;
  Vertex* to;
};

/***/

template <typename DIRECTION>
struct adjacency_list<DIRECTION>::Vertex {
  Vertex(unsigned long i) : id(i), num_edges_to_add(0), edges(5) {}

  void removeEdge(Edge* e)
  {
    unsigned long lock_val = mt_readfe(num_edges_to_add);

    // Find the edge.
    unsigned long i = 0;
    for ( ; i < edges.size() && edges[i] != e; ++i);

    // If it isn't the last edge, replace the empty entry with the last edge.
    // This test catches the case when there is only one edge as, for this
    // case, edges.size() - 1 is 0.
    if (i < edges.size() - 1)
    {
      edges[i] = edges[edges.size() - 1];
    }

    // If the edge was found it is now in the last position.  Make the array
    // one smaller.
    if (i < edges.size()) edges.resize(edges.size() - 1);

    mt_write(num_edges_to_add, lock_val);
  }

  unsigned long id;
  unsigned long num_edges_to_add;
  dynamic_array<Edge*> edges;
};

/***/

template <typename DIRECTION>
void
adjacency_list<DIRECTION>::clear()
{
  nVertices = 0;
  nEdges = 0;

  #pragma mta assert parallel
  for (unsigned long i = 0; i < vertices.size(); ++i) delete vertices[i];

  #pragma mta assert parallel
  for (unsigned long i = 0; i < edges.size(); ++i) delete edges[i];

  // Empty the vertex and edge arrays.
  vertices.clear();
  edges.clear();
}

/***/

template <typename DIRECTION>
typename adjacency_list<DIRECTION>::Vertex*
adjacency_list<DIRECTION>::addVertex()
{
  unsigned long id = mt_readfe(nVertices);
  Vertex* v = new Vertex(id);
  vertices.push_back(v);
  mt_write(nVertices, nVertices + 1);

  return v;
}

/***/

template <typename DIRECTION>
void
adjacency_list<DIRECTION>::addVertices(unsigned long num_verts)
{
  unsigned long id = mt_readfe(nVertices);

  vertices.resize(id + num_verts);

  // I might be able to move the mt_write to here, but for now we need to
  // be conservative with unlocking.

  #pragma mta assert nodep
  for (unsigned long i = 0; i < num_verts; ++i)
  {
    vertices[id + i] = new Vertex(id + i);
  }

  mt_write(nVertices, id + num_verts);
}

/***/

template <typename DIRECTION>
void
adjacency_list<DIRECTION>::removeVertices(unsigned long num_verts, Vertex** v)
{
  unsigned long id = mt_readfe(nVertices);

  // Create a set that has all the vertices to be deleted.
  xmt_hash_set<unsigned long> delete_verts(2 * num_verts);

  #pragma mta assert parallel
  for (unsigned long i = 0; i < num_verts; ++i) delete_verts.insert(v[i]->id);

  // Count the number of edges in the graph that have one or more endpoints
  // that will be deleted.
  unsigned long total_edges = 0;
  for (unsigned long i = 0; i < id; ++i)
  {
    for (unsigned long j = 0; j < vertices[i]->edges.size(); ++j)
    {
      total_edges += (delete_verts.member(vertices[i]->edges[j]->from->id) ||
                      delete_verts.member(vertices[i]->edges[j]->to->id));
    }
  }

  // Loop over all the vertices and all their adjacency lists.  Mark an edge
  // to be deleted if either its source or target belong to the set of
  // vertices to be deleted.
  Edge** edges_to_delete = (Edge**) malloc(sizeof(Edge*) * total_edges);
  int edge_pos = 0;
  #pragma mta assert parallel
  for (unsigned long i = 0; i < id; ++i)
  {
    #pragma mta assert parallel
    for (unsigned long j = 0; j < vertices[i]->edges.size(); ++j)
    {
      if (delete_verts.member(vertices[i]->edges[j]->from->id) ||
          delete_verts.member(vertices[i]->edges[j]->to->id))
      {
        int cur_pos = mt_incr(edge_pos, 1);
        edges_to_delete[cur_pos] = vertices[i]->edges[j];
      }
    }
  }

  // Remove the edges and free the temporary memory.
  removeEdges(total_edges, edges_to_delete);
  free(edges_to_delete);

  // Remove the vertices from the vertex list that are in the area that is to
  // be deleted.
  #pragma mta assert nodep
  for (unsigned long i = 0; i < num_verts; ++i)
  {
    if (v[i]->id >= id - num_verts) vertices[v[i]->id] = 0;
  }

  // Remove the vertices from the vertex list that are not in the area that is
  // to be deleted.
  unsigned long moved_verts = 0;
  #pragma mta assert nodep
  for (unsigned long i = 0; i < num_verts; ++i)
  {
    if (v[i]->id < id - num_verts)
    {
      // Find the next last vertex in the area to be deleted that hasn't
      // already been deleted.
      unsigned long nmv = mt_incr(moved_verts, 1);
      while (vertices[id - 1 - nmv] == 0) nmv = mt_incr(moved_verts, 1);

      // Move the last vertex into the deleted vertex's position.
      vertices[v[i]->id] = vertices[id - 1 - nmv];

      // Change the moved vertex to have the id of the vertex it replaced.
      vertices[v[i]->id]->id = v[i]->id;
    }
  }

  vertices.resize(id - num_verts);

  mt_write(nVertices, id - num_verts);

  // Delete the memory associated with the deleted vertices.
  #pragma mta assert parallel
  for (unsigned long i = 0; i < num_verts; ++i) delete v[i];
}

/***/

template <typename DIRECTION>
void
adjacency_list<DIRECTION>::removeVertices(unsigned long num_verts,
                                          unsigned long* v)
{
  unsigned long id = mt_readfe(nVertices);

  // Create a set that has all the vertices to be deleted.
  xmt_hash_set<unsigned long> delete_verts(2 * num_verts);

  #pragma mta assert parallel
  for (unsigned long i = 0; i < num_verts; ++i) delete_verts.insert(v[i]);

  // Count the number of edges in the graph that have one or more endpoints
  // that will be deleted.
  unsigned long total_edges = 0;
  for (unsigned long i = 0; i < id; ++i)
  {
    for (unsigned long j = 0; j < vertices[i]->edges.size(); ++j)
    {
      total_edges += (delete_verts.member(vertices[i]->edges[j]->from->id) ||
                      delete_verts.member(vertices[i]->edges[j]->to->id));
    }
  }

  // Loop over all the vertices and all their adjacency lists.  Mark an edge
  // to be deleted if either its source or target belong to the set of
  // vertices to be deleted.
  Edge** edges_to_delete = (Edge**) malloc(sizeof(Edge*) * total_edges);
  int edge_pos = 0;
  #pragma mta assert parallel
  for (unsigned long i = 0; i < id; ++i)
  {
    #pragma mta assert parallel
    for (unsigned long j = 0; j < vertices[i]->edges.size(); ++j)
    {
      if (delete_verts.member(vertices[i]->edges[j]->from->id) ||
          delete_verts.member(vertices[i]->edges[j]->to->id))
      {
        int cur_pos = mt_incr(edge_pos, 1);
        edges_to_delete[cur_pos] = vertices[i]->edges[j];
      }
    }
  }

  // Remove the edges and free the temporary memory.
  removeEdges(total_edges, edges_to_delete);
  free(edges_to_delete);

  // Remove the vertices from the vertex list that are in the area that is to
  // be deleted and delete their memory.
  #pragma mta assert nodep
  for (unsigned long i = 0; i < num_verts; ++i)
  {
    if (v[i] >= id - num_verts)
    {
      delete vertices[v[i]];
      vertices[v[i]] = 0;
    }
  }

  // Remove the vertices from the vertex list that are not in the area that is
  // to be deleted and delete their memory.
  unsigned long moved_verts = 0;
  #pragma mta assert nodep
  for (unsigned long i = 0; i < num_verts; ++i)
  {
    if (v[i] < id - num_verts)
    {
      // Find the next last vertex in the area to be deleted that hasn't
      // already been deleted.
      unsigned long nmv = mt_incr(moved_verts, 1);
      while (vertices[id - 1 - nmv] == 0) nmv = mt_incr(moved_verts, 1);

      delete vertices[v[i]];

      // Move the last vertex into the deleted vertex's position.
      vertices[v[i]] = vertices[id - 1 - nmv];

      // Change the moved vertex to have the id of the vertex it replaced.
      vertices[v[i]]->id = v[i];
    }
  }

  vertices.resize(id - num_verts);

  mt_write(nVertices, id - num_verts);
}

/***/

template <typename DIRECTION>
typename adjacency_list<DIRECTION>::Edge*
adjacency_list<DIRECTION>::addEdge(Vertex* f, Vertex* t)
{
  unsigned long id = mt_readfe(nEdges);
  Edge* e = new Edge(f, t, id);
  edges.push_back(e);

  f->edges.push_back(e);
  if (is_undirected()) t->edges.push_back(e);

  mt_write(nEdges, nEdges + 1);

  return e;
}

/***/

template <typename DIRECTION>
void
adjacency_list<DIRECTION>::addEdges(unsigned long num_edges, Vertex** f,
                                    Vertex** t)
{
  unsigned long id = mt_readfe(nEdges);

  edges.resize(id + num_edges);

  // Add the new edges to the edges array.
  #pragma mta assert nodep
  for (unsigned long i = 0; i < num_edges; ++i)
  {
    edges[id + i] = new Edge(f[i], t[i], id + i);
  }

  // Count the number of edges to add to each vertex's edge list.
  #pragma mta assert nodep
  for (unsigned long i = 0; i < num_edges; ++i)
  {
    mt_incr(f[i]->num_edges_to_add, 1);
    if (is_undirected()) mt_incr(t[i]->num_edges_to_add, 1);
  }

  // TODO: Instead of using a hash set and looping over the edges, I could
  //       also just loop over all the vertices in the graph.  Vertices that
  //       don't have edges being added will have num_edges_to_add set to
  //       0, so the reserve call won't do anything.

  // Resize each vertex's edge list and reset each vertex's num_edges_to_add
  // to 0.
  xmt_hash_set<unsigned long> unique_verts(2 * num_edges);

  #pragma mta assert parallel
  for (unsigned long i = 0; i < num_edges; ++i)
  {
    if (unique_verts.insert(f[i]->id))
    {
      f[i]->edges.reserve(f[i]->edges.size() + f[i]->num_edges_to_add);
      f[i]->num_edges_to_add = 0;
    }

    if (is_undirected())
    {
      if (unique_verts.insert(t[i]->id))
      {
        t[i]->edges.reserve(t[i]->edges.size() + t[i]->num_edges_to_add);
        t[i]->num_edges_to_add = 0;
      }
    }
  }

  // Add the new edges to each vertex's edge list.  This can be done
  // "unsafely" since we have already resized all the edge lists
  // appropriately.
  #pragma mta assert nodep
  for (unsigned long i = 0; i < num_edges; ++i)
  {
    f[i]->edges.unsafe_push_back(edges[id + i]);
    if (is_undirected()) t[i]->edges.unsafe_push_back(edges[id + i]);
  }

  mt_write(nEdges, id + num_edges);
}

/***/

template <typename DIRECTION>
void
adjacency_list<DIRECTION>::addEdges(unsigned long num_edges, unsigned long* f,
                                    unsigned long* t)
{
  unsigned long id = mt_readfe(nEdges);

  edges.resize(id + num_edges);

  // Add the new edges to the edges array.
  #pragma mta assert nodep
  for (unsigned long i = 0; i < num_edges; ++i)
  {
    edges[id + i] = new Edge(vertices[f[i]], vertices[t[i]], id + i);
  }

  // Count the number of edges to add to each vertex's edge list.
  #pragma mta assert nodep
  for (unsigned long i = 0; i < num_edges; ++i)
  {
    mt_incr(vertices[f[i]]->num_edges_to_add, 1);
    if (is_undirected()) mt_incr(vertices[t[i]]->num_edges_to_add, 1);
  }

  // TODO: Instead of using a hash set and looping over the edges, I could
  //       also just loop over all the vertices in the graph.  Vertices that
  //       don't have edges being added will have num_edges_to_add set to
  //       0, so the reserve call won't do anything.

  // Resize each vertex's edge list and reset each vertex's num_edges_to_add
  // to 0.
  xmt_hash_set<unsigned long> unique_verts(2 * num_edges);

  #pragma mta assert parallel
  for (unsigned long i = 0; i < num_edges; ++i)
  {
    if (unique_verts.insert(vertices[f[i]]->id))
    {
      vertices[f[i]]->edges.reserve(vertices[f[i]]->edges.size() +
                                    vertices[f[i]]->num_edges_to_add);
      vertices[f[i]]->num_edges_to_add = 0;
    }

    if (is_undirected())
    {
      if (unique_verts.insert(vertices[t[i]]->id))
      {
        vertices[t[i]]->edges.reserve(vertices[t[i]]->edges.size() +
                                      vertices[t[i]]->num_edges_to_add);
        vertices[t[i]]->num_edges_to_add = 0;
      }
    }
  }

  // Add the new edges to each vertex's edge list.  This can be done
  // "unsafely" since we have already resized all the edge lists
  // appropriately.
  #pragma mta assert nodep
  for (unsigned long i = 0; i < num_edges; ++i)
  {
    vertices[f[i]]->edges.unsafe_push_back(edges[id + i]);
    if (is_undirected()) vertices[t[i]]->edges.unsafe_push_back(edges[id + i]);
  }

  mt_write(nEdges, id + num_edges);
}

/***/

template <typename DIRECTION>
void
adjacency_list<DIRECTION>::removeEdges(unsigned long num_edges, Edge** e)
{
  unsigned long id = mt_readfe(nEdges);

  // Remove the edges from the source vertices' adjacency lists.
  #pragma mta assert parallel
  for (unsigned long i = 0; i < num_edges; ++i)
  {
    e[i]->from->removeEdge(e[i]);
  }

  // If the graph is undirected, remove the edges from the target vertices'
  // adjacency lists.
  if (is_undirected())
  {
    #pragma mta assert parallel
    for (unsigned long i = 0; i < num_edges; ++i) e[i]->to->removeEdge(e[i]);
  }

  // Remove the edges from the edge list that are in the area that is to be
  // deleted.
  #pragma mta assert nodep
  for (unsigned long i = 0; i < num_edges; ++i)
  {
    if (e[i]->id >= id - num_edges) edges[e[i]->id] = 0;
  }

  // Remove the edges from the edge list that are not in the area that is
  // to be deleted.
  unsigned long moved_edges = 0;
  #pragma mta assert nodep
  for (unsigned long i = 0; i < num_edges; ++i)
  {
    if (e[i]->id < id - num_edges)
    {
      // Find the next last edge in the area to be deleted that hasn't already
      // been deleted.
      unsigned long nme = mt_incr(moved_edges, 1);
      while (edges[id - 1 - nme] == 0) nme = mt_incr(moved_edges, 1);

      // Move the last edge into the deleted edge's position.
      edges[e[i]->id] = edges[id - 1 - nme];

      // Change the moved edge to have the id of the edge it replaced.
      edges[e[i]->id]->id = e[i]->id;
    }
  }

  edges.resize(id - num_edges);

  mt_write(nEdges, id - num_edges);

  // Delete the memory associated with the edges.
  #pragma mta assert nodep
  for (unsigned long i = 0; i < num_edges; ++i) delete e[i];
}

/***/

template <typename DIRECTION>
void
adjacency_list<DIRECTION>::removeEdges(unsigned long num_edges,
                                       unsigned long* e)
{
  unsigned long id = mt_readfe(nEdges);

  // Remove the edges from the source vertices' adjacency lists.
  #pragma mta assert parallel
  for (unsigned long i = 0; i < num_edges; ++i)
  {
    edges[e[i]]->from->removeEdge(edges[e[i]]);
  }

  // If the graph is undirected, remove the edges from the target vertices'
  // adjacency lists.
  if (is_undirected())
  {
    #pragma mta assert parallel
    for (unsigned long i = 0; i < num_edges; ++i)
    {
      edges[e[i]]->to->removeEdge(edges[e[i]]);
    }
  }

  // Remove the edges from the edge list that are in the area that is to be
  // deleted and delete their memory.
  #pragma mta assert nodep
  for (unsigned long i = 0; i < num_edges; ++i)
  {
    if (e[i] >= id - num_edges)
    {
      delete edges[e[i]];
      edges[e[i]] = 0;
    }
  }

  // Remove the edges from the edge list that are not in the area that is
  // to be deleted and delete their memory.
  unsigned long moved_edges = 0;
  #pragma mta assert nodep
  for (unsigned long i = 0; i < num_edges; ++i)
  {
    if (e[i] < id - num_edges)
    {
      // Find the next last edge in the area to be deleted that hasn't already
      // been deleted.
      unsigned long nme = mt_incr(moved_edges, 1);
      while (edges[id - 1 - nme] == 0) nme = mt_incr(moved_edges, 1);

      delete edges[e[i]];

      // Move the last edge into the deleted edge's position.
      edges[e[i]] = edges[id - 1 - nme];

      // Change the moved edge to have the id of the edge it replaced.
      edges[e[i]]->id = e[i];
    }
  }

  edges.resize(id - num_edges);

  mt_write(nEdges, id - num_edges);
}

/***/

template <typename DIRECTION>
void
adjacency_list<DIRECTION>::init(unsigned long num_verts,
                                unsigned long num_edges,
                                unsigned long* sources, unsigned long* targets)
{
  addVertices(num_verts);
  addEdges(num_edges, sources, targets);
}

/***/

template <typename DIRECTION>
void
adjacency_list<DIRECTION>::print() const
{
  std::cout << std::endl << "Vertices: " << nVertices << std::endl;
  unsigned long nVerts = vertices.size();
  for (unsigned long i = 0; i < nVerts; ++i)
  {
    Vertex* v = vertices[i];

    std::cout << v->id << std::endl;
  }

  std::cout << std::endl << "Edges: " << nEdges << std::endl;
  unsigned long num_edges = edges.size();
  for (unsigned long i = 0; i < num_edges; ++i)
  {
    Edge* e = edges[i];

    std::cout << e->id << " (" << e->from->id << ", "
              << e->to->id << ")" << std::endl;
  }
}

/***/

template <typename DIRECTION>
void
adjacency_list<DIRECTION>::printDotfile(const std::string& filename) const
{
  FILE* GRAPHFILE = fopen(filename.c_str(), "w");
  fprintf(GRAPHFILE, "digraph gname {\n");
  fprintf(GRAPHFILE, "  ratio=.5;\n");
  fprintf(GRAPHFILE, "  margin=.1;\n\n");

  for (unsigned long i = 0; i < nEdges; ++i)
  {
    fprintf(GRAPHFILE, "  %7d -> %7d;\n", edges[i]->from->id,
            edges[i]->to->id);
  }

  fprintf(GRAPHFILE, "}\n");
  fclose(GRAPHFILE);
}

/***/

template <typename DIRECTION>
void
adjacency_list<DIRECTION>::deep_copy(const adjacency_list& rhs)
{
  // Sets the number of vertices and edges and the edge direction.
  nVertices = rhs.vertices.size();
  nEdges = rhs.edges.size();

  vertices.resize(nVertices);
  edges.resize(nEdges);

  // Add the vertices.
  #pragma mta assert parallel
  for (unsigned long i = 0; i < nVertices; ++i)
  {
    vertices[i] = new Vertex(i);

    // Set the initial size of the vertices edge lists to the number of edges
    // they will contain.
    vertices[i]->edges.resize(rhs.vertices[i]->edges.size());
  }

  // Add the edges.
  #pragma mta assert parallel
  for (unsigned long i = 0; i < nEdges; ++i)
  {
    edges[i] = new Edge(vertices[rhs.edges[i]->from->id],
                        vertices[rhs.edges[i]->to->id], i);
  }

  // Put the edges in the vertex edge lists.
  for (unsigned long i = 0; i < nVertices; ++i)
  {
    for (unsigned long j = 0; j < rhs.vertices[i]->edges.size(); ++j)
    {
      vertices[i]->edges[j] = edges[rhs.vertices[i]->edges[j]->id];
    }
  }
}

}

#endif

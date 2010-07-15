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
/*! \file graph.hpp

    \brief This file defines the interface to graphs, vertices, and edges.
           Notice that most of the complexity associated with graph algorithms
           (for example, "expandOneedge", filters, etc.) is handled at the
           visitor and algorithm design levels.  This leaves a simple graph
           level that can easily be modified.  There is only one type of graph
           in this prototype infrastructure: a directed graph object with
           edges and vertices.

    \author Jon Berry (jberry@sandia.gov)
    \author Greg Mackey (gemacke@sandia.gov)

    \date 3/2/2005

    \deprecated This file is deprecated in favor of adjacency_list.hpp.
*/
/****************************************************************************/

#ifndef MTGL_GRAPH_HPP
#define MTGL_GRAPH_HPP

#include <mtgl/dynamic_array.hpp>
#include <mtgl/graph_traits.hpp>

namespace mtgl {

class edge;

/****************************************************************************/
/*! \brief A vertex object contains an edge set, and this set is represented
           at the moment by a simple dynamic array.  The edge set is public
           for the moment.  Many such structures are public in this prototype
           to reduce the clutter of accessor methods.
*/
/****************************************************************************/
class vertex {
public:
  typedef unsigned long count_t;

  vertex(count_t idd = INT_MAX) : id(idd), num_edges_to_add(0), edges(0) {}
  vertex(count_t idd, count_t init_degree) : id(idd), num_edges_to_add(0),
                                             edges(init_degree) {}
  ~vertex() {}

  void addedge(edge* e) { edges.push_back(e); }
  void unsafeAddEdge(edge* e) { edges.unsafe_push_back(e); }

  template <typename vistype>
  void visit(vistype& fobj)
  {
    fobj(this);
  }

  count_t get_id() const { return id; }
  count_t degree() const { return edges.size(); }
  void set_id(count_t i) { id = i; }

  inline dynamic_array<count_t>* neighborIDs() const;
  inline void printNeighbors() const;

public:
  count_t id;
  unsigned long num_edges_to_add;
  dynamic_array<edge*> edges;

  /*! \fn vertex(int idd = INT_MAX)
      \brief This constructor initializes the id and edge set.  The default
             size of the edge set is defined in dynamic_array.h, and at the
             time of this writing (6/21/05) is 5.
      \param idd The vertex id.
  */
  /*! \fn vertex(int idd = INT_MAX, int init_degree)
      \brief This constructor initializes the id and edge set along with an
             initial size.
      \param idd The vertex id.
      \param init_degree The initial size of the edge set.
  */
  /*! \fn void addedge(edge* e)
      \brief edge e has already been constructed; add it to this vertex's
             edge set.
      \param e A pointer to the constructed edge object.
  */
  /*! \fn void unsafeAddEdge(edge* e)
      \brief edge e has already been constructed; add it to this vertex's
             edge set.
      \param e A pointer to the constructed edge object.

      The edge sets of the endpoints of this edge have been pre-sized using
      dynamic_array::buildIncr calls, so they will not need reallocation.  The
      dynamic_array::unsafe_push_back method avoids locking the entire array
      during insertions.
  */
  /*! \fn template <typename vistype> void visit(vistype fobj)
      \brief Apply the method encapsulated within fobj to this vertex.
      \param fobj A closure object encapsulating a method that expects two
                  arguments: a vertex pointer and an edge pointer.
  */
  /*! \fn void degree()
      \brief Return the number of edges in the edge set.  Note that since
             every endpoint has access to every edge object that touches it,
             regardless of direction, the result will not be the sum of the
             in-degree and out-degree.  These individual methods will be added
             as needed.
  */
  /*! \fn void printNeighbors() const
      \brief A simple debugging routine.
  */
  /*! \var id The vertex id.  Not public to save the accessor method; will
              become protected as needed.
  */
};

/****************************************************************************/
/*! \brief An edge object stores pointers to its endpoints.  Both endpoints
           will have access to this object, even if it represents a directed
           edge from one to the other.
*/
/****************************************************************************/
class edge {
public:
  typedef unsigned long count_t;

  edge(vertex* v1 = 0, vertex* v2 = 0, count_t idd = INT_MAX) :
    vert1(v1), vert2(v2), id(idd) {}
  ~edge() {}

  vertex* other(const vertex* v) const { return v == vert1 ? vert2 : vert1; }
  vertex* other(count_t vid) const { return vid == vert1->id ? vert2 : vert1; }

  count_t get_id() const { return id; }
  vertex* get_vert1() const { return vert1; }
  vertex* get_vert2() const { return vert2; }

public:
  vertex* vert1;
  vertex* vert2;
  count_t id;

  /*! \fn edge(vertex* v1 = 0, vertex* v2 = 0, int idd = INT_MAX)
      \brief This constructor initializes member data.
      \param v1 The source vertex.
      \param v2 The sink vertex.
      \param idd The vertex id.
  */
  /*! \fn other(const vertex *v)
      \brief Given a vertex pointer (one of the endpoints), return the
             other endpoint.
      \param v One of the edge's endpoints.
  */
  /*! \var vert1
      \brief The source vertex.
  */
  /*! \var vert2
      \brief The destination vertex.
  */
  /*! \var id
      \brief The edge id.
  */
};

/****************************************************************************/
/*! \brief A simple directed graph class.

    \deprecated This class is deprecated in favor of adjacency_list.
*/
/****************************************************************************/
template <typename DIRECTION = directedS>
class graph {
public:
  typedef unsigned long count_t;

  graph() : order(0), size(0), edges(0), bigend(0) {}
  graph(const graph& g) : bigend(0) { deep_copy(g); }

  ~graph()
  {
    if (bigend) free(bigend);

    #pragma mta assert parallel
    for (int i = 0; i < vertices.size(); ++i) delete vertices[i];

    #pragma mta assert parallel
    for (int i = 0; i < edges.size(); ++i) delete edges[i];
  }

  graph& operator=(const graph& rhs);

  void clear();

  vertex** get_vertices() { return vertices.get_data(); }
  edge** get_edges() { return edges.get_data(); }
  edge* get_edge(count_t i) { return edges[i]; }
  vertex* get_vertex(count_t i) { return vertices[i]; }

  void addvertex(count_t id, count_t init_degree = 5)
  {
    vertices.push_back(new vertex(id, init_degree));
    mt_incr(order, 1);
  }

  vertex** addVertices(count_t lower, count_t upper, count_t init_degree = 5);

  void init(count_t n_verts, count_t n_edges, count_t* srcs,
                   count_t* dests);
  void init(count_t n_verts, count_t n_edges, vertex** srcs,
                   vertex** dests);

  edge* addedge(vertex* v1, vertex* v2, count_t idd);

  edge* addedge(count_t v1, count_t v2, count_t idd)
  {
    return addedge(getvertex(v1), getvertex(v2), idd);
  }

  edge** addedges(count_t* heads, count_t* tails, count_t num_to_add);

  edge** addedges(vertex** heads, vertex** tails, count_t num_to_add);

  edge** add_edges(count_t num_to_add, vertex** heads, vertex** tails)
  {
    return addedges(heads, tails, num_to_add);
  }

  void addedges(edge** edges, count_t num);

  edge** begin_incident_edges(count_t i) const
  {
    return vertices[i]->edges.get_data();
  }

  edge** end_incident_edges(count_t i) const
  {
    dynamic_array<edge*>& egs = vertices[i]->edges;
    return egs.get_data() + egs.size();
  }

  count_t getOrder() const { return order; }
  count_t getSize() const { return size; }
  count_t get_order() const { return order; }
  count_t get_size() const { return size; }
  edge* get_edge(count_t i) const { return edges[i]; }
  bool* get_bigend() const { return bigend; }

  bool is_directed() const { return DIRECTION::is_directed(); }
  bool is_undirected() const { return !is_directed(); }

  inline vertex* getvertex(count_t id) const;
  void updateBigend();
  void print() const;

private:
  void deep_copy(const graph& rhs);

public:
  unsigned long order;  /* order = num verts */
  unsigned long size;   /* size  = num edges */
  dynamic_array<vertex*> vertices;
  dynamic_array<edge*> edges;
  bool* bigend;

  /*! \fn graph()
      \brief Initialize the order (number of vertices) and size (number of
             edges) to 0.
  */
  /*! \fn graph(const graph& g)
      \brief Copy graph g.  The vertex and edge sets are dynamic_array
             objects, and dynamic_array has a copy constructor.  So, this is
             a deep copy.
  */
  /*! \fn void clear()
      \brief Call the "clear" method of the vertex and edge sets, and reset
             the order and size to 0.
  */
  /*! \fn void addvertex(int id, int init_degree = 5)
      \brief Allocate a new vertex object and insert it into the vertex array.
             Note that this is very inefficient in a multithreaded
             environment.  Block allocations of space without constructor
             calls are currently necessary in that environment.
      \param id The vertex id.
      \param init_degree The initial edge set size for the vertex.
  */
  /*! \fn vertex** addVertices(int lower, int upper, int init_degree = 5)
      \brief Allocate vertex objects with (lower <= id <= upper) and insert
             them into the vertex array.  This is still done inefficiently in
             a multithreaded environment because individual vertex object
             constructor calls are made. It could be improved.
      \param lower The lowest new id to add.
      \param upper The highest new id to add.
      \param init_degree The initial edge set size for the vertices.
  */
  /*! \fn edge* addedge(int v1, int v2, int idd)
      \brief Constructs vertex objects and an edge object and inserts it in
             the edge sets of v1 and v2 (even though the true direction is
             v1->v2.  This is confusing to the user now; the interface should
             be improved.  Right now, addedge must be called only once per
             adjacency (in the undirected sense), and types must be changed
             to represent directionality.
      \param v1 The source vertex of this directed edge.
      \param v2 The sink vertex of this directed edge.
      \param idd The edge id.

      This method currently does not ensure that only one edge object per
      undirected adjacency exists.  This has to be fixed.
  */
  /*! \fn edge* addedge(vertex* v1, vertex* v2, int idd)
      \brief Accepts vertex objects and an edge object and inserts it in the
             edge sets of v1 and v2 (even though the true direction is
             v1->v2.  This is confusing to the user now; the interface should
             be improved.  Right now, addedge must be called only once per
             adjacency (in the undirected sense), and types must be changed
             to represent directionality.
      \param v1 The source vertex of this directed edge.
      \param v2 The sink vertex of this directed edge.
      \param idd The edge id.

      This method currently does not ensure that only one edge object per
      undirected adjacency exists.  This has to be fixed.
  */
  /*! \fn edge** addedges(vertex** heads, vertex** tails,
                          unsigned int num_to_add)
      \brief Given an array of edge heads and tails, add them to the graph
             in parallel.
      \param heads An array of edge sources.
      \param tails An array of edge sinks.
      \param num_to_add The cardinality of each array.

      Note that the typical graph theory meaning of "head" and "tail" is in
      the sense of an arrow: the "head" is the point, and the "tail" is the
      set of feathers.  However, the computer science linked-list sense of
      "head" and "tail" reverses the meanings.  We named the variables here
      with the latter interpretation in mind.

      This method currently does not ensure that only one edge object per
      undirected adjacency exists.  This has to be fixed.
  */
  /*! \fn void edge** addedges(edge** edges, unsigned int num_to_add)
      \brief Given an array of edge objects, add them to the graph in
             parallel.
      \param edges An array of edge objects.
      \param num_to_add The cardinality of the array.

      This method currently does not ensure that only one edge object per
      undirected adjacency exists.  This has to be fixed.
  */
  /*! \fn vertex* getvertex(int id) const
      \brief Given an id, return a pointer to the vertex object with that id.
      \param id The index into the graph's vertex array.
  */
  /*! \fn void print(ostream& os) const
      \brief A simple debugging method.
  */
  /*! \var order
      \brief The number of vertices in the graph.
  */
  /*! \var size
      \brief The number of edge objects in the edge array of the graph. In
             other words, the number of adjacencies in the undirected sense.
  */
  /*! \var dynamic_array<vertex*, edge*> vertices
      \brief The vertex array.  The second template parameter indicates the
             expected type of environment data to be included with closures
             that visit each vertex in the vertex array.
  */
  /*! \var dynamic_array<vertex*, edge*> edges
      \brief The edge array.  The second template parameter indicates the
             expected type of environment data to be included with closures
             that visit each edge in the edge array.
  */
};

template <>
class graph<bidirectionalS> {
public:
  graph()
  {
    fprintf(stderr, "** Error: Bidirectional direction not yet supported "
            "for graph. **\n");
    exit(1);
  }
};

dynamic_array<unsigned long>*
vertex::neighborIDs() const
{
  count_t deg = edges.size();
  dynamic_array<count_t>* result = new dynamic_array<count_t>(deg);

  if (deg > 100)
  {
    #pragma mta assert parallel
    #pragma mta loop future
    for (count_t i = 0; i < deg; ++i)
    {
      edge* e = edges[i];
      result->push_back(e->other(this)->id);
    }
  }
  else
  {
    for (count_t i = 0; i < deg; ++i)
    {
      edge* e = edges[i];
      result->unsafe_push_back(e->other(this)->id);
    }
  }

  return result;
}

void
vertex::printNeighbors() const
{
  printf("%lu: ", id);

  for (count_t i = 0; i < edges.size(); ++i)
  {
    edge* e = edges[i];
    printf("%lu ", e->other(this)->id);
  }

  printf("\n");
}

template <typename DIRECTION>
graph<DIRECTION>&
graph<DIRECTION>::operator=(const graph& rhs)
{
  if (bigend)
  {
    free(bigend);
    bigend = 0;
  }

  // Deallocate memory for the currently held vertices and edges.
  #pragma mta assert parallel
  for (int i = 0; i < vertices.size(); ++i) delete vertices[i];

  #pragma mta assert parallel
  for (int i = 0; i < edges.size(); ++i) delete edges[i];

  deep_copy(rhs);

  return *this;
}

template <typename DIRECTION>
void
graph<DIRECTION>::clear()
{
  if (bigend)
  {
    free(bigend);
    bigend = 0;
  }

  order = 0;
  size = 0;

  // Deallocate memory for the currently held vertices and edges.
  #pragma mta assert parallel
  for (int i = 0; i < vertices.size(); ++i) delete vertices[i];

  #pragma mta assert parallel
  for (int i = 0; i < edges.size(); ++i) delete edges[i];

  vertices.clear();
  edges.clear();
}

template <typename DIRECTION>
void
graph<DIRECTION>::print() const
{
  printf("[");

  count_t i;

  for (i = 0; i + 1 < order; ++i) printf("%lu ", vertices[i]->id);

  printf("%lu]\n", vertices[i]->id);

  for (i = 0; i < size; ++i)
  {
    printf("{%lu: %lu, %lu}\n", edges[i]->id, edges[i]->vert1->id,
           edges[i]->vert2->id);
  }
}

template <typename DIRECTION>
vertex**
graph<DIRECTION>::addVertices(count_t lower, count_t upper, count_t init_degree)
{
  // Doesn't check for duplicates yet.
  count_t num_new = upper - lower + 1;

  vertex** new_vertptrs = (vertex**) malloc(num_new * sizeof(vertex*));

  #pragma mta assert nodep
  for (count_t i = 0; i < num_new; ++i)
  {
    new_vertptrs[i] = new vertex(lower + i);
  }

  vertices.push_back(new_vertptrs, num_new);
  mt_incr(order, num_new);

  return new_vertptrs;
}

typedef vertex* vertexPtr;

inline
int
vcmp(const vertexPtr& v1, const vertexPtr& v2)
{
  return v1->id - v2->id;
}

template <typename DIRECTION>
vertex*
graph<DIRECTION>::getvertex(count_t id) const
{
  vertex dummy(id, 1);

  unsigned long index = 0;
  for ( ; index < order && id != vertices[index]->id ; ++index);

  return index < order ? vertices[index] : 0;
}

// Precondition: v1 and v2 have been added.
template <typename DIRECTION>
edge*
graph<DIRECTION>::addedge(vertex* v1, vertex* v2, count_t idd)
{
  (void) mt_incr(size, 1);

  edge* e = new edge(v1, v2, idd);

  v1->addedge(e);
  if (is_undirected()) v2->addedge(e);
  edges.push_back(e);

  return e;
}

template <typename DIRECTION>
void
graph<DIRECTION>::init(count_t n_verts, count_t n_edges, count_t* srcs,
                       count_t* dests)
{
  vertex** vertices = addVertices(0, n_verts - 1);

  edge** new_edgeptrs = (edge**) malloc(sizeof(edge*) * n_edges);

  #pragma mta assert parallel
  for (count_t i = 0; i < n_edges; ++i)
  {
    new_edgeptrs[i] = new edge(vertices[srcs[i]], vertices[dests[i]], i);
  }

  addedges(new_edgeptrs, n_edges);

  free(new_edgeptrs);
  free(vertices);
}

template <typename DIRECTION>
void
graph<DIRECTION>::init(count_t n_verts, count_t n_edges, vertex** srcs,
                       vertex** dests)
{
  edge** new_edgeptrs = (edge**) malloc(sizeof(edge*) * n_edges);

  #pragma mta assert parallel
  for (count_t i = 0; i < n_edges; ++i)
  {
    new_edgeptrs[i] = new edge(srcs[i], dests[i], i);
  }

  addedges(new_edgeptrs, n_edges);

  free(new_edgeptrs);
}

template <typename DIRECTION>
edge**
graph<DIRECTION>::addedges(vertex** heads, vertex** tails, count_t num_to_add)
{
  edge** new_edgeptrs = (edge**) malloc(sizeof(edge*) * num_to_add);
  for (count_t i = 0; i < num_to_add; ++i)
  {
    new_edgeptrs[i] = new edge(heads[i], tails[i], i + size);
  }

  #pragma mta assert parallel
  for (count_t i = 0; i < num_to_add; ++i)
  {
    mt_incr(heads[i]->num_edges_to_add, 1);
    if (is_undirected()) mt_incr(tails[i]->num_edges_to_add, 1);
  }

  #pragma mta assert parallel
  for (count_t i = 0; i < order; ++i)
  {
    vertices[i]->edges.reserve(vertices[i]->edges.size() +
                               vertices[i]->num_edges_to_add);
    vertices[i]->num_edges_to_add = 0;
  }

  #pragma mta assert nodep
  for (count_t i = 0; i < num_to_add; ++i)
  {
    heads[i]->unsafeAddEdge(new_edgeptrs[i]);
    if (is_undirected()) tails[i]->unsafeAddEdge(new_edgeptrs[i]);
  }

  edges.push_back(new_edgeptrs, num_to_add);
  size += num_to_add;

  return new_edgeptrs;
}

template <typename DIRECTION>
edge**
graph<DIRECTION>::addedges(count_t* heads, count_t* tails, count_t num_to_add)
{
  edge** new_edgeptrs = (edge**) malloc(sizeof(edge*) * num_to_add);
  for (count_t i = 0; i < num_to_add; ++i)
  {
    new_edgeptrs[i] = new edge(vertices[heads[i]], vertices[tails[i]],
                               i + size);
  }

  #pragma mta assert parallel
  for (count_t i = 0; i < num_to_add; ++i)
  {
    mt_incr(vertices[heads[i]]->num_edges_to_add, 1);
    if (is_undirected()) mt_incr(vertices[tails[i]]->num_edges_to_add, 1);
  }

  #pragma mta assert parallel
  for (count_t i = 0; i < order; ++i)
  {
    vertices[i]->edges.reserve(vertices[i]->edges.size() +
                               vertices[i]->num_edges_to_add);
    vertices[i]->num_edges_to_add = 0;
  }

  #pragma mta assert nodep
  for (count_t i = 0; i < num_to_add; ++i)
  {
    vertices[heads[i]]->unsafeAddEdge(new_edgeptrs[i]);
    if (is_undirected()) vertices[tails[i]]->unsafeAddEdge(new_edgeptrs[i]);
  }

  edges.push_back(new_edgeptrs, num_to_add);
  size += num_to_add;

  return new_edgeptrs;
}

template <typename DIRECTION>
void
graph<DIRECTION>::addedges(edge** new_edges, count_t num_to_add)
{
  #pragma mta assert parallel
  for (count_t i = 0; i < num_to_add; ++i)
  {
    mt_incr(new_edges[i]->vert1->num_edges_to_add, 1);
    if (is_undirected()) mt_incr(new_edges[i]->vert2->num_edges_to_add, 1);
  }

  #pragma mta assert parallel
  for (count_t i = 0; i < order; ++i)
  {
    vertices[i]->edges.reserve(vertices[i]->edges.size() +
                               vertices[i]->num_edges_to_add);
    vertices[i]->num_edges_to_add = 0;
  }

  #pragma mta assert parallel
  for (count_t i = 0; i < num_to_add; ++i)
  {
    new_edges[i]->vert1->unsafeAddEdge(new_edges[i]);
    if (is_undirected()) new_edges[i]->vert2->unsafeAddEdge(new_edges[i]);
  }

  edges.push_back(new_edges, num_to_add);
  size += num_to_add;
}

template <typename DIRECTION>
void
graph<DIRECTION>::updateBigend()
{
  if (bigend) free(bigend);
  bigend = (bool*) malloc(sizeof(bool) * edges.size());

  count_t size = edges.size();

  #pragma mta assert nodep
  for (count_t i = 0; i < size; ++i)
  {
    bigend[i] = edges[i]->vert1->degree() > edges[i]->vert2->degree();
  }
}

template <typename DIRECTION>
void
graph<DIRECTION>::deep_copy(const graph& rhs)
{
  if (rhs.bigend)
  {
    bigend = (bool*) malloc(sizeof(bool) * size);
    for (count_t i = 0; i < size; ++i) bigend[i] = rhs.bigend[i];
  }

  // Sets the number of vertices and edges and the edge direction.
  order = rhs.order;
  size = rhs.size;

  vertices.resize(order);
  edges.resize(size);

  // Add the vertices.
  #pragma mta assert parallel
  for (unsigned long i = 0; i < order; ++i)
  {
    vertices[i] = new vertex(i);

    // Set the initial size of the vertices edge lists to the number of edges
    // they will contain.
    vertices[i]->edges.resize(rhs.vertices[i]->edges.size());
  }

  // Add the edges.
  #pragma mta assert parallel
  for (unsigned long i = 0; i < size; ++i)
  {
    edges[i] = new edge(vertices[rhs.edges[i]->get_vert1()->get_id()],
                        vertices[rhs.edges[i]->get_vert2()->get_id()], i);
  }

  // Put the edges in the vertex edge lists.
  for (unsigned long i = 0; i < order; ++i)
  {
    for (int j = 0; j < rhs.vertices[i]->edges.size(); ++j)
    {
      vertices[i]->edges[j] = edges[rhs.vertices[i]->edges[j]->get_id()];
    }
  }
}

struct link_graph_printer {
  typedef unsigned long count_t;

  link_graph_printer(FILE* fpp, dynamic_array<edge*>& edgs) :
    edges(edgs), fp(fpp) {}

  void getvertexNames()
  {
    for (count_t i = 0; i < edges.size(); ++i)
    {
      vertex* v1 = edges[i]->vert1;
      vertex* v2 = edges[i]->vert2;
      bool found_v1=false;
      bool found_v2=false;

      for (count_t j = 0; j < vertices.size(); ++j)
      {
        if (vertices[j] == v1) found_v1 = true;
        if (vertices[j] == v2) found_v2 = true;
      }

      if (!found_v1) vertices.push_back(v1);
      if (!found_v2) vertices.push_back(v2);
    }
  }

  void print()
  {
    getvertexNames();

    fprintf(fp, "{ graph UBingraph\n");
    fprintf(fp, "( \n");
    fprintf(fp, " aux_label(\"\") \n");
    fprintf(fp, ")\n");
    fprintf(fp, "Vertices [\n");

    for (count_t i = 0; i < vertices.size(); ++i)
    {
      vertex* v = vertices[i];
      fprintf(fp, "  %lu ( aux_label(\"%lu\") )\n", v->id, v->id);
    }

    fprintf(fp, "]\n");
    fprintf(fp, "edges {\n");

    for (count_t i = 0; i < edges.size(); ++i)
    {
      edge* e = edges[i];
      vertex* v1 = e->vert1;
      vertex* v2 = e->vert2;

      fprintf(fp, "{ %lu %lu } (name(\"%lu\")", v1->id, v2->id, e->id);
      fprintf(fp, "aux_label(\"%lu\") )\n", e->id);
    }

    fprintf(fp, "}}\n");
  }

  dynamic_array<edge*>& edges;
  dynamic_array<vertex*> vertices;
  FILE* fp;
};

template <typename T, typename DIRECTION>
void
separateByDegree(graph<DIRECTION>* g, T* indicesOfBigs, T& numBig,
                 T* indicesOfSmalls, T& numSmall, T deg_threshold = 100)
{
  T ord = g->order;
  numSmall = 0;
  numBig = 0;

  vertex** vertices = g->vertices.get_data();

  #pragma mta assert parallel
  for (T i = 0; i < ord; ++i)
  {
    if (vertices[i]->degree() > deg_threshold)
    {
      T mine = mt_incr(numBig, 1);
      indicesOfBigs[mine] = i;
    }
    else
    {
      T mine = mt_incr(numSmall, 1);
      indicesOfSmalls[mine] = i;
    }
  }
}

}

#endif

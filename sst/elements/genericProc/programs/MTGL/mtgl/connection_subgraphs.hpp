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
/*! \file connection_subgraphs.hpp

    \brief This source file contains the connection subgraphs algorithm
           from Falutsos, McCurley, Tomkins (KDD'04).

    \author William McLendon (wcmclen@sandia.gov)

    \date 1/5/2006
    \date 8/27/2007
*/
/****************************************************************************/

#ifndef MTGL_CONNECTION_SUBGRAPHS_HPP
#define MTGL_CONNECTION_SUBGRAPHS_HPP

#include <cstdio>
#include <climits>
#include <cassert>

#include <mtgl/mtgl_adapter.hpp>
#include <mtgl/graph_traits.hpp>
#include <mtgl/util.hpp>
#include <mtgl/dynamic_array.hpp>
#include <mtgl/st_search.hpp>
#include <mtgl/k_distance_neighborhood.hpp>
#include <mtgl/induced_subgraph.hpp>
#include <mtgl/st_connectivity.hpp>
#include <mtgl/SMVkernel.h>
#include <mtgl/array2d.hpp>
#include <mtgl/quicksort.hpp>

#ifdef __MTA__
#include <sys/mta_task.h>
#include <machine/runtime.h>
#endif

#define CSG_DEBUG 0

#define TIMER_RESTART(timer)  \
{                             \
  timer.stop();               \
  timer.start();              \
}

#define TIMER_SECT(timer, label, cond)         \
{                                              \
  if (cond)                                    \
  {                                            \
    timer.stop();                              \
    printf("TIME: %35s = %7.4lf\n",            \
           label, timer.getElapsedSeconds());  \
    timer.start();                             \
  }                                            \
}

#define DEBUG_BREAK()  { printf("..."); getc(stdin); }

#ifdef __MTA__
  #define BEGIN_LOGGING(S)\
    printf("%s\n", S); fflush(stdout);\
    mt_timer __mttimer;\
    mta_resume_event_logging();\
    int issues      = mta_get_task_counter(RT_ISSUES);\
    int streams     = mta_get_task_counter(RT_STREAMS);\
    int concurrency = mta_get_task_counter(RT_CONCURRENCY);\
    int traps       = mta_get_task_counter(RT_TRAP);\
    __mttimer.start();

  #define END_LOGGING()                                                \
  {                                                                    \
    __mttimer.stop();                                                  \
    issues      = mta_get_task_counter(RT_ISSUES) - issues;            \
    streams     = mta_get_task_counter(RT_STREAMS) - streams;          \
    concurrency = mta_get_task_counter(RT_CONCURRENCY) - concurrency;  \
    traps       = mta_get_task_counter(RT_TRAP) - traps;               \
    mta_suspend_event_logging();                                       \
    printf("issues: %d, streams: %lf, concurrency: %lf, traps: %d "    \
           "time: %.3f\n", issues, streams / (double) issues,          \
           concurrency / (double) issues, traps,                       \
           __mttimer.getElapsedSeconds());                             \
  }
#else
  #define BEGIN_LOGGING() ;
  #define END_LOGGING()   ;
#endif

namespace mtgl {

const double CSG_ALPHA  = 1.0;
const double CSG_VOLT_S = 1.0;
const double CSG_VOLT_T = 0.0;

const int VERT_UNKNOWN  = -1;

template <typename graph_adapter> class connection_subgraph;

// -----[ class csg_wrapper ] ---------------------------------------------
/// \brief Experimental code!  Testing a wrapper for CSG to combine graph
///        preprocessing with CSG to improve performance and result quality.
template <typename graph_adapter>
class csg_wrapper {
public:
  typedef typename graph_traits<graph_adapter>::size_type size_type;
  csg_wrapper(graph_adapter& graph,
              size_type sourceVertexId,
              size_type targetVertexId,
              dynamic_array<size_type>& vertexIdList,
              size_type distPastShortPath,
              size_type numExpansions,
              double penaltyMultiplier) :
    _graph(graph),
    _sourceVertexId(sourceVertexId),
    _targetVertexId(targetVertexId),
    _vertexIdList(vertexIdList),
    _distPastShortPath(distPastShortPath),
    _numExpansions(numExpansions),
    _penaltyMultiplier(penaltyMultiplier)
  {
    _changeSolverTol = false;
  }

  ~csg_wrapper() {}

  void run()
  {
    // (1) extract ST graph.
    dynamic_array<size_type> shortPathIds;
    st_search<graph_adapter, UNDIRECTED>
    sts(_graph, _sourceVertexId, _targetVertexId, shortPathIds);
    sts.run(VERTICES);

    shortPathIds.print();

    // (2) extract k-distance neighborhood of the small graph.
    dynamic_array<size_type> neighborhoodIds;
    k_distance_neighborhood<graph_adapter, UNDIRECTED>
    kdn(_graph, _distPastShortPath, neighborhoodIds);
    kdn.run(shortPathIds);

    neighborhoodIds.print();

    // (3) extract neighborhood as a subgraph from G.
    bool* vmask = (bool*) malloc(sizeof(bool) * num_vertices(_graph));

    #pragma mta assert no dependence
    for (size_type i = 0; i < num_vertices(_graph); i++) vmask[i] = false;

    graph_adapter neighborhoodGraph;
    size_type* svid_to_gvid;
    subgraph_generator<graph_adapter, VMASK>
    subgen(neighborhoodGraph, _graph, vmask, svid_to_gvid);
    subgen.run();

    // !wcm looks like subgraph generator is currently broken under
    //      the mtgl-boost framework.  This really needs to get updated
    //      to work with MTGL-BOOST framework -and- to work with a vertex
    //      mask provided.

    // free up some temp memory.
    free(vmask);  vmask = NULL;
    neighborhoodIds.clear();
    shortPathIds.clear();

    printf("size of small graph = %d\n", num_vertices(*neighborhoodGraph));

    // (4) Compute CSG on subgraph.
    connection_subgraph<graph_adapter>
    csg_search(_graph, _sourceVertexId, _targetVertexId,
               _vertexIdList, _distPastShortPath, _numExpansions,
               _penaltyMultiplier);
    if (_changeSolverTol)
    {
      csg_search.setSolverTolerance(_changeSolverTol);
    }

    csg_search.run();
  }

  void setSolverTolerance(double newSolverTolerance)
  {
    _changeSolverTol = true;
    _solverTolerance = newSolverTolerance;
  }

private:
  graph_adapter& _graph;
  size_type _sourceVertexId, _targetVertexId;
  size_type _numExpansions, _distPastShortPath;
  double _penaltyMultiplier;
  double _solverTolerance;
  bool _changeSolverTol;
  dynamic_array<size_type>& _vertexIdList;
};

// -----[ class connection_subgraph ]--------------------------------------
/*! \brief Implements the connection subgraphs algorithm which is based on
           the paper by Faloutsos, et. al.  KDD'04.

    For now, CSG treats edges as undirected edges.  Needs a bit of a revamp
    to consider direction with edges correctly.
*/
template <typename graph_adapter>
class connection_subgraph {
public:
  typedef typename graph_traits<graph_adapter>::size_type size_type;
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_t;
  typedef typename graph_traits<graph_adapter>::out_edge_iterator EITER_T;
  typedef typename graph_traits<graph_adapter>::vertex_iterator vertex_iterator;

  // -----[ connection_subgraph() ]-------------------------------------
  /*! \brief This is the constructor for the connection subgraphs routine.
             It will call st_connectivity to determine the path length from
             vs to vt (if one exists) inside the constructor.
             The parameter distPastShortPath is added to the short-path
             length to provide a cap to the maximum length that CSG will
             consider in building a connection-subgraph.

    \param g   (IN) A pointer to a graph.
    \param sourceVertexId     (IN)  The index # of the source vertex.
    \param targetVertexId     (IN)  The index # of the target vertex.
    \param vertexIdList       (OUT) List of vertices included in the CSG.
    \param distPastShortPath  (IN)  Appended (must be >0) to the length of
                                    the shortest path possible between
                                    vs and vt.
    \param numExpansions      (IN)  Number of times to expand the output
                                    graph.  Setting this to one would mean
                                    we include only the simplest path, each
                                    subsequent iteration adds the next-best
                                    path to the output graph.
    \param penaltyMultiplier  (IN)  Adjusts the penalty factor against a
                                    node.  Allowable range is 0 .. 5.0.
                                    [OPTIONAL: default=1.0]
  */
  connection_subgraph(graph_adapter&  _ga,
                      size_type sourceVertexId,
                      size_type targetVertexId,
                      dynamic_array<size_type>& _vertexIdList,
                      size_type _distPastShortPath,
                      size_type _numExpansions,
                      double penaltyMultiplier = 1.0) :
    ga(_ga), vs(sourceVertexId), vt(targetVertexId),
    vertexIdList(_vertexIdList),
    numExpansions(_numExpansions),
    alpha(penaltyMultiplier)
  {
    vs_and_vt_connected = true;
    node_voltages = (size_type)0;
    edge_currents = (size_type)0;
    nverts = num_vertices(ga);
    if (numExpansions > nverts) { numExpansions = nverts; }

    // set default solver tolerance
    solverTolerance = 1.0E-16;

    if (vs >= nverts || vs < 0)
    {
      fprintf(stderr,"ERROR: source out-of-bounds in connection subgraphs!\n");
#if CSG_DEBUG
      fprintf(stderr, "     source vertex, %d, is out of bounds!\n",
              vs);
      fprintf(stderr, "     There are only %d vertices!\n", nverts);
#endif
      exit(1);
    }
    else if (vt >= nverts || vt < 0)
    {
      fprintf(stderr, "ERROR target out-of-bounds connection subgraphs!\n");
#if CSG_DEBUG
      fprintf(stderr, "     target vertex, %d, is out of bounds!\n", vt);
      fprintf(stderr, "     There are only %d vertices!\n", nverts);
      exit(1);
#endif
    }

    // determine the length of the shortest path via st_connectivity
    maxPathLength = st_path_length(ga, vs, vt);

    if (maxPathLength >= 0)
    {
      _distPastShortPath = _distPastShortPath > 0 ? _distPastShortPath : 0;
      _distPastShortPath = _distPastShortPath > nverts ? nverts : _distPastShortPath;
      mt_incr(maxPathLength, _distPastShortPath);
    }

    maxSolverIters = 50;

    if (alpha < 0.0) alpha = 0.0;
    if (alpha > 5.0) alpha = 5.0;
  }

  // -----[ connection_subgraph() ]-------------------------------------

  /// Default destructor.
  ~connection_subgraph() {}

  // -----[ run() ]-----------------------------------------------------
  /*! \brief Executes the connection subgraphs search.

      Returns -1 if no path exists between vs and vt; 0 otherwise.
   */
  size_type run()
  {
    vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, ga);

    // if graph vs and vt are not in the same component...
    if (maxPathLength < 0)
    {
      fprintf(stderr, "csg:: Vs and Vt are not connected\n");
      return((size_type)-1);
    }

    csg_vertex_mask = (bool*) malloc(sizeof(bool) * nverts);

    #pragma mta assert no dependence
    for (size_type i = 0; i < nverts; i++)
    {
      csg_vertex_mask[i] = false;
    }

    bool vs_vt_identical = vs == vt ? true : false;
    bool vs_vt_adjacent  = test_vertex_adjacency(ga, vs, vt, vid_map);

    if (!vs_vt_identical && !vs_vt_adjacent)
    {
      node_voltages = (double*) malloc(sizeof(double) * nverts);

      compute_node_voltages(vid_map);

#if CSG_DEBUG
      // print out node voltages
      for (size_type vid = 0; vid < num_vertices(ga); vid++)
      {
        printf("volts[%2d] == %8.4f\n", vid, node_voltages[vid]);
      }
#endif

      generate_display_graph(vid_map);

      free(node_voltages);
      node_voltages = NULL;
    }

    // add vertices into dynamic_array<size_type> result.
    size_type rCount = 0;

    #pragma mta assert parallel
    for (size_type vid = 0; vid < num_vertices(ga); vid++)
    {
      if (csg_vertex_mask[vid] == 1)
      {
        mt_incr(rCount, 1);
      }
    }

    vertexIdList.reserve(rCount);   // should this be resize?

    #pragma mta assert parallel
    for (size_type vid = 0; vid < num_vertices(ga); vid++)
    {
      if (csg_vertex_mask[vid] == 1)
      {
        vertexIdList.push_back(vid);
      }
    }

    free(csg_vertex_mask);
    csg_vertex_mask = NULL;

    return((size_type)0);
  }

  // -----[ run() ]-----------------------------------------------------

  // -----[ setSolverTolerance() ]--------------------------------------
  void setSolverTolerance(double newSolverTolerance)
  {
    solverTolerance = newSolverTolerance;
  }

  // -----[ setSolverTolerance() ]--------------------------------------

  // -------------------------------------------------------------------
  //      ####  ####  ####  #####
  //      #  #  #  #  #  #    #
  //      ####  ####  #  #    #
  //      #     # #   #  #    #
  //      #     #  #  ####    #
  // -------------------------------------------------------------------

protected:
  // -----[ get_neighbors() ]-------------------------------------------
  void get_neighbors(vertex_t& v, dynamic_array<size_type>& neighbor_ids)
  {
    vertex_id_map<graph_adapter> vipm = get(_vertex_id_map, ga);

    size_type deg = out_degree(v, ga);
    EITER_T oedges = out_edges(v, ga);

    for (size_type i = 0; i < deg; i++)
    {
      edge_t e = oedges[i];
      size_type v2id = get(vipm, target(e, ga));
      neighbor_ids.push_back(v2id);
    }
  }

  // -----[ get_neighbors() ]-------------------------------------------

  // -----[ st_path_length() ]------------------------------------------
  #pragma mta inline
  size_type st_path_length(graph_adapter& ga, size_type vs, size_type vt)
  {
    size_type dist = 0;
    size_type numVisited = 0;
    st_connectivity<graph_adapter, UNDIRECTED>
    st_conn(ga, vs, vt, dist, numVisited);
    st_conn.run();

    return(dist);
  }

  // -----[ st_path_length() ]------------------------------------------

  // -----[ test_vertex_adjacency() ]-----------------------------------
  bool test_vertex_adjacency(graph_adapter ga, size_type v1id, size_type v2id,
                             vertex_id_map<graph_adapter>& vid_map)
  {
    bool ret = false;

    vertex_t v1 = get_vertex(v1id, ga);
    size_type deg = out_degree(v1, ga);
    EITER_T eiter = out_edges(v1, ga);

    #pragma mta assert parallel
    for (size_type i = 0; i < deg; i++)
    {
      edge_t e = eiter[i];
      size_type u = get(vid_map, target(e, ga));
      if (v2id == u && ret == false)
      {
        ret = true;
      }
    }

    return(ret);
  }


  // -----[ fillVertexMaps() ]------------------------------------------
  void fillVertexMaps(size_type vs, size_type vt,
                      size_type* Midx2Vidx,
                      size_type* Vidx2Midx)
  {
    size_type vlo, vhi;

    if (vs > vt)
    {
      vhi = vs; vlo = vt;
    }
    else
    {
      vhi = vt; vlo = vs;
    }

    #pragma mta assert parallel
    for (size_type vidx = 0; vidx < nverts; vidx++)
    {
      if (vidx < vlo)
      {
        Midx2Vidx[vidx]   = vidx;
        Vidx2Midx[vidx]   = vidx;
      }
      else if (vidx > vlo && vidx < vhi)
      {
        Midx2Vidx[vidx - 1] = vidx;
        Vidx2Midx[vidx]     = vidx - 1;
      }
      else if (vidx > vhi)
      {
        Midx2Vidx[vidx - 2] = vidx;
        Vidx2Midx[vidx]     = vidx - 2;
      }
    }
  }

  // -----[ fillVertexMaps() ]------------------------------------------

  // -----[ fillMatrix() ]----------------------------------------------
  SparseMatrixCSR<size_type,double>
  fillMatrix(size_type vs, size_type vt,
             size_type* Midx2Vidx, size_type* Vidx2Midx,
             size_type nRows)
  {
    size_type numNonZeros;
    vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, ga);
    vertex_iterator verts = vertices(ga);

#if CSG_DEBUG
    printf(">\tfillMatrix()\n");
    printf("\tvs=%d\tvt=%d\n", vs, vt);
    fflush(stdout);
#endif

    // Count up the nonzeros in M and fill indx[]
    // * remove self-loop edges
    // * include Identity-matrix nonzeros
    size_type* t_rowcount = (size_type*) malloc(sizeof(size_type) * nRows);

    #pragma mta assert parallel
    for (size_type irow = 0; irow < nRows; irow++)
    {
      size_type vid = Midx2Vidx[irow];
      vertex_t vv = verts[vid];

      t_rowcount[irow] = 1;                     // for ident

      size_type deg = out_degree(vv, ga);
      EITER_T edge_iter = out_edges(vv, ga);

      #pragma mta assert parallel
      for (size_type i = 0; i < deg; i++)
      {
        edge_t e = edge_iter[i];
        size_type v2id = get(vid_map, target(e, ga));

        if (v2id != vs && v2id != vt && v2id != vid)
        {
          mt_incr(t_rowcount[irow], 1);
        }
      }
    }

    size_type* indx = (size_type*) malloc(sizeof(size_type) * (nRows + 1));
    indx[0] = 0;

    // serial loop -- can we avoid this one somehow?
    for (size_type i = 1; i < nRows + 1; i++)
    {
      indx[i] = t_rowcount[i - 1] + indx[i - 1];
    }
    free(t_rowcount); t_rowcount = NULL;

    numNonZeros = indx[nRows];

    // Allocate storage for vals and cols
    size_type* cols = (size_type*) malloc(sizeof(size_type) * numNonZeros);
    double* vals = (double*) malloc(sizeof(double) * numNonZeros);

    #pragma mta assert parallel
    for (size_type irow = 0; irow < nRows; irow++)
    {
      size_type ioffset = indx[irow];
      bool hasIdent = false;
      size_type vid = Midx2Vidx[irow];
      vertex_t vv = verts[vid];
      size_type vvdeg = out_degree(vv, ga);
      EITER_T edge_iter = out_edges(vv, ga);

      for (size_type i = 0; i < vvdeg; i++)
      {
        edge_t e = edge_iter[i];

        size_type v2id  = get(vid_map, target(e, ga));
        size_type v2col = Vidx2Midx[v2id];
        if (v2id != vs && v2id != vt && v2id != vid)
        {
          if ( !hasIdent && v2id > vid)
          {
#if CSG_DEBUG
            printf("\t%d->%d: vals[%d] = 1.0\n", vid, v2id);
#endif
            vals[ioffset] = -1.0;
            cols[ioffset] = irow;
            mt_incr(ioffset, 1);
            hasIdent = true;
          }
#if CSG_DEBUG
          printf("\t%d->%d: vals[%d] = %f\n",
                 vid, v2id, ioffset, 1.0 / (neighbors.size() + alpha));
#endif

          // note: use vertex's actuall degree, not # of nonzeros here!
          vals[ioffset] = 1.0 / (vvdeg + alpha);
          cols[ioffset] = v2col;
          mt_incr(ioffset, 1);
        }
      }

      if (!hasIdent)
      {
        vals[ioffset] = -1.0;
        cols[ioffset] = irow;
      }
    }

#if CSG_DEBUG
    printf("\tnumNonZeros = %d\n", numNonZeros);
    print_array(indx, nRows + 1,    "indx");
    print_array(vals, numNonZeros, "vals");
    print_array(cols, numNonZeros, "cols");
    fflush(stdout);
#endif

    SparseMatrixCSR<size_type, double> MATRIX(nRows, nRows, numNonZeros);
    MATRIX.fill(indx, vals, cols);

    // clean-up
    free(indx);  indx = NULL;
    free(cols);  cols = NULL;
    free(vals);  vals = NULL;

#if CSG_DEBUG
    printf("<\tfillMatrix()\n");
    fflush(stdout);
#endif

    return(MATRIX);
  }

  // -----[ fillMatrix() ]----------------------------------------------

  // -----[ fillSourceVector() ]----------------------------------------
  DenseVector<size_type,double>
  fillSourceVector(size_type vs, size_type* Vidx2Midx, size_type nRows)
  {
#if CSG_DEBUG
    printf(">\tfillSourceVector()\n");
    fflush(stdout);
#endif

    vertex_id_map<graph_adapter> vid_map = get(_vertex_id_map, ga);

    double* temp = (double*) malloc(sizeof(double) * nRows);

    #pragma mta assert no dependence
    for (size_type i = 0; i < nRows; i++)
    {
      temp[i] = 0.0;
    }

    vertex_t vsource  = get_vertex(vs, ga);
    size_type vsDegree = out_degree(vsource, ga);
    EITER_T edge_iter = out_edges(vsource, ga);

    #pragma mta assert parallel
    for (size_type i = 0; i < vsDegree; i++)
    {
      edge_t e = edge_iter[i];
      vertex_t v2       = target(e, ga);
      size_type v2id     = get(vid_map, v2);
      size_type v2degree = out_degree(v2, ga);

      if ( v2id != vs )
      {
        size_type v2row = Vidx2Midx[v2id];
        mt_readfe(temp[v2row]);
        mt_write(temp[v2row], -1.0 / (v2degree + alpha));
      }

    }

    DenseVector<size_type,double> V_VOLTS(nRows);
    V_VOLTS.fill(temp);
    free(temp); temp = NULL;
    return(V_VOLTS);
  }

  // -----[ fillSourceVector() ]----------------------------------------

  // -----[ compute_node_voltages() ]-----------------------------------
  size_type compute_node_voltages(vertex_id_map<graph_adapter>& vid_map)
  {
#if CSG_DEBUG
    printf(">\tcompute_node_voltages()\n");
    printf("\talpha = %f\n", alpha);
    fflush(stdout);
#endif

    // set up vertex matrix, M_VERT
    size_type M_nRows = nverts - 2;

    // Generate VID mapping for vertex IDs in the VERT matrix
    // - Midx2Vidx[idx] = vid :: idx = row/col index in M_VERT
    size_type* Midx2Vidx   = (size_type*) malloc(sizeof(size_type) * M_nRows);
    size_type* Vidx2Midx   = (size_type*) malloc(sizeof(size_type) * nverts);

#if CSG_DEBUG
    assert(Midx2Vidx   != NULL);
    assert(Vidx2Midx   != NULL);
#endif

    Vidx2Midx[vs] = -1;
    Vidx2Midx[vt] = -1;

    // Fill the vertex maps.
    // note: don't include rows or cols for Vs or Vt
    fillVertexMaps(vs, vt, Midx2Vidx, Vidx2Midx);

    SparseMatrixCSR<size_type,double> M_VERT = fillMatrix(vs, vt, Midx2Vidx,
                                                Vidx2Midx, M_nRows);

    // FILL the source term vector, V_STERM
    DenseVector<size_type,double> V_STERM = fillSourceVector(vs, Vidx2Midx, M_nRows);

#if CSG_DEBUG
    M_VERT.MatrixPrint("M_VERT");
    V_STERM.VectorPrint("V_STERM");
    fflush(stdout);
#endif

    // Compute voltages using conjugate gradient method
    double err;
    DenseVector<size_type,double> V_VOLTS(M_nRows);

    linbcg(M_VERT, V_VOLTS, V_STERM, maxSolverIters, err, solverTolerance);

#if CSG_DEBUG
    printf("\tCompleted linbcg() solver\n");
    V_VOLTS.VectorPrint("V_VOLTS");
    fflush(stdout);
#endif

    // Copy voltages into voltage array
    node_voltages[vs] = CSG_VOLT_S;
    node_voltages[vt] = CSG_VOLT_T;

    #pragma mta assert no dependence
    for (size_type mvid = 0; mvid < M_nRows; mvid++)
    {
      node_voltages[ Midx2Vidx[mvid] ] = V_VOLTS[ mvid ];
    }

    // Clean up now that we have our voltages.
    free(Midx2Vidx); Midx2Vidx = NULL;
    free(Vidx2Midx); Vidx2Midx = NULL;
    V_STERM.clear();
    V_VOLTS.clear();
    M_VERT.clear();

    return(0);
  }

  // -----[ compute_node_voltages() ]-----------------------------------

  // -----[ fill_iout() ]-----------------------------------------------
  void fill_iout(double* Iout, size_type Iout_sz,
                 vertex_id_map<graph_adapter>& vid_map)
  {
    vertex_iterator verts = vertices(ga);

    #pragma mta assert parallel
    for (size_type v1id = 0; v1id < Iout_sz; v1id++)
    {
      Iout[v1id] = 0;

      vertex_t v1ref = verts[v1id];
      size_type deg = out_degree(v1ref, ga);
      EITER_T edge_iter = out_edges(v1ref, ga);

      for (size_type ineigh = 0; ineigh < deg; ineigh++)
      {
        edge_t e = edge_iter[ineigh];
        size_type v2id = get( vid_map, target(e, ga));
        if (v1id != v2id)
        {
          double Iuv = node_voltages[v1id] - node_voltages[v2id];
          if (Iuv > 0.0)
          {
            // Can this be parallelized?  Iout is a double, so int_fetch_add
            // won't work.
            Iout[v1id] += Iuv;
#if CSG_DEBUG
            printf("\tIout[%d]+= %f => %f\n", v1id, Iuv, Iout[v1id]);
#endif
          }
        }
      }
    }
#if CSG_DEBUG
    print_array(Iout, Iout_sz, "Iout");
#endif
  }

  // -----[ fill_iout() ]-----------------------------------------------

  // -----[ generate_display_graph() ]----------------------------------
  void generate_display_graph(vertex_id_map<graph_adapter>& vid_map)
  {
    #pragma mta trace "generate_display_graph start"

#if CSG_DEBUG
    printf(">\tgenerate_display_graph()\n");
    fflush(stdout);
#endif

    double* Iout = (double*) malloc(sizeof(double) * nverts);
    size_type P = maxPathLength;

    /* Fill Iout[] */
    fill_iout(Iout, nverts, vid_map);

    // Generate array U of vertex ids (indices atm) in
    // order of descending node voltages.
    size_type* U       = (size_type*) malloc(sizeof(size_type) * nverts);
    double* t_volts = (double*) malloc(sizeof(double) * nverts);

    #pragma mta assert no dependence
    for (size_type i = 0; i < nverts; i++)
    {
      U[i] = i;
      t_volts[i] = node_voltages[i];
    }

    // sort U in order of descending node voltages.
    quicksort<double, DESCENDING, size_type> vsort(t_volts, nverts, U);
    vsort.run();

    free(t_volts);
    t_volts = NULL;

    // create mapping from vertex idx to U-index.
    size_type* Vidx2Uidx = (size_type*) malloc(nverts * sizeof(size_type));

    // allocate csg_mask
    #pragma mta assert no dependence
    for (size_type i = 0; i < nverts; i++)
    {
      Vidx2Uidx[ U[i] ] = i;
    }

    array2d<double> D(nverts, P);
    array2d<size_type>    Dsrc(nverts, P);

    bool noPathFound = false;

    vertex_iterator verts = vertices(ga);

    for (size_type iexpansion = 0;
        (noPathFound == false) && (iexpansion < numExpansions); iexpansion++)
    {
      size_type k_idx  = 0;
      size_type v      = vt;

#if CSG_DEBUG
      printf("========================================\n");
      printf("numExpansions = %d\n", numExpansions);
      print_array(U, nverts, "U:");
      print_array(Iout, nverts, "Iout");
      print_array(csg_vertex_mask, num_vertices(ga), "csg_vmask");
      printf(">>>csg_vertex_mask[U[0]] = %d\n", csg_vertex_mask[U[0]]);
      fflush(stdout);
#endif

      // initialize delivery/source matrices.
      D.init(0.0);
      Dsrc.init(VERT_UNKNOWN);

      if (csg_vertex_mask[U[0]])
      {
        D[U[0]][0] = Iout[vs];
      }
      else
      {
        D[U[0]][1] = Iout[vs];
      }

      // verts = rows
      // most bang for the buck could be parallelizing rows, -but-
      // each row is dependent on its previous row...
      for (size_type vidx = 1; vidx < nverts; vidx++)
      {
        size_type vid       = U[vidx];
        vertex_t vptr = verts[vid];

        // k = columns O(sp_length+max_past_splen)

        #pragma mta assert parallel
        for (size_type k = 0; k < (P - 1); k++)
        {
          // if v is in Gout, set k' = k.
          size_type k_prime  = csg_vertex_mask[vid] ? k : k + 1;
          size_type u_max    = -1;
          double d_max    = -1.0;

          size_type v_degree = out_degree(vptr, ga);
          EITER_T eiter = out_edges(vptr, ga);

          #pragma mta assert parallel
          for (size_type i = 0; i < v_degree; i++)
          {
            edge_t e = eiter[i];

            double d_prime;
            size_type uid  = get(vid_map, target(e, ga));
            size_type uidx = Vidx2Uidx[uid];

            if ((uidx < vidx) && (D[uid][k] > 0))
            {
              double Iuv = node_voltages[uid] - node_voltages[vid];
              d_prime    = D[uid][k] *Iuv / Iout[uid];

              if (d_prime > d_max)
              {
                // reader/writer locks
                double dmax_t = mt_readfe(d_max);
                size_type umax_t = mt_readfe(u_max);

                if (d_prime > dmax_t)
                {
                  dmax_t = d_prime;
                  umax_t = uid;
                }
                mt_write(d_max, dmax_t);
                mt_write(u_max, umax_t);
              }
            }
          }

          if (d_max >= 0.0)
          {
#if CSG_DEBUG
            printf("D[%d][%d] <= %.6f\n", vid, k_prime, d_max);
            fflush(stdout);
#endif
            D[vid][k_prime] = d_max;
            Dsrc[vid][k_prime] = u_max;
          }
        }
      }

      /* get column with max D[vt][k]/k */
      // wcm: can MTA compiler detect this reduction???
      double D_max = 0.0;

      #pragma mta assert parallel
      for (size_type k = 1; k < P; k++)
      {
        double D_p = D[vt][k] / k;

        if (D_p > D_max)
        {
          D_max = D_p;
          k_idx = k;
        }
      }

      v = vt;

      if (k_idx > 0)
      {
        while (v != vs)
        {
          size_type v_src = Dsrc[v][k_idx];

          if (!csg_vertex_mask[v])
          {
            csg_vertex_mask[v] = true;
            mt_incr(k_idx, -1);
          }

          v = v_src;
          assert(v >= 0);
        }

        if (!csg_vertex_mask[vs])
        {
          csg_vertex_mask[vs] = true;
        }
      }
      else
      {
        noPathFound = true;
      }
#if CSG_DEBUG
      print_array(csg_vertex_mask, nverts, "csg_vertex_mask:");
#endif
    }

    free(Iout);
    free(U);
    free(Vidx2Uidx);
    U = NULL;
    Iout = NULL;
    Vidx2Uidx = NULL;
    #pragma mta trace "generate_display_graph stop"
  }

  // -----[ generate_display_graph() ]----------------------------------

  // ----------------------------------------------------------------------
  //      ####  ####  ###  #   #   ##   #####  ####
  //      #  #  #  #   #   #   #  #  #    #    #
  //      ####  ####   #   #   #  ####    #    ###
  //      #     # #    #    # #   #  #    #    #
  //      #     #  #  ###    #    #  #    #    ####
  // ----------------------------------------------------------------------

private:
  graph_adapter& ga;
  size_type nverts;
  size_type vs, vt;
  dynamic_array<size_type>& vertexIdList;

  size_type numExpansions;
  size_type maxPathLength;           // maximum path length to consider
  double* node_voltages;
  double* edge_currents;
  bool*   csg_vertex_mask;          // vertices to be added to the subgraph
  bool vs_and_vt_connected;
  double alpha;                     // alpha parameter for 'tax' edges
  double solverTolerance;           // tolerance for linear solver
  size_type maxSolverIters;         // max iters for linear solver
};
// ------------------ end class connection_subgraph -----------------------

}
// ------------------------- end namespace mtgl ---------------------------

// Cleanup definitions which should remain local to this file
#undef CSG_DEBUG
#undef TIMER_SECT
#undef TIMER_RESTART
#undef DEBUG_BREAK
#undef BEGIN_LOGGING
#undef END_LOGGING

#endif

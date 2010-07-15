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

// ***********************************************************************
// ** gen_rmat_graph.hpp
// ***********************************************************************
// **
// ** Author: Kamesh Madduri
// ** Creation: 2007
/*! \file gen_rmat_graph.hpp
    \brief A scale-free graph generator based on the R-MAT (recursive matrix)
           algorithm. D. Chakrabarti, Y. Zhan, and C. Faloutsos, "R-MAT: A
           recursive model for graph mining", SIAM Data Mining 2004.
*/
// **********************************************************************

#ifndef GEN_RMAT_GRAPH_H
#define GEN_RMAT_GRAPH_H

#include <stdio.h>
#include <limits.h>
#ifdef HAVE_MALLOC_H
# include <malloc.h>
#endif
#include <assert.h>
#include <math.h>
#include <mtgl/util.h>
#include <mtgl/graph.hpp>
#include <mtgl/hash_set.h>
#include <mtgl/rand_fill.hpp>
#include <mtgl/mtgl_adapter.hpp>
#include <mtgl/xmt_hash_table.hpp>

#ifdef __MTA__
#include <sys/mta_task.h>
#include <machine/runtime.h>
#include <luc/luc_exported.h>
#include <snapshot/client.h>
#else
#include <mtgl/snap_util.h>
#endif

#ifdef USING_QTHREADS
# include <qthread/qutil.h>
# include <qthread/qloop.h>
#endif

namespace mtgl {

#define GEN_HAMILTONIAN_CYCLE 0


template <class graph_adapter>
class gen_rmat_graph {
public:
  typedef xmt_hash_table<int64_t, int64_t> hash_table_t;
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph_adapter>::size_type count_t;

  gen_rmat_graph(int SCALE_, int edgeFactor_, double a = 0.57,
                 double b = 0.19, double c = 0.19, double d = 0.05) :
    SCALE(SCALE_), edgeFactor(edgeFactor_), ptrGraph(NULL),
    _a0(a), _b0(b), _c0(c), _d0(d)
  {
    // "nasty" //_a0 = 0.57; _b0 = 0.19; _c0 = 0.19; _d0 = 0.05;
    // "nice" // _a0 = 0.45; _b0 = 0.15; _c0 = 0.15; _d0 = 0.25;
  }

  void set_graph_pointer( graph_adapter* ga_ )
  {
    ptrGraph = ga_;
    assert(ptrGraph != NULL);
  }

  void set_a0(double val) { _a0 = val; }
  void set_b0(double val) { _b0 = val; }
  void set_c0(double val) { _c0 = val; }
  void set_d0(double val) { _d0 = val; }

  double get_a0(void) { return(_a0); }
  double get_b0(void) { return(_b0); }
  double get_c0(void) { return(_c0); }
  double get_d0(void) { return(_d0); }

#ifdef USING_QTHREADS
  struct runloop1_s {
    random_value* const rv;
    double* const rv_p;

    runloop1_s(random_value* const rv_i, double* const rv_p_i) :
      rv(rv_i), rv_p(rv_p_i) {}
  };

  static void runloop1(qthread_t* me, const size_t startat,
                       const size_t stopat, const size_t step, void* arg)
  {
    double* const rv_p(((runloop1_s*) arg)->rv_p);
    random_value* const rv(((runloop1_s*) arg)->rv);
    const random_value rv_max(rand_fill::get_max());

    for (size_t i = startat; i < stopat; i++)
    {
      rv_p[i] = ((double) rv[i]) / rv_max;
    }
  }

  struct runloop2_s {
    const double a0, b0, c0, d0;
    const count_t n;
    count_t& numedgesAdded;
    double* const rv_p;
    hash_table_t& ht;
    gen_rmat_graph* const t;
    count_t* srcs;
    count_t* dests;

    runloop2_s(const double a, const double b, const double c, const double d,
               const count_t n_i, double* const rv_p_i, count_t* const sr,
               count_t* const de, count_t& nea, hash_table_t& h,
               gen_rmat_graph* const th) :
      a0(a), b0(b), c0(c), d0(d), n(n_i), numedgesAdded(nea),
      rv_p(rv_p_i), ht(h), t(th), srcs(sr), dests(de) {}
  };

  static void runloop2(qthread_t* me, const size_t startat,
                       const size_t stopat, const size_t step, void* arg)
  {
    runloop2_s* args = (runloop2_s*) arg;
    double* const rv_p(args->rv_p);
    hash_table_t& ht(args->ht);
    gen_rmat_graph* const t(args->t);
    const count_t SCALE(t->SCALE);
    const count_t n(args->n);
    const double a0(args->a0); const double b0(args->b0);
    const double c0(args->c0); const double d0(args->d0);

    for (size_t i = startat; i < stopat; i++)
    {
      double a = a0; double b = b0;
      double c = c0; double d = d0;
      count_t u = (count_t) 1;
      count_t v = (count_t) 1;
      count_t step = n / 2;

      for (count_t j = 0; j < SCALE; j++)
      {
        t->choosePartition(rv_p[(i * SCALE + j)], &u, &v, step, a, b, c, d);
        step = step / 2;
        t->varyParams(rv_p, (i * SCALE + j), &a, &b, &c, &d);
      }

      int64_t key = ((int64_t) (u - 1)) * n + (int64_t) (v - 1);
      bool added = ht.insert(key, key);

      if (added)
      {
        aligned_t my_ind = mt_incr(args->numedgesAdded, 1);
        args->srcs[my_ind]  = u - 1;
        args->dests[my_ind] = v - 1;
      }
    }
  }

#endif

  void gen_rmat_body(count_t*& srcs, count_t*& dests, count_t&  n, count_t& m)
  {
    random_value* rv;
    int randValSize /*, rv_index*/;
    count_t numedgesPerPhase, numedgesAdded; //, numedgesToBeAdded;
    double* rv_p;
    double a0, b0, c0, d0;
    //BXM: make these as parameters in the constructor
    float alpha;
    int tolerance;

    n = 1 << SCALE;
    m = edgeFactor * n;

    alpha = 0.05;
    tolerance = (int) ((1.0 - alpha) * m); //lower bound on the tolerance

    fprintf(stderr, "n: %lu, m: %lu, tolerance: %d\n", n, m, tolerance);

    /* First create a Hamiltonian cycle, so that we have a connected graph */
    srcs  = (count_t*) malloc(m * sizeof(count_t));
    dests = (count_t*) malloc(m * sizeof(count_t));

    // JWB: getting rid of the Hamilton cycle 3/08
#if GEN_HAMILTONIAN_CYCLE
    #pragma mta assert nodep
    for (i = 0; i < n - 1; i++)
    {
      srcs[i] = i;
      dests[i] = i + 1;
    }

    srcs[n - 1] = n - 1;
    dests[n - 1] = 0;
#endif

    // We need 13*SCALE random numbers for adding one edge
    // The edges are generated in phases, in order to be able to
    // reproduce the same graph for a given SCALE value

    // Generate 2^22 (4.1M) edges in each phase on MTA
    //          2^18 (262k) edges in each phase if not on MTA
    // We fill an array of random numbers of size
    // 13*SCALE*numedgesPerPhase in each phase
#ifdef __MTA__
    numedgesPerPhase = 1 << 22;
#else
    numedgesPerPhase = 1 << 16;
#endif

    if (m < numedgesPerPhase) numedgesPerPhase = m;

    randValSize = 13 * numedgesPerPhase * SCALE;
    rv = (random_value*) malloc(randValSize * sizeof(random_value));
    rv_p = (double*) malloc(randValSize * sizeof(double));

    /* Start adding edges */
    //a0=0.45;  b0=0.15;  c0=0.15;  d0=0.25; // >|< original settings >|<
    //a0=0.57;  b0=0.19;  c0=0.19;  d0=0.05; // Jon's Settings
    a0 = this->_a0;  // default = 0.57
    b0 = this->_b0;  // default = 0.19
    c0 = this->_c0;  // default = 0.19
    d0 = this->_d0;  // default = 0.05

#if GEN_HAMILTONIAN_CYCLE
    numedgesAdded = n;
#else
    numedgesAdded = 0;  // JWB: getting rid of the Hamilton cycle
#endif

    xmt_hash_table<int64_t, int64_t> htab((int) (1.2 * tolerance), true);
    //int next=0;

    createEdgesLoop(numedgesAdded, numedgesPerPhase, rv, rv_p, randValSize,
                    n, m, tolerance, srcs, dests, a0, b0, c0, d0, htab);

    m = htab.size();

    if (numedgesAdded != htab.size())
    {
      fprintf(stderr, "error: num_edges: %ld, ht.sz: %ld\n",
              numedgesAdded, htab.size());
      exit(1);
    }

    free(rv);
    free(rv_p);

    // in DIMACS-C version, genEdgeWeights() is called here to set edge weights.
  }

/*! \class gen_rmat_graph
    \brief The basic idea behind the R-MAT algorithm is to recursively
           sub-divide the adjacency matrix of the graph into four equal-sized
           partitions, and distribute edges within these partitions with
           unequal probabilities. Starting off with an empty adjacency matrix,
           edges are added one at a time. Each edge chooses one of the four
           partitions with probabilities a, b, a, and d. a+b+c+d = 1. Some
           noise is added at each stage of the recursion, and the a,b,c,d
           values are renormalized.

           The generator is called with just one input parameter SCALE. The
           number of vertices is set to 2^SCALE, and the number of edges
           to 8*2^SCALE. The values of a, b, c, d are set to 0.45, 0.15, 0.15
           and 0.25 respectively. This generates two big communities (a, d),
           with cross-links between them (b, c). The recursive nature of the
           generator results in the "communities within communities" effect.
*/
  graph_adapter* run(bool undirected = true, bool snap = false,
                     char* src_filename = 0, char* dest_filename = 0)
  {
    typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex_t;
    typedef typename graph_traits<graph_adapter>::size_type count_t;

    graph_adapter* g = NULL;
    if (ptrGraph)
    {
      g = ptrGraph;
    }
    else
    {
      g = new graph_adapter;
    }
//    graph_adapter *g = new graph_adapter;
    count_t* srcs;
    count_t* dests;
    count_t n, m;

    gen_rmat_body(srcs, dests, n, m);

#ifdef PERMUTE_NODES
    randPerm(n, m, srcs, dests);
#endif
    if (snap)
    {
      if (snap_init() != SNAP_ERR_OK)
      {
        return NULL;
      }
      if (snap_snapshot(src_filename, srcs, m * sizeof(count_t), NULL) !=
          SNAP_ERR_OK)
      {
        return NULL;
      }
      if (snap_snapshot(dest_filename, dests, m * sizeof(count_t), NULL) !=
          SNAP_ERR_OK)
      {
        return NULL;
      }
    }

    init(n, m, srcs, dests, *g);

    free(srcs);
    free(dests);
    //rmat_stats(*g);

    return g;
  }

  graph_adapter* old_run(bool undirected = true)
  {
    graph_adapter* g = NULL;
    if (ptrGraph)
    {
      g = ptrGraph;
    }
    else
    {
      g = new graph_adapter;
    }

//    graph_adapter *g = new graph_adapter;
    int* srcs;
    int* dests;
    int n, m;
    random_value* rv;
    int randValSize;
    count_t numedgesPerPhase, numedgesAdded; //, numedgesToBeAdded;
    double* rv_p;
    double a0, b0, c0, d0;
    //BXM: make these as parameters in the constructor
    float alpha;
    int tolerance;

    n = 1 << SCALE;
    m = edgeFactor * n;

    alpha = 0.05;
    tolerance = (int) ((1.0 - alpha) * m); //lower bound on the tolerance

    fprintf(stderr, "n: %d, m: %d, tolerance: %d\n", n, m, tolerance);

    /* First create a Hamiltonian cycle, so that we have a connected graph */
    srcs  = (int*) malloc(m * sizeof(int));
    dests = (int*) malloc(m * sizeof(int));

    // JWB: getting rid of the Hamilton cycle 3/08

#if GEN_HAMILTONIAN_CYCLE
    #pragma mta assert nodep
    for (i = 0; i < n - 1; i++)
    {
      srcs[i] = i;
      dests[i] = i + 1;
    }

    srcs[n - 1] = n - 1;
    dests[n - 1] = 0;
#endif

    // We need 13*SCALE random numbers for adding one edge
    // The edges are generated in phases, in order to be able to
    // reproduce the same graph for a given SCALE value

    // Generate 2^22 (4.1M) edges in each phase on MTA
    //          2^18 (262k) edges in each phase if not on MTA
    // We fill an array of random numbers of size
    // 13*SCALE*numedgesPerPhase in each phase
#ifdef __MTA__
    numedgesPerPhase = 1 << 22;
#else
    numedgesPerPhase = 1 << 16;
#endif

    if (m < numedgesPerPhase) numedgesPerPhase = m;

    randValSize = 13 * numedgesPerPhase * SCALE;
    rv = (random_value*) malloc(randValSize * sizeof(random_value));
    rv_p = (double*) malloc(randValSize * sizeof(double));

    /* Start adding edges */
    //a0=0.45;  b0=0.15;  c0=0.15;  d0=0.25; // >|< original settings >|<
    //a0=0.57;  b0=0.19;  c0=0.19;  d0=0.05; // Jon's Settings
    a0 = this->_a0;  // default = 0.57
    b0 = this->_b0;  // default = 0.19
    c0 = this->_c0;  // default = 0.19
    d0 = this->_d0;  // default = 0.05

#if GEN_HAMILTONIAN_CYCLE
    numedgesAdded = n;
#else
    numedgesAdded = 0;  // JWB: getting rid of the Hamilton cycle
#endif

    xmt_hash_table<int64_t, int64_t> htab((int) (1.2 * tolerance), true);
    //int next=0;

    createEdgesLoop(numedgesAdded, numedgesPerPhase, rv, rv_p, randValSize,
                    n, m, tolerance, srcs, dests, a0, b0, c0, d0, htab);

    m = htab.size();
    if (numedgesAdded != htab.size())
    {
      fprintf(stderr, "error: num_edges: %ld, ht.sz: %ld\n",
              numedgesAdded, htab.size());
      exit(1);
    }

    free(rv);
    free(rv_p);

    // in DIMACS-C version, genEdgeWeights() is called here to set edge weights.

#ifdef PERMUTE_NODES
    randPerm(n, m, srcs, dests);
#endif

    init(n, m, srcs, dests, g);

    free(srcs);
    free(dests);
    //rmat_stats(*g);

    return g;
  }

  void createEdgesLoop(count_t& numedgesAdded, count_t& numedgesPerPhase,
                       random_value*& rv, double*& rv_p,
                       int& randValSize, count_t& n, count_t& m,
                       int& tolerance,
                       count_t*& srcs, count_t*& dests,
                       const double& a0, const double& b0, const double& c0,
                       const double& d0, xmt_hash_table<int64_t, int64_t>& ht)
  {
    printf("createEdgesLoop: numedgesPerPhase: %ld\n", numedgesPerPhase);
    count_t numedgesToBeAdded;

    while (numedgesAdded <  (count_t)tolerance)
    {
      printf("createEdgesLoop: iter: %ld\n", numedgesAdded);

      rand_fill::generate(randValSize, rv);

#ifdef USING_QTHREADS
      runloop1_s rl1_args(rv, rv_p);
      if (0 < randValSize)
      {
        qt_loop_balance(0, randValSize, runloop1, &rl1_args);
      }
#else
      random_value rv_max = rand_fill::get_max();
      #pragma mta assert nodep
      for (int i = 0; i < randValSize; i++)
      {
        rv_p[i] = ((double) rv[i]) / rv_max;
      }
#endif

      if (m - numedgesAdded < numedgesPerPhase)
      {
        numedgesToBeAdded = m - numedgesAdded;
      }
      else
      {
        numedgesToBeAdded = numedgesPerPhase;
      }

#ifdef USING_QTHREADS
      runloop2_s rl2_args(a0, b0, c0, d0, n, rv_p, srcs, dests,
                          numedgesAdded, ht, this);
      qt_loop_balance(0, numedgesToBeAdded, runloop2, &rl2_args);
#else
      createEdges(numedgesToBeAdded, numedgesAdded, srcs, dests, n, rv_p,
                  a0, b0, c0, d0, ht);
#endif

      fprintf(stderr, "%ld ", numedgesAdded);
    }

    fprintf(stderr, "\n");
  }

  void sort_srcs_dests(int& size, int*& srcs, int*& dests, int*& start)
  {
    int max_key = -1;

    #pragma mta assert parallel
    for (int i = 0; i < size; i++)
    {
      if (srcs[i] > max_key)
      {
        max_key = srcs[i];
      }
    }

    bucket_sort(srcs, size, max_key, dests, start);
  }

/*
  void takeoutDupes(int*& srcs, int*& dests, int& numedgesAdded)
  {
    int* start = 0;
    sort_srcs_dests(numedgesAdded, srcs, dests, start);

    int* ret_srcs = (int *) malloc(numedgesAdded*sizeof(int));
    int* ret_dests = (int *) malloc(numedgesAdded*sizeof(int));

    ret_srcs[0] = srcs[0];
    ret_dests[0] = dests[0];

    int ret_vector_size = 1;

    #pragma mta assert parallel
    for (int i = 1; i < numedgesAdded; i++) {
      int start_ind = start[srcs[i]];
      bool found = false;

      #pragma mta assert nodep
      for (int j = start_ind; j < i; j++) {
        if (dests[j] == dests[i]) {
          found = true;
#ifndef __MTA__
          break;
#endif
        }
      }

      if (!found) {
        int ind = mt_incr(ret_vector_size, 1);
        ret_srcs[ind] = srcs[i];
        ret_dests[ind] = dests[i];
      }
    }

    int* tmp_srcs;
    int* tmp_dests;

    tmp_srcs = srcs;
    srcs = ret_srcs;
    free(tmp_srcs);

    tmp_dests = dests;
    dests = ret_dests;
    free(tmp_dests);

    numedgesAdded = ret_vector_size;

    if (start)  free (start);
  }
*/
  void takeoutDupes(int*& srcs, int*& dests, count_t& numedgesAdded,
                    xmt_hash_table<int64_t, int64_t>& htab, int n)
  {
    int* ret_srcs = (int*) malloc(numedgesAdded * sizeof(int));
    int* ret_dests = (int*) malloc(numedgesAdded * sizeof(int));

    count_t next = 0;
    int dups = 0;

    #pragma mta assert parallel
    for (count_t i = 0; i < numedgesAdded; i++)
    {
      int64_t key = srcs[i] *n + dests[i];
      bool added = htab.insert(key, key);
      if (added)
      {
        count_t my_ind = mt_incr(next, 1);
        ret_srcs[my_ind] = srcs[i];
        ret_dests[my_ind] = dests[i];
      }
    }

    int* tmp_srcs;
    int* tmp_dests;

    tmp_srcs = srcs;
    srcs = ret_srcs;
    free(tmp_srcs);

    tmp_dests = dests;
    dests = ret_dests;
    free(tmp_dests);

    numedgesAdded = next;
  }

  void createEdges(count_t& numedgesToBeAdded, count_t& next,
                   count_t*& srcs, count_t*& dests,
                   const count_t& n, double*& rv_p,
                   const double& a0, const double& b0,
                   const double& c0, const double& d0,
                   xmt_hash_table<int64_t, int64_t>& ht)
  {
    int failures = 0;

    #pragma mta assert parallel
    for (count_t i = 0; i < numedgesToBeAdded; i++)
    {
      double a = a0; double b = b0;
      double c = c0; double d = d0;
      count_t u = (count_t) 1;
      count_t v = (count_t) 1;
      count_t step = (count_t) (n / 2);

      for (int j = 0; j < SCALE; j++)
      {
        choosePartition(rv_p[(count_t) (i * SCALE + j)], &u, &v,
                        step, a, b, c, d);
        step = step / 2;
        varyParams(rv_p, (i * SCALE + j), &a, &b, &c, &d);
      }

      int64_t key = (u - 1) * n + (v - 1);
      bool added = ht.insert(key, key);
      if (added)
      {
        int my_ind = mt_incr(next, 1);
        srcs[my_ind] = u - 1;
        dests[my_ind] = v - 1;
      }
      else
      {
        failures++;
      }
    }
    printf("createEdges: toadd: %lu, next: %lu, failures: %d\n",
           numedgesToBeAdded, next, failures);
  }

  void choosePartition(double p, count_t* u, count_t* v, count_t step,
                       double a, double b, double c, double d)
  {
    if (p < a)
    {
      /* Do nothing */
    }
    else if ((p >= a) && (p < a + b))
    {
      *v += step;
    }
    else if ((p >= a + b) && (p < a + b + c))
    {
      *u += step;
    }
    else if ((p >= a + b + c) && (p < a + b + c + d))
    {
      *u += step;
      *v += step;
    }
  }

  void varyParams(double* rv_p, count_t offset, double* a,
                  double* b, double* c, double* d)
  {
    double v, S;

    /* Allow a max. of 10% variation */
    v = 0.10;
    if (rv_p[offset + 1] > 0.5)
    {
      *a += *a * v * rv_p[offset + 5];
    }
    else
    {
      *a -= *a * v * rv_p[offset + 6];
    }

    if (rv_p[offset + 2] > 0.5)
    {
      *b += *b * v * rv_p[offset + 7];
    }
    else
    {
      *b -= *b * v * rv_p[offset + 8];
    }

    if (rv_p[offset + 3] > 0.5)
    {
      *c += *c * v * rv_p[offset + 9];
    }
    else
    {
      *c -= *c * v * rv_p[offset + 10];
    }

    if (rv_p[offset + 4] > 0.5)
    {
      *d += *d * v * rv_p[offset + 11];
    }
    else
    {
      *d -= *d * v * rv_p[offset + 12];
    }

    S = *a + *b + *c + *d;

    *a = *a / S;
    *b = *b / S;
    *c = *c / S;
    *d = *d / S;
  }

  void rmat_stats(graph_adapter& g)
  {
    typedef typename graph_traits<graph_adapter>::vertex_descriptor
                     vertex_descriptor;
    typedef typename graph_traits<graph_adapter>::size_type count_t;
    typedef typename graph_traits<graph_adapter>::vertex_iterator
                     vertex_iterator;
    typedef typename graph_traits<graph_adapter>::adjacency_iterator
                     adjacency_iterator;

    count_t num_a = 0, num_b = 0, num_c = 0, num_d = 0;
    vertex_id_map<graph_adapter> vipm = get(_vertex_id_map, g);
    count_t n = num_vertices(g);
    count_t m = num_edges(g);

    vertex_iterator verts = vertices(g);

    #pragma mta assert parallel
    #pragma mta loop future
    for (count_t i = 0; i < n; i++)
    {
      vertex_descriptor v = verts[i];
      count_t sid = get(vipm, v);
      count_t deg = out_degree(v, g);
      adjacency_iterator adj_verts = adjacent_vertices(v, g);

      #pragma mta assert parallel
      for (count_t j = 0; j < deg; j++)
      {
        vertex_descriptor trg = adj_verts[j];
        int tid = get(vipm, trg);

        if (sid <= n / 2 && tid <= n / 2)
        {
          mt_incr(num_a, 1);
        }
        else if (sid <= n / 2 && tid > n / 2)
        {
          mt_incr(num_b, 1);
        }
        else if (sid > n / 2 && tid <= n / 2)
        {
          mt_incr(num_c, 1);
        }
        else
        {
          mt_incr(num_d, 1);
        }
      }
    }

    printf("num_a: %d (%lf%%)\n", num_a, num_a / (double) m);
    printf("num_b: %d (%lf%%)\n", num_b, num_b / (double) m);
    printf("num_c: %d (%lf%%)\n", num_c, num_c / (double) m);
    printf("num_d: %d (%lf%%)\n", num_d, num_d / (double) m);
  }

private:
  int SCALE;        // 2^SCALE vertices
  int edgeFactor;   // 2^SCALE*edgeFactor edges to be generated
  graph_adapter* ptrGraph;  // a pointer to a graph (if provided).
  double _a0;
  double _b0;
  double _c0;
  double _d0;
};

}

#endif

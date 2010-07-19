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
/*! \file generate_rmat_graph.hpp

    \brief A scale-free graph generator based on the R-MAT (recursive matrix)
           algorithm. D. Chakrabarti, Y. Zhan, and C. Faloutsos, "R-MAT: A
           recursive model for graph mining", SIAM Data Mining 2004.

    \author Kamesh Madduri
    \author Greg Mackey (gemacke@sandia.gov)

    \date 12/4/2007
*/
/****************************************************************************/

#ifndef MTGL_GENERATE_RMAT_GRAPH_HPP
#define MTGL_GENERATE_RMAT_GRAPH_HPP

#include <cstdio>

#include <mtgl/util.hpp>
#include <mtgl/rand_fill.hpp>
#include <mtgl/mtgl_adapter.hpp>
#include <mtgl/xmt_hash_table.hpp>

#ifdef USING_QTHREADS
  # include <qthread/qutil.h>
  # include <qthread/qloop.h>
#endif

#define GEN_HAMILTONIAN_CYCLE 0

namespace mtgl {

template <class graph_adapter>
class gen_rmat_graph {
public:
  typedef xmt_hash_table<int64_t, int64_t> hash_table_t;
  typedef typename graph_traits<graph_adapter>::size_type size_type;

  gen_rmat_graph(int scale_, int edge_factor_, double a,
                 double b, double c, double d) :
    scale(scale_), edge_factor(edge_factor_), _a0(a), _b0(b), _c0(c), _d0(d) {}

#ifdef USING_QTHREADS
  struct runloop1_s {
    random_value* const rv;
    double* const rv_p;

    runloop1_s(random_value* const rv_i, double* const rv_p_i) :
      rv(rv_i), rv_p(rv_p_i) {}
  };

  static void runloop1(qthread_t* me, const size_t startat,
                       const size_t stopat, void* arg)
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
    const double a0;
    const double b0;
    const double c0;
    const double d0;
    const size_type n;
    size_type& num_edges_added;
    double* const rv_p;
    hash_table_t& ht;
    gen_rmat_graph* const t;
    size_type* srcs;
    size_type* dests;

    runloop2_s(const double a, const double b, const double c, const double d,
               const size_type n_i, double* const rv_p_i, size_type* const sr,
               size_type* const de, size_type& nea, hash_table_t& h,
               gen_rmat_graph* const th) :
      a0(a), b0(b), c0(c), d0(d), n(n_i), num_edges_added(nea),
      rv_p(rv_p_i), ht(h), t(th), srcs(sr), dests(de) {}
  };

  static void runloop2(qthread_t* me, const size_t startat,
                       const size_t stopat, void* arg)
  {
    runloop2_s* args = (runloop2_s*) arg;

    double* const rv_p(args->rv_p);
    hash_table_t& ht(args->ht);
    gen_rmat_graph* const t(args->t);
    const size_type scale(t->scale);
    const size_type n(args->n);
    const double a0(args->a0);
    const double b0(args->b0);
    const double c0(args->c0);
    const double d0(args->d0);

    for (size_t i = startat; i < stopat; i++)
    {
      printf(" rl %d %d\n", i, stopat);
      double a = a0;
      double b = b0;
      double c = c0;
      double d = d0;
      size_type u = (size_type) 1;
      size_type v = (size_type) 1;
      size_type step = n / 2;

      for (size_type j = 0; j < scale; j++)
      {
        t->choose_partition(rv_p[i * scale + j], &u, &v, step, a, b, c, d);
        step = step / 2;
        t->vary_params(rv_p, i * scale + j, &a, &b, &c, &d);
      }

      int64_t key = ((int64_t) (u - 1)) * n + (int64_t) (v - 1);
      bool added = ht.insert(key, key);

      if (added)
      {
        aligned_t my_ind = mt_incr(args->num_edges_added, 1);
        args->srcs[my_ind] = u - 1;
        args->dests[my_ind] = v - 1;
      }
    }
  }

#endif

  void generate_rmat_body(size_type*& srcs, size_type*& dests, size_type& n,
                          size_type& m)
  {
    n = 1 << scale;
    m = edge_factor * n;

    float alpha = 0.05;
    int tolerance = (int) ((1.0 - alpha) * m); // Lower bound on the tolerance.

    //#ifdef DEBUG
    printf("n: %lu, m: %lu, tolerance: %d\n", n, m, tolerance);
    //#endif

    /* First create a Hamiltonian cycle, so that we have a connected graph */
    srcs  = (size_type*) malloc(m * sizeof(size_type));
    dests = (size_type*) malloc(m * sizeof(size_type));

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

  // We need 13 * scale random numbers for adding one edge.  The edges are
  // generated in phases, in order to be able to reproduce the same graph
  // for a given scale value

  // Generate 2^22 (4.1M) edges in each phase on MTA
  //          2^18 (262k) edges in each phase if not on MTA.
  // We fill an array of random numbers of size
  // 13 * scale * num_edges_per_phase in each phase.
#ifdef __MTA__
    size_type num_edges_per_phase = 1 << 22;
#else
    size_type num_edges_per_phase = 1 << 16;
#endif

    if (m < num_edges_per_phase) num_edges_per_phase = m;

    int rand_val_size = 13 * num_edges_per_phase * scale;

    /* Start adding edges */
    double a0 = this->_a0;  // default = 0.57
    double b0 = this->_b0;  // default = 0.19
    double c0 = this->_c0;  // default = 0.19
    double d0 = this->_d0;  // default = 0.05

#if GEN_HAMILTONIAN_CYCLE
    size_type num_edges_added = n;
#else
    size_type num_edges_added = 0;  // JWB: getting rid of the Hamilton cycle
#endif

    xmt_hash_table<int64_t, int64_t> htab((int) (1.2 * tolerance));

    create_edges_loop(num_edges_added, num_edges_per_phase, rand_val_size,
                      n, m, tolerance, srcs, dests, a0, b0, c0, d0, htab);

    m = htab.size();

    if (num_edges_added != htab.size())
    {
      fprintf(stderr, "error: num_edges: %ld, ht.sz: %ld\n",
              num_edges_added, htab.size());
      exit(1);
    }

    // in DIMACS-C version, genEdgeWeights() is called here to set edge weights.
  }

  void run(graph_adapter& g)
  {
    typedef typename graph_traits<graph_adapter>::size_type size_type;

    size_type* srcs;
    size_type* dests;
    size_type n;
    size_type m;

    generate_rmat_body(srcs, dests, n, m);

#ifdef PERMUTE_NODES
    random_permutation(n, m, srcs, dests);
#endif

    init(n, m, srcs, dests, g);

    free(srcs);
    free(dests);
  }

  void create_edges_loop(size_type& num_edges_added,
                         size_type& num_edges_per_phase, int& rand_val_size,
                         size_type& n, size_type& m, int& tolerance,
                         size_type*& srcs, size_type*& dests,
                         const double& a0, const double& b0,
                         const double& c0, const double& d0,
                         xmt_hash_table<int64_t, int64_t>& ht)
  {
    random_value* rv =
      (random_value*) malloc(rand_val_size * sizeof(random_value));
    double* rv_p = (double*) malloc(rand_val_size * sizeof(double));

#ifdef DEBUG
    printf("create_edges_loop(): num_edges_per_phase: %ld\n",
           num_edges_per_phase);
#endif

    size_type num_edges_to_be_added;

    while (num_edges_added <  (size_type) tolerance)
    {
#ifdef DEBUG
      printf("create_edges_loop(): iter: %ld\n", num_edges_added);
#endif

      rand_fill::generate(rand_val_size, rv);

#ifdef USING_QTHREADS
      runloop1_s rl1_args(rv, rv_p);
      if (0 < rand_val_size)
      {
        qt_loop_balance(0, rand_val_size, runloop1, &rl1_args);
      }
#else
      random_value rv_max = rand_fill::get_max();
      #pragma mta assert nodep
      for (int i = 0; i < rand_val_size; i++)
      {
        rv_p[i] = ((double) rv[i]) / rv_max;
      }
#endif

      if (m - num_edges_added < num_edges_per_phase)
      {
        num_edges_to_be_added = m - num_edges_added;
      }
      else
      {
        num_edges_to_be_added = num_edges_per_phase;
      }

#ifdef USING_QTHREADS
      runloop2_s rl2_args(a0, b0, c0, d0, n, rv_p, srcs, dests,
                          num_edges_added, ht, this);
      qt_loop_balance(0, num_edges_to_be_added, runloop2, &rl2_args);
#else
      create_edges(num_edges_to_be_added, num_edges_added, srcs, dests, n, rv_p,
                   a0, b0, c0, d0, ht);
#endif

#ifdef DEBUG
      fprintf(stderr, "%ld ", num_edges_added);
#endif
    }

#ifdef DEBUG
    fprintf(stderr, "\n");
#endif

    free(rv);
    free(rv_p);
  }

  void takeout_dupes(int*& srcs, int*& dests, size_type& num_edges_added,
                     xmt_hash_table<int64_t, int64_t>& htab, int n)
  {
    int* ret_srcs = (int*) malloc(num_edges_added * sizeof(int));
    int* ret_dests = (int*) malloc(num_edges_added * sizeof(int));

    size_type next = 0;
    int dups = 0;

    #pragma mta assert parallel
    for (size_type i = 0; i < num_edges_added; i++)
    {
      int64_t key = srcs[i] * n + dests[i];
      bool added = htab.insert(key, key);
      if (added)
      {
        size_type my_ind = mt_incr(next, 1);
        ret_srcs[my_ind] = srcs[i];
        ret_dests[my_ind] = dests[i];
      }
    }

    free(srcs);
    free(dests);
    srcs = ret_srcs;
    dests = ret_dests;

    num_edges_added = next;
  }

  void create_edges(size_type& num_edges_to_be_added, size_type& next,
                    size_type*& srcs, size_type*& dests,
                    const size_type& n, double*& rv_p,
                    const double& a0, const double& b0,
                    const double& c0, const double& d0,
                    xmt_hash_table<int64_t, int64_t>& ht)
  {
    int failures = 0;

    #pragma mta assert parallel
    for (size_type i = 0; i < num_edges_to_be_added; i++)
    {
      double a = a0;
      double b = b0;
      double c = c0;
      double d = d0;
      size_type u = 1;
      size_type v = 1;
      size_type step = n / 2;

      for (int j = 0; j < scale; j++)
      {
        choose_partition(rv_p[i * scale + j], &u, &v, step, a, b, c, d);
        step = step / 2;
        vary_params(rv_p, (i * scale + j), &a, &b, &c, &d);
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

#ifdef DEBUG
    printf("create_edges(): toadd: %lu, next: %lu, failures: %d\n",
           num_edges_to_be_added, next, failures);
#endif
  }

  void choose_partition(double p, size_type* u, size_type* v, size_type step,
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

  void vary_params(double* rv_p, size_type offset, double* a,
                  double* b, double* c, double* d)
  {
    // Allow a max. of 10% variation.
    double v = 0.10;

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

    double S = *a + *b + *c + *d;

    *a = *a / S;
    *b = *b / S;
    *c = *c / S;
    *d = *d / S;
  }

  void rmat_stats(graph_adapter& g)
  {
    typedef typename graph_traits<graph_adapter>::vertex_descriptor
                     vertex_descriptor;
    typedef typename graph_traits<graph_adapter>::size_type size_type;
    typedef typename graph_traits<graph_adapter>::vertex_iterator
                     vertex_iterator;
    typedef typename graph_traits<graph_adapter>::adjacency_iterator
                     adjacency_iterator;

    size_type num_a = 0, num_b = 0, num_c = 0, num_d = 0;
    vertex_id_map<graph_adapter> vipm = get(_vertex_id_map, g);
    size_type n = num_vertices(g);
    size_type m = num_edges(g);

    vertex_iterator verts = vertices(g);

    #pragma mta assert parallel
    #pragma mta loop future
    for (size_type i = 0; i < n; i++)
    {
      vertex_descriptor v = verts[i];
      size_type sid = get(vipm, v);
      size_type deg = out_degree(v, g);
      adjacency_iterator adj_verts = adjacent_vertices(v, g);

      #pragma mta assert parallel
      for (size_type j = 0; j < deg; j++)
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
  int scale;        // 2^scale vertices
  int edge_factor;  // 2^scale * edge_factor edges to be generated
  double _a0;
  double _b0;
  double _c0;
  double _d0;
};

/****************************************************************************/
/*! \brief A scale-free graph generator based on the R-MAT (recursive matrix)
           algorithm. D. Chakrabarti, Y. Zhan, and C. Faloutsos, "R-MAT: A
           recursive model for graph mining", SIAM Data Mining 2004.

    \author Kamesh Madduri
    \author Greg Mackey (gemacke@sandia.gov)

    The basic idea behind the R-MAT algorithm is to recursively sub-divide
    the adjacency matrix of the graph into four equal-sized partitions, and
    distribute edges within these partitions with unequal probabilities.
    Starting off with an empty adjacency matrix, edges are added one at a
    time. Each edge chooses one of the four partitions with probabilities a,
    b, a, and d, where a + b + c + d = 1. Some noise is added at each stage of
    the recursion, and the a, b, c, d values are renormalized.

    The generator is called with just one input parameter scale. The number
    of vertices is set to 2^scale, and the number of edges to 8*2^scale. The
    values of a, b, c, d are set to 0.45, 0.15, 0.15 and 0.25 respectively.
    This generates two big communities (a, d), with cross-links between them
    (b, c). The recursive nature of the generator results in the "communities
    within communities" effect.

      Nasty settings:  a0 = 0.45;  b0 = 0.15;  c0 = 0.15;  d0 = 0.25;
       Nice settings:  a0 = 0.57;  b0 = 0.19;  c0 = 0.19;  d0 = 0.05;

    The algorithm defaults to the nasty settings.
*/
/****************************************************************************/

template <typename graph_adapter>
void
generate_rmat_graph(graph_adapter& g, int scale, int edge_factor,
                    double a0 = 0.57, double b0 = 0.19,
                    double c0 = 0.19, double d0 = 0.05)
{
  gen_rmat_graph<graph_adapter> grg(scale, edge_factor, a0, b0, c0, d0);
  grg.run(g);
}

}

#endif

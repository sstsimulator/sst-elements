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
/*! \file metrics.hpp

    \brief This code contains the classes used by the graph metrics such as
           assortativity, cluster coefficient, degree degree correlation,
           degree distribution, and modularity.

    \author Jon Berry (jberry@sandia.gov)
    \author Vitus Leung (vjleung@sandia.gov)

    \date 11/2006
*/
/****************************************************************************/

#ifndef MTGL_METRICS_HPP
#define MTGL_METRICS_HPP

#include <cstdio>
#include <climits>
#include <cassert>
#include <cmath>

#include <mtgl/util.hpp>
#include <mtgl/xmt_hash_set.hpp>
#include <mtgl/xmt_hash_table.hpp>
#include <mtgl/psearch.hpp>
#include <mtgl/triangles.hpp>
#include <mtgl/dynamic_array.hpp>

namespace mtgl {

// We'll store the neighbor id's of high-degree vertices in hash sets.
// The following structure will serve as an input to the xmt_hash_set::visit()
// method, which will be called when two high-degree vertices are adjacent.
struct hvis {
  hvis(int sid, int tid, xmt_hash_set<int>* tnghs,
       dynamic_array<triple<int, int, int> >& res) :
    srcid(sid), destid(tid), tneighs(tnghs), result(res) {}

  void operator()(int neigh)
  {
    if (tneighs->member(neigh))
    {
      triple<int, int, int> t(srcid, destid, neigh);
      result.push_back(t);
    }
  }

  int srcid, destid;
  xmt_hash_set<int>* tneighs;
  dynamic_array<triple<int, int, int> >& result;
};

class htvis {
public:
  htvis(int types) : same_type_sum(0), ass_types(types) {}

  void operator()(const int& k, int& v)
  {
    if (k / ass_types == k % ass_types)
    {
      same_type_sum += v;
    }
  }

  int same_type_sum;

private:
  int ass_types;
};

class intvis {
public:
  intvis() : all_types_sum(0){}

  void operator()(int data) { all_types_sum += data; }

  int all_types_sum;
};

/// \brief Describes what happens during a graph search to find the type
///        mixing matrix used to compute assortativity and modularity.
template <typename graph>
class type_mixing_matrix_visitor : public default_psearch_visitor<graph> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  type_mixing_matrix_visitor(vertex_id_map<graph>& vm, int* res, int* typs,
                             int* map, graph& _g, int* subp = 0,
                             int my_subp = 0) :
    result(res), types(typs), subproblem(subp), id2comm(map),
    my_subproblem(my_subp), vid_map(vm), g(_g) {}

  bool visit_test(edge e, vertex v)
  {
    return (!subproblem || subproblem[get(vid_map, v)] == my_subproblem);
  }

  void tree_edge(edge e, vertex src)
  {
    #pragma mta trace "te"
    int st = id2comm[types[get(vid_map, src)]];
    int dt = id2comm[types[get(vid_map, other(e, src, g))]];

    if (st == dt)
    {
      mt_incr(result[0], 1);
    }
    else
    {
      mt_incr(result[1], 1);
    }
  }

  void back_edge(edge e, vertex src) { tree_edge(e, src); }

protected:
  int* result;
  int* types;
  int* subproblem;
  int my_subproblem;
  int* id2comm;         // Given id of leader node, get commun. #
  vertex_id_map<graph>& vid_map;
  graph& g;
};

/// \brief Describes what happens during a graph search to find the
///        degree degree correlation.
template <typename graph>
class degree_degree_correlation_visitor : public default_psearch_visitor<graph>
{
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  degree_degree_correlation_visitor(vertex_id_map<graph>& vm, int** res,
                                    graph& _g, int* subp = 0, int my_subp = 0) :
    result(res), subproblem(subp), my_subproblem(my_subp), vid_map(vm), g(_g) {}

  bool visit_test(edge e, vertex v)
  {
    return (!subproblem || subproblem[get(vid_map, v)] == my_subproblem);
  }

  void tree_edge(edge e, vertex src)
  {
    if (e.first == src)
    {
      #pragma mta trace "te"
      vertex dest = other(e, src, g);

      int log2d = (int) ceil(log(dest->degree() + 1.0) / log(2.0)) + 1;
      int log2s = (int) ceil(log(src->degree() + 1.0) / log(2.0)) + 1;

      mt_incr(result[log2d][log2s], 1);
    }
  }

  void back_edge(edge e, vertex src) { tree_edge(e, src); }

protected:
  int** result;
  int* subproblem;
  int my_subproblem;
  vertex_id_map<graph>& vid_map;
  graph& g;
};

/// \brief Describes what happens during a graph search to find the
///        degree distribution.
template <typename graph>
class degree_distribution_visitor : public default_psearch_visitor<graph> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  degree_distribution_visitor(vertex_id_map<graph>& vm, int* res,
                              int* subp = 0, int my_subp = 0) :
    result(res), subproblem(subp), my_subproblem(my_subp), vid_map(vm) {}

  void pre_visit(vertex v)
  {
    #pragma mta trace "te"
    int log2d = (int) ceil(log(v->degree() + 1.0) / log(2.0)) + 1;
    mt_incr(result[log2d], 1);
  }

  bool visit_test(edge e, vertex v)
  {
    return (!subproblem || subproblem[get(vid_map, v)] == my_subproblem);
  }

protected:
  int* result;
  int* subproblem;
  int my_subproblem;
  vertex_id_map<graph>& vid_map;
};

/// \brief Describes what happens during a graph search to find the
///        connected triples used to find the cluster coefficient.
template <typename graph>
class connected_triples_visitor : public default_psearch_visitor<graph> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  connected_triples_visitor(vertex_id_map<graph>& vm, int* c,
                            int* subp = 0, int my_subp = 0) :
    count(c), subproblem(subp), my_subproblem(my_subp), vid_map(vm) {}

  void pre_visit(vertex v)
  {
    #pragma mta trace "te"
    int sd = v->degree();
    if (sd > 1)
    {
      int act = sd * (sd - 1) / 2;
      mt_incr(*count, act);
    }
  }

  bool visit_test(edge e, vertex v)
  {
    return (!subproblem || subproblem[get(vid_map, v)] == my_subproblem);
  }

protected:
  int* count;
  int* subproblem;
  int my_subproblem;
  vertex_id_map<graph>& vid_map;
};

/// \brief Given a set of community assignments (on nodes) and an edge
///        support, compute the "support variance" -- a measure of how well
///        the community assigment conforms to the support.
template <typename graph>
class support_variance_visitor {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;

  support_variance_visitor(graph& gg, int* comm, double* supp, double& res,
                           vertex_id_map<graph>& vm, edge_id_map<graph>& em) :
    g(gg), result(res), community(comm), support(supp),
    vid_map(vm), eid_map(em) {}

  void operator()(edge_t e)
  {
    int eid = get(eid_map, e);

    vertex_t src = source(e, g);
    vertex_t trg = target(e, g);
    int sid = get(vid_map, src);
    int tid = get(vid_map, trg);

    int same = (community[sid] == community[tid]);
    double diff = same - support[eid];
    double res = mt_readfe(result);

    mt_write(result, res + diff * diff);
  }

protected:
  graph& g;
  double& result;
  int* community;
  double* support;
  vertex_id_map<graph>& vid_map;
  edge_id_map  <graph>& eid_map;
};

/// \brief This is the algorithm object that users will invoke to find the
///        type mixing matrix of a graph.  Used to find assortativity and
///        modularity.
template <typename graph>
class find_type_mixing_matrix {
public:
  find_type_mixing_matrix(graph& gg, int* res, int* typs, int* map,
                          int* subp = 0, int my_subp = 0) :
    g(gg), result(res), types(typs),
    id2comm(map), subproblem(subp), my_subproblem(my_subp) {}

  int run()
  {
    mt_timer mttimer;
    mttimer.start();

#ifdef __MTA__
    mta_resume_event_logging();
    int issues = mta_get_task_counter(RT_ISSUES);
    int memrefs = mta_get_task_counter(RT_MEMREFS);
    int m_nops = mta_get_task_counter(RT_M_NOP);
    int a_nops = mta_get_task_counter(RT_M_NOP);
    int fp_ops = mta_get_task_counter(RT_FLOAT_TOTAL);
#endif

    vertex_id_map<graph> vid_map = get(_vertex_id_map, g);

    type_mixing_matrix_visitor<graph> av(vid_map, result, types,
                                         id2comm, g, subproblem, my_subproblem);
    psearch_high_low<graph, type_mixing_matrix_visitor<graph>,
                     AND_FILTER, UNDIRECTED, 10> psrch(g, av);
    psrch.run();

    mttimer.stop();
    long postticks = mttimer.getElapsedTicks();

#ifdef __MTA__
    issues = mta_get_task_counter(RT_ISSUES) - issues;
    memrefs = mta_get_task_counter(RT_MEMREFS) - memrefs;
    m_nops = mta_get_task_counter(RT_M_NOP)  - m_nops;
    a_nops = mta_get_task_counter(RT_A_NOP)  - a_nops;
    a_nops = mta_get_task_counter(RT_FLOAT_TOTAL) - fp_ops;
#endif

    return postticks;
  }

private:
  graph& g;
  int* result;
  int* types;
  int* id2comm;          // Given leader id, return comm #
  int* subproblem;
  int my_subproblem;
};

/// \brief This is the algorithm object that users will invoke to find the
///        assortativity of a graph.
template <typename graph>
class find_assortativity {
public:
  find_assortativity(graph& gg, double* res, int ntypes) :
    g(gg), result(res), num_types(ntypes) {}

  int run()
  {
    unsigned int ord = num_vertices(g);
    int i;
    int tmm[2];
    int* server = new int[ord];
    int* id_inversemap = new int[ord];

    tmm[0] = tmm[1] = 0;

    #pragma mta assert nodep
    for (i = 0; i < ord; i++)
    {
      server[i] = i;
      id_inversemap[i] = i % num_types;
    }

    find_type_mixing_matrix<graph> ftmm(g, tmm, server, id_inversemap);

    //mta_resume_event_logging();
    int ticks1 = ftmm.run();
    //mta_suspend_event_logging();
    int all = 0, same = 0;

    same = tmm[0];
    all = tmm[0] + tmm[1];
    *result = (double) same / all;

    printf("assortativity: %lf\n", *result);

    return ticks1;
  }

private:
  graph& g;
  double* result;
  int num_types;
};

/// \brief This is the algorithm object that users will invoke to find the
///        degree degree correlation of a graph.
template <typename graph>
class find_degree_degree_correlation {
public:
  find_degree_degree_correlation(graph& gg, double** res,
                                 int* subp = 0, int my_subp = 0) :
    g(gg), result(res), subproblem(subp), my_subproblem(my_subp) {}

  int run()
  {
    mt_timer mttimer;
    mttimer.start();

#ifdef __MTA__
    mta_resume_event_logging();
    int issues = mta_get_task_counter(RT_ISSUES);
    int memrefs = mta_get_task_counter(RT_MEMREFS);
    int m_nops = mta_get_task_counter(RT_M_NOP);
    int a_nops = mta_get_task_counter(RT_M_NOP);
    int fp_ops = mta_get_task_counter(RT_FLOAT_TOTAL);
#endif

    vertex_id_map<graph> vid_map = get(_vertex_id_map, g);

    unsigned int ord = num_vertices(g);
    int size = num_edges(g);
    int maxdegree = (int) ceil(log(2.0 * ord - 1) / log(2.0)) + 1, i, j;

    int** res = (int**) malloc(maxdegree * sizeof(int));
    #pragma mta assert parallel
    for(i = 0; i < maxdegree; ++i)
    {
      res[i] = (int*) calloc(maxdegree, sizeof(int));
    }

    degree_degree_correlation_visitor<graph> ddcv(vid_map, res, g, subproblem,
                                                  my_subproblem);
    psearch_high_low<graph, degree_degree_correlation_visitor<graph>,
                     AND_FILTER, UNDIRECTED, 10> psrch(g, ddcv);
    psrch.run();

    #pragma mta assert nodep
    for(i = 1; i < maxdegree; ++i)
    {
      for(j = 1; j < maxdegree; ++j)
      {
        result[i][j] = (double) res[i][j] / size;
      }
    }

    mttimer.stop();
    long postticks = mttimer.getElapsedTicks();

#ifdef __MTA__
    issues = mta_get_task_counter(RT_ISSUES) - issues;
    memrefs = mta_get_task_counter(RT_MEMREFS) - memrefs;
    m_nops = mta_get_task_counter(RT_M_NOP)  - m_nops;
    a_nops = mta_get_task_counter(RT_A_NOP)  - a_nops;
    a_nops = mta_get_task_counter(RT_FLOAT_TOTAL) - fp_ops;
#endif

    return postticks;
  }

private:
  graph& g;
  double** result;
  int* subproblem;
  int my_subproblem;
};

/// \brief This is the algorithm object that users will invoke to find the
///        degree distribution of a graph.
template <typename graph>
class find_degree_distribution {
public:
  find_degree_distribution(graph& gg, double* res, const char* fname,
                           int* subp = 0, int my_subp = 0) :
    g(gg), result(res), filename(fname),
    subproblem(subp), my_subproblem(my_subp) {}

  int run()
  {
    mt_timer mttimer;
    mttimer.start();

#ifdef __MTA__
    mta_resume_event_logging();
    int issues = mta_get_task_counter(RT_ISSUES);
    int memrefs = mta_get_task_counter(RT_MEMREFS);
    int m_nops = mta_get_task_counter(RT_M_NOP);
    int a_nops = mta_get_task_counter(RT_M_NOP);
    int fp_ops = mta_get_task_counter(RT_FLOAT_TOTAL);
#endif

    vertex_id_map<graph> vid_map = get(_vertex_id_map, g);

    unsigned int ord = num_vertices(g);
    unsigned int m = (int) ceil(log(2.0 * ord - 1) / log(2.0)) + 1;

    int* res = (int*) calloc(m, sizeof(int));

    degree_distribution_visitor<graph> ddv(vid_map, res,
                                           subproblem, my_subproblem);
    psearch_high_low<graph, degree_distribution_visitor<graph>,
                     AND_FILTER, UNDIRECTED, 10> psrch(g, ddv);
    psrch.run();

    FILE* file = fopen(filename, "w");

    for(int i = 0; i < m; i++)
    {
      fprintf(file, "%d %d\n", i, res[i]);
    }

    #pragma mta assert nodep
    for(int i = 0; i < m; i++)
    {
      result[i] = (double) res[i] / ord;
    }

    mttimer.stop();
    long postticks = mttimer.getElapsedTicks();

#ifdef __MTA__
    issues = mta_get_task_counter(RT_ISSUES) - issues;
    memrefs = mta_get_task_counter(RT_MEMREFS) - memrefs;
    m_nops = mta_get_task_counter(RT_M_NOP)  - m_nops;
    a_nops = mta_get_task_counter(RT_A_NOP)  - a_nops;
    a_nops = mta_get_task_counter(RT_FLOAT_TOTAL) - fp_ops;
#endif

    return postticks;
  }

private:
  graph& g;
  double* result;
  const char* filename;
  int* subproblem;
  int my_subproblem;
};

/// \brief This is the algorithm object that users will invoke to find the
///        connected triples of a graph.  Used to find the cluster coefficient
///        of a graph.
template <typename graph>
class find_connected_triples {
public:
  find_connected_triples(graph& gg, int* c, int* subp = 0, int my_subp = 0) :
    g(gg), count(c), subproblem(subp), my_subproblem(my_subp) {}

  int run()
  {
    mt_timer mttimer;
    mttimer.start();

#ifdef __MTA__
    mta_resume_event_logging();
    int issues = mta_get_task_counter(RT_ISSUES);
    int memrefs = mta_get_task_counter(RT_MEMREFS);
    int m_nops = mta_get_task_counter(RT_M_NOP);
    int a_nops = mta_get_task_counter(RT_M_NOP);
    int fp_ops = mta_get_task_counter(RT_FLOAT_TOTAL);
#endif

    vertex_id_map<graph> vid_map = get(_vertex_id_map, g);

    int count2 = 0;

    connected_triples_visitor<graph> ctv(vid_map, &count2,
                                         subproblem, my_subproblem);
    psearch_high_low<graph, connected_triples_visitor<graph>,
                     AND_FILTER, UNDIRECTED, 10> psrch(g, ctv);
    psrch.run();

    *count = count2;

    mttimer.stop();
    long postticks = mttimer.getElapsedTicks();

#ifdef __MTA__
    issues = mta_get_task_counter(RT_ISSUES) - issues;
    memrefs = mta_get_task_counter(RT_MEMREFS) - memrefs;
    m_nops = mta_get_task_counter(RT_M_NOP)  - m_nops;
    a_nops = mta_get_task_counter(RT_A_NOP)  - a_nops;
    a_nops = mta_get_task_counter(RT_FLOAT_TOTAL) - fp_ops;
#endif

    return postticks;
  }

private:
  graph& g;
  int* count;
  int* subproblem;
  int my_subproblem;
};

template <class graph>
class count_triangles : public default_triangles_visitor<graph> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::size_type count_t;
  typedef typename graph_traits<graph>::edge_descriptor edge;

  count_triangles(graph& gg,
                  dynamic_array<triple<count_t, count_t, count_t> >& res,
                  count_t& ct) :
    g(gg), count(ct), result(res) {}

  void operator()(count_t v1, count_t v2, count_t v3) { mt_incr(count, 1); }

private:
  graph& g;
  count_t& count;
  dynamic_array<triple<count_t, count_t, count_t> >& result;
};

/// \brief This is the algorithm object that users will invoke to find the
///        cluster coefficient of a graph.
template <typename graph>
class find_cluster_coefficient {
public:
  find_cluster_coefficient(graph& gg, double* cc) : g(gg), clco(cc) {}

  int run()
  {
#ifdef __MTA__
    mta_resume_event_logging();
    int issues = mta_get_task_counter(RT_ISSUES);
    int memrefs = mta_get_task_counter(RT_MEMREFS);
    int m_nops = mta_get_task_counter(RT_M_NOP);
    int a_nops = mta_get_task_counter(RT_M_NOP);
    int fp_ops = mta_get_task_counter(RT_FLOAT_TOTAL);
#endif

    typedef typename count_triangles<graph>::count_t count_t;

    count_t count = 0;

    dynamic_array<triple<count_t, count_t, count_t> > result;

    count_triangles<graph> ctv(g, result, count);
    find_triangles<graph, count_triangles<graph> > ft(g, ctv);

    int ticks1 = ft.run();

    fprintf(stdout, "found %lu triangles\n", (long unsigned) count);

    int count2;

    find_connected_triples<graph> fct(g, &count2);
    int ticks2 = fct.run();

    fprintf(stdout, "found %d connected triples\n", count2);

    *clco = (double) 3 * count / count2;

#ifdef __MTA__
    issues = mta_get_task_counter(RT_ISSUES) - issues;
    memrefs = mta_get_task_counter(RT_MEMREFS) - memrefs;
    m_nops = mta_get_task_counter(RT_M_NOP) - m_nops;
    a_nops = mta_get_task_counter(RT_A_NOP) - a_nops;
    a_nops = mta_get_task_counter(RT_FLOAT_TOTAL) - fp_ops;
#endif

    return ticks1 + ticks2;
  }

private:
  graph& g;
  double* clco;
};

template <typename graph>
class edge_type_matcher {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;

  edge_type_matcher(graph& gg, int* clust, bool* same_clust,
                    vertex_id_map<graph>& vmap,
                    edge_id_map<graph>& emap, int& mism) :
    g(gg), cluster(clust), same_cluster(same_clust),
    vid_map(vmap), eid_map(emap), mismatches(mism) {}

  void operator()(edge_t e)
  {
    int eid = get(eid_map, e);

    // in this context, 0 -> same cluster
    //                  1 -> diff cluster
    vertex_t v1 = source(e, g);
    vertex_t v2 = target(e, g);
    int v1id = get(vid_map, v1);
    int v2id = get(vid_map, v2);

    bool test_same_cluster = (cluster[v1id] == cluster[v2id]);

    if (test_same_cluster != !same_cluster[eid]) mt_incr(mismatches, 1);
  }

private:
  graph& g;
  vertex_id_map<graph>& vid_map;
  edge_id_map<graph>& eid_map;
  int* cluster;
  bool* same_cluster;
  int& mismatches;
};

template <typename graph>
class edge_hamming_distance {
public:
  edge_hamming_distance(graph& gg, int* clust, bool* same_clust) :
    g(gg), cluster(clust), same_cluster(same_clust) {}

  int run()
  {
    vertex_id_map<graph> vid_map = get(_vertex_id_map, g);
    edge_id_map<graph> eid_map = get(_edge_id_map, g);

    int mismatches = 0;
    edge_type_matcher<graph> etm(g, cluster, same_cluster,
                                 vid_map, eid_map, mismatches);
    visit_edges(g, etm);

    return mismatches;
  }

private:
  graph& g;
  int* cluster;
  bool* same_cluster;
};

template <typename graph>
class support_variance {
public:
  support_variance(graph& gg, int* comm, double* supp) :
    g(gg), community(comm), support(supp) {}

  double run()
  {
    vertex_id_map<graph> vid_map = get(_vertex_id_map, g);
    edge_id_map<graph> eid_map =  get(_edge_id_map, g);

    double sv = 0.0;

    support_variance_visitor<graph> svv(g, community, support,
                                        sv, vid_map, eid_map);
    visit_edges(g, svv);

    return sv;
  }

private:
  graph& g;
  int* community;
  double* support;
};

template <class graph_adapter>
int* degree_distribution(graph_adapter& g, int num_bins = 32)
{
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph_adapter>::vertex_iterator vertex_iterator;

  unsigned int order = num_vertices(g);

  int* count  = new int[num_bins];
  int* degs  = new int[order];

  #pragma mta assert nodep
  for (int i = 0; i < num_bins; i++) count[i] = 0;

  vertex_iterator verts = vertices(g);

  #pragma mta assert nodep
  for (unsigned int i = 0; i < order; i++)
  {
    vertex_t v = verts[i];
    degs[i] = out_degree(v, g);
  }

  for (unsigned i = 0; i < order; i++)
  {
    int log2deg = (int) (log((double) degs[i]) / log(2.0));
    if (log2deg < 0) log2deg = 0;

    int index = (log2deg >= num_bins) ? num_bins - 1 : log2deg;
    count[index]++;
  }

  delete [] degs;

  return count;
}

}

#endif

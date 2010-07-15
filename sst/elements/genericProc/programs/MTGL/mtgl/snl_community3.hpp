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
/*! \file snl_community3.hpp

    \author Jon Berry (jberry@sandia.gov)

    \date 8/21/2008
*/
/****************************************************************************/

#ifndef MTGL_SNL_COMMUNITY3_HPP
#define MTGL_SNL_COMMUNITY3_HPP

#include <cstdio>
#include <climits>
#include <cstdlib>

#include <mtgl/mtgl_adapter.hpp>
#include <mtgl/util.hpp>
#include <mtgl/triangles.hpp>
#include <mtgl/ufl.h>
#include <mtgl/metrics.hpp>
#include <mtgl/rand_fill.hpp>
#include <mtgl/xmt_hash_table.hpp>
#include <mtgl/neighborhoods.hpp>
#include <mtgl/topsort.hpp>
#include <mtgl/visit_adj.hpp>

#define UNIT 1024

#ifdef __MTA__
#include <sys/mta_task.h>
#include <machine/runtime.h>
#endif

namespace mtgl {

template <class graph_adapter>
class edge_server_col_iterator {
public:
  typedef typename graph_adapter::edge_descriptor edge_t;
  typedef typename graph_adapter::vertex_descriptor vertex_t;

  // TODO:  make this class aware of deg_zero_vertices
  edge_server_col_iterator(const graph_adapter& g, int id, int* zdv,
                           int nzdv, int pos = 0) :
    position(pos), g_size(num_edges(g)), eid(id), ga(g),
    vipm(get(_vertex_id_map, g)), order(num_vertices(g)),
    deg_zero_vertices(zdv), num_deg_zero_vertices(nzdv)
  {
    if (eid < g_size)
    {
      last_pos = 1;
    }
    else
    {
      last_pos = 0;        // We'll never iterate, so a sensible
    }                      // program won't invoke the op*.
  }

  bool operator<(const edge_server_col_iterator& cit)
  {
    return position < cit.position;
  }

  bool operator!=(const edge_server_col_iterator& cit)
  {
    return position != cit.position;
  }

  void operator++(int) { position++; }
  int get_position() const { return position; }
  void set_position(int p) { position = p; }
  int operator*()
  {
    int num_original_edges = (g_size - num_deg_zero_vertices) / 2;
    if (eid < num_original_edges)
    {
      edge_t my_edge = get_edge(eid, ga);
      vertex_t g_src = source(my_edge, ga);
      vertex_t g_trg = target(my_edge, ga);
      int g_sid = get(vipm, g_src);
      int g_tid = get(vipm, g_trg);

      if (position == 0)
      {
        return g_sid;
      }
      else if (position == 1)
      {
        return g_tid;
      }
      else
      {
        return -1;
      }
    }
    else if (eid < num_original_edges + num_deg_zero_vertices)
    {
      return deg_zero_vertices[eid - num_original_edges];
    }
    else
    {
      return -1;
    }
  }

private:
  const graph_adapter& ga;
  vertex_id_map<graph_adapter> vipm;
  int position, eid;
  int g_size;
  unsigned int order;
  int last_pos;
  int* deg_zero_vertices;
  int num_deg_zero_vertices;
};

template <class graph_adapter>
class edge_server_val_iterator {
public:
  typedef typename graph_adapter::edge_descriptor edge_t;
  typedef typename graph_adapter::vertex_descriptor vertex_t;

  edge_server_val_iterator(const graph_adapter& g, int id, int pos = 0) :
    position(pos), g_size(num_edges(g)), eid(id), ga(g)
  {
    if (eid < g_size)
    {
      last_pos = 1;
    }
    else
    {
      last_pos = 0;        // We'll never iterate, so a sensible
    }                      // program won't invoke the op*.
  }

  bool operator<(const edge_server_val_iterator& cit)
  {
    return position < cit.position;
  }

  bool operator!=(const edge_server_val_iterator& cit)
  {
    return position != cit.position;
  }

  void operator++(int) { position++; }
  int get_position() const { return position; }
  void set_position(int p) { position = p; }
  int operator*() { return 0; }

private:
  const graph_adapter& ga;
  int position, eid;
  int g_size;
  int last_pos;
};

template <class graph_adapter>
class edge_server_adapter : public graph_adapter {
public:
  typedef typename graph_adapter::vertex_descriptor vertex_descriptor;
  typedef typename graph_adapter::edge_descriptor edge_descriptor;
  typedef typename graph_adapter::edge_descriptor edge_t;
  typedef typename graph_adapter::vertex_iterator vertex_iterator;
  typedef typename graph_adapter::edge_iterator edge_iterator;
  typedef typename graph_adapter::out_edge_iterator out_edge_iterator;
  typedef typename graph_adapter::adjacency_iterator adjacency_iterator;
  typedef edge_server_col_iterator<edge_server_adapter<graph_adapter> >
  column_iterator;
  typedef edge_server_val_iterator<edge_server_adapter<graph_adapter> >
  value_iterator;

  #pragma mta debug level 0
  edge_server_adapter(graph_adapter& g, int* dzv, int ndzv)
    : ga(g), deg_zero_vertices(dzv), num_deg_zero_vertices(ndzv)
  {
    g_order = num_vertices(ga);
    g_size  = num_edges(ga);
    order = g_size + num_deg_zero_vertices;
    size = 2 * g_size + num_deg_zero_vertices;
  }

  int* get_index() const { return 0; }
  unsigned int MatrixRows() const { return order; }
  int num_nonzero() const { return size; }

  edge_t get_edge(int eid) const { return get_edge(eid, ga); }

  int col_index(int i) const
  {
    if (i < g_size)
    {
      return 2 * i;
    }
    else
    {
      return 2 * g_size + (i - g_size);
    }
  }

  column_iterator col_indices_begin(int i) const
  {
    return column_iterator(*this, i, deg_zero_vertices, num_deg_zero_vertices);
  }

  column_iterator col_indices_end(int i) const
  {
    return column_iterator(*this, i, deg_zero_vertices,
                           num_deg_zero_vertices, 1);
  }

  value_iterator col_values_begin(int i) const
  {
    return value_iterator(*this, i);
  }

  value_iterator col_values_end(int i) const
  {
    return value_iterator(*this, i, 1);
  }

  unsigned int get_order() const { return order; }
  int get_size() const { return size; }

private:
  graph_adapter& ga;
  unsigned int order, g_order;
  int size, g_size;
  int* deg_zero_vertices, num_deg_zero_vertices;
};

template <class graph_adapter>
typename graph_traits<edge_server_adapter<graph_adapter> >::edge_iterator
edges(const edge_server_adapter<graph_adapter>& g)
{
  return edges((graph_adapter) g);
}

template <class graph_adapter>
typename graph_traits<edge_server_adapter<graph_adapter> >::out_edge_iterator
out_edges(const typename edge_server_adapter<graph_adapter>::vertex_descriptor& v,
          const edge_server_adapter<graph_adapter>& g)
{
  return out_edges((typename graph_adapter::vertex_descriptor) v, (graph_adapter) g);
}

template <class graph_adapter>
typename graph_traits<edge_server_adapter<graph_adapter> >::vertex_iterator
vertices(const edge_server_adapter<graph_adapter>& g)
{
  return vertices((graph_adapter) g);
}

template <class graph_adapter>
typename graph_traits<edge_server_adapter<graph_adapter> >::adjacency_iterator
adjacent_vertices(
    const typename edge_server_adapter<graph_adapter>::vertex_descriptor& v,
    const edge_server_adapter<graph_adapter>& g)
{
  return adjacent_vertices((typename graph_adapter::vertex_descriptor) v,
                           (graph_adapter) g);
}

template <class graph_adapter>
class vertex_id_map<edge_server_adapter<graph_adapter> > :
  public vertex_id_map<graph_adapter>{
public:
  vertex_id_map(edge_server_adapter<graph_adapter>& ga) :
    vertex_id_map<graph_adapter>(ga) {}

  vertex_id_map<edge_server_adapter<graph_adapter> >() :
    vertex_id_map<graph_adapter>() {}
};

template <class graph_adapter>
class edge_id_map<edge_server_adapter<graph_adapter> > :
  public edge_id_map<graph_adapter>{
public:
  edge_id_map(edge_server_adapter<graph_adapter>& ga) :
    edge_id_map<graph_adapter>(ga) {}

  edge_id_map() : edge_id_map<graph_adapter>() {}
};

template <class graph>
class cost_visitor {
public:
  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge_t;
  typedef xmt_hash_table<int64_t, size_type> hash_table_t;

  cost_visitor(graph& gg, bool* ldrs, double* open,
               hash_table_t& ew, size_type un) :
    g(gg), leaders(ldrs), opening_costs(open),
    vid_map(get(_vertex_id_map, g)), e_weight(ew),
    eid_map(get(_edge_id_map, g)), order(num_vertices(g)), unit(un) {}

  void operator()(edge_t e)
  {
    int eid = get(eid_map, e);
    leaders[eid] = false;

    vertex_t src = source(e, g);
    vertex_t dest = target(e, g);

    int sid = get(vid_map, src);
    int tid = get(vid_map, dest);

    if (sid == tid) return;

    order_pair(sid, tid);

    int64_t key = sid * order + tid;
    double ew = e_weight[key] / unit;

    opening_costs[eid] = 1 - ew;

    //printf("oc(%d,%d) k:%jd, id: %d, 1 - ceil(%lf/%lf=%lf)\n", sid, tid,
    //       key, eid, good_en, en, opening_costs[eid]);
  }

private:
  graph& g;
  vertex_id_map<graph> vid_map;
  edge_id_map<graph> eid_map;
  hash_table_t& e_weight;
  size_type unit;
  bool* leaders;
  int* vn_count;
  unsigned int order;
  double* opening_costs;
};

/*! ********************************************************************
    \class snl_community3
    \brief
 *  ********************************************************************
*/
template <typename graph>
class snl_community3 {
public:
  typedef typename graph_traits<graph>::size_type size_type;
  typedef typename graph_traits<graph>::vertex_iterator vertex_iterator;
  typedef graph Graph;
  typedef xmt_hash_table<int64_t, size_type> hash_table_t;

  snl_community3(graph& gg, hash_table_t& ew, int* res,
                 double* psol = 0, double* supp = 0) :
    g(gg), server(res), primal(psol), support(supp), e_weight(ew) {}

  bool * run()
  {
    mt_timer mttimer;
    mttimer.start();

    printf("snl_community3: start\n"); fflush(stdout);

    vertex_id_map<graph> vid_map = get(_vertex_id_map, g);
    edge_id_map<graph> eid_map   = get(_edge_id_map, g);

    unsigned int order = num_vertices(g);
    unsigned int size = num_edges(g);

    vertex_iterator verts = vertices(g);

    int num_deg_zero = 0;
    #pragma mta assert parallel
    for (int i = 0; i < order; i++)
    {
      if (out_degree(verts[i], g) == 0) mt_incr(num_deg_zero, 1);
    }
    int* deg_zero_vertices = new int[num_deg_zero];
    int pos = 0;
    #pragma mta assert parallel
    for (int i = 0; i < order; i++)
    {
      if (out_degree(verts[i], g) == 0)
      {
        int my_pos = mt_incr(pos, 1);
        deg_zero_vertices[my_pos] = i;
      }
    }
    printf("snl_community3: found zdv\n"); fflush(stdout);
    edge_server_adapter<graph> ega(g, deg_zero_vertices, num_deg_zero);
    printf("snl_community3: init'd adapter\n"); fflush(stdout);

    vertex_id_map<graph> es_vid_map = get(_vertex_id_map, ega);
    edge_id_map<graph> es_eid_map   = get(_edge_id_map, ega);

    hashvis h;

    if (order < 50) print(g);

    int locations = num_vertices(ega);
    int customers = order;
    int nnz =  ega.num_nonzero();
    printf("nnz: %d\n", nnz);

    //weights already set...
    //hash_table_t en_count(int64_t(1.5 * size), true);
    //hash_table_t good_en_count(int64_t(1.5 * size), true);
    //weight_by_neighborhoods<Graph> wbn(g, e_weight);
    //wbn.run(1, good_en_count, en_count);

    int num_leaders = 0;    // will do ufl soon
    bool* leaders = (bool*) malloc(locations * sizeof(bool));
    double* opening_costs = (double*) malloc(locations * sizeof(double));
    double* init_lambda = (double*) malloc(locations * sizeof(double));
    primal = (double*) malloc((locations + nnz) * sizeof(double));

    cost_visitor<Graph> cv(g, leaders, opening_costs, e_weight, UNIT);
    visit_edges(g, cv);
    printf("snl_community3: ran cost_visitor\n"); fflush(stdout);
    #pragma mta assert nodep
    for (int i = size; i < locations; i++)
    {
      opening_costs[i] = 0.0;
    }
    #pragma mta assert nodep
    for (int i = 0; i < locations; i++)
    {
      init_lambda[i] = 1.0;
    }

    printf("calling ufl_module(%d,%d,%d,%d)\n", order, locations, customers, nnz);
    ufl_module<double, edge_server_adapter<graph> >(locations, customers, nnz,
                                                    num_leaders, opening_costs,
                                                    ega, 0.001, leaders,
                                                    server, primal, init_lambda);

    mttimer.stop();
    double sec = mttimer.getElapsedSeconds();
    printf("snl_community time: %lf\n", sec);

/*
  free(servcosts);

  num_leaders = 0;

  for (int i=0; i<ord; i++)
  {
    if (leaders[i])
    {
      num_leaders++;
      printf("leader[%d] = 1\n", i);
    }
  }

  vertex_id_map<graph> vid_map2 = get(_vertex_id_map, g);
  edge_id_map<graph> eid_map2 = get(_edge_id_map, g);
*/
    // ********************************************************
    return leaders; 
  }       // run()

private:
  graph& g;
  hash_table_t& e_weight;
  int* server;
  double*& primal;
  double* support;
};

}

#endif

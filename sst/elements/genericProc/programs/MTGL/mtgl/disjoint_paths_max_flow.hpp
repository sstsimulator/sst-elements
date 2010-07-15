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
/*! \file disjoint_paths_max_flow.hpp

    \brief Performs maxflow getting parallelism by finding disjoint paths.

    \author Brad Mancke

    \date 4/1/2008
*/
/****************************************************************************/

#ifndef MTGL_DISJOINT_PATHS_MAX_FLOW_HPP
#define MTGL_DISJOINT_PATHS_MAX_FLOW_HPP

#include <iostream>
#include <fstream>
#include <climits>
#include <cassert>

#include <mtgl/mtgl_adapter.hpp>
#include <mtgl/rand_fill.hpp>
#include <mtgl/util.hpp>
#include <mtgl/breadth_first_search_mtgl.hpp>
#include <mtgl/dynamic_array.hpp>
#include <mtgl/quicksort.hpp>
//#include <mtgl/xmt_hash_table.hpp>

#ifdef __MTA__
#include <sys/mta_task.h>
#include <machine/runtime.h>
#endif

namespace mtgl {

template <typename graph_adapter>
class ddp_build_visitor : public default_bfs_mtgl_visitor<graph_adapter> {
public:
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_t;

  ddp_build_visitor(graph_adapter& gg, vertex_id_map<graph_adapter> vm,
                    unsigned*& nl) :
    ga(gg), vid_map(vm), node_level(nl) {}

  ddp_build_visitor(const ddp_build_visitor& dbv) :
    ga(dbv.ga), vid_map(dbv.vid_map), node_level(dbv.node_level) {}

  void bfs_root(vertex_t& v) const
  {
    int vid = get(vid_map, v);
    node_level[vid] = 1;
  }

  int visit_test(edge_t &e, vertex_t& src) { return 0; }

  void tree_edge(edge_t& e, vertex_t& src) const
  {
    vertex_t oth = other(e, src, ga);
    int oid = get(vid_map, oth);
    int sid = get(vid_map, src);

    node_level[oid] = node_level[sid] + 1;
  }

  const graph_adapter& ga;
  const vertex_id_map<graph_adapter> vid_map;
  unsigned* node_level;
};

template <typename graph_adapter, typename flow_t>
class ddp_rebuild_visitor : public default_bfs_mtgl_visitor<graph_adapter> {
public:
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_t;

  ddp_rebuild_visitor(const graph_adapter& gg,
                      const vertex_id_map<graph_adapter> vm, int*& nv,
                      unsigned*& nl, flow_t*& fl, flow_t*& cap,
                      const vertex_t& snk) :
    ga(gg), vid_map(vm), node_visited(nv), node_level(nl), flow(fl),
    capacity(cap), sink(snk) {}

  ddp_rebuild_visitor(const ddp_rebuild_visitor& drv) :
    ga(drv.ga), vid_map(drv.vid_map),
    node_visited(drv.node_visited), node_level(drv.node_level),
    flow(drv.flow), capacity(drv.capacity), sink(drv.sink) {}

  void bfs_root(vertex_t& v) const
  {
    int vid = get(vid_map, v);
    node_level[vid] = 1;
  }

  int visit_test(edge_t & e, vertex_t& src)
  {
    int eid = e.get_id();
    vertex_t oth = source(e, ga);
    int retval = 0;
    int sid = get(vid_map, src);

    if (src == oth && flow[eid] != capacity[eid])
    {
      oth = target(e, ga);
      int oid = get(vid_map, oth);
      int val = mt_incr(node_visited[oid], 1);

      if (!val)
      {
        node_level[oid] = node_level[sid] + 1;

        if (oth != sink) retval = 1;
      }

    }
    else if (src != oth && flow[eid])
    {
      int oid = get(vid_map, oth);
      int val = mt_incr(node_visited[oid], 1);

      if (!val)
      {
        node_level[oid] = node_level[sid] + 1;

        if (oth != sink) retval = 1;  // Don't think i need this.
      }
    }

    return retval;
  }

  const graph_adapter& ga;
  const vertex_id_map<graph_adapter> vid_map;
  unsigned* node_level;
  int* node_visited;
  const flow_t* flow;
  const flow_t* capacity;
  const vertex_t& sink;
};

template <typename graph_adapter, typename flow_t>
class mincut_visitor : public default_bfs_mtgl_visitor<graph_adapter> {
public:
  typedef typename graph_traits<graph_adapter>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph_adapter>::edge_descriptor edge_t;

  mincut_visitor(const graph_adapter& gg,
                 const vertex_id_map<graph_adapter>& vm,
                 const edge_id_map<graph_adapter>& em,
                 int* vv1, flow_t* fl, flow_t* cap) :
    ga(gg), vid_map(vm), eid_map(em), v1(vv1), flow(fl), capacity(cap) {}

  mincut_visitor(const mincut_visitor& mv) :
    ga(mv.ga), vid_map(mv.vid_map), eid_map(mv.eid_map), v1(mv.v1),
    flow(mv.flow), capacity(mv.capacity) {}

  void pre_visit(vertex_t& v) const
  {
    int vid = get(vid_map, v);
    v1[vid] = 1;
  }

  int visit_test(edge_t& e, vertex_t& src)
  {
    int eid = get(eid_map, e);
    vertex_t oth = source(e, ga);

    // If e is a forward edge and the flow is less than the capacity, return
    // true.  If e is a back edge and there is flow, return true.  Otherwise,
    // return false.
    int retval = ((src == oth && flow[eid] < capacity[eid]) ||
                  (src != oth && flow[eid] > 0)) ? 1 : 0;

    return retval;
  }

  const graph_adapter& ga;
  const vertex_id_map<graph_adapter>& vid_map;
  const edge_id_map<graph_adapter>& eid_map;
  int* v1;
  flow_t* flow;
  flow_t* capacity;
};

template <typename bid_graph, typename flow_t>
class disjoint_paths_max_flow {
public:
  typedef typename graph_traits<bid_graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<bid_graph>::edge_descriptor edge_t;
  typedef typename graph_traits<bid_graph>::out_edge_iterator out_edge_iter_t;
  typedef typename graph_traits<bid_graph>::in_edge_iterator in_edge_iter_t;
  typedef typename graph_traits<bid_graph>::vertex_iterator vertex_iter_t;
  typedef typename graph_traits<bid_graph>::edge_iterator edge_iter_t;

  disjoint_paths_max_flow(bid_graph& gg, vertex_t& s,
                          vertex_t& t, flow_t*& cap) :
    ga(gg), src(&s), sink(&t), vid_map(get(_vertex_id_map, ga)),
    eid_map(get(_edge_id_map,ga)), total_flow(0), capacity(cap) {}

  ~disjoint_paths_max_flow()
  {
    for (int i = 0; i < num_paths; ++i)
    {
      delete paths[i];
      delete nodes_visited[i];
    }

    delete [] paths;
    delete [] nodes_visited;
//    delete [] nodes_visited_hash;
    delete [] path_created;
    delete [] node_level;
    delete [] pointed_to_sink;
    delete [] flow;
    delete [] edges_visited;
  }

  bool linear_search(int* arr, int arr_size, int value)
  {
    for (int i = 0; i < arr_size; i++)
    {
      if (arr[i] == value) return true;
    }

    return false;
  }

  flow_t run()
  {
    int source_id = get(vid_map, *src);
    int target_id = get(vid_map, *sink);

    mt_timer timer;

    // Get the maximum number of graphs and set the algorithm
    // direction (whether to go from sink to source or source to sink)
    set_algorithm_direction();

    // Init arrays
    init_arrays(10, 20);

    timer.start();

    // This is to run the bfs and mark the depth
    build_tree();

    // The function that runs the parallel dfs
    find_disjoint_paths();

    // Check to make sure we have a connected path
    int keep_going = at_least_one_path_created();

    while (keep_going)
    {
      // Push the flow on the paths
      augment_paths(keep_going);

      // Run parallel dfs again
      find_disjoint_paths();

      // Check if path is created again
      keep_going = at_least_one_path_created();
    }

    timer.stop();
//    std::cerr << "secs: " << timer.getElapsedSeconds() << std::endl;

    return total_flow;
  }

  // It is expected that the user will have preallocated v1 to a size equal
  // to the number of vertices in the graph.  It is a bitfield where each
  // entry indiates to which partition the vertex with the corresponding id
  // belongs.  1 means the vertex belongs to v1, and 0 means the vertex
  // belongs to v2.
  flow_t minimum_cut(int* v1)
  {
    flow_t flow_value = run();

    int order = num_vertices(ga);

    // Initialize v1 to be all 0's.
    memset(v1, 0, order * sizeof(int));

    // The color vector used by bfs to mark vertices its visited.
    int* color = (int*) malloc(order * sizeof(int));
    memset(color, 0, order * sizeof(int));

    // Do a breadth-first search through the residual network of ga starting
    // at the source.  Mark every node that is reachable from the source on
    // the residual network.  These are the nodes in v1.  The remainder of
    // the nodes are in v2.
    mincut_visitor<bid_graph, flow_t> mv(ga, vid_map, eid_map, v1, flow,
                                         capacity);
    breadth_first_search_mtgl<bid_graph, int*,
        mincut_visitor<bid_graph, flow_t>, UNDIRECTED,
        AND_FILTER, 100> bfs(ga, color, mv);

    bfs.run(*src, true);

    free(color);

    return flow_value;
  }

  bool check_balance_constraints()
  {
    unsigned int order = num_vertices(ga);
    int source_id = get(vid_map, *src);
    int sink_id = get(vid_map, *sink);

    vertex_iter_t verts = vertices(ga);

    int num_violations = 0;

    #pragma mta assert parallel
    for (unsigned int i = 0; i < order; i++)
    {
      if (i != source_id && i != sink_id)
      {
        flow_t sum = 0;
        vertex_t s = verts[i];

        int in_deg = in_degree(s, ga);
        in_edge_iter_t in_edgs = in_edges(s, ga);

        #pragma mta assert parallel
        for (int j = 0; j < in_deg; j++)
        {
          edge_t e = in_edgs[j];
          int edge_id = get(eid_map, e);

          mt_incr(sum, flow[edge_id]);

        }

        int out_deg = out_degree(s, ga);
        out_edge_iter_t out_edgs = out_edges(s, ga);

        #pragma mta assert parallel
        for (int j = 0; j < out_deg; j++)
        {
          edge_t e = out_edgs[j];
          int edge_id = get(eid_map, e);

          mt_incr(sum, -flow[edge_id]);
        }

        if (sum != 0)
        {
          std::cerr << "vertex " << i << " violates" << std::endl;
          mt_incr(num_violations, 1);
        }
      }
    }

    std::cerr << "number of balance constraint violators: " << num_violations
              << std::endl;

    return num_violations;
  }

  //#pragma mta no inline
  bool check_to_see_if_optimal()
  {
    unsigned int order = num_vertices(ga);
    int size = num_edges(ga);
    memset(node_level, 0, order * sizeof(unsigned));

    int *node_vis = new int[order];
    memset(node_vis, 0, order * sizeof(int));

    int *result = new int[order];
    #pragma mta assert nodep
    for (unsigned int i = 0; i < order; i++) result[i] = 1;

    int source_id = get(vid_map, *src);
    result[source_id] = 0;

    // Do a BFS from source to sink, but only follow edges if there is
    // a residual flow
    ddp_rebuild_visitor<bid_graph, flow_t>
      drv(ga, vid_map, node_vis, node_level, flow, capacity, *sink);
    breadth_first_search_mtgl<bid_graph, int*,
                              ddp_rebuild_visitor<bid_graph, flow_t>,
                              UNDIRECTED, NO_FILTER, 100>
      bfs(ga, result, drv);

    bfs.run(*src, true);

    bool retval;

    // Need to check the edges going into sink, just not result.
    int sink_id = get(vid_map, *sink);

    if (node_level[sink_id])
    {
      std::cerr << "SOLUTION NOT OPTIMAL: " << node_level[sink_id] << std::endl;
      retval = false;
    }
    else
    {
      std::cerr << "SOLUTION IS OPTIMAL" << std::endl;
      retval = true;
    }

    /* //DEBUG PRINT STATEMENTS
    for (int i = 0; i < order; i++)
    {
      //if (node_level[i]) {
        printf("node: %d, level: %d\n", i, node_level[i]);
        //}
    }

    edge_iter_t edgs = edges(ga);

    for (int i = 0; i < size; i++)
    {
      //if (edge_level[i]) {
        edge_t e = edgs[i];
        vertex_t s = source(e, ga);
        vertex_t t = target(e, ga);
        int sid = get(vid_map, s);
        int tid = get(vid_map, t);
        printf("%d->%d: flow: %d, cap: %d\n", sid, tid, flow[i], capacity[i]);
        //}
    }
    */

    return retval;
  }

  //#pragma mta no inline
  int at_least_one_path_created()
  {
    int n = 0;
    int max_path = 0;
    int sum_path_lengths = 0;

    // Maybe can also do a sum?
    #pragma mta assert parallel
    for (int i = 0; i < num_paths; i++)
    {
      if (path_created[i]) mt_incr(n, 1);
    }

    return n;
  }

  // This sets the way the algorithm runs.  It will run the algorithm from the
  // smaller degree of the source or the sink.
  void set_algorithm_direction()
  {
    int sink_in_degree = in_degree(*sink, ga);
    int source_out_degree = out_degree(*src, ga);

    if (sink_in_degree <= source_out_degree)
    {
      run_from_sink = true;
      num_paths = sink_in_degree;
      algo_start_vertex = sink;
      algo_end_vertex = src;
    }
    else
    {
      run_from_sink = false;
      num_paths = source_out_degree;
      algo_start_vertex = src;
      algo_end_vertex = sink;
    }
  }

  //#pragma mta no inline
  void init_arrays(const random_value min, const random_value max)
  {
    int size = num_edges(ga);
    unsigned int order = num_vertices(ga);

    if (!capacity)
    {
      capacity = new flow_t[size];

      const random_value mod_factor = max - min + 1;

      edge_iter_t edgs = edges(ga);

      #pragma mta assert parallel
      for (int i = 0; i < size; i++)
      {
        edge_t e = edgs[i];
        vertex_t s = source(e, ga);
        vertex_t t = target(e, ga);
        int sid = get(vid_map, s);
        int tid = get(vid_map, t);
        capacity[i] = (flow_t)(((sid+tid) % mod_factor) + min);
      }
    }

    node_level = new unsigned[order];
    memset(node_level, 0, order * sizeof(unsigned));

    flow = new flow_t[size];
    memset(flow, 0, size * sizeof(flow_t));

    edges_visited = new int[size];
    memset(edges_visited, 0, size * sizeof(int));

    paths = new dynamic_array<int>*[num_paths];
    nodes_visited = new dynamic_array<int>*[num_paths];
    //    nodes_visited_hash = new xmt_hash_table<int, int>[num_paths];
    path_created = new bool[num_paths];
    pointed_to_sink = new bool[size];

    #pragma mta assert parallel
    for (int i = 0; i < num_paths; i++)
    {
      // Should see if we can cut this down.
      paths[i] = new dynamic_array<int>(200);
      nodes_visited[i] = new dynamic_array<int>(201);
    }
  }

  //#pragma mta no inline
  void build_tree()
  {
    unsigned int order = num_vertices(ga);
    int size = num_edges(ga);

    int *result = new int[order];
    memset(result, 0, order * sizeof(int));

    int start_vert_id = get(vid_map, *algo_start_vertex);


    ddp_build_visitor<bid_graph> dbv(ga, vid_map, node_level);

    if (run_from_sink)
    {
      breadth_first_search_mtgl<bid_graph, int*, ddp_build_visitor<bid_graph>,
                                DIRECTED, NO_FILTER, 100>
        bfs(ga, result, dbv);

      bfs.run(*algo_end_vertex, true);
    }
    else
    {
      breadth_first_search_mtgl<bid_graph, int*, ddp_build_visitor<bid_graph>,
                                REVERSED, NO_FILTER, 100>
        bfs(ga, result, dbv);

      bfs.run(*algo_end_vertex, true);
    }

    if (!result[start_vert_id])
    {
      std::cerr << "ERROR: SINK IS NOT REACHABLE BY A DIRECTED PATH FROM THE "
                << "SOURCE!" << std::endl;
    }

    //debug, print out levels of the verts
    //for (int i = 0; i < order; i++) {
    //printf("%d: %d\n", i, node_level[i]);
    //}

    delete [] result;
    result = 0;
  }

  void get_eligible_arcs(vertex_t& s, int*& eligible_arcs,
                         int& eligible_arc_size, int& path_id)
  {
    int in_deg = in_degree(s, ga);
    int out_deg = out_degree(s, ga);
    int deg = in_deg + out_deg;

    int deg_cutoff = 500;

    eligible_arcs = new int[deg];
    int *levels = new int[deg];

    in_edge_iter_t in_edgs = in_edges(s, ga);
    out_edge_iter_t out_edgs = out_edges(s, ga);

    eligible_arc_size = 0;

    if (deg > deg_cutoff)
    {
      quicksort<int> qs((*nodes_visited[path_id]).get_data(),
                        (*nodes_visited[path_id]).size());
      qs.run();

      // Go over all edges and filter out the ones we want
      #pragma mta assert parallel
      for (int i = 0; i < deg; i++)
      {
        edge_t e;
        flow_t edge_flow_check;
        int edge_id;

        if (i < in_deg)
        {
          e = in_edgs[i];
          edge_id = get(eid_map, e);

          if (run_from_sink)
          {
            edge_flow_check = capacity[edge_id];
          }
          else
          {
            edge_flow_check = 0;
          }
        }
        else
        {
          e = out_edgs[i - in_deg];
          edge_id = get(eid_map, e);

          if (run_from_sink)
          {
            edge_flow_check = 0;
          }
          else
          {
            edge_flow_check = capacity[edge_id];
          }
        }

        vertex_t tmp_t = other(e, s, ga);
        int tmp_tid = get(vid_map, tmp_t);

        int pos;
        bool node_visited = binary_search((*nodes_visited[path_id]).get_data(),
                                          (*nodes_visited[path_id]).size(),
                                          tmp_tid, pos);

/*
        // Lookup Hash
        int value;
        bool node_visited = nodes_visited_hash[path_id].lookup(tmp_tid, value);

        // Linear Search
        bool node_visited = linear_search((*nodes_visited[path_id]).get_data(),
                                          (*nodes_visited[path_id]).size(),
                                          tmp_tid);
*/

        // Check to see if the edge has some residual, that it's a node
        // that is reachable, and we haven't seen the node before
        if (flow[edge_id] != edge_flow_check &&
            node_level[tmp_tid] && !node_visited)
        {
          int ind = mt_incr(eligible_arc_size, 1);
          eligible_arcs[ind] = edge_id;
          levels[ind] = node_level[tmp_tid];
        }
      }
    }
    else
    {
      quicksort<int> qs((*nodes_visited[path_id]).get_data(),
                        (*nodes_visited[path_id]).size());
      qs.run();

      // Go over all edges and filter out the ones we want
      for (int i = 0; i < deg; i++)
      {
        edge_t e;
        flow_t edge_flow_check;
        int edge_id;

        if (i < in_deg)
        {
          e = in_edgs[i];
          edge_id = get(eid_map, e);

          if (run_from_sink)
          {
            edge_flow_check = capacity[edge_id];
          }
          else
          {
            edge_flow_check = 0;
          }
        }
        else
        {
          e = out_edgs[i - in_deg];
          edge_id = get(eid_map, e);

          if (run_from_sink)
          {
            edge_flow_check = 0;
          }
          else
          {
            edge_flow_check = capacity[edge_id];
          }
        }

        vertex_t tmp_t = other(e, s, ga);
        int tmp_tid = get(vid_map, tmp_t);

        int pos;
        bool node_visited = binary_search((*nodes_visited[path_id]).get_data(),
                                          (*nodes_visited[path_id]).size(),
                                          tmp_tid, pos);

/*
        // Lookup Hash
        int value;
        bool node_visited = nodes_visited_hash[path_id].lookup(tmp_tid, value);

        // Linear Search
        bool node_visited = linear_search((*nodes_visited[path_id]).get_data(),
                                          (*nodes_visited[path_id]).size(),
                                          tmp_tid);
*/

        // Check to see if the edge has some residual, that it's a node
        // that is reachable, and we haven't seen the node before.
        if (flow[edge_id] != edge_flow_check &&
            node_level[tmp_tid] && !node_visited)
        {
          int ind = mt_incr(eligible_arc_size, 1);
          eligible_arcs[ind] = edge_id;
          levels[ind] = node_level[tmp_tid];
        }
      }
    }

    int max_val = 0;

    // MAKE SURE THIS IS A REDUCTION.
    for (int i = 0; i < eligible_arc_size; i++)
    {
      if (levels[i] > max_val) max_val = levels[i];
    }

    // Sort the eligible arcs based on the levels
    bucket_sort_par_cutoff(levels, eligible_arc_size, max_val, eligible_arcs,
                           deg_cutoff);

    delete [] levels;
    levels = 0;
  }

//  #pragma mta no inline
  void find_disjoint_paths_loop_dfs(vertex_t& s, int& path_id)
  {
    int num_possible_paths;
    int in_deg, out_deg;
    bool stop = false;

    if (s == *algo_end_vertex) path_created[path_id] = true;

    edge_iter_t edgs = edges(ga);

    // The main dfs loop
    while (!stop && s != *algo_end_vertex)
    {
      int* eligible_arcs;
      int eligible_arc_size;
      int tsid = get(vid_map, s);

      // Get the arcs for the current vertex that haven't been traveled on
      get_eligible_arcs(s, eligible_arcs, eligible_arc_size, path_id);

      stop = true;
      for (int i = 0; i < eligible_arc_size; i++)
      {
        int edge_id = eligible_arcs[i];
        int val = mt_incr(edges_visited[edge_id], 1);

        if (!val)
        {
          edge_t e = edgs[edge_id];

          if (s == target(e, ga))
          {
            pointed_to_sink[edge_id] = run_from_sink;
          }
          else
          {
            pointed_to_sink[edge_id] = !run_from_sink;
          }

          s = other(e, s, ga);

          int next_node_id = get(vid_map, s);
          int value = 1;
          //nodes_visited_hash[path_id].insert(next_node_id, value);
          (*nodes_visited[path_id]).push_back(next_node_id);

          (*paths[path_id]).push_back(edge_id);
          stop = false;

          if (s == *algo_end_vertex) path_created[path_id] = true;

          break;
        }
      }

      // Go back an edge if there are no eligible edges to go on.  If contract
      // to path length 1, then we stop.
      if (stop)
      {
        if ((*paths[path_id]).size() > 1)
        {
          (*paths[path_id]).pop_back();
          edge_t prev_edge = edgs[(*paths[path_id])[(*paths[path_id]).size()]];

          s = other(prev_edge, s, ga);
          stop = false;
        }
      }

      // Should we just init these once to the max degree of a vert.
      // delete [] directions;
      // directions = 0;
      delete [] eligible_arcs;
      eligible_arcs = 0;
    }
  }

  //#pragma mta no inline
  void find_disjoint_paths()
  {
    unsigned int order = num_vertices(ga);
    int size = num_edges(ga);

    memset(path_created, 0, num_paths * sizeof(bool));
    memset(edges_visited, 0, size * sizeof(int));

    int path_id_iter = 0;
    int algo_start_vert_id = get(vid_map, *algo_start_vertex);

    if (run_from_sink)
    {
      in_edge_iter_t inc_edges = in_edges(*algo_start_vertex, ga);

      #pragma mta assert parallel
      #pragma mta dynamic schedule
      for (int i = 0; i < num_paths; i++)
      {
        edge_t e = inc_edges[i];
        int eid = get(eid_map, e);

        vertex_t s = source(e, ga);
        int sid = get(vid_map, s);

        if (flow[eid] != capacity[eid] && node_level[sid] &&
            s != *algo_start_vertex)
        {

          int path_id = mt_incr(path_id_iter, 1);

          edges_visited[eid] = 1;
          (*paths[path_id]).clear();
          (*paths[path_id]).unsafe_push_back(eid);

          pointed_to_sink[eid] = true;

          (*nodes_visited[path_id]).clear();
          (*nodes_visited[path_id]).unsafe_push_back(algo_start_vert_id);
          (*nodes_visited[path_id]).unsafe_push_back(sid);

          find_disjoint_paths_loop_dfs(s, path_id);
        }
      }
    }
    else
    {
      out_edge_iter_t inc_edges = out_edges(*algo_start_vertex, ga);

      #pragma mta assert parallel
      #pragma mta dynamic schedule
      for (int i = 0; i < num_paths; i++)
      {
        edge_t e = inc_edges[i];
        int eid = get(eid_map, e);

        vertex_t t = target(e, ga);
        int tid = get(vid_map, t);

        if (flow[eid] != capacity[eid] && node_level[tid] &&
            t != *algo_start_vertex)
        {
          int path_id = mt_incr(path_id_iter, 1);

          edges_visited[eid] = 1;
          (*paths[path_id]).clear();
          (*paths[path_id]).unsafe_push_back(eid);

          pointed_to_sink[eid] = true;

          (*nodes_visited[path_id]).clear();
          (*nodes_visited[path_id]).unsafe_push_back(algo_start_vert_id);
          (*nodes_visited[path_id]).unsafe_push_back(tid);

          find_disjoint_paths_loop_dfs(t, path_id);
        }
      }
    }

#if 0
    edge_iter_t edgs = edges(ga);
    for (int i = 0; i < num_paths; i++)
    {
      printf("looking at path %d, size %d, path created %d\n", i,
             (*paths[i]).size(), path_created[i]);
//      if (path_created[i]) {
      for (int j = 0; j < (*paths[i]).size(); j++)
      {
        edge_t e = edgs[(*paths[i])[j]];
        vertex_t s = source(e, ga);
        vertex_t t = target(e, ga);
        int sid = get(vid_map, s);
        int tid = get(vid_map, t);

        printf("%d->%d cap: %d, flow: %d, p_t_r: %d\n", sid, tid,
               capacity[(*paths[i])[j]], flow[(*paths[i])[j]],
               pointed_to_sink[(*paths[i])[j]]);
      }
//      }
    }
#endif
  }


//  #pragma mta no inline
  void augment_paths(int num_contracted_paths)
  {
    // Contract paths.
    dynamic_array<int>** contracted_paths =
        new dynamic_array<int>*[num_contracted_paths];

    int contracted_paths_iter = 0;

    #pragma mta assert parallel
    for (int i = 0; i < num_paths; i++)
    {
      if (path_created[i])
      {
        int ind = mt_incr(contracted_paths_iter, 1);
        contracted_paths[ind] = paths[i];
      }
    }

    flow_t** violations = new flow_t*[num_contracted_paths];
    flow_t* min_vals = new flow_t[num_contracted_paths];

    #pragma mta assert parallel
    for (int i = 0; i < num_contracted_paths; i++)
    {
      violations[i] = new flow_t[contracted_paths[i]->size()];
      min_vals[i] = rand_fill::get_max();
    }

    #pragma mta assert parallel
    for (int i = 0; i < num_contracted_paths; i++)
    {
      #pragma mta assert parallel
      for (int j = 0; j < contracted_paths[i]->size(); j++)
      {
        int eid = (*contracted_paths[i])[j];

        if (pointed_to_sink[eid])
        {
          violations[i][j] = (capacity[eid] - flow[eid]);
        }
        else
        {
          violations[i][j] = flow[eid];
        }
      }
    }

    #pragma mta assert parallel
    for (int i = 0; i < num_contracted_paths; i++)
    {
      #pragma mta assert parallel
      for (int j = 0; j < contracted_paths[i]->size(); j++)
      {
        if (violations[i][j] < min_vals[i]) min_vals[i] = violations[i][j];
      }
    }

    flow_t tmp_total_flow = 0;
    #pragma mta assert parallel
    for (int i = 0; i < num_contracted_paths; i++)
    {
      tmp_total_flow += min_vals[i];
    }

    total_flow += tmp_total_flow;

    #pragma mta assert parallel
    for (int i = 0; i < num_contracted_paths; i++)
    {
      #pragma mta assert parallel
      for (int j = 0; j < contracted_paths[i]->size(); j++)
      {
        int eid = (*contracted_paths[i])[j];

        if (pointed_to_sink[eid])
        {
          flow[eid] += min_vals[i];
        }
        else
        {
          flow[eid] -= min_vals[i];
        }
      }
    }

    #pragma mta assert parallel
    for (int i = 0; i < num_contracted_paths; i++)
    {
      delete [] violations[i];
      violations[i] = 0;
    }

    delete [] contracted_paths;
    delete [] violations;
    violations = 0;
    delete [] min_vals;
    min_vals = 0;
  }

  void print(bool filter = false)
  {
    int size = num_edges(ga);
    edge_iter_t edgs = edges(ga);

    for (int i = 0; i < size; i++)
    {
      edge_t e = edgs[i];
      vertex_t s = source(e, ga);
      vertex_t t = target(e, ga);
      int sid = get(vid_map, s);
      int tid = get(vid_map, t);
      if ((filter && flow[i]) || !filter)
      std::cerr << "edge: " << i << " (" << sid << "->" << tid << "), flow: "
                << flow[i] << ", cap: " << capacity[i] << std::endl;
    }
  }

  void print_iteration_file_init()
  {
    std::ofstream fp;
    fp.open("disjoint_max_flow_iterations.csv", std::ios::out);

    fp << "ID, TO_ID, PATH_ID, FLOW, CAPACITY, ITERATION" << std::endl;

    int size = num_edges(ga);
    edge_iter_t edgs = edges(ga);
    for (int i = 0; i < size; i++)
    {
      edge_t e = edgs[i];
      vertex_t s = source(e, ga);
      vertex_t t = target(e, ga);
      int sid = get(vid_map, s);
      int tid = get(vid_map, t);

      fp << sid << "," << tid << ",0,0,0,0" << std::endl;;
    }

    fp.flush();
    fp.close();
  }

  void print_iteration_to_file(int iteration)
  {
    std::ofstream fp;
    fp.open("disjoint_max_flow_iterations.csv", std::ios::app);

    unsigned int order = num_vertices(ga);
    int size = num_edges(ga);
    edge_iter_t edgs = edges(ga);

    for (int i = 0; i < num_paths; i++)
    {
      if (path_created[i])
      {
        for (int j = 0; j < (*paths[i]).size(); j++)
        {
          int eid = paths[i][j];
          edge_t e = edgs[i];
          vertex_t s = source(e, ga);
          vertex_t t = target(e, ga);
          int sid = get(vid_map, s);
          int tid = get(vid_map, t);

          fp << sid << "," << tid << "," << i << "," << flow[eid] << ","
             << capacity[eid] << "," << iteration << std::endl;;
        }
      }
    }

    fp.flush();
    fp.close();
  }

  void print_source_sink_edges()
  {
    int sid = get(vid_map, *src);
    int sink_id = get(vid_map, *sink);
    int num_out = 0;
    int num_in = 0;

    int out_deg = out_degree(*src, ga);
    out_edge_iter_t out_edgs = out_edges(*src, ga);

    std::cerr << "edges from source: " << sid << ", deg: " << out_deg
              << std::endl;

    #pragma mta assert parallel
    for (int i = 0; i < out_deg; i++)
    {
      edge_t e = out_edgs[i];

      int edge_id = get(eid_map, e);

      vertex_t tmp_t = target(e, ga);
      vertex_t tmp_s = source(e, ga);

      int tmp_tid = get(vid_map, tmp_t);
      int tmp_sid = get(vid_map, tmp_s);
      if (flow[edge_id] != capacity[edge_id])
      {
        //printf("%d: %d->%d, flow: %d, capacity: %d\n", edge_id, tmp_sid,
        //       tmp_tid, flow[edge_id], capacity[edge_id]);
        mt_incr(num_out, 1);
      }
    }

    int in_deg = in_degree(*sink, ga);
    in_edge_iter_t in_edgs = in_edges(*sink, ga);

    std::cerr << "num out not at cap: " << num_out << std::endl
              << "edges to sink: " << sink_id << ", deg: " << in_deg
              << std::endl;

    edge_iter_t edgs = edges(ga);

    #pragma mta assert parallel
    for (int i = 0; i < in_deg; i++)
    {
      edge_t e = in_edgs[i];
      int edge_id = get(eid_map, e);
      //      edge_id = ga.get_cross_cross_index(edge_id);
      e = edgs[edge_id];

      vertex_t tmp_t = target(e, ga);
      vertex_t tmp_s = source(e, ga);

      int tmp_tid = get(vid_map, tmp_t);
      int tmp_sid = get(vid_map, tmp_s);
      if (flow[edge_id] != capacity[edge_id])
      {
        //printf("%d: %d->%d, flow: %d, capacity: %d\n", edge_id, tmp_sid,
        //       tmp_tid, flow[edge_id], capacity[edge_id]);
        mt_incr(num_in, 1);
      }
    }

    std::cerr << "num in not at cap: " << num_in << std::endl;
  }

  void get_vertex_ids(edge_t& e, int& sid, int& tid)
  {
    vertex_t s = source(e, ga);
    vertex_t t = target(e, ga);
    sid = get(vid_map, s);
    tid = get(vid_map, t);
  }

  void get_vertex_ids(int& e, int& sid, int& tid)
  {
    edge_t ege = get_edge(e, ga);
    vertex_t s = source(ege, ga);
    vertex_t t = target(ege, ga);
    sid = get(vid_map, s);
    tid = get(vid_map, t);
  }

private:
  bid_graph& ga;
  vertex_t* src;
  vertex_t* sink;

  unsigned* node_level;
  flow_t*& capacity;
  flow_t* flow;

  bool run_from_sink;
  vertex_t* algo_start_vertex;
  vertex_t* algo_end_vertex;

  int num_paths;
  dynamic_array<int>** paths;
  bool* path_created;
  bool* pointed_to_sink;

  dynamic_array<int>** nodes_visited;
  int* edges_visited;

  flow_t total_flow;

  vertex_id_map<bid_graph> vid_map;
  edge_id_map<bid_graph> eid_map;

  //  xmt_hash_table<int, int>* nodes_visited_hash;
};

}

#endif

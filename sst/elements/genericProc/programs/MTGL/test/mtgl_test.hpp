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
/*! \file mtgl_test.hpp

    \brief A simple driver code to set up tests of mtgl algorithms.

    \author Jon Berry (jberry@sandia.gov)

    \date 12/4/2007
*/
/****************************************************************************/

#ifndef MTGL_MTGL_TEST_HPP
#define MTGL_MTGL_TEST_HPP

#include <cstring>

#include <mtgl/mtgl_boost_property.hpp>
#include <mtgl/util.hpp>
#include <mtgl/dynamic_array.hpp>
#include <mtgl/mtgl_io.hpp>
#include <mtgl/read_dimacs.hpp>
#include <mtgl/generate_mesh_graph.hpp>
#include <mtgl/generate_plod_graph.hpp>
#include <mtgl/generate_rmat_graph.hpp>

namespace mtgl {

template <typename T>
bool find(char** v, T len, const char* key)
{
  T i = 0;

  while (i < len)
  {
    if (strcmp(v[i++], key) == 0) return true;
  }

  return false;
}

#define MAX_ALGS 10

class kernel_test_info {
public:
  ~kernel_test_info() {}

  kernel_test_info() :
    ticks(0),
    freq(0), seconds(0),
    algs((char**) malloc(sizeof(char*) * MAX_ALGS)),
    levels(0),
    num_srcs(1),
    trials(1),
    threads(1),
    st_trials(0), si_walklen(0),
    problem(0),
    curArgIndex(1),
    source(0),
    sink(0),
    vs(-1),
    vt(-1),
    v1(-1), v2(-1), v3(-1), v4(-1),
    k_param(0),
    algCount(0),
    walk_fname(0),
    graph_type(RMAT),
    save_graph(false),
    graph_out_filename(0),
    save_walk(false),
    load_walk(false),
    cutstep(false),
    target_size(1),
    direction(UNDIRECTED),
    alpha(1.0),
    beta(1.0),
    threshold(5000),
    numX(1),
    numY(1),
    numZ(1),
    numSCC(1)
  {
    strcpy(lf, "");
    strcpy(mf, "");
  }

public:
  int assort_types;
  int vb_par;
  int ticks;
  double freq;
  double seconds;
  char** algs;
  int levels;
  int num_srcs;
  int trials;
  int threads;
  int st_trials;
  int si_walklen;
  int problem;
  int curArgIndex;
  dynamic_array<int> weights;
  int source;
  int sink;
  int vs, vt;
  int v1, v2, v3, v4;
  int k_param;
  int algCount;
  char* walk_fname;
  int graph_type;
  char graph_filename[256];
  char aux_filename[256];
  bool save_graph;
  char* graph_out_filename;
  char lf[256];
  char mf[256];
  bool save_walk;
  bool load_walk;
  bool cutstep;
  int target_size;
  int direction;
  double alpha;
  double beta;
  int threshold;
  int numX;
  int numY;
  int numZ;
  int numSCC;

  void process_args(int argc, char* argv[])
  {
    if (argc < 5)
    {
      fprintf(stderr, "usage: kernelTest [-debug] (-bu | -cc | -ch "
              "| -dfs | "
              "-st <trials> | -tri | -is | -mis "
              "| -assort <num_types> | dd | ddc "
              "|-si <walk_len> | -ts | -mod <num_types> | -fc)*"
              " --graph_type <"
              "dimacs|rmat>"
              " --level <levels> "
              " --filename <"
              "dimacs graph filename> "
              " --aux_filename <another_filename> "
              " --graph <Cray graph number> "
              "(0..15)>\n");
      exit(1);
    }

    int curArgIndex = 1;

    while (curArgIndex < argc)
    {
      if (strcmp(argv[curArgIndex], "-cc") == 0)
      {
        algs[algCount] = (char*) malloc(sizeof(char) * 3);
        strcpy(algs[algCount++], "cc");
      }
      else if (strcmp(argv[curArgIndex], "-sv") == 0)
      {
        algs[algCount] = (char*) malloc(sizeof(char) * 3);
        strcpy(algs[algCount++], "sv");
      }
      else if (strcmp(argv[curArgIndex], "-bu") == 0)
      {
        algs[algCount] = (char*) malloc(sizeof(char) * 3);
        strcpy(algs[algCount++], "bu");
      }
      else if (strcmp(argv[curArgIndex], "-dfs") == 0)
      {
        algs[algCount] = (char*) malloc(sizeof(char) * 4);
        strcpy(algs[algCount++], "dfs");
      }
      else if (strcmp(argv[curArgIndex], "-ch") == 0)
      {
        algs[algCount] = (char*) malloc(sizeof(char) * 3);
        strcpy(algs[algCount++], "ch");
      }
      else if (strcmp(argv[curArgIndex], "-ds") == 0)
      {
        algs[algCount] = (char*) malloc(sizeof(char) * 3);
        strcpy(algs[algCount++], "ds");
      }
      else if (strcmp(argv[curArgIndex], "-sssp_et") == 0)
      {
        algs[algCount] = (char*) malloc(sizeof(char) * 16);
        strcpy(algs[algCount++], "sssp_et");
      }
      else if (strcmp(argv[curArgIndex], "-sollin") == 0)
      {
        algs[algCount] = (char*) malloc(sizeof(char) * 10);
        strcpy(algs[algCount++], "sollin");
      }
      else if (strcmp(argv[curArgIndex], "-scc") == 0)
      {
        algs[algCount] = (char*) malloc(sizeof(char) * 4);
        strcpy(algs[algCount++], "scc");
      }
      else if (strcmp(argv[curArgIndex], "-debug") == 0)
      {
        algs[algCount] = (char*) malloc(sizeof(char) * 6);
        strcpy(algs[algCount++], "debug");
      }
      else if (strcmp(argv[curArgIndex], "-ts") == 0)
      {
        algs[algCount] = (char*) malloc(sizeof(char) * 3);
        strcpy(algs[algCount++], "ts");
      }
      else if (strcmp(argv[curArgIndex], "-tri") == 0)
      {
        algs[algCount] = (char*) malloc(sizeof(char) * 4);
        strcpy(algs[algCount++], "tri");
      }
      else if (strcmp(argv[curArgIndex], "-clco") == 0)
      {
        algs[algCount] = (char*) malloc(sizeof(char) * 5);
        strcpy(algs[algCount++], "clco");
      }
      else if (strcmp(argv[curArgIndex], "-pd") == 0)
      {
        algs[algCount] = (char*) malloc(sizeof(char) * 3);
        strcpy(algs[algCount++], "pd");
      }
      else if (strcmp(argv[curArgIndex], "-vb") == 0)
      {
        algs[algCount] = (char*) malloc(sizeof(char) * 3);
        strcpy(algs[algCount++], "vb");
        vb_par = atoi(argv[++curArgIndex]);
      }
      else if (strcmp(argv[curArgIndex], "-is") == 0)
      {
        algs[algCount] = (char*) malloc(sizeof(char) * 3);
        strcpy(algs[algCount++], "is");
      }
      else if (strcmp(argv[curArgIndex], "-mis") == 0)
      {
        algs[algCount] = (char*) malloc(sizeof(char) * 4);
        strcpy(algs[algCount++], "mis");
      }
      else if (strcmp(argv[curArgIndex], "-assort") == 0)
      {
        algs[algCount] = (char*) malloc(sizeof(char) * 7);
        strcpy(algs[algCount++], "assort");
        assort_types = atoi(argv[++curArgIndex]);
        if (assort_types == 1) assort_types = 2;
      }
      else if (strcmp(argv[curArgIndex], "-dd") == 0)
      {
        algs[algCount] = (char*) malloc(sizeof(char) * 3);
        strcpy(algs[algCount++], "dd");
      }
      else if (strcmp(argv[curArgIndex], "-ddc") == 0)
      {
        algs[algCount] = (char*) malloc(sizeof(char) * 4);
        strcpy(algs[algCount++], "ddc");
      }
      else if (strcmp(argv[curArgIndex], "-mod") == 0)
      {
        algs[algCount] = (char*) malloc(sizeof(char) * 4);
        strcpy(algs[algCount++], "mod");
        assort_types = atoi(argv[++curArgIndex]);
        if (assort_types == 1) assort_types = 2;
      }
      else if (strcmp(argv[curArgIndex], "-fc") == 0)
      {
        algs[algCount] = (char*) malloc(sizeof(char) * 3);
        strcpy(algs[algCount++], "fc");
      }
      else if (strcmp(argv[curArgIndex], "-scc2") == 0)
      {
        algs[algCount] = (char*) malloc(sizeof(char) * 5);
        strcpy(algs[algCount++], "scc2");
      }
      else if (strcmp(argv[curArgIndex], "-scc3") == 0)
      {
        algs[algCount] = (char*) malloc(sizeof(char) * 5);
        strcpy(algs[algCount++], "scc3");
      }
      else if (strcmp(argv[curArgIndex], "-scc4") == 0)
      {
        algs[algCount] = (char*) malloc(sizeof(char) * 5);
        strcpy(algs[algCount++], "scc4");
      }
      else if (strcmp(argv[curArgIndex], "-scc_dfs") == 0)
      {
        algs[algCount] = (char*) malloc(sizeof(char) * 8);
        strcpy(algs[algCount++], "scc_dfs");
      }
      else if (strcmp(argv[curArgIndex], "-st") == 0)
      {
        algs[algCount] = (char*) malloc(sizeof(char) * 3);
        strcpy(algs[algCount++], "st");
        st_trials = atoi(argv[++curArgIndex]);
      }
      else if (strcmp(argv[curArgIndex], "-stsearch") == 0)
      {
        /* Short path search */
        algs[algCount] = (char*) malloc(sizeof(char) * 9);
        strcpy(algs[algCount++], "stsearch");
        //st_trials = atoi(argv[++curArgIndex]);
      }
      else if (strcmp(argv[curArgIndex], "-knn") == 0)
      {
        algs[algCount] = (char*) malloc(sizeof(char) * 4);
        strcpy(algs[algCount++], "knn");
      }
      else if (strcmp(argv[curArgIndex], "-csg") == 0)
      {
        /* Connection Subgraphs */
        algs[algCount] = (char*) malloc(sizeof(char) * 4);
        strcpy(algs[algCount++], "csg");
      }
      else if (strcmp(argv[curArgIndex], "-sio") == 0)
      {
        algs[algCount] = (char*) malloc(sizeof(char) * 4);
        strcpy(algs[algCount++], "sio");
        si_walklen = atoi(argv[++curArgIndex]);
      }
      else if (strcmp(argv[curArgIndex], "-si") == 0)
      {
        algs[algCount] = (char*) malloc(sizeof(char) * 3);
        strcpy(algs[algCount++], "si");
        si_walklen = atoi(argv[++curArgIndex]);
      }
      else if (strcmp(argv[curArgIndex], "--source") == 0)
      {
        source = atoi(argv[++curArgIndex]);
      }
      else if (strcmp(argv[curArgIndex], "--sink") == 0)
      {
        sink = atoi(argv[++curArgIndex]);
      }
      else if (strcmp(argv[curArgIndex], "-saveWalk") == 0)
      {
        walk_fname = argv[++curArgIndex];
        save_walk = true;
      }
      else if (strcmp(argv[curArgIndex], "-loadWalk") == 0)
      {
        walk_fname = argv[++curArgIndex];
        load_walk = true;
      }
      else if (strcmp(argv[curArgIndex], "--save_graph") == 0)
      {
        graph_out_filename = argv[++curArgIndex];
        save_graph = true;
      }
      else if (strcmp(argv[curArgIndex], "-c") == 0)
      {
        cutstep = true;
      }
      else if (strcmp(argv[curArgIndex], "-lf") == 0)
      {
        strcpy(lf, argv[++curArgIndex]);
      }
      else if (strcmp(argv[curArgIndex], "-mf") == 0)
      {
        strcpy(mf, argv[++curArgIndex]);
      }
      else if (strcmp(argv[curArgIndex], "--threshold") == 0)
      {
        threshold = atoi(argv[++curArgIndex]);
      }
      else if (strcmp(argv[curArgIndex], "--level") == 0)
      {
        levels = atoi(argv[++curArgIndex]);
      }
      else if (strcmp(argv[curArgIndex], "--graph") == 0)
      {
        problem = atoi(argv[++curArgIndex]);
      }
      else if (strcmp(argv[curArgIndex], "--aux_filename") == 0)
      {
        strcpy(aux_filename, argv[++curArgIndex]);
      }
      else if (strcmp(argv[curArgIndex], "--filename") == 0)
      {
        strcpy(graph_filename, argv[++curArgIndex]);
      }
      else if (strcmp(argv[curArgIndex], "--graph_type") == 0)
      {
        char gt[256];
        strcpy(gt, argv[++curArgIndex]);
        if (strcmp(gt, "plod") == 0)
        {
          graph_type = PLOD;
        }
        else if (strcmp(gt, "rmat") == 0)
        {
          graph_type = RMAT;
        }
        else if (strcmp(gt, "mesh") == 0)
        {
          graph_type = MESH;
        }
        else if (strcmp(gt, "rand") == 0)
        {
          graph_type = RAND;
        }
        else if (strcmp(gt, "dimacs") == 0)
        {
          graph_type = DIMACS;
        }
        else if (strcmp(gt, "snapshot") == 0)
        {
          graph_type = SNAPSHOT;
        }
        else
        {
          fprintf(stderr, "illegal graph type: %s\n", gt);
          exit(1);
        }
      }
      else if (strcmp(argv[curArgIndex], "--direction") == 0)
      {
        char ds[256];
        strcpy(ds, argv[++curArgIndex]);

        if(strcmp(ds, "directed") == 0)
        {
          direction = DIRECTED;
        }
        else if(strcmp(ds, "reversed") == 0)
        {
          direction = REVERSED;
        }
        else if(strcmp(ds, "undirected") == 0)
        {
          direction = UNDIRECTED;
        }
        else
        {
          fprintf(stderr, "illegal direction type: %s\n",ds);
          exit(1);
        }
      }
      else if (strcmp(argv[curArgIndex], "--alpha") == 0)
      {
        alpha = atof(argv[++curArgIndex]);
      }
      else if (strcmp(argv[curArgIndex], "--beta") == 0)
      {
        beta = atof(argv[++curArgIndex]);
      }
      else if (strcmp(argv[curArgIndex], "--graph_filename") == 0)
      {
        strcpy(graph_filename, argv[++curArgIndex]);
      }
      else if (strcmp(argv[curArgIndex], "--vs") == 0)
      {
        vs = atoi(argv[++curArgIndex]);
      }
      else if (strcmp(argv[curArgIndex], "--vt") == 0)
      {
        vt = atoi(argv[++curArgIndex]);
      }
      else if (strcmp(argv[curArgIndex], "--v1") == 0)
      {
        v1 = atoi(argv[++curArgIndex]);
      }
      else if (strcmp(argv[curArgIndex], "--v2") == 0)
      {
        v2 = atoi(argv[++curArgIndex]);
      }
      else if (strcmp(argv[curArgIndex], "--v3") == 0)
      {
        v3 = atoi(argv[++curArgIndex]);
      }
      else if (strcmp(argv[curArgIndex], "--v4") == 0)
      {
        v4 = atoi(argv[++curArgIndex]);
      }
      else if (strcmp(argv[curArgIndex], "--trials") == 0)
      {
        trials = atoi(argv[++curArgIndex]);
      }
      else if (strcmp(argv[curArgIndex], "--threads") == 0)
      {
        threads = atoi(argv[++curArgIndex]);
      }
      else if (strcmp(argv[curArgIndex], "--num_srcs") == 0)
      {
        num_srcs = atoi(argv[++curArgIndex]);
      }
      else if (strcmp(argv[curArgIndex], "--k") == 0)
      {
        k_param = atoi(argv[++curArgIndex]);
      }
      else if(strcmp(argv[curArgIndex], "--target_size") == 0)
      {
        target_size = atoi(argv[++curArgIndex]);
      }
      else if(strcmp(argv[curArgIndex], "--numX") == 0)
      {
        numX = atoi(argv[++curArgIndex]);
      }
      else if(strcmp(argv[curArgIndex], "--numY") == 0)
      {
        numY = atoi(argv[++curArgIndex]);
      }
      else if(strcmp(argv[curArgIndex], "--numZ") == 0)
      {
        numZ = atoi(argv[++curArgIndex]);
      }
      else if(strcmp(argv[curArgIndex], "--numSCC") == 0)
      {
        numSCC = atoi(argv[++curArgIndex]);
      }
      curArgIndex++;
    }
  }

  template <class graph_adapter>
  void gen_graph(graph_adapter& ga)
  {
#ifdef __MTA__
    mta_set_trace_limit(10000000000);
    mta_reserve_task_event_counter(3, (CountSource) RT_FLOAT_TOTAL);
    mta_reserve_task_event_counter(2, (CountSource) RT_M_NOP);
    mta_reserve_task_event_counter(1, (CountSource) RT_A_NOP);
    int procs = mta_nprocs();
    freq = mta_clock_freq();
    int mhz = freq / 1000000;
    fprintf(stderr, "%d processors, %d MHz clock\n\n", procs, mhz);
#else
    freq = 1000000;
    srand48(0);  // can't specify a seed in MTA prand; awkward situation
#endif

    typedef typename graph_traits<graph_adapter>::size_type size_type;

    if (find<int>(algs, algCount, "debug"))
    {
      size_type num_verts = 6;
      size_type num_edges = 8;

      size_type* srcs = (size_type*) malloc(sizeof(size_type) * num_edges);
      size_type* dests = (size_type*) malloc(sizeof(size_type) * num_edges);

      srcs[0] = 0; dests[0] = 1;
      srcs[1] = 0; dests[1] = 2;
      srcs[2] = 1; dests[2] = 2;
      srcs[3] = 1; dests[3] = 3;
      srcs[4] = 2; dests[4] = 4;
      srcs[5] = 3; dests[5] = 4;
      srcs[6] = 3; dests[6] = 5;
      srcs[7] = 4; dests[7] = 5;

      init(num_verts, num_edges, srcs, dests, ga);
    }
    else if (graph_type == PLOD)
    {
      generate_plod_graph(ga, problem, alpha, beta);
    }
    else if (graph_type == MESH)
    {
      generate_mesh_graph(ga, numX, numY, numZ, numSCC);
    }
    else if (graph_type == RMAT)
    {
      int edgeFactor = 8;
      generate_rmat_graph(ga, problem, edgeFactor);
    }
    else if (graph_type == DIMACS)
    {
      if (!graph_filename)
      {
        printf("For Dimacs, you have to enter a file name\n");
        return;
      }

      read_dimacs(ga, graph_filename, weights);
    }
    else if (graph_type == SNAPSHOT)
    {
      if (!graph_filename)
      {
        printf("For Snapshot, you have to enter a file name\n");
        return;
      }

      std::string srcs_fname(graph_filename);
      std::string dests_fname(graph_filename);

      srcs_fname += ".srcs";
      dests_fname += ".dests";

      read_binary(ga, srcs_fname.c_str(), dests_fname.c_str());
    }
  }
};

}

#endif

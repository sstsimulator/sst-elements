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
/*! \file test_find_triangles.cpp

    \author Jon Berry (jberry@sandia.gov)

    \date 12/4/2007
*/
/****************************************************************************/

#include "mtgl_test.hpp"

#include <mtgl/util.hpp>
#include <mtgl/triangles.hpp>
#include <mtgl/static_graph_adapter.hpp>
#include <mtgl/dynamic_array.hpp>

using namespace mtgl;

double get_freq()
{
#ifdef __MTA__
  double freq = mta_clock_freq();
#else
  double freq = 1000000;
#endif

  return freq;
}

template <class graph>
class count_triangles : public default_triangles_visitor<graph> {
public:
  typedef typename graph_traits<graph>::vertex_descriptor vertex_t;
  typedef typename graph_traits<graph>::edge_descriptor edge;
  count_triangles(graph& gg, dynamic_array<triple<int, int, int> >& res,
                  int& ct) :
    g(gg), count(ct), result(res) {}

  void operator()(vertex_t v1, vertex_t v2, vertex_t v3)
  {
//    result.push_back(triple<int,int,int>(v1->id, v2->id, v3->id));
    mt_incr(count, 1);
  }

private:
  int& count;
  graph& g;
  dynamic_array<triple<int, int, int> >& result;
};

template <typename graph>
void test_find_triangles(graph& g)
{
  int count = 0;
  dynamic_array<triple<int, int, int> > result;
  count_triangles<graph> ctv(g, result, count);
  find_triangles<graph, count_triangles<graph> > ft(g, ctv);

  int ticks1 = ft.run();

  fprintf(stdout, "found %d triangles\n", count);
  fprintf(stderr, "RESULT: find_triangles %lu (%6.2lf,0)\n", num_edges(g),
          ticks1 / get_freq());
}

int main(int argc, char* argv[])
{
  if (argc < 5)
  {
    fprintf(stderr, "usage: %s [-debug] "
            " [-tri] "
            " --graph_type <dimacs|cray> "
            " --level <levels> --graph <Cray graph number> "
            " --filename <dimacs graph filename> "
            " [<0..15>]\n"
            , argv[0]);
    exit(1);
  }

  static_graph_adapter<directedS> ga;

  kernel_test_info kti;
  kti.process_args(argc, argv);
  kti.gen_graph(ga);

  if (find(kti.algs, kti.algCount, "tri"))
  {
    test_find_triangles<graph_adapter>(ga);
  }
}

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
/*! \file shiloach_vishkin.hpp

    \author Jon Berry (jberry@sandia.gov)

    \date 4/22/2005
*/
/****************************************************************************/

#ifndef MTGL_SHILOACH_VISHKIN_HPP
#define MTGL_SHILOACH_VISHKIN_HPP

#include <climits>
#include <cstdio>

#include <mtgl/closure.hpp>

namespace mtgl {

typedef quadruple<int, int, int, int*> QuadSV;

struct default_sv_visitor {
  void init_round() const { }
  void graft(int rank, int c) const { }
  void hook(int gparent, int child) const { }
};

template <typename edgeMap, typename sv_visitor>
class ShiloachVishkin {
public:
  typedef mclosure<ShiloachVishkin<edgeMap, sv_visitor>, QuadSV&, int>
          svqclosure_t;
  typedef mclosure<ShiloachVishkin<edgeMap, sv_visitor>, int, int>
          sviclosure_t;

  ShiloachVishkin(int* c, sv_visitor vis, int ecount,
                  int& lcount, bool& m_work);

  ~ShiloachVishkin();

  void graftRead(QuadSV& p, int dummy = 0) const;
  void graftWrite(QuadSV& p, int dummy = 0) const;
  void hookRead(int i, int dummy = 0) const;
  void hookWrite(int i, int dummy = 0) const;
  void vertexVisit(int i);
  void run(edgeMap& edgs);
  void printBuffer();
  void printC();

  sv_visitor visitor;
  svqclosure_t readGraftClosure;
  svqclosure_t writeGraftClosure;
  sviclosure_t readHookClosure;
  sviclosure_t writeHookClosure;
  int& leader_count;
  int edge_count;
  QuadSV* buffer;
  int* C;
  bool& more_work;
};

template <typename edgeMap, typename sv_visitor>
ShiloachVishkin<edgeMap, sv_visitor>::ShiloachVishkin(
  int* comp, sv_visitor vis, int ecount, int& lcount, bool& m_work) :
    visitor(vis),  C(comp), more_work(m_work),
    edge_count(ecount), leader_count(lcount),
    readGraftClosure(
     mem_fun(this, &mtgl::ShiloachVishkin<edgeMap, sv_visitor>::graftRead), 0),
    writeGraftClosure(
     mem_fun(this, &mtgl::ShiloachVishkin<edgeMap, sv_visitor>::graftWrite), 0),
    readHookClosure(
     mem_fun(this, &mtgl::ShiloachVishkin<edgeMap, sv_visitor>::hookRead), 0),
    writeHookClosure(
     mem_fun(this, &mtgl::ShiloachVishkin<edgeMap, sv_visitor>::hookWrite), 0)
{
  buffer = (QuadSV*) malloc(ecount * sizeof(QuadSV));
}

template <typename edgeMap, typename sv_visitor>
ShiloachVishkin<edgeMap, sv_visitor>::~ShiloachVishkin()
{
  delete buffer;
}

template <typename edgeMap, typename sv_visitor>
void ShiloachVishkin<edgeMap, sv_visitor>::printBuffer()
{
  for (int i = 0; i < edge_count; i++)
  {
    printf("buffer[%d]: <%d,%d,%d>\n", i, buffer[i].first,
           buffer[i].second, buffer[i].third);
  }
}

template <typename edgeMap, typename sv_visitor>
void ShiloachVishkin<edgeMap, sv_visitor>::printC()
{
  for (int i = 0; i < leader_count; i++) printf("C[%d]: %d\n", i, C[i]);
}

template <typename edgeMap, typename sv_visitor>
void ShiloachVishkin<edgeMap, sv_visitor>::graftRead(QuadSV& p, int d) const
{
  int id = p.first, i = p.second, j = p.third;

  buffer[id].first = C[i];
  buffer[id].second = C[j];
  buffer[id].third = C[C[j]];
  buffer[id].fourth = &C[C[j]];
}

template <typename edgeMap, typename sv_visitor>
void ShiloachVishkin<edgeMap, sv_visitor>::graftWrite(QuadSV& p, int d) const
{
  int id = p.first, i = p.second, j = p.third;
  int ci = buffer[id].first;
  int cj = buffer[id].second;
  int ccj = buffer[id].third;
  int* dest = buffer[id].fourth;

  if (ci < cj && cj == ccj)
  {
    if (ci < *dest)
    {
      *dest = ci;
      visitor.graft(ccj, ci);
      more_work = true;
    }
  }
}

template <typename edgeMap, typename sv_visitor>
void ShiloachVishkin<edgeMap, sv_visitor>::hookRead(int i, int dummy) const
{
  buffer[i].second = C[i];
  buffer[i].third = C[C[i]];
}

template <typename edgeMap, typename sv_visitor>
void ShiloachVishkin<edgeMap, sv_visitor>::hookWrite(int i, int dummy) const
{
  int ci = buffer[i].second;
  int cci = buffer[i].third;

  if (cci < ci)
  {
    C[i] = cci;
    visitor.hook(ci, cci);
    more_work = true;
  }
}

template <typename edgeMap, typename sv_visitor>
void ShiloachVishkin<edgeMap, sv_visitor>::run(edgeMap& sg)
{
  while (more_work)
  {
    #pragma mta trace "SV"
    more_work = false;

    #pragma mta assert parallel
    for (int i = 0; i < edge_count; i++) readGraftClosure(sg[i]);

    #pragma mta assert parallel
    for (int i = 0; i < edge_count; i++) writeGraftClosure(sg[i]);

    #pragma mta assert parallel
    for (int i = 0; i < leader_count; i++) readHookClosure(i);

    #pragma mta assert parallel
    for (int i = 0; i < leader_count; i++) writeHookClosure(i);
  }
}

}

#endif

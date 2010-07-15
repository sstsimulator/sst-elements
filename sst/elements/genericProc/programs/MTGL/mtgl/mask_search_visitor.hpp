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
/*! \file mask_search_visitor.hpp

    \brief Defines the visitor used by mask_search.hpp.

    \author Brad Mancke

    \date 12/11/2007
*/
/****************************************************************************/

#ifndef MTGL_MASK_SEARCH_VISITOR_HPP
#define MTGL_MASK_SEARCH_VISITOR_HPP

#include <cstdio>
#include <climits>

#include <mtgl/util.hpp>
#include <mtgl/mask_search.hpp>

namespace mtgl {

template <typename adjlist, typename rdist_type, typename sol_type,
          typename fix_type, typename fcost_type>
class ss_mask_visitor : public default_mask_search_visitor<adjlist> {
public:
  typedef typename adjlist::column_iterator column_iterator;

  ss_mask_visitor(adjlist _d, rdist_type rd, sol_type sln, fix_type fx,
                  fcost_type fc, int nlocations) :
    _dist(_d), rdist(rd), sol(sln), fix(fx), fcost(fc), nloc(nlocations),
    num_open(0), value(0), sum_fcosts(0)
  {
    sum = new double[nloc];
  }

  void pre_visit(int src) const { sum[src] = 0; }
  int start_test(int src) const { return true; }
  int visit_test(int src, int dest) const { return true; }

  void tree_edge(int src, int dest) const
  {
    if (rdist[dest] < 0.0) sum += rdist[dest];
    sol[nloc + dest] = 0;
  }

  // Update neighbors.
  void finish_vertex(int src)
  {
    int xi;

    if (fix.size() > 0)
    {
      if (fix[src] == 0)
      {
        xi = 0;
      }
      else if (fix[src] == 1)
      {
        xi = 1;
      }
      else if ( fcost[src] + sum[src] >= 0. )
      {
        xi = 0;
      }
      else
      {
        xi = 1;
      }
    }
    else
    {
      if ( fcost[src] + sum[src] >= 0. )
      {
        xi = 0;
      }
      else
      {
        xi = 1;
      }
    }

    sol[src] = xi;

    double cost_incr = xi * fcost[src];
    sum_fcosts += cost_incr;

    double tmp_value = 0.0;

    if (xi == 1)
    {
      mt_incr(num_open, 1);
      int i_index = _dist.col_index(src);
      int next_index = _dist.col_index(src + 1);
      column_iterator begin_cols = _dist.col_indices_begin(src);
      column_iterator end_cols   = _dist.col_indices_end(src);
      column_iterator ptr1 = begin_cols;

      #pragma mta serial // parallel / parallel future
      for (int k = i_index; k < next_index; k++)
      {
        if ( rdist[k] < 0. )
        {
          ptr1.set_position(k - i_index);
          int j = *ptr1;
          sol[nloc + k] = (j + 1);  // Remember who's served.
          tmp_value += rdist[k];
        }
      }
    }

    value += tmp_value;
  }

private:
  // Do I have to make sum an array?
  double* sum;
  int nloc;
  double value;
  double sum_fcosts;
  adjlist _dist;
  rdist_type rdist;
  sol_type sol;
  fix_type fix;
  fcost_type fcost;
  int num_open;
};

}

#endif

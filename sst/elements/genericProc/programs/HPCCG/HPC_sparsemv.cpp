
//@HEADER
// ************************************************************************
// 
//               HPCCG: Simple Conjugate Gradient Benchmark Code
//                 Copyright (2006) Sandia Corporation
// 
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
// 
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//  
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//  
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
// Questions? Contact Michael A. Heroux (maherou@sandia.gov) 
// 
// ************************************************************************
//@HEADER

/////////////////////////////////////////////////////////////////////////

// Routine to compute matrix vector product y = Ax where:
// First call exchange_externals to get off-processor values of x

// A - known matrix 
// x - known vector
// y - On exit contains Ax.

/////////////////////////////////////////////////////////////////////////

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cassert>
#include <string>
#include <cmath>
#include "HPC_sparsemv.hpp"

#ifdef USING_QTHREADS
#include <qthread/qthread.h>
#include <qthread/qloop.h>
#define MT_LOOP_CHUNK_OUTER 20000
#define MT_LOOP_CHUNK_INNER 100

#if defined(PIM_MATVEC) && PIM_MATVEC==1
#include <ppcPimCalls.h>
#endif

struct HPC_sparsemv_o_args
{
    HPC_Sparse_Matrix * const A;
    const double * const x;
    double * const y;
    HPC_sparsemv_o_args(HPC_Sparse_Matrix * const a, const double * const x, double * const y) :
	A(a), x(x), y(y) {}
};

static void HPC_sparsemv_outerloop(qthread_t *me, const size_t startat, const size_t stopat, void * arg)
{
    const double * const * const ptr_to_vals_in_row((((struct HPC_sparsemv_o_args*)arg)->A)->ptr_to_vals_in_row);
    const int * const * const ptr_to_inds_in_row((((struct HPC_sparsemv_o_args*)arg)->A)->ptr_to_inds_in_row);
    const int * const nnz_in_row((((struct HPC_sparsemv_o_args*)arg)->A)->nnz_in_row);
    const double * const x(((struct HPC_sparsemv_o_args *)arg)->x);

    for (size_t i = startat; i < stopat; ++i) {
	double sum = 0.0;
	const unsigned int cur_nnz = nnz_in_row[i];
	const double * const cur_vals = ptr_to_vals_in_row[i];
	const int * const cur_inds = ptr_to_inds_in_row[i];
#if defined(PIM_MATVEC) && PIM_MATVEC==1
	int result = 0;
	do {
	  sum = 0.0;
	  result = PIM_MatVec(0,cur_nnz,cur_vals,x,cur_inds, &sum);
	} while (result == 0);
	// throttle (currently hard coded to 7)
	while (PIM_OutstandingMR() > 7) {;}
#else
	/* this inner-loop is too small to be worth parallelizing for now */
	sum = 0.0;
	for (unsigned int j=0; j<cur_nnz; j++)
	  sum += cur_vals[j] * x[cur_inds[j]];
	//printf(" sum should be %f\n", sum);
#endif
	((struct HPC_sparsemv_o_args*)arg)->y[i] = sum;
    }
#if defined(PIM_MATVEC) && PIM_MATVEC==1
    // make sure everything is done before we move on
    while (PIM_OutstandingMR() > 0) {;}
#endif
}
#endif

int HPC_sparsemv( HPC_Sparse_Matrix *A, 
		 const double * const x, double * const y)
{

  const int nrow = (int) A->local_nrow;

#ifdef USING_QTHREADS
  extern int tcount;
  if (tcount > 0) {
      struct HPC_sparsemv_o_args arg(A, x, y);
      qt_loop_balance(0, nrow, HPC_sparsemv_outerloop, &arg);
  } else
#endif
  for (int i=0; i< nrow; i++)
    {
      double sum = 0.0;
      const double * const cur_vals = 
     (const double *) A->ptr_to_vals_in_row[i];

      const int    * const cur_inds = 
     (const int    *) A->ptr_to_inds_in_row[i];

      const int cur_nnz = (int) A->nnz_in_row[i];
#if defined(PIM_MATVEC) && PIM_MATVEC==1
      sum = 0.0;
      for (int j=0; j< cur_nnz; j++) {
	/*printf(" j=%d  cur_inds[j] = %d\n", j, cur_inds[j]);
	  printf(" %f * %f\n", cur_vals[j], x[cur_inds[j]]);*/
	sum += cur_vals[j]*x[cur_inds[j]];
      }
      printf("sum should be %f\n", sum); sum = 0.0;

      int result = 0;
      do {
	sum = 0.0;
	//printf("MatVec: 0,%d,%p,%p,%p,%p\n",cur_nnz,cur_vals,x,cur_inds,&sum);
	result = PIM_MatVec(0,cur_nnz,cur_vals,x,cur_inds, &sum);
      } while (result == 0);
#else
      for (int j=0; j< cur_nnz; j++)
          sum += cur_vals[j]*x[cur_inds[j]];
#endif
      y[i] = sum;
    }
  return(0);
}

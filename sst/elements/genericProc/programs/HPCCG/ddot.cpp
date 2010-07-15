
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

// Routine to compute the dot product of two vectors where:

// n - number of vector elements (on this processor)

// x, y - input vectors

// residual - pointer to scalar value, on exit will contain result.

/////////////////////////////////////////////////////////////////////////

#include "ddot.hpp"

#ifdef USING_QTHREADS
#include <qthread/qthread.h>
#include <qthread/qloop.h>

static void ddot_square(qthread_t *me, const size_t startat, const size_t stopat, const double * const x, void *ret)
{
    double sum = x[startat] * x[startat];
    for (size_t i = startat+1; i < stopat; i++) {
	sum += x[i] * x[i];
    }
    *(double*)ret = sum;
}

struct ddot_mult_args
{
    const double * const x;
    const double * const y;
    ddot_mult_args(const double * const X, const double * const Y) :
	x(X), y(Y) {}
};

static void ddot_mult(qthread_t *me, const size_t startat, const size_t stopat, void * arg, void *ret)
{
    const double * const x(((struct ddot_mult_args *)arg)->x);
    const double * const y(((struct ddot_mult_args *)arg)->y);
    double sum = x[startat] * y[startat];
    for (size_t i = startat + 1; i < stopat; i++) {
	sum += x[i] * y[i];
    }
    *(double *)ret = sum;
}
#endif

int ddot (const int n, const double * const x, const double * const y, 
	  double * const result, double & time_allreduce)
{  
  double local_result = 0.0;
#ifdef USING_QTHREADS
  extern int tcount;
  if (tcount > 0) {
      if (y==x) {
	  qt_loopaccum_balance(0, n, sizeof(double), &local_result, (qt_loopr_f)ddot_square, (void*)x, qt_dbl_add_acc);
      } else {
	  struct ddot_mult_args arg(x,y);
	  qt_loopaccum_balance(0,n,sizeof(double),&local_result,ddot_mult,&arg,qt_dbl_add_acc);
      }
  } else
#endif
  if (y==x)
    for (int i=0; i<n; i++) local_result += x[i]*x[i];
  else
    for (int i=0; i<n; i++) local_result += x[i]*y[i];

#ifdef USING_MPI
  // Use MPI's reduce function to collect all partial sums
  double t0 = mytimer();
  double global_result = 0.0;
  MPI_Allreduce(&local_result, &global_result, 1, MPI_DOUBLE, MPI_SUM, 
                MPI_COMM_WORLD);
  *result = global_result;
  time_allreduce += mytimer() - t0;
#else
  *result = local_result;
#endif

  return(0);
}

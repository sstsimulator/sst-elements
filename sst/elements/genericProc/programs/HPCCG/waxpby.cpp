
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

// Routine to compute the update of a vector with the sum of two 
// scaled vectors where:

// w = alpha*x + beta*y

// n - number of vector elements (on this processor)

// x, y - input vectors

// alpha, beta - scalars applied to x and y respectively.

// w - output vector.

/////////////////////////////////////////////////////////////////////////

#include "waxpby.hpp"

#ifdef USING_QTHREADS
#include <qthread/qthread.h>
#include <qthread/qloop.h>

struct three_args
{
    const double * const x;
    const double * const y;
    const double ba;
    double * const w;
    three_args(const double * const X, const double BA, const double * const Y, double * const W) :
	x(X), y(Y), ba(BA), w(W) {}
};

static void waxpby_noalpha(qthread_t *me, const size_t startat, const size_t stopat, void *arg)
{
    const double * const x(((struct three_args *)arg)->x);
    const double beta(((struct three_args *)arg)->ba);
    const double * const y(((struct three_args *)arg)->y);
    double * const w(((struct three_args *)arg)->w);
    for (size_t i = startat; i < stopat; ++i) {
	w[i] = x[i] + beta * y[i];
    }
}

static void waxpby_nobeta(qthread_t *me, const size_t startat, const size_t stopat, void *arg)
{
    const double alpha(((struct three_args *)arg)->ba);
    const double * const x(((struct three_args *)arg)->x);
    const double * const y(((struct three_args *)arg)->y);
    double * const w(((struct three_args *)arg)->w);
    for (size_t i = startat; i < stopat; ++i) {
	w[i] = alpha * x[i] + y[i];
    }
}

struct four_args
{
    const double * const x;
    const double * const y;
    const double beta;
    const double alpha;
    double * const w;
    four_args(const double * const X, const double BETA, const double ALPHA, const double * const Y, double * const W) :
	x(X), y(Y), beta(BETA), alpha(ALPHA), w(W) {}
};

static void waxpby_both(qthread_t *me, const size_t startat, const size_t stopat, void *arg)
{
    const double alpha(((struct four_args *)arg)->alpha);
    const double beta(((struct four_args *)arg)->beta);
    const double * const x(((struct four_args *)arg)->x);
    const double * const y(((struct four_args *)arg)->y);
    double * const w(((struct four_args *)arg)->w);
    for (size_t i = startat; i < stopat; ++i) {
	w[i] = alpha * x[i] + beta * y[i];
    }
}
#endif

int waxpby (const int n, const double alpha, const double * const x, 
	    const double beta, const double * const y, 
		     double * const w)
{  
#ifdef USING_QTHREADS
    extern int tcount;
    if (tcount > 0) {
	if (alpha == 1.0) {
	    struct three_args args(x,beta,y,w);
	    qt_loop_balance(0, n, waxpby_noalpha, &args);
	} else if (beta == 1.0) {
	    struct three_args args(x,alpha,y,w);
	    qt_loop_balance(0, n, waxpby_nobeta, &args);
	} else {
	    struct four_args args(x,beta,alpha,y,w);
	    qt_loop_balance(0, n, waxpby_both, &args);
	}
    } else
#endif
  if (alpha==1.0)
    for (int i=0; i<n; i++) w[i] = x[i] + beta * y[i];
  else if(beta==1.0)
    for (int i=0; i<n; i++) w[i] = alpha * x[i] + y[i];
  else 
    for (int i=0; i<n; i++) w[i] = alpha * x[i] + beta * y[i];

  return(0);
}

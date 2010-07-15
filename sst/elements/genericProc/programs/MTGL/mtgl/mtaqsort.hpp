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
/*! \file mtaqsort.hpp

    \brief Implements a quicksort algorithm with a parallel partitioner
           based on the technique outlined by Simon Kahan and a C
           implementation contributed by John Feo.  THIS STRATEGY
           DOESN'T WORK WELL IN PRACTICE.  IF SORTING IN PLACE DOESN'T
           MATTER, USE bucket_sort in UTILS.H.

    \author William McLendon (wcmclen@sandia.gov)

    \date 2/22/2007
*/
/****************************************************************************/

#ifndef MTGL_MTAQSORT_HPP
#define MTGL_MTAQSORT_HPP

#ifndef ASCENDING
#  define ASCENDING 0
#endif
#ifndef DESCENDING
#  define DESCENDING 1
#endif

/// \brief Quicksort with a parallel partitioning step.  Sorts an
///        array of values in either ASCENDING or DESCENDING order.
///        Also can mirror sorting into a shadow array as well.
template <typename ATYPE, int dir = ASCENDING, typename BTYPE = int,
          int NUMTHREADS = 70>
class mtaqsort {
public:
  mtaqsort(ATYPE* A, int size, BTYPE* B = 0) : _A(A), _size(size), B_(B) {}
  ~mtaqsort() {}

  void run() { __sort(_A, _size, B_); }

protected:
  void __sort(ATYPE* A, int size, BTYPE* B = 0)
  {
    if (size <= 1) return;
    if (size == 2)
    {
      if (dir == ASCENDING)
      {
        if (A[0] > A[1])
        {
          swap(A, A + 1);
          if (B) swap(B, B + 1);
        }
      }
      else
      {
        if (A[0] < A[1])
        {
          swap(A, A + 1);
          if (B) swap(B, B + 1);
        }
      }
      return;
    }

    int pp;
    int up;

    pivot(A, size, &pp, &up, B);

    ATYPE* A_up = A + up + 1;
    BTYPE* B_up = 0;
    if (B) B_up = B + up + 1;

#ifdef __MTA__
    if ((pp >= 256) || (size - up - 1 >= 256))
    {
      future int left;
      future left(A, pp, B) {__sort(A, pp, B);
                             return 1; }
      __sort(A_up, size - up - 1, B_up);
      touch(&left);
    }
    else
    {
      __sort(A, pp, B);
      __sort(A_up, size - up - 1, B_up);
    }
#else
    __sort(A, pp, B);
    __sort(A_up, size - up - 1, B_up);
#endif
  }

  /*****************************************************************
   * Pivot
   *
   *   aaaaaaaappppbbbbbbbccccccc          a < pivot
   *   ^       ^   ^     ^      ^          p = pivot
   *   0      pp  lp    up     N-1         b = not looked at yet
   *                                       c > pivot
   *****************************************************************/
  void pivot(ATYPE* A, int size, int* PP, int* UP, BTYPE* B = 0)
  {
    int minpp = size - 1;
    int up = size - 1;
    int maxup = 0;
    int lp = 0;
    int pp = 0;

    ATYPE pvt;
    if (size > 1000)
    {
      pvt = median_of_three(A, size, B);
    }
    else
    {
      pvt = (A[lp] + A[up]) / 2;
    }

    if (size >= 5 * NUMTHREADS)
    {
      #pragma mta loop future
      #pragma mta assert no dependence
      for (int i = 0; i < NUMTHREADS; i++)
      {
        int lp = i;
        int pp = i;
        int up = (size - 1) - ((size - i - 1) % NUMTHREADS);

        while (lp <= up)
        {
          if ((dir == ASCENDING  && A[lp] < pvt) ||
              (dir == DESCENDING && A[lp] > pvt))
          {
            swap(A + lp, A + pp);
            if (B) swap(B + lp, B + pp);

            lp += NUMTHREADS;
            pp += NUMTHREADS;
          }
          else if (A[lp] == pvt)
          {
            lp += NUMTHREADS;
          }
          else
          {
            swap(A + lp, A + up);
            if (B) swap(B + lp, B + up);

            up -= NUMTHREADS;
          }
        }

        if (pp < minpp) minpp = pp;
        if (up > maxup) maxup = up;
      }

      lp = pp = minpp;
      up = maxup;
    }

    /* Data between minpp and maxup is not partitioned correctly */
    while (lp <= up)
    {
      if ((dir == ASCENDING  && A[lp] < pvt) ||
          (dir == DESCENDING && A[lp] > pvt))
      {
        swap(A + lp, A + pp);
        if (B) swap(B + lp, B + pp);

        lp++;
        pp++;
      }
      else if (A[lp] == pvt)
      {
        lp++;
      }
      else
      {
        swap(A + lp, A + up);
        if (B) swap(B + lp, B + up);

        up--;
      }
    }

    *PP = pp;
    *UP = up;
  }

  #pragma mta inline
  template <typename U>
  void swap(U* a, U* b)
  {
    if (*a == *b) return;

    U temp;
    temp = *a;
    *a = *b;
    *b = temp;
  }

  #pragma mta inline
  ATYPE median_of_three(ATYPE* A, int size, BTYPE* B)
  {
    int l = 0;
    int m = size / 2;
    int r = size - 1;

    if (dir == ASCENDING)
    {
      if (A[l] > A[m])
      {
        swap(A + l, A + m);
        if (B) swap(B + l, B + m);
      }
      if (A[m] > A[r])
      {
        swap(A + m, A + r);
        if (B) swap(B + m, B + r);
      }
      if (A[l] > A[m])
      {
        swap(A + l, A + m);
        if (B) swap(B + l, B + m);
      }
    }
    else
    {
      if (A[l] < A[m])
      {
        swap(A + l, A + m);
        if (B) swap(B + l, B + m);
      }
      if (A[m] < A[r])
      {
        swap(A + m, A + r);
        if (B) swap(B + m, B + r);
      }
      if (A[l] < A[m])
      {
        swap(A + l, A + m);
        if (B) swap(B + l, B + m);
      }
    }

    return(A[m]);
  }

private:
  ATYPE* _A;
  BTYPE* B_;
  int _size;
};

#endif

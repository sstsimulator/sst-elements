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
/*! \file serqsort.hpp

    \brief A serial quicksort algorithm.

    \author William McLendon (wcmclen@sandia.gov)

    \date 2/22/2007
*/
/****************************************************************************/

#ifndef MTGL_SERQSORT_HPP
#define MTGL_SERQSORT_HPP

#ifndef ASCENDING
#define ASCENDING 0
#endif
#ifndef DESCENDING
#define DESCENDING 1
#endif

/// \brief Implements a Quicksort routine with the capability to concurrently
///        sort a 2nd array in a mirrored fashion with the original array.
 ///       This quicksort implementation will sort the array in-place.
template<typename ATYPE, int dir=ASCENDING, typename BTYPE=int>
class serqsort {
public:
  serqsort(ATYPE* A, int size, BTYPE* B = NULL) : _A(A), B_(B), _size(size) {}
  ~serqsort() {}

  void run() { __run(0, _size-1); }

protected:
  void __run(int lo, int hi)
  {
    int i = lo;
    int j = hi;
    int size = hi - lo + 1;

    ATYPE piv;

    if (lo >= hi)
    {
      return;
    }
    else if (lo + 1 == hi)
    {
      if ((dir==DESCENDING && _A[lo] < _A[hi]) ||
          (dir==ASCENDING  && _A[lo] > _A[hi]))
      {
        swap(_A + lo, _A + hi);

        if (B_) swap(B_ + lo, B_ + hi);
      }
    }
    else
    {
      if (size > 10)
      {
        BTYPE* B_lo = NULL;

        if (B_) B_lo = B_ + lo;

        piv = median_of_three(_A + lo, size, B_lo);
      }
      else
      {
        piv = _A[(lo + hi) / 2];
      }

      do {
        if (dir == ASCENDING)
        {
          while (_A[i] < piv) i++;
          while (_A[j] > piv) j--;
        }
        else
        {
          while (_A[i] > piv) i++;
          while (_A[j] < piv) j--;
        }

        if (i <= j)
        {
          swap(_A + i, _A + j);

          if (B_) swap(B_ + i, B_ + j);

          i++;
          j--;
        }
      } while (i <= j);

      if (lo < j) __run(lo, j);
      if (i < hi) __run(i, hi);
    }
  }

  template<typename __U>
  void swap(__U* a, __U* b)
  {
    __U tmp = *a;
    *a = *b;
    *b = tmp;
  }

  ATYPE median_of_three(ATYPE* A, int size, BTYPE* B = NULL)
  {
    int l = 0;
    int m = size / 2;
    int r = size - 1;

    if (dir == ASCENDING)
    {
      if (A[l] > A[r])
      {
        swap(A + l, A + r);
        if (B) swap(B + l, B + r);
      }

      if (A[m] > A[l])
      {
        swap(A + m, A + r);
        if (B) swap(B + m, B + r);
      }

      if (A[l] > A[r])
      {
        swap(A + l, A + r);
        if (B) swap(B + l, B + r);
      }
    }
    else
    {
      if (A[l] < A[r])
      {
        swap(A + l, A + r);
        if (B) swap(B + l, B + r);
      }

      if (A[m] < A[l])
      {
        swap(A + m, A + r);
        if (B) swap(B + m, B + r);
      }

      if (A[l] < A[r])
      {
        swap(A + l, A + r);
        if (B) swap(B + l, B + r);
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

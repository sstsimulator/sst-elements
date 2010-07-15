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
/*! \file quicksort.hpp

    \brief The driver file for quicksort that chooses between the serial
           and parallel algorithms.

    \author William McLendon (wcmclen@sandia.gov)

    \date 2/22/2007
*/
/****************************************************************************/

#ifndef MTGL_QUICKSORT_HPP
#define MTGL_QUICKSORT_HPP

#include <mtgl/serqsort.hpp>
#include <mtgl/mtaqsort.hpp>

#ifndef ASCENDING
#define ASCENDING  0
#endif
#ifndef DESCENDING
#define DESCENDING 1
#endif
#ifndef UNORDERED
#define UNORDERED  2
#endif


/*! \brief Quicksort driver class.  Will use the mtaqsort routine when on
           the MTA machine and the serial version otherwise.

    The quicksort implemented in mtaqsort will work on non-MTA systems but is
    not as efficient as the serqsort version for those systems.

    Allows the sorting of an array of values in ASCENDING or DESCENDING order
    with or without the reorder being mirrored into a shadow array.
 */
template <typename ATYPE, int dir = ASCENDING, typename BTYPE = int,
          int NUMTHREADS = 70>
class quicksort {
public:
  /*! \brief Constructor.

      \param A Array of values to be sorted.
      \param size Size of the array.
      \param B Shadow array to be ordered with A, defaults to NULL
               if omitted.
   */
  quicksort(ATYPE* A, int size, BTYPE* B = 0) : _A(A), B_(B), _size(size) {}

  ~quicksort() {}

  /// Executes the sort against the data.
  void run()
  {
#ifdef __MTA__
    mtaqsort<ATYPE, dir, BTYPE, NUMTHREADS> sorter(_A, _size, B_);
#else
    serqsort<ATYPE, dir, BTYPE> sorter(_A, _size, B_);
#endif

    sorter.run();
  }


  /*! \brief Performs a validation on the resultant array and returns a value
             corresponding to the ordered status of the array.  Returns
             ASCENDING, DESCENDING, or UNORDERED.
   */
  int validate()
  {
    int ret = dir;

    if (dir == ASCENDING)
    {
      #pragma mta assert no dependence
      for (int i = 1; i < _size; i++)
      {
        if (_A[i - 1] > _A[i]) ret = UNORDERED;
      }
    }
    else
    {
      #pragma mta assert no dependence
      for (int i = 1; i < _size; i++)
      {
        if (_A[i - 1] < _A[i]) ret = UNORDERED;
      }
    }

    return(ret);
  }

private:
  ATYPE* _A;
  BTYPE* B_;
  int _size;
};

#endif

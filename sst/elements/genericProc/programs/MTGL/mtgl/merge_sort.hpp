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
/*! \file merge_sort.hpp

    \brief Performs a merge sort in parallel.

    \author Greg Mackey (gemacke@sandia.gov)

    \date 5/5/2009
*/
/****************************************************************************/

#ifndef MTGL_MERGESORT_HPP
#define MTGL_MERGESORT_HPP

#include <mtgl/util.hpp>

/****************************************************************************/
/*! \brief Performs a merge sort in parallel.

    Running time: O(n log n).
    Space utilized: 2n + 6n / block_size

    This is a parallel version of merge sort designed to work well on the XMT
    or any massively multithreaded architecture with a large shared memory.
    The function is templated to work with any data type with the requirement
    that the data type must have the '<' operator defined.

    Merge sort works as follows.  The array a is divided into a sequence of
    arrays of size starting with 2 and doubling at each iteration until there
    is only one array.  Each array represents two half arrays that need to be
    merged.  Each half array is a list of sorted data, so their merger results
    in a sorted list of data.  The final result is a single sorted list.

    While there are lots of arrays, merge sort as written provides a lot of
    parallelism.  However, as the number of arrays decreases, so does the
    parallelism.  In fact, the last iteration of the algorithm is serial as
    only a single merge is performed.

    This implementation of the algorithm utilizes the fact that each half
    array can be divided into independent blocks that can be merged in
    parallel.  This is accomplished by picking a pivot in one half array and
    finding the corresponding pivot in the other array such that no elements
    at or to the right of the pivot in the second array are less than any
    elements to the left of the pivot in the first array.  Thus, we have two
    independent blocks to merge.

    The alogrithm guarantees that there are a minimum of n / (2 * block_size)
    pairs of blocks to merge at every level.  The first part that implements
    normal merge sort starts with n / 2 pairs of blocks and decreases by a
    factor of 2 until there are n / (2 * block_size) pairs of blocks.  The
    second part tries to keep the number of pairs of blocks near
    n / (2 * block_size), but it can vary between n / (2 * block_size) and
    n / block_size.

    The algorithm works as follows.

     1) Pick a block_size.
     2) Perform normal merge sort, going over the arrays in parallel, until
        the half arrays grow to the size of two blocks.
     3) At this point start dividing the half arrays into independent blocks
        that can be merged in parallel.
     4) Go through the arrays in parallel evenly dividing the left half arrays
        into blocks of size block_size.
     5) For each pivot (internal point in a half array that divides two
        blocks) in the left half arrays, find the corresponding pivot in the
        corresponding right half array using binary search.
     6) Store the starting and ending indices of each corresponding block from
        the left and right half arrays.
     7) This creates potentially very uneven size blocks in the right array.
        In fact, a single block of the right array could be half the entire a
        array in the worst case.  To increase the parallelism when the right
        half blocks are large, go through the blocks in parallel.  If any
        right block is larger than block_size, divide it into blocks no larger
        than block_size.  For each new pivot in the right half block, find the
        corresponding pivot in the corresponding left half block.
     8) Store the starting and ending indices of each corresponding new
        subblock from the left half and right half blocks and modify the
        starting and ending indices of the original left and right half blocks
        accordingly.
     9) We now have two lists of independent blocks that can be merged in
        parallel.  Note that each block is at most of size block_size.
    10) Go through the two lists of blocks in parallel performing a serial
        merge on each pair of blocks.
    11) At this point there still might be an extra left half array.  Note
        that the half array will already be sorted.  Copy over the half
        array in parallel, as it is being merged with nothing.
    12) Increase the array size.  If the array size is less than 2 * the size
        of a, then go back to step 4.  Otherwise, the array is sorted.
*/
/****************************************************************************/
namespace mtgl {
template<typename T>
void merge_sort(T* array, int size, int block_size = 1024)
{
  // a starts out pointing to the original array, and b starts out pointing to
  // the temporary memory.  a and b are going to be swapped back and forth by
  // the algorithm.  At the beginning of each iteration, a holds the unmerged
  // arrays.  The merged results are put into b.  Then the pointers for a and
  // b are swapped, so the result is pointed to by a at the end.
  T* new_array = (T*) malloc(size * sizeof(T));
  T* a = array;
  T* b = new_array;

  // Set the split point for when to switch from normal parallel merge sort
  // to utilizing parallel merging.
  int split_point = block_size * 4;

  // Each array represents two half arrays that need to be merged.
  int array_size = 2;

  // Until the array size reaches four times the block size (aka there are two
  // blocks in each half array, enough to take advantage of parallel merging),
  // go through the blocks in parallel, merging each one in serial.
  for ( ; array_size < (size * 2) && array_size < split_point; array_size *= 2)
  {
    #pragma mta noalias *a, *b
    #pragma mta assert nodep
    #pragma mta interleave schedule
    for (int j = 0; j < size; j += array_size)
    {
      // Get the starting and ending positions in the left and right halves.
      int left_pos = j;
      int right_pos = j + (array_size / 2);
      int left_end = size > right_pos ? right_pos : size;
      int right_end = size > j + array_size ? j + array_size : size;
      int b_pos = left_pos;

      // Merge the left and right halves until one of them is empty.
      while (left_pos < left_end && right_pos < right_end)
      {
        if (a[left_pos] < a[right_pos])
        {
          b[b_pos++] = a[left_pos++];
        }
        else
        {
          b[b_pos++] = a[right_pos++];
        }
      }

      // There will be a tail left in either the left or right half.  Copy
      // the tail to b.
      if (left_pos < left_end)
      {
        // Copy the tail in the left half to b.
        #pragma mta noalias *a, *b
        #pragma mta assert nodep
        for (int k = left_pos; k < left_end; ++k)
        {
          b[b_pos + (k - left_pos)] = a[k];
        }
      }
      else
      {
        // Copy the tail in the right half to b.
        #pragma mta noalias *a, *b
        #pragma mta assert nodep
        for (int k = right_pos; k < right_end; ++k)
        {
          b[b_pos + (k - right_pos)] = a[k];
        }
      }
    }

    // Swap a and b.
    T* tmp = a;
    a = b;
    b = tmp;
  }

  // For the normal block indices, we need a max of num_blocks storage, where
  // num_blocks is the total number of blocks in the array a, as that is the
  // max number of block pairs.  The worst possible case happens when there
  // are 2^n + block_size items.  During the last round (array size of
  // 2^(n+1)), all of the blocks except for 1 are in the left half array.
  //
  // For the extra block indices, we need a max of num_blocks / 2 storage, as
  // that is the max number of block pairs.  The extra blocks can only come
  // from the right array which is always shortchanged blocks if the number of
  // blocks is not a power of 2.  The worst possible case happens when there
  // are 2^n items.  During the last round, there are 2 half arrays of size
  // 2^(n-1).  Assume that every item in the left half array is smaller than
  // every item in the right half array.  Therefore, the right half array
  // will be one large block.  Dividing yields num_blocks / 2 total blocks.
  int num_blocks = (size + block_size - 1) / block_size;
  int* left_start = (int*) malloc(num_blocks * sizeof(int));
  int* left_end = (int*) malloc(num_blocks * sizeof(int));
  int* right_start = (int*) malloc(num_blocks * sizeof(int));
  int* right_end = (int*) malloc(num_blocks * sizeof(int));
  int* extra_left_start = (int*) malloc(num_blocks / 2 * sizeof(int));
  int* extra_left_end = (int*) malloc(num_blocks / 2 * sizeof(int));
  int* extra_right_start = (int*) malloc(num_blocks / 2 * sizeof(int));
  int* extra_right_end = (int*) malloc(num_blocks / 2 * sizeof(int));

  // Now we've gotten to the point where the half arrays are the size of two
  // blocks.  Go through the arrays in parallel, but also divide each half
  // array into independent blocks that can be merged in parallel giving two
  // level parallelism.
  for ( ; array_size < (size * 2); array_size *= 2)
  {
    int half_array_size = array_size / 2;
    int blocks_in_half_array = half_array_size / block_size;
    int num_half_arrays = (size + half_array_size - 1) / half_array_size;
    int half_num_half_arrays = num_half_arrays / 2;
    int num_array_blocks = half_num_half_arrays * blocks_in_half_array;

    // Go through the array setting the starting and ending positions of all
    // blocks in the left and right halves.  The left half will be evenly
    // divided into blocks of size block_size.  The right half will contain
    // uneven size blocks.  For each pivot (internal point in a half array
    // that divides two blocks) in the left half, find the corresponding pivot
    // in the right half using binary search.

    // Go over the entire array, setting the starting and ending indices for
    // all blocks in the left halves.
    for (int i = 0; i < num_array_blocks; ++i)
    {
      // The pivots in a are all evenly spaced.
      left_start[i] = i / blocks_in_half_array * array_size +
                      (i % blocks_in_half_array) * block_size;
      left_end[i] = left_start[i] + block_size;
    }

    // The ending index for the last half block is guaranteed to be less than
    // the boundary of the array, so we don't need to check.

    // Set the starting index for the first right half block.
    right_start[0] = half_array_size;

    // Go over the entire array, setting the ending indices for all but the
    // last right half block and setting the starting indices for all but the
    // first right half block.
    for (int i = 1; i < num_array_blocks; ++i)
    {
      if (i % blocks_in_half_array == 0)
      {
        // This is the beginning of a right half array.
        right_end[i-1] = left_start[i];
        right_start[i] = left_start[i] + half_array_size;
      }
      else
      {
        // This is an interior pivot for a right half array.  Use binary
        // search to find the entry in the right half that corresponds to
        // the current pivot in the left half.
        int left_pivot = left_start[i];
        int right_first = i / blocks_in_half_array * array_size +
                          half_array_size;
        int right_last = size < right_first + half_array_size ?
                         size - 1 : right_first + half_array_size - 1;
        int right_center;

        // Check if all the entries in the right half are greater than all the
        // entries in the block to the left of left_pivot.  If so, set the
        // pivot in the right half to the beginning of the right half.
        if (a[left_pivot - 1] < a[right_first])
        {
          right_last = right_first;
        }
        // Check if all the entries in the right half are less than all the
        // entries in the block that left_pivot begins.  If so, set the pivot
        // in the right half to one past the end of the right half.
        else if (a[right_last] < a[left_pivot])
        {
          ++right_last;
        }
        else
        {
          // Perform a binary search for the entry in the right half that
          // corresponds to the ith pivot in the left half.
          while (right_last - right_first > 1)
          {
            right_center = right_first + (right_last - right_first) / 2;

            if (a[right_center] < a[left_pivot])
            {
              right_first = right_center;
            }
            else if (a[left_pivot] < a[right_center])
            {
              right_last = right_center;
            }
            else
            {
              if (right_center != 0)
              {
                right_first = right_center - 1;
                right_last = right_center;
              }
              else
              {
                right_first = right_center;
                right_last = right_center + 1;
              }

              break;
            }
          }
        }

        right_end[i - 1] = right_last;
        right_start[i] = right_last;
      }
    }

    // Set the ending index for the last right half block.  Make sure it
    // doesn't go beyond the past the boundary of the array.
    right_end[num_array_blocks - 1] = size < array_size * half_num_half_arrays ?
                                      size : array_size * half_num_half_arrays;

    int num_extra_blocks = 0;

    // Go over the blocks again to see if any of the right blocks are large
    // enough to warrant splitting them.  If so, split the block.
    #pragma mta assert nodep
    for (int i = 0; i < num_array_blocks; ++i)
    {
      // There will be a total of num_new_subblocks + 1 subblocks in the right
      // block.  All of the subblocks except the last will be of size
      // block_size.  The last subblock is <= block_size.
      int num_new_subblocks =
          (right_end[i] - right_start[i] + block_size - 1) / block_size - 1;

      if (num_new_subblocks > 0)
      {
        int extra_pos = mt_incr(num_extra_blocks, num_new_subblocks);

        extra_left_start[extra_pos] = left_start[i];
        extra_right_start[extra_pos] = right_start[i];
        extra_right_end[extra_pos] = right_start[i] + block_size;

        // Split the block into num_new_subblocks.
        for (int j = 0; j < num_new_subblocks; ++j)
        {
          int right_pivot = extra_right_end[extra_pos + j];
          int left_first = extra_left_start[extra_pos + j];
          int left_last = left_end[i] - 1;
          int left_center;

          // Check if all the entries in the right half are greater than all
          // the entries in the block to the left of right_pivot.  If so, set
          // the pivot in the right half to the beginning of the right half.
          if (a[right_pivot - 1] < a[left_first])
          {
            left_last = left_first;
          }
          // Check if all the entries in the right half are less than all the
          // entries in the block that right_pivot begins.  If so, set the
          // pivot in the right half to one past the end of the right half.
          else if (a[left_last] < a[right_pivot])
          {
            ++left_last;
          }
          else
          {
            // Perform a binary search for the entry in the right half that
            // corresponds to the ith pivot in the left half.
            while (left_last - left_first > 1)
            {
              left_center = left_first + (left_last - left_first) / 2;

              if (a[left_center] < a[right_pivot])
              {
                left_first = left_center;
              }
              else if (a[right_pivot] < a[left_center])
              {
                left_last = left_center;
              }
              else
              {
                if (left_center != 0)
                {
                  left_first = left_center - 1;
                  left_last = left_center;
                }
                else
                {
                  left_first = left_center;
                  left_last = left_center + 1;
                }

                break;
              }
            }
          }

          // Set the indices for the left two blocks.
          extra_left_end[extra_pos + j] = left_last;

          if (j + 1 < num_new_subblocks)
          {
            extra_left_start[extra_pos + j + 1] = left_last;
            extra_right_start[extra_pos + j + 1] =
                extra_right_end[extra_pos + j];
            extra_right_end[extra_pos + j + 1] =
                extra_right_end[extra_pos + j] + block_size;
          }
        }

        left_start[i] = extra_left_end[extra_pos + num_new_subblocks - 1];
        right_start[i] = extra_right_end[extra_pos + num_new_subblocks - 1];
      }
    }

    // Merge the independent blocks in parallel.
    #pragma mta assert nodep
    for (int i = 0; i < num_array_blocks; ++i)
    {
      // Set the ranges.
      int left_pos = left_start[i];
      int right_pos = right_start[i];
      int b_pos = left_start[i] + right_start[i] -
                  (i / blocks_in_half_array * array_size + half_array_size);

      // Merge the two blocks until one is empty.
      while (left_pos < left_end[i] && right_pos < right_end[i])
      {
        if (a[left_pos] < a[right_pos])
        {
          b[b_pos++] = a[left_pos++];
        }
        else
        {
          b[b_pos++] = a[right_pos++];
        }
      }

      // There will be a tail left in either the left or right block.  Copy
      // the tail to b.
      if (left_pos < left_end[i])
      {
        // Copy the tail in the left block to b.
        for ( ; left_pos < left_end[i]; ++left_pos)
        {
          b[b_pos++] = a[left_pos];
        }
      }
      else
      {
        // Copy the tail in the right block to b.
        for ( ; right_pos < right_end[i]; ++right_pos)
        {
          b[b_pos++] = a[right_pos];
        }
      }
    }

    // Merge the extra independent blocks in parallel.
    #pragma mta assert nodep
    for (int i = 0; i < num_extra_blocks; ++i)
    {
      // Set the ranges.
      int left_pos = extra_left_start[i];
      int right_pos = extra_right_start[i];
      int b_pos = extra_left_start[i] + extra_right_start[i] -
                  (extra_right_start[i] / array_size * array_size +
                   half_array_size);


      // Merge the two blocks until one is empty.
      while (left_pos < extra_left_end[i] && right_pos < extra_right_end[i])
      {
        if (a[left_pos] < a[right_pos])
        {
          b[b_pos++] = a[left_pos++];
        }
        else
        {
          b[b_pos++] = a[right_pos++];
        }
      }

      // There will be a tail left in either the left or right block.  Copy
      // the tail to b.
      if (left_pos < extra_left_end[i])
      {
        // Copy the tail in the left block to b.
        for ( ; left_pos < extra_left_end[i]; ++left_pos)
        {
          b[b_pos++] = a[left_pos];
        }
      }
      else
      {
        // Copy the tail in the right block to b.
        for ( ; right_pos < extra_right_end[i]; ++right_pos)
        {
          b[b_pos++] = a[right_pos];
        }
      }
    }

    // If there is an extra left half array, copy it to b.  This happens when
    // there are an odd number of half arrays with the last half array being
    // anwhere between 1 and half_array_size items.
    if (num_half_arrays % 2 == 1)
    {
      #pragma mta assert nodep
      for (int i = half_num_half_arrays * array_size; i < size; ++i)
      {
        b[i] = a[i];
      }
    }

    // Swap a and b.
    T* tmp = a;
    a = b;
    b = tmp;
  }

  // The answer is pointed to by a.  If a ends up pointing to the temporary
  // memory, copy the result to the passed in array.
  if (a != array)
  {
    #pragma mta assert nodep
    for (int i = 0; i < size; ++i)  array[i] = a[i];
  }

  free(left_start);
  free(left_end);
  free(right_start);
  free(right_end);
  free(extra_left_start);
  free(extra_left_end);
  free(extra_right_start);
  free(extra_right_end);
  free(new_array);
}

// Same as above function, but an extra N size cost for second array
// swap.  This swaps the data for the second array whenever the first
// array is swapped.
template<typename T, typename T1>
void merge_sort(T* array, int size, T1* array1, int block_size = 1024)
{
  // a starts out pointing to the original array, and b starts out pointing to
  // the temporary memory.  a and b are going to be swapped back and forth by
  // the algorithm.  At the beginning of each iteration, a holds the unmerged
  // arrays.  The merged results are put into b.  Then the pointers for a and
  // b are swapped, so the result is pointed to by a at the end.
  T* new_array = (T*) malloc(size * sizeof(T));
  T* a = array;
  T* b = new_array;

  T1* new_array1 = (T1*) malloc(size * sizeof(T1));
  T1* a1 = array1;
  T1* b1 = new_array1;

  // Set the split point for when to switch from normal parallel merge sort
  // to utilizing parallel merging.
  int split_point = block_size * 4;

  // Each array represents two half arrays that need to be merged.
  int array_size = 2;

  // Until the array size reaches four times the block size (aka there are two
  // blocks in each half array, enough to take advantage of parallel merging),
  // go through the blocks in parallel, merging each one in serial.
  for ( ; array_size < (size * 2) && array_size < split_point; array_size *= 2)
  {
    #pragma mta noalias *a, *b, *a1, *b1
    #pragma mta assert nodep
    #pragma mta interleave schedule
    for (int j = 0; j < size; j += array_size)
    {
      // Get the starting and ending positions in the left and right halves.
      int left_pos = j;
      int right_pos = j + (array_size / 2);
      int left_end = size > right_pos ? right_pos : size;
      int right_end = size > j + array_size ? j + array_size : size;
      int b_pos = left_pos;

      // Merge the left and right halves until one of them is empty.
      while (left_pos < left_end && right_pos < right_end)
      {
        if (a[left_pos] < a[right_pos])
        {
          b[b_pos] = a[left_pos];
          b1[b_pos] = a1[left_pos];
          b_pos++;
          left_pos++;
        }
        else
        {
          b[b_pos] = a[right_pos];
          b1[b_pos] = a1[right_pos];
          b_pos++;
          right_pos++;
        }
      }

      // There will be a tail left in either the left or right half.  Copy
      // the tail to b.
      if (left_pos < left_end)
      {
        // Copy the tail in the left half to b.
        #pragma mta noalias *a, *b, *a1, *b1
        #pragma mta assert nodep
        for (int k = left_pos; k < left_end; ++k)
        {
          b[b_pos + (k - left_pos)] = a[k];
          b1[b_pos + (k - left_pos)] = a1[k];
        }
      }
      else
      {
        // Copy the tail in the right half to b.
        #pragma mta noalias *a, *b, *a1, *b1
        #pragma mta assert nodep
        for (int k = right_pos; k < right_end; ++k)
        {
          b[b_pos + (k - right_pos)] = a[k];
          b1[b_pos + (k - right_pos)] = a1[k];
        }
      }
    }

    // Swap a and b.
    T* tmp = a;
    a = b;
    b = tmp;

    T1* tmp1 = a1;
    a1 = b1;
    b1 = tmp1;
  }

  // For the normal block indices, we need a max of num_blocks storage, where
  // num_blocks is the total number of blocks in the array a, as that is the
  // max number of block pairs.  The worst possible case happens when there
  // are 2^n + block_size items.  During the last round (array size of
  // 2^(n+1)), all of the blocks except for 1 are in the left half array.
  //
  // For the extra block indices, we need a max of num_blocks / 2 storage, as
  // that is the max number of block pairs.  The extra blocks can only come
  // from the right array which is always shortchanged blocks if the number of
  // blocks is not a power of 2.  The worst possible case happens when there
  // are 2^n items.  During the last round, there are 2 half arrays of size
  // 2^(n-1).  Assume that every item in the left half array is smaller than
  // every item in the right half array.  Therefore, the right half array
  // will be one large block.  Dividing yields num_blocks / 2 total blocks.
  int num_blocks = (size + block_size - 1) / block_size;
  int* left_start = (int*) malloc(num_blocks * sizeof(int));
  int* left_end = (int*) malloc(num_blocks * sizeof(int));
  int* right_start = (int*) malloc(num_blocks * sizeof(int));
  int* right_end = (int*) malloc(num_blocks * sizeof(int));
  int* extra_left_start = (int*) malloc(num_blocks / 2 * sizeof(int));
  int* extra_left_end = (int*) malloc(num_blocks / 2 * sizeof(int));
  int* extra_right_start = (int*) malloc(num_blocks / 2 * sizeof(int));
  int* extra_right_end = (int*) malloc(num_blocks / 2 * sizeof(int));

  // Now we've gotten to the point where the half arrays are the size of two
  // blocks.  Go through the arrays in parallel, but also divide each half
  // array into independent blocks that can be merged in parallel giving two
  // level parallelism.
  for ( ; array_size < (size * 2); array_size *= 2)
  {
    int half_array_size = array_size / 2;
    int blocks_in_half_array = half_array_size / block_size;
    int num_half_arrays = (size + half_array_size - 1) / half_array_size;
    int half_num_half_arrays = num_half_arrays / 2;
    int num_array_blocks = half_num_half_arrays * blocks_in_half_array;

    // Go through the array setting the starting and ending positions of all
    // blocks in the left and right halves.  The left half will be evenly
    // divided into blocks of size block_size.  The right half will contain
    // uneven size blocks.  For each pivot (internal point in a half array
    // that divides two blocks) in the left half, find the corresponding pivot
    // in the right half using binary search.

    // Go over the entire array, setting the starting and ending indices for
    // all blocks in the left halves.
    for (int i = 0; i < num_array_blocks; ++i)
    {
      // The pivots in a are all evenly spaced.
      left_start[i] = i / blocks_in_half_array * array_size +
                      (i % blocks_in_half_array) * block_size;
      left_end[i] = left_start[i] + block_size;
    }

    // The ending index for the last half block is guaranteed to be less than
    // the boundary of the array, so we don't need to check.

    // Set the starting index for the first right half block.
    right_start[0] = half_array_size;

    // Go over the entire array, setting the ending indices for all but the
    // last right half block and setting the starting indices for all but the
    // first right half block.
    for (int i = 1; i < num_array_blocks; ++i)
    {
      if (i % blocks_in_half_array == 0)
      {
        // This is the beginning of a right half array.
        right_end[i-1] = left_start[i];
        right_start[i] = left_start[i] + half_array_size;
      }
      else
      {
        // This is an interior pivot for a right half array.  Use binary
        // search to find the entry in the right half that corresponds to
        // the current pivot in the left half.
        int left_pivot = left_start[i];
        int right_first = i / blocks_in_half_array * array_size +
                          half_array_size;
        int right_last = size < right_first + half_array_size ?
                         size - 1 : right_first + half_array_size - 1;
        int right_center;

        // Check if all the entries in the right half are greater than all the
        // entries in the block to the left of left_pivot.  If so, set the
        // pivot in the right half to the beginning of the right half.
        if (a[left_pivot - 1] < a[right_first])
        {
          right_last = right_first;
        }
        // Check if all the entries in the right half are less than all the
        // entries in the block that left_pivot begins.  If so, set the pivot
        // in the right half to one past the end of the right half.
        else if (a[right_last] < a[left_pivot])
        {
          ++right_last;
        }
        else
        {
          // Perform a binary search for the entry in the right half that
          // corresponds to the ith pivot in the left half.
          while (right_last - right_first > 1)
          {
            right_center = right_first + (right_last - right_first) / 2;

            if (a[right_center] < a[left_pivot])
            {
              right_first = right_center;
            }
            else if (a[left_pivot] < a[right_center])
            {
              right_last = right_center;
            }
            else
            {
              if (right_center != 0)
              {
                right_first = right_center - 1;
                right_last = right_center;
              }
              else
              {
                right_first = right_center;
                right_last = right_center + 1;
              }

              break;
            }
          }
        }

        right_end[i - 1] = right_last;
        right_start[i] = right_last;
      }
    }

    // Set the ending index for the last right half block.  Make sure it
    // doesn't go beyond the past the boundary of the array.
    right_end[num_array_blocks - 1] = size < array_size * half_num_half_arrays ?
                                      size : array_size * half_num_half_arrays;

    int num_extra_blocks = 0;

    // Go over the blocks again to see if any of the right blocks are large
    // enough to warrant splitting them.  If so, split the block.
    #pragma mta assert nodep
    for (int i = 0; i < num_array_blocks; ++i)
    {
      // There will be a total of num_new_subblocks + 1 subblocks in the right
      // block.  All of the subblocks except the last will be of size
      // block_size.  The last subblock is <= block_size.
      int num_new_subblocks =
          (right_end[i] - right_start[i] + block_size - 1) / block_size - 1;

      if (num_new_subblocks > 0)
      {
        int extra_pos = mt_incr(num_extra_blocks, num_new_subblocks);

        extra_left_start[extra_pos] = left_start[i];
        extra_right_start[extra_pos] = right_start[i];
        extra_right_end[extra_pos] = right_start[i] + block_size;

        // Split the block into num_new_subblocks.
        for (int j = 0; j < num_new_subblocks; ++j)
        {
          int right_pivot = extra_right_end[extra_pos + j];
          int left_first = extra_left_start[extra_pos + j];
          int left_last = left_end[i] - 1;
          int left_center;

          // Check if all the entries in the right half are greater than all
          // the entries in the block to the left of right_pivot.  If so, set
          // the pivot in the right half to the beginning of the right half.
          if (a[right_pivot - 1] < a[left_first])
          {
            left_last = left_first;
          }
          // Check if all the entries in the right half are less than all the
          // entries in the block that right_pivot begins.  If so, set the
          // pivot in the right half to one past the end of the right half.
          else if (a[left_last] < a[right_pivot])
          {
            ++left_last;
          }
          else
          {
            // Perform a binary search for the entry in the right half that
            // corresponds to the ith pivot in the left half.
            while (left_last - left_first > 1)
            {
              left_center = left_first + (left_last - left_first) / 2;

              if (a[left_center] < a[right_pivot])
              {
                left_first = left_center;
              }
              else if (a[right_pivot] < a[left_center])
              {
                left_last = left_center;
              }
              else
              {
                if (left_center != 0)
                {
                  left_first = left_center - 1;
                  left_last = left_center;
                }
                else
                {
                  left_first = left_center;
                  left_last = left_center + 1;
                }

                break;
              }
            }
          }

          // Set the indices for the left two blocks.
          extra_left_end[extra_pos + j] = left_last;

          if (j + 1 < num_new_subblocks)
          {
            extra_left_start[extra_pos + j + 1] = left_last;
            extra_right_start[extra_pos + j + 1] =
                extra_right_end[extra_pos + j];
            extra_right_end[extra_pos + j + 1] =
                extra_right_end[extra_pos + j] + block_size;
          }
        }

        left_start[i] = extra_left_end[extra_pos + num_new_subblocks - 1];
        right_start[i] = extra_right_end[extra_pos + num_new_subblocks - 1];
      }
    }

    // Merge the independent blocks in parallel.
    #pragma mta assert nodep
    for (int i = 0; i < num_array_blocks; ++i)
    {
      // Set the ranges.
      int left_pos = left_start[i];
      int right_pos = right_start[i];
      int b_pos = left_start[i] + right_start[i] -
                  (i / blocks_in_half_array * array_size + half_array_size);

      // Merge the two blocks until one is empty.
      while (left_pos < left_end[i] && right_pos < right_end[i])
      {
        if (a[left_pos] < a[right_pos])
        {
          b[b_pos] = a[left_pos];
          b1[b_pos] = a1[left_pos];
          b_pos++;
          left_pos++;
        }
        else
        {
          b[b_pos] = a[right_pos];
          b1[b_pos] = a1[right_pos];
          b_pos++;
          right_pos++;
        }
      }

      // There will be a tail left in either the left or right block.  Copy
      // the tail to b.
      if (left_pos < left_end[i])
      {
        // Copy the tail in the left block to b.
        for ( ; left_pos < left_end[i]; ++left_pos)
        {
          b[b_pos] = a[left_pos];
          b1[b_pos] = a1[left_pos];
          b_pos++;
        }
      }
      else
      {
        // Copy the tail in the right block to b.
        for ( ; right_pos < right_end[i]; ++right_pos)
        {
          b[b_pos] = a[right_pos];
          b1[b_pos] = a1[right_pos];
          b_pos++;
        }
      }
    }

    // Merge the extra independent blocks in parallel.
    #pragma mta assert nodep
    for (int i = 0; i < num_extra_blocks; ++i)
    {
      // Set the ranges.
      int left_pos = extra_left_start[i];
      int right_pos = extra_right_start[i];
      int b_pos = extra_left_start[i] + extra_right_start[i] -
                  (extra_right_start[i] / array_size * array_size +
                   half_array_size);


      // Merge the two blocks until one is empty.
      while (left_pos < extra_left_end[i] && right_pos < extra_right_end[i])
      {
        if (a[left_pos] < a[right_pos])
        {
          b[b_pos] = a[left_pos];
          b1[b_pos] = a1[left_pos];
          b_pos++;
          left_pos++;
        }
        else
        {
          b[b_pos] = a[right_pos];
          b1[b_pos] = a1[right_pos];
          b_pos++;
          right_pos++;
        }
      }

      // There will be a tail left in either the left or right block.  Copy
      // the tail to b.
      if (left_pos < extra_left_end[i])
      {
        // Copy the tail in the left block to b.
        for ( ; left_pos < extra_left_end[i]; ++left_pos)
        {
          b[b_pos] = a[left_pos];
          b1[b_pos] = a1[left_pos];
          b_pos++;
        }
      }
      else
      {
        // Copy the tail in the right block to b.
        for ( ; right_pos < extra_right_end[i]; ++right_pos)
        {
          b[b_pos] = a[right_pos];
          b1[b_pos] = a1[right_pos];
          b_pos++;
        }
      }
    }

    // If there is an extra left half array, copy it to b.  This happens when
    // there are an odd number of half arrays with the last half array being
    // anwhere between 1 and half_array_size items.
    if (num_half_arrays % 2 == 1)
    {
      #pragma mta assert nodep
      for (int i = half_num_half_arrays * array_size; i < size; ++i)
      {
        b[i] = a[i];
        b1[i] = a1[i];
      }
    }

    // Swap a and b.
    T* tmp = a;
    a = b;
    b = tmp;

    T1* tmp1 = a1;
    a1 = b1;
    b1 = tmp1;
  }

  // The answer is pointed to by a.  If a ends up pointing to the temporary
  // memory, copy the result to the passed in array.
  if (a != array)
  {
    #pragma mta assert nodep
    for (int i = 0; i < size; ++i)  array[i] = a[i];

    #pragma mta assert nodep
    for (int i = 0; i < size; ++i)  array1[i] = a1[i];
  }

  free(left_start);
  free(left_end);
  free(right_start);
  free(right_end);
  free(extra_left_start);
  free(extra_left_end);
  free(extra_right_start);
  free(extra_right_end);
  free(new_array);
  free(new_array1);
}

}

#endif

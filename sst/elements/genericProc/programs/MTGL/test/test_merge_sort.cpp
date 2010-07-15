/*  _________________________________________________________________________
 *
 *  MTGL: The MultiThreaded Graph Library
 *  Copyright (c) 2009 Sandia Corporation.
 *  This software is distributed under the BSD License.
 *  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 *  the U.S. Government retains certain rights in this software.
 *  For more information, see the README file in the top MTGL directory.
 *  _________________________________________________________________________
 */

/****************************************************************************/
/*! \file test_merge_sort.cpp

    \author Brad Mancke
    \author Greg Mackey (gemacke@sandia.gov)

    \date 6/4/2009
*/
/****************************************************************************/

#include <mtgl/merge_sort.hpp>
#include <mtgl/util.hpp>

#include <mta_rng.h>

using namespace mtgl;

bool check_not_sorted(int* a, int size)
{
  int cnt = 0;

  for (int i = 1; i < size; i++)
  {
    if (a[i - 1] > a[i]) cnt++;
  }

  return cnt;
}

int main(int argc, char* argv[])
{
  if (argc < 2)
  {
    fprintf(stderr, "Usage: %s <random_array_size>", argv[0]);
    exit(1);
  }

  mt_timer mt;
  int size;
  int* arr, * arr1, * arr2;

  size = atoi(argv[1]);
  arr = (int*) malloc(size * sizeof(int));
  arr1 = (int*) malloc(size * sizeof(int));
  arr2 = (int*) malloc(size * sizeof(int));

  prand_int(size, arr);

  int mod_size = size / 2;

  #pragma mta noalias *arr
  #pragma mta assert nodep
  for (int i = 0; i < size; i++)
  {
    arr[i] = arr[i] % mod_size + mod_size;
    arr1[i] = arr[i];
    arr2[i] = arr[i];
  }

  #pragma mta fence
  mt.start();
  merge_sort(arr, size);
  #pragma mta fence
  mt.stop();

  fprintf(stderr, "Sorting one array time: %f\n", mt.getElapsedSeconds());

  if (check_not_sorted(arr, size))
  {
    fprintf(stderr, "Array not sorted correctly\n");
  }
  else
  {
    fprintf(stderr, "Array sorted correctly\n");
  }

  #pragma mta fence
  mt.start();
  merge_sort(arr1, size, arr2);
  #pragma mta fence
  mt.stop();

  fprintf(stderr, "Sorting two arrays time: %f\n", mt.getElapsedSeconds());

  if (check_not_sorted(arr1, size))
  {
    fprintf(stderr, "Array1 not sorted correctly\n");
  }
  else
  {
    fprintf(stderr, "Array1 sorted correctly\n");
  }

  if (check_not_sorted(arr2, size))
  {
    fprintf(stderr, "Array2 not sorted correctly\n");
  }
  else
  {
    fprintf(stderr, "Array2 sorted correctly\n");
  }

  int cnt = 0;
  for (int i = 0; i < size; i++)
  {
    if (arr1[i] != arr2[i]) cnt++;
  }

  if (cnt)
  {
    fprintf(stderr, "sorting two arrays didn't work out\n");
  }
  else
  {
    fprintf(stderr, "sorting two arrays worked out\n");
  }

  free(arr);
  free(arr1);
  free(arr2);

  return 0;
}

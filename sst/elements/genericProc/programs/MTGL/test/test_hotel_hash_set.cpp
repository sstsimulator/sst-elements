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
/*! \file test_hotel_hash_set.cpp

    \author Eric Goodman (elgoodm@sandia.gov)

    \date 10/14/2009
*/
/****************************************************************************/

#include <mtgl/hotel_hash_set.hpp>
#include <mtgl/snap_util.h>

int main (int argc, const char* argv[])
{
  char* filename = argv[1];
  printf("filename %s\n", filename);
  int64_t snap_error;
  int64_t file_size = get_file_size(filename);
  printf("file_size %d\n", file_size);

  void* file_buffer = malloc(file_size);


  snap_init();
  snap_restore(filename, file_buffer, file_size, &snap_error);
  printf("restored %s\n", filename);
  int* array = ((int*) file_buffer);
  int num_ints = file_size / sizeof(int);
  printf("number of ints %d\n", num_ints);

  mt_timer timer;
  hotel_hash_set<int, int, add_function<int, int>, default_hash_func<int>,
                 default_eqfcn<int> > set(num_ints  / 2);

  #pragma mta fence
  timer.start();

  #pragma mta assert parallel
  for (int i = 0; i < 100000; i++) set.insert(i, 0);

  int one = 1;
  printf("about to insert\n");

  #pragma mta assert parallel
  #pragma mta block schedule
  for (int i = 0; i < num_ints; i++) set.insert_global(array[i], one);

//  set.consolidate();

  #pragma mta fence
  timer.stop();
  printf("%f\n", timer.getElapsedSeconds());

  set.print(25);

  printf("\nPrinting out 0 - 10\n");
  printf("Number of 0: %d\n", set.get(0));
  printf("Number of 1: %d\n", set.get(1));
  printf("Number of 2: %d\n", set.get(2));
  printf("Number of 3: %d\n", set.get(3));
  printf("Number of 4: %d\n", set.get(4));
  printf("Number of 5: %d\n", set.get(5));
  printf("Number of 6: %d\n", set.get(6));
  printf("Number of 7: %d\n", set.get(7));
  printf("Number of 8: %d\n", set.get(8));
  printf("Number of 9: %d\n", set.get(9));
  printf("Number of 10: %d\n", set.get(10));
}

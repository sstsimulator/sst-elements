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
/*! \file test_xmt_hash_table_eric.cpp

    \author Greg Mackey (gemacke@sandia.gov)

    \date 3/31/2010
*/
/****************************************************************************/

#include <cmath>

#include <mtgl/xmt_hash_table.hpp>
#include <mtgl/util.hpp>
#include <mtgl/mtgl_io.hpp>
#include <mtgl/snap_util.h>

using namespace mtgl;

typedef xmt_hash_table<int, int> hash_type;

int main (int argc, const char* argv[])
{
  if (argc != 3)
  {
    printf("Usage: %s filename table_size\n\n"
           "The file should be located in the lustre file system.  The table "
           "size should\n"
           "be given as a power of 2, e.g. '5' indictates table_size = "
           "pow(2,5).\n", argv[0]);

    exit(1);
  }

  // Getting the name of the file.
  char* filename = const_cast<char*>(argv[1]);
  printf("  filename: %s\n", filename);

  // Getting the size of the file.
  int64_t snap_error;
  int64_t file_size = get_file_size(filename);
  printf(" file_size: %d\n", file_size);

  int num_ints = file_size / sizeof(int);
  printf("  num_ints: %d\n", num_ints);

  // Getting the desired table size.
  int log_table_size = atoi(argv[2]);
  int table_size = pow(2, log_table_size);
  printf("table_size: %d\n\n", table_size);

  // Creating a buffer to store the contents of the file.
  void* file_buffer = malloc(file_size);

  mt_timer timer;

  #pragma mta fence
  timer.start();

  // Using the snapshot libraries to load the file into the buffer.
  snap_init();
  snap_restore(filename, file_buffer, file_size, &snap_error);

  #pragma mta fence
  timer.stop();
  printf("File Restore Time: %f\n\n", timer.getElapsedSeconds());

  // Assumes that the buffer is a bunch of integers.
  int* array = ((int*) file_buffer);

  mt_timer timer_everything;
  #pragma mta fence
  timer_everything.start();

  #pragma mta fence
  timer.start();

  // Creating the hash table.
  hash_type* table = new hash_type(table_size);

  #pragma mta fence
  timer.stop();
  printf("Construction Time: %f\n", timer.getElapsedSeconds());

  #pragma mta fence
  timer.start();

  hash_mt_incr<int> vis;

  #pragma mta assert parallel
  #pragma mta block schedule
  for (int i = 0; i < num_ints; i++)
  {
    table->update_insert(array[i], 1, vis);
  }

  #pragma mta fence
  timer.stop();
  printf("   Insertion Time: %f\n", timer.getElapsedSeconds());

  #pragma mta fence
  timer.start();

  delete table;

  #pragma mta fence
  timer.stop();
  printf(" Destruction Time: %f\n", timer.getElapsedSeconds());

  #pragma mta fence
  timer_everything.stop();
  printf("       Total Time: %f\n", timer_everything.getElapsedSeconds());

  free(file_buffer);

/*
  table->print(25);

  printf("\nPrinting out 0 - 10\n");
  printf("Number of 0: %d\n", table->get(0));
  printf("Number of 1: %d\n", table->get(1));
  printf("Number of 2: %d\n", table->get(2));
  printf("Number of 3: %d\n", table->get(3));
  printf("Number of 4: %d\n", table->get(4));
  printf("Number of 5: %d\n", table->get(5));
  printf("Number of 6: %d\n", table->get(6));
  printf("Number of 7: %d\n", table->get(7));
  printf("Number of 8: %d\n", table->get(8));
  printf("Number of 9: %d\n", table->get(9));
  printf("Number of 10: %d\n", table->get(10));
*/
}

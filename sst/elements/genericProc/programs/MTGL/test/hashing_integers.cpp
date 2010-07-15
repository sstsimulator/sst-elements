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
/*! \file hashing_integers.cpp

    \author Eric Goodman (elgoodm@sandia.gov)

    \date 1/12/2010
*/
/****************************************************************************/

#include <cmath>

#include <mtgl/util.hpp>
#include <mtgl/mtgl_io.hpp>
#include <mtgl/snap_util.h>

using namespace mtgl;

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

  // Creating the values array.
  int* values = (int*) malloc(sizeof(int) * table_size);

  // Creating the keys array.
  int* keys = (int*) calloc(sizeof(int), table_size);

  // Creating the occupied array that keeps track of which buckets are
  // occupied.
  int* occupied = (int*) calloc(sizeof(int), table_size);

  // The mask is used to bitwise and with values, rather than using the
  // modulus operator.
  int mask = table_size - 1;

  #pragma mta fence
  timer.stop();
  printf("Construction Time: %f\n", timer.getElapsedSeconds());

//  int num_steps = 0;

  #pragma mta fence
  timer.start();

  #pragma mta assert parallel
  #pragma mta block schedule
  for (int i = 0; i < num_ints; i++)
  {
    int index = array[i] * 31280644937747LL;
    index = index & mask;
    int key = array[i];
    unsigned int j = index;
    do
    {
      int probed = occupied[j];
      if (probed > 0)
      {
        if (keys[j] == key)
        {
          int_fetch_add(&values[j], 1);
          break;
        }
      }
      else
      {
        probed = readfe(&occupied[j]);
        if (probed == 0)
        {
          keys[j] = key;
          writeef(&occupied[j], 1);
          int_fetch_add(&values[j], 1);
          break;
        }
        else
        {
          if (keys[j] == key)
          {
            writeef(&occupied[j], 1);
            int_fetch_add(&values[j], 1);
            break;
          }
          writeef(&occupied[j], 1);
        }
      }
      // The below line is used to record the number of collisions.
      // However, it has a significant impact on performance, so leave
      // it commented out except for when you need collision numbers.
      //int_fetch_add(&num_steps, 1);
      j++;
      if (j == table_size) j = 0;
    } while (j != index );
  }

  #pragma mta fence
  timer.stop();
  printf("   Insertion Time: %f\n", timer.getElapsedSeconds());

//  printf("num steps %d\n", num_steps);

  #pragma mta fence
  timer.start();

  // Clearing memory.
  free(keys);
  free(values);
  free(occupied);

  #pragma mta fence
  timer.stop();
  printf(" Destruction Time: %f\n", timer.getElapsedSeconds());

  #pragma mta fence
  timer_everything.stop();
  printf("       Total Time: %f\n", timer_everything.getElapsedSeconds());

  free(file_buffer);

/*
  int num_unique_keys = 0;
  #pragma mta assert parallel
  for (int i = 0; i < table_size; i++)
  {
    if (occupied[i] > 0)
    {
      int_fetch_add(&num_unique_keys, 1);
    }
  }
  printf("Number of unique keys %d\n", num_unique_keys);

  printf("\nPrinting out 0 - 10\n");
  printf("Number of 0: %d\n", values[0 * 31280644937747LL & mask]);
  printf("Number of 1: %d\n", values[1 * 31280644937747LL & mask]);
  printf("Number of 2: %d\n", values[2 * 31280644937747LL & mask]);
  printf("Number of 3: %d\n", values[3 * 31280644937747LL & mask]);
  printf("Number of 4: %d\n", values[4 * 31280644937747LL & mask]);
  printf("Number of 5: %d\n", values[5 * 31280644937747LL & mask]);
  printf("Number of 6: %d\n", values[6 * 31280644937747LL & mask]);
  printf("Number of 7: %d\n", values[7 * 31280644937747LL & mask]);
  printf("Number of 8: %d\n", values[8 * 31280644937747LL & mask]);
  printf("Number of 9: %d\n", values[9 * 31280644937747LL & mask]);
  printf("Number of 10: %d\n", values[10 * 31280644937747LL & mask]);
*/
}

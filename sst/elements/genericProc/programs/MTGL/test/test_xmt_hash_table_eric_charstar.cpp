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
/*! \file test_xmt_hash_table_eric_charstar.cpp

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

typedef xmt_hash_table<char*, int> hash_type;

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

  int num_chars = file_size / sizeof(char);
  printf(" num_chars: %d\n", num_chars);

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

  // Assumes that the buffer is a bunch of chars.
  char* array = ((char*) file_buffer);

  mt_timer timer_everything;
  #pragma mta fence
  timer_everything.start();

  #pragma mta fence
  timer.start();

  hash_type* table = new hash_type(table_size);

  // Creating space to have pointers to the words.
  char** words = (char**) malloc((num_chars) * sizeof(char*));
  int num_words = 0;  // Assumes that the first character is not alphanumeric.

  #pragma mta fence
  timer.stop();
  printf("Construction Time: %f\n", timer.getElapsedSeconds());

  #pragma mta fence
  timer.start();

  #pragma mta assert parallel
  for (int i = 0; i < num_chars - 2; i++)
  {
    if (!isalnum(array[i]))
    {
      array[i] = '\0';
      int pos = mt_incr(num_words, 1);
      words[pos] = &array[i + 1];
    }
  }
  array[num_chars - 1] = '\0';      // Might mess up the last word.

  #pragma mta fence
  timer.stop();

  printf("  Word Parse Time: %f\n", timer.getElapsedSeconds());
  printf("  Number of words: %d\n", num_words);

  #pragma mta fence
  timer.start();

  hash_mt_incr<int> vis;

  #pragma mta assert parallel
  #pragma mta block schedule
  for (int i = 0; i < num_words; i++)
  {
    if (strlen(words[i]) > 0)
    {
      table->update_insert(words[i], 1, vis);
    }
  }

  #pragma mta fence
  timer.stop();
  printf("   Insertion Time: %f\n", timer.getElapsedSeconds());

  #pragma mta fence
  timer.start();

  delete table;
  free(words);

  #pragma mta fence
  timer.stop();
  printf(" Destruction Time: %f\n", timer.getElapsedSeconds());

  #pragma mta fence
  timer_everything.stop();
  printf("       Total Time: %f\n", timer_everything.getElapsedSeconds());

  free(file_buffer);

/*
  printf("Number of the: %d\n", table->get("the"));
  printf("Number of it: %d\n", table->get("it"));
  printf("Number of The: %d\n", table->get("The"));
  printf("Number of states: %d\n", table->get("states"));
  printf("Number of would: %d\n", table->get("would"));
*/
}

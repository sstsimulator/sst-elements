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
/*! \file test_hotel_hash_set_charstar.cpp

    \author Eric Goodman (elgoodm@sandia.gov)

    \date 11/13/2009
*/
/****************************************************************************/

#include <cctype>

#include <mtgl/hotel_hash_set.hpp>
#include <mtgl/snap_util.h>
#include <mtgl/merge_sort.hpp>

using namespace mtgl;

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
  char* array = ((char*) file_buffer);

  int num_chars = file_size / sizeof(char);
  int size = num_chars / 8;
  printf("num_chars %d, size %d\n", num_chars, size);

  mt_timer timer;
  mt_timer timer_all;

  timer_all.start();

  hotel_hash_set<char*, int, add_function<char*, int>,
                 default_hash_func<char*>, default_eqfcn<char*> > set(size);

  char** words = (char**) malloc((num_chars) * sizeof(char*));
  int num_words = 0;  // Assumes that the first character is not alphanumeric.

  #pragma mta fence
  timer.start();

  printf("About to find the words\n");

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
  printf("Found all the words\n");
  array[num_chars - 1] = '\0';    // Might mess up the last word.

  #pragma mta fence
  timer.stop();
  printf("Time to parse words%f\n", timer.getElapsedSeconds());

  printf("Number of words %d\n", num_words);

  #pragma mta fence
  timer.start();

  int one = 1;
  printf("about to insert\n");

  #pragma mta assert parallel
  #pragma mta block schedule
  for (int i = 0; i < num_words; i++)
  {
    if (strlen(words[i]) > 0)
    {
      set.insert_global(words[i], one);
    }
  }

  //set.consolidate();

  #pragma mta fence
  timer.stop();
  timer_all.stop();

  printf("Time for inserts %f\n", timer.getElapsedSeconds());
  printf("Time for everything except file io %f\n",
         timer_all.getElapsedSeconds());

  printf("Number of the: %d\n", set.get("the"));
  printf("Number of it: %d\n", set.get("it"));
  printf("Number of The: %d\n", set.get("The"));
  printf("Number of states: %d\n", set.get("states"));
  printf("Number of would: %d\n", set.get("would"));

/*
  printf("Number of 5: %d\n", set.get(5));
  printf("Number of 6: %d\n", set.get(6));
  printf("Number of 7: %d\n", set.get(7));
  printf("Number of 8: %d\n", set.get(8));
  printf("Number of 9: %d\n", set.get(9));
  printf("Number of 10: %d\n", set.get(10));
*/

  int table_size = 0;
  int* values = set.get_raw_values(table_size);
  int* occupied = set.get_occupied(table_size);

  int* counts = (int*) malloc(sizeof(int) * table_size);
  int num_distinct = 0;

  #pragma mta assert parallel
  for (int i = 0; i < size; i++)
  {
    if (occupied[i] == 1)
    {
      int pos = mt_incr(num_distinct, 1);
      counts[pos] = values[i];
    }
  }

  printf("num distinct %d\n", num_distinct);
  merge_sort<int>(counts, num_distinct);

/*
  #pragma mta parallel off
  for(int i = 0; i < num_distinct; i++)
  {
    printf("%d\n", counts[num_distinct -1 - i]);
  }
*/
}

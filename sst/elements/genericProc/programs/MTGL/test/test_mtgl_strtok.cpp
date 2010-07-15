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
/*! \file test_mtgl_strtok.cpp

    \author Eric Goodman (elgoodm@sandia.gov)

    \brief Tests the mtgl_strtok function.

    \date April 8, 2010
*/
/****************************************************************************/

#include <mtgl/mtgl_io.hpp>
#include <mtgl/util.hpp>
#include <mtgl/mtgl_string.h>

using namespace mtgl;

int main (int argc, const char* argv[])
{
  if (argc < 3)
  {
    printf("usage: %s -input_file <file_name>\n"
           "\t-delimiter <delimiter>\n"
           "\t-print <num_examples>\n"
           "input_file is required.  If delimiter is not specified\n"
           "the file will be tokenized into alphanumeric chunks\n\n"
           , argv[0]);
    return -1;
  }

  char* infile = NULL;
  int cur_arg_index = 1;
  int num_print = 0;
  char* delimiter = NULL;

  while (cur_arg_index < argc)
  {
    if (strcmp(argv[cur_arg_index], "-input_file") == 0)
    {
      ++cur_arg_index;
      infile = const_cast<char*>(argv[cur_arg_index]);
    }
    else if (strcmp(argv[cur_arg_index], "-print") == 0)
    {
      ++cur_arg_index;
      num_print = atoi(argv[cur_arg_index]);
    }
    else if (strcmp(argv[cur_arg_index], "-delimiter") == 0)
    {
      ++cur_arg_index;
      delimiter = const_cast<char*>(argv[cur_arg_index]);
    }
    else
    {
      printf("Unrecognized option %s\n", argv[cur_arg_index]);
    }

    ++cur_arg_index;
  }

  if (infile == NULL)
  {
    printf("The input file must be specified\n");
    return -1;
  }

  printf("input file %s\n", infile);

  mt_timer timer;

  timer.start();
  int size_array = 0;
  char* array = read_array<char>(infile, size_array);

  #pragma mta fence
  timer.stop();

  double seconds = timer.getElapsedSeconds();
  double rate = sizeof(char) * size_array / seconds / (1024 * 1024);

  printf("\tTime to read in the data %f\n", seconds);
  printf("\tRate: %f MB/second\n", rate);

  timer.start();
  int num_words = -1;
  int est_num_words = size_array / 2;
  char** words;

  if (delimiter == NULL)
  {
    words = mtgl_strtok(array, size_array, num_words, est_num_words);
  }
  else
  {
    words = mtgl_strtok(array, size_array, num_words, delimiter, est_num_words);
  }

  #pragma mta fence
  timer.stop();

  printf("\tTime to tokenize the data %f\n", timer.getElapsedSeconds());
  printf("\tNum words %d\n", num_words);

  if (num_print > num_words)
  {
    num_print = num_words;
  }
  printf("Below are %d example tokenized outputs\n", num_print);
  for (int i = 0; i < num_print; ++i)
  {
    printf("word[%d] %s\n", i, words[i]);
  }

  return 0;
}

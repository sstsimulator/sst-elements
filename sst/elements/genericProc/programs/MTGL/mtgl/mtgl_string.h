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
/*! \file mtgl_string.h

    \author Eric Goodman (elgoodm@sandia.gov)
    \author Brad Mancke (bmancke@bbn.com)

    \brief A file with utility functions operating on character arrays.

    \date April 7, 2010
*/
/****************************************************************************/

#ifndef MTGL_MTGL_STRING_H
#define MTGL_MTGL_STRING_H

#define NUM_STREAMS_TOKENIZE 60

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>

#include <mtgl/util.hpp>

namespace mtgl {

inline bool matches(char* long_string, int long_len,
                    char* short_string, int short_len)
{
  bool match = true;
  int i = 0;

  while (match && i < short_len && i < long_len)
  {
    if (long_string[i] != short_string[i]) match = false;

    i++;
  }

  return match;
}

char* remove_escapes(char* delimiter)
{

  int len = strlen(delimiter);
  char temp[len + 1];
  int new_len = 0;

  for (int i = 0; i < len; i++)
  {
    if (i == 0)
    {
      if (delimiter[i] == '\"' || delimiter[i] != '\'') continue;
    }

    if (i + 1 < len)
    {
      if (delimiter[i] == '\\')
      {
        if ( delimiter[i + 1] == '<')
        {
          continue;
        }
        else if ( delimiter[i + 1] == '>')
        {
          continue;
        }
      }
    }

    temp[new_len] = delimiter[i];
    new_len++;
  }

  for (int i = 0; i < new_len; i++) delimiter[i] = temp[i];

  delimiter[new_len] = '\0';

#ifdef DEBUG
  printf("delimiter %s\n", delimiter);
#endif

  return delimiter;
}

/*! \brief The function finds all sequences of alphanumeric characters
            and tokenizes them.

    \param array The character array to be tokenized.
    \param num_chars The number of characters in the array.
    \param num_words This paramter is set to the number of tokenized words.
    \param est_num_words The word array is allocated before it is known how
                          many words exist.  This parameter allows the user to
                          provide the estimate.  Otherwise, the word array
                          is allocated to be the same size as the character
                          array.

    \returns Returns a 2-dimensional character array that contains the
            tokenized words.

    CAUTION: The function canabalizes the input array, inserting null
             terminators in place of the start of an instance of a delimiter.
*/
char** mtgl_strtok(char* array, int num_chars, int& num_words,
                   int est_num_words = -1)
{
  num_words = 0;
  if (est_num_words < 0) est_num_words = num_chars;

  // In allocating the word buffer, we over-estimate the number of words.
  char** words = (char**) malloc((est_num_words) * sizeof(char*));

  int i = 0;
  int num_streams = 1;

  // Tokenize the string.
  #pragma mta trace "Before tokenization for loop"
  #pragma mta use NUM_STREAMS_TOKENIZE streams
  #pragma mta for all streams i of num_streams
  {
    int beg, end;
    determine_beg_end(num_chars, num_streams, i, beg, end);
    int num_local_words = 0;

    // Special case processing for beginning.
    if (beg == 0)
    {
      if (isalnum(array[beg])) num_local_words++;
    }

    // First go through and see how many words there are.
    if (end != num_chars)
    {
      for (int j = beg; j < end; j++)
      {
        if (!isalnum(array[j]))
        {
          array[j] = '\0';

          if (isalnum(array[j + 1])) num_local_words++;
        }
      }
    }
    else
    {
      for (int j = beg; j < end - 1; j++)
      {
        if (!isalnum(array[j]))
        {
          array[j] = '\0';

          if (isalnum(array[j + 1])) num_local_words++;
        }
      }
    }

    // Claim this thread's portion of the word array.
    int word_index = mt_incr(num_words, num_local_words);

    // Go through again and add the words.
    if (end != num_chars)
    {
      for (int j = beg; j < end; j++)
      {
        if (array[j] == '\0')
        {
          if (isalnum(array[j + 1]))
          {
            words[word_index] = &array[j + 1];
            word_index++;
          }
        }
      }
    }
    else
    {
      for (int j = beg; j < end - 1; j++)
      {
        if (array[j] == '\0')
        {
          if (isalnum(array[j + 1]))
          {
            words[word_index] = &array[j + 1];
            word_index++;
          }
        }
      }
    }

    // Special case processing for beginning.
    if (beg == 0)
    {
      if (isalnum(array[beg]))
      {
        words[word_index] = &array[beg];
        word_index++;
      }
    }

  }

  array[num_chars - 1] = '\0';        // Might mess up the last word.

  return words;
}

/*! \brief Similar to the other mtgl_strtok function, but this determines
           the token boundaries according to a specified delimiter.

    \param array The character array to be tokenized.
    \param num_chars The number of characters in the array.
    \param num_words This paramter is set to the number of tokenized words.
    \param char* delimiter The string used to found boudaries between tokens.
    \param est_num_words The word array is allocated before it is known how
                          many words exist.  This parameter allows the user to
                          provide the estimate.  Otherwise, the word array
                          is allocated to be the same size as the character
                          array.

    \returns Returns a 2-dimensional character array that contains the
            tokenized words.

    Currently, regular expressions in the delimiter is not supported.

    CAUTION: The function canabalizes the input array, inserting null
             terminators in place of the start of an instance of a delimiter.
*/
char** mtgl_strtok(char* array, int num_chars, int& num_words,
                   char* delimiter, int est_num_words = -1)
{

  delimiter = remove_escapes(delimiter);

  num_words = 0;
  if (est_num_words < 0) est_num_words = num_chars;

  int delimiter_length = strlen(delimiter);

  int max_num_procs = 512;
  int max_num_streams = NUM_STREAMS_TOKENIZE * max_num_procs;
  int size_replicated = max_num_streams * delimiter_length;

  char* delimiter_replicated = (char*) malloc(sizeof(char) * size_replicated);
  for (int i = 0; i < size_replicated; i++)
  {
    delimiter_replicated[i] = delimiter[i % delimiter_length];
  }

  // In allocating the word buffer, we over-estimate the number of words.
  char** words = (char**) malloc((est_num_words) * sizeof(char*));

  int i = 0;
  int num_streams = 1;

  // Tokenize the string.
  #pragma mta trace "Before tokenization for loop"
  #pragma mta use NUM_STREAMS_TOKENIZE streams
  #pragma mta for all streams i of num_streams
  {
    int beg, end;
    determine_beg_end(num_chars, num_streams, i, beg, end);

    int num_local_words = 0;

    // Special case processing for beginning.
    if (beg == 0)
    {
      if (matches(&array[beg], num_chars,
                  &delimiter_replicated[i * delimiter_length],
                  delimiter_length))
      {
        num_local_words++;
      }
    }

    // First go through and see how many words there are.
    if (end != num_chars)
    {
      for (int j = beg; j < end; j++)
      {
        if (matches(&array[j], num_chars - j,
                    &delimiter_replicated[i * delimiter_length],
                    delimiter_length))
        {
          array[j] = '\0';

          if (isalnum(array[j + 1])) num_local_words++;
        }
      }
    }
    else
    {
      for (int j = beg; j < end - delimiter_length + 1; j++)
      {
        if (matches(&array[j], num_chars - j,
                    &delimiter_replicated[i * delimiter_length],
                    delimiter_length))
        {
          array[j] = '\0';

          if (isalnum(array[j + 1])) num_local_words++;
        }
      }
    }

    // Claim this thread's portion of the word array.
    int word_index = mt_incr(num_words, num_local_words);

    // Go through again and add the words.
    if (end != num_chars)
    {
      for (int j = beg; j < end; j++)
      {
        if (array[j] == '\0' &&
            matches(&array[j + 1], num_chars - (j + 1),
                    &delimiter_replicated[i * delimiter_length + 1],
                    delimiter_length - 1))

        {
          words[word_index] = &array[j + delimiter_length];
          word_index++;
        }
      }
    }
    else
    {
      for (int j = beg; j < end - delimiter_length; j++)
      {
        if (array[j] == '\0' &&
            matches(&array[j + 1], num_chars - (j + 1),
                    &delimiter_replicated[i * delimiter_length + 1],
                    delimiter_length - 1))

        {
          words[word_index] = &array[j + delimiter_length];
          word_index++;
        }
      }
    }

    // Special case processing for beginning.
    if (beg == 0)
    {
      if (matches(&array[beg], num_chars,
                  &delimiter_replicated[i * delimiter_length],
                  delimiter_length))
      {
        words[word_index] = &array[beg + delimiter_length];
        word_index++;
      }
    }

  }

  array[num_chars - 1] = '\0';        // Might mess up the last word.

  return words;
}

}

#endif

/* _________________________________________________________________________
 *
 * MTGL: The MultiThreaded Graph Library
 * Copyright (c) 2008 Sandia Corporation.
 * This software is distributed under the BSD License.
 * Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 * the U.S. Government retains certain rights in this software.
 * For more information, see the README file in the top MTGL directory.
 * _________________________________________________________________________
 */

/****************************************************************************/
/*! \file dictionary_encoding.cpp

    \author Eric Goodman (elgoodm@sandia.gov)

    \date 2/18/2010
*/
/****************************************************************************/

#include <cctype>
#include <cmath>

#include <mtgl/lp_hash_set.hpp>
#include <mtgl/snap_util.h>
#include <mtgl/merge_sort.hpp>
#include <mtgl/mtgl_io.hpp>
#include <mtgl/multimap.hpp>
#include <mtgl/rdf_quad.hpp>

#define SUBJECT_INDEX 0
#define PREDICATE_INDEX 1
#define OBJECT_INDEX 2
#define CONTEXT_INDEX 3

#define RDFS_SUBPROPERTY "<rdfs:subPropertyOf>"
#define RDFS_SUBCLASS "<rdfs:subClassOf>"
#define RDFS_DOMAIN "<rdfs:domain>"
#define RDFS_RANGE "<rdfs:range>"

using namespace mtgl;

typedef lp_hash_set<char*, int, min_insert_function<char*, int>,
                    default_hash_func<char*>, default_eqfcn<char*> >
        hash_type;

void create_index(hash_type* set, int* output_buffer, int num_words)
{
  mt_timer timer;
  timer.start();
  int estimate_num_rdfs = num_words / 50;
  int estimate_num_keys = estimate_num_rdfs / 2;
  multimap<int, rdf_quad<int>*> rdfs_index(estimate_num_keys,
                                           estimate_num_rdfs);

  int subproperty_id = set->get(RDFS_SUBPROPERTY);
  int subclass_id = set->get(RDFS_SUBCLASS);
  int domain_id = set->get(RDFS_DOMAIN);
  int range_id = set->get(RDFS_RANGE);

  #pragma mta fence
  timer.stop();
  printf("Time for initializing map and other intial setup for creating index "
         "%f\n", timer.getElapsedSeconds());
  timer.start();

  int* indices = (int*) malloc(sizeof(int) * num_words / 4);

  #pragma mta assert parallel
  #pragma mta block schedule
  #pragma mta use 100 streams
  for (int i = 0; i < num_words / 4; i++)
  {
    int predicate_id = output_buffer[4 * i + 1];

    if (predicate_id == subclass_id ||
        predicate_id == domain_id ||
        predicate_id == range_id ||
        predicate_id == subproperty_id)
    {
      int object_id = output_buffer[4 * i];
      int index = rdfs_index.increment(object_id);
      indices[i] = index;
    }
  }

  #pragma mta fence
  timer.stop();
  printf("Time for incrementing index %f\n", timer.getElapsedSeconds());

  timer.start();
  rdfs_index.initialize_value_storage();

  #pragma mta fence
  timer.stop();
  printf("Time for initializing the storage:\t %f\n",
         timer.getElapsedSeconds());

  #pragma mta assert parallel
  #pragma mta block schedule
  #pragma mta use 100 streams
  for (int i = 0; i < num_words / 4; i++)
  {
    int predicate_id = output_buffer[4 * i + 1];

    if (predicate_id == subclass_id ||
        predicate_id == domain_id ||
        predicate_id == range_id ||
        predicate_id == subproperty_id)
    {
      int index = indices[i];
      int object_id = output_buffer[4 * i];
      int subject_id = output_buffer[4 * i + 2];
      int context_id = output_buffer[4 * i + 3];

      rdf_quad<int>* quad = new rdf_quad<int>(object_id, predicate_id,
                                              subject_id, context_id);

      rdfs_index.insert(object_id, quad, index);
    }
  }

  #pragma mta fence
  timer.stop();
  printf("Time for inserting the values:\t %f\n", timer.getElapsedSeconds());

  timer.start();
  free(indices);

  #pragma mta fence
  timer.stop();
  printf("Time for freeing the index array:\t %f\n", timer.getElapsedSeconds());
}

/*! \brief Function that encapsulates the processing of a URI element.
    \param num_words Reference to the global counter for the total number
                     of words.
    \param words The words to be processed, the processed URI is added to
                 this array.
    \param array The original string of everything.
    \param j The thread's current index into the string of everything.
    \param count A boolean to indicate if the processed Uri should be added
                 to the list of words.
*/
int process_uri(int& num_words, char** words, char* array, int& j,
                bool count = true)
{
  int pos = -1;

  if (count)
  {
    pos = mt_incr(num_words, 1);
    words[pos] = &array[j];
  }

  while (array[j] != '>') ++j;

  ++j;

  array[j] = '\0';
  return pos;
}

/*! \brief Function that encapsulates the processing of a blank node.
    \param num_words Reference to the global counter for the total number
                     of words.
    \param words The words to be processed, the processed node is added to
                 this array.
    \param array The original string of everything.
    \param j The thread's current index into the string of everything.
    \param count A boolean to indicate if the node should be added to the
                 list of words.
*/
int process_node(int& num_words, char** words, char* array, int& j,
                 bool count = true)
{
  int pos = -1;

  if (count)
  {
    pos = mt_incr(num_words, 1);
    words[pos] = &array[j];
  }

  j = j + 2;

  while (array[j] != ' ' && array[j] != '\t') ++j;

  array[j] = '\0';
  return pos;
}

/*! \brief Function that encapsulates the processing of a literal.
    \param num_words Reference to the global counter for the total number
                     of words.
    \param words The words to be processed, the processed literal is added to
                 this array.
    \param array The original string of everything.
    \param j The thread's current index into the string of everything.
    \param count A boolean to indicate if the literal should be added to
                 the list of words.
*/
int process_literal(int& num_words, char** words, char* array, int& j,
                    bool count = true)
{
  int pos = -1;

  if (count)
  {
    pos = mt_incr(num_words, 1);
    words[pos] = &array[j];
  }

  ++j;
  bool found = false;

  while (!found)
  {
    while (array[j] != '"') ++j;

    int k = j - 1;
    int total = 0;
    while (array[k] == '\\')
    {
      ++total;
      --k;
    }

    if ((total % 2) == 0) found = true;

    ++j;
  }

  if (array[j] == '^' && array[j + 1] == '^')
  {
    j = j + 2;
    if (array[j] == '<')
    {
      while (array[j] != '>') ++j;

      ++j;
    }
    else
    {
      printf("parsing error %d\n", j);
    }
  }
  else if (array[j] == '@')
  {
    ++j;

    while (isalnum(array[j]) || array[j] == '-') ++j;
  }

  array[j] = '\0';
  return pos;
}

/*! \brief Encapsluates the processing of a line.
    \param num_words Reference to the global counter for the total number
                     of words.
    \param words The words to be processed, the processed uris, nodes, and
                 literals are added to this array.
    \param array The original string of everything.
    \param j The thread's current index into the string of everything.
*/
void process_line(int& num_words, char** words, char* array, int& j,
                  int my_line, int* word_mapping)
{
  int id; // The assigned id.

  // Subject.
  while (array[j] == ' ' && array[j] == '\t') ++j;

  int word_pos = -1;
  if (array[j] == '<')
  {
    word_pos = process_uri(num_words, words, array, j, true);
  }
  else if (array[j] == '_' && array[j + 1] == ':')
  {
    word_pos = process_node(num_words, words, array, j, true);
  }

  ++j; // The space.

  if (word_pos != -1)
  {
    word_mapping[2 * word_pos] = my_line;
    word_mapping[2 * word_pos + 1] = SUBJECT_INDEX;
  }

  // Predicate.
  if (array[j] == '<')
  {
    word_pos = process_uri(num_words, words, array, j, true);
  }

  ++j; // The space.

  if (word_pos != -1)
  {
    word_mapping[2 * word_pos] = my_line;
    word_mapping[2 * word_pos + 1] = PREDICATE_INDEX;
  }

  // Object.
  if (array[j] == '<')
  {
    word_pos = process_uri(num_words, words, array, j, true);
  }
  else if (array[j] == '_' && array[j + 1] == ':')
  {
    word_pos = process_node(num_words, words, array, j, true);
  }
  else if (array[j] == '"')
  {
    word_pos = process_literal(num_words, words, array, j, true);
  }
  else
  {
    printf("no object %d\n", j);
  }

  ++j; // The space.

  if (word_pos != -1)
  {
    word_mapping[2 * word_pos] = my_line;
    word_mapping[2 * word_pos + 1] = OBJECT_INDEX;
  }

  // Context.
  if (array[j] == '<')
  {
    word_pos = process_uri(num_words, words, array, j, true);
  }
  else if (array[j] == '_' && array[j + 1] == ':')
  {
    word_pos = process_node(num_words, words, array, j, true);
  }
  else if (array[j] == '"')
  {
    word_pos = process_literal(num_words, words, array, j, true);
  }

  ++j; // The space.

  if (word_pos != -1)
  {
    word_mapping[2 * word_pos] = my_line;
    word_mapping[2 * word_pos + 1] = CONTEXT_INDEX;
  }
}

int main (int argc, const char* argv[])
{
  int cur_arg_index = 1;

  if (argc < 5)
  {
    fprintf(stderr, "Usage: %s \n"
            "\t-input_file <infile>\n "
            "\t-table_size <log of the table size>\n "
            "\t-output_file <outfile>"
            "\t -print\t:Prints out the mapping of every word, and also the "
            "translated quads, though they will probably not be in the same "
            "order as the original file. \n"
            "\t-index_rdfs <index file>\t:Creates an index file for the"
            " ontological triples\n"
            , argv[0]);
    printf("input_file and table_size are mandatory\n");

    return -1;
  }

  char* infile;
  char* outfile = NULL;
  char* rdfs_outfile = NULL;
  int table_size;
  bool print_output = false;

  while (cur_arg_index < argc)
  {
    if (strcmp(argv[cur_arg_index], "-input_file") == 0)
    {
      ++cur_arg_index;
      infile = const_cast<char*>(argv[cur_arg_index]);
    }
    else if (strcmp(argv[cur_arg_index], "-output_file") == 0)
    {
      ++cur_arg_index;
      outfile = const_cast<char*>(argv[cur_arg_index]);
    }
    else if (strcmp(argv[cur_arg_index], "-table_size") == 0)
    {
      ++cur_arg_index;
      int log_table_size = atoi(argv[cur_arg_index]);
      table_size = pow(2, log_table_size);
    }
    else if (strcmp(argv[cur_arg_index], "-print") == 0)
    {
      print_output = true;
    }
    else if (strcmp(argv[cur_arg_index], "-index_rdfs") == 0)
    {
      ++cur_arg_index;
      rdfs_outfile = const_cast<char*>(argv[cur_arg_index]);
    }
    else
    {
      fprintf(stderr, "Unrecognized option %s\n", argv[cur_arg_index]);
    }

    ++cur_arg_index;
  }

  // Printing the options selected.
  printf("input_file:\t%s\n", infile);
  if (outfile != NULL)
  {
    printf("output_file:\t%d\n", outfile);
  }
  printf("table size:\t%d\n", table_size);
  if (rdfs_outfile != NULL)
  {
    printf("rdfs index file:\t%s", rdfs_outfile);
  }

  // Getting the input file size in preperation for snap_restore.
  int64_t snap_error;
  int64_t file_size = get_file_size(infile);
  printf("Size of input file %d\n", file_size);

  mt_timer timer;

  // Creating a buffer to read in the file.
  #pragma mta fence
  timer.start();
  void* file_buffer = malloc(file_size);
  #pragma mta fence
  timer.stop();
  printf("time for constructing file_buffer %f\n", timer.getElapsedSeconds());
  timer.start();

  // Restoring the file.
  snap_init();
  snap_restore(infile, file_buffer, file_size, &snap_error);
  char* array = ((char*) file_buffer);
  int num_chars = file_size / sizeof(char);
  #pragma mta fence
  timer.stop();
  printf("time for restoring %f\n", timer.getElapsedSeconds());
  printf("num_chars %d\n", num_chars);
  timer.start();

  // Constructing the hash set.
  hash_type* set = new hash_type(table_size);

  #pragma mta fence
  timer.stop();
  printf("time for construction of hash set %f\n", timer.getElapsedSeconds());
  timer.start();

  int num_words_estimate = num_chars / 32;
  char** words = (char**) malloc(num_words_estimate * sizeof(char*));
  int num_words = 0;  // Assumes that the first character is not alphanumeric.
  int* word_mapping = (int*) malloc(num_words_estimate * 2 * sizeof(int));
  int num_lines = 0;

  #pragma mta fence
  timer.stop();
  printf("time for construction of node buffer %f\n",
         timer.getElapsedSeconds());
  printf("About to find the words\n");
  timer.start();

  int temp1 = 0;
  process_line(num_words, words, array, temp1, num_lines, word_mapping);
  num_lines++;

  #pragma mta assert parallel
  #pragma mta block schedule
  #pragma mta use 100 streams
  for (int i = 2; i < num_chars - 2; ++i)
  {
    if (array[i] == '\n')
    {
      array[i] = '\0';
      int j = i + 1;
      int my_line = mt_incr(num_lines, 1);
      process_line(num_words, words, array, j, my_line, word_mapping);
    }
  }

  array[num_chars - 1] = '\0'; // Might mess up the last word.

  #pragma mta fence
  timer.stop();
  printf("Time to parse words%f\n", timer.getElapsedSeconds());
  printf("Number of words %d\n", num_words);
  printf("about to insert\n");
  #pragma mta fence
  timer.start();

  int* output_buffer;
  int size_output_buffer = 4 * num_lines * sizeof(int);
  if (outfile != NULL)
  {
    output_buffer = (int*) malloc(sizeof(int) * size_output_buffer);
    #pragma mta fence
    timer.stop();
    printf("Time to create output buffer %f\n", timer.getElapsedSeconds());
    timer.start();
  }

  int word_id = 0;
  int one = 1;
  #pragma mta assert parallel
  #pragma mta block schedule
  for (int i = 0; i < num_words; ++i)
  {
    if (strlen(words[i]) > 0)
    {
      set->insert(words[i], i);
      if (outfile != NULL )
      {
        int id = set->get(words[i]);
        int line_number = word_mapping[2 * i];
        int quad_index = word_mapping[2 * i + 1];
        output_buffer[line_number * 4 + quad_index] = id;
      }
    }
  }

  #pragma mta fence
  timer.stop();
  printf("Time for inserts %f\n", timer.getElapsedSeconds());

  if (rdfs_outfile != NULL)
  {
    create_index(set, output_buffer, num_words);
  }

  // Not including time to go to standard out since it isn't usually
  // used during timing runs.
  if (print_output)
  {
    for (int i = 0; i < num_words; i++)
    {
      printf("%s %d\n", words[i], set->get(words[i]));
    }

    for (int i = 0; i < num_lines; i++)
    {
      printf("%d %d %d %d\n", output_buffer[i * 4], output_buffer[i * 4 + 1],
             output_buffer[i * 4 + 2], output_buffer[i * 4 + 3]);
    }
  }

  // Not including this in the time since it is for diagnostics.
  int num_unique = set->get_num_unique_keys();
  printf("num_unique %d\n", num_unique);

  #pragma mta fence
  timer.start();

  if (outfile != NULL)
  {
    snap_snapshot(outfile, output_buffer, size_output_buffer, &snap_error);
    #pragma mta fence
    timer.stop();
    printf("Time for snapshotting the output %f\n", timer.getElapsedSeconds());
    timer.start();
  }

  delete (set);
  free(words);
  free(file_buffer);

  #pragma mta fence
  timer.stop();
  printf("Time for deletion %f\n", timer.getElapsedSeconds());
}

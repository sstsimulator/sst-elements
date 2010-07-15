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

// *****************************************************************
// Algorithm Copyright Pacific Northwest National Laboratories 2009.
// *****************************************************************

/****************************************************************************/
/*! \file hotel_hash_set.hpp

    \brief A thread-safe key-based dictionary abstraction.

    \author Eric Goodman (elgoodm@sandia.gov)

    \date 10/14/2009

    Concerning the extensive use of macros:  To be the most general, I could
    have made everything K*& and V*&, so that you could use the template with
    both primitive types and objects.  However, K*& and V*& seemed a bit
    overboard for primitive types, and I also noticed a performance hit with
    all the pointer dereferencing.  Thus, I decided to make a general template
    with a number of specializations (i.e. 1 base one and 3 specializations).
    However, the code was extremely similar in all cases, so I decided to use
    Macros to reduce the code duplication.  It's not the friendliest to read
    or debug, but it seemed like a good tradeoff so I don't have to propagate
    the changes to one template to all the rest.
*/
/****************************************************************************/

#ifndef MTGL_HOTEL_HASH_SET_HPP
#define MTGL_HOTEL_HASH_SET_HPP

#include <cmath>

#include <mtgl/util.hpp>

namespace mtgl {

template <typename K, typename V, typename IF, typename HF, typename EQF>
class hotel_hash_set;

template <typename K, typename V>
class unsafe_insert_function;

template <typename K, typename V>
class safe_insert_function;

template <typename K, typename V>
class min_insert_function;

template <typename K, typename V>
class add_function;

#define HASH_SET_CONSTRUCTOR(K, V)                               \
  hotel_hash_set(int size, bool use_local = true) :              \
    abstract_hash_set < IF, HF, EQF > (size, use_local)          \
    {                                                            \
      keys = (K*) malloc(sizeof(K) * table_size);                \
      values = (V*) calloc(table_size, sizeof(V));               \
      visited_keys = (K*) malloc(sizeof(K) * table_size);        \
      local_keys = (K*) malloc(TOTAL_SIZE_CACHE * sizeof(K));    \
      local_values = (V*) malloc(TOTAL_SIZE_CACHE * sizeof(V));  \
    }

//code that dealt with the concise key list
/*if(!was_occupied) {
        int pos = mt_incr(&num_visited, 1);
        visited_keys[pos] = key;
}*/

#define HASH_SET_INSERT_GLOBAL(K, V)                      \
  bool insert_global(K key, V value)                      \
  {                                                       \
    bool was_occupied;                                    \
    int index = get_global_index(key, was_occupied);      \
    bool result = insert_func(key, value, was_occupied,   \
                              false, values, index);      \
    return result;                                        \
  }

#define HASH_SET_INSERT(K, V)                                \
  bool insert(K key, V value)                                \
  {                                                          \
    bool was_occupied;                                       \
    bool is_local;                                           \
    int index;                                               \
    index = get_index(key, was_occupied, is_local);          \
    bool result;                                             \
    if (is_local)                                            \
    {                                                        \
      result = insert_func(key, value, was_occupied,         \
                           is_local, local_values, index);   \
    }                                                        \
    else                                                     \
    {                                                        \
      result = insert_func(key, value, was_occupied,         \
                           is_local, values, index);         \
    }                                                        \
    return result;                                           \
  }

#define HASH_SET_GET(K, V)                             \
  V get(K key)                                         \
  {                                                    \
    bool was_occupied;                                 \
    int index = get_global_index(key, was_occupied);   \
    if (was_occupied)                                  \
    {                                                  \
      return values[index];                            \
    }                                                  \
    else                                               \
    {                                                  \
      return NULL;                                     \
    }                                                  \
  }

/*! \fn get_index(K key, bool &was_occupied, bool &is_local)
    \brief Gets the index into the value array for a give key.
    \param key The key to get the index for.
    \param was_occupied If the bucket was previously occupied, this is set
                        to true.
    \param is_local Set to true if the value was accessed using the
                    thread-dedicated local storage.
*/
#define HASH_SET_GET_INDEX(K)                                    \
  int get_index(K key, bool & was_occupied, bool & is_local)     \
  {                                                              \
    int index = get_local_index(key, was_occupied);              \
    if (index >= 0)                                              \
    {                                                            \
      is_local = true;                                           \
      return index;                                              \
    }                                                            \
    is_local = false;                                            \
    return get_global_index(key, was_occupied);                  \
  }

/*! \fn get_global_index(K key, bool &was_occupied)
    \brief Accesses the global hash set instead of the local hash set for
           the given thread.
    \param key The key to get the index for.
    \param was_occupied If the bucket was previously occupied, this is set
                        to true.
*/
#define HASH_SET_GET_GLOBAL_INDEX(K)                   \
  int get_global_index(K key, bool & was_occupied)     \
  {                                                    \
    int index = hash(key);                             \
    unsigned int i = index;                            \
    was_occupied = false;                              \
    do                                                 \
    {                                                  \
      int probed = occupied[i];                        \
      if (probed > 0)                                  \
      {                                                \
        if (compare(keys[i], key))                     \
        {                                              \
          was_occupied = true;                         \
          return i;                                    \
        }                                              \
      }                                                \
      else                                             \
      {                                                \
        probed = readfe(&occupied[i]);                 \
        if (probed == 0)                               \
        {                                              \
          keys[i] = key;                               \
          writeef(&occupied[i], 1);                    \
          return i;                                    \
        }                                              \
        else                                           \
        {                                              \
          if (compare(keys[i], key))                   \
          {                                            \
            writeef(&occupied[i], 1);                  \
            was_occupied = true;                       \
            return i;                                  \
          }                                            \
          writeef(&occupied[i], 1);                    \
        }                                              \
      }                                                \
      i++;                                             \
      if (i == table_size) i = 0;                      \
    } while (i != index);                              \
    printf("return -1 in get_index %d\n", key);        \
    return -1;                                         \
  }

/*! \fn get_local_index(K key, bool &was_occupied)
    \brief Gets an index from the local hash set, rather than from the global
           instance.
    \param key The key to get the index for.
    \param was_occupied If the bucket was previously occupied, this is set
                        to true.
*/
#define HASH_SET_GET_LOCAL_INDEX(K)                                       \
  int get_local_index(K key, bool & was_occupied)                         \
  {                                                                       \
    unsigned int stream_num = ((MTA_DOMAIN_IDENTIFIER_SAVE() >> 1) +      \
                               MTA_STREAM_IDENTIFIER_SAVE());             \
    unsigned int index = hash_local(key);                                 \
    index = index + stream_num * MAX_CACHE_SIZE;                          \
    unsigned int beg_cache = stream_num * MAX_CACHE_SIZE;                 \
    unsigned int end_cache = ((stream_num + 1) * MAX_CACHE_SIZE) - 1;     \
    unsigned int i = index;                                               \
    was_occupied = false;                                                 \
    do {                                                                  \
      int probed = local_occupied[i];                                     \
      if (probed > 0)                                                     \
      {                                                                   \
        if (compare(local_keys[i], key))                                  \
        {                                                                 \
          was_occupied = true;                                            \
          return i;                                                       \
        }                                                                 \
      }                                                                   \
      else if (num_elements_stream[stream_num] < (MAX_CACHE_SIZE >> 1))   \
      {                                                                   \
        local_occupied[i] = 1;                                            \
        local_keys[i] = key;                                              \
        num_elements_stream[stream_num]++;                                \
        was_occupied = false;                                             \
        return i;                                                         \
      }                                                                   \
      else                                                                \
      {                                                                   \
        return -1;                                                        \
      }                                                                   \
      i++;                                                                \
      if (i == end_cache) i = beg_cache;                                  \
    } while (i != index);                                                 \
    return -1;                                                            \
  }

#define HASH_SET_PRINT(ptrtype1, ptrtype2)                                   \
  void print(int num)                                                        \
  {                                                                          \
    if (num > table_size) num = table_size;                                  \
    int seen = 0;                                                            \
    int mysize = get_num_unique_keys();                                      \
    printf("table_size %d num occupied buckets %d\n", table_size, mysize);   \
    if (num != table_size)                                                   \
    {                                                                        \
      for (int i = 0; i < table_size && seen < num; i++)                     \
      {                                                                      \
        if (occupied[i] == 1)                                                \
        {                                                                    \
          printf("%d %d: %d\n", i, ptrtype1 keys[i],                         \
                 ptrtype2 values[i]);                                        \
          seen = seen + 1;                                                   \
        }                                                                    \
      }                                                                      \
    }                                                                        \
    else                                                                     \
    {                                                                        \
      for (int i = 0; i < table_size; i++)                                   \
      {                                                                      \
        if (occupied[i] == 1)                                                \
        {                                                                    \
          printf("%d %d: %d\n", i, ptrtype1 keys[i],                         \
                 ptrtype2 values[i]);                                        \
          mt_incr(seen, 1);                                                  \
        }                                                                    \
      }                                                                      \
    }                                                                        \
  }

#define HASH_SET_GET_RAW_KEYS(K)   \
  K * get_raw_keys(int & size)     \
  {                                \
    size = table_size;             \
    return keys;                   \
  }

#define HASH_SET_GET_RAW_VALUES(V)   \
  V * get_raw_values(int & size)     \
  {                                  \
    size = table_size;               \
    return values;                   \
  }

#define HASH_SET_GET_CONCISE_KEY_LIST(K)                    \
  K * get_concise_key_list(int & num_keys)                  \
  {                                                         \
    num_keys = 0;                                           \
    int* array = (int*) malloc(sizeof(int) * table_size);   \
    for (int i = 0; i < table_size; i++)                    \
    {                                                       \
      if (occupied[i] == 1)                                 \
      {                                                     \
        int pos = mt_incr(num_keys, 1);                     \
        array[pos] = keys[i];                               \
      }                                                     \
    }                                                       \
                                                            \
    return array;                                           \
  }

#define HASH_SET_GET_KEYS(K)   \
  K * get_keys(int & size)     \
  {                            \
    size = num_visited;        \
    return visited_keys;       \
  }

#define HASH_SET_GET_NUM_VALUES_1(V)   \
  int get_num_values(V value)          \
  {                                    \
    int num = 0;

#define HASH_SET_GET_NUM_VALUES_2                \
    for (int i = 0; i < table_size; i++)           \
    {                                              \
      if (occupied[i] == 1)                        \
      {                                            \
        if (values[i] == value) mt_incr(num, 1);   \
      }                                            \
    }                                              \
    return num;                                    \
  }

#define HASH_SET_SET_ALL_VISITED_1    \
  void set_all_visited()              \
  {                                   \
    int index = 0;

#define HASH_SET_SET_ALL_VISITED_2         \
    for (int i = 0; i < table_size; i++)   \
    {                                      \
      if (occupied[i] == 1)                \
      {                                    \
        int pos = mt_incr(index, 1);       \
        visited_keys[pos] = keys[i];       \
      }                                    \
    }                                      \
    num_visited = index;                   \
  }

#define HASH_SET_PRIVATE_VARIABLES(K, V)   \
  V * values;                              \
  K* keys;                                 \
  K* visited_keys;                         \
  V* local_values;                         \
  K* local_keys;

#define HASH_SET_HASH_AND_COMPARE_FUNCTIONS(K)   \
  unsigned int hash(K key)                       \
  {                                              \
    return hash_func(key) & global_mask;         \
  }                                              \
                                                 \
  unsigned int hash_local(K key)                 \
  {                                              \
    return hash_func(key) & local_mask;          \
  }                                              \
                                                 \
  bool compare(K key1, K key2)                   \
  {                                              \
    return compare_func(key1, key2);             \
  }

#define HASH_SET_CONSOLIDATE_LOOP                                  \
  for (int i = 0; i < TOTAL_SIZE_CACHE; i++)                       \
  {                                                                \
    if (local_occupied[i] > 0)                                     \
    {                                                              \
      bool was_occupied;                                           \
      int index = get_global_index(local_keys[i], was_occupied);   \
      insert_func(local_keys[i], local_values[i], was_occupied,    \
                  false, values, index);                           \
    }                                                              \
  }

#define HASH_SET_GET_KEYS_LOOP           \
  for (int i = 0; i < table_size; i++)   \
  {                                      \
    if (occupied[i] > 0)                 \
    {                                    \
      mt_incr(num_keys, 1)

#define MAX_NUM_DOMAINS 128
#define MAX_NUM_STREAMS 128
#define MAX_CACHE_SIZE 32768
#define TOTAL_SIZE_CACHE MAX_NUM_DOMAINS * MAX_NUM_STREAMS * MAX_CACHE_SIZE

template <typename IF, typename HF, typename EQF>
class abstract_hash_set {
public:
  abstract_hash_set(int size, bool in_use_local = false) : use_local(use_local)
  {
    double logSize = log(size) / log(2);
    if (size - floor(size) == 0)
    {
      table_size = pow(2, floor(logSize) + 1);
    }
    else
    {
      table_size = pow(2, logSize);
    }

    global_mask = table_size - 1;
    occupied = (int*) calloc(table_size, sizeof(int));
    num_visited = 0;
    num_unique_keys = -1;

    if (use_local)
    {
      local_mask = MAX_CACHE_SIZE - 1;
      local_occupied = (int*) calloc(TOTAL_SIZE_CACHE, sizeof(int));
      num_elements_stream =
        (int*) calloc(MAX_NUM_DOMAINS * MAX_NUM_STREAMS, sizeof(int));
    }
  }

  /* \brief Since we keep the insert function as minimal as possible,
            the number of unique keys is not known without a post-
            processing step.  You must call this function to determine
            the number of unique keys.
  */
  int get_num_unique_keys()
  {
    if (num_unique_keys < 0)
    {
      int total = 0;

      #pragma mta assert parallel
      for (int i = 0; i < table_size; i++)
      {
        if (occupied[i] == 1) mt_incr(total, 1);
      }

      num_unique_keys = total;

      return num_unique_keys;
    }
    else
    {
      return num_unique_keys;
    }
  }

  /* \fn reset_num_unique_keys()
     \brief We keep the insert function as minimal as possible, so many things
            that we should keep track of are left for post-processing.  Thus,
            getting the number of unique keys is left for after insertion.
            However, if you later do an insert again, the number of unique
            keys will not be updated, and you need to indicate that the
            num_unique_keys is outdated by using this function.
  */
//  int reset_num_unique_keys() { num_unique_keys = -1; }

  int* get_occupied(int& size)
  {
    size = table_size;
    return occupied;
  }

  /// Resets all keys to being unvisited.
  void reset_visited() { num_visited = 0; }


protected:

  HF hash_func;
  EQF compare_func;
  IF insert_func;
  int num_visited;       // The number of buckets that have been visited and
                         // changed.

  int table_size;        // Size of the table set at initialization.
  int num_unique_keys;
  int* occupied;         // Used for determining if a bucket is in use or not
                         // in the global array.
  int* local_occupied;   // Used for determining if a bucket is in use or not
                         // in the local array.
  int global_mask;       // Used to quickly determine indices into the global
                         // array.
  int local_mask;        // Used to quickly determine indices int the local
                         // array.

  int* num_elements_stream;

  bool use_local;        // Determines if we use the local array or just the
                         // global array.
};


template <typename K, typename V, typename IF = unsafe_insert_function<K, V>,
          typename HF = default_hash_func<K>, typename EQF = default_eqfcn<K> >
class hotel_hash_set : abstract_hash_set<IF, HF, EQF> {
public:
  HASH_SET_CONSTRUCTOR(K, V)
  HASH_SET_INSERT(K, V)
  HASH_SET_INSERT_GLOBAL(K, V)
  HASH_SET_GET(K, V)
  HASH_SET_GET_RAW_KEYS(K)
  HASH_SET_GET_RAW_VALUES(V)
  HASH_SET_PRINT(, )

  HASH_SET_GET_NUM_VALUES_1(V)
  #pragma mta assert parallel
  HASH_SET_GET_NUM_VALUES_2

  void reset_visited() { abstract_hash_set<IF, HF, EQF>::reset_visited(); }

  int* get_occupied(int& size)
  {
    return abstract_hash_set<IF, HF, EQF>::get_occupied(size);
  }

  HASH_SET_SET_ALL_VISITED_1
  #pragma mta assert parallel
  HASH_SET_SET_ALL_VISITED_2


  void consolidate()
  {
    #pragma mta assert parallel
    HASH_SET_CONSOLIDATE_LOOP
  }

private:
  HASH_SET_PRIVATE_VARIABLES(K, V)
  HASH_SET_GET_GLOBAL_INDEX(K)
  HASH_SET_GET_INDEX(K)
  HASH_SET_GET_LOCAL_INDEX(K)
  HASH_SET_HASH_AND_COMPARE_FUNCTIONS(K)
};

template <typename K, typename V, typename IF, typename HF, typename EQF>
class hotel_hash_set<K, V*, IF, HF, EQF> : abstract_hash_set<IF, HF, EQF> {
public:
  HASH_SET_CONSTRUCTOR(K, V*)
  HASH_SET_INSERT(K, V*)
  HASH_SET_INSERT_GLOBAL(K, V*)
  HASH_SET_GET(K, V*)
  HASH_SET_GET_RAW_KEYS(K)
  HASH_SET_GET_RAW_VALUES(V*)
  HASH_SET_PRINT(, *)

  HASH_SET_GET_NUM_VALUES_1(V*)
  #pragma mta assert parallel
  HASH_SET_GET_NUM_VALUES_2

  void reset_visited() { abstract_hash_set<IF, HF, EQF>::reset_visited(); }

  int* get_occupied(int& size)
  {
    return abstract_hash_set<IF, HF, EQF>::get_occupied(size);
  }

  HASH_SET_SET_ALL_VISITED_1
  #pragma mta assert parallel
  HASH_SET_SET_ALL_VISITED_2

  void consolidate()
  {
    #pragma mta assert parallel
    HASH_SET_CONSOLIDATE_LOOP
  }

private:
  HASH_SET_PRIVATE_VARIABLES(K, V*)
  HASH_SET_GET_INDEX(K)
  HASH_SET_GET_GLOBAL_INDEX(K)
  HASH_SET_GET_LOCAL_INDEX(K)
  HASH_SET_HASH_AND_COMPARE_FUNCTIONS(K)
};

template <typename K, typename V, typename IF, typename HF, typename EQF>
class hotel_hash_set<K*, V*, IF, HF, EQF> : abstract_hash_set<IF, HF, EQF> {
public:
  HASH_SET_CONSTRUCTOR(K *, V*)
  HASH_SET_INSERT(K *, V*)
  HASH_SET_INSERT_GLOBAL(K *, V*)
  HASH_SET_GET(K *, V*)
  HASH_SET_GET_RAW_KEYS(K*)
  HASH_SET_GET_RAW_VALUES(V*)
  HASH_SET_PRINT(*, *)

  HASH_SET_GET_NUM_VALUES_1(V*)
  #pragma mta assert parallel
  HASH_SET_GET_NUM_VALUES_2

  void reset_visited() { abstract_hash_set<IF, HF, EQF>::reset_visited(); }

  int* get_occupied(int& size)
  {
    return abstract_hash_set<IF, HF, EQF>::get_occupied(size);
  }

  HASH_SET_SET_ALL_VISITED_1
  #pragma mta assert parallel
  HASH_SET_SET_ALL_VISITED_2

  void consolidate()
  {
    #pragma mta assert parallel
    HASH_SET_CONSOLIDATE_LOOP
  }

private:
  HASH_SET_PRIVATE_VARIABLES(K *, V*)
  HASH_SET_GET_INDEX(K*)
  HASH_SET_GET_GLOBAL_INDEX(K*)
  HASH_SET_GET_LOCAL_INDEX(K*)
  HASH_SET_HASH_AND_COMPARE_FUNCTIONS(K*)
};

template <typename K, typename V>
class unsafe_insert_function {
public:
  bool operator()(K key, V value, bool was_occupied, bool is_local,
                  V* values, int index) const
  {
    values[index] = value;
    return true;
  }
};

template <typename K, typename V>
class min_insert_function {
public:
  /*! \brief Inserts value if it is less than the current value or if the
             bucket is unoccupied.  Uses the '<' operator, so must be
             properly overriden for type V.
      \param key The key into the hash set
      \param value The value to insert
  */
  bool operator()(K key, V value, bool was_occupied, bool is_local,
                  V* values, int index) const
  {
    if (is_local)
    {
      if (was_occupied)
      {
        if (value < values[indes]) values[index] = value;
      }
      else
      {
        values[index] = value;
      }
    }
    else
    {
      if (was_occupied)
      {
        V probe_value = readff(&values[index]);

        if (value < probe_value)
        {
          probe_value = readfe(&values[index]);

          if (value < probe_value)
          {
            writeef(&values[index], value);
            return true;
          }
          else
          {
            writeef(&values[index], probe_value);
          }
        }
      }
      else
      {
        // Expects that value should have been locked.
        writeef(&values[index], value);
        return true;
      }

    }
  }
};

template <typename K, typename V>
class add_function {
public:
  bool operator()(K key, V value, bool was_occupied, bool is_local,
                  V* values, int index) const
  {
    if (is_local)
    {
      values[index] += value;
    }
    else
    {
      mt_incr(&values[index], value);
    }

    return true;
  }
};

}

#endif

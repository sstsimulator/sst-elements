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
/*! \file multimap.hpp

    \author Eric Goodman (elgoodm@sandia.gov)

    \date 3/25/2010
*/
/****************************************************************************/

#ifndef MTGL_MULTIMAP_HPP
#define MTGL_MULTIMAP_HPP

#include <mtgl/hash_defs.hpp>
#include <mtgl/util.hpp>
#include <mtgl/allocation_pool.hpp>

namespace mtgl {

template <typename K, typename V, typename HF = default_hash_func<K>,
          typename EQF = default_eqfcn<K> >
class multimap {
public:

  // Table size should be about ? times the number of expected keys.
  // value_stoarge_size should be about ? times the number of expected values.
  inline multimap(int in_table_size, int in_value_storage_size = 0)
  {
    // Making the hash table size a power of two.
    double logSize = log(in_table_size) / log(2);
    if (in_table_size - floor(in_table_size) == 0)
    {
      table_size = pow(2, floor(logSize) + 1);
    }
    else
    {
      table_size = pow(2, logSize);
    }

    // Setting the mask so that we can use & instead of % to calculate
    // remainders.
    mask = table_size - 1;

    count = (int*) calloc(table_size, sizeof(int));
    occupied = (int*) calloc(table_size, sizeof(int));
    capacity = (int*) calloc(table_size, sizeof(int));
    keys = (K*) malloc(sizeof(K) * table_size);

    if (in_value_storage_size > 0)
    {
      int num_sections = log(in_value_storage_size);
      pool = new allocation_pool<V>(in_value_storage_size, num_sections,
                                    MALLOC_STYLE);
    }
    else
    {
      pool = NULL;
    }

//    misses = 0;
  }

  inline ~multimap()
  {
    free(count);
    free(occupied);
    free(keys);
    free(capacity);
    if (pool != NULL) delete pool;
  }

  /// \brief Initializes the storage arrays as indicated by the count array.
  ///        This should be called after all the increment calls are done.
  inline void initialize_value_storage()
  {
    values = (V**) malloc(sizeof(V*) * table_size);

    if (pool != NULL)
    {
      #pragma mta assert parallel
      #pragma mta block schedule
      for (int i = 0; i < table_size; i++)
      {
        capacity[i] = count[i];

        if (occupied[i] == 1) values[i] = pool->get(count[i]);
      }
    }
    else
    {
      #pragma mta assert parallel
      #pragma mta block schedule
      for (int i = 0; i < table_size; i++)
      {
        capacity[i] = count[i];

        if (occupied[i] == 1) values[i] = (V*) malloc(sizeof(V) * count[i]);
      }
    }
  }

  inline void resize()
  {
    if (pool != NULL)
    {
      #pragma mta assert parallel
      #pragma mta block schedule
      for (int i = 0; i < table_size; i++)
      {
        if (count[i] > capacity[i])
        {
          V* new_storage = pool->get(count[i]);

          for (int j = 0; j < capacity[i]; j++) new_storage[j] = values[i][j];

          values[i] = new_storage;
          capacity[i] = count[i];
        }
      }
    }
    else
    {
      #pragma mta assert parallel
      #pragma mta block schedule
      for (int i = 0; i < table_size; i++)
      {
        if (count[i] > capacity[i])
        {
          V* new_storage = (V*) malloc(sizeof(V) * count[i]);

          for (int j = 0; j < capacity[i]; j++) new_storage[j] = values[i][j];

          free(values[i]);

          values[i] = new_storage;
          capacity[i] = count[i];
        }
      }
    }
  }

  /*! \brief Inserts the value into the array for the given key at
             value_index.  This should be called after
             initialize_value_storage.

      \param key The key.
      \param value The value to be inserted.
      \param value_index The index within the array for a given key that the
                         value should be placed.
      \returns Returns true if the operation was successful.
  */
  inline bool insert(K key, V value, int value_index)
  {
    int index = get_index(key, false);
    values[index][value_index] = value;
    return true;
  }

  inline int increment(K key)
  {
    int index = get_index(key, true);
    return mt_incr(count[index], 1);
  }

  inline bool member(K key)
  {
    int index = get_index(key, false);

    return index >= 0;
  }

  inline K* get_keys(int& size, K* key_buffer)
  {
    int index = 0;

    #pragma mta assert parallel
    for (int i = 0; i < table_size; i++)
    {
      if (occupied[i] == 1)
      {
        int pos = mt_incr(index, 1);
        key_buffer[pos] = keys[i];
      }
    }

    size = index;

    return key_buffer;
  }

  inline bool lookup(K key, int& num_values, V*& in_values)
  {
    int index = get_index(key, false);

    if (index < 0)
    {
      num_values = 0;
      return false;
    }

    num_values = count[index];
    in_values = values[index];

    return true;
  }

  inline V* get_values(K key, int& num_values)
  {
    int index = get_index(key, false);

    if (index < 0)
    {
      num_values = 0;
      return NULL;
    }

    num_values = count[index];

    return values[index];
  }

  inline V get(K key, int value_index)
  {
    int index = get_index(key, false);
    return values[index][value_index];
  }

  inline int get_table_size() { return table_size; }

  /*! \brief Returns the number of total elements within the multimap (i.e.
             the total number of values, not keys).

      \returns Returns an integer

      The total size is computed as post-analysis method rather than during
      insertion as keeping track during insertion can cause hot-spotting.

      There might be some useful optimizations where we only compute the size
      if a valid cached size is not available.  However, any modifications to
      keep track of validity of the stored value might impact the efficiency
      of the insert or increment functions.
  */
  inline int size()
  {
    int total = 0;

    #pragma mta assert parallel
    for (int i = 0; i < table_size; i++)
    {
      if (occupied[i] == 1) mt_incr(total, count[i]);
    }

    return total;
  }

  inline int get_count(K key)
  {
    int index = get_index(key, false);
    return count[index];
  }

  inline void print() { print(table_size); }

  inline void print(int num)
  {
    if (num > table_size) num = table_size;

    int seen = 0;
    for (int i = 0; i < table_size && seen < num; i++)
    {
      if (occupied[i] == 1)
      {
        mt_incr(seen, 1);
        printf("key %d i %d: ", keys[i], i);

        for (int j = 0; j < count[i]; j++) printf("%d, ", values[i][j]);

        printf("\n");
      }
    }
  }

  /*! \brief Writes the multimap out to a binary file.

      \param filename The location where the multimap is to be saved.

      This creates a file with the following format:
      - table_size, size: sizeof(int)
      - count array, size: table_size*sizeof(int)
      - occupied array, size: table_size*sizeof(int)
      - key array, size: table_size * sizeof (K)

      Note: Don't believe this will work for K or V that are object types.
  */
  inline void write(char* filename)
  {
    int num_keys;
    K* keys = get_keys(num_keys);
    int s1 = size();
    int temp;
    K key;
    int max_count = get_max_count(temp, key);
    printf("max_count %d\n", max_count);

    int file_size = (2 * table_size + 1) * sizeof(int) +
                    table_size * sizeof(K) +
                    table_size * max_count * sizeof(V);

    void* array = (void*) malloc(file_size);
    ((int*) array)[0] = table_size;

    #pragma mta assert nodep
    for (int i = 0; i < table_size; i++)
    {
      ((int*) array)[i + 1] = count[i];
      ((int*) array)[2 * i + 1] = occupied[i];
    }

    K* key_pointer = &((int*) array)[2 * table_size + 1];

    #pragma mta assert nodep
    for (int i = 0; i < table_size; i++) key_pointer[i] = keys[i];

    int total_bytes = 0;
    void* value_pointer = &(key_pointer[table_size + 1]);

//    #pragma mta assert parallel
//    for(int i = 0; i < table_size; i++) {
//    }

    free(keys);
    free(array);
  }

/*
  void read(char* filename)
  {
    int* array = read_array<int>(filename);

    table_size = array[0];              // First int is the table size.
    count = &array[1];                  // Next come the count array.
    occupied = &array[table_size + 1];  // Then the occupied array.

    int start_keys = &array[2*table_size + 1];
    keys = start_keys;
  }
*/

  /*! \brief Finds which element has the largest count and returns the count.

      \param index This is set to the index of the element (the first one if
                   there are ties)

      \return Returns the count of the most prolific key
  */
  inline int get_max_count(int& index, K& key)
  {
    int max = -1;
    index = -1;

    #pragma mta assert parallel
    for (int i = 0; i < table_size; i++)
    {
      int current_max = readff(&max);

      if (count[i] > current_max)
      {
        current_max = readfe(&max);

        if (count[i] > current_max)
        {
          index = i;
          key = keys[i];
          writeef(&max, count[i]);
        }
        else
        {
          writeef(&max, current_max);
        }
      }
    }

    return max;
  }

private:
  int* count;
  int* occupied;
  int* capacity;
  int table_size;
  K* keys;
  V** values;
  int mask;
  HF hash_func;
  EQF compare;
  allocation_pool<V>* pool;

  inline int get_index(const K key, const bool claim_slot)
  {
    unsigned int index = hash(key);
    unsigned int i = index;
    do
    {
      int probed = readff(&occupied[i]);
      if (probed == 1)
      {
        if (compare(keys[i], key)) return i;
      }
      else
      {
        if (claim_slot)
        {
          probed = readfe(&occupied[i]);

          if (probed == 0)
          {
            keys[i] = key;
            writeef(&occupied[i], 1);
            return i;
          }
          else
          {
            if (compare(keys[i], key))
            {
              writeef(&occupied[i], 1);
              return i;
            }

            writeef(&occupied[i], 1);
          }
        }
        else
        {
          return -1;
        }
      }

      i++;
      if (i == table_size) i = 0;
    } while (i != index);

    return -1;
  }

  inline unsigned int hash(K key) { return hash_func(key) & mask; }
};

}

#endif

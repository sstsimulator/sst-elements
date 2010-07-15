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
/*! \file lp_hash_set.hpp

    \brief A thread-safe key-based dictionary abstraction.  It uses linear
           probing and open addressing.  To account for collisions, it employs
           a two-step acquistion process to claim a slot in the array.

    \author Eric Goodman (elgoodm@sandia.gov)

    \date 12/21/2009

    There is an array for keys, and array for values, and an occupied array,
    which holds the state of each slot in the array, whether or not it has
    been claimed.

    The lp_hash_set class is configurable, though for the most part the
    configuration is down via the insert classes, currently
        unsafe_insert_function
        min_insert_function
        add_function
    Each of these insert classes has two static variables, use_updated and
    lock_value.

    use_updated is used to determine if the lp_hash_set should keep track of
    which keys have been updated.  Any time there is an update to a value, a
    counter is incremented and the key is added to a dynamically growing list.
    When get_keys() is called, the list of modified keys (which can contain
    duplicates) is returned.  reset_updated() resets the list of modified keys
    to 0, while set_all_updated() adds all occupied slots in the array.  This
    pattern was developed in support of the single path shortest path
    MapReduce algorithm.

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

#ifndef MTGL_LP_HASH_SET_HPP
#define MTGL_LP_HASH_SET_HPP

#include <mtgl/util.hpp>
#include <mtgl/hash_defs.hpp>

namespace mtgl {

template <typename K, typename V>
class unsafe_insert_function;

template <typename K, typename V>
class min_insert_function;

template <typename K, typename V>
class add_function;

template <typename K, typename V, typename IF = unsafe_insert_function<K, V>,
          typename HF = default_hash_func<K>, typename EQF = default_eqfcn<K> >
class lp_hash_set {
public:
  /*! \brief Construtor for the lp_hash_set class.
      \param size The size of the table.  Right now it is rounded up to the
                  nearest power of two, though that may be unnecessary.  The
                  modulus operator appears to work just fine when the result
                  of the hash function is first anded with 2^53 - 1 (to remove
                  the traps associated with division operations on numbers
                  greater than 2^53).
  */
  inline lp_hash_set(int size)
  {
    int value = 16;
    if (size < 16)  size = 16;
    while (value < size)  { value *= 2; }
    table_size = value;

    global_mask = table_size - 1;

    occupied = (int*) calloc(table_size, sizeof(int));

    num_unique_keys = -1;

    if (insert_func.use_updated == true)
    {
      use_update_array = true;
    }
    else
    {
      use_update_array = false;
    }

    if (use_update_array) num_updated = 0;

    keys = (K*) malloc(sizeof(K) * table_size);
    values = (V*) calloc(table_size, sizeof(V));
    if (use_update_array) updated = (K*) malloc(sizeof(K) * table_size);
  }

  /// Destrutor for the lp_hash_set class.
  inline ~lp_hash_set()
  {
    free(occupied);
    free(keys);
    free(values);
    if (use_update_array) free(updated);
  }

  inline bool insert(K key, V value)
  {
    bool was_occupied;
    int index = get_index(key, was_occupied, insert_func.lock_value);
    bool result = insert_func(key, value, was_occupied, values, index, updated,
                              num_updated);
    return result;
  }

  inline V get(K key)
  {
    bool was_occupied;
    int index = get_index(key, was_occupied);

    if (was_occupied)
    {
      return values[index];
    }
    else
    {
      return __null;
    }
  }

  inline K* get_raw_keys(int& size)
  {
    size = table_size;
    return keys;
  }

  inline V* get_raw_values(int& size)
  {
    size = table_size;
    return values;
  }

  inline void print(int num)
  {
    if (num > table_size) num = table_size;

    int seen = 0;
    int mysize = get_num_unique_keys();

    printf("table_size %d num occupied buckets %d\n", table_size, mysize);

    if (num != table_size)
    {
      for (int i = 0; i < table_size && seen < num; i++)
      {
        if (occupied[i] == 1)
        {
          printf("%d %d: %d\n", i, keys[i], values[i]);
          seen = seen + 1;
        }
      }
    }
    else
    {
      for (int i = 0; i < table_size; i++)
      {
        if (occupied[i] == 1)
        {
          printf("%d %d: %d\n", i, keys[i], values[i]);
          mt_incr(seen, 1);
        }
      }
    }
  }

  inline K* get_keys(int& size)
  {
    if (use_update_array)
    {
      return get_updated_keys(size);
    }
    else
    {
      return get_concise_key_list(size);
    }
  }

  inline K* get_updated_keys(int& size)
  {
    size = num_updated;
    return updated;
  }

  inline int get_num_values(V value)
  {
    int num = 0;

    #pragma mta assert parallel
    for (int i = 0; i < table_size; i++)
    {
      if (occupied[i] == 1 && values[i] == value) mt_incr(num, 1);
    }

    return num;
  }

  inline K* get_concise_key_list(int& num_keys)
  {
    mt_timer timer;
    timer.start();
    num_keys = 0;
    K* array = (K*) malloc(sizeof(K) * table_size);
    timer.stop();
    printf("time to create key array %f\n", timer.getElapsedSeconds());
    timer.start();
    #pragma mta assert parallel
    #pragma mta block schedule
    for (int i = 0; i < table_size; i++)
    {
      if (occupied[i] == 1)
      {
        int pos = mt_incr(num_keys, 1);
        array[pos] = keys[i];
      }
    }
    #pragma mta fence
    timer.stop();
    printf("time to populate key array %f\n", timer.getElapsedSeconds());

    return array;
  }

  inline void set_all_updated()
  {
    #pragma mta assert parallel
    for (int i = 0; i < table_size; i++)
    {
      if (occupied[i] == 1)
      {
        int pos = mt_incr(num_updated, 1);
        updated[pos] = keys[i];
      }
    }
  }

  inline void reset_updated() { num_updated = 0; }

  /*! \brief Since we keep the insert function as minimal as possible,
             the number of unique keys is not known without a post-
             processing step.  You must call this function to determine
             the number of unique keys.
  */
  inline int get_num_unique_keys()
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

  inline int* get_occupied(int& size)
  {
    size = table_size;
    return occupied;
  }

  /// Resets all keys to being unvisited.
  inline void reset_visited() { num_visited = 0; }

private:
  /*! \brief Gets the index for the given key, claiming a slot for the key if
             not already claimed.
      \param key The key to get the index for.
      \param was_occupied If the bucket was previously occupied, this is set
                          to true.
      \param lock_value If the bucket was unoccupied, then the value array at
                        the given index is locked with a readfe.
  */
  inline int get_index(K key, bool& was_occupied, bool lock_value = false)
  {
    unsigned int index = hash(key);
    unsigned int i = index;
    was_occupied = false;

    do
    {
      int probed = occupied[i];

      if (probed > 0)
      {
        if (compare(keys[i], key))
        {
          was_occupied = true;
          return i;
        }
      }
      else
      {
        probed = readfe(&occupied[i]);

        if (probed == 0)
        {
          if (lock_value) readfe(&values[i]);

          keys[i] = key;
          writeef(&occupied[i], 1);
          return i;
        }
        else
        {
          if (compare(keys[i], key))
          {
            writeef(&occupied[i], 1);
            was_occupied = true;
            return i;
          }

          writeef(&occupied[i], 1);
        }
      }

      i++;

      if (i == table_size) i = 0;
    } while (i != index);

    return -1;
  }

  inline unsigned int hash(K key) { return hash_func(key) & global_mask; }
  inline bool compare(K key1, K key2) { return compare_func(key1, key2); }

private:
  HF hash_func;
  EQF compare_func;
  IF insert_func;

  int table_size;  // Size of the table set at initialization.
  int num_unique_keys;

  // Used for determining if a bucket is in use or not in the global array.
  int* occupied;

  bool use_update_array;  // Indicates if we should use the update array.
  int num_updated;  // Keeps track of the number of updates to the array.
  int global_mask;  // Used to quickly determine indices into the global array.

  V* values;
  K* keys;
  K* updated;
};

template <typename K, typename V>
class unsafe_insert_function {
public:
  inline bool operator()(K key, V value, bool was_occupied, V* values,
                  int index, K* updated, int& num_updated) const
  {
    values[index] = value;
    return true;
  }

  // Used by the lp_hash_set class to determine if the updated array should
  // be used.
  const static bool use_updated;

  // Used by the lp_hash_set class to determine if get_index requires a lock
  // on the value when a slot is first declared occupied by the get_index
  // method.
  const static bool lock_value;
};

template <typename K, typename V>
const bool unsafe_insert_function<K, V>::use_updated = false;

template <typename K, typename V>
const bool unsafe_insert_function<K, V>::lock_value = false;

template <typename K, typename V>
class min_insert_function {
public:
  /*! \brief Inserts value if it is less than the current value or if the
             bucket is unoccupied.  Uses the '<' operator, so it must be
             properly overriden for type V.
      \param key The key into the hash set.
      \param value The value to insert.
      \param was_occupied Indicates if the slot was occupied previously.
      \param index The index where the value should be inserted.
      \param updated An array that signifies if a value has been updated.
      \return Returns true if an updated of the value occurred, false
              otherwise.
  */
  inline bool operator()(K key, V value, bool was_occupied, V* values,
                         int index, K* updated, int& num_updated) const
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
          int pos = mt_incr(num_updated, 1);
          updated[pos] = key;

          return true;
        }
        else
        {
          writeef(&values[index], probe_value);
          return false;
        }
      }
    }
    else
    {
      // Should have been locked during the get_index method.
      writeef(&values[index], value);
      int pos = mt_incr(num_updated, 1);
      updated[pos] = key;

      return true;
    }

    return false;
  }

  // Used by the lp_hash_set class to determine if the updated array should
  // be used.
  const static bool use_updated;

  // Used by the lp_hash_set class to determine if get_index requires a lock
  // on the value when a slot is first declared occupied by the get_index
  // method.
  const static bool lock_value;
};

template <typename K, typename V>
const bool min_insert_function<K, V>::use_updated = true;

template <typename K, typename V>
const bool min_insert_function<K, V>::lock_value = true;

template <typename K, typename V>
class add_function {
public:
  inline bool operator()(K key, V value, bool was_occupied, V* values,
                  int index, K* updated, int& num_updated) const
  {
    mt_incr(values[index], value);
    return true;
  }

  // Used by the lp_hash_set class to determine if the updated array should
  // be used.
  const static bool use_updated;

  // Used by the lp_hash_set class to determine if get_index requires a lock
  // on the value when a slot is first declared occupied by the get_index
  // method.
  const static bool lock_value;
};

template <typename K, typename V>
const bool add_function<K, V>::use_updated = false;

template <typename K, typename V>
const bool add_function<K, V>::lock_value = false;

}

#endif

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
/*! \file xmt_hash_table.hpp

    \brief A thread-safe key-based dictionary abstraction implemented using
           open address hashing.

    \author Greg Mackey (gemacke@sandia.gov)
    \author Eric Goodman (elgoodm@sandia.gov)
    \author Jon Berry (jberry@sandia.gov)

    \date 4/5/2008
*/
/****************************************************************************/

#ifndef MTGL_XMT_HASH_TABLE_HPP
#define MTGL_XMT_HASH_TABLE_HPP

#include <limits>
#include <iostream>

#include <mtgl/util.hpp>
#include <mtgl/hash_defs.hpp>

#define XMT_HASH_TABLE_INIT_SIZE 1024

namespace mtgl {

/*! \brief A map data structure implemented using hashing with open
           addressing.

    \author Greg Mackey (gemacke@sandia.gov)
    \author Eric Goodman (elgoodm@sandia.gov)
    \author Jon Berry (jberry@sandia.gov)

    \tparam K Key type.
    \tparam T Value type.
    \tparam HF Function object type defining the hash function.
    \tparam EQF Function object type defining the equality function for keys.

    The hash table uses fine-grained locking at the entry level, and it uses
    the double locking method developed by Eric Goodman of Sandia National
    Labs and David Haglin of Pacific Northwest National Labs.  It also avoids
    locking whenever possible.  Locking is only used when modifying a key.
    All of these methods increase the parallelism of the hash table.

    The hash table enforces unique keys, and it allows deletes.  Note that
    keys cannot be modified.  Once they are inserted into the hash, they can
    only be deleted.  The values, however, are modifiable via the update or
    the visit functions.  Because performing an int_fetch_add during insert on
    an element counter causes a hotspot, the table doesn't keep track of
    the number of elements currently in the table.  Instead, a linear cost
    size() function is provided.

    The [] operator is slightly less scalable and slightly slower than the
    other element access functions (insert, update, etc.).  Those functions
    should be preferred over the [] operator when possible.

    The hash table performs deletions by marking the entries as deleted.  The
    normal insert functions ignore these deleted entries when searching for an
    open spot in the table.  This method of deletion is known as tombstoning.
    Once a position in the table is deleted, the position is unavailable for
    use by later insertions.  Thus, erasing elements shrinks the size of the
    table.  This is done because reusing the elements makes the inserts
    more expensive in instruction count, loads, and stores, although the time
    performance was the same in my tests for inserts into an empty table.  If
    space is more of a concern than performance, insert functions that reuse
    the deleted positions are provided.  They are the insert functions whose
    names end in "_reuse".

    There are two main classes of functions: those designed to be called in a
    parallel context and those designed to be called in a serial context.  The
    parallel context functions can be further divided into functions that
    modify the keys and functions that don't.  Here is the division of
    functions:

      - Parallel Context Functions
         - Modify the Key
            - operator[]()
            - insert()
            - insert_reuse()
            - erase()
            - update_insert()

         - Don't Modify the Key
            - member()
            - lookup()
            - update()

      - Serial Context Functions
         - Constructors
         - Destructor
         - operator=()
         - size()
         - resize()
         - capacity()
         - empty()
         - swap()
         - clear()
         - visit()
         - get_keys()
         - assign_contiguous_ids()
         - print()

    Parallel context functions that don't modify the key are sometimes
    parallelized automatically by the compiler.  Sometimes, though, they need
    a "#pragma mta assert nodep" line before the loop containing the
    function.  Parallel context functions the modify the key require a
    "#pragma mta assert parallel" statement before the loop containing the
    function.  This is because these functions implement locking.  Most
    functions seem to get the best performance when block scheduled.  Here is
    an example of parallelizing insert, a parallel context function that
    modifies the key.

      #pragma mta block schedule
      #pragma mta assert parallel
      for (int i = 0; i < 1000; ++i)
      {
        xht.insert(i, i);
      }

    Here is an example of parallelizing update, a parallel context function
    that doesn't modify the key.

      #pragma mta block schedule
      #pragma mta assert nodep
      for (int i = 0; i < 1000; ++i)
      {
        xht.update(i, i * 2);
      }

    All of the parallel context functions can be interleaved with each other
    inside loops.  They are guaranteed to successfully complete to the
    expected state even though only functions that modify the key implement
    locking.  As an example, consider the case where two threads are updating
    the same entry in the table.  The outcome of this case depends on the
    order the threads access the entry.  Even with the threads interleaving
    execution, the only way an inconsistent state could be achieved is if the
    assignment of the new value is not atomic.  In this case locking around
    the assignment of the value would be required to achieve a consistent
    state.

    We provide versions of the update functions that take a third parameter
    of a function object that is applied to the value.  The second parameter
    for the update function is passed to the function object.  This provides
    a lot of flexibility for the user.  There are two pre-defined function
    objects for use with update in hash_defs.hpp: hash_mt_incr and hash_min.
    The functor hash_mt_incr peforms an atomic int-fetch-add on the value
    incrementing it by the second parameter passed to update.  The functor
    hash_min updates value with the second parameter passed to update only if
    the second parameter is less than the current value.  Here is the
    definition of hash_min as an example update function object.  All update
    function objects need to define the operator() as the example below where
    'v' is the current value in the hash table and 'value' is the second
    parameter to update.  Note that whatever locking is necessary on v to
    provide accurate results must be implemented by the function object.

      template <typename T>
      struct hash_mt_incr
      {
        void operator()(T& v, const T& value) const
        { mt_incr(v, value); }
      };

    Instead of providing iterators, which cause considerable complication with
    thread safety, the visit function is provided.  The visit function allows
    read-only iteration across all the elements in the hash.  A user need only
    define a visitor function object that accepts a key and a value as copy
    constructed or const reference parameters.  The () operator of the visitor
    object is applied to each element in the hash.  Below is an example of a
    visitor object that increments the value by the key.  All visitor objects
    need to define the operator() as the example below where 'k' is the key
    and 'v' is the value.

      class table_visitor {
      public:
        void operator()(const int& k, int& v) { v += k; }
      };

    Now an example of using the visit function.

      xmt_hash_table<int, int> ht;
      <Items inserted into the hash here.>
      table_visitor tv;
      ht.visit(tv);

    The hash table offers static resizing via the resize function.  Note that
    this function is the only way to resize the table.  Also, it is not
    thread-safe in the sense that other operations on the hash table cannot be
    called at the same time as the resize function.  The user must ensure that
    the resize function is not interleaved with other hash table functions.
    (i.e. The user shouldn't put the resize function inside a loop.)  Making
    the resize function completely thread-safe would require implementing a
    reader-writer locking scheme which could significantly slow down the hash
    table performance.
*/
template <typename K, typename T, typename HF = default_hash_func<K>,
          typename EQF = default_eqfcn<K> >
class xmt_hash_table {
private:
  class hash_table_node;

public:
  typedef K key_type;
  typedef T mapped_type;
  typedef T value_type;
  typedef EQF key_compare;
  typedef hash_size_type size_type;

  /// \brief The normal constructor.
  /// \param size The initial allocation size.
  inline xmt_hash_table(size_type size = XMT_HASH_TABLE_INIT_SIZE);

  /// \brief The copy constructor, which performs a deep copy.
  /// \param h The xmt_hash_table to be copied.
  inline xmt_hash_table(const xmt_hash_table& h);

  /// The destructor.
  inline ~xmt_hash_table();

  /// \brief The assignment operator, which performs a deep copy.
  /// \param rhs The xmt_hash_table to be copied.
  inline xmt_hash_table& operator=(const xmt_hash_table& rhs);

  /// \brief Returns the number of elements in the hash table.
  ///
  /// This is a linear cost operation.
  inline size_type size() const;

  /// \brief Resizes the hash table to have new_size slots.  If there are
  ///        any entries in the table, they are rehased to the new table.
  /// \param new_size The new size for the hash table.
  inline void resize(size_type new_size);

  /// Accessor method returns the allocation size of the table.
  size_type capacity() const { return table_size; }

  /// \brief Returns true if there are no elements in the hash table.
  ///
  /// This is a linear cost operation.
  inline size_type empty() const { return size() == 0; }

  /// \brief Returns a proxy object that resolves to a reference to the data
  ///        associated with key.
  /// \param key The search key.
  ///
  /// \deprecated This function has been deprecated.  It has horrible
  ///             performance and scaling, and using it can cause other
  ///             functions to have horrible performance and scaling.  It is
  ///             only included to not break old code.  Do NOT use it in new
  ///             code.  Instead, use the insert(), update(), or
  ///             update_insert() functions as appropriate.
  inline mapped_type& operator[](const key_type& key);

  /// \brief Tests if key exists in the table.
  /// \param key The search key.
  inline bool member(const key_type& key) const;

  /// \brief Returns the value associated with key.
  /// \param key The search key.
  /// \param value Return value that holds the value associated with key.
  inline bool lookup(const key_type& key, mapped_type& value) const;

  /// \brief Inserts an element into the hash table.
  /// \param key The insertion key.
  /// \param value The insertion value.
  inline bool insert(const key_type& key, const mapped_type& value);

  /// \brief Inserts an element into the hash table reusing deleted entries.
  /// \param key The insertion key.
  /// \param value The insertion value.
  ///
  /// This version of insert is slightly more expensive than insert().
  inline bool insert_reuse(const key_type& key, const mapped_type& value);

  /// \brief Erases key from the table.
  /// \param key The delete key.
  inline bool erase(const key_type& key);

  /// \brief Updates the value associated with key.
  /// \param key The update key.
  /// \param value The new value to associate with key.
  inline bool update(const key_type& key, const mapped_type& value);

  /// \brief Updates the value associated with key.
  /// \tparam Vis Type of the visitor function object.
  /// \param key The update key.
  /// \param value Parameter passed to the visitor function object.
  /// \param visitor The visitor function object that is applied to the value
  ///                associated with key.
  template <typename Vis> inline
  bool update(const key_type& key, const mapped_type& value, Vis visitor);

  /// \brief Updates the value associated with key.  If key doesn't exist in
  ///        the table, insert it with a value of value.
  /// \param key The update key.
  /// \param value The new value to associate with key.
  inline bool update_insert(const key_type& key, const mapped_type& value);

  /// \brief Updates the value associated with key.  If key doesn't exist in
  ///        the table, insert it with a value of value.
  /// \tparam Vis Type of the visitor function object.
  /// \param key The update key.
  /// \param value Parameter passed to the visitor function object or the
  ///              value associated with key if key doesn't already exist in
  ///              the hash table.
  /// \param visitor The visitor function object that is applied to the value
  ///                associated with key.
  template <typename Vis> inline
  bool update_insert(const key_type& key, const mapped_type& value,
                     Vis visitor);

  /// Swaps the contents of the two hash tables.
  inline void swap(xmt_hash_table& rhs);

  /// Clears all entries from the hash table.
  inline void clear();

  /// \brief Apply the method encapsulated by "visitor" to the key and data
  ///        of each table element, in parallel.
  /// \tparam Vis Type of the visitor function object.
  /// \param visitor A closure object containing a function to be applied to
  ///        each element of the table.
  template <typename Vis> inline void visit(Vis& visitor);

  /// \brief Returns a contiguous array containing the keys in the hash
  ///        table.
  inline void get_keys(size_type& num_keys, key_type* key_buffer);

  /// \brief Assigns contiguous values to each of the entries in the table.
  /// \param starting_value The starting value for the contiguous ids.
  inline void assign_contiguous_ids(mapped_type starting_value = 0);

  /// Prints the key-value pairs in the hash table.
  void print() const;

private:
  /// \brief Performs a deep copy from rhs to the calling object.
  /// \param rhs The xmt_hash_table to be copied.
  inline void deep_copy(const xmt_hash_table& rhs);

  /// \brief Returns the index into table where key belongs.
  ///
  /// Ands the user supplied hash function with the mask to get an index
  /// within the range 0 <= index < capacity().
  size_type hash(const key_type& key) const { return hash_func(key) & mask; }

private:
  size_type mask;
  hash_table_node* table;
  long* occupied;
  size_type table_size;
  EQF compare;
  HF hash_func;
};

/***/

/*! \class hash_table_node
    \brief This is the type of the hash table entries.
*/
template <typename K, typename T, typename HF, typename EQF>
class xmt_hash_table<K, T, HF, EQF>::hash_table_node {
public:
  K key;
  T data;
};

/***/

template <typename K, typename T, typename HF, typename EQF>
xmt_hash_table<K, T, HF, EQF>::xmt_hash_table(size_type size)
{
  size_type value = 16;
  if (size < 16)  size = 16;
  while (value < size)  { value *= 2; }
  printf("xmt table size %i\n", (int)value);
  table_size = value;
  mask = value - 1;

  // Allocate memory for table and occupied vector, and initialize occupied
  // vector to all 0's (empty).
  table = (hash_table_node*) malloc(table_size * sizeof(hash_table_node));
  occupied = (long*) calloc(table_size, sizeof(long));
}

/***/

template <typename K, typename T, typename HF, typename EQF>
xmt_hash_table<K, T, HF, EQF>::xmt_hash_table(const xmt_hash_table& h) :
    mask(h.mask), table_size(h.table_size)
{
  deep_copy(h);
}

/***/

template <typename K, typename T, typename HF, typename EQF>
xmt_hash_table<K, T, HF, EQF>::~xmt_hash_table()
{
  free(table);
  free(occupied);
}

/***/

template <typename K, typename T, typename HF, typename EQF>
xmt_hash_table<K, T, HF, EQF>&
xmt_hash_table<K, T, HF, EQF>::operator=(const xmt_hash_table& rhs)
{
  free(table);
  free(occupied);

  mask = rhs.mask;
  table_size = rhs.table_size;

  deep_copy(rhs);

  return *this;
}

/***/

template <typename K, typename T, typename HF, typename EQF>
typename xmt_hash_table<K, T, HF, EQF>::size_type
xmt_hash_table<K, T, HF, EQF>::size() const
{
  size_type num_elements = 0;
  for (size_type i = 0; i < table_size; ++i) num_elements += (occupied[i] == 2);
  return num_elements;
}

/***/

template <typename K, typename T, typename HF, typename EQF>
void
xmt_hash_table<K, T, HF, EQF>::resize(size_type new_size)
{
  // Get copy of old values.
  hash_table_node* old_table = table;
  long* old_occupied = occupied;
  size_type old_table_size = table_size;

  size_type num_elements = size();

  // Find the new size of the table.
  size_type value = 16;
  if (new_size < 16)  new_size = 16;
  while (value < new_size || value < num_elements)  { value *= 2; }
  table_size = value;
  mask = value - 1;

  // Allocate memory for table and occupied vector, and initialize occupied
  // vector to all 0's (empty).
  table = (hash_table_node*) malloc(table_size * sizeof(hash_table_node));
  occupied = (long*) calloc(table_size, sizeof(long));

  // If the table isn't empty, copy the existing entries.
  if (num_elements > 0)
  {
    // Insert all the existing entries into the new table.
    #pragma mta assert parallel
    #pragma mta block schedule
    for (size_type i = 0; i < old_table_size; ++i)
    {
      if (old_occupied[i] == 2) insert(old_table[i].key, old_table[i].data);
    }
  }

  // Delete memory for the old table and occupied vector.
  free(old_table);
  free(old_occupied);
}

/***/

template <typename K, typename T, typename HF, typename EQF>
T&
xmt_hash_table<K, T, HF, EQF>::operator[](const K& key)
{
  // Find the element.
  size_type index = hash(key);
  size_type i = index;

  do
  {
    if (occupied[i] == 2)
    {
      if (compare(table[i].key, key)) return table[i].data;
    }
    else if (occupied[i] == 0)
    {
      // Acquire the lock.
      long probed = mt_readfe(occupied[i]);

      // Make sure the entry is still empty.
      if (probed == 0)
      {
        // Add the element at the current position in table.
        table[i].key = key;
        table[i].data = T();
        mt_write(occupied[i], 2);

        return table[i].data;
      }
      else if (probed == 2)
      {
        if (compare(table[i].key, key))
        {
          mt_write(occupied[i], 2);
          return table[i].data;
        }
      }

      // The entry had been grabbed by someone else.  Release the lock.
      mt_write(occupied[i], probed);
    }

    i = (i + 1) & mask;
  } while (i != index);

  // TODO:  There is an error here.  If the element doesn't exist in the hash
  //        and the hash is full, a reference can't be returned.  In this
  //        case, we just need to die for now.  This will be fixed when
  //        dynamic reallocation is added.
  assert(false);

  // This line is only here to get rid of a compiler warning.  The code should
  // never get here.
  return table[i].data;
}

/***/

template <typename K, typename T, typename HF, typename EQF>
bool xmt_hash_table<K, T, HF, EQF>::member(const K& key) const
{
  size_type index = hash(key);
  size_type i = index;

  do
  {
    if (occupied[i] == 2)
    {
      if (compare(table[i].key, key)) return true;
    }
    else if (occupied[i] == 0)
    {
      break;
    }

    i = (i + 1) & mask;
  } while (i != index);

  return false;
}

/***/

template <typename K, typename T, typename HF, typename EQF>
bool xmt_hash_table<K, T, HF, EQF>::lookup(const K& key, T& value) const
{
  size_type index = hash(key);
  size_type i = index;

  do
  {
    if (occupied[i] == 2)
    {
      if (compare(table[i].key, key))
      {
        value = table[i].data;
        return true;
      }
    }
    else if (occupied[i] == 0)
    {
      break;
    }

    i = (i + 1) & mask;
  } while (i != index);

  return false;
}

/***/

template <typename K, typename T, typename HF, typename EQF>
bool xmt_hash_table<K, T, HF, EQF>::insert(const K& key, const T& value)
{
  size_type index = hash(key);
  size_type i = index;

  do
  {
    // Entry is empty.  Add the element.
    if (occupied[i] == 0)
    {
      // Acquire the lock.
      long probed = mt_readfe(occupied[i]);

      // Make sure the entry is still empty.
      if (probed == 0)
      {
        // Add the element at the current position in table.
        table[i].key = key;
        table[i].data = value;
        mt_write(occupied[i], 2);

        return true;
      }
      else
      {
        // If this entry is occupied and we match the key, exit without
        // inserting.
        if (probed == 2 && compare(table[i].key, key))
        {
          mt_write(occupied[i], probed);
          return false;
        }
      }

      // The entry had been grabbed by someone else.  Release the lock.
      mt_write(occupied[i], probed);
    }
    else
    {
      // If this entry is occupied and we match the key, exit without
      // inserting.
      if (occupied[i] == 2 && compare(table[i].key, key)) return false;
    }

    i = (i + 1) & mask;
  } while (i != index);

  return false;
}

/***/

template <typename K, typename T, typename HF, typename EQF>
bool xmt_hash_table<K, T, HF, EQF>::insert_reuse(const K& key, const T& value)
{
  size_type index = hash(key);
  size_type i = index;
  size_type insert_pos = std::numeric_limits<size_type>::max();

  do
  {
    // Entry is empty.  Add the element.
    if (occupied[i] == 0)
    {
      // We found a deleted entry earlier.  Add the element at that position
      // in table.
      if (insert_pos != std::numeric_limits<size_type>::max())
      {
        // Acquire the lock.
        long probed = mt_readfe(occupied[insert_pos]);

        // Make sure the entry is still deleted.
        if (probed == 1)
        {
          // Add the element at the current position in table.
          table[insert_pos].key = key;
          table[insert_pos].data = value;
          mt_write(occupied[insert_pos], 2);

          return true;
        }
        else if (compare(table[i].key, key))
        {
          // This entry is occupied, and we matched the key.  Exit without
          // inserting.
          mt_write(occupied[insert_pos], probed);
          return false;
        }

        // We need to start searching from insert_pos again.
        i = insert_pos;

        // The entry had been grabbed by someone else.  Release the lock.
        mt_write(occupied[insert_pos], probed);
      }
      else
      {
        // Acquire the lock.
        long probed = mt_readfe(occupied[i]);

        // Make sure the entry is still empty.
        if (probed == 0)
        {
          // Add the element at the current position in table.
          table[i].key = key;
          table[i].data = value;
          mt_write(occupied[i], 2);

          return true;
        }
        else if (probed == 2)
        {
          // This entry is occupied.  If we match the key, exit without
          // inserting.
          if (compare(table[i].key, key))
          {
            mt_write(occupied[i], probed);
            return false;
          }
        }
        // We know that at this point the entry contains a deleted value.
        else if (insert_pos == std::numeric_limits<size_type>::max())
        {
          // This is the first deleted entry that has been encountered.  Keep
          // the lock here (because this is where the new element should be
          // inserted) and continue searching until either the element is found
          // or an empty entry is discovered.
          insert_pos = i;
        }

        // The entry had been grabbed by someone else.  Release the lock.
        mt_write(occupied[i], probed);
      }
    }
    else if (occupied[i] == 2)
    {
      // This entry is occupied.  If we match the key, exit without inserting.
      if (compare(table[i].key, key)) return false;
    }
    // We know that at this point the entry contains a deleted value.
    else if (insert_pos == std::numeric_limits<size_type>::max())
    {
      // This is the first deleted entry that has been encountered.  Keep
      // the lock here (because this is where the new element should be
      // inserted) and continue searching until either the element is found
      // or an empty entry is discovered.
      insert_pos = i;
    }

    i = (i + 1) & mask;
  } while (i != index);

  return false;
}

/***/

template <typename K, typename T, typename HF, typename EQF>
bool xmt_hash_table<K, T, HF, EQF>::erase(const K& key)
{
  size_type index = hash(key);
  size_type i = index;

  do
  {
    // Entry is full.
    if (occupied[i] == 2)
    {
      // Match the entry.
      if (compare(table[i].key, key))
      {
        // Acquire the lock.
        long probed = mt_readfe(occupied[i]);

        // If the entry was deleted already, we don't care.  If the entry was
        // deleted and then the same key inserted again, we don't care.  Just
        // delete it again.  However, if it was deleted and then another key
        // inserted in the same entry, we need to not delete the new key.
        if (compare(table[i].key, key))
        {
          // Set the entry to deleted and release the lock.
          mt_write(occupied[i], 1);

          return true;
        }

        // The entry had been deleted by another thread and then a different
        // key inserted by a second thread.  Release the lock.
        mt_write(occupied[i], probed);
      }
    }
    else if (occupied[i] == 0)
    {
      return false;
    }

    i = (i + 1) & mask;
  } while (i != index);

  return false;
}

/***/

template <typename K, typename T, typename HF, typename EQF>
bool xmt_hash_table<K, T, HF, EQF>::update(const K& key, const T& value)
{
  size_type index = hash(key);
  size_type i = index;

  do
  {
    if (occupied[i] == 2)
    {
      if (compare(table[i].key, key))
      {
        table[i].data = value;
        return true;
      }
    }
    else if (occupied[i] == 0)
    {
      break;
    }

    i = (i + 1) & mask;
  } while (i != index);

  return false;
}

/***/

template <typename K, typename T, typename HF, typename EQF>
template <typename Vis>
bool xmt_hash_table<K, T, HF, EQF>::update(const K& key, const T& value,
                                           Vis visitor)
{
  size_type index = hash(key);
  size_type i = index;

  do
  {
    if (occupied[i] == 2)
    {
      if (compare(table[i].key, key))
      {
        visitor(table[i].data, value);
        return true;
      }
    }
    else if (occupied[i] == 0)
    {
      break;
    }

    i = (i + 1) & mask;
  } while (i != index);

  return false;
}

/***/

template <typename K, typename T, typename HF, typename EQF>
bool xmt_hash_table<K, T, HF, EQF>::update_insert(const K& key, const T& value)
{
  size_type index = hash(key);
  size_type i = index;

  do
  {
    if (occupied[i] == 2)
    {
      if (compare(table[i].key, key))
      {
        table[i].data = value;
        return true;
      }
    }
    else if (occupied[i] == 0)
    {
      // Acquire the lock.
      long probed = mt_readfe(occupied[i]);

      // Make sure the entry is still empty.
      if (probed == 0)
      {
        // Add the element at the current position in table.
        table[i].key = key;
        table[i].data = value;
        mt_write(occupied[i], 2);

        return true;
      }
      else if (probed == 2)
      {
        if (compare(table[i].key, key))
        {
          mt_write(occupied[i], 2);
          table[i].data = value;
          return true;
        }
      }

      // The entry had been grabbed by someone else.  Release the lock.
      mt_write(occupied[i], probed);
    }

    i = (i + 1) & mask;
  } while (i != index);

  return false;
}

/***/

template <typename K, typename T, typename HF, typename EQF>
template <typename Vis>
bool xmt_hash_table<K, T, HF, EQF>::update_insert(const K& key, const T& value,
                                                  Vis visitor)
{
  size_type index = hash(key);
  size_type i = index;

  do
  {
    if (occupied[i] == 2)
    {
      if (compare(table[i].key, key))
      {
        visitor(table[i].data, value);
        return true;
      }
    }
    else if (occupied[i] == 0)
    {
      // Acquire the lock.
      long probed = mt_readfe(occupied[i]);

      // Make sure the entry is still empty.
      if (probed == 0)
      {
        // Add the element at the current position in table.
        table[i].key = key;
        table[i].data = value;
        mt_write(occupied[i], 2);

        return true;
      }
      else if (probed == 2)
      {
        if (compare(table[i].key, key))
        {
          mt_write(occupied[i], 2);
          visitor(table[i].data, value);
          return true;
        }
      }

      // The entry had been grabbed by someone else.  Release the lock.
      mt_write(occupied[i], probed);
    }

    i = (i + 1) & mask;
  } while (i != index);

  return false;
}

/***/

template <typename K, typename T, typename HF, typename EQF>
void
xmt_hash_table<K, T, HF, EQF>::swap(xmt_hash_table& rhs)
{
  size_type temp_mask = rhs.mask;
  hash_table_node* temp_table = rhs.table;
  long* temp_occupied = rhs.occupied;
  size_type temp_table_size = rhs.table_size;

  rhs.mask = mask;
  rhs.table = table;
  rhs.occupied = occupied;
  rhs.table_size = table_size;

  mask = temp_mask;
  table = temp_table;
  occupied = temp_occupied;
  table_size = temp_table_size;
}

/***/

template <typename K, typename T, typename HF, typename EQF>
void xmt_hash_table<K, T, HF, EQF>::clear()
{
  memset(occupied, 0, table_size * sizeof(long));
}

/***/

template <typename K, typename T, typename HF, typename EQF>
template <typename Vis>
void xmt_hash_table<K, T, HF, EQF>::visit(Vis& visitor)
{
  #pragma mta assert parallel
  for (size_type i = 0; i < table_size; ++i)
  {
    if (occupied[i] == 2) visitor(table[i].key, table[i].data);
  }
}

/***/

template <typename K, typename T, typename HF, typename EQF>
void xmt_hash_table<K, T, HF, EQF>::get_keys(size_type& num_keys,
                                             key_type* key_buffer)
{
  // Assumes that key_buffer is already allocated to size() or bigger.
  assert(key_buffer != 0);

  num_keys = 0;

#ifdef __MTA__
  int stream_id;
  int num_streams;

  #pragma mta use 60 streams
  #pragma mta for all streams stream_id of num_streams
  {
    int begin;
    int end;
    determine_beg_end<int>(table_size, num_streams, stream_id, begin, end);

    // First find how many keys are in the stream's region of the array.
    size_type num_local_keys = 0;

    for (int i = begin; i < end; ++i)
    {
      if (occupied[i] == 2) ++num_local_keys;
    }

    // Claim a block of indices for the keys in the stream's region.
    size_type start_index = mt_incr(num_keys, num_local_keys);

    // Insert the keys into the key buffer.
    for (int i = begin; i < end; ++i)
    {
      if (occupied[i] == 2)
      {
        key_buffer[start_index] = table[i].key;
        ++start_index;
      }
    }
  }
#else
  // Insert the keys into the key buffer.
  for (size_type i = 0; i < table_size; ++i)
  {
    if (occupied[i] == 2)
    {
      key_buffer[num_keys] = table[i].key;
      ++num_keys;
    }
  }
#endif

}

/***/

template <typename K, typename T, typename HF, typename EQF>
void xmt_hash_table<K, T, HF, EQF>::assign_contiguous_ids(T starting_value)
{
#ifdef __MTA__
  int stream_id;
  int num_streams;

  #pragma mta for all streams stream_id of num_streams
  {
    int begin;
    int end;
    determine_beg_end(table_size, num_streams, stream_id, begin, end);

    // Count the number of occupied entries in this stream's block region of
    // the hash table.
    T num_local_occupied = 0;

    for (int i = begin; i < end; ++i)
    {
      if (occupied[i] == 2) ++num_local_occupied;
    }

    // Claim a block of indices for the occupied entries in the stream's
    // region.
    T start_index = mt_incr(starting_value, num_local_occupied);

    // Assign contiguous ids to the occupied entries in this stream's region.
    for (int i = begin; i < end; ++i)
    {
      if (occupied[i] == 2)
      {
        table[i].data = start_index;
        ++start_index;
      }
    }
  }
#else
  // Assign contiguous ids to the occupied entries in this stream's region.
  for (size_type i = 0; i < table_size; ++i)
  {
    if (occupied[i] == 2)
    {
      table[i].data = starting_value;
      ++starting_value;
    }
  }
#endif
}

/***/

template <typename K, typename T, typename HF, typename EQF>
void xmt_hash_table<K, T, HF, EQF>::print() const
{
  std::cout << "Elements: " << size() << std::endl;
  for (size_type i = 0; i < table_size; ++i)
  {
    if (occupied[i] == 2)
    {
      std::cout << i << ": (" << table[i].key << ", " << table[i].data
                << ")" << std::endl;
    }
    else if (occupied[i] == 1)
    {
      std::cout << i << ": Tombstoned." << std::endl;
    }
  }
}

/***/

template <typename K, typename T, typename HF, typename EQF>
void
xmt_hash_table<K, T, HF, EQF>::deep_copy(const xmt_hash_table& rhs)
{
  table = (hash_table_node*) malloc(table_size * sizeof(hash_table_node));
  occupied = (long*) malloc(table_size * sizeof(long));

  size_type ts = table_size;

  #pragma mta assert nodep
  for (size_type i = 0; i < ts; ++i)
  {
    occupied[i] = rhs.occupied[i];
  }

  #pragma mta assert nodep
  for (size_type i = 0; i < ts; ++i)
  {
    if (occupied[i]) table[i] = rhs.table[i];
  }
}

}

#endif

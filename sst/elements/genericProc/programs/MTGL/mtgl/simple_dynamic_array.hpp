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
/*! \file simple_dynamic_array.hpp

    \brief Dynamically-allocated arrays that support quick additions.

    \author Eric Goodman (elgoodm@sandia.gov)

    \date 5/22/2009

    \warning GEM: This implementation is NOT thread-safe.  There is no way
                  to get around locking the array during every operation if
                  you allow automatic resizing.  At some point, an update will
                  access the array pointer when it is being reallocated, and
                  you will have an error.

    \deprecated This file needs to be removed as it is not thread-safe.  The
                only reason it is still here is because diffraction_array.hpp
                depends on it.  This class should NOT be used under any
                circumstances as it doesn't work.  Use dynamic_array.hpp
                instead.

    The original dynamic array class wasn't suiting performance objectives,
    so I created this class as an intended base class with stripped down
    functionality.  Additions are done quickly by only locking during
    reallocation.
*/
/****************************************************************************/

#ifndef MTGL_SIMPLE_DYNAMIC_ARRAY_HPP
#define MTGL_SIMPLE_DYANMIC_ARRAY_HPP

#include <mtgl/util.hpp>

#define EXPANSION_FACTOR 2.0
#define INIT_SIZE 100

namespace mtgl {

template <typename T>
class simple_dynamic_array {
public:
  simple_dynamic_array(int initsz = INIT_SIZE);
  ~simple_dynamic_array();
  int add(const T& key);
  bool insert(const T& key, unsigned int pos);
  int get_alloc_size();
  int get_num_elements();
  T& operator[](int i);

private:
  int allocLock;                      // Used as lock during reallocation.
  T* _store;                          // The values stored in this array.
  int num_elements;                   // The number of elements in the array.
  int alloc_size;                     // The allocated size of the array.
  bool reallocate(unsigned int pos);  // If pos > alloc_size, reallocate.
};

template <typename T>
simple_dynamic_array<T>::simple_dynamic_array(int initsz)
{
  printf("initsize %d\n", initsz);
  alloc_size = initsz;
  _store = (T*) malloc(alloc_size * sizeof(T));
  num_elements = 0;
  allocLock = 0;
}

template <typename T>
simple_dynamic_array<T>::~simple_dynamic_array()
{
  free(_store);
}

template <typename T>
int simple_dynamic_array<T>::add(const T& key)
{
  int mypos = mt_incr(num_elements, 1);
  reallocate(mypos);
  _store[mypos] = key;
  return mypos;
}

/// Returns whether or not the array was resize
template <typename T>
bool simple_dynamic_array<T>::insert(const T& key, unsigned int pos)
{
  bool resized = reallocate(pos);
  _store[pos] = key;
  return resized;
}

/// If pos >= alloc_size, we do a resize.  Returns whether or not the array
/// was resized.
#pragma mta inline
template <typename T>
bool simple_dynamic_array<T>::reallocate(unsigned int pos)
{
  bool resized = false;
  if (pos >= alloc_size)
  {
    readfe(&allocLock);
    int origAllocSize = alloc_size;
    T* tmpStore = _store;
    T* new_a = tmpStore;
    if (pos >= alloc_size)
    {
      printf("reallocating\n");
      resized = true;
      do {
        alloc_size = int(EXPANSION_FACTOR * alloc_size);
      } while (pos >= alloc_size);

      new_a = (T*)  malloc(alloc_size * sizeof(T));

      #pragma mta assert parallel
      for (int i = 0; i < origAllocSize; i++)
      {
        new_a[i] = _store[i];
      }
      free(tmpStore);
    }
    _store = new_a;
    writeef(&allocLock, 0);
  }
  readff(&allocLock);
  return resized;
}

template <typename T>
int simple_dynamic_array<T>::get_alloc_size()
{
  return alloc_size;
}

template <typename T>
int simple_dynamic_array<T>::get_num_elements()
{
  return num_elements;
}

template <class T>
T & simple_dynamic_array<T>::operator[](int i)
{
  #ifdef CHECK_BOUNDS
  if (i < 0 || i > num_elements)
  {
    cerr << "error: index " << i << " out of bounds" << endl;
    exit(1);              // exceptions on MTA?
  }
  #endif
  return _store[i];
}

}

#endif

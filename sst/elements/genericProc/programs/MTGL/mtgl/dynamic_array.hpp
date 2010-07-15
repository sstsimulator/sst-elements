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
/*! \file dynamic_array.hpp

    \brief Simple implementation of an array that automatically grows as
           necessary when inserting items.

    \author Greg Mackey (gemacke@sandia.gov)
    \author Jon Berry (jberry@sandia.gov)

    \date 12/4/2007
*/
/****************************************************************************/

#ifndef MTGL_DYNAMIC_ARRAY_HPP
#define MTGL_DYNAMIC_ARRAY_HPP

#include <mtgl/util.hpp>

#define EXPANSION_FACTOR 2.0
#define INIT_SIZE 0

namespace mtgl {

/****************************************************************************/
/*! \brief Simple implementation of an array that automatically grows as
           necessary when inserting items.

    \author Greg Mackey (gemacke@sandia.gov)
    \author Jon Berry (jberry@sandia.gov)

    \tparam Element type.

    There are two main classes of functions: those designed to be called in a
    parallel context and those designed to be called in a serial context.
    Here is the division of functions:

      - Parallel Context Functions
         - operator[]()
         - push_back()
         - unsafe_push_back()
         - pop_back()

      - Serial Context Functions
         - Constructors
         - Destructor
         - operator=()
         - size()
         - resize()
         - capacity()
         - empty()
         - reserve()
         - get_data()
         - swap()
         - clear()
         - print()

    Although the dynamic_array offers on-the-fly resizing, you will not get
    any parallelization if you use this feature.  Because of the locking
    necessary to ensure thread-safety, the push_back() function that adds a
    single element to the back of the array will be mostly serialized when
    called in a loop.

    There are three methods for using the resize capability of the
    dynamic_array and retaining good parallelism.  The first method is to call
    the version of push_back() that adds an array of elements to the back of
    the array.  This method is as scalable as if you used a C-style array.
    The second method is to count the number of items you need to put in the
    array (if you don't already have the count), resize the array, and add the
    items to the array using the [] operator.  Although this method requires
    the extra work of counting the number of elements to add if you don't
    already know the count, it is also as scalable as if you used a C-style
    array.  It also avoids having to explicitly create an array of the items
    to add if you don't already have one.  The third method is the same as the
    second except that you use the unsafe_push_back() function, which performs
    no resizing, to add the elements.  This method is slightly less scalable,
    as there is a hotspot on the individual increments of the number of
    elements.  However, it is useful if you are updating many dynamic_arrays
    using a single loop.  An example of this is updating the edge lists of
    vertices when adding edges to a graph.

    In most cases the operator[] compiles away to nothing and the compiler
    performs optimizations as if the dyanmic_array were a simple C-style
    array.  Using the operator[] is the preferred method.  However, we have
    seen instances where the compiler performs better optimizations when
    accessing the C-style array directly.  To allow for this, we include the
    get_data() function, which returns a pointer to the underlying array
    representing the data.  Note that this pointer will not be valid if the
    dynamic array is resized.

    The default for declaring a dynamic_array is to declare the data array
    with no elements allocated.  When a dynamic_array automatically resizes
    itself, it doubles the size of its data array.  These implementation
    choices make the dynamic_array behave like most implementations of
    stl::vector.  One difference in resizing from stl::vector is that
    reserve() and resize(), when it changes the size of the data array, set
    the new size of the data array to exactly the size specified by the user.
    This conserves memory by not allocating up to the next smallest power of
    2.
*/
/****************************************************************************/
template <typename T>
class dynamic_array {
public:
  typedef T value_type;
  typedef unsigned long size_type;

  /// \brief the normal constructor.
  /// \param size The initial allocation size.
  inline dynamic_array(size_type size = INIT_SIZE);

  /// \brief The copy constructor, which performs a deep copy.
  /// \param a The dynamic_array to be copied.
  inline dynamic_array(const dynamic_array& a);

  /// The destructor.
  ~dynamic_array() { free(data); }

  /// \brief The assignment operator, which performs a deep copy.
  /// \param rhs The dynamic_array to be copied.
  inline dynamic_array& operator=(const dynamic_array& rhs);

  /// Returns the number of elements.
  size_type size() const { return num_elements; }

  /// \brief Changes the number of elements in the array.
  /// \param new_size The new number of elements for the array.
  ///
  /// If the number of elements is increased, the new elements are NOT
  /// initialized.  If the new number of elements is greater than the
  /// allocation size, the array is automatically grown.
  ///
  /// If the number of elements is decreased, the array is not shrunk.
  inline void resize(size_type new_size);

  /// Returns the current allocation size of the array.
  size_type capacity() const { return alloc_size; }

  /// Returns true if there are no elements in the array.
  bool empty() const { return num_elements == 0; }

  /// \brief Changes the allocation size of the array.
  /// \param new_size The new allocation size of the array.
  ///
  /// This function will only increase, and never decrease, the allocation
  /// size.  Note that the number of elements is not changed.
  inline void reserve(size_type new_size);

  /// \brief Element accessor that returns a mutable reference.
  /// \param i The index.  Bounds checking is controlled by the
  ///          CHECK_BOUNDS #define.
  inline T& operator[](size_type i);

  /// \brief Element accessor that returns a constant reference.
  /// \param i The index.  Bounds checking is controlled by the
  ///          CHECK_BOUNDS #define.
  inline const T& operator[](size_type i) const;

  /// Returns a pointer to the data array.
  T* get_data() const { return data; }

  /// \brief Adds an element to the end of the data array.
  /// \param key The element to add.
  ///
  /// Returns the index of the newly added element.  Note that this function
  /// effectively executes in serial because of the locking.
  inline size_type push_back(const T& key);

  /// \brief Adds an array of elements to the end of the data array.
  /// \param a The elements to add.
  /// \param size The number of elements to add.
  ///
  /// Returns the index of the last element added.
  inline size_type push_back(T* a, size_type size);

  /// \brief Adds an element to the end of the data array without checking
  ///        if the array is big enough.
  /// \param key The element to add.
  ///
  /// Returns the index of the newly added element.  This function will run
  /// fine in parallel, but it doesn't check if the array needs to be resized.
  /// Use this only when you are certain the array is already large enough.
  inline size_type unsafe_push_back(const T& key);

  /// \brief Removes elements from the end of the data array.
  /// \param size The number of elements to remove.
  void pop_back(size_type size = 1) { mt_incr(num_elements, -size); }

  /// Swaps the contents of the two arrays.
  inline void swap(dynamic_array& rhs);

  /// \brief Removes all the entries from the data array.
  void clear() { num_elements = 0; }

  /// Prints the entries in the array.
  inline void print() const;

private:
  /// \brief Resizes the data array.
  /// \param new_size The new size of the array.
  ///
  /// Allocates new memory for the data array, copies over the elements, and
  /// deletes the old memory.
  inline void internal_resize(size_type new_size);

private:
  size_type num_elements;
  size_type alloc_size;
  T* data;
};

template <typename T>
dynamic_array<T>::dynamic_array(size_type size) :
  num_elements(0), alloc_size(size)
{
  data = (T*) malloc(alloc_size * sizeof(T));
}

template <typename T>
dynamic_array<T>::dynamic_array(const dynamic_array& a) :
  num_elements(a.num_elements), alloc_size(a.alloc_size)
{
  data = (T*) malloc(alloc_size * sizeof(T));

  for (size_type i = 0; i < num_elements; ++i) data[i] = a[i];
}

template <typename T>
dynamic_array<T>&
dynamic_array<T>::operator=(const dynamic_array& rhs)
{
  free(data);

  num_elements = rhs.num_elements;
  alloc_size = rhs.alloc_size;

  data = (T*) malloc(alloc_size * sizeof(T));

  for (size_type i = 0; i < num_elements; ++i) data[i] = rhs[i];

  return *this;
}

template<typename T>
void dynamic_array<T>::resize(size_type new_size)
{
  if (new_size > alloc_size) internal_resize(new_size);

  num_elements = new_size;
}

template<typename T>
void dynamic_array<T>::reserve(size_type new_size)
{
  if (new_size > alloc_size) internal_resize(new_size);
}

template <typename T>
T& dynamic_array<T>::operator[](size_type i)
{
#ifdef CHECK_BOUNDS
  if (i < 0 || i > num_elements)
  {
    cerr << "error: index " << i << " out of bounds" << endl;
    exit(1);
  }
#endif

  return data[i];
}

template <typename T>
const T& dynamic_array<T>::operator[](size_type i) const
{
#ifdef CHECK_BOUNDS
  if (i < 0 || i > num_elements)
  {
    cerr << "error: index " << i << " out of bounds" << endl;
    exit(1);
  }
#endif

  return data[i];
}

template <typename T>
typename dynamic_array<T>::size_type
dynamic_array<T>::push_back(const T& key)
{
  size_type current_size = mt_readfe(num_elements);

  if (current_size == alloc_size)
  {
    internal_resize(alloc_size == 0 ? 1 :
                    static_cast<size_type>(EXPANSION_FACTOR * alloc_size));
  }

  data[current_size] = key;

  mt_write(num_elements, current_size + 1);

  return current_size;
}

template <typename T>
typename dynamic_array<T>::size_type
dynamic_array<T>::push_back(T* keys, size_type num_to_add)
{
  size_type current_size = mt_readfe(num_elements);
  size_type resulting_size = current_size + num_to_add;

  if (resulting_size >= alloc_size)
  {
    size_type new_alloc_size = alloc_size == 0 ? 1 :
                               static_cast<size_type>(EXPANSION_FACTOR *
                                                      alloc_size);

    if (new_alloc_size < resulting_size)
    {
      new_alloc_size = static_cast<size_type>(EXPANSION_FACTOR *
                                              resulting_size);
    }

    internal_resize(new_alloc_size);
  }

  #pragma mta assert nodep
  #pragma mta block schedule
  for (size_type i = 0; i < num_to_add; ++i) data[current_size + i] = keys[i];

  mt_write(num_elements, resulting_size);

  return resulting_size - 1;
}

template <typename T>
typename dynamic_array<T>::size_type
dynamic_array<T>::unsafe_push_back(const T& key)
{
  size_type current_size = mt_incr(num_elements, 1);

  data[current_size] = key;

  return current_size;
}

template <typename T>
void dynamic_array<T>::swap(dynamic_array& rhs)
{
  size_type temp_num_elements = rhs.num_elements;
  size_type temp_alloc_size = rhs.alloc_size;
  T* temp_data = rhs.data;

  rhs.num_elements = num_elements;
  rhs.alloc_size = alloc_size;
  rhs.data = data;

  num_elements = temp_num_elements;
  alloc_size = temp_alloc_size;
  data = temp_data;
}

template <typename T>
void dynamic_array<T>::print() const
{
  for (int i = 0; i < num_elements; ++i)
  {
    printf("%d ", data[i]);
  }

  printf("\n");
}

template<typename T>
void dynamic_array<T>::internal_resize(size_type new_size)
{
  alloc_size = new_size;
  T* new_data = (T*) malloc (alloc_size * sizeof(T));

  size_type ne = num_elements;
  #pragma mta assert nodep
  #pragma mta block schedule
  for (size_type i = 0; i < ne; ++i) new_data[i] = data[i];

  free(data);
  data = new_data;
}

}

#endif

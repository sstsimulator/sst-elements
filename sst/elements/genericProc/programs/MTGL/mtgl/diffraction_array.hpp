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
/*! \file diffraction_array.hpp

    \brief A class that logically acts as one array but has multiple physical
           arrays as its implementation.

    \author Eric Goodman (elgoodm@sandia.gov)

    \date 6/30/2009
*/
/****************************************************************************/

#ifndef MTGL_DIFFRACTION_ARRAY_HPP
#define MTGL_DIFFRACTION_ARRAY_HPP

#include <cmath>

#include <mtgl/simple_dynamic_array.hpp>
#include <mtgl/random_number_generator.hpp>

namespace mtgl {

template <typename T>
class diffraction_array {
public:
  diffraction_array(int inarrays = 1, int initsz = 4)
  {
    rng1 = new random_number_generator<int>();

    num_arrays = inarrays;

    // Make sure num_arrays is a power of two.
    double log_arrays = log(num_arrays) / log(2);
    if (log_arrays - floor(log_arrays) > 0)
    {
      num_arrays = pow(2, floor(log_arrays) + 1);
    }
    printf("number of arrays %d\n", inarrays);

    // Set the bit mask so that is all ones up to log(num_array) bits.
    bit_mask = num_arrays - 1;

    arrays = new simple_dynamic_array<T>*[num_arrays];
    for (int i = 0; i < num_arrays; i++)
    {
      arrays[i] = new simple_dynamic_array<T>(initsz);
    }
  }

  ~diffraction_array()
  {
    if (arrays != NULL)
    {
      for (int i = 0; i < num_arrays; i++)
      {
        if (arrays[i] != NULL) delete arrays[i];
      }

      delete arrays;
    }
  }

  simple_dynamic_array<T>* getArray(int i) { return arrays[i]; }

  // The last two parameters allow the caller to know into which array and
  // what position the value was inserted.
  void add(T value, int rand_val, unsigned int& index, unsigned int& pos)
  {
    index = MTA_BIT_AND(rand_val, bit_mask);
    pos = arrays[index]->add(value);
  }

  void add(T value, unsigned int& index, unsigned int& pos)
  {
    int rand_val = rng1->get_random_number();
    add(value, rand_val, index, pos);
  }

  // Allows you to insert a value into a particular array at a given position.
  void insert(T value, unsigned int index, unsigned int pos)
  {
    arrays[index]->insert(value, pos);
  }

  int size()
  {
    int sum = 0;

    for (int i = 0; i < num_arrays; i++) sum += arrays[i]->get_num_elements();

    return sum;
  }

  T* get_single_array(int& total_size)
  {
    total_size = size();
    T* r_array = (T*) malloc (sizeof(T) * total_size);
    int start_index = 0;

    for (int i = 0; i < num_arrays; i++)
    {
      int current_array_size = arrays[i]->get_num_elements();

      #pragma mta assert parallel
      for (int j = 0; j < current_array_size; j++)
      {
        r_array[start_index + j] = (*arrays[i])[j];
      }

      start_index = start_index + current_array_size;
    }

    return r_array;
  }

  T operator[](int i)
  {
    int index1 = -1;
    int index2 = -1;
    int total = 0;

    do
    {
      index1++;
      total += arrays[index1]->getNumElements();
    } while (total <= i);

    index2 = i - (total - arrays[index1]->getNumElements());

    return (*arrays[index1])[index2];
  }

  unsigned int get_num_arrays() { return num_arrays; }

private:
  // The total number of arrays.
  unsigned int num_arrays;

  // The set of dynamic arrays.
  simple_dynamic_array<T>** arrays;

  // A bit mask to help with mods during inserts.
  unsigned int bit_mask;

  // Creating the random number generator.
  random_number_generator<int>* rng1;
};

}

#endif

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
/*! \file random_number_generator.hpp

    \brief A class to simplify getting random numbers.  Templatized to
           support ints and unsigned ints.

    \author Eric Goodman (elgoodm@sandia.gov)

    \date 6/25/2009
*/
/****************************************************************************/

#ifndef MTGL_RANDOM_NUMBER_GENERATOR_HPP
#define MTGL_RANDOM_NUMBER_GENERATOR_HPP

#include <cstdlib>
#include <cstdio>
#include <cmath>

#include <mta_rng.h>
#include <machine/mtaops.h>

namespace mtgl {

template <typename T>
class random_number_generator {
public:
  random_number_generator(int insize = pow(2, 20));
  T get_random_number();

private:
  unsigned int size;      // The size of the random pool.
  unsigned int mask;
  T* randVals;
  int* currentRandIndex;  // Holds the current index into the random array
                          // for streams.
  int* domainMapping;

  void initialize(int insize);
};

template <typename T>
random_number_generator<T>::random_number_generator(int insize) {}

template <>
random_number_generator<int>::random_number_generator(int insize)
{
  initialize(insize);
  randVals = (int*) malloc(size * sizeof(int));
  prand_int(size, randVals);
}

template <>
random_number_generator<unsigned int>::random_number_generator(int insize)
{
  initialize(insize);
  randVals = (unsigned int*) malloc(size * sizeof(unsigned int));
  prand_int(size, (int*) randVals);
}

template <typename T>
void random_number_generator<T>::initialize(int insize)
{
  size = insize;

  double logSize = log(size) / log(2);

  // Making size a power of two.
  if (logSize - floor(logSize) > 0) size = pow(2, floor(logSize) + 1);

  mask = size - 1;
}

template <typename T>
T random_number_generator<T>::get_random_number()
{
  unsigned int index = MTA_DOMAIN_IDENTIFIER_SAVE() +
                       MTA_STREAM_IDENTIFIER_SAVE();

  unsigned int clockTime = MTA_CLOCK(index);
  unsigned int randIndex = index * clockTime & mask;

  T randNum = randVals[randIndex];

  return randNum;
}

}

#endif

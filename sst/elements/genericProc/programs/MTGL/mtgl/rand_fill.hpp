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
/*! \file rand_fill.hpp

    \brief This file contains functions to fill arrays with random
           numbers.  On the MTA-2/XMT, this uses the native prand library.
           On linux, it uses drand48. On winders using rand_s(uint *)

    \author Jon Berry (jberry@sandia.gov)

    \date 12/4/2007
*/
/****************************************************************************/

#ifndef MTGL_RAND_FILL_HPP
#define MTGL_RAND_FILL_HPP

#include <cassert>

#ifdef HAVE_CONFIG_H
  #include <mtgl/mtgl_config.h>
#elif defined(WIN32)
  #define HAVE_RAND_S
#else
  #define HAVE_LRAND48 1
#endif

#include <mtgl/util.hpp>

namespace mtgl {

#ifdef __MTA__
  #include <mta_rng.h>
  typedef unsigned __short32 random_value;
#else
  #include <climits>
  typedef unsigned int random_value;
#endif

class rand_fill {
public:
  template <class T>
  static void generate(T n, random_value* v)
  {
    assert(v != 0);

#ifdef __MTA__
    prand_short(n, (short*) v);
#else
    for (T i = 0; i < n; ++i)           // Improve for qthread case.
    {
#ifdef HAVE_RAND_S
      unsigned int rand_num;
      rand_s(&rand_num);
      v[i] = rand_num;
#elif HAVE_LRAND48
      v[i] = static_cast<unsigned int>(lrand48());
#else
      #error No acceptable random function!
#endif
    }
#endif
  }

#ifdef __MTA__
  template <class T>
  static void generate(T n, unsigned int* v)
  {
    assert(v != 0);

    prand_int(n, (int*) v);
  }
#endif

  template <class T>
  static void generate(T n, double* v)
  {
    assert(v != 0);

#ifdef __MTA__
    prand(n, v);
#else
    for (T i = 0; i < n; ++i)           // Improve for qthread case.
    {
#ifdef HAVE_RAND_S
      unsigned int rand_num;
      rand_s(&rand_num);
      v[i] = (double) rand_num / (double) UINT_MAX;
#elif HAVE_LRAND48
      v[i] = drand48();
#else
      #error No acceptable random function!
#endif
    }
#endif
  }

  static random_value get_max(void)
  {
#ifdef __MTA__
#ifndef __SHORT_WIDTH_16__
    return UINT32_MAX;
#else
    return USHRT_MAX;
#endif
#else
# if HAVE_RAND_S
    return RAND_MAX;
# elif HAVE_LRAND48
    return ((1u<<31u)-1u);
# endif
#endif
  }
};

/*! \brief Takes an array and permutes the elements
    \param n The size of the array
    \param array The array to be permuted.
 */
template <typename T, typename size_type>
void random_permutation(size_type n, T* array)
{
  random_value* randVals = (random_value*) malloc(n * sizeof(random_value));
  rand_fill::generate(n, randVals);

#ifdef __MTA__
  if (n > 10000)
  {
    #pragma mta assert parallel
    for (size_type i = 0; i < n; ++i)
    {
      size_type j = randVals[i] % n;
      if (i != j)
      {
        T x = mt_readfe(array[i]);
        T y = mt_readfe(array[j]);
        mt_write(array[i], y);
        mt_write(array[j], x);
      }
    }
  }
  else
  {
    for (size_type i = 0; i < n; ++i)
    {
      size_type j = randVals[i] % n;
      if (i != j)
      {
        T tmp = array[i];
        array[i] = array[j];
        array[j] = tmp;
      }
    }
  }
#else
  for (size_type i = 0; i < n; ++i)
  {
    size_type j = randVals[i] % ((unsigned int) n);
    if (i != j)
    {
      T tmp = array[i];
      array[i] = array[j];
      array[j] = tmp;
    }
  }
#endif

  free(randVals);
}


// random_permutation: Adapted from Kamesh Madduri's code for DIMACS shortest
//                     path challenge.
template <typename T, typename size_type>
void random_permutation(size_type n, size_type m, T* srcs, T* dests)
{
  random_value* randVals = (random_value*) malloc(n * sizeof(random_value));
  rand_fill::generate(n, randVals);

  T* perm = (T*) malloc(n * sizeof(T));

  for (size_type i = 0; i < n; ++i) perm[i] = i;

  #pragma mta assert parallel
  for (size_type i = 0; i < n; ++i)
  {
#ifdef __MTA__
    size_type j = randVals[i] % n;
#else
    size_type j = randVals[i] % ((unsigned int) n);
#endif

    if (i == j) continue;

    /* Swap perm[i] and perm[j] */
#ifdef __MTA__
    T x = mt_readfe(perm[i]);
    T y = mt_readfe(perm[j]);
    mt_write(perm[i], y);
    mt_write(perm[j], x);
#else
    T tmp = perm[i];
    perm[i] = perm[j];
    perm[j] = tmp;
#endif
  }

  #pragma mta assert nodep
  for (size_type i = 0; i < m; ++i)
  {
    srcs[i] = perm[srcs[i]];
    dests[i] = perm[dests[i]];
  }

  free(perm);
  free(randVals);
}

}

#endif

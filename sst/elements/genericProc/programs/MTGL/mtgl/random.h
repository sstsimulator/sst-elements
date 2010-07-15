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
/*! \file random.h

    \author Jon Berry (jberry@sandia.gov)

    \date 12/4/2007
*/
/****************************************************************************/

#ifndef MTGL_RANDOM_H
#define MTGL_RANDOM_H

#include <cstdlib>

#include <mtgl/util.hpp>
#include <mtgl/rand_fill.hpp>

namespace mtgl {

class random {
public:
  random(long int sd = 0, int sz = 5000) : seed(sd), size(sz)
  {
    // No seed for prand?
    _store = (random_value*) malloc(size * sizeof(random_value));
    rand_fill::generate(size, _store);
    current = 0;
    refills = 0;
    numTaken = 0;
  }

  /// \brief Return the rank'th random value in a sequence of sequences of
  ///        random values.
  random_value nthValue(int rank)  // Rank is which number in the sequence
  {                                // you want.
    random_value result;
    bool my_values_ready = false;
    int index = rank % size;
    int turn  = rank / size;

    while (!my_values_ready)
    {
      int nrefills = mt_readff(refills);
      if (nrefills == turn) my_values_ready = true;
    }

    result = _store[index];
    int nt = mt_incr(numTaken, 1);

    if (nt == size - 1)
    {
      rand_fill::generate(size, _store);  // Really need to seed!
      mt_incr(numTaken, -numTaken);
      mt_incr(refills, 1);
    }

    return result;
  }

  ~random() { free(_store); }

private:
  long int seed;
  int size;
  int current;
  int numTaken;
  random_value* _store;
  int refills;
};

}

#endif

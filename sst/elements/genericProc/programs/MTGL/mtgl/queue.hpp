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
/*! \file queue.hpp

    \author Jon Berry (jberry@sandia.gov)
    \author Brad Mancke

    \date 12/4/2007
*/
/****************************************************************************/

#ifndef MTGL_QUEUE_HPP
#define MTGL_QUEUE_HPP

namespace mtgl {

template <class T>
class queue {
public:
  queue(int size)
  {
    Q = (T*) malloc(size * sizeof(T));

    #pragma mta assert nodep
    for (int i = 0; i < size; ++i) Q[i] = (T) 0;
  }

  queue(const queue& q) : Q(q.Q) {}
  ~queue() { clear(); }

  void push(T val, int index) { Q[index] = val; }
  T elem(int index) { return Q[index]; }

  void clear()
  {
    if (Q)
    {
      free(Q);
      Q = NULL;
    }
  }

public:
  T* Q;
};

}

#endif

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
/*! \file test_random_number_generator.cpp

    \author Eric Goodman (elgoodm@sandia.gov)

    \date 7/1/2009
*/
/****************************************************************************/

#include <cstdlib>
#include <cstdio>

#include <mtgl/random_number_generator.hpp>
#include <mtgl/util.hpp>

using namespace mtgl;

int main (int argc, const char* argv[])
{
 // The first argument is the log_2 of the number of ints to create.
 // The second argument is the number of bins.

  int powNum = atoi(argv[1]);
  int numBins = atoi(argv[2]);
  int numInts = pow(2, powNum);
  int mask = numBins - 1;

  printf("numBins %d, numInts %d\n", numBins, numInts);

  int* count = (int*) malloc(sizeof(int) * numBins);

  mt_timer timer;

  #pragma mta fence
  timer.start();

  random_number_generator<unsigned int> rng1;

  #pragma mta fence
  timer.stop();

  printf("time for initialization %f\n", timer.getElapsedSeconds());

  for (int i = 0; i < numBins; i++)
  {
    count[i] = 0;
    printf("count[%d] %d\n", i, count[i]);
  }

  #pragma mta fence
  timer.start();

  #pragma mta use 80 streams
  #pragma mta assert parallel
  for (int i = 0; i < numInts; ++i)
  {
    unsigned int randomNum = rng1.get_random_number();
//    printf("%u\n", randomNum);
    unsigned int index = randomNum & mask;
    int result = int_fetch_add(&count[index], 1);
  }

  #pragma mta fence
  timer.stop();
  printf("time for adding values %f\n", timer.getElapsedSeconds());

  for (int i = 0; i < numBins; i++) printf("%d\n", count[i]);
}

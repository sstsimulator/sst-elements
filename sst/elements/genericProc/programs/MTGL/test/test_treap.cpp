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
/*! \file test_treap.cpp

    \brief Driver code to test the implemented Treap Set operations.

    \author Kamesh Madduri

    \date 10/20/2009
*/
/****************************************************************************/

#include <iostream>
#include <cstdio>
#include <climits>

#include <mtgl/treap.hpp>

#ifdef __MTA__
#include <sys/mta_task.h>
#include <machine/runtime.h>

double timer() { return((double) mta_get_clock(0) / mta_clock_freq()); }
#endif

using namespace std;
using namespace mtgl;

int main(int argc, char* argv[])
{
  double t;

  if (argc < 2)
  {
    cout << "Usage: executable <n>" << endl;
    cout << "Test Set size - 2^n elements" << endl << endl;
    exit(1);
  }

#ifdef __MTA__
  int procs = mta_nprocs();
  double freq = mta_clock_freq();
  int mhz = freq / 1000000;
  fprintf(stderr, "%d processors, %d MHz clock\n\n", procs, mhz);
  int ticks = mta_get_clock(0);
#endif

#ifdef __MTA__
  t = timer();
#endif

  int n = atoi(argv[1]);

  int numElems = 1 << n;

  cout << endl << " Num Elements "  << numElems << endl;

  Treap<int> t3, t4, t5;
  Treap<int> test;
  int* keyArr1 = new int[numElems], * keyArr2 = new int[numElems];

#ifdef __MTA__
  #pragma mta assert parallel
#endif
  for (int i = 0; i < numElems; i++)
  {
    keyArr1[i] = i;
    keyArr2[i] = 2 * i; // lrand48()
  }

  // Treap Creation

#ifdef __MTA__
  t = timer();
#endif

  Treap<int> t1(keyArr1, numElems);

#ifdef __MTA__
  t = timer() - t;
  cout << "Treap Creation - " << t << " seconds" << endl;
#endif

#ifdef __MTA__
  t = timer();
#endif

  for (int i = 0; i < 100; i++) test = test >> i;

#ifdef __MTA__
  t = timer() - t;
  cout << "Treap Insertion - " << t / 100 << " seconds" << endl;
#endif

#ifdef __MTA__
  t = timer();
#endif

  for (int i = 0; i < 100; i++) test = test << i;

#ifdef __MTA__
  t = timer() - t;
  cout << "Treap Deletion - " << t / 100 << " seconds" << endl;
#endif

  // Union

  Treap<int> t2(keyArr2, numElems);

#ifdef __MTA__
  t = timer();
#endif

  t3 = t1 | t2;

#ifdef __MTA__
  t = timer() - t;
  cout << "Set Union - " << t << " seconds" << endl;
#endif

  // Intersection

#ifdef __MTA__
  t = timer();
#endif

  t4 = t1 & t2;

#ifdef __MTA__
  t = timer() - t;
  cout << "Set Intersection - " << t << " seconds" << endl;
#endif

  // Difference

#ifdef __MTA__
  t = timer();
#endif

  t5 = t1 - t2;

#ifdef __MTA__
  t = timer() - t;
  cout << "Set Difference - " << t << " seconds" << endl;
#endif

  delete (keyArr1);
  delete (keyArr2);
}

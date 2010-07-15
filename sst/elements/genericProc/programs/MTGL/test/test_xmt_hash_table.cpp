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
/*! \file test_xmt_hash_table.cpp

    \author Greg Mackey (gemacke@sandia.gov)

    \brief Tests the functionality of the xmt_hash_table.

    \date 3/5/2010
*/
/****************************************************************************/

#include <mtgl/xmt_hash_table.hpp>
#include <mtgl/util.hpp>

using namespace mtgl;

typedef xmt_hash_table<int, int> hash_table_type;

#define MAX_LOOP 100000000
#define OPEN_ADDRESS_HASH_SIZE 200000000

class table_visitor {
public:
  void operator()(const int& k, int& v) { v += k; }
};

int main()
{
  int data;
  bool truth;
  table_visitor tv1;
  hash_mt_incr<int> uv1;

  mt_timer timer;

  #pragma mta fence
  timer.start();
  hash_table_type xht(OPEN_ADDRESS_HASH_SIZE);
  #pragma mta fence
  timer.stop();

  printf("                 MAX_LOOP: %9lu\n", MAX_LOOP);
  printf("               capacity(): %9lu\n\n", xht.capacity());

  printf("           Initialization: %9.6lf\n", timer.getElapsedSeconds());

  #pragma mta fence
  timer.start();
  #pragma mta assert parallel
  #pragma mta block schedule
  for (int i = 0; i < MAX_LOOP; ++i)
  {
//    if (!xht.insert(i, i))  printf("Couldn't insert %d.\n", i);
    xht.insert(i, i);
  }
  #pragma mta fence
  timer.stop();

  printf("                Insertion: %9.6lf\n", timer.getElapsedSeconds());

  #pragma mta fence
  timer.start();
  hash_table_type xht2(xht);
  #pragma mta fence
  timer.stop();

  printf("         Copy constructor: %9.6lf\n", timer.getElapsedSeconds());

  #pragma mta fence
  timer.start();
  xht2 = xht;
  #pragma mta fence
  timer.stop();

  printf("      Assignment operator: %9.6lf\n", timer.getElapsedSeconds());

//  if (xht2.lookup(20, data))  printf("Found: 20 -> %d\n", data);
//  else  printf("Value 20 not found.\n");

  #pragma mta fence
  timer.start();
  xht2.visit(tv1);
  #pragma mta fence
  timer.stop();

//  if (xht2.lookup(20, data))  printf("Found: 20 -> %d\n", data);
//  else  printf("Value 20 not found.\n");

  printf("                    Visit: %9.6lf\n", timer.getElapsedSeconds());

  #pragma mta fence
  timer.start();
  xht2.clear();
  #pragma mta fence
  timer.stop();

  printf("                    Clear: %9.6lf\n", timer.getElapsedSeconds());

  #pragma mta fence
  timer.start();
  xht2.resize(10000);
  #pragma mta fence
  timer.stop();

  printf("             Empty Resize: %9.6lf\n", timer.getElapsedSeconds());

  #pragma mta fence
  timer.start();
  #pragma mta block schedule
  for (int i = 0; i < MAX_LOOP; ++i)
  {
//    if (!xht.member(i))  printf("Didn't find %d.\n", i);
    truth = xht.member(i);
  }
  #pragma mta fence
  timer.stop();

  printf("          Membership test: %9.6lf        %d\n",
         timer.getElapsedSeconds(), truth);

  #pragma mta fence
  timer.start();
  #pragma mta block schedule
  for (int i = 0; i < MAX_LOOP; ++i)
  {
//    if (!xht.lookup(i, data))  printf("Didn't find %d.\n", i);
    data = xht.lookup(i, data);
  }
  #pragma mta fence
  timer.stop();

  printf("                   Lookup: %9.6lf        %d\n",
         timer.getElapsedSeconds(), data);

#if 0
  #pragma mta fence
  timer.start();
  #pragma mta assert parallel
  #pragma mta block schedule
  for (int i = 0; i < MAX_LOOP; ++i)
  {
    data = xht[i];
  }
  #pragma mta fence
  timer.stop();

  printf("            Lookup via []: %9.6lf        %d\n",
         timer.getElapsedSeconds(), data);

  #pragma mta fence
  timer.start();
  int sum = 0;
  #pragma mta assert parallel
  for (int i = 0; i < MAX_LOOP; ++i)
  {
    sum += xht[i];
  }
  #pragma mta fence
  timer.stop();

  printf("                 [] Usage: %9.6lf        %d\n",
         timer.getElapsedSeconds(), sum);

/*
  // This is the case that causes deadlock for the [] operator.
  timer.start();
  #pragma mta assert parallel
  for (int i = 0; i < MAX_LOOP; ++i)
  {
    xht[i] = xht[i] + 4;
  }
  timer.stop();

  printf("         Usage and lookup: %9.6lf\n", timer.getElapsedSeconds());
*/
#endif

  #pragma mta fence
  timer.start();
  #pragma mta block schedule
  #pragma mta assert nodep
  for (int i = 0; i < MAX_LOOP; ++i)
  {
//    if (!xht.update(i, i * 2))  printf("Couldn't update %d.\n", i);
    xht.update(i, i * 2);
  }
  #pragma mta fence
  timer.stop();

  printf("                   Update: %9.6lf\n", timer.getElapsedSeconds());

  #pragma mta fence
  timer.start();
  #pragma mta block schedule
  #pragma mta assert nodep
  for (int i = 0; i < MAX_LOOP; ++i)
  {
//    if (!xht.update(i, i * 2))  printf("Couldn't update %d.\n", i);
    xht.update(i, 2, uv1);
  }
  #pragma mta fence
  timer.stop();

  printf("           Update-mt_incr: %9.6lf\n", timer.getElapsedSeconds());

  #pragma mta fence
  timer.start();
  #pragma mta block schedule
  #pragma mta assert parallel
  for (int i = 0; i < MAX_LOOP; ++i)
  {
    xht.update_insert(i, i * 3);
  }
  #pragma mta fence
  timer.stop();

  printf("            Update-Insert: %9.6lf\n", timer.getElapsedSeconds());

  #pragma mta fence
  timer.start();
  #pragma mta block schedule
  #pragma mta assert parallel
  for (int i = 0; i < MAX_LOOP; ++i)
  {
    xht.update_insert(i, 3, uv1);
  }
  #pragma mta fence
  timer.stop();

  printf("    Update-Insert-mt_incr: %9.6lf\n", timer.getElapsedSeconds());

#if 0
  #pragma mta fence
  timer.start();
  #pragma mta assert parallel
  for (int i = 0; i < MAX_LOOP; ++i)
  {
    xht[i] = i * 2;
  }
  #pragma mta fence
  timer.stop();

  printf("            Update via []: %9.6lf\n", timer.getElapsedSeconds());

#endif

  #pragma mta fence
  timer.start();
  xht.resize(MAX_LOOP);
  #pragma mta fence
  timer.stop();

  printf("              Full Resize: %9.6lf\n", timer.getElapsedSeconds());

//  if (xht.lookup(20, data))  printf("Found: 20 -> %d\n", data);

  #pragma mta fence
  timer.start();
  #pragma mta block schedule
  #pragma mta assert parallel
  for (int i = 0; i < MAX_LOOP; i += 2)
  {
//    if (!xht.erase(i))  printf("Didn't find %d.\n", i);
    xht.erase(i);
  }
  #pragma mta fence
  timer.stop();

  printf("                    Erase: %9.6lf\n", timer.getElapsedSeconds());

//  if (xht.lookup(20, data))  printf("Found: 20 -> %d\n", data);
//  else  printf("Value 20 not found.\n");
//  if (xht.lookup(139, data))  printf("Found: 139 -> %d\n", data);
//  else  printf("Value 139 not found.\n");

  printf("\n");
  printf("             Num Elements: %9lu\n", xht.size());
  printf("               capacity(): %9lu\n\n", xht.capacity());

  return 0;
}

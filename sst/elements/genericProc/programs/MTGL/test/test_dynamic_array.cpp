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
/*! \file test_dynamic_array.cpp

    \brief Tests the functionality of the dynamic_array.

    \author Greg Mackey (gemacke@sandia.gov)

    \date 4/13/2010
*/
/****************************************************************************/

#include <mtgl/dynamic_array.hpp>
#include <mtgl/util.hpp>

using namespace mtgl;

#define MAX_LOOP 100000000
#define FIRST_LOOP 100000
#define SECOND_LOOP (MAX_LOOP - FIRST_LOOP)

int main()
{
  int sum;

  mt_timer timer;

  #pragma mta fence
  timer.start();
  dynamic_array<int> darr(FIRST_LOOP + 1);
  #pragma mta fence
  timer.stop();

  printf("                 MAX_LOOP: %9ld\n", MAX_LOOP);
  printf("               FIRST_LOOP: %9ld\n", FIRST_LOOP);
  printf("              SECOND_LOOP: %9ld\n", SECOND_LOOP);
  printf("               capacity(): %9lu\n", darr.capacity());
  printf("                   size(): %9lu\n\n", darr.size());

  printf("           Initialization: %9.6lf\n", timer.getElapsedSeconds());

  #pragma mta fence
  timer.start();

  for (int i = 0; i < FIRST_LOOP; ++i) darr.push_back(i);

  #pragma mta fence
  timer.stop();

  printf("                      Add: %9.6lf\n", timer.getElapsedSeconds());

  #pragma mta fence
  timer.start();

  darr.reserve(MAX_LOOP + 1);

  #pragma mta fence
  timer.stop();

  printf("                  Reserve: %9.6lf\n\n", timer.getElapsedSeconds());
  printf("               capacity(): %9lu\n", darr.capacity());
  printf("                   size(): %9lu\n\n", darr.size());

  // Declare a temporary array.
  int* temp_arr = (int*) malloc(SECOND_LOOP * sizeof(int));
  for (int i = 0; i < SECOND_LOOP; ++i) temp_arr[i] = i + FIRST_LOOP;

  #pragma mta fence
  timer.start();

  darr.push_back(temp_arr, SECOND_LOOP);

  #pragma mta fence
  timer.stop();

  printf("                Array Add: %9.6lf\n", timer.getElapsedSeconds());

  int* data = &darr[0];

  #pragma mta fence
  timer.start();

  sum = 0;
  #pragma mta block schedule
  for (int i = 0; i < MAX_LOOP; ++i) sum += data[i];

  #pragma mta fence
  timer.stop();

  printf("                Array Sum: %9.6lf        %d\n",
         timer.getElapsedSeconds(), sum);

  #pragma mta fence
  timer.start();

  sum = 0;
  #pragma mta block schedule
  for (int i = 0; i < MAX_LOOP; ++i) sum += darr[i];

  #pragma mta fence
  timer.stop();

  printf("                 [] Usage: %9.6lf        %d\n",
         timer.getElapsedSeconds(), sum);

  printf("\n");
  printf("               capacity(): %9lu\n", darr.capacity());
  printf("                   size(): %9lu\n\n", darr.size());

  #pragma mta fence
  timer.start();

  for (int i = 0; i < FIRST_LOOP; ++i) darr.pop_back();

  #pragma mta fence
  timer.stop();

  printf("               pop_back(): %9.6lf\n", timer.getElapsedSeconds());

  printf("\n");
  printf("               capacity(): %9lu\n", darr.capacity());
  printf("                   size(): %9lu\n\n", darr.size());

  #pragma mta fence
  timer.start();

  darr.clear();

  #pragma mta fence
  timer.stop();

  printf("                    Clear: %9.6lf\n", timer.getElapsedSeconds());

  printf("\n");
  printf("               capacity(): %9lu\n", darr.capacity());
  printf("                   size(): %9lu\n\n", darr.size());

  return 0;
}
